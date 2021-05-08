#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_forms.py

===============================================================================

    Copyright (C) 2012-2020 Rudolf Cardinal (rudolf@pobox.com).

    This file is part of CamCOPS.

    CamCOPS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CamCOPS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CamCOPS. If not, see <https://www.gnu.org/licenses/>.

===============================================================================

**Forms for use by the web front end.**

*COLANDER NODES, NULLS, AND VALIDATION*

- Surprisingly tricky.
- Nodes must be validly intialized with NO USER-DEFINED PARAMETERS to __init__;
  the Deform framework clones them.
- A null appstruct is used to initialize nodes as Forms are created.
  Therefore, the "default" value must be acceptable to the underlying type's
  serialize() function. Note in particular that "default = None" is not
  acceptable to Integer. Having no default is fine, though.
- In general, flexible inheritance is very hard to implement.

- Note that this error:

  .. code-block:: none

    AttributeError: 'EditTaskFilterSchema' object has no attribute 'typ'

  means you have failed to call super().__init__() properly from __init__().

- When creating a schema, its members seem to have to be created in the class
  declaration as class properties, not in __init__().

*ACCESSING THE PYRAMID REQUEST IN FORMS AND SCHEMAS*

We often want to be able to access the request for translation purposes, or
sometimes more specialized reasons.

Forms are created dynamically as simple Python objects. So, for a
:class:`deform.form.Form`, just add a ``request`` parameter to the constructor,
and pass it when you create the form. An example is
:class:`camcops_server.cc_modules.cc_forms.DeleteCancelForm`.

For a :class:`colander.Schema` and :class:`colander.SchemaNode`, construction
is separate from binding. The schema nodes are created as part of a schema
class, not a schema instance. The schema is created by the form, and then bound
to a request. Access to the request is therefore via the :func:`after_bind`
callback function, offered by colander, via the ``kw`` parameter or
``self.bindings``. We use ``Binding.REQUEST`` as a standard key for this
dictionary. The bindings are also available in :func:`validator` and similar
functions, as ``self.bindings``.

All forms containing any schema that needs to see the request should have this
sort of ``__init__`` function:

.. code-block:: python

    class SomeForm(...):
        def __init__(...):
            schema = schema_class().bind(request=request)
            super().__init__(
                schema,
                ...,
                **kwargs
            )

The simplest thing, therefore, is for all forms to do this. Some of our forms
use a form superclass that does this via the ``schema_class`` argument (which
is not part of colander, so if you see that, the superclass should do the work
of binding a request).

For translation, throughout there will be ``_ = self.gettext`` or ``_ =
request.gettext``.

Form titles need to be dynamically written via
:class:`cardinal_pythonlib.deform_utils.DynamicDescriptionsForm` or similar.

"""

import json
import logging
import os
from typing import (Any, Callable, Dict, List, Optional,
                    Tuple, Type, TYPE_CHECKING, Union)

from cardinal_pythonlib.colander_utils import (
    AllowNoneType,
    BooleanNode,
    DateSelectorNode,
    DateTimeSelectorNode,
    DEFAULT_WIDGET_DATE_OPTIONS_FOR_PENDULUM,
    DEFAULT_WIDGET_TIME_OPTIONS_FOR_PENDULUM,
    get_child_node,
    get_values_and_permissible,
    HiddenIntegerNode,
    HiddenStringNode,
    MandatoryStringNode,
    OptionalEmailNode,
    OptionalIntNode,
    OptionalPendulumNode,
    OptionalStringNode,
    ValidateDangerousOperationNode,
)
from cardinal_pythonlib.deform_utils import (
    DynamicDescriptionsForm,
    InformativeForm,
)
from cardinal_pythonlib.logs import (
    BraceStyleAdapter,
)
from cardinal_pythonlib.sqlalchemy.dialect import SqlaDialectName
from cardinal_pythonlib.sqlalchemy.orm_query import CountStarSpecializedQuery
# noinspection PyProtectedMember
from colander import (
    Boolean,
    Date,
    drop,
    Integer,
    Invalid,
    Length,
    MappingSchema,
    null,
    OneOf,
    Range,
    Schema,
    SchemaNode,
    SchemaType,
    SequenceSchema,
    Set,
    String,
    _null,
)
from deform.form import Button
from deform.widget import (
    CheckboxChoiceWidget,
    CheckedPasswordWidget,
    # DateInputWidget,
    DateTimeInputWidget,
    FormWidget,
    HiddenWidget,
    MappingWidget,
    PasswordWidget,
    RadioChoiceWidget,
    SelectWidget,
    SequenceWidget,
    TextAreaWidget,
    TextInputWidget,
    Widget,
)

from pendulum import Duration

# import as LITTLE AS POSSIBLE; this is used by lots of modules
# We use some delayed imports here (search for "delayed import")
from camcops_server.cc_modules.cc_baseconstants import (
    DEFORM_SUPPORTS_CSP_NONCE,
    TEMPLATE_DIR,
)
from camcops_server.cc_modules.cc_constants import (
    ConfigParamSite,
    DEFAULT_ROWS_PER_PAGE,
    SEX_OTHER_UNSPECIFIED,
    SEX_FEMALE,
    SEX_MALE,
    StringLengths,
    USER_NAME_FOR_SYSTEM,
)
from camcops_server.cc_modules.cc_group import Group
from camcops_server.cc_modules.cc_idnumdef import (
    IdNumDefinition,
    ID_NUM_VALIDATION_METHOD_CHOICES,
    validate_id_number,
)
from camcops_server.cc_modules.cc_ipuse import IpUse
from camcops_server.cc_modules.cc_language import (
    DEFAULT_LOCALE,
    POSSIBLE_LOCALES,
    POSSIBLE_LOCALES_WITH_DESCRIPTIONS,
)
from camcops_server.cc_modules.cc_patient import Patient
from camcops_server.cc_modules.cc_patientidnum import PatientIdNum
from camcops_server.cc_modules.cc_policy import (
    TABLET_ID_POLICY_STR,
    TokenizedPolicy,
)
from camcops_server.cc_modules.cc_pyramid import (
    FormAction,
    RequestMethod,
    ViewArg,
    ViewParam,
)
from camcops_server.cc_modules.cc_task import tablename_to_task_class_dict
from camcops_server.cc_modules.cc_taskschedule import (
    TaskSchedule,
    TaskScheduleEmailTemplateFormatter,
)
from camcops_server.cc_modules.cc_validators import (
    ALPHANUM_UNDERSCORE_CHAR,
    validate_anything,
    validate_by_char_and_length,
    validate_group_name,
    validate_hl7_aa,
    validate_hl7_id_type,
    validate_ip_address,
    validate_new_password,
    validate_redirect_url,
    validate_username,
)

if TYPE_CHECKING:
    from deform.field import Field
    from camcops_server.cc_modules.cc_request import CamcopsRequest
    from camcops_server.cc_modules.cc_task import Task
    from camcops_server.cc_modules.cc_user import User

log = BraceStyleAdapter(logging.getLogger(__name__))

ColanderNullType = _null
ValidatorType = Callable[[SchemaNode, Any], None]  # called as v(node, value)


# =============================================================================
# Debugging options
# =============================================================================

DEBUG_CSRF_CHECK = False

if DEBUG_CSRF_CHECK:
    log.warning("Debugging options enabled!")


# =============================================================================
# Constants
# =============================================================================

class Binding(object):
    """
    Keys used for binding dictionaries with Colander schemas (schemata).

    Must match ``kwargs`` of calls to ``bind()`` function of each ``Schema``.
    """
    GROUP = "group"
    OPEN_ADMIN = "open_admin"
    OPEN_WHAT = "open_what"
    OPEN_WHEN = "open_when"
    OPEN_WHO = "open_who"
    REQUEST = "request"
    TRACKER_TASKS_ONLY = "tracker_tasks_only"
    USER = "user"


class BootstrapCssClasses(object):
    """
    Constants from Bootstrap to control display.
    """
    FORM_INLINE = "form-inline"
    RADIO_INLINE = "radio-inline"
    LIST_INLINE = "list-inline"
    CHECKBOX_INLINE = "checkbox-inline"


AUTOCOMPLETE_ATTR = "autocomplete"


class AutocompleteAttrValues(object):
    """
    Some values for the HTML "autocomplete" attribute, as per
    https://developer.mozilla.org/en-US/docs/Web/HTML/Attributes/autocomplete.
    Not all are used.
    """
    BDAY = "bday"
    CURRENT_PASSWORD = "current-password"
    EMAIL = "email"
    FAMILY_NAME = "family-name"
    GIVEN_NAME = "given-name"
    NEW_PASSWORD = "new-password"
    OFF = "off"
    ON = "on"  # browser decides
    STREET_ADDRESS = "stree-address"
    USERNAME = "username"


# =============================================================================
# Common phrases for translation
# =============================================================================

def or_join_description(request: "CamcopsRequest") -> str:
    _ = request.gettext
    return _("If you specify more than one, they will be joined with OR.")


def change_password_title(request: "CamcopsRequest") -> str:
    _ = request.gettext
    return _("Change password")


def sex_choices(request: "CamcopsRequest") -> List[Tuple[str, str]]:
    _ = request.gettext
    return [
        (SEX_FEMALE, _("Female (F)")),
        (SEX_MALE, _("Male (M)")),
        # TRANSLATOR: sex code description
        (SEX_OTHER_UNSPECIFIED, _("Other/unspecified (X)")),
    ]


# =============================================================================
# Deform bug fix: SelectWidget "multiple" attribute
# =============================================================================

class BugfixSelectWidget(SelectWidget):
    """
    Fixes a bug where newer versions of Chameleon (e.g. 3.8.0) render Deform's
    ``multiple = False`` (in ``SelectWidget``) as this, which is wrong:

    .. code-block:: none

        <select name="which_idnum" id="deformField2" class=" form-control " multiple="False">
                                                                            ^^^^^^^^^^^^^^^^
            <option value="1">CPFT RiO number</option>
            <option value="2">NHS number</option>
            <option value="1000">MyHospital number</option>
        </select>

    ... whereas previous versions of Chameleon (e.g. 3.4) omitted the tag.
    (I think it's a Chameleon change, anyway! And it's probably a bugfix in
    Chameleon that exposed a bug in Deform.)

    See :func:`camcops_server.cc_modules.webview.debug_form_rendering`.
    """  # noqa
    def __init__(self, multiple=False, **kwargs) -> None:
        multiple = True if multiple else None  # None, not False
        super().__init__(multiple=multiple, **kwargs)


SelectWidget = BugfixSelectWidget


# =============================================================================
# Form that handles Content-Security-Policy nonce tags
# =============================================================================

class InformativeNonceForm(InformativeForm):
    """
    A Form class to use our modifications to Deform, as per
    https://github.com/Pylons/deform/issues/512, to pass a nonce value through
    to the ``<script>`` and ``<style>`` tags in the Deform templates.

    todo: if Deform is updated, work this into ``cardinal_pythonlib``.
    """
    if DEFORM_SUPPORTS_CSP_NONCE:
        def __init__(self, schema: Schema, **kwargs) -> None:
            request = schema.request  # type: CamcopsRequest
            kwargs["nonce"] = request.nonce
            super().__init__(schema, **kwargs)


class DynamicDescriptionsNonceForm(DynamicDescriptionsForm):
    """
    Similarly; see :class:`InformativeNonceForm`.

    todo: if Deform is updated, work this into ``cardinal_pythonlib``.
    """
    if DEFORM_SUPPORTS_CSP_NONCE:
        def __init__(self, schema: Schema, **kwargs) -> None:
            request = schema.request  # type: CamcopsRequest
            kwargs["nonce"] = request.nonce
            super().__init__(schema, **kwargs)


# =============================================================================
# Mixin for Schema/SchemaNode objects for translation
# =============================================================================

GETTEXT_TYPE = Callable[[str], str]


class RequestAwareMixin(object):
    """
    Mixin to add Pyramid request awareness to Schema/SchemaNode objects,
    together with some translations and other convenience functions.
    """
    def __init__(self, *args, **kwargs) -> None:
        # Stop multiple inheritance complaints
        super().__init__(*args, **kwargs)

    # noinspection PyUnresolvedReferences
    @property
    def request(self) -> "CamcopsRequest":
        return self.bindings[Binding.REQUEST]

    # noinspection PyUnresolvedReferences,PyPropertyDefinition
    @property
    def gettext(self) -> GETTEXT_TYPE:
        return self.request.gettext

    @property
    def or_join_description(self) -> str:
        return or_join_description(self.request)


# =============================================================================
# Translatable version of ValidateDangerousOperationNode
# =============================================================================

class TranslatableValidateDangerousOperationNode(
        ValidateDangerousOperationNode, RequestAwareMixin):
    """
    Translatable version of ValidateDangerousOperationNode.
    """
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        super().after_bind(node, kw)  # calls set_description()
        _ = self.gettext
        node.title = _("Danger")
        user_entry = get_child_node(self, "user_entry")
        user_entry.title = _("Validate this dangerous operation")

    def set_description(self, target_value: str) -> None:
        # Overrides parent version (q.v.).
        _ = self.gettext
        user_entry = get_child_node(self, "user_entry")
        prefix = _("Please enter the following: ")
        user_entry.description = prefix + target_value


# =============================================================================
# Translatable version of SequenceWidget
# =============================================================================

class TranslatableSequenceWidget(SequenceWidget):
    """
    SequenceWidget does support translation via _(), but not in a
    request-specific way.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        super().__init__(**kwargs)
        _ = request.gettext
        self.add_subitem_text_template = _('Add') + ' ${subitem_title}'


# =============================================================================
# Translatable version of OptionalPendulumNode
# =============================================================================

class TranslatableOptionalPendulumNode(OptionalPendulumNode,
                                       RequestAwareMixin):
    """
    Translates the "Date" and "Time" labels for the widget, via
    the request.

    .. todo:: TranslatableOptionalPendulumNode not fully implemented
    """
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.widget = None  # type: Optional[Widget]

    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.widget = DateTimeInputWidget(
            date_options=DEFAULT_WIDGET_DATE_OPTIONS_FOR_PENDULUM,
            time_options=DEFAULT_WIDGET_TIME_OPTIONS_FOR_PENDULUM
        )
        # log.critical("TranslatableOptionalPendulumNode.widget: {!r}",
        #              self.widget.__dict__)


class TranslatableDateTimeSelectorNode(DateTimeSelectorNode,
                                       RequestAwareMixin):
    """
    Translates the "Date" and "Time" labels for the widget, via
    the request.

    .. todo:: TranslatableDateTimeSelectorNode not fully implemented
    """
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.widget = None  # type: Optional[Widget]

    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.widget = DateTimeInputWidget()
        # log.critical("TranslatableDateTimeSelectorNode.widget: {!r}",
        #              self.widget.__dict__)


'''
class TranslatableDateSelectorNode(DateSelectorNode,
                                   RequestAwareMixin):
    """
    Translates the "Date" and "Time" labels for the widget, via
    the request.

    .. todo:: TranslatableDateSelectorNode not fully implemented
    """
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.widget = None  # type: Optional[Widget]

    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.widget = DateInputWidget()
        # log.critical("TranslatableDateSelectorNode.widget: {!r}",
        #              self.widget.__dict__)
'''


# =============================================================================
# CSRF
# =============================================================================

class CSRFToken(SchemaNode, RequestAwareMixin):
    """
    Node to embed a cross-site request forgery (CSRF) prevention token in a
    form.

    As per https://deformdemo.repoze.org/pyramid_csrf_demo/, modified for a
    more recent Colander API.

    NOTE that this makes use of colander.SchemaNode.bind; this CLONES the
    Schema, and resolves any deferred values by means of the keywords passed to
    bind(). Since the Schema is created at module load time, but since we're
    asking the Schema to know about the request's CSRF values, this is the only
    mechanism
    (https://docs.pylonsproject.org/projects/colander/en/latest/api.html#colander.SchemaNode.bind).

    From https://deform2000.readthedocs.io/en/latest/basics.html:

    "The default of a schema node indicates the value to be serialized if a
    value for the schema node is not found in the input data during
    serialization. It should be the deserialized representation. If a schema
    node does not have a default, it is considered "serialization required"."

    "The missing of a schema node indicates the value to be deserialized if a
    value for the schema node is not found in the input data during
    deserialization. It should be the deserialized representation. If a schema
    node does not have a missing value, a colander.Invalid exception will be
    raised if the data structure being deserialized does not contain a matching
    value."

    RNC: Serialized values are always STRINGS.

    """  # noqa
    schema_type = String
    default = ""
    missing = ""
    title = " "
    # ... evaluates to True but won't be visible, if the "hidden" aspect ever
    # fails
    widget = HiddenWidget()

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        request = self.request
        csrf_token = request.session.get_csrf_token()
        if DEBUG_CSRF_CHECK:
            log.debug("Got CSRF token from session: {!r}", csrf_token)
        self.default = csrf_token

    def validator(self, node: SchemaNode, value: Any) -> None:
        # Deferred validator via method, as per
        # https://docs.pylonsproject.org/projects/colander/en/latest/basics.html  # noqa
        request = self.request
        csrf_token = request.session.get_csrf_token()  # type: str
        matches = value == csrf_token
        if DEBUG_CSRF_CHECK:
            log.debug("Validating CSRF token: form says {!r}, session says "
                      "{!r}, matches = {}", value, csrf_token, matches)
        if not matches:
            log.warning("CSRF token mismatch; remote address {}",
                        request.remote_addr)
            _ = request.gettext
            raise Invalid(node, _("Bad CSRF token"))


