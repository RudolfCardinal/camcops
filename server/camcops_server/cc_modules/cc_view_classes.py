"""
camcops_server/cc_modules/cc_view_classes.py

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

Django-style class-based views for Pyramid.
Adapted from Django's ``views/generic/base.py`` and ``views/generic/edit.py``.

Django has the following licence:

.. code-block:: none

    Copyright (c) Django Software Foundation and individual contributors.
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright
           notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright
           notice, this list of conditions and the following disclaimer in the
           documentation and/or other materials provided with the distribution.

        3. Neither the name of Django nor the names of its contributors may be
           used to endorse or promote products derived from this software
           without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

Custom views typically inherit from :class:`CreateView`, :class:`DeleteView` or
:class:`UpdateView`.

A Pyramid view function with a named route should create a view of the custom
class, passing in the request, and return the results of its ``dispatch()``
method. For example:

.. code-block:: python

    @view_config(route_name="edit_server_created_patient")
    def edit_server_created_patient(req: Request) -> Response:
        return EditServerCreatedPatientView(req).dispatch()

To provide a custom view class to create a new object in the database:

* Inherit from :class:`CreateView`.
* Set the ``object_class`` property.
* Set the ``form_class`` property.
* Set the ``template_name`` property.
* Override ``get_extra_context()`` for any extra parameters to pass to the
  template.
* Set ``success_url`` or override ``get_success_url()`` to be the redirect on
  successful creation.
* Override ``get_form_kwargs()`` for any extra parameters to pass to the form
  constructor.
* For simple views, set the ``model_form_dict`` property to be a mapping of
  object properties to form parameters.
* Override ``get_form_values()`` with any values additional to
  ``model_form_dict`` to populate the form.
* Override ``save_object()` to do anything more than a simple record save
  (saving related objects, for example).

To provide a custom view class to delete an object from the database:

* Inherit from :class:`DeleteView`.
* Set the ``object_class`` property.
* Set the ``form_class`` property.
* Set the ``template_name`` property.
* Override ``get_extra_context()``. for any extra parameters to pass to the
  template.
* Set ``success_url`` or override ``get_success_url()`` to be the redirect on
  successful creation.
* Override ``get_form_kwargs()`` for any extra parameters to pass to the form
  constructor.
* Set the ``pk_param`` property to be the name of the parameter in the request
  that holds the unique/primary key of the object to be deleted.
* Set the ``server_pk_name`` property to be the name of the property on the
  object that is the unique/primary key.
* Override ``get_object()`` if the object cannot be retrieved with the above.
* Override ``delete()`` to do anything more than a simple record delete; for
  example, to delete dependant objects

To provide a custom view class to update an object in the database:

* Inherit from :class:`UpdateView`.
* Set the ``object_class`` property.
* Set the ``form_class`` property.
* Set the ``template_name`` property.
* Override ``get_extra_context()`` for any extra parameters to pass to the
  template.
* Set ``success_url`` or override ``get_success_url()`` to be the redirect on
  successful creation.
* Override ``get_form_kwargs()`` for any extra parameters to pass to the form
  constructor.
* Set the ``pk_param`` property to be the name of the parameter in the request
  that holds the unique/primary key of the object to be updated.
* Set the ``server_pk_name`` property to be the name of the property on the
  object that is the unique/primary key.
* Override ``get_object()`` if the object cannot be retrieved with the above.
* For simple views, set the ``model_form_dict`` property to be a mapping of
  object properties to form parameters.
* Override ``save_object()`` to do anything more than a simple record save
  (saving related objects, for example).

You can use mixins for settings common to multiple views.

.. note::

    When we move to Python 3.8, there is ``typing.Protocol``, which allows
    mixins to be type-checked properly. Currently we suppress warnings.

Some examples are in ``webview.py``.

"""

from pyramid.httpexceptions import (
    HTTPBadRequest,
    HTTPFound,
    HTTPMethodNotAllowed,
)
from pyramid.renderers import render_to_response
from pyramid.response import Response

import logging
from typing import Any, Dict, List, NoReturn, Optional, Type, TYPE_CHECKING

