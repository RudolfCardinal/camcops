#!/usr/bin/env python

"""
camcops_server/tasks/diagnosis.py

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
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.

===============================================================================

"""

from abc import ABC
import logging
from typing import Any, Dict, List, Optional, Type, TYPE_CHECKING

from cardinal_pythonlib.classes import classproperty
from cardinal_pythonlib.colander_utils import (
    get_child_node,
    OptionalIntNode,
)
from cardinal_pythonlib.datetimefunc import pendulum_date_to_datetime_date
from cardinal_pythonlib.logs import BraceStyleAdapter
import cardinal_pythonlib.rnc_web as ws
from cardinal_pythonlib.sqlalchemy.dump import get_literal_query
from colander import (
    Invalid,
    SchemaNode,
    SequenceSchema,
    String,
)
import hl7
from pyramid.renderers import render_to_response
from pyramid.response import Response
from sqlalchemy.ext.declarative import declared_attr
from sqlalchemy.sql.expression import (
    and_, exists, literal, not_, or_, select, union,
)
from sqlalchemy.sql.selectable import SelectBase
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Date, Integer, UnicodeText

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CtvInfo
from camcops_server.cc_modules.cc_db import (
    ancillary_relationship,
    GenericTabletRecordMixin,
    TaskDescendant,
)
from camcops_server.cc_modules.cc_forms import (
    LinkingIdNumSelector,
    or_join_description,
    ReportParamSchema,
    RequestAwareMixin,
)
from camcops_server.cc_modules.cc_hl7 import make_dg1_segment
from camcops_server.cc_modules.cc_html import answer, tr
from camcops_server.cc_modules.cc_nlp import guess_name_components
from camcops_server.cc_modules.cc_patient import Patient
from camcops_server.cc_modules.cc_patientidnum import PatientIdNum
from camcops_server.cc_modules.cc_pyramid import CamcopsPage, ViewParam
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_exportrecipient import ExportRecipient
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_report import Report
from camcops_server.cc_modules.cc_snomed import (
    SnomedConcept,
    SnomedExpression,
    SnomedFocusConcept,
)
from camcops_server.cc_modules.cc_sqlalchemy import Base, DeclarativeAndABCMeta
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    DiagnosticCodeColType,
)

if TYPE_CHECKING:
    from sqlalchemy.sql.elements import ColumnElement

log = BraceStyleAdapter(logging.getLogger(__name__))

# =============================================================================
# Helpers
# =============================================================================

FK_COMMENT = "FK to parent table"


# =============================================================================
# DiagnosisBase
# =============================================================================