class CSRFSchema(Schema, RequestAwareMixin):
    """
    Base class for form schemas that use CSRF (XSRF; cross-site request
    forgery) tokens.

    You can't put the call to ``bind()`` at the end of ``__init__()``, because
    ``bind()`` calls ``clone()`` with no arguments and ``clone()`` ends up
    calling ``__init__()```...

    The item name should be one that the ZAP penetration testing tool expects,
    or you get:

    .. code-block:: none

        No known Anti-CSRF token [anticsrf, CSRFToken,
        __RequestVerificationToken, csrfmiddlewaretoken, authenticity_token,
        OWASP_CSRFTOKEN, anoncsrf, csrf_token, _csrf, _csrfSecret] was found in
        the following HTML form: [Form 1: "_charset_" "__formid__"
        "deformField1" "deformField2" "deformField3" "deformField4" ].

    """
    csrf_token = CSRFToken()  # name must match ViewParam.CSRF_TOKEN
    # ... name should also be one that ZAP expects, as above


# =============================================================================
# Horizontal forms
# =============================================================================

class HorizontalFormWidget(FormWidget):
    """
    Widget to render a form horizontally, with custom templates.

    See :class:`deform.template.ZPTRendererFactory`, which explains how strings
    are resolved to Chameleon ZPT (Zope) templates.

    See

    - https://stackoverflow.com/questions/12201835/form-inline-inside-a-form-horizontal-in-twitter-bootstrap
    - https://stackoverflow.com/questions/18429121/inline-form-nested-within-horizontal-form-in-bootstrap-3
    - https://stackoverflow.com/questions/23954772/how-to-make-a-horizontal-form-with-deform-2
    """  # noqa
    basedir = os.path.join(TEMPLATE_DIR, "deform")
    readonlydir = os.path.join(basedir, "readonly")
    form = "horizontal_form.pt"
    mapping_item = "horizontal_mapping_item.pt"

    template = os.path.join(basedir, form)  # default "form" = deform/templates/form.pt  # noqa
    readonly_template = os.path.join(readonlydir, form)  # default "readonly/form"  # noqa
    item_template = os.path.join(basedir, mapping_item)  # default "mapping_item"  # noqa
    readonly_item_template = os.path.join(readonlydir, mapping_item)  # default "readonly/mapping_item"  # noqa


class HorizontalFormMixin(object):
    """
    Modification to a Deform form that displays itself with horizontal layout,
    using custom templates via :class:`HorizontalFormWidget`. Not fantastic.
    """
    def __init__(self, schema: Schema, *args, **kwargs) -> None:
        kwargs = kwargs or {}

        # METHOD 1: add "form-inline" to the CSS classes.
        # extra_classes = "form-inline"
        # if "css_class" in kwargs:
        #     kwargs["css_class"] += " " + extra_classes
        # else:
        #     kwargs["css_class"] = extra_classes

        # Method 2: change the widget
        schema.widget = HorizontalFormWidget()

        # OK, proceed.
        super().__init__(schema, *args, **kwargs)


def add_css_class(kwargs: Dict[str, Any],
                  extra_classes: str,
                  param_name: str = "css_class") -> None:
    """
    Modifies a kwargs dictionary to add a CSS class to the ``css_class``
    parameter.

    Args:
        kwargs: a dictionary
        extra_classes: CSS classes to add (as a space-separated string)
        param_name: parameter name to modify; by default, "css_class"
    """
    if param_name in kwargs:
        kwargs[param_name] += " " + extra_classes
    else:
        kwargs[param_name] = extra_classes


class FormInlineCssMixin(object):
    """
    Modification to a Deform form that makes it display "inline" via CSS. This
    has the effect of wrapping everything horizontally.

    Should PRECEDE the :class:`Form` (or something derived from it) in the
    inheritance order.
    """
    def __init__(self, *args, **kwargs) -> None:
        kwargs = kwargs or {}
        add_css_class(kwargs, BootstrapCssClasses.FORM_INLINE)
        super().__init__(*args, **kwargs)


def make_widget_horizontal(widget: Widget) -> None:
    """
    Applies Bootstrap "form-inline" styling to the widget.
    """
    widget.item_css_class = BootstrapCssClasses.FORM_INLINE


def make_node_widget_horizontal(node: SchemaNode) -> None:
    """
    Applies Bootstrap "form-inline" styling to the schema node's widget.

    **Note:** often better to use the ``inline=True`` option to the widget's
    constructor.
    """
    make_widget_horizontal(node.widget)


# =============================================================================
# Specialized Form classes
# =============================================================================

class SimpleSubmitForm(InformativeNonceForm):
    """
    Form with a simple "submit" button.
    """
    def __init__(self,
                 schema_class: Type[Schema],
                 submit_title: str,
                 request: "CamcopsRequest",
                 **kwargs) -> None:
        """
        Args:
            schema_class:
                class of the Colander :class:`Schema` to use as this form's
                schema
            submit_title:
                title (text) to be used for the "submit" button
            request:
                :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        """
        schema = schema_class().bind(request=request)
        super().__init__(
            schema,
            buttons=[Button(name=FormAction.SUBMIT,
                            title=submit_title)],
            **kwargs
        )


class ApplyCancelForm(InformativeNonceForm):
    """
    Form with "apply" and "cancel" buttons.
    """
    def __init__(self,
                 schema_class: Type[Schema],
                 request: "CamcopsRequest",
                 **kwargs) -> None:
        schema = schema_class().bind(request=request)
        _ = request.gettext
        super().__init__(
            schema,
            buttons=[
                Button(name=FormAction.SUBMIT, title=_("Apply")),
                Button(name=FormAction.CANCEL, title=_("Cancel")),
            ],
            **kwargs
        )


class AddCancelForm(InformativeNonceForm):
    """
    Form with "add" and "cancel" buttons.
    """
    def __init__(self,
                 schema_class: Type[Schema],
                 request: "CamcopsRequest",
                 **kwargs) -> None:
        schema = schema_class().bind(request=request)
        _ = request.gettext
        super().__init__(
            schema,
            buttons=[
                Button(name=FormAction.SUBMIT, title=_("Add")),
                Button(name=FormAction.CANCEL, title=_("Cancel")),
            ],
            **kwargs
        )


class DangerousForm(DynamicDescriptionsNonceForm):
    """
    Form with one "submit" button (with user-specifiable title text and action
    name), in a CSS class indicating that it's a dangerous operation, plus a
    "Cancel" button.
    """
    def __init__(self,
                 schema_class: Type[Schema],
                 submit_action: str,
                 submit_title: str,
                 request: "CamcopsRequest",
                 **kwargs) -> None:
        schema = schema_class().bind(request=request)
        _ = request.gettext
        super().__init__(
            schema,
            buttons=[
                Button(name=submit_action, title=submit_title,
                       css_class="btn-danger"),
                Button(name=FormAction.CANCEL, title=_("Cancel")),
            ],
            **kwargs
        )


class DeleteCancelForm(DangerousForm):
    """
    Form with a "delete" button (visually marked as dangerous) and a "cancel"
    button.
    """
    def __init__(self,
                 schema_class: Type[Schema],
                 request: "CamcopsRequest",
                 **kwargs) -> None:
        _ = request.gettext
        super().__init__(
            schema_class=schema_class,
            submit_action=FormAction.DELETE,
            submit_title=_("Delete"),
            request=request,
            **kwargs
        )


# =============================================================================
# Specialized SchemaNode classes used in several contexts
# =============================================================================

# -----------------------------------------------------------------------------
# Task types
# -----------------------------------------------------------------------------

class OptionalSingleTaskSelector(OptionalStringNode, RequestAwareMixin):
    """
    Node to pick one task type.
    """
    def __init__(self, *args, tracker_tasks_only: bool = False,
                 **kwargs) -> None:
        """
        Args:
            tracker_tasks_only: restrict the choices to tasks that offer
                trackers.
        """
        self.title = ""  # for type checker
        self.tracker_tasks_only = tracker_tasks_only
        self.widget = None  # type: Optional[Widget]
        self.validator = None  # type: Optional[ValidatorType]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Task type")
        if Binding.TRACKER_TASKS_ONLY in kw:
            self.tracker_tasks_only = kw[Binding.TRACKER_TASKS_ONLY]
        values, pv = get_values_and_permissible(self.get_task_choices(),
                                                True, _("[Any]"))
        self.widget = SelectWidget(values=values)
        self.validator = OneOf(pv)

    def get_task_choices(self) -> List[Tuple[str, str]]:
        from camcops_server.cc_modules.cc_task import Task  # delayed import
        choices = []  # type: List[Tuple[str, str]]
        for tc in Task.all_subclasses_by_shortname():
            if self.tracker_tasks_only and not tc.provides_trackers:
                continue
            choices.append((tc.tablename, tc.shortname))
        return choices


class MandatorySingleTaskSelector(MandatoryStringNode, RequestAwareMixin):
    """
    Node to pick one task type.
    """
    def __init__(self, *args: Any, **kwargs: Any) -> None:
        self.title = ""  # for type checker
        self.widget = None  # type: Optional[Widget]
        self.validator = None  # type: Optional[ValidatorType]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Task type")
        values, pv = get_values_and_permissible(self.get_task_choices(), False)
        self.widget = SelectWidget(values=values)
        self.validator = OneOf(pv)

    @staticmethod
    def get_task_choices() -> List[Tuple[str, str]]:
        from camcops_server.cc_modules.cc_task import Task  # delayed import
        choices = []  # type: List[Tuple[str, str]]
        for tc in Task.all_subclasses_by_shortname():
            choices.append((tc.tablename, tc.shortname))
        return choices


class MultiTaskSelector(SchemaNode, RequestAwareMixin):
    """
    Node to select multiple task types.
    """
    schema_type = Set
    default = ""
    missing = ""

    def __init__(self, *args, tracker_tasks_only: bool = False,
                 minimum_number: int = 0, **kwargs) -> None:
        self.tracker_tasks_only = tracker_tasks_only
        self.minimum_number = minimum_number
        self.widget = None  # type: Optional[Widget]
        self.validator = None  # type: Optional[ValidatorType]
        self.title = ""  # for type checker
        self.description = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        request = self.request  # noqa: F841
        self.title = _("Task type(s)")
        self.description = (
            _("If none are selected, all task types will be offered.") +
            " " + self.or_join_description
        )
        if Binding.TRACKER_TASKS_ONLY in kw:
            self.tracker_tasks_only = kw[Binding.TRACKER_TASKS_ONLY]
        values, pv = get_values_and_permissible(self.get_task_choices())
        self.widget = CheckboxChoiceWidget(values=values, inline=True)
        self.validator = Length(min=self.minimum_number)

    def get_task_choices(self) -> List[Tuple[str, str]]:
        from camcops_server.cc_modules.cc_task import Task  # delayed import
        choices = []  # type: List[Tuple[str, str]]
        for tc in Task.all_subclasses_by_shortname():
            if self.tracker_tasks_only and not tc.provides_trackers:
                continue
            choices.append((tc.tablename, tc.shortname))
        return choices


# -----------------------------------------------------------------------------
# Use the task index?
# -----------------------------------------------------------------------------

class ViaIndexSelector(BooleanNode, RequestAwareMixin):
    """
    Node to choose whether we use the server index or not.
    Default is true.
    """
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, default=True, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Use server index?")
        self.label = _("Use server index? (Default is true; much faster.)")


# -----------------------------------------------------------------------------
# ID numbers
# -----------------------------------------------------------------------------

class MandatoryWhichIdNumSelector(SchemaNode, RequestAwareMixin):
    """
    Node to enforce the choice of a single ID number type (e.g. "NHS number"
    or "study Blah ID number").
    """
    widget = SelectWidget()

    def __init__(self, *args, **kwargs) -> None:
        if not hasattr(self, "allow_none"):
            # ... allows parameter-free (!) inheritance by OptionalWhichIdNumSelector  # noqa
            self.allow_none = False
        self.title = ""  # for type checker
        self.description = ""  # for type checker
        self.validator = None  # type: Optional[ValidatorType]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        request = self.request
        _ = request.gettext
        self.title = _("Identifier")
        values = []  # type: List[Tuple[Optional[int], str]]
        for iddef in request.idnum_definitions:
            values.append((iddef.which_idnum, iddef.description))
        values, pv = get_values_and_permissible(values, self.allow_none,
                                                _("[ignore]"))
        # ... can't use None, because SelectWidget() will convert that to
        # "None"; can't use colander.null, because that converts to
        # "<colander.null>"; use "", which is the default null_value of
        # SelectWidget.
        self.widget.values = values
        self.validator = OneOf(pv)

    @staticmethod
    def schema_type() -> SchemaType:
        return Integer()


class LinkingIdNumSelector(MandatoryWhichIdNumSelector):
    """
    Convenience node: pick a single ID number, with title/description
    indicating that it's the ID number to link on.
    """

    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        super().after_bind(node, kw)
        _ = self.gettext
        self.title = _("Linking ID number")
        self.description = _("Which ID number to link on?")


class MandatoryIdNumValue(SchemaNode, RequestAwareMixin):
    """
    Mandatory node to capture an ID number value.
    """
    schema_type = Integer
    validator = Range(min=0)

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("ID# value")


class MandatoryIdNumNode(MappingSchema, RequestAwareMixin):
    """
    Mandatory node to capture an ID number type and the associated actual
    ID number (value).

    This is also where we apply ID number validation rules (e.g. NHS number).
    """
    which_idnum = MandatoryWhichIdNumSelector()  # must match ViewParam.WHICH_IDNUM  # noqa
    idnum_value = MandatoryIdNumValue()  # must match ViewParam.IDNUM_VALUE

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("ID number")

    # noinspection PyMethodMayBeStatic
    def validator(self, node: SchemaNode, value: Dict[str, int]) -> None:
        assert isinstance(value, dict)
        req = self.request
        _ = req.gettext
        which_idnum = value[ViewParam.WHICH_IDNUM]
        idnum_value = value[ViewParam.IDNUM_VALUE]
        idnum_def = req.get_idnum_definition(which_idnum)
        if not idnum_def:
            raise Invalid(node, _("Bad ID number type"))  # shouldn't happen
        method = idnum_def.validation_method
        if method:
            valid, why_invalid = validate_id_number(req, idnum_value, method)
            if not valid:
                raise Invalid(node, why_invalid)


class IdNumSequenceAnyCombination(SequenceSchema, RequestAwareMixin):
    """
    Sequence to capture multiple ID numbers (as type/value pairs).
    """
    idnum_sequence = MandatoryIdNumNode()

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("ID numbers")
        self.widget = TranslatableSequenceWidget(request=self.request)

    # noinspection PyMethodMayBeStatic
    def validator(self, node: SchemaNode, value: List[Dict[str, int]]) -> None:
        assert isinstance(value, list)
        list_of_lists = [(x[ViewParam.WHICH_IDNUM], x[ViewParam.IDNUM_VALUE])
                         for x in value]
        if len(list_of_lists) != len(set(list_of_lists)):
            _ = self.gettext
            raise Invalid(
                node,
                _("You have specified duplicate ID definitions"))


class IdNumSequenceUniquePerWhichIdnum(SequenceSchema, RequestAwareMixin):
    """
    Sequence to capture multiple ID numbers (as type/value pairs) but with only
    up to one per ID number type.
    """
    idnum_sequence = MandatoryIdNumNode()

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("ID numbers")
        self.widget = TranslatableSequenceWidget(request=self.request)

    # noinspection PyMethodMayBeStatic
    def validator(self, node: SchemaNode, value: List[Dict[str, int]]) -> None:
        assert isinstance(value, list)
        which_idnums = [x[ViewParam.WHICH_IDNUM] for x in value]
        if len(which_idnums) != len(set(which_idnums)):
            _ = self.gettext
            raise Invalid(
                node,
                _("You have specified >1 value for one ID number type")
            )


# -----------------------------------------------------------------------------
# Sex
# -----------------------------------------------------------------------------

class OptionalSexSelector(OptionalStringNode, RequestAwareMixin):
    """
    Optional node to choose sex.
    """
    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.validator = None  # type: Optional[ValidatorType]
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Sex")
        choices = sex_choices(self.request)
        values, pv = get_values_and_permissible(choices, True, _("Any"))
        self.widget = RadioChoiceWidget(values=values, inline=True)
        self.validator = OneOf(pv)


class MandatorySexSelector(MandatoryStringNode, RequestAwareMixin):
    """
    Mandatory node to choose sex.
    """
    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.validator = None  # type: Optional[ValidatorType]
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Sex")
        choices = sex_choices(self.request)
        values, pv = get_values_and_permissible(choices)
        self.widget = RadioChoiceWidget(values=values, inline=True)
        self.validator = OneOf(pv)