from cardinal_pythonlib.deform_utils import get_head_form_html
from cardinal_pythonlib.logs import BraceStyleAdapter
from deform.exception import ValidationFailure

from camcops_server.cc_modules.cc_exception import raise_runtime_error
from camcops_server.cc_modules.cc_pyramid import FormAction
from camcops_server.cc_modules.cc_resource_registry import (
    CamcopsResourceRegistry
)

if TYPE_CHECKING:
    from deform.form import Form
    from camcops_server.cc_modules.cc_request import CamcopsRequest

log = BraceStyleAdapter(logging.getLogger(__name__))


class ContextMixin(object):
    """
    A default context mixin that passes the keyword arguments received by
    get_context_data() as the template context.
    """
    def get_extra_context(self) -> Dict[str, Any]:
        """
        Override to provide extra context, merged in by
        :meth:`get_context_data`.
        """
        return {}

    def get_context_data(self, **kwargs: Any) -> Dict[str, Any]:
        """
        Provides context for a template, including the ``view`` argument and
        any additional context provided by :meth:`get_extra_context`.
        """
        kwargs.setdefault("view", self)
        kwargs.update(self.get_extra_context())

        return kwargs


class View(object):
    """
    Simple parent class for all views
    """
    http_method_names = ["get", "post"]

    def __init__(self, request: "CamcopsRequest") -> None:
        """
        Args:
            request:
                a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        """
        self.request = request

    def dispatch(self) -> Response:
        """
        Try to dispatch to the right HTTP method (e.g. GET, POST). If a method
        doesn't exist, defer to the error handler. Also defer to the error
        handler if the request method isn't on the approved list.
        """
        handler = self.http_method_not_allowed

        if self.request.method.lower() in self.http_method_names:
            handler = getattr(self, self.request.method.lower(),
                              handler)
        return handler()

    def http_method_not_allowed(self) -> NoReturn:
        """
        Raise a :exc:`pyramid.httpexceptions.HTTPMethodNotAllowed` indicating
        that the selected HTTP method is not allowed.
        """
        log.warning(
            "Method Not Allowed (%s): %s",
            self.request.method, self.request.path,
            extra={"status_code": 405, "request": self.request}
        )
        raise HTTPMethodNotAllowed(
            detail=f"Allowed methods: {self._allowed_methods}"
        )

    def _allowed_methods(self) -> List[str]:
        """
        Which HTTP methods are allowed? Returns a list of upper-case strings.
        """
        return [m.upper() for m in self.http_method_names if hasattr(self, m)]


class TemplateResponseMixin(object):
    """
    A mixin that can be used to render a template.
    """
    request: "CamcopsRequest"
    template_name: str = None

    def render_to_response(self, context: Dict) -> Response:
        """
        Takes the supplied context, renders it through our specified template
        (set by ``template_name``), and returns a
        :class:`pyramid.response.Response`.
        """
        if self.template_name is None:
            raise_runtime_error(f"No template_name set for {self.__class__}.")

        return render_to_response(
            self.template_name,
            context,
            request=self.request
        )