class DiagnosisItemBase(GenericTabletRecordMixin, Base):
    __abstract__ = True

    # noinspection PyMethodParameters
    @declared_attr
    def seqnum(cls) -> Column:
        return Column(
            "seqnum", Integer,
            nullable=False,
            comment="Sequence number (consistently 1-based as of 2018-12-01)"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def code(cls) -> Column:
        return Column(
            "code", DiagnosticCodeColType,
            comment="Diagnostic code"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def description(cls) -> Column:
        return CamcopsColumn(
            "description", UnicodeText,
            exempt_from_anonymisation=True,
            comment="Description of the diagnostic code"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def comment(cls) -> Column:
        return Column(  # new in v2.0.0
            "comment", UnicodeText,
            comment="Clinician's comment"
        )

    def get_html_table_row(self) -> str:
        return tr(
            self.seqnum,
            answer(ws.webify(self.code)),
            answer(ws.webify(self.description)),
            answer(ws.webify(self.comment)),
        )

    def get_code_for_hl7(self) -> str:
        # Normal format is to strip out periods, e.g. "F20.0" becomes "F200"
        if not self.code:
            return ""
        return self.code.replace(".", "").upper()

    def get_text_for_hl7(self) -> str:
        return self.description or ""

    def is_empty(self) -> bool:
        return not bool(self.code)


class DiagnosisBase(TaskHasClinicianMixin, TaskHasPatientMixin, Task, ABC,
                    metaclass=DeclarativeAndABCMeta):
    __abstract__ = True

    # noinspection PyMethodParameters
    @declared_attr
    def relates_to_date(cls) -> Column:
        return Column(  # new in v2.0.0
            "relates_to_date", Date,
            comment="Date that diagnoses relate to"
        )

    items = None  # type: List[DiagnosisItemBase]  # must be overridden by a relationship  # noqa: E501

    hl7_coding_system = "?"

    def get_num_items(self) -> int:
        return len(self.items)

    def is_complete(self) -> bool:
        if self.relates_to_date is None:
            return False
        if self.get_num_items() == 0:
            return False
        for item in self.items:  # type: DiagnosisItemBase
            if item.is_empty():
                return False
        return True

    def get_task_html(self, req: CamcopsRequest) -> str:
        html = f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="10%">Diagnosis #</th>
                    <th width="10%">Code</th>
                    <th width="40%">Description</th>
                    <th width="40%">Comment</th>
                </tr>
        """
        for item in self.items:
            html += item.get_html_table_row()
        html += """
            </table>
        """
        return html

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        infolist = []
        for item in self.items:
            infolist.append(CtvInfo(content=(
                f"<b>{ws.webify(item.code)}</b>: {ws.webify(item.description)}"
            )))
        return infolist

    # noinspection PyUnusedLocal
    def get_hl7_extra_data_segments(self, recipient_def: ExportRecipient) \
            -> List[hl7.Segment]:
        segments = []
        clinician = guess_name_components(self.clinician_name)
        for i in range(len(self.items)):
            set_id = i + 1  # make it 1-based, not 0-based
            item = self.items[i]
            segments.append(make_dg1_segment(
                set_id=set_id,
                diagnosis_datetime=self.get_creation_datetime(),
                coding_system=self.hl7_coding_system,
                diagnosis_identifier=item.get_code_for_hl7(),
                diagnosis_text=item.get_text_for_hl7(),
                clinician_surname=clinician.get("surname") or "",
                clinician_forename=clinician.get("forename") or "",
                clinician_prefix=clinician.get("prefix") or "",
                attestation_datetime=self.get_creation_datetime(),
            ))
        return segments


# =============================================================================
# DiagnosisIcd10
# =============================================================================

class DiagnosisIcd10Item(DiagnosisItemBase, TaskDescendant):
    __tablename__ = "diagnosis_icd10_item"

    diagnosis_icd10_id = Column(
        "diagnosis_icd10_id", Integer,
        nullable=False,
        comment=FK_COMMENT,
    )

    # -------------------------------------------------------------------------
    # TaskDescendant overrides
    # -------------------------------------------------------------------------

    @classmethod
    def task_ancestor_class(cls) -> Optional[Type["Task"]]:
        return DiagnosisIcd10

    def task_ancestor(self) -> Optional["DiagnosisIcd10"]:
        return DiagnosisIcd10.get_linked(self.diagnosis_icd10_id, self)


class DiagnosisIcd10(DiagnosisBase):
    """
    Server implementation of the Diagnosis/ICD-10 task.
    """
    __tablename__ = "diagnosis_icd10"

    items = ancillary_relationship(
        parent_class_name="DiagnosisIcd10",
        ancillary_class_name="DiagnosisIcd10Item",
        ancillary_fk_to_parent_attr_name="diagnosis_icd10_id",
        ancillary_order_by_attr_name="seqnum"
    )

    shortname = "Diagnosis_ICD10"
    dependent_classes = [DiagnosisIcd10Item]
    hl7_coding_system = "I10"
    # Page A-129 of https://www.hl7.org/special/committees/vocab/V26_Appendix_A.pdf  # noqa: E501

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Diagnostic codes, ICD-10")

    def get_snomed_codes(self, req: CamcopsRequest,
                         fallback: bool = True) -> List[SnomedExpression]:
        """
        Returns all SNOMED-CT codes for this task.

        Args:
            req: the
                :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            fallback: for example, if F32.10 is unknown, should we fall back to
                F32.1?

        Returns:
            a list of
            :class:`camcops_server.cc_modules.cc_snomed.SnomedExpression`
            objects
        """
        if not req.icd10_snomed_supported:
            return []
        snomed_codes = []  # type: List[SnomedExpression]
        # noinspection PyTypeChecker
        for item in self.items:
            concepts = self._get_snomed_concepts(item.code, req, fallback)
            if not concepts:
                continue
            focusconcept = SnomedFocusConcept(concepts)
            snomed_codes.append(SnomedExpression(focusconcept))
        return snomed_codes

    @staticmethod
    def _get_snomed_concepts(icd10_code: str,
                             req: CamcopsRequest,
                             fallback: bool = True) -> List[SnomedConcept]:
        """
        Internal function to return :class:`SnomedConcept` objects for an
        ICD-10 code.

        Args:
            icd10_code: the ICD-10 code
            req: the
                :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
            fallback: for example, if F32.10 is unknown, should we fall back to
                F32.1?

        Returns:
            list: of :class:`SnomedConcept` objects

        """
        concepts = []  # type: List[SnomedConcept]
        while icd10_code:
            try:
                concepts = req.icd10_snomed(icd10_code)
            except KeyError:  # no known code
                pass
            if concepts or not fallback:
                return concepts
            # Now fall back
            icd10_code = icd10_code[:-1]
        # Run out of code
        return concepts


# =============================================================================
# DiagnosisIcd9CM
# =============================================================================

class DiagnosisIcd9CMItem(DiagnosisItemBase, TaskDescendant):
    __tablename__ = "diagnosis_icd9cm_item"

    diagnosis_icd9cm_id = Column(
        "diagnosis_icd9cm_id", Integer,
        nullable=False,
        comment=FK_COMMENT,
    )

    # -------------------------------------------------------------------------
    # TaskDescendant overrides
    # -------------------------------------------------------------------------

    @classmethod
    def task_ancestor_class(cls) -> Optional[Type["Task"]]:
        return DiagnosisIcd9CM

    def task_ancestor(self) -> Optional["DiagnosisIcd9CM"]:
        return DiagnosisIcd9CM.get_linked(self.diagnosis_icd9cm_id, self)


class DiagnosisIcd9CM(DiagnosisBase):
    """
    Server implementation of the Diagnosis/ICD-9-CM task.
    """
    __tablename__ = "diagnosis_icd9cm"

    items = ancillary_relationship(
        parent_class_name="DiagnosisIcd9CM",
        ancillary_class_name="DiagnosisIcd9CMItem",
        ancillary_fk_to_parent_attr_name="diagnosis_icd9cm_id",
        ancillary_order_by_attr_name="seqnum"
    )

    shortname = "Diagnosis_ICD9CM"
    dependent_classes = [DiagnosisIcd9CMItem]
    hl7_coding_system = "I9CM"
    # Page A-129 of https://www.hl7.org/special/committees/vocab/V26_Appendix_A.pdf  # noqa: E501

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Diagnostic codes, ICD-9-CM (DSM-IV-TR)")

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        if not req.icd9cm_snomed_supported:
            return []
        snomed_codes = []  # type: List[SnomedExpression]
        # noinspection PyTypeChecker
        for item in self.items:
            try:
                concepts = req.icd9cm_snomed(item.code)
            except KeyError:  # no known code
                continue
            if not concepts:
                continue
            focusconcept = SnomedFocusConcept(concepts)
            snomed_codes.append(SnomedExpression(focusconcept))
        return snomed_codes


# =============================================================================
# Reports
# =============================================================================

# -----------------------------------------------------------------------------
# Helpers
# -----------------------------------------------------------------------------

ORDER_BY = ["surname", "forename", "dob", "sex",
            "when_created", "system", "code"]


# noinspection PyProtectedMember,PyUnresolvedReferences
def get_diagnosis_report_query(req: CamcopsRequest,
                               diagnosis_class: Type[DiagnosisBase],
                               item_class: Type[DiagnosisItemBase],
                               item_fk_fieldname: str,
                               system: str) -> SelectBase:
    # SELECT surname, forename, dob, sex, ...
    select_fields = [
        Patient.surname.label("surname"),
        Patient.forename.label("forename"),
        Patient.dob.label("dob"),
        Patient.sex.label("sex"),
    ]
    from_clause = (
        # FROM patient
        Patient.__table__
        # INNER JOIN dxset ON (dxtable.patient_id == patient.id AND ...)
        .join(diagnosis_class.__table__, and_(
            diagnosis_class.patient_id == Patient.id,
            diagnosis_class._device_id == Patient._device_id,
            diagnosis_class._era == Patient._era
        ))
        # INNER JOIN dxrow ON (dxrow.fk_dxset = dxset.pk AND ...)
        .join(item_class.__table__, and_(
            getattr(item_class, item_fk_fieldname) == diagnosis_class.id,
            item_class._device_id == diagnosis_class._device_id,
            item_class._era == diagnosis_class._era
        ))
    )
    for iddef in req.idnum_definitions:
        n = iddef.which_idnum
        desc = iddef.short_description
        aliased_table = PatientIdNum.__table__.alias(f"i{n}")
        # ... [also] SELECT i1.idnum_value AS 'NHS' (etc.)
        select_fields.append(aliased_table.c.idnum_value.label(desc))
        # ... [from] OUTER JOIN patientidnum AS i1 ON (...)
        from_clause = from_clause.outerjoin(aliased_table, and_(
            aliased_table.c.patient_id == Patient.id,
            aliased_table.c._device_id == Patient._device_id,
            aliased_table.c._era == Patient._era,
            # Note: the following are part of the JOIN, not the WHERE:
            # (or failure to match a row will wipe out the Patient from the
            # OUTER JOIN):
            aliased_table.c._current == True,  # noqa: E712
            aliased_table.c.which_idnum == n,  # noqa: E712
        ))  # noqa: E712
    select_fields += [
        diagnosis_class.when_created.label("when_created"),
        literal(system).label("system"),
        item_class.code.label("code"),
        item_class.description.label("description"),
    ]
    # WHERE...
    wheres = [
        Patient._current == True,  # noqa: E712
        diagnosis_class._current == True,
        item_class._current == True,
    ]  # noqa: E712
    if not req.user.superuser:
        # Restrict to accessible groups
        group_ids = req.user.ids_of_groups_user_may_report_on
        wheres.append(diagnosis_class._group_id.in_(group_ids))
        # Helpfully, SQLAlchemy will render this as "... AND 1 != 1" if we
        # pass an empty list to in_().
    query = select(select_fields).select_from(from_clause).where(and_(*wheres))
    return query


def get_diagnosis_report(req: CamcopsRequest,
                         diagnosis_class: Type[DiagnosisBase],
                         item_class: Type[DiagnosisItemBase],
                         item_fk_fieldname: str,
                         system: str) -> SelectBase:
    query = get_diagnosis_report_query(req, diagnosis_class, item_class,
                                       item_fk_fieldname, system)
    query = query.order_by(*ORDER_BY)
    return query


# -----------------------------------------------------------------------------
# Plain "all diagnoses" reports
# -----------------------------------------------------------------------------

class DiagnosisICD9CMReport(Report):
    """Report to show ICD-9-CM (DSM-IV-TR) diagnoses."""

    # noinspection PyMethodParameters
    @classproperty
    def report_id(cls) -> str:
        return "diagnoses_icd9cm"

    @classmethod
    def title(cls, req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Diagnosis – ICD-9-CM (DSM-IV-TR) diagnoses for all "
                 "patients")

    # noinspection PyMethodParameters
    @classproperty
    def superuser_only(cls) -> bool:
        return False

    def get_query(self, req: CamcopsRequest) -> SelectBase:
        return get_diagnosis_report(
            req,
            diagnosis_class=DiagnosisIcd9CM,
            item_class=DiagnosisIcd9CMItem,
            item_fk_fieldname='diagnosis_icd9cm_id',
            system='ICD-9-CM'
        )


class DiagnosisICD10Report(Report):
    """Report to show ICD-10 diagnoses."""

    # noinspection PyMethodParameters
    @classproperty
    def report_id(cls) -> str:
        return "diagnoses_icd10"

    @classmethod
    def title(cls, req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Diagnosis – ICD-10 diagnoses for all patients")

    # noinspection PyMethodParameters
    @classproperty
    def superuser_only(cls) -> bool:
        return False

    def get_query(self, req: CamcopsRequest) -> SelectBase:
        return get_diagnosis_report(
            req,
            diagnosis_class=DiagnosisIcd10,
            item_class=DiagnosisIcd10Item,
            item_fk_fieldname='diagnosis_icd10_id',
            system='ICD-10'
        )


class DiagnosisAllReport(Report):
    """Report to show all diagnoses."""

    # noinspection PyMethodParameters
    @classproperty
    def report_id(cls) -> str:
        return "diagnoses_all"

    @classmethod
    def title(cls, req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Diagnosis – All diagnoses for all patients")

    # noinspection PyMethodParameters
    @classproperty
    def superuser_only(cls) -> bool:
        return False

    def get_query(self, req: CamcopsRequest) -> SelectBase:
        sql_icd9cm = get_diagnosis_report_query(
            req,
            diagnosis_class=DiagnosisIcd9CM,
            item_class=DiagnosisIcd9CMItem,
            item_fk_fieldname='diagnosis_icd9cm_id',
            system='ICD-9-CM'
        )
        sql_icd10 = get_diagnosis_report_query(
            req,
            diagnosis_class=DiagnosisIcd10,
            item_class=DiagnosisIcd10Item,
            item_fk_fieldname='diagnosis_icd10_id',
            system='ICD-10'
        )
        query = union(sql_icd9cm, sql_icd10)
        query = query.order_by(*ORDER_BY)
        return query


# -----------------------------------------------------------------------------
# "Find me patients matching certain diagnostic criteria"
# -----------------------------------------------------------------------------

class DiagnosisNode(SchemaNode, RequestAwareMixin):
    schema_type = String

    def __init__(self, *args, **kwargs) -> None:
        self.title = ""  # for type checker
        self.description = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        self.title = _("Diagnostic code")
        self.description = _(
            "Type in a diagnostic code; you may use SQL 'LIKE' syntax for "
            "wildcards, i.e. _ for one character and % for zero/one/lots"
        )


class DiagnosesSequence(SequenceSchema, RequestAwareMixin):
    diagnoses = DiagnosisNode()

    def __init__(self, *args, minimum_number: int = 0, **kwargs) -> None:
        self.minimum_number = minimum_number
        self.title = ""  # for type checker
        self.description = ""  # for type checker
        super().__init__(*args, **kwargs)

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        request = self.request
        _ = request.gettext
        self.title = _("Diagnostic codes")
        self.description = (
            _("Use % as a wildcard (e.g. F32 matches only F32, but F32% "
              "matches F32, F32.1, F32.2...).") +
            " " +
            or_join_description(request)
        )

    def validator(self, node: SchemaNode, value: List[str]) -> None:
        assert isinstance(value, list)
        _ = self.gettext
        if len(value) < self.minimum_number:
            raise Invalid(
                node,
                _("You must specify at least") + f" {self.minimum_number}")
        if len(value) != len(set(value)):
            raise Invalid(node, _("You have specified duplicate diagnoses"))


class DiagnosisFinderReportSchema(ReportParamSchema):
    which_idnum = LinkingIdNumSelector()  # must match ViewParam.WHICH_IDNUM
    diagnoses_inclusion = DiagnosesSequence(minimum_number=1)  # must match ViewParam.DIAGNOSES_INCLUSION  # noqa: E501
    diagnoses_exclusion = DiagnosesSequence()  # must match ViewParam.DIAGNOSES_EXCLUSION  # noqa: E501
    age_minimum = OptionalIntNode()  # must match ViewParam.AGE_MINIMUM
    age_maximum = OptionalIntNode()  # must match ViewParam.AGE_MAXIMUM

    # noinspection PyUnusedLocal
    def after_bind(self, node: SchemaNode, kw: Dict[str, Any]) -> None:
        _ = self.gettext
        diagnoses_inclusion = get_child_node(self, "diagnoses_inclusion")
        diagnoses_inclusion.title = _("Inclusion diagnoses (lifetime)")
        diagnoses_exclusion = get_child_node(self, "diagnoses_exclusion")
        diagnoses_exclusion.title = _("Exclusion diagnoses (lifetime)")
        age_minimum = get_child_node(self, "age_minimum")
        age_minimum.title = _("Minimum age (years) (optional)")
        age_maximum = get_child_node(self, "age_maximum")
        age_maximum.title = _("Maximum age (years) (optional)")


# noinspection PyProtectedMember
def get_diagnosis_inc_exc_report_query(req: CamcopsRequest,
                                       diagnosis_class: Type[DiagnosisBase],
                                       item_class: Type[DiagnosisItemBase],
                                       item_fk_fieldname: str,
                                       system: str,
                                       which_idnum: int,
                                       inclusion_dx: List[str],
                                       exclusion_dx: List[str],
                                       age_minimum_y: int,
                                       age_maximum_y: int) -> SelectBase:
    """
    As for get_diagnosis_report_query, but this makes some modifications to
    do inclusion and exclusion criteria.

    - We need a linking number to perform exclusion criteria.
    - Therefore, we use a single ID number, which must not be NULL.
    """
    # The basics:
    desc = req.get_id_desc(which_idnum) or "BAD_IDNUM"
    select_fields = [
        Patient.surname.label("surname"),
        Patient.forename.label("forename"),
        Patient.dob.label("dob"),
        Patient.sex.label("sex"),
        PatientIdNum.idnum_value.label(desc),
        diagnosis_class.when_created.label("when_created"),
        literal(system).label("system"),
        item_class.code.label("code"),
        item_class.description.label("description"),
    ]
    # noinspection PyUnresolvedReferences
    select_from = (
        Patient.__table__
        .join(diagnosis_class.__table__, and_(
            diagnosis_class.patient_id == Patient.id,
            diagnosis_class._device_id == Patient._device_id,
            diagnosis_class._era == Patient._era,
            diagnosis_class._current == True,  # noqa: E712
        ))
        .join(item_class.__table__, and_(
            getattr(item_class, item_fk_fieldname) == diagnosis_class.id,
            item_class._device_id == diagnosis_class._device_id,
            item_class._era == diagnosis_class._era,
            item_class._current == True,
        ))
        .join(PatientIdNum.__table__, and_(
            PatientIdNum.patient_id == Patient.id,
            PatientIdNum._device_id == Patient._device_id,
            PatientIdNum._era == Patient._era,
            PatientIdNum._current == True,
            PatientIdNum.which_idnum == which_idnum,
            PatientIdNum.idnum_value.isnot(None),  # NOT NULL
        ))
    )
    wheres = [
        Patient._current == True,  # noqa: E712
    ]
    if not req.user.superuser:
        # Restrict to accessible groups
        group_ids = req.user.ids_of_groups_user_may_report_on
        wheres.append(diagnosis_class._group_id.in_(group_ids))
    else:
        group_ids = []  # type: List[int]  # to stop type-checker moaning below

    # Age limits are simple, as the same patient has the same age for
    # all diagnosis rows.
    today = req.today
    if age_maximum_y is not None:
        # Example: max age is 40; earliest (oldest) DOB is therefore 41
        # years ago plus one day (e.g. if it's 15 June 2010, then earliest
        # DOB is 16 June 1969; a person born then will be 41 tomorrow).
        earliest_dob = pendulum_date_to_datetime_date(
            today.subtract(years=age_maximum_y + 1).add(days=1)
        )
        wheres.append(Patient.dob >= earliest_dob)
    if age_minimum_y is not None:
        # Example: min age is 20; latest (youngest) DOB is therefore 20
        # years ago (e.g. if it's 15 June 2010, latest DOB is 15 June 1990;
        # if you're born after that, you're not 20 yet).
        latest_dob = pendulum_date_to_datetime_date(
            today.subtract(years=age_minimum_y)
        )
        wheres.append(Patient.dob <= latest_dob)

    # Diagnosis criteria are a little bit more complex.
    #
    # We can reasonably do inclusion criteria as "show the diagnoses
    # matching the inclusion criteria" (not the more complex "show all
    # diagnoses for patients having at least one inclusion diagnosis",
    # which is likely to be too verbose for patient finding).
    inclusion_criteria = []  # type: List[ColumnElement]
    for idx in inclusion_dx:
        inclusion_criteria.append(item_class.code.like(idx))
    wheres.append(or_(*inclusion_criteria))

    # Exclusion criteria are the trickier: we need to be able to link
    # multiple diagnoses for the same patient, so we need to use a linking
    # ID number.
    if exclusion_dx:
        # noinspection PyUnresolvedReferences
        edx_items = item_class.__table__.alias("edx_items")
        # noinspection PyUnresolvedReferences
        edx_sets = diagnosis_class.__table__.alias("edx_sets")
        # noinspection PyUnresolvedReferences
        edx_patient = Patient.__table__.alias("edx_patient")
        # noinspection PyUnresolvedReferences
        edx_idnum = PatientIdNum.__table__.alias("edx_idnum")
        edx_joined = (
            edx_items
            .join(edx_sets, and_(
                getattr(edx_items.c, item_fk_fieldname) == edx_sets.c.id,
                edx_items.c._device_id == edx_sets.c._device_id,
                edx_items.c._era == edx_sets.c._era,
                edx_items.c._current == True,  # noqa: E712
            ))
            .join(edx_patient, and_(
                edx_sets.c.patient_id == edx_patient.c.id,
                edx_sets.c._device_id == edx_patient.c._device_id,
                edx_sets.c._era == edx_patient.c._era,
                edx_sets.c._current == True,  # noqa: E712
            ))
            .join(edx_idnum, and_(
                edx_idnum.c.patient_id == edx_patient.c.id,
                edx_idnum.c._device_id == edx_patient.c._device_id,
                edx_idnum.c._era == edx_patient.c._era,
                edx_idnum.c._current == True,  # noqa: E712
                edx_idnum.c.which_idnum == which_idnum,
            ))
        )
        exclusion_criteria = []  # type: List[ColumnElement]
        for edx in exclusion_dx:
            exclusion_criteria.append(edx_items.c.code.like(edx))
        edx_wheres = [
            edx_items.c._current == True,  # noqa: E712
            edx_idnum.c.idnum_value == PatientIdNum.idnum_value,
            or_(*exclusion_criteria)
        ]
        # Note the join above between the main and the EXISTS clauses.
        # We don't use an alias for the main copy of the PatientIdNum table,
        # and we do for the EXISTS version. This is fine; e.g.
        # https://msdn.microsoft.com/en-us/library/ethytz2x.aspx example:
        #   SELECT boss.name, employee.name
        #   FROM employee
        #   INNER JOIN employee boss ON employee.manager_id = boss.emp_id;
        if not req.user.superuser:
            # Restrict to accessible groups
            # group_ids already defined from above
            edx_wheres.append(edx_sets.c._group_id.in_(group_ids))
            # ... bugfix 2018-06-19: "wheres" -> "edx_wheres"
        exclusion_select = (
            select(["*"])
            .select_from(edx_joined)
            .where(and_(*edx_wheres))
        )
        wheres.append(not_(exists(exclusion_select)))

    query = select(select_fields).select_from(select_from).where(and_(*wheres))
    return query


# noinspection PyAbstractClass
class DiagnosisFinderReportBase(Report):
    """Report to show all diagnoses."""

    # noinspection PyMethodParameters
    @classproperty
    def superuser_only(cls) -> bool:
        return False

    @staticmethod
    def get_paramform_schema_class() -> Type["ReportParamSchema"]:
        return DiagnosisFinderReportSchema

    @classmethod
    def get_specific_http_query_keys(cls) -> List[str]:
        return [
            ViewParam.WHICH_IDNUM,
            ViewParam.DIAGNOSES_INCLUSION,
            ViewParam.DIAGNOSES_EXCLUSION,
            ViewParam.AGE_MINIMUM,
            ViewParam.AGE_MAXIMUM
        ]

    def render_single_page_html(self,
                                req: "CamcopsRequest",
                                column_names: List[str],
                                page: CamcopsPage) -> Response:
        which_idnum = req.get_int_param(ViewParam.WHICH_IDNUM)
        inclusion_dx = req.get_str_list_param(ViewParam.DIAGNOSES_INCLUSION)
        exclusion_dx = req.get_str_list_param(ViewParam.DIAGNOSES_EXCLUSION)
        age_minimum = req.get_int_param(ViewParam.AGE_MINIMUM)
        age_maximum = req.get_int_param(ViewParam.AGE_MAXIMUM)
        idnum_desc = req.get_id_desc(which_idnum) or "BAD_IDNUM"
        query = self.get_query(req)
        sql = get_literal_query(query, bind=req.engine)

        return render_to_response(
            "diagnosis_finder_report.mako",
            dict(title=self.title(req),
                 page=page,
                 column_names=column_names,
                 report_id=self.report_id,
                 idnum_desc=idnum_desc,
                 inclusion_dx=inclusion_dx,
                 exclusion_dx=exclusion_dx,
                 age_minimum=age_minimum,
                 age_maximum=age_maximum,
                 sql=sql),
            request=req
        )


class DiagnosisICD10FinderReport(DiagnosisFinderReportBase):
    # noinspection PyMethodParameters
    @classproperty
    def report_id(cls) -> str:
        return "diagnoses_finder_icd10"

    @classmethod
    def title(cls, req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Diagnosis – Find patients by ICD-10 diagnosis ± age")

    def get_query(self, req: CamcopsRequest) -> SelectBase:
        which_idnum = req.get_int_param(ViewParam.WHICH_IDNUM)
        inclusion_dx = req.get_str_list_param(ViewParam.DIAGNOSES_INCLUSION)
        exclusion_dx = req.get_str_list_param(ViewParam.DIAGNOSES_EXCLUSION)
        age_minimum = req.get_int_param(ViewParam.AGE_MINIMUM)
        age_maximum = req.get_int_param(ViewParam.AGE_MAXIMUM)

        q = get_diagnosis_inc_exc_report_query(
            req,
            diagnosis_class=DiagnosisIcd10,
            item_class=DiagnosisIcd10Item,
            item_fk_fieldname='diagnosis_icd10_id',
            system='ICD-10',
            which_idnum=which_idnum,
            inclusion_dx=inclusion_dx,
            exclusion_dx=exclusion_dx,
            age_minimum_y=age_minimum,
            age_maximum_y=age_maximum,
        )
        q = q.order_by(*ORDER_BY)
        # log.debug("Final query:\n{}", get_literal_query(q, bind=req.engine))
        return q

    @staticmethod
    def get_test_querydict() -> Dict[str, Any]:
        return {
            ViewParam.WHICH_IDNUM: 1,
            ViewParam.DIAGNOSES_INCLUSION: ['F32%'],
            ViewParam.DIAGNOSES_EXCLUSION: [],
            ViewParam.AGE_MINIMUM: None,
            ViewParam.AGE_MAXIMUM: None,
        }


class DiagnosisICD9CMFinderReport(DiagnosisFinderReportBase):
    # noinspection PyMethodParameters
    @classproperty
    def report_id(cls) -> str:
        return "diagnoses_finder_icd9cm"

    @classmethod
    def title(cls, req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _(
            "Diagnosis – Find patients by ICD-9-CM (DSM-IV-TR) diagnosis ± age"
        )

    def get_query(self, req: CamcopsRequest) -> SelectBase:
        which_idnum = req.get_int_param(ViewParam.WHICH_IDNUM)
        inclusion_dx = req.get_str_list_param(ViewParam.DIAGNOSES_INCLUSION)
        exclusion_dx = req.get_str_list_param(ViewParam.DIAGNOSES_EXCLUSION)
        age_minimum = req.get_int_param(ViewParam.AGE_MINIMUM)
        age_maximum = req.get_int_param(ViewParam.AGE_MAXIMUM)

        q = get_diagnosis_inc_exc_report_query(
            req,
            diagnosis_class=DiagnosisIcd9CM,
            item_class=DiagnosisIcd9CMItem,
            item_fk_fieldname='diagnosis_icd9cm_id',
            system='ICD-9-CM',
            which_idnum=which_idnum,
            inclusion_dx=inclusion_dx,
            exclusion_dx=exclusion_dx,
            age_minimum_y=age_minimum,
            age_maximum_y=age_maximum,
        )
        q = q.order_by(*ORDER_BY)
        # log.debug("Final query:\n{}", get_literal_query(q, bind=req.engine))
        return q

    @staticmethod
    def get_test_querydict() -> Dict[str, Any]:
        return {
            ViewParam.WHICH_IDNUM: 1,
            ViewParam.DIAGNOSES_INCLUSION: ['296%'],
            ViewParam.DIAGNOSES_EXCLUSION: [],
            ViewParam.AGE_MINIMUM: None,
            ViewParam.AGE_MAXIMUM: None,
        }