# -----------------------------------------------------------------------------
# Users
# -----------------------------------------------------------------------------

class MandatoryUserIdSelectorUsersAllowedToSee(SchemaNode, RequestAwareMixin):
    """
    Mandatory node to choose a user, from the users that the requesting user
    is allowed to see.
    """
    schema_type = Integer

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.validator = None  # type: Optional[ValidatorType]
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        from camcops_server.cc_modules.cc_user import User  # delayed import
        _ = self.gettext
        self.title = _("User")
        request = self.request
        dbsession = request.dbsession
        user = request.user
        if user.superuser:
            users = dbsession.query(User).order_by(User.username)
        else:
            # Users in my groups, or groups I'm allowed to see
            my_allowed_group_ids = user.ids_of_groups_user_may_see
            users = dbsession.query(User)\
                .join(Group)\
                .filter(Group.id.in_(my_allowed_group_ids))\
                .order_by(User.username)
        values = []  # type: List[Tuple[Optional[int], str]]
        for user in users:
            values.append((user.id, user.username))
        values, pv = get_values_and_permissible(values, False)
        self.widget = SelectWidget(values=values)
        self.validator = OneOf(pv)


class OptionalUserNameSelector(OptionalStringNode, RequestAwareMixin):
    """
    Optional node to select a username, from all possible users.
    """
    title = "User"

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.validator = None  # type: Optional[ValidatorType]
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        from camcops_server.cc_modules.cc_user import User  # delayed import
        _ = self.gettext
        self.title = _("User")
        request = self.request
        dbsession = request.dbsession
        values = []  # type: List[Tuple[str, str]]
        users = dbsession.query(User).order_by(User.username)
        for user in users:
            values.append((user.username, user.username))
        values, pv = get_values_and_permissible(values, True, _("[ignore]"))
        self.widget = SelectWidget(values=values)
        self.validator = OneOf(pv)


class UsernameNode(SchemaNode, RequestAwareMixin):
    """
    Node to enter a username.
    """
    schema_type = String
    widget = TextInputWidget(attributes={
        AUTOCOMPLETE_ATTR: AutocompleteAttrValues.OFF
    })

    def __init__(self,
                 *args,
                 autocomplete: str = AutocompleteAttrValues.OFF,
                 **kwargs) -> None:
        self.title = ""  # for type checker
        self.autocomplete = autocomplete
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Username")
        # noinspection PyUnresolvedReferences
        self.widget.attributes[AUTOCOMPLETE_ATTR] = self.autocomplete

    def validator(self, node: SchemaNode, value: str) -> None:
        if value == USER_NAME_FOR_SYSTEM:
            _ = self.gettext
            raise Invalid(
                node,
                _("Cannot use system username") + " " +
                repr(USER_NAME_FOR_SYSTEM)
            )
        try:
            validate_username(value, self.request)
        except ValueError as e:
            raise Invalid(node, str(e))


class UserFilterSchema(Schema, RequestAwareMixin):
    """
    Schema to filter the list of users
    """
    # must match ViewParam.INCLUDE_AUTO_GENERATED
    include_auto_generated = BooleanNode()

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        include_auto_generated = get_child_node(self, "include_auto_generated")
        include_auto_generated.title = _("Include auto-generated users")
        include_auto_generated.label = None


class UserFilterForm(InformativeNonceForm):
    """
    Form to filter the list of users
    """
    def __init__(self, request: "CamcopsRequest", **kwargs: Any) -> None:
        _ = request.gettext
        schema = UserFilterSchema().bind(request=request)
        super().__init__(
            schema,
            buttons=[Button(name=FormAction.SET_FILTERS,
                            title=_("Refresh"))],
            css_class=BootstrapCssClasses.FORM_INLINE,
            method=RequestMethod.GET,
            **kwargs
        )


# -----------------------------------------------------------------------------
# Devices
# -----------------------------------------------------------------------------

class MandatoryDeviceIdSelector(SchemaNode, RequestAwareMixin):
    """
    Mandatory node to select a client device ID.
    """
    schema_type = Integer

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.validator = None  # type: Optional[ValidatorType]
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        from camcops_server.cc_modules.cc_device import Device  # delayed import  # noqa
        _ = self.gettext
        self.title = _("Device")
        request = self.request
        dbsession = request.dbsession
        devices = dbsession.query(Device).order_by(Device.friendly_name)
        values = []  # type: List[Tuple[Optional[int], str]]
        for device in devices:
            values.append((device.id, device.friendly_name))
        values, pv = get_values_and_permissible(values, False)
        self.widget = SelectWidget(values=values)
        self.validator = OneOf(pv)


# -----------------------------------------------------------------------------
# Server PK
# -----------------------------------------------------------------------------

class ServerPkSelector(OptionalIntNode, RequestAwareMixin):
    """
    Optional node to request an integer, marked as a server PK.
    """
    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Server PK")


# -----------------------------------------------------------------------------
# Dates/times
# -----------------------------------------------------------------------------

class StartPendulumSelector(TranslatableOptionalPendulumNode,
                            RequestAwareMixin):
    """
    Optional node to select a start date/time.
    """
    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        super().__init__(*args, **kwargs)

    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        super().after_bind(node, kw)
        _ = self.gettext
        self.title = _("Start date/time (local timezone; inclusive)")


class EndPendulumSelector(TranslatableOptionalPendulumNode,
                          RequestAwareMixin):
    """
    Optional node to select an end date/time.
    """
    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        super().__init__(*args, **kwargs)

    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        super().after_bind(node, kw)
        _ = self.gettext
        self.title = _("End date/time (local timezone; exclusive)")


class StartDateTimeSelector(TranslatableDateTimeSelectorNode,
                            RequestAwareMixin):
    """
    Optional node to select a start date/time (in UTC).
    """
    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        super().__init__(*args, **kwargs)

    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        super().after_bind(node, kw)
        _ = self.gettext
        self.title = _("Start date/time (UTC; inclusive)")


class EndDateTimeSelector(TranslatableDateTimeSelectorNode,
                          RequestAwareMixin):
    """
    Optional node to select an end date/time (in UTC).
    """
    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        super().__init__(*args, **kwargs)

    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        super().after_bind(node, kw)
        _ = self.gettext
        self.title = _("End date/time (UTC; exclusive)")


'''
class StartDateSelector(TranslatableDateSelectorNode,
                        RequestAwareMixin):
    """
    Optional node to select a start date (in UTC).
    """
    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        super().__init__(*args, **kwargs)

    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        super().after_bind(node, kw)
        _ = self.gettext
        self.title = _("Start date (UTC; inclusive)")


class EndDateSelector(TranslatableDateSelectorNode,
                      RequestAwareMixin):
    """
    Optional node to select an end date (in UTC).
    """
    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        super().__init__(*args, **kwargs)

    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        super().after_bind(node, kw)
        _ = self.gettext
        self.title = _("End date (UTC; inclusive)")
'''


# -----------------------------------------------------------------------------
# Rows per page
# -----------------------------------------------------------------------------

class RowsPerPageSelector(SchemaNode, RequestAwareMixin):
    """
    Node to select how many rows per page are shown.
    """
    _choices = ((10, "10"), (25, "25"), (50, "50"), (100, "100"))

    schema_type = Integer
    default = DEFAULT_ROWS_PER_PAGE
    widget = RadioChoiceWidget(values=_choices)
    validator = OneOf(list(x[0] for x in _choices))

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Items to show per page")


# -----------------------------------------------------------------------------
# Groups
# -----------------------------------------------------------------------------

class MandatoryGroupIdSelectorAllGroups(SchemaNode, RequestAwareMixin):
    """
    Offers a picklist of groups from ALL POSSIBLE GROUPS.
    Used by superusers: "add user to any group".
    """
    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.validator = None  # type: Optional[ValidatorType]
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Group")
        request = self.request
        dbsession = request.dbsession
        groups = dbsession.query(Group).order_by(Group.name)
        values = [(g.id, g.name) for g in groups]
        values, pv = get_values_and_permissible(values)
        self.widget = SelectWidget(values=values)
        self.validator = OneOf(pv)

    @staticmethod
    def schema_type() -> SchemaType:
        return Integer()


class MandatoryGroupIdSelectorAdministeredGroups(SchemaNode, RequestAwareMixin):
    """
    Offers a picklist of groups from GROUPS ADMINISTERED BY REQUESTOR.
    Used by groupadmins: "add user to one of my groups".
    """
    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.validator = None  # type: Optional[ValidatorType]
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Group")
        request = self.request
        dbsession = request.dbsession
        administered_group_ids = request.user.ids_of_groups_user_is_admin_for
        groups = dbsession.query(Group).order_by(Group.name)
        values = [(g.id, g.name) for g in groups
                  if g.id in administered_group_ids]
        values, pv = get_values_and_permissible(values)
        self.widget = SelectWidget(values=values)
        self.validator = OneOf(pv)

    @staticmethod
    def schema_type() -> SchemaType:
        return Integer()


class MandatoryGroupIdSelectorOtherGroups(SchemaNode, RequestAwareMixin):
    """
    Offers a picklist of groups THAT ARE NOT THE SPECIFIED GROUP (as specified
    in ``kw[Binding.GROUP]``).
    Used by superusers: "which other groups can this group see?"
    """
    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.validator = None  # type: Optional[ValidatorType]
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Other group")
        request = self.request
        group = kw[Binding.GROUP]  # type: Group  # ATYPICAL BINDING
        dbsession = request.dbsession
        groups = dbsession.query(Group).order_by(Group.name)
        values = [(g.id, g.name) for g in groups if g.id != group.id]
        values, pv = get_values_and_permissible(values)
        self.widget = SelectWidget(values=values)
        self.validator = OneOf(pv)

    @staticmethod
    def schema_type() -> SchemaType:
        return Integer()


class MandatoryGroupIdSelectorUserGroups(SchemaNode, RequestAwareMixin):
    """
    Offers a picklist of groups from THOSE THE USER IS A MEMBER OF.
    Used for: "which of your groups do you want to upload into?"
    """
    def __init__(self, *args, **kwargs) -> None:
        if not hasattr(self, "allow_none"):
            # ... allows parameter-free (!) inheritance by OptionalGroupIdSelectorUserGroups  # noqa
            self.allow_none = False
        self.title = ""  # for type checker
        self.validator = None  # type: Optional[ValidatorType]
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Group")
        user = kw[Binding.USER]  # type: User  # ATYPICAL BINDING
        groups = sorted(list(user.groups), key=lambda g: g.name)
        values = [(g.id, g.name) for g in groups]
        values, pv = get_values_and_permissible(values, self.allow_none,
                                                _("[None]"))
        self.widget = SelectWidget(values=values)
        self.validator = OneOf(pv)

    @staticmethod
    def schema_type() -> SchemaType:
        return Integer()


class OptionalGroupIdSelectorUserGroups(MandatoryGroupIdSelectorUserGroups):
    """
    Offers a picklist of groups from THOSE THE USER IS A MEMBER OF.
    Used for "which do you want to upload into?". Optional.
    """
    default = None
    missing = None

    def __init__(self, *args, **kwargs) -> None:
        self.allow_none = True
        super().__init__(*args, **kwargs)

    @staticmethod
    def schema_type() -> SchemaType:
        return AllowNoneType(Integer())


class MandatoryGroupIdSelectorAllowedGroups(SchemaNode, RequestAwareMixin):
    """
    Offers a picklist of groups from THOSE THE USER IS ALLOWED TO SEE.
    Used for task filters.
    """
    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.validator = None  # type: Optional[ValidatorType]
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Group")
        request = self.request
        dbsession = request.dbsession
        user = request.user
        if user.superuser:
            groups = dbsession.query(Group).order_by(Group.name)
        else:
            groups = sorted(list(user.groups), key=lambda g: g.name)
        values = [(g.id, g.name) for g in groups]
        values, pv = get_values_and_permissible(values)
        self.widget = SelectWidget(values=values)
        self.validator = OneOf(pv)

    @staticmethod
    def schema_type() -> SchemaType:
        return Integer()


class GroupsSequenceBase(SequenceSchema, RequestAwareMixin):
    """
    Sequence schema to capture zero or more non-duplicate groups.
    """
    def __init__(self, *args, minimum_number: int = 0, **kwargs) -> None:
        self.title = ""  # for type checker
        self.minimum_number = minimum_number
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Groups")
        self.widget = TranslatableSequenceWidget(request=self.request)

    # noinspection PyMethodMayBeStatic
    def validator(self,
                  node: SchemaNode,
                  value: List[int]) -> None:
        assert isinstance(value, list)
        _ = self.gettext
        if len(value) != len(set(value)):
            raise Invalid(node, _("You have specified duplicate groups"))
        if len(value) < self.minimum_number:
            raise Invalid(
                node,
                _("You must specify at least {} group(s)").format(
                    self.minimum_number
                )
            )


class AllGroupsSequence(GroupsSequenceBase):
    """
    Sequence to offer a choice of all possible groups.

    Typical use: superuser assigns group memberships to a user.
    """
    group_id_sequence = MandatoryGroupIdSelectorAllGroups()


class AdministeredGroupsSequence(GroupsSequenceBase):
    """
    Sequence to offer a choice of the groups administered by the requestor.

    Typical use: (non-superuser) group administrator assigns group memberships
    to a user.
    """
    group_id_sequence = MandatoryGroupIdSelectorAdministeredGroups()

    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, minimum_number=1, **kwargs)


class AllOtherGroupsSequence(GroupsSequenceBase):
    """
    Sequence to offer a choice of all possible OTHER groups (as determined
    relative to the group specified in ``kw[Binding.GROUP]``).

    Typical use: superuser assigns group permissions to another group.
    """
    group_id_sequence = MandatoryGroupIdSelectorOtherGroups()


class AllowedGroupsSequence(GroupsSequenceBase):
    """
    Sequence to offer a choice of all the groups the user is allowed to see.
    """
    group_id_sequence = MandatoryGroupIdSelectorAllowedGroups()

    def __init__(self, *args, **kwargs) -> None:
        self.description = ""  # for type checker
        super().__init__(*args, **kwargs)

    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        super().after_bind(node, kw)
        self.description = self.or_join_description


# -----------------------------------------------------------------------------
# Languages (strictly, locales)
# -----------------------------------------------------------------------------

class LanguageSelector(SchemaNode, RequestAwareMixin):
    """
    Node to choose a language code, from those supported by the server.
    """
    _choices = POSSIBLE_LOCALES_WITH_DESCRIPTIONS
    schema_type = String
    default = DEFAULT_LOCALE
    missing = DEFAULT_LOCALE
    widget = SelectWidget(values=_choices)  # intrinsically translated!
    validator = OneOf(POSSIBLE_LOCALES)

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Group")
        request = self.request  # noqa: F841
        self.title = _("Language")


# -----------------------------------------------------------------------------
# Validating dangerous operations
# -----------------------------------------------------------------------------

class HardWorkConfirmationSchema(CSRFSchema):
    """
    Schema to make it hard to do something. We require a pattern of yes/no
    answers before we will proceed.
    """
    confirm_1_t = BooleanNode(default=False)
    confirm_2_t = BooleanNode(default=True)
    confirm_3_f = BooleanNode(default=True)
    confirm_4_t = BooleanNode(default=False)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        confirm_1_t = get_child_node(self, "confirm_1_t")
        confirm_1_t.title = _("Really?")
        confirm_2_t = get_child_node(self, "confirm_2_t")
        # TRANSLATOR: string context described here
        confirm_2_t.title = _("Leave ticked to confirm")
        confirm_3_f = get_child_node(self, "confirm_3_f")
        confirm_3_f.title = _("Please untick to confirm")
        confirm_4_t = get_child_node(self, "confirm_4_t")
        confirm_4_t.title = _("Be really sure; tick here also to confirm")

    # noinspection PyMethodMayBeStatic
    def validator(self, node: SchemaNode, value: Any) -> None:
        if ((not value['confirm_1_t']) or
                (not value['confirm_2_t']) or
                value['confirm_3_f'] or
                (not value['confirm_4_t'])):
            _ = self.gettext
            raise Invalid(node, _("Not fully confirmed"))


# -----------------------------------------------------------------------------
# URLs
# -----------------------------------------------------------------------------

class HiddenRedirectionUrlNode(HiddenStringNode, RequestAwareMixin):
    """
    Note to encode a hidden URL, for redirection.
    """
    # noinspection PyMethodMayBeStatic
    def validator(self, node: SchemaNode, value: str) -> None:
        if value:
            try:
                validate_redirect_url(value, self.request)
            except ValueError:
                _ = self.gettext
                raise Invalid(node, _("Invalid redirection URL"))


# =============================================================================
# Login
# =============================================================================