class FormMixin(ContextMixin):
    """
    Provide a way to show and handle a form in a request.
    """
    cancel_url = None
    form_class: Type["Form"] = None
    success_url = None
    _form = None
    _error = None

    request: "CamcopsRequest"

    def get_form_class(self) -> Optional[Type["Form"]]:
        """
        Return the form class to use.
        """
        return self.form_class

    def get_form(self) -> "Form":
        """
        Return an instance of the form to be used in this view.
        """

        if self._form is None:
            form_class = self.get_form_class()
            if not form_class:
                raise_runtime_error("Your view must provide a form_class.")

            assert form_class is not None  # type checker

            self._form = form_class(**self.get_form_kwargs())

        return self._form

    def get_form_kwargs(self) -> Dict[str, Any]:
        """
        Return the keyword arguments for instantiating the form.
        """

        registry = CamcopsResourceRegistry()

        kwargs = {
            "request": self.request,
            "resource_registry": registry,
        }

        return kwargs

    def get_cancel_url(self) -> str:
        """
        Return the URL to redirect to when cancelling a form.
        """
        if not self.cancel_url:
            return self.get_success_url()
        return str(self.cancel_url)  # cancel_url may be lazy

    def get_success_url(self) -> str:
        """
        Return the URL to redirect to after processing a valid form.
        """
        if not self.success_url:
            raise_runtime_error("Your view must provide a success_url.")
        return str(self.success_url)  # success_url may be lazy

    def form_valid(self, form: "Form", appstruct: Dict[str, Any]) -> Response:
        """
        Called when the form is valid.
        Redirects to the supplied "success" URL.
        """
        raise HTTPFound(self.get_success_url())

    def form_invalid(self, validation_error: ValidationFailure) -> Response:
        """
        Called when the form is invalid.
        Returns a response with a rendering of the invalid form.
        """
        self._error = validation_error

        # noinspection PyUnresolvedReferences
        return self.render_to_response(
            self.get_context_data()
        )

    def get_context_data(self, **kwargs: Any) -> Dict[str, Any]:
        """
        Insert the rendered form (as HTML) into the context dict.
        """

        form = self.get_form()
        kwargs["form"] = self.get_rendered_form()
        kwargs["head_form_html"] = get_head_form_html(
            self.request, [form]
        )

        return super().get_context_data(**kwargs)

    def get_rendered_form(self) -> str:
        """
        Returns the form, rendered as HTML.
        """
        if self._error is not None:
            return self._error.render()

        form = self.get_form()
        # noinspection PyUnresolvedReferences
        appstruct = self.get_form_values()

        return form.render(appstruct)


class SingleObjectMixin(ContextMixin):
    """
    Represents a single ORM object, for use as a mixin.
    """
    object: Any
    object_class: Optional[Type[Any]]
    pk_param: str
    request: "CamcopsRequest"
    server_pk_name: str

    def get_context_data(self, **kwargs: Any) -> Dict[str, Any]:
        """
        Insert the single object into the context dict.
        """
        context = {}
        if self.object:
            context["object"] = self.object

        context.update(kwargs)

        return super().get_context_data(**context)

    def get_object(self) -> Any:
        pk_value = self.request.get_int_param(self.pk_param)

        if self.object_class is None:
            raise_runtime_error("Your view must provide an object_class.")

        pk_property = getattr(self.object_class, self.server_pk_name)

        obj = self.request.dbsession.query(self.object_class).filter(
            pk_property == pk_value
        ).one_or_none()

        if obj is None:
            _ = self.request.gettext

            assert self.object_class is not None  # type checker

            raise HTTPBadRequest(
                _("Cannot find {object_class} with {server_pk_name}:{pk_value}").format(  # noqa: E501
                    object_class=self.object_class.__name__,
                    server_pk_name=self.server_pk_name,
                    pk_value=pk_value
                )
            )

        return obj


class ModelFormMixin(FormMixin, SingleObjectMixin):
    """
    Represents an ORM object and an associated form.
    """
    object_class: Optional[Type[Any]] = None

    model_form_dict: Dict
    object: Any
    request: "CamcopsRequest"

    def form_valid(self, form: "Form", appstruct: Dict[str, Any]) -> Response:
        """
        Called when the form is valid.
        Saves the associated model.
        """
        self.save_object(appstruct)
        return super().form_valid(form, appstruct)

    def save_object(self, appstruct: Dict[str, Any]) -> None:
        """
        Saves the object in the database, from data provided via the form.
        """
        if self.object is None:
            if self.object_class is None:
                raise_runtime_error("Your view must provide an object_class.")

            assert self.object_class is not None  # type checker

            self.object = self.object_class()

        self.set_object_properties(appstruct)

        self.request.dbsession.add(self.object)

    def set_object_properties(self, appstruct: Dict[str, Any]) -> None:
        """
        Sets properties of the object, from form data.
        """
        for (model_attr, form_param) in self.model_form_dict.items():
            value = appstruct.get(form_param)
            setattr(self.object, model_attr, value)

    def get_form_values(self) -> Dict[str, Any]:
        """
        Reads form values from the object (or provides an empty dictionary if
        there is no object yet).
        """
        form_values = {}

        if self.object is not None:
            for (model_attr, form_param) in self.model_form_dict.items():
                value = getattr(self.object, model_attr)

                # Not sure if this is a good idea. There may be legitimate
                # reasons for keeping the value None here, but the view is
                # likely to be overriding get_form_values() in that case.
                # The alternative is we have to set all None string values
                # to empty, in order to prevent the word None from appearing
                # in text input fields.
                if value is None:
                    value = ""

                form_values[form_param] = value

        return form_values