class LoginSchema(CSRFSchema):
    """
    Schema to capture login details.
    """
    username = UsernameNode(
        autocomplete=AutocompleteAttrValues.USERNAME
    )  # name must match ViewParam.USERNAME
    password = SchemaNode(  # name must match ViewParam.PASSWORD
        String(),
        widget=PasswordWidget(attributes={
            AUTOCOMPLETE_ATTR: AutocompleteAttrValues.CURRENT_PASSWORD
        }),
    )
    redirect_url = HiddenRedirectionUrlNode()  # name must match ViewParam.REDIRECT_URL  # noqa

    def __init__(self, *args, autocomplete_password: bool = True,
                 **kwargs) -> None:
        self.autocomplete_password = autocomplete_password
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        password = get_child_node(self, "password")
        password.title = _("Password")
        password.widget.attributes[AUTOCOMPLETE_ATTR] = (
            AutocompleteAttrValues.CURRENT_PASSWORD
            if self.autocomplete_password else AutocompleteAttrValues.OFF
        )


class LoginForm(InformativeNonceForm):
    """
    Form to capture login details.
    """
    def __init__(self,
                 request: "CamcopsRequest",
                 autocomplete_password: bool = True,
                 **kwargs) -> None:
        """
        Args:
            autocomplete_password:
                suggest to the browser that it's OK to store the password for
                autocompletion? Note that browsers may ignore this.
        """
        _ = request.gettext
        schema = LoginSchema(
            autocomplete_password=autocomplete_password
        ).bind(request=request)
        super().__init__(
            schema,
            buttons=[Button(name=FormAction.SUBMIT, title=_("Log in"))],
            # autocomplete=autocomplete_password,
            **kwargs
        )
        # Suboptimal: autocomplete_password is not applied to the password
        # widget, just to the form; see
        # http://stackoverflow.com/questions/2530
        # Note that e.g. Chrome may ignore this.
        # ... fixed 2020-09-29 by applying autocomplete to LoginSchema.password


# =============================================================================
# Change password
# =============================================================================

class MustChangePasswordNode(SchemaNode, RequestAwareMixin):
    """
    Boolean node: must the user change their password?
    """
    schema_type = Boolean
    default = True
    missing = True

    def __init__(self, *args, **kwargs) -> None:
        self.label = ""  # for type checker
        self.title = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.label = _("User must change password at next login")
        self.title = _("Must change password at next login?")


class OldUserPasswordCheck(SchemaNode, RequestAwareMixin):
    """
    Schema to capture an old password (for when a password is being changed).
    """
    schema_type = String
    widget = PasswordWidget(attributes={
        AUTOCOMPLETE_ATTR: AutocompleteAttrValues.CURRENT_PASSWORD
    })

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Old password")

    def validator(self, node: SchemaNode, value: str) -> None:
        request = self.request
        user = request.user
        assert user is not None
        if not user.is_password_correct(value):
            _ = request.gettext
            raise Invalid(node, _("Old password incorrect"))


class NewPasswordNode(SchemaNode, RequestAwareMixin):
    """
    Node to enter a new password.
    """
    schema_type = String
    widget = CheckedPasswordWidget(attributes={
        AUTOCOMPLETE_ATTR: AutocompleteAttrValues.NEW_PASSWORD
    })

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.description = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("New password")
        self.description = _("Type the new password and confirm it")

    def validator(self, node: SchemaNode, value: str) -> None:
        try:
            validate_new_password(value, self.request)
        except ValueError as e:
            raise Invalid(node, str(e))


class ChangeOwnPasswordSchema(CSRFSchema):
    """
    Schema to change one's own password.
    """
    old_password = OldUserPasswordCheck()
    new_password = NewPasswordNode()  # name must match ViewParam.NEW_PASSWORD

    def __init__(self, *args, must_differ: bool = True, **kwargs) -> None:
        """
        Args:
            must_differ:
                must the new password be different from the old one?
        """
        self.must_differ = must_differ
        super().__init__(*args, **kwargs)

    def validator(self, node: SchemaNode, value: Dict[str, str]) -> None:
        if self.must_differ and value['new_password'] == value['old_password']:
            _ = self.gettext
            raise Invalid(node, _("New password must differ from old"))


class ChangeOwnPasswordForm(InformativeNonceForm):
    """
    Form to change one's own password.
    """
    def __init__(self, request: "CamcopsRequest",
                 must_differ: bool = True,
                 **kwargs) -> None:
        """
        Args:
            must_differ:
                must the new password be different from the old one?
        """
        schema = ChangeOwnPasswordSchema(must_differ=must_differ).\
            bind(request=request)
        super().__init__(
            schema,
            buttons=[Button(name=FormAction.SUBMIT,
                            title=change_password_title(request))],
            **kwargs
        )


class ChangeOtherPasswordSchema(CSRFSchema):
    """
    Schema to change another user's password.
    """
    user_id = HiddenIntegerNode()  # name must match ViewParam.USER_ID
    must_change_password = MustChangePasswordNode()  # match ViewParam.MUST_CHANGE_PASSWORD  # noqa
    new_password = NewPasswordNode()  # name must match ViewParam.NEW_PASSWORD


class ChangeOtherPasswordForm(SimpleSubmitForm):
    """
    Form to change another user's password.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        super().__init__(schema_class=ChangeOtherPasswordSchema,
                         submit_title=change_password_title(request),
                         request=request, **kwargs)


# =============================================================================
# Offer/agree terms
# =============================================================================

class OfferTermsSchema(CSRFSchema):
    """
    Schema to offer terms and ask the user to accept them.
    """
    pass


class OfferTermsForm(SimpleSubmitForm):
    """
    Form to offer terms and ask the user to accept them.
    """
    def __init__(self,
                 request: "CamcopsRequest",
                 agree_button_text: str,
                 **kwargs) -> None:
        """
        Args:
            agree_button_text:
                text for the "agree" button
        """
        super().__init__(schema_class=OfferTermsSchema,
                         submit_title=agree_button_text,
                         request=request, **kwargs)


# =============================================================================
# View audit trail
# =============================================================================

class OptionalIPAddressNode(OptionalStringNode, RequestAwareMixin):
    """
    Optional IPv4 or IPv6 address.
    """
    def validator(self, node: SchemaNode, value: str) -> None:
        try:
            validate_ip_address(value, self.request)
        except ValueError as e:
            raise Invalid(node, e)


class OptionalAuditSourceNode(OptionalStringNode, RequestAwareMixin):
    """
    Optional IPv4 or IPv6 address.
    """
    def validator(self, node: SchemaNode, value: str) -> None:
        try:
            validate_by_char_and_length(
                value,
                permitted_char_expression=ALPHANUM_UNDERSCORE_CHAR,
                min_length=0,
                max_length=StringLengths.AUDIT_SOURCE_MAX_LEN,
                req=self.request
            )
        except ValueError as e:
            raise Invalid(node, e)


class AuditTrailSchema(CSRFSchema):
    """
    Schema to filter audit trail entries.
    """
    rows_per_page = RowsPerPageSelector()  # must match ViewParam.ROWS_PER_PAGE
    start_datetime = StartPendulumSelector()  # must match ViewParam.START_DATETIME  # noqa
    end_datetime = EndPendulumSelector()  # must match ViewParam.END_DATETIME
    source = OptionalAuditSourceNode()  # must match ViewParam.SOURCE  # noqa
    remote_ip_addr = OptionalIPAddressNode()  # must match ViewParam.REMOTE_IP_ADDR  # noqa
    username = OptionalUserNameSelector()  # must match ViewParam.USERNAME  # noqa
    table_name = OptionalSingleTaskSelector()  # must match ViewParam.TABLENAME  # noqa
    server_pk = ServerPkSelector()  # must match ViewParam.SERVER_PK
    truncate = BooleanNode(default=True)  # must match ViewParam.TRUNCATE

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        source = get_child_node(self, "source")
        source.title = _("Source (e.g. webviewer, tablet, console)")
        remote_ip_addr = get_child_node(self, "remote_ip_addr")
        remote_ip_addr.title = _("Remote IP address")
        truncate = get_child_node(self, "truncate")
        truncate.title = _("Truncate details for easy viewing")


class AuditTrailForm(SimpleSubmitForm):
    """
    Form to filter and then view audit trail entries.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        _ = request.gettext
        super().__init__(schema_class=AuditTrailSchema,
                         submit_title=_("View audit trail"),
                         request=request, **kwargs)


# =============================================================================
# View export logs
# =============================================================================

class OptionalExportRecipientNameSelector(OptionalStringNode,
                                          RequestAwareMixin):
    """
    Optional node to pick an export recipient name from those present in the
    database.
    """
    title = "Export recipient"

    def __init__(self, *args, **kwargs) -> None:
        self.validator = None  # type: Optional[ValidatorType]
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        from camcops_server.cc_modules.cc_exportrecipient import ExportRecipient  # delayed import  # noqa
        request = self.request
        _ = request.gettext
        dbsession = request.dbsession
        q = (
            dbsession.query(ExportRecipient.recipient_name)
            .distinct()
            .order_by(ExportRecipient.recipient_name)
        )
        values = []  # type: List[Tuple[str, str]]
        for row in q:
            recipient_name = row[0]
            values.append((recipient_name, recipient_name))
        values, pv = get_values_and_permissible(values, True, _("[Any]"))
        self.widget = SelectWidget(values=values)
        self.validator = OneOf(pv)


class ExportedTaskListSchema(CSRFSchema):
    """
    Schema to filter HL7 message logs.
    """
    rows_per_page = RowsPerPageSelector()  # must match ViewParam.ROWS_PER_PAGE
    recipient_name = OptionalExportRecipientNameSelector()  # must match ViewParam.RECIPIENT_NAME  # noqa
    table_name = OptionalSingleTaskSelector()  # must match ViewParam.TABLENAME  # noqa
    server_pk = ServerPkSelector()  # must match ViewParam.SERVER_PK
    id = OptionalIntNode()  # must match ViewParam.ID  # noqa
    start_datetime = StartDateTimeSelector()  # must match ViewParam.START_DATETIME  # noqa
    end_datetime = EndDateTimeSelector()  # must match ViewParam.END_DATETIME

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        id_ = get_child_node(self, "id")
        id_.title = _("ExportedTask ID")


class ExportedTaskListForm(SimpleSubmitForm):
    """
    Form to filter and then view exported task logs.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        _ = request.gettext
        super().__init__(schema_class=ExportedTaskListSchema,
                         submit_title=_("View exported task log"),
                         request=request, **kwargs)


# =============================================================================
# Task filters
# =============================================================================

class TextContentsSequence(SequenceSchema, RequestAwareMixin):
    """
    Sequence to capture multiple pieces of text (representing text contents
    for a task filter).
    """
    text_sequence = SchemaNode(
        String(),
        validator=Length(0, StringLengths.FILTER_TEXT_MAX_LEN)
    )  # BEWARE: fairly unrestricted contents.

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.description = ""  # for type checker
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Text contents")
        self.description = self.or_join_description
        self.widget = TranslatableSequenceWidget(request=self.request)
        # Now it'll say "[Add]" Text Sequence because it'll make the string
        # "Text Sequence" from the name of text_sequence. Unless we do this:
        text_sequence = get_child_node(self, "text_sequence")
        # TRANSLATOR: For the task filter form: the text in "Add text"
        text_sequence.title = _("text")

    # noinspection PyMethodMayBeStatic
    def validator(self, node: SchemaNode, value: List[str]) -> None:
        assert isinstance(value, list)
        if len(value) != len(set(value)):
            _ = self.gettext
            raise Invalid(node, _("You have specified duplicate text filters"))


class UploadingUserSequence(SequenceSchema, RequestAwareMixin):
    """
    Sequence to capture multiple users (for task filters: "uploaded by one of
    the following users...").
    """
    user_id_sequence = MandatoryUserIdSelectorUsersAllowedToSee()

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.description = ""  # for type checker
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Uploading users")
        self.description = self.or_join_description
        self.widget = TranslatableSequenceWidget(request=self.request)

    # noinspection PyMethodMayBeStatic
    def validator(self, node: SchemaNode, value: List[int]) -> None:
        assert isinstance(value, list)
        if len(value) != len(set(value)):
            _ = self.gettext
            raise Invalid(node, _("You have specified duplicate users"))


class DevicesSequence(SequenceSchema, RequestAwareMixin):
    """
    Sequence to capture multiple client devices (for task filters: "uploaded by
    one of the following devices...").
    """
    device_id_sequence = MandatoryDeviceIdSelector()

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.description = ""  # for type checker
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Uploading devices")
        self.description = self.or_join_description
        self.widget = TranslatableSequenceWidget(request=self.request)

    # noinspection PyMethodMayBeStatic
    def validator(self, node: SchemaNode, value: List[int]) -> None:
        assert isinstance(value, list)
        if len(value) != len(set(value)):
            raise Invalid(node, "You have specified duplicate devices")


class OptionalPatientNameNode(OptionalStringNode, RequestAwareMixin):
    def validator(self, node: SchemaNode, value: str) -> None:
        try:
            # TODO: Validating human names is hard.
            # Decide if validation here is necessary and whether it should
            # be configurable.
            # validate_human_name(value, self.request)

            # Does nothing but better to be explicit
            validate_anything(value, self.request)
        except ValueError as e:
            # Should never happen with validate_anything
            raise Invalid(node, str(e))


class EditTaskFilterWhoSchema(Schema, RequestAwareMixin):
    """
    Schema to edit the "who" parts of a task filter.
    """
    surname = OptionalPatientNameNode()  # must match ViewParam.SURNAME  # noqa
    forename = OptionalPatientNameNode()  # must match ViewParam.FORENAME  # noqa
    dob = SchemaNode(Date(), missing=None)  # must match ViewParam.DOB
    sex = OptionalSexSelector()  # must match ViewParam.SEX
    id_references = IdNumSequenceAnyCombination()  # must match ViewParam.ID_REFERENCES  # noqa

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        surname = get_child_node(self, "surname")
        surname.title = _("Surname")
        forename = get_child_node(self, "forename")
        forename.title = _("Forename")
        dob = get_child_node(self, "dob")
        dob.title = _("Date of birth")
        id_references = get_child_node(self, "id_references")
        id_references.description = self.or_join_description


class EditTaskFilterWhenSchema(Schema):
    """
    Schema to edit the "when" parts of a task filter.
    """
    start_datetime = StartPendulumSelector()  # must match ViewParam.START_DATETIME  # noqa
    end_datetime = EndPendulumSelector()  # must match ViewParam.END_DATETIME


class EditTaskFilterWhatSchema(Schema, RequestAwareMixin):
    """
    Schema to edit the "what" parts of a task filter.
    """
    text_contents = TextContentsSequence()  # must match ViewParam.TEXT_CONTENTS  # noqa
    complete_only = BooleanNode(default=False)  # must match ViewParam.COMPLETE_ONLY  # noqa
    tasks = MultiTaskSelector()  # must match ViewParam.TASKS

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        complete_only = get_child_node(self, "complete_only")
        only_completed_text = _("Only completed tasks?")
        complete_only.title = only_completed_text
        complete_only.label = only_completed_text


class EditTaskFilterAdminSchema(Schema):
    """
    Schema to edit the "admin" parts of a task filter.
    """
    device_ids = DevicesSequence()  # must match ViewParam.DEVICE_IDS
    user_ids = UploadingUserSequence()  # must match ViewParam.USER_IDS
    group_ids = AllowedGroupsSequence()  # must match ViewParam.GROUP_IDS


class EditTaskFilterSchema(CSRFSchema):
    """
    Schema to edit a task filter.
    """
    who = EditTaskFilterWhoSchema(  # must match ViewParam.WHO
        widget=MappingWidget(template="mapping_accordion", open=False)
    )
    what = EditTaskFilterWhatSchema(  # must match ViewParam.WHAT
        widget=MappingWidget(template="mapping_accordion", open=False)
    )
    when = EditTaskFilterWhenSchema(  # must match ViewParam.WHEN
        widget=MappingWidget(template="mapping_accordion", open=False)
    )
    admin = EditTaskFilterAdminSchema(  # must match ViewParam.ADMIN
        widget=MappingWidget(template="mapping_accordion", open=False)
    )

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        # log.debug("EditTaskFilterSchema.after_bind")
        # log.debug("{!r}", self.__dict__)
        # This is pretty nasty. By the time we get here, the Form class has
        # made Field objects, and, I think, called a clone() function on us.
        # Objects like "who" are not in our __dict__ any more. Our __dict__
        # looks like:
        #   {
        #       'typ': <colander.Mapping object at 0x7fd7989b18d0>,
        #       'bindings': {
        #           'open_who': True,
        #           'open_when': True,
        #           'request': ...,
        #       },
        #       '_order': 118,
        #       'children': [
        #           <...CSRFToken object at ... (named csrf)>,
        #           <...EditTaskFilterWhoSchema object at ... (named who)>,
        #           ...
        #       ],
        #       'title': ''
        #   }
        _ = self.gettext
        who = get_child_node(self, "who")
        what = get_child_node(self, "what")
        when = get_child_node(self, "when")
        admin = get_child_node(self, "admin")
        who.title = _("Who")
        what.title = _("What")
        when.title = _("When")
        admin.title = _("Administrative criteria")
        # log.debug("who = {!r}", who)
        # log.debug("who.__dict__ = {!r}", who.__dict__)
        who.widget.open = kw[Binding.OPEN_WHO]
        what.widget.open = kw[Binding.OPEN_WHAT]
        when.widget.open = kw[Binding.OPEN_WHEN]
        admin.widget.open = kw[Binding.OPEN_ADMIN]


class EditTaskFilterForm(InformativeNonceForm):
    """
    Form to edit a task filter.
    """
    def __init__(self,
                 request: "CamcopsRequest",
                 open_who: bool = False,
                 open_what: bool = False,
                 open_when: bool = False,
                 open_admin: bool = False,
                 **kwargs) -> None:
        _ = request.gettext
        schema = EditTaskFilterSchema().bind(request=request,
                                             open_admin=open_admin,
                                             open_what=open_what,
                                             open_when=open_when,
                                             open_who=open_who)
        super().__init__(
            schema,
            buttons=[Button(name=FormAction.SET_FILTERS,
                            title=_("Set filters")),
                     Button(name=FormAction.CLEAR_FILTERS,
                            title=_("Clear"))],
            **kwargs
        )


class TasksPerPageSchema(CSRFSchema):
    """
    Schema to edit the number of rows per page, for the task view.
    """
    rows_per_page = RowsPerPageSelector()  # must match ViewParam.ROWS_PER_PAGE


class TasksPerPageForm(InformativeNonceForm):
    """
    Form to edit the number of tasks per page, for the task view.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        _ = request.gettext
        schema = TasksPerPageSchema().bind(request=request)
        super().__init__(
            schema,
            buttons=[Button(name=FormAction.SUBMIT_TASKS_PER_PAGE,
                            title=_("Set n/page"))],
            css_class=BootstrapCssClasses.FORM_INLINE,
            **kwargs
        )


class RefreshTasksSchema(CSRFSchema):
    """
    Schema for a "refresh tasks" button.
    """
    pass


class RefreshTasksForm(InformativeNonceForm):
    """
    Form for a "refresh tasks" button.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        _ = request.gettext
        schema = RefreshTasksSchema().bind(request=request)
        super().__init__(
            schema,
            buttons=[Button(name=FormAction.REFRESH_TASKS,
                            title=_("Refresh"))],
            **kwargs
        )


# =============================================================================
# Trackers
# =============================================================================

class TaskTrackerOutputTypeSelector(SchemaNode, RequestAwareMixin):
    """
    Node to select the output format for a tracker.
    """
    # Choices don't require translation
    _choices = ((ViewArg.HTML, "HTML"),
                (ViewArg.PDF, "PDF"),
                (ViewArg.XML, "XML"))

    schema_type = String
    default = ViewArg.HTML
    missing = ViewArg.HTML
    widget = RadioChoiceWidget(values=_choices)
    validator = OneOf(list(x[0] for x in _choices))

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("View as")


class ChooseTrackerSchema(CSRFSchema):
    """
    Schema to select a tracker or CTV.
    """
    which_idnum = MandatoryWhichIdNumSelector()  # must match ViewParam.WHICH_IDNUM  # noqa
    idnum_value = MandatoryIdNumValue()  # must match ViewParam.IDNUM_VALUE  # noqa
    start_datetime = StartPendulumSelector()  # must match ViewParam.START_DATETIME  # noqa
    end_datetime = EndPendulumSelector()  # must match ViewParam.END_DATETIME
    all_tasks = BooleanNode(default=True)  # match ViewParam.ALL_TASKS
    tasks = MultiTaskSelector()  # must match ViewParam.TASKS
    # tracker_tasks_only will be set via the binding
    via_index = ViaIndexSelector()  # must match ViewParam.VIA_INDEX
    viewtype = TaskTrackerOutputTypeSelector()  # must match ViewParams.VIEWTYPE  # noqa

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        all_tasks = get_child_node(self, "all_tasks")
        text = _("Use all eligible task types?")
        all_tasks.title = text
        all_tasks.label = text


class ChooseTrackerForm(InformativeNonceForm):
    """
    Form to select a tracker or CTV.
    """
    def __init__(self, request: "CamcopsRequest",
                 as_ctv: bool, **kwargs) -> None:
        """
        Args:
            as_ctv: CTV, not tracker?
        """
        _ = request.gettext
        schema = ChooseTrackerSchema().bind(request=request,
                                            tracker_tasks_only=not as_ctv)
        super().__init__(
            schema,
            buttons=[
                Button(
                    name=FormAction.SUBMIT,
                    title=(_("View CTV") if as_ctv else _("View tracker"))
                )
            ],
            **kwargs
        )


# =============================================================================
# Reports, which use dynamically created forms
# =============================================================================

class ReportOutputTypeSelector(SchemaNode, RequestAwareMixin):
    """
    Node to select the output format for a report.
    """
    schema_type = String
    default = ViewArg.HTML
    missing = ViewArg.HTML

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.widget = None  # type: Optional[Widget]
        self.validator = None  # type: Optional[ValidatorType]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("View as")
        choices = self.get_choices()
        values, pv = get_values_and_permissible(choices)
        self.widget = RadioChoiceWidget(values=choices)
        self.validator = OneOf(pv)

    def get_choices(self) -> Tuple[Tuple[str, str]]:
        _ = self.gettext
        # noinspection PyTypeChecker
        return (
            (ViewArg.HTML, _("HTML")),
            (ViewArg.ODS, _("OpenOffice spreadsheet (ODS) file")),
            (ViewArg.TSV, _("TSV (tab-separated values)")),
            (ViewArg.XLSX, _("XLSX (Microsoft Excel) file"))
        )


class ReportParamSchema(CSRFSchema):
    """
    Schema to embed a report type (ID) and output format (view type).
    """
    viewtype = ReportOutputTypeSelector()  # must match ViewParam.VIEWTYPE
    report_id = HiddenStringNode()  # must match ViewParam.REPORT_ID
    # Specific forms may inherit from this.


class DateTimeFilteredReportParamSchema(ReportParamSchema):
    start_datetime = StartPendulumSelector()
    end_datetime = EndPendulumSelector()


class ReportParamForm(SimpleSubmitForm):
    """
    Form to view a specific report. Often derived from, to configure the report
    in more detail.
    """
    def __init__(self, request: "CamcopsRequest",
                 schema_class: Type[ReportParamSchema], **kwargs) -> None:
        _ = request.gettext
        super().__init__(schema_class=schema_class,
                         submit_title=_("View report"),
                         request=request, **kwargs)


# =============================================================================
# View DDL
# =============================================================================

def get_sql_dialect_choices(
        request: "CamcopsRequest") -> List[Tuple[str, str]]:
    _ = request.gettext
    return [
        # http://docs.sqlalchemy.org/en/latest/dialects/
        (SqlaDialectName.MYSQL, "MySQL"),
        (SqlaDialectName.MSSQL, "Microsoft SQL Server"),
        (SqlaDialectName.ORACLE, "Oracle" + _("[WILL NOT WORK]")),
        # ... Oracle doesn't work; SQLAlchemy enforces the Oracle rule of a 30-
        # character limit for identifiers, only relaxed to 128 characters in
        # Oracle 12.2 (March 2017).
        (SqlaDialectName.FIREBIRD, "Firebird"),
        (SqlaDialectName.POSTGRES, "PostgreSQL"),
        (SqlaDialectName.SQLITE, "SQLite"),
        (SqlaDialectName.SYBASE, "Sybase"),
    ]


class DatabaseDialectSelector(SchemaNode, RequestAwareMixin):
    """
    Node to choice an SQL dialect (for viewing DDL).
    """
    schema_type = String
    default = SqlaDialectName.MYSQL
    missing = SqlaDialectName.MYSQL

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.widget = None  # type: Optional[Widget]
        self.validator = None  # type: Optional[ValidatorType]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("SQL dialect to use (not all may be valid)")
        choices = get_sql_dialect_choices(self.request)
        values, pv = get_values_and_permissible(choices)
        self.widget = RadioChoiceWidget(values=values)
        self.validator = OneOf(pv)


class ViewDdlSchema(CSRFSchema):
    """
    Schema to choose how to view DDL.
    """
    dialect = DatabaseDialectSelector()  # must match ViewParam.DIALECT


class ViewDdlForm(SimpleSubmitForm):
    """
    Form to choose how to view DDL (and then view it).
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        _ = request.gettext
        super().__init__(schema_class=ViewDdlSchema,
                         submit_title=_("View DDL"),
                         request=request, **kwargs)


# =============================================================================
# Add/edit/delete users
# =============================================================================

class UserGroupPermissionsGroupAdminSchema(CSRFSchema):
    """
    Edit group-specific permissions for a user. For group administrators.
    """
    may_upload = BooleanNode(default=True)  # match ViewParam.MAY_UPLOAD and User attribute  # noqa
    may_register_devices = BooleanNode(default=True)  # match ViewParam.MAY_REGISTER_DEVICES and User attribute  # noqa
    may_use_webviewer = BooleanNode(default=True)  # match ViewParam.MAY_USE_WEBVIEWER and User attribute  # noqa
    view_all_patients_when_unfiltered = BooleanNode(default=False)  # match ViewParam.VIEW_ALL_PATIENTS_WHEN_UNFILTERED and User attribute  # noqa
    may_dump_data = BooleanNode(default=False)  # match ViewParam.MAY_DUMP_DATA and User attribute  # noqa
    may_run_reports = BooleanNode(default=False)  # match ViewParam.MAY_RUN_REPORTS and User attribute  # noqa
    may_add_notes = BooleanNode(default=False)  # match ViewParam.MAY_ADD_NOTES and User attribute  # noqa

    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        may_upload = get_child_node(self, "may_upload")
        mu_text = _("Permitted to upload from a tablet/device")
        may_upload.title = mu_text
        may_upload.label = mu_text
        may_register_devices = get_child_node(self, "may_register_devices")
        mrd_text = _("Permitted to register tablet/client devices")
        may_register_devices.title = mrd_text
        may_register_devices.label = mrd_text
        may_use_webviewer = get_child_node(self, "may_use_webviewer")
        ml_text = _("May log in to web front end")
        may_use_webviewer.title = ml_text
        may_use_webviewer.label = ml_text
        view_all_patients_when_unfiltered = get_child_node(self, "view_all_patients_when_unfiltered")  # noqa
        vap_text = _(
            "May view (browse) records from all patients when no patient "
            "filter set"
        )
        view_all_patients_when_unfiltered.title = vap_text
        view_all_patients_when_unfiltered.label = vap_text
        may_dump_data = get_child_node(self, "may_dump_data")
        md_text = _("May perform bulk data dumps")
        may_dump_data.title = md_text
        may_dump_data.label = md_text
        may_run_reports = get_child_node(self, "may_run_reports")
        mrr_text = _("May run reports")
        may_run_reports.title = mrr_text
        may_run_reports.label = mrr_text
        may_add_notes = get_child_node(self, "may_add_notes")
        man_text = _("May add special notes to tasks")
        may_add_notes.title = man_text
        may_add_notes.label = man_text


class UserGroupPermissionsFullSchema(UserGroupPermissionsGroupAdminSchema):
    """
    Edit group-specific permissions for a user. For superusers; includes the
    option to make the user a groupadmin.
    """
    groupadmin = BooleanNode(default=True)  # match ViewParam.GROUPADMIN and User attribute  # noqa

    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        super().after_bind(node, kw)
        _ = self.gettext
        groupadmin = get_child_node(self, "groupadmin")
        text = _("User is a privileged group administrator for this group")
        groupadmin.title = text
        groupadmin.label = text


class EditUserGroupAdminSchema(CSRFSchema):
    """
    Schema to edit a user. Version for group administrators.
    """
    username = UsernameNode()  # name must match ViewParam.USERNAME and User attribute  # noqa
    fullname = OptionalStringNode(  # name must match ViewParam.FULLNAME and User attribute  # noqa
        validator=Length(0, StringLengths.FULLNAME_MAX_LEN)
    )
    email = OptionalEmailNode()  # name must match ViewParam.EMAIL and User attribute  # noqa
    must_change_password = MustChangePasswordNode()  # match ViewParam.MUST_CHANGE_PASSWORD and User attribute  # noqa
    language = LanguageSelector()  # must match ViewParam.LANGUAGE
    group_ids = AdministeredGroupsSequence()  # must match ViewParam.GROUP_IDS

    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        fullname = get_child_node(self, "fullname")
        fullname.title = _("Full name")
        email = get_child_node(self, "email")
        email.title = _("E-mail address")


class EditUserFullSchema(EditUserGroupAdminSchema):
    """
    Schema to edit a user. Version for superusers; can also make the user a
    superuser.
    """
    superuser = BooleanNode(default=False)  # match ViewParam.SUPERUSER and User attribute  # noqa
    group_ids = AllGroupsSequence()  # must match ViewParam.GROUP_IDS

    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        superuser = get_child_node(self, "superuser")
        text = _("Superuser (CAUTION!)")
        superuser.title = text
        superuser.label = text


class EditUserFullForm(ApplyCancelForm):
    """
    Form to edit a user. Full version for superusers.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        super().__init__(schema_class=EditUserFullSchema,
                         request=request, **kwargs)


class EditUserGroupAdminForm(ApplyCancelForm):
    """
    Form to edit a user. Version for group administrators.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        super().__init__(schema_class=EditUserGroupAdminSchema,
                         request=request, **kwargs)


class EditUserGroupPermissionsFullForm(ApplyCancelForm):
    """
    Form to edit a user's permissions within a group. Version for superusers.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        super().__init__(schema_class=UserGroupPermissionsFullSchema,
                         request=request, **kwargs)


class EditUserGroupMembershipGroupAdminForm(ApplyCancelForm):
    """
    Form to edit a user's permissions within a group. Version for group
    administrators.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        super().__init__(schema_class=UserGroupPermissionsGroupAdminSchema,
                         request=request, **kwargs)


class AddUserSuperuserSchema(CSRFSchema):
    """
    Schema to add a user. Version for superusers.
    """
    username = UsernameNode()  # name must match ViewParam.USERNAME and User attribute  # noqa
    new_password = NewPasswordNode()  # name must match ViewParam.NEW_PASSWORD
    must_change_password = MustChangePasswordNode()  # match ViewParam.MUST_CHANGE_PASSWORD and User attribute  # noqa
    group_ids = AllGroupsSequence()  # must match ViewParam.GROUP_IDS


class AddUserGroupadminSchema(AddUserSuperuserSchema):
    """
    Schema to add a user. Version for group administrators.
    """
    group_ids = AdministeredGroupsSequence()  # must match ViewParam.GROUP_IDS


class AddUserSuperuserForm(AddCancelForm):
    """
    Form to add a user. Version for superusers.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        super().__init__(schema_class=AddUserSuperuserSchema,
                         request=request, **kwargs)


class AddUserGroupadminForm(AddCancelForm):
    """
    Form to add a user. Version for group administrators.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        super().__init__(schema_class=AddUserGroupadminSchema,
                         request=request, **kwargs)


class SetUserUploadGroupSchema(CSRFSchema):
    """
    Schema to choose the group into which a user uploads.
    """
    upload_group_id = OptionalGroupIdSelectorUserGroups()  # must match ViewParam.UPLOAD_GROUP_ID  # noqa

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        upload_group_id = get_child_node(self, "upload_group_id")
        upload_group_id.title = _("Group into which to upload data")
        upload_group_id.description = _(
            "Pick a group from those to which the user belongs")


class SetUserUploadGroupForm(InformativeNonceForm):
    """
    Form to choose the group into which a user uploads.
    """
    def __init__(self, request: "CamcopsRequest", user: "User",
                 **kwargs) -> None:
        _ = request.gettext
        schema = SetUserUploadGroupSchema().bind(request=request,
                                                 user=user)  # UNUSUAL
        super().__init__(
            schema,
            buttons=[
                Button(name=FormAction.SUBMIT, title=_("Set")),
                Button(name=FormAction.CANCEL, title=_("Cancel")),
            ],
            **kwargs
        )


class DeleteUserSchema(HardWorkConfirmationSchema):
    """
    Schema to delete a user.
    """
    user_id = HiddenIntegerNode()  # name must match ViewParam.USER_ID
    danger = TranslatableValidateDangerousOperationNode()


class DeleteUserForm(DeleteCancelForm):
    """
    Form to delete a user.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        super().__init__(schema_class=DeleteUserSchema,
                         request=request, **kwargs)


# =============================================================================
# Add/edit/delete groups
# =============================================================================

class PolicyNode(MandatoryStringNode, RequestAwareMixin):
    """
    Node to capture a CamCOPS ID number policy, and make sure it is
    syntactically valid.
    """
    def validator(self, node: SchemaNode, value: Any) -> None:
        _ = self.gettext
        if not isinstance(value, str):
            # unlikely!
            raise Invalid(node, _("Not a string"))
        policy = TokenizedPolicy(value)
        if not policy.is_syntactically_valid():
            raise Invalid(node, _("Syntactically invalid policy"))
        if not policy.is_valid_for_idnums(self.request.valid_which_idnums):
            raise Invalid(
                node,
                _("Invalid policy. Have you referred to non-existent ID "
                  "numbers? Is the policy less restrictive than the tablet’s "
                  "minimum ID policy?") +
                f" [{TABLET_ID_POLICY_STR!r}]"
            )


class GroupNameNode(MandatoryStringNode, RequestAwareMixin):
    """
    Node to capture a CamCOPS group name, and check it's valid as a string.
    """
    def validator(self, node: SchemaNode, value: str) -> None:
        try:
            validate_group_name(value, self.request)
        except ValueError as e:
            raise Invalid(node, str(e))


class GroupIpUseWidget(Widget):
    basedir = os.path.join(TEMPLATE_DIR, "deform")
    readonlydir = os.path.join(basedir, "readonly")
    form = "group_ip_use.pt"
    template = os.path.join(basedir, form)
    readonly_template = os.path.join(readonlydir, form)

    def __init__(self, request: "CamcopsRequest", **kwargs: Any) -> None:
        super().__init__(**kwargs)
        self.request = request

    def serialize(self,
                  field: "Field",
                  cstruct: Union[Dict[str, Any], None, ColanderNullType],
                  **kw: Any) -> Any:
        if cstruct in (None, null):
            cstruct = {}

        cstruct: Dict[str, Any]  # For type checker

        for context in IpUse.CONTEXTS:
            value = cstruct.get(context, False)
            kw.setdefault(context, value)

        readonly = kw.get("readonly", self.readonly)
        template = readonly and self.readonly_template or self.template
        values = self.get_template_values(field, cstruct, kw)

        _ = self.request.gettext

        values.update(
            introduction=_(
                "These settings will be applied to the patient's device "
                "when operating in single user mode."
            ),
            reason=_(
                "The settings here influence whether CamCOPS will consider "
                "some third-party tasks “permitted” on your behalf, according "
                "to their published use criteria. They do <b>not</b> remove "
                "your responsibility to ensure that you use them in accordance "
                "with their own requirements."
            ),
            warning=_(
                "WARNING. Providing incorrect information here may lead to you "
                "VIOLATING copyright law, by using a task for a purpose that "
                "is not permitted, and being subject to damages and/or "
                "prosecution."
            ),
            disclaimer=_(
                "The authors of CamCOPS cannot be held responsible or liable "
                "for any consequences of you misusing materials subject to "
                "copyright."
            ),
            preamble=_("In which contexts does this group operate?"),
            clinical_label=_("Clinical"),
            medical_device_warning=_(
                "WARNING: NOT FOR GENERAL CLINICAL USE; not a Medical Device; "
                "see Terms and Conditions"
            ),
            commercial_label=_("Commercial"),
            educational_label=_("Educational"),
            research_label=_("Research"),
        )

        return field.renderer(template, **values)

    def deserialize(
            self,
            field: "Field",
            pstruct: Union[Dict[str, Any], ColanderNullType]
    ) -> Dict[str, bool]:
        if pstruct is null:
            pstruct = {}

        pstruct: Dict[str, Any]  # For type checker

        # It doesn't really matter what the pstruct values are. Only the
        # options that are ticked will be present as keys in pstruct
        return {k: k in pstruct for k in IpUse.CONTEXTS}


class IpUseType(object):
    # noinspection PyMethodMayBeStatic,PyUnusedLocal
    def deserialize(
            self,
            node: SchemaNode,
            cstruct: Union[Dict[str, Any], None, ColanderNullType]) \
            -> Optional[IpUse]:
        if cstruct in (None, null):
            return None

        cstruct: Dict[str, Any]  # For type checker

        return IpUse(**cstruct)

    # noinspection PyMethodMayBeStatic,PyUnusedLocal
    def serialize(
            self,
            node: SchemaNode,
            ip_use: Union[IpUse, None, ColanderNullType]) \
            -> Union[Dict, ColanderNullType]:
        if ip_use in [null, None]:
            return null

        return {
            context: getattr(ip_use, context) for context in IpUse.CONTEXTS
        }


class GroupIpUseNode(SchemaNode, RequestAwareMixin):
    schema_type = IpUseType

    # noinspection PyUnusedLocal,PyAttributeOutsideInit
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        self.widget = GroupIpUseWidget(self.request)


class EditGroupSchema(CSRFSchema):
    """
    Schema to edit a group.
    """
    group_id = HiddenIntegerNode()  # must match ViewParam.GROUP_ID
    name = GroupNameNode()  # must match ViewParam.NAME
    description = MandatoryStringNode(  # must match ViewParam.DESCRIPTION
        validator=Length(StringLengths.GROUP_DESCRIPTION_MIN_LEN,
                         StringLengths.GROUP_DESCRIPTION_MAX_LEN),
    )
    ip_use = GroupIpUseNode()

    group_ids = AllOtherGroupsSequence()  # must match ViewParam.GROUP_IDS
    upload_policy = PolicyNode()  # must match ViewParam.UPLOAD_POLICY
    finalize_policy = PolicyNode()  # must match ViewParam.FINALIZE_POLICY

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        name = get_child_node(self, "name")
        name.title = _("Group name")

        ip_use = get_child_node(self, "ip_use")
        ip_use.title = _("Group intellectual property settings")

        group_ids = get_child_node(self, "group_ids")
        group_ids.title = _("Other groups this group may see")
        upload_policy = get_child_node(self, "upload_policy")
        upload_policy.title = _("Upload policy")
        upload_policy.description = _(
            "Minimum required patient information to copy data to server")
        finalize_policy = get_child_node(self, "finalize_policy")
        finalize_policy.title = _("Finalize policy")
        finalize_policy.description = _(
            "Minimum required patient information to clear data off "
            "source device")

    def validator(self, node: SchemaNode, value: Any) -> None:
        request = self.bindings[Binding.REQUEST]  # type: CamcopsRequest
        q = CountStarSpecializedQuery(Group, session=request.dbsession)\
            .filter(Group.id != value[ViewParam.GROUP_ID])\
            .filter(Group.name == value[ViewParam.NAME])
        if q.count_star() > 0:
            _ = request.gettext
            raise Invalid(node, _("Name is used by another group!"))


class EditGroupForm(InformativeNonceForm):
    """
    Form to edit a group.
    """
    def __init__(self, request: "CamcopsRequest", group: Group,
                 **kwargs) -> None:
        _ = request.gettext
        schema = EditGroupSchema().bind(request=request,
                                        group=group)  # UNUSUAL BINDING
        super().__init__(
            schema,
            buttons=[
                Button(name=FormAction.SUBMIT, title=_("Apply")),
                Button(name=FormAction.CANCEL, title=_("Cancel")),
            ],
            **kwargs
        )


class AddGroupSchema(CSRFSchema):
    """
    Schema to add a group.
    """
    name = GroupNameNode()  # name must match ViewParam.NAME

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        name = get_child_node(self, "name")
        name.title = _("Group name")

    def validator(self, node: SchemaNode, value: Any) -> None:
        request = self.bindings[Binding.REQUEST]  # type: CamcopsRequest
        q = CountStarSpecializedQuery(Group, session=request.dbsession)\
            .filter(Group.name == value[ViewParam.NAME])
        if q.count_star() > 0:
            _ = request.gettext
            raise Invalid(node, _("Name is used by another group!"))


class AddGroupForm(AddCancelForm):
    """
    Form to add a group.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        super().__init__(schema_class=AddGroupSchema,
                         request=request, **kwargs)


class DeleteGroupSchema(HardWorkConfirmationSchema):
    """
    Schema to delete a group.
    """
    group_id = HiddenIntegerNode()  # name must match ViewParam.GROUP_ID
    danger = TranslatableValidateDangerousOperationNode()


class DeleteGroupForm(DeleteCancelForm):
    """
    Form to delete a group.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        super().__init__(schema_class=DeleteGroupSchema,
                         request=request, **kwargs)


# =============================================================================
# Offer research dumps
# =============================================================================

class DumpTypeSelector(SchemaNode, RequestAwareMixin):
    """
    Node to select the filtering method for a data dump.
    """
    schema_type = String
    default = ViewArg.EVERYTHING
    missing = ViewArg.EVERYTHING

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.widget = None  # type: Optional[Widget]
        self.validator = None  # type: Optional[ValidatorType]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Dump method")
        choices = (
            (ViewArg.EVERYTHING, _("Everything")),
            (ViewArg.USE_SESSION_FILTER,
             _("Use the session filter settings")),
            (ViewArg.SPECIFIC_TASKS_GROUPS,
             _("Specify tasks/groups manually (see below)")),
        )
        self.widget = RadioChoiceWidget(values=choices)
        self.validator = OneOf(list(x[0] for x in choices))


class SpreadsheetFormatSelector(SchemaNode, RequestAwareMixin):
    """
    Node to select a way of downloading an SQLite database.
    """
    schema_type = String
    default = ViewArg.XLSX
    missing = ViewArg.XLSX

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.widget = None  # type: Optional[Widget]
        self.validator = None  # type: Optional[ValidatorType]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Spreadsheet format")
        choices = (
            (ViewArg.R, _("R script")),
            (ViewArg.ODS, _("OpenOffice spreadsheet (ODS) file")),
            (ViewArg.XLSX, _("XLSX (Microsoft Excel) file")),
            (ViewArg.TSV_ZIP, _("ZIP file of tab-separated value (TSV) files")),
        )
        values, pv = get_values_and_permissible(choices)
        self.widget = RadioChoiceWidget(values=values)
        self.validator = OneOf(pv)


class DeliveryModeNode(SchemaNode, RequestAwareMixin):
    """
    Mode of delivery of data downloads.
    """
    schema_type = String
    default = ViewArg.EMAIL
    missing = ViewArg.EMAIL

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Delivery")
        choices = (
            (ViewArg.IMMEDIATELY, _("Serve immediately")),
            (ViewArg.EMAIL, _("E-mail me")),
            (ViewArg.DOWNLOAD, _("Create a file for me to download")),
        )
        values, pv = get_values_and_permissible(choices)
        self.widget = RadioChoiceWidget(values=values)

    # noinspection PyUnusedLocal
    def validator(self, node: SchemaNode, value: Any) -> None:
        request = self.bindings[Binding.REQUEST]  # type: CamcopsRequest
        _ = request.gettext
        if value == ViewArg.IMMEDIATELY:
            if not request.config.permit_immediate_downloads:
                raise Invalid(
                    self,
                    _("Disabled by the system administrator") +
                    f" [{ConfigParamSite.PERMIT_IMMEDIATE_DOWNLOADS}]"
                )
        elif value == ViewArg.EMAIL:
            if not request.user.email:
                raise Invalid(
                    self, _("Your user does not have an email address"))
        elif value == ViewArg.DOWNLOAD:
            if not request.user_download_dir:
                raise Invalid(
                    self,
                    _("User downloads not configured by administrator") +
                    f" [{ConfigParamSite.USER_DOWNLOAD_DIR}, "
                    f"{ConfigParamSite.USER_DOWNLOAD_MAX_SPACE_MB}]"
                )
        else:
            raise Invalid(self, _("Bad value"))


class SqliteSelector(SchemaNode, RequestAwareMixin):
    """
    Node to select a way of downloading an SQLite database.
    """
    schema_type = String
    default = ViewArg.SQLITE
    missing = ViewArg.SQLITE

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.widget = None  # type: Optional[Widget]
        self.validator = None  # type: Optional[ValidatorType]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Database download method")
        choices = (
            # http://docs.sqlalchemy.org/en/latest/dialects/
            (ViewArg.SQLITE, _("Binary SQLite database")),
            (ViewArg.SQL, _("SQL text to create SQLite database")),
        )
        values, pv = get_values_and_permissible(choices)
        self.widget = RadioChoiceWidget(values=values)
        self.validator = OneOf(pv)


class SortTsvByHeadingsNode(SchemaNode, RequestAwareMixin):
    """
    Boolean node: sort TSV files by column name?
    """
    schema_type = Boolean
    default = False
    missing = False

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.label = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Sort columns?")
        self.label = _("Sort by heading (column) names within spreadsheets?")


class IncludeInformationSchemaColumnsNode(SchemaNode, RequestAwareMixin):
    """
    Boolean node: should INFORMATION_SCHEMA.COLUMNS be included (for
    downloads)?

    False by default -- adds about 350 kb to an ODS download, for example.
    """
    schema_type = Boolean
    default = False
    missing = False

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.label = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Include column information?")
        self.label = _("Include details of all columns in the source database?")  # noqa


class IncludeBlobsNode(SchemaNode, RequestAwareMixin):
    """
    Boolean node: should BLOBs be included (for downloads)?
    """
    schema_type = Boolean
    default = False
    missing = False

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.label = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Include BLOBs?")
        self.label = _(
            "Include binary large objects (BLOBs)? WARNING: may be large")


class PatientIdPerRowNode(SchemaNode, RequestAwareMixin):
    """
    Boolean node: should patient ID information, and other cross-referencing
    denormalized info, be included per row?

    See :ref:`DB_PATIENT_ID_PER_ROW <DB_PATIENT_ID_PER_ROW>`.
    """
    schema_type = Boolean
    default = True
    missing = True

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.label = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Patient ID per row?")
        self.label = _(
            "Include patient ID numbers and task cross-referencing "
            "(denormalized) information per row?")


class OfferDumpManualSchema(Schema, RequestAwareMixin):
    """
    Schema to offer the "manual" settings for a data dump (groups, task types).
    """
    group_ids = AllowedGroupsSequence()  # must match ViewParam.GROUP_IDS
    tasks = MultiTaskSelector()  # must match ViewParam.TASKS

    widget = MappingWidget(template="mapping_accordion", open=False)

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Manual settings")


class OfferBasicDumpSchema(CSRFSchema):
    """
    Schema to choose the settings for a basic (TSV/ZIP) data dump.
    """
    dump_method = DumpTypeSelector()  # must match ViewParam.DUMP_METHOD
    sort = SortTsvByHeadingsNode()  # must match ViewParam.SORT
    include_information_schema_columns = IncludeInformationSchemaColumnsNode()  # must match ViewParam.INCLUDE_INFORMATION_SCHEMA_COLUMNS  # noqa
    manual = OfferDumpManualSchema()  # must match ViewParam.MANUAL
    viewtype = SpreadsheetFormatSelector()  # must match ViewParams.VIEWTYPE  # noqa
    delivery_mode = DeliveryModeNode()  # must match ViewParam.DELIVERY_MODE


class OfferBasicDumpForm(SimpleSubmitForm):
    """
    Form to offer a basic (TSV/ZIP) data dump.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        _ = request.gettext
        super().__init__(schema_class=OfferBasicDumpSchema,
                         submit_title=_("Dump"),
                         request=request, **kwargs)


class OfferSqlDumpSchema(CSRFSchema):
    """
    Schema to choose the settings for an SQL data dump.
    """
    dump_method = DumpTypeSelector()  # must match ViewParam.DUMP_METHOD
    sqlite_method = SqliteSelector()  # must match ViewParam.SQLITE_METHOD
    include_information_schema_columns = IncludeInformationSchemaColumnsNode()  # must match ViewParam.INCLUDE_INFORMATION_SCHEMA_COLUMNS  # noqa
    include_blobs = IncludeBlobsNode()  # must match ViewParam.INCLUDE_BLOBS
    patient_id_per_row = PatientIdPerRowNode()  # must match ViewParam.PATIENT_ID_PER_ROW  # noqa
    manual = OfferDumpManualSchema()  # must match ViewParam.MANUAL
    delivery_mode = DeliveryModeNode()  # must match ViewParam.DELIVERY_MODE


class OfferSqlDumpForm(SimpleSubmitForm):
    """
    Form to choose the settings for an SQL data dump.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        _ = request.gettext
        super().__init__(schema_class=OfferSqlDumpSchema,
                         submit_title=_("Dump"),
                         request=request, **kwargs)


# =============================================================================
# Edit server settings
# =============================================================================

class EditServerSettingsSchema(CSRFSchema):
    """
    Schema to edit the global settings for the server.
    """
    database_title = SchemaNode(  # must match ViewParam.DATABASE_TITLE
        String(),
        validator=Length(StringLengths.DATABASE_TITLE_MIN_LEN,
                         StringLengths.DATABASE_TITLE_MAX_LEN),
    )

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        database_title = get_child_node(self, "database_title")
        database_title.title = _("Database friendly title")


class EditServerSettingsForm(ApplyCancelForm):
    """
    Form to edit the global settings for the server.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        super().__init__(schema_class=EditServerSettingsSchema,
                         request=request, **kwargs)


# =============================================================================
# Edit ID number definitions
# =============================================================================

class IdDefinitionDescriptionNode(SchemaNode, RequestAwareMixin):
    """
    Node to capture the description of an ID number type.
    """
    schema_type = String
    validator = Length(1, StringLengths.ID_DESCRIPTOR_MAX_LEN)

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Full description (e.g. “NHS number”)")


class IdDefinitionShortDescriptionNode(SchemaNode, RequestAwareMixin):
    """
    Node to capture the short description of an ID number type.
    """
    schema_type = String
    validator = Length(1, StringLengths.ID_DESCRIPTOR_MAX_LEN)

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.description = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Short description (e.g. “NHS#”)")
        self.description = _("Try to keep it very short!")


class IdValidationMethodNode(OptionalStringNode, RequestAwareMixin):
    """
    Node to choose a build-in ID number validation method.
    """
    widget = SelectWidget(values=ID_NUM_VALIDATION_METHOD_CHOICES)
    validator = OneOf(list(x[0] for x in ID_NUM_VALIDATION_METHOD_CHOICES))

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.description = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Validation method")
        self.description = _("Built-in CamCOPS ID number validation method")


class Hl7AssigningAuthorityNode(OptionalStringNode, RequestAwareMixin):
    """
    Optional node to capture the name of an HL7 Assigning Authority.
    """
    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.description = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("HL7 Assigning Authority")
        self.description = _(
            "For HL7 messaging: "
            "HL7 Assigning Authority for ID number (unique name of the "
            "system/organization/agency/department that creates the data)."
        )

    # noinspection PyMethodMayBeStatic
    def validator(self, node: SchemaNode, value: str) -> None:
        try:
            validate_hl7_aa(value, self.request)
        except ValueError as e:
            raise Invalid(node, str(e))


class Hl7IdTypeNode(OptionalStringNode, RequestAwareMixin):
    """
    Optional node to capture the name of an HL7 Identifier Type code.
    """
    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.description = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("HL7 Identifier Type")
        self.description = _(
            "For HL7 messaging: "
            "HL7 Identifier Type code: ‘a code corresponding to the type "
            "of identifier. In some cases, this code may be used as a "
            "qualifier to the “Assigning Authority” component.’"
        )

    # noinspection PyMethodMayBeStatic
    def validator(self, node: SchemaNode, value: str) -> None:
        try:
            validate_hl7_id_type(value, self.request)
        except ValueError as e:
            raise Invalid(node, str(e))


class EditIdDefinitionSchema(CSRFSchema):
    """
    Schema to edit an ID number definition.
    """
    which_idnum = HiddenIntegerNode()  # must match ViewParam.WHICH_IDNUM
    description = IdDefinitionDescriptionNode()  # must match ViewParam.DESCRIPTION  # noqa
    short_description = IdDefinitionShortDescriptionNode()  # must match ViewParam.SHORT_DESCRIPTION  # noqa
    validation_method = IdValidationMethodNode()  # must match ViewParam.VALIDATION_METHOD  # noqa
    hl7_id_type = Hl7IdTypeNode()  # must match ViewParam.HL7_ID_TYPE
    hl7_assigning_authority = Hl7AssigningAuthorityNode()  # must match ViewParam.HL7_ASSIGNING_AUTHORITY  # noqa

    def validator(self, node: SchemaNode, value: Any) -> None:
        request = self.bindings[Binding.REQUEST]  # type: CamcopsRequest
        _ = request.gettext
        qd = CountStarSpecializedQuery(IdNumDefinition,
                                       session=request.dbsession)\
            .filter(IdNumDefinition.which_idnum !=
                    value[ViewParam.WHICH_IDNUM])\
            .filter(IdNumDefinition.description ==
                    value[ViewParam.DESCRIPTION])
        if qd.count_star() > 0:
            raise Invalid(node, _("Description is used by another ID number!"))
        qs = CountStarSpecializedQuery(IdNumDefinition,
                                       session=request.dbsession)\
            .filter(IdNumDefinition.which_idnum !=
                    value[ViewParam.WHICH_IDNUM])\
            .filter(IdNumDefinition.short_description ==
                    value[ViewParam.SHORT_DESCRIPTION])
        if qs.count_star() > 0:
            raise Invalid(node,
                          _("Short description is used by another ID number!"))


class EditIdDefinitionForm(ApplyCancelForm):
    """
    Form to edit an ID number definition.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        super().__init__(schema_class=EditIdDefinitionSchema,
                         request=request, **kwargs)


class AddIdDefinitionSchema(CSRFSchema):
    """
    Schema to add an ID number definition.
    """
    which_idnum = SchemaNode(  # must match ViewParam.WHICH_IDNUM
        Integer(),
        validator=Range(min=1)
    )
    description = IdDefinitionDescriptionNode()  # must match ViewParam.DESCRIPTION  # noqa
    short_description = IdDefinitionShortDescriptionNode()  # must match ViewParam.SHORT_DESCRIPTION  # noqa
    validation_method = IdValidationMethodNode()  # must match ViewParam.VALIDATION_METHOD  # noqa

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        which_idnum = get_child_node(self, "which_idnum")
        which_idnum.title = _("Which ID number?")
        which_idnum.description = (
            "Specify the integer to represent the type of this ID "
            "number class (e.g. consecutive numbering from 1)"
        )

    def validator(self, node: SchemaNode, value: Any) -> None:
        request = self.bindings[Binding.REQUEST]  # type: CamcopsRequest
        _ = request.gettext
        qw = (
            CountStarSpecializedQuery(IdNumDefinition,
                                      session=request.dbsession)
            .filter(IdNumDefinition.which_idnum ==
                    value[ViewParam.WHICH_IDNUM])
        )
        if qw.count_star() > 0:
            raise Invalid(node, _("ID# clashes with another ID number!"))
        qd = (
            CountStarSpecializedQuery(IdNumDefinition,
                                      session=request.dbsession)
            .filter(IdNumDefinition.description ==
                    value[ViewParam.DESCRIPTION])
        )
        if qd.count_star() > 0:
            raise Invalid(node, _("Description is used by another ID number!"))
        qs = (
            CountStarSpecializedQuery(IdNumDefinition,
                                      session=request.dbsession)
            .filter(IdNumDefinition.short_description ==
                    value[ViewParam.SHORT_DESCRIPTION])
        )
        if qs.count_star() > 0:
            raise Invalid(node,
                          _("Short description is used by another ID number!"))


class AddIdDefinitionForm(AddCancelForm):
    """
    Form to add an ID number definition.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        super().__init__(schema_class=AddIdDefinitionSchema,
                         request=request, **kwargs)


class DeleteIdDefinitionSchema(HardWorkConfirmationSchema):
    """
    Schema to delete an ID number definition.
    """
    which_idnum = HiddenIntegerNode()  # name must match ViewParam.WHICH_IDNUM
    danger = TranslatableValidateDangerousOperationNode()


class DeleteIdDefinitionForm(DangerousForm):
    """
    Form to add an ID number definition.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        _ = request.gettext
        super().__init__(schema_class=DeleteIdDefinitionSchema,
                         submit_action=FormAction.DELETE,
                         submit_title=_("Delete"),
                         request=request, **kwargs)


# =============================================================================
# Special notes
# =============================================================================

class AddSpecialNoteSchema(CSRFSchema):
    """
    Schema to add a special note to a task.
    """
    table_name = HiddenStringNode()  # must match ViewParam.TABLENAME
    server_pk = HiddenIntegerNode()  # must match ViewParam.SERVER_PK
    note = MandatoryStringNode(  # must match ViewParam.NOTE
        widget=TextAreaWidget(rows=20, cols=80)
    )
    danger = TranslatableValidateDangerousOperationNode()


class AddSpecialNoteForm(DangerousForm):
    """
    Form to add a special note to a task.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        _ = request.gettext
        super().__init__(schema_class=AddSpecialNoteSchema,
                         submit_action=FormAction.SUBMIT,
                         submit_title=_("Add"),
                         request=request, **kwargs)


class DeleteSpecialNoteSchema(CSRFSchema):
    """
    Schema to add a special note to a task.
    """
    note_id = HiddenIntegerNode()  # must match ViewParam.NOTE_ID
    danger = TranslatableValidateDangerousOperationNode()


class DeleteSpecialNoteForm(DangerousForm):
    """
    Form to delete (hide) a special note.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        _ = request.gettext
        super().__init__(schema_class=DeleteSpecialNoteSchema,
                         submit_action=FormAction.SUBMIT,
                         submit_title=_("Delete"),
                         request=request, **kwargs)


# =============================================================================
# The unusual data manipulation operations
# =============================================================================

class EraseTaskSchema(HardWorkConfirmationSchema):
    """
    Schema to erase a task.
    """
    table_name = HiddenStringNode()  # must match ViewParam.TABLENAME
    server_pk = HiddenIntegerNode()  # must match ViewParam.SERVER_PK
    danger = TranslatableValidateDangerousOperationNode()


class EraseTaskForm(DangerousForm):
    """
    Form to erase a task.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        _ = request.gettext
        super().__init__(schema_class=EraseTaskSchema,
                         submit_action=FormAction.DELETE,
                         submit_title=_("Erase"),
                         request=request, **kwargs)


class DeletePatientChooseSchema(CSRFSchema):
    """
    Schema to delete a patient.
    """
    which_idnum = MandatoryWhichIdNumSelector()  # must match ViewParam.WHICH_IDNUM  # noqa
    idnum_value = MandatoryIdNumValue()  # must match ViewParam.IDNUM_VALUE
    group_id = MandatoryGroupIdSelectorAdministeredGroups()  # must match ViewParam.GROUP_ID  # noqa
    danger = TranslatableValidateDangerousOperationNode()


class DeletePatientChooseForm(DangerousForm):
    """
    Form to delete a patient.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        _ = request.gettext
        super().__init__(schema_class=DeletePatientChooseSchema,
                         submit_action=FormAction.SUBMIT,
                         submit_title=_("Show tasks that will be deleted"),
                         request=request, **kwargs)


class DeletePatientConfirmSchema(HardWorkConfirmationSchema):
    """
    Schema to confirm deletion of a patient.
    """
    which_idnum = HiddenIntegerNode()  # must match ViewParam.WHICH_IDNUM
    idnum_value = HiddenIntegerNode()  # must match ViewParam.IDNUM_VALUE
    group_id = HiddenIntegerNode()  # must match ViewParam.GROUP_ID
    danger = TranslatableValidateDangerousOperationNode()


class DeletePatientConfirmForm(DangerousForm):
    """
    Form to confirm deletion of a patient.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        _ = request.gettext
        super().__init__(schema_class=DeletePatientConfirmSchema,
                         submit_action=FormAction.DELETE,
                         submit_title=_("Delete"),
                         request=request, **kwargs)


class DeleteServerCreatedPatientSchema(HardWorkConfirmationSchema):
    """
    Schema to delete a patient created on the server.
    """
    # name must match ViewParam.SERVER_PK
    server_pk = HiddenIntegerNode()
    danger = TranslatableValidateDangerousOperationNode()


class DeleteServerCreatedPatientForm(DeleteCancelForm):
    """
    Form to delete a patient created on the server
    """
    def __init__(self, request: "CamcopsRequest", **kwargs: Any) -> None:
        super().__init__(schema_class=DeleteServerCreatedPatientSchema,
                         request=request, **kwargs)


EDIT_PATIENT_SIMPLE_PARAMS = [
    ViewParam.FORENAME,
    ViewParam.SURNAME,
    ViewParam.DOB,
    ViewParam.SEX,
    ViewParam.ADDRESS,
    ViewParam.EMAIL,
    ViewParam.GP,
    ViewParam.OTHER,
]


class TaskScheduleSelector(SchemaNode, RequestAwareMixin):
    """
    Drop-down with all available task schedules
    """
    widget = SelectWidget()

    def __init__(self, *args: Any, **kwargs: Any) -> None:
        self.title = ""  # for type checker
        self.name = ""  # for type checker
        self.validator = None  # type: Optional[ValidatorType]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        request = self.request
        _ = request.gettext
        self.title = _("Task schedule")
        values = []  # type: List[Tuple[Optional[int], str]]

        task_schedules = (
            request.dbsession.query(TaskSchedule)
            .order_by(TaskSchedule.name)
        )

        for task_schedule in task_schedules:
            values.append((task_schedule.id, task_schedule.name))
        values, pv = get_values_and_permissible(values, add_none=False)

        self.widget.values = values
        self.validator = OneOf(pv)

    @staticmethod
    def schema_type() -> SchemaType:
        return Integer()


class JsonType(object):
    """
    Schema type for JsonNode
    """
    # noinspection PyMethodMayBeStatic, PyUnusedLocal
    def deserialize(self, node: SchemaNode,
                    cstruct: Union[str, ColanderNullType, None]) -> Any:
        # is null when form is empty
        if cstruct in (null, None):
            return None

        cstruct: str

        try:
            # Validation happens on the widget class
            json_value = json.loads(cstruct)
        except json.JSONDecodeError:
            return None

        return json_value

    # noinspection PyMethodMayBeStatic,PyUnusedLocal
    def serialize(
            self,
            node: SchemaNode,
            appstruct: Union[Dict, None, ColanderNullType]) \
            -> Union[str, ColanderNullType]:
        # is null when form is empty (new record)
        # is None when populated from empty value in the database
        if appstruct in (null, None):
            return null

        # appstruct should be well formed here (it would already have failed
        # when reading from the database)
        return json.dumps(appstruct)


class JsonWidget(Widget):
    """
    Widget supporting jsoneditor https://github.com/josdejong/jsoneditor
    """
    basedir = os.path.join(TEMPLATE_DIR, "deform")
    readonlydir = os.path.join(basedir, "readonly")
    form = "json.pt"
    template = os.path.join(basedir, form)
    readonly_template = os.path.join(readonlydir, form)
    requirements = (('jsoneditor', None),)

    def __init__(self, request: "CamcopsRequest", **kwargs: Any) -> None:
        super().__init__(**kwargs)
        self.request = request

    def serialize(
        self, field: "Field", cstruct: Union[str, ColanderNullType], **kw: Any
    ) -> Any:
        if cstruct is null:
            cstruct = ""

        readonly = kw.get('readonly', self.readonly)
        template = readonly and self.readonly_template or self.template

        values = self.get_template_values(field, cstruct, kw)

        return field.renderer(template, **values)

    def deserialize(
        self, field: "Field", pstruct: Union[str, ColanderNullType]
    ) -> Union[str, ColanderNullType]:
        # is empty string when field is empty
        if pstruct in (null, ""):
            return null

        _ = self.request.gettext
        error_message = _("Please enter valid JSON or leave blank")

        pstruct: str

        try:
            json.loads(pstruct)
        except json.JSONDecodeError:
            raise Invalid(field, error_message, pstruct)

        return pstruct


class JsonNode(SchemaNode, RequestAwareMixin):
    schema_type = JsonType
    missing = null

    # noinspection PyUnusedLocal,PyAttributeOutsideInit
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        self.widget = JsonWidget(self.request)


class TaskScheduleNode(MappingSchema, RequestAwareMixin):
    schedule_id = TaskScheduleSelector()  # must match ViewParam.SCHEDULE_ID  # noqa: E501
    # must match ViewParam.START_DATETIME
    start_datetime = StartPendulumSelector()
    settings = JsonNode()  # must match ViewParam.SETTINGS

    def __init__(self, *args: Any, **kwargs: Any) -> None:
        self.title = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Task schedule")
        start_datetime = get_child_node(self, "start_datetime")
        start_datetime.description = _(
            "Leave blank for the date the patient first downloads the schedule"
        )
        settings = get_child_node(self, "settings")
        settings.title = _("Task-specific settings for this patient")
        settings.description = _(
            "ADVANCED. Only applicable to tasks that are configurable on a "
            "per-patient basis. Format: JSON object, with settings keyed on "
            "task table name."
        )

    def validator(self, node: SchemaNode, value: Any) -> None:
        settings_value = value["settings"]

        if settings_value is not None:
            # will be None if JSON failed to validate
            if not isinstance(settings_value, dict):
                _ = self.request.gettext
                error_message = _(
                    "Please enter a valid JSON object (with settings keyed on "
                    "task table name) or leave blank"
                )

                raise Invalid(node, error_message)


class TaskScheduleSequence(SequenceSchema, RequestAwareMixin):
    task_schedule_sequence = TaskScheduleNode()
    missing = drop

    def __init__(self, *args: Any, **kwargs: Any) -> None:
        self.title = ""  # for type checker
        self.widget = None  # type: Optional[Widget]
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Task Schedules")
        self.widget = TranslatableSequenceWidget(request=self.request)


class EditPatientSchema(CSRFSchema):
    """
    Schema to edit a patient.
    """
    server_pk = HiddenIntegerNode()  # must match ViewParam.SERVER_PK
    forename = OptionalPatientNameNode()  # must match ViewParam.FORENAME
    surname = OptionalPatientNameNode()  # must match ViewParam.SURNAME
    dob = DateSelectorNode()  # must match ViewParam.DOB
    sex = MandatorySexSelector()  # must match ViewParam.SEX
    address = OptionalStringNode()  # must match ViewParam.ADDRESS
    email = OptionalEmailNode()  # must match ViewParam.EMAIL
    gp = OptionalStringNode()  # must match ViewParam.GP
    other = OptionalStringNode()  # must match ViewParam.OTHER
    id_references = IdNumSequenceUniquePerWhichIdnum()  # must match ViewParam.ID_REFERENCES  # noqa

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        dob = get_child_node(self, "dob")
        dob.title = _("Date of birth")
        gp = get_child_node(self, "gp")
        gp.title = _("GP")

    def validator(self, node: SchemaNode, value: Any) -> None:
        request = self.bindings[Binding.REQUEST]  # type: CamcopsRequest
        dbsession = request.dbsession
        group_id = value[ViewParam.GROUP_ID]
        group = Group.get_group_by_id(dbsession, group_id)
        testpatient = Patient()
        for k in EDIT_PATIENT_SIMPLE_PARAMS:
            setattr(testpatient, k, value[k])
        testpatient.idnums = []
        for idrefdict in value[ViewParam.ID_REFERENCES]:
            pidnum = PatientIdNum()
            pidnum.which_idnum = idrefdict[ViewParam.WHICH_IDNUM]
            pidnum.idnum_value = idrefdict[ViewParam.IDNUM_VALUE]
            testpatient.idnums.append(pidnum)
        tk_finalize_policy = TokenizedPolicy(group.finalize_policy)
        if not testpatient.satisfies_id_policy(tk_finalize_policy):
            _ = self.gettext
            raise Invalid(
                node,
                _("Patient would not meet 'finalize' ID policy for group:")
                + f" {group.name}! [" +
                _("That policy is:") +
                f" {group.finalize_policy!r}]"
            )


class DangerousEditPatientSchema(EditPatientSchema):
    group_id = HiddenIntegerNode()  # must match ViewParam.GROUP_ID
    danger = TranslatableValidateDangerousOperationNode()


class EditServerCreatedPatientSchema(EditPatientSchema):
    # Must match ViewParam.GROUP_ID
    group_id = MandatoryGroupIdSelectorAdministeredGroups(
        insert_before="forename"
    )
    task_schedules = TaskScheduleSequence()  # must match ViewParam.TASK_SCHEDULES  # noqa: E501


class EditFinalizedPatientForm(DangerousForm):
    """
    Form to edit a finalized patient.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs: Any) -> None:
        _ = request.gettext
        super().__init__(schema_class=DangerousEditPatientSchema,
                         submit_action=FormAction.SUBMIT,
                         submit_title=_("Submit"),
                         request=request, **kwargs)


class EditServerCreatedPatientForm(DynamicDescriptionsNonceForm):
    """
    Form to add or edit a patient not yet on the device (for scheduled tasks)
    """
    def __init__(self, request: "CamcopsRequest", **kwargs: Any) -> None:
        schema = EditServerCreatedPatientSchema().bind(request=request)
        _ = request.gettext
        super().__init__(
            schema,
            request=request,
            buttons=[
                Button(name=FormAction.SUBMIT, title=_("Submit"),
                       css_class="btn-danger"),
                Button(name=FormAction.CANCEL, title=_("Cancel")),
            ],
            **kwargs
        )


class EmailTemplateNode(OptionalStringNode, RequestAwareMixin):
    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.description = ""  # for type checker
        self.formatter = TaskScheduleEmailTemplateFormatter()
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Email template")
        self.description = _(
            "Template of email to be sent to patients when inviting them to "
            "complete the tasks in the schedule. Valid placeholders: {}"
        ).format(self.formatter.get_valid_parameters_string())

        # noinspection PyAttributeOutsideInit
        self.widget = TextAreaWidget(rows=20, cols=80)

    def validator(self, node: SchemaNode, value: Any) -> None:
        _ = self.gettext

        try:
            self.formatter.validate(value)
            return
        except KeyError as e:
            error = _("{bad_key} is not a valid placeholder").format(
                bad_key=e,
            )
        except ValueError:
            error = _(
                "Invalid email template. Is there a missing '{' or '}' ?"
            )

        raise Invalid(node, error)


class TaskScheduleSchema(CSRFSchema):
    name = OptionalStringNode()
    group_id = MandatoryGroupIdSelectorAdministeredGroups()  # must match ViewParam.GROUP_ID  # noqa
    email_subject = OptionalStringNode()
    email_template = EmailTemplateNode()


class EditTaskScheduleForm(DynamicDescriptionsNonceForm):
    def __init__(self, request: "CamcopsRequest", **kwargs: Any) -> None:
        schema = TaskScheduleSchema().bind(request=request)
        _ = request.gettext
        super().__init__(
            schema,
            request=request,
            buttons=[
                Button(name=FormAction.SUBMIT, title=_("Submit"),
                       css_class="btn-danger"),
                Button(name=FormAction.CANCEL, title=_("Cancel")),
            ],
            **kwargs
        )


class DeleteTaskScheduleSchema(HardWorkConfirmationSchema):
    """
    Schema to delete a task schedule.
    """
    # name must match ViewParam.SCHEDULE_ID
    schedule_id = HiddenIntegerNode()
    danger = TranslatableValidateDangerousOperationNode()


class DeleteTaskScheduleForm(DeleteCancelForm):
    """
    Form to delete a task schedule.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs: Any) -> None:
        super().__init__(schema_class=DeleteTaskScheduleSchema,
                         request=request, **kwargs)


class DurationWidget(Widget):
    """
    Widget for entering a duration as a number of months, weeks and days.
    The default template renders three text input fields.
    Total days = (months * 30) + (weeks * 7) + days.
    """
    basedir = os.path.join(TEMPLATE_DIR, "deform")
    readonlydir = os.path.join(basedir, "readonly")
    form = "duration.pt"
    template = os.path.join(basedir, form)
    readonly_template = os.path.join(readonlydir, form)

    def __init__(self, request: "CamcopsRequest", **kwargs: Any) -> None:
        super().__init__(**kwargs)
        self.request = request

    def serialize(self,
                  field: "Field",
                  cstruct: Union[Dict[str, Any], None, ColanderNullType],
                  **kw: Any) -> Any:
        # called when rendering the form with values from DurationType.serialize
        if cstruct in (None, null):
            cstruct = {}

        cstruct: Dict[str, Any]

        months = cstruct.get("months", "")
        weeks = cstruct.get("weeks", "")
        days = cstruct.get("days", "")

        kw.setdefault("months", months)
        kw.setdefault("weeks", weeks)
        kw.setdefault("days", days)

        readonly = kw.get("readonly", self.readonly)
        template = readonly and self.readonly_template or self.template
        values = self.get_template_values(field, cstruct, kw)

        _ = self.request.gettext

        values.update(
            weeks_placeholder=_("1 week = 7 days"),
            months_placeholder=_("1 month = 30 days"),
            months_label=_("Months"),
            weeks_label=_("Weeks"),
            days_label=_("Days"),
        )

        return field.renderer(template, **values)

    def deserialize(
            self,
            field: "Field",
            pstruct: Union[Dict[str, Any], ColanderNullType]
    ) -> Dict[str, int]:
        # called when validating the form on submission
        # value is passed to the schema deserialize()

        if pstruct is null:
            pstruct = {}

        pstruct: Dict[str, Any]

        errors = []

        try:
            days = int(pstruct.get("days") or "0")
        except ValueError:
            errors.append("Please enter a valid number of days or leave blank")

        try:
            weeks = int(pstruct.get("weeks") or "0")
        except ValueError:
            errors.append("Please enter a valid number of weeks or leave blank")

        try:
            months = int(pstruct.get("months") or "0")
        except ValueError:
            errors.append(
                "Please enter a valid number of months or leave blank"
            )

        if len(errors) > 0:
            raise Invalid(field, errors, pstruct)

        # noinspection PyUnboundLocalVariable
        return {
            "days": days,
            "months": months,
            "weeks": weeks,
        }


class DurationType(object):
    """
    Custom colander schema type to convert between Pendulum Duration objects
    and months, weeks and days.
    """

    # noinspection PyMethodMayBeStatic,PyUnusedLocal
    def deserialize(
            self,
            node: SchemaNode,
            cstruct: Union[Dict[str, Any], None, ColanderNullType]) \
            -> Optional[Duration]:
        # called when validating the submitted form with the total days
        # from DurationWidget.deserialize()
        if cstruct in (None, null):
            return None

        cstruct: Dict[str, Any]

        # may be passed invalid values when re-rendering widget with error
        # messages
        try:
            days = int(cstruct.get("days") or "0")
        except ValueError:
            days = 0

        try:
            weeks = int(cstruct.get("weeks") or "0")
        except ValueError:
            weeks = 0

        try:
            months = int(cstruct.get("months") or "0")
        except ValueError:
            months = 0

        total_days = months * 30 + weeks * 7 + days

        return Duration(days=total_days)

    # noinspection PyMethodMayBeStatic,PyUnusedLocal
    def serialize(
            self,
            node: SchemaNode,
            duration: Union[Duration, ColanderNullType]) \
            -> Union[Dict, ColanderNullType]:
        if duration is null:
            # For new schedule item
            return null

        duration: Duration

        total_days = duration.in_days()

        months = total_days // 30
        weeks = (total_days % 30) // 7
        days = (total_days % 30) % 7

        # Existing schedule item
        cstruct = {
            "days": days,
            "months": months,
            "weeks": weeks,
        }

        return cstruct


class DurationNode(SchemaNode, RequestAwareMixin):
    schema_type = DurationType

    # noinspection PyUnusedLocal,PyAttributeOutsideInit
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        self.widget = DurationWidget(self.request)


class TaskScheduleItemSchema(CSRFSchema):
    schedule_id = HiddenIntegerNode()  # name must match ViewParam.SCHEDULE_ID
    # name must match ViewParam.TABLE_NAME
    table_name = MandatorySingleTaskSelector()
    # name must match ViewParam.CLINICIAN_CONFIRMATION
    clinician_confirmation = BooleanNode(default=False)
    due_from = DurationNode()  # name must match ViewParam.DUE_FROM
    due_within = DurationNode()  # name must match ViewParam.DUE_WITHIN

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        due_from = get_child_node(self, "due_from")
        due_from.title = _("Due from")
        due_from.description = _(
            "Time from the start of schedule when the patient may begin this "
            "task"
        )
        due_within = get_child_node(self, "due_within")
        due_within.title = _("Due within")
        due_within.description = _(
            "Time the patient has to complete this task"
        )
        clinician_confirmation = get_child_node(self, "clinician_confirmation")
        clinician_confirmation.title = _("Allow clinician tasks")
        clinician_confirmation.label = None
        clinician_confirmation.description = _(
            "Tick this box to schedule a task that would normally be completed "
            "by (or with) a clinician"
        )

    def validator(self, node: SchemaNode, value: Dict[str, Any]) -> None:
        task_class = self._get_task_class(value)

        self._validate_clinician_status(node, value, task_class)
        self._validate_due_dates(node, value)
        self._validate_task_ip_use(node, value, task_class)

    # noinspection PyMethodMayBeStatic
    def _get_task_class(self, value: Dict[str, Any]) -> Type["Task"]:
        return tablename_to_task_class_dict()[value[ViewParam.TABLE_NAME]]

    def _validate_clinician_status(self,
                                   node: SchemaNode,
                                   value: Dict[str, Any],
                                   task_class: Type["Task"]) -> None:

        _ = self.gettext
        clinician_confirmation = value[ViewParam.CLINICIAN_CONFIRMATION]
        if task_class.has_clinician and not clinician_confirmation:
            raise Invalid(
                node,
                _(
                    "You have selected the task '{task_name}', which a "
                    "patient would not normally complete by themselves. "
                    "If you are sure you want to do this, you must tick "
                    "'Allow clinician tasks'."
                ).format(task_name=task_class.shortname)
            )

    def _validate_due_dates(self,
                            node: SchemaNode,
                            value: Dict[str, Any]) -> None:
        _ = self.gettext
        due_from = value[ViewParam.DUE_FROM]
        if due_from.total_days() < 0:
            raise Invalid(
                node,
                _("'Due from' must be zero or more days"),
            )

        due_within = value[ViewParam.DUE_WITHIN]
        if due_within.total_days() <= 0:
            raise Invalid(
                node,
                _("'Due within' must be more than zero days"),
            )

    def _validate_task_ip_use(self,
                              node: SchemaNode,
                              value: Dict[str, Any],
                              task_class: Type["Task"]) -> None:

        _ = self.gettext

        if not task_class.prohibits_anything():
            return

        schedule_id = value[ViewParam.SCHEDULE_ID]
        schedule = self.request.dbsession.query(TaskSchedule).filter(
            TaskSchedule.id == schedule_id
        ).one()

        if schedule.group.ip_use is None:
            raise Invalid(
                node, _(
                    "The task you have selected prohibits use in certain "
                    "contexts. The group '{group_name}' has no intellectual "
                    "property settings. "
                    "You need to edit the group '{group_name}' to say which "
                    "contexts it operates in.".format(
                        group_name=schedule.group.name
                    )
                )
            )

        # TODO: One the client we say 'to use this task, you must seek
        # permission from the copyright holder'. We could do the same but at the
        # moment there isn't a way of telling the system that we have done so.
        if task_class.prohibits_commercial and schedule.group.ip_use.commercial:
            raise Invalid(
                node,
                _("The group '{group_name}' associated with schedule "
                  "'{schedule_name}' operates in a "
                  "commercial context but the task you have selected "
                  "prohibits commercial use.").format(
                      group_name=schedule.group.name,
                      schedule_name=schedule.name
                  )
            )

        if task_class.prohibits_clinical and schedule.group.ip_use.clinical:
            raise Invalid(
                node,
                _("The group '{group_name}' associated with schedule "
                  "'{schedule_name}' operates in a "
                  "clinical context but the task you have selected "
                  "prohibits clinical use.").format(
                      group_name=schedule.group.name,
                      schedule_name=schedule.name
                  )
            )

        if task_class.prohibits_educational and schedule.group.ip_use.educational:  # noqa
            raise Invalid(
                node,
                _("The group '{group_name}' associated with schedule "
                  "'{schedule_name}' operates in an "
                  "educational context but the task you have selected "
                  "prohibits educational use.").format(
                      group_name=schedule.group.name,
                      schedule_name=schedule.name
                  )
            )

        if task_class.prohibits_research and schedule.group.ip_use.research:
            raise Invalid(
                node,
                _("The group '{group_name}' associated with schedule "
                  "'{schedule_name}' operates in a "
                  "research context but the task you have selected "
                  "prohibits research use.").format(
                      group_name=schedule.group.name,
                      schedule_name=schedule.name
                  )
            )


class EditTaskScheduleItemForm(DynamicDescriptionsNonceForm):
    def __init__(self, request: "CamcopsRequest", **kwargs: Any) -> None:
        schema = TaskScheduleItemSchema().bind(request=request)
        _ = request.gettext
        super().__init__(
            schema,
            request=request,
            buttons=[
                Button(name=FormAction.SUBMIT, title=_("Submit"),
                       css_class="btn-danger"),
                Button(name=FormAction.CANCEL, title=_("Cancel")),
            ],
            **kwargs
        )


class DeleteTaskScheduleItemSchema(HardWorkConfirmationSchema):
    """
    Schema to delete a task schedule item.
    """
    # name must match ViewParam.SCHEDULE_ITEM_ID
    schedule_item_id = HiddenIntegerNode()
    danger = TranslatableValidateDangerousOperationNode()


class DeleteTaskScheduleItemForm(DeleteCancelForm):
    """
    Form to delete a task schedule item.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs: Any) -> None:
        super().__init__(schema_class=DeleteTaskScheduleItemSchema,
                         request=request, **kwargs)


class ForciblyFinalizeChooseDeviceSchema(CSRFSchema):
    """
    Schema to force-finalize records from a device.
    """
    device_id = MandatoryDeviceIdSelector()  # must match ViewParam.DEVICE_ID
    danger = TranslatableValidateDangerousOperationNode()


class ForciblyFinalizeChooseDeviceForm(SimpleSubmitForm):
    """
    Form to force-finalize records from a device.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        _ = request.gettext
        super().__init__(schema_class=ForciblyFinalizeChooseDeviceSchema,
                         submit_title=_("View affected tasks"),
                         request=request, **kwargs)


class ForciblyFinalizeConfirmSchema(HardWorkConfirmationSchema):
    """
    Schema to confirm force-finalizing of a device.
    """
    device_id = HiddenIntegerNode()  # must match ViewParam.DEVICE_ID
    danger = TranslatableValidateDangerousOperationNode()


class ForciblyFinalizeConfirmForm(DangerousForm):
    """
    Form to confirm force-finalizing of a device.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        _ = request.gettext
        super().__init__(schema_class=ForciblyFinalizeConfirmSchema,
                         submit_action=FormAction.FINALIZE,
                         submit_title=_("Forcibly finalize"),
                         request=request, **kwargs)


# =============================================================================
# User downloads
# =============================================================================

class UserDownloadDeleteSchema(CSRFSchema):
    """
    Schema to capture details of a file to be deleted.
    """
    filename = HiddenStringNode()  # name must match ViewParam.FILENAME


class UserDownloadDeleteForm(SimpleSubmitForm):
    """
    Form that provides a single button to delete a user download.
    """
    def __init__(self, request: "CamcopsRequest", **kwargs) -> None:
        _ = request.gettext
        super().__init__(schema_class=UserDownloadDeleteSchema,
                         submit_title=_("Delete"),
                         request=request, **kwargs)