class ProcessFormView(View):
    """
    Render a form on GET and processes it on POST.
    """
    def get(self) -> Response:
        """
        Handle GET requests: instantiate a blank version of the form.
        """
        # noinspection PyUnresolvedReferences
        return self.render_to_response(self.get_context_data())

    def post(self) -> Response:
        """
        Handle POST requests: instantiate a form instance with the passed
        POST variables and then check if it's valid.
        """
        if FormAction.CANCEL in self.request.POST:
            # noinspection PyUnresolvedReferences
            raise HTTPFound(self.get_cancel_url())

        # noinspection PyUnresolvedReferences
        form = self.get_form()

        controls = list(self.request.POST.items())

        try:
            appstruct = form.validate(controls)

            # noinspection PyUnresolvedReferences
            return self.form_valid(form, appstruct)
        except ValidationFailure as e:
            # noinspection PyUnresolvedReferences
            return self.form_invalid(e)


class BaseFormView(FormMixin, ProcessFormView):
    """
    A base view for displaying a form.
    """
    pass


class FormView(TemplateResponseMixin, BaseFormView):
    """
    A view for displaying a form and rendering a template response.
    """
    pass


class BaseCreateView(ModelFormMixin, ProcessFormView):
    """
    Base view for creating a new object instance.

    Using this base class requires subclassing to provide a response mixin.
    """
    def get(self) -> Any:
        self.object = None
        return super().get()

    def post(self) -> Any:
        self.object = None
        return super().post()


class CreateView(TemplateResponseMixin, BaseCreateView):
    """
    View for creating a new object, with a response rendered by a template.
    """
    pass


class BaseUpdateView(ModelFormMixin, ProcessFormView):
    """
    Base view for updating an existing object.

    Using this base class requires subclassing to provide a response mixin.
    """
    pk = None

    def get(self) -> Any:
        self.object = self.get_object()
        return super().get()

    def post(self) -> Any:
        self.object = self.get_object()
        return super().post()


class UpdateView(TemplateResponseMixin, BaseUpdateView):
    """
    View for updating an object, with a response rendered by a template.
    """
    pass


class BaseDeleteView(FormMixin, SingleObjectMixin, View):
    """
    Base view for deleting an object.

    Using this base class requires subclassing to provide a response mixin.
    """
    success_url = None

    def delete(self) -> None:
        """
        Delete the fetched object
        """
        self.request.dbsession.delete(self.object)

    def get(self) -> Response:
        """
        Handle GET requests: fetch the object from the database, and renders
        a form with its data.
        """
        self.object = self.get_object()
        context = self.get_context_data(object=self.object)
        # noinspection PyUnresolvedReferences
        return self.render_to_response(context)

    def post(self) -> Response:
        """
        Handle POST requests: instantiate a form instance with the passed
        POST variables and then check if it's valid.
        """
        self.object = self.get_object()

        if FormAction.CANCEL in self.request.POST:
            raise HTTPFound(self.get_cancel_url())

        form = self.get_form()
        controls = list(self.request.POST.items())

        try:
            appstruct = form.validate(controls)

            return self.form_valid(form, appstruct)
        except ValidationFailure as e:
            return self.form_invalid(e)

    def form_valid(self, form: "Form", appstruct: Dict[str, Any]) -> Response:
        """
        Called when the form is valid.
        Deletes the associated model.
        """

        self.delete()

        return super().form_valid(form, appstruct)

    # noinspection PyMethodMayBeStatic
    def get_form_values(self) -> Dict[str, Any]:
        return {}


class DeleteView(TemplateResponseMixin, BaseDeleteView):
    """
    View for deleting an object retrieved with self.get_object(), with a
    response rendered by a template.
    """
    pass
