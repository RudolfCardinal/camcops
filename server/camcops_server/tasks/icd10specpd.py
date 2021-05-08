#!/usr/bin/env python

"""
camcops_server/tasks/icd10specpd.py

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

"""

from typing import Any, Dict, List, Optional, Tuple, Type

from cardinal_pythonlib.datetimefunc import format_datetime
import cardinal_pythonlib.rnc_web as ws
from cardinal_pythonlib.stringfunc import strseq
from cardinal_pythonlib.typetests import is_false
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Boolean, Date, UnicodeText

from camcops_server.cc_modules.cc_constants import (
    CssClass,
    DateFormat,
    ICD10_COPYRIGHT_DIV,
    PV,
)
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import (
    answer,
    get_yes_no_none,
    get_yes_no_unknown,
    subheading_spanning_two_columns,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    BIT_CHECKER,
    CamcopsColumn,
)
from camcops_server.cc_modules.cc_string import AS
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)


# =============================================================================
# Icd10SpecPD
# =============================================================================

def ctv_info_pd(req: CamcopsRequest,
                condition: str, has_it: Optional[bool]) -> CtvInfo:
    return CtvInfo(content=condition + ": " + get_yes_no_unknown(req, has_it))


class Icd10SpecPDMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Icd10SpecPD'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "g", 1, cls.N_GENERAL, Boolean,
            pv=PV.BIT,
            comment_fmt="G{n}: {s}",
            comment_strings=["pathological 1", "pervasive",
                             "pathological 2", "persistent",
                             "primary 1", "primary 2"]
        )
        add_multiple_columns(
            cls, "g1_", 1, cls.N_GENERAL_1, Boolean,
            pv=PV.BIT,
            comment_fmt="G1{n}: {s}",
            comment_strings=["cognition", "affectivity",
                             "impulse control", "interpersonal"]
        )
        add_multiple_columns(
            cls, "paranoid", 1, cls.N_PARANOID, Boolean,
            pv=PV.BIT,
            comment_fmt="Paranoid ({n}): {s}",
            comment_strings=["sensitive", "grudges", "suspicious",
                             "personal rights", "sexual jealousy",
                             "self-referential", "conspiratorial"]
        )
        add_multiple_columns(
            cls, "schizoid", 1, cls.N_SCHIZOID,
            Boolean,
            pv=PV.BIT,
            comment_fmt="Schizoid ({n}): {s}",
            comment_strings=["little pleasure",
                             "cold/detached",
                             "limited capacity for warmth",
                             "indifferent to praise/criticism",
                             "little interest in sex",
                             "solitary",
                             "fantasy/introspection",
                             "0/1 close friends/confidants",
                             "insensitive to social norms"]
        )
        add_multiple_columns(
            cls, "dissocial", 1, cls.N_DISSOCIAL, Boolean,
            pv=PV.BIT,
            comment_fmt="Dissocial ({n}): {s}",
            comment_strings=["unconcern", "irresponsibility",
                             "incapacity to maintain relationships",
                             "low tolerance to frustration",
                             "incapacity for guilt",
                             "prone to blame others"]
        )
        add_multiple_columns(
            cls, "eu", 1, cls.N_EU, Boolean,
            pv=PV.BIT,
            comment_fmt="Emotionally unstable ({n}): {s}",
            comment_strings=["act without considering consequences",
                             "quarrelsome", "outbursts of anger",
                             "can't maintain actions with immediate reward",
                             "unstable/capricious mood",
                             "uncertain self-image",
                             "intense/unstable relationships",
                             "avoids abandonment",
                             "threats/acts of self-harm",
                             "feelings of emptiness"]
        )
        add_multiple_columns(
            cls, "histrionic", 1, cls.N_HISTRIONIC, Boolean,
            pv=PV.BIT,
            comment_fmt="Histrionic ({n}): {s}",
            comment_strings=["theatricality",
                             "suggestibility",
                             "shallow/labile affect",
                             "centre of attention",
                             "inappropriately seductive",
                             "concerned with attractiveness"]
        )
        add_multiple_columns(
            cls, "anankastic", 1, cls.N_ANANKASTIC, Boolean,
            pv=PV.BIT,
            comment_fmt="Anankastic ({n}): {s}",
            comment_strings=["doubt/caution",
                             "preoccupation with details",
                             "perfectionism",
                             "excessively conscientious",
                             "preoccupied with productivity",
                             "excessive pedantry",
                             "rigid/stubborn",
                             "require others do things specific way"]
        )
        add_multiple_columns(
            cls, "anxious", 1, cls.N_ANXIOUS, Boolean,
            pv=PV.BIT,
            comment_fmt="Anxious ({n}), {s}",
            comment_strings=["tension/apprehension",
                             "preoccupied with criticism/rejection",
                             "won't get involved unless certain liked",
                             "need for security restricts lifestyle",
                             "avoidance of interpersonal contact"]
        )
        add_multiple_columns(
            cls, "dependent", 1, cls.N_DEPENDENT, Boolean,
            pv=PV.BIT,
            comment_fmt="Dependent ({n}): {s}",
            comment_strings=["others decide",
                             "subordinate needs to those of others",
                             "unwilling to make reasonable demands",
                             "uncomfortable/helpless when alone",
                             "fears of being left to oneself",
                             "everyday decisions require advice/reassurance"]
        )
        super().__init__(name, bases, classdict)


class Icd10SpecPD(TaskHasClinicianMixin, TaskHasPatientMixin, Task,
                  metaclass=Icd10SpecPDMetaclass):
    """
    Server implementation of the ICD10-PD task.
    """
    __tablename__ = "icd10specpd"
    shortname = "ICD10-PD"

    date_pertains_to = Column(
        "date_pertains_to", Date,
        comment="Date the assessment pertains to"
    )
    comments = Column(
        "comments", UnicodeText,
        comment="Clinician's comments"
    )
    skip_paranoid = CamcopsColumn(
        "skip_paranoid", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Skip questions for paranoid PD?"
    )
    skip_schizoid = CamcopsColumn(
        "skip_schizoid", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Skip questions for schizoid PD?"
    )
    skip_dissocial = CamcopsColumn(
        "skip_dissocial", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Skip questions for dissocial PD?"
    )
    skip_eu = CamcopsColumn(
        "skip_eu", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Skip questions for emotionally unstable PD?"
    )
    skip_histrionic = CamcopsColumn(
        "skip_histrionic", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Skip questions for histrionic PD?"
    )
    skip_anankastic = CamcopsColumn(
        "skip_anankastic", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Skip questions for anankastic PD?"
    )
    skip_anxious = CamcopsColumn(
        "skip_anxious", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Skip questions for anxious PD?"
    )
    skip_dependent = CamcopsColumn(
        "skip_dependent", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Skip questions for dependent PD?"
    )
    other_pd_present = CamcopsColumn(
        "other_pd_present", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Is another personality disorder present?"
    )
    vignette = Column(
        "vignette", UnicodeText,
        comment="Vignette"
    )

    N_GENERAL = 6
    N_GENERAL_1 = 4
    N_PARANOID = 7
    N_SCHIZOID = 9
    N_DISSOCIAL = 6
    N_EU = 10
    N_EUPD_I = 5
    N_HISTRIONIC = 6
    N_ANANKASTIC = 8
    N_ANXIOUS = 5
    N_DEPENDENT = 6

    GENERAL_FIELDS = strseq("g", 1, N_GENERAL)
    GENERAL_1_FIELDS = strseq("g1_", 1, N_GENERAL_1)
    PARANOID_FIELDS = strseq("paranoid", 1, N_PARANOID)
    SCHIZOID_FIELDS = strseq("schizoid", 1, N_SCHIZOID)
    DISSOCIAL_FIELDS = strseq("dissocial", 1, N_DISSOCIAL)
    EU_FIELDS = strseq("eu", 1, N_EU)
    EUPD_I_FIELDS = strseq("eu", 1, N_EUPD_I)  # impulsive
    EUPD_B_FIELDS = strseq("eu", N_EUPD_I + 1, N_EU)  # borderline
    HISTRIONIC_FIELDS = strseq("histrionic", 1, N_HISTRIONIC)
    ANANKASTIC_FIELDS = strseq("anankastic", 1, N_ANANKASTIC)
    ANXIOUS_FIELDS = strseq("anxious", 1, N_ANXIOUS)
    DEPENDENT_FIELDS = strseq("dependent", 1, N_DEPENDENT)

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("ICD-10 criteria for specific personality disorders (F60)")

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        infolist = [ctv_info_pd(req,
                                self.wxstring(req, "meets_general_criteria"),
                                self.has_pd()),
                    ctv_info_pd(req,
                                self.wxstring(req, "paranoid_pd_title"),
                                self.has_paranoid_pd()),
                    ctv_info_pd(req,
                                self.wxstring(req, "schizoid_pd_title"),
                                self.has_schizoid_pd()),
                    ctv_info_pd(req,
                                self.wxstring(req, "dissocial_pd_title"),
                                self.has_dissocial_pd()),
                    ctv_info_pd(req,
                                self.wxstring(req, "eu_pd_i_title"),
                                self.has_eupd_i()),
                    ctv_info_pd(req,
                                self.wxstring(req, "eu_pd_b_title"),
                                self.has_eupd_b()),
                    ctv_info_pd(req,
                                self.wxstring(req, "histrionic_pd_title"),
                                self.has_histrionic_pd()),
                    ctv_info_pd(req,
                                self.wxstring(req, "anankastic_pd_title"),
                                self.has_anankastic_pd()),
                    ctv_info_pd(req,
                                self.wxstring(req, "anxious_pd_title"),
                                self.has_anxious_pd()),
                    ctv_info_pd(req,
                                self.wxstring(req, "dependent_pd_title"),
                                self.has_dependent_pd())]
        return infolist

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="meets_general_criteria", coltype=Boolean(),
                value=self.has_pd(),
                comment="Meets general criteria for personality disorder?"),
            SummaryElement(
                name="paranoid_pd", coltype=Boolean(),
                value=self.has_paranoid_pd(),
                comment="Meets criteria for paranoid PD?"),
            SummaryElement(
                name="schizoid_pd", coltype=Boolean(),
                value=self.has_schizoid_pd(),
                comment="Meets criteria for schizoid PD?"),
            SummaryElement(
                name="dissocial_pd", coltype=Boolean(),
                value=self.has_dissocial_pd(),
                comment="Meets criteria for dissocial PD?"),
            SummaryElement(
                name="eupd_i", coltype=Boolean(),
                value=self.has_eupd_i(),
                comment="Meets criteria for EUPD (impulsive type)?"),
            SummaryElement(
                name="eupd_b", coltype=Boolean(),
                value=self.has_eupd_b(),
                comment="Meets criteria for EUPD (borderline type)?"),
            SummaryElement(
                name="histrionic_pd", coltype=Boolean(),
                value=self.has_histrionic_pd(),
                comment="Meets criteria for histrionic PD?"),
            SummaryElement(
                name="anankastic_pd", coltype=Boolean(),
                value=self.has_anankastic_pd(),
                comment="Meets criteria for anankastic PD?"),
            SummaryElement(
                name="anxious_pd", coltype=Boolean(),
                value=self.has_anxious_pd(),
                comment="Meets criteria for anxious PD?"),
            SummaryElement(
                name="dependent_pd", coltype=Boolean(),
                value=self.has_dependent_pd(),
                comment="Meets criteria for dependent PD?"),
        ]

    # noinspection PyUnresolvedReferences
    def is_pd_excluded(self) -> bool:
        return (
            is_false(self.g1) or
            is_false(self.g2) or
            is_false(self.g3) or
            is_false(self.g4) or
            is_false(self.g5) or
            is_false(self.g6) or
            (
                self.all_fields_not_none(self.GENERAL_1_FIELDS) and
                self.count_booleans(self.GENERAL_1_FIELDS) <= 1
            )
        )

    def is_complete_general(self) -> bool:
        return (
            self.all_fields_not_none(self.GENERAL_FIELDS) and
            self.all_fields_not_none(self.GENERAL_1_FIELDS)
        )

    def is_complete_paranoid(self) -> bool:
        return self.all_fields_not_none(self.PARANOID_FIELDS)

    def is_complete_schizoid(self) -> bool:
        return self.all_fields_not_none(self.SCHIZOID_FIELDS)

    def is_complete_dissocial(self) -> bool:
        return self.all_fields_not_none(self.DISSOCIAL_FIELDS)

    def is_complete_eu(self) -> bool:
        return self.all_fields_not_none(self.EU_FIELDS)

    def is_complete_histrionic(self) -> bool:
        return self.all_fields_not_none(self.HISTRIONIC_FIELDS)

    def is_complete_anankastic(self) -> bool:
        return self.all_fields_not_none(self.ANANKASTIC_FIELDS)

    def is_complete_anxious(self) -> bool:
        return self.all_fields_not_none(self.ANXIOUS_FIELDS)

    def is_complete_dependent(self) -> bool:
        return self.all_fields_not_none(self.DEPENDENT_FIELDS)

    # Meets criteria? These also return null for unknown.
    def has_pd(self) -> Optional[bool]:
        if self.is_pd_excluded():
            return False
        if not self.is_complete_general():
            return None
        return (
                self.all_truthy(self.GENERAL_FIELDS) and
                self.count_booleans(self.GENERAL_1_FIELDS) > 1
        )

    def has_paranoid_pd(self) -> Optional[bool]:
        hpd = self.has_pd()
        if not hpd:
            return hpd
        if not self.is_complete_paranoid():
            return None
        return self.count_booleans(self.PARANOID_FIELDS) >= 4

    def has_schizoid_pd(self) -> Optional[bool]:
        hpd = self.has_pd()
        if not hpd:
            return hpd
        if not self.is_complete_schizoid():
            return None
        return self.count_booleans(self.SCHIZOID_FIELDS) >= 4

    def has_dissocial_pd(self) -> Optional[bool]:
        hpd = self.has_pd()
        if not hpd:
            return hpd
        if not self.is_complete_dissocial():
            return None
        return self.count_booleans(self.DISSOCIAL_FIELDS) >= 3

    # noinspection PyUnresolvedReferences
    def has_eupd_i(self) -> Optional[bool]:
        hpd = self.has_pd()
        if not hpd:
            return hpd
        if not self.is_complete_eu():
            return None
        return (
            self.count_booleans(self.EUPD_I_FIELDS) >= 3 and
            self.eu2
        )

    def has_eupd_b(self) -> Optional[bool]:
        hpd = self.has_pd()
        if not hpd:
            return hpd
        if not self.is_complete_eu():
            return None
        return (
            self.count_booleans(self.EUPD_I_FIELDS) >= 3 and
            self.count_booleans(self.EUPD_B_FIELDS) >= 2
        )

    def has_histrionic_pd(self) -> Optional[bool]:
        hpd = self.has_pd()
        if not hpd:
            return hpd
        if not self.is_complete_histrionic():
            return None
        return self.count_booleans(self.HISTRIONIC_FIELDS) >= 4

    def has_anankastic_pd(self) -> Optional[bool]:
        hpd = self.has_pd()
        if not hpd:
            return hpd
        if not self.is_complete_anankastic():
            return None
        return self.count_booleans(self.ANANKASTIC_FIELDS) >= 4

    def has_anxious_pd(self) -> Optional[bool]:
        hpd = self.has_pd()
        if not hpd:
            return hpd
        if not self.is_complete_anxious():
            return None
        return self.count_booleans(self.ANXIOUS_FIELDS) >= 4

    def has_dependent_pd(self) -> Optional[bool]:
        hpd = self.has_pd()
        if not hpd:
            return hpd
        if not self.is_complete_dependent():
            return None
        return self.count_booleans(self.DEPENDENT_FIELDS) >= 4

    def is_complete(self) -> bool:
        return (
            self.date_pertains_to is not None and (
                self.is_pd_excluded() or (
                    self.is_complete_general() and
                    (self.skip_paranoid or self.is_complete_paranoid()) and
                    (self.skip_schizoid or self.is_complete_schizoid()) and
                    (self.skip_dissocial or self.is_complete_dissocial()) and
                    (self.skip_eu or self.is_complete_eu()) and
                    (self.skip_histrionic or self.is_complete_histrionic()) and
                    (self.skip_anankastic or self.is_complete_anankastic()) and
                    (self.skip_anxious or self.is_complete_anxious()) and
                    (self.skip_dependent or self.is_complete_dependent())
                )
            ) and
            self.field_contents_valid()
        )

    def pd_heading(self, req: CamcopsRequest, wstringname: str) -> str:
        return f"""
            <tr class="{CssClass.HEADING}">
                <td colspan="2">{self.wxstring(req, wstringname)}</td>
            </tr>
        """

    def pd_skiprow(self, req: CamcopsRequest, stem: str) -> str:
        return self.get_twocol_bool_row(
            req, "skip_" + stem, label=self.wxstring(req, "skip_this_pd"))

    def pd_subheading(self, req: CamcopsRequest, wstringname: str) -> str:
        return f"""
            <tr class="{CssClass.SUBHEADING}">
                <td colspan="2">{self.wxstring(req, wstringname)}</td>
            </tr>
        """

    def pd_general_criteria_bits(self, req: CamcopsRequest) -> str:
        return f"""
            <tr>
                <td>{self.wxstring(req, "general_criteria_must_be_met")}</td>
                <td><i><b>{get_yes_no_unknown(req, self.has_pd())}</b></i></td>
            </tr>
        """

    def pd_b_text(self, req: CamcopsRequest, wstringname: str) -> str:
        return f"""
            <tr>
                <td>{self.wxstring(req, wstringname)}</td>
                <td class="{CssClass.SUBHEADING}"></td>
            </tr>
        """

    def pd_basic_row(self, req: CamcopsRequest, stem: str, i: int) -> str:
        return self.get_twocol_bool_row_true_false(
            req, stem + str(i), self.wxstring(req, stem + str(i)))

    def standard_pd_html(self, req: CamcopsRequest, stem: str, n: int) -> str:
        html = self.pd_heading(req, stem + "_pd_title")
        html += self.pd_skiprow(req, stem)
        html += self.pd_general_criteria_bits(req)
        html += self.pd_b_text(req, stem + "_pd_B")
        for i in range(1, n + 1):
            html += self.pd_basic_row(req, stem, i)
        return html

    def get_task_html(self, req: CamcopsRequest) -> str:
        h = self.get_standard_clinician_comments_block(req, self.comments)
        h += f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
        """
        h += self.get_is_complete_tr(req)
        h += tr_qa(req.wappstring(AS.DATE_PERTAINS_TO),
                   format_datetime(self.date_pertains_to,
                                   DateFormat.LONG_DATE, default=None))
        h += tr_qa(self.wxstring(req, "meets_general_criteria"),
                   get_yes_no_none(req, self.has_pd()))
        h += tr_qa(self.wxstring(req, "paranoid_pd_title"),
                   get_yes_no_none(req, self.has_paranoid_pd()))
        h += tr_qa(self.wxstring(req, "schizoid_pd_title"),
                   get_yes_no_none(req, self.has_schizoid_pd()))
        h += tr_qa(self.wxstring(req, "dissocial_pd_title"),
                   get_yes_no_none(req, self.has_dissocial_pd()))
        h += tr_qa(self.wxstring(req, "eu_pd_i_title"),
                   get_yes_no_none(req, self.has_eupd_i()))
        h += tr_qa(self.wxstring(req, "eu_pd_b_title"),
                   get_yes_no_none(req, self.has_eupd_b()))
        h += tr_qa(self.wxstring(req, "histrionic_pd_title"),
                   get_yes_no_none(req, self.has_histrionic_pd()))
        h += tr_qa(self.wxstring(req, "anankastic_pd_title"),
                   get_yes_no_none(req, self.has_anankastic_pd()))
        h += tr_qa(self.wxstring(req, "anxious_pd_title"),
                   get_yes_no_none(req, self.has_anxious_pd()))
        h += tr_qa(self.wxstring(req, "dependent_pd_title"),
                   get_yes_no_none(req, self.has_dependent_pd()))

        h += f"""
                </table>
            </div>
            <div>
                <p><i>Vignette:</i></p>
                <p>{answer(ws.webify(self.vignette),
                           default_for_blank_strings=True)}</p>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="80%">Question</th>
                    <th width="20%">Answer</th>
                </tr>
        """

        # General
        h += subheading_spanning_two_columns(self.wxstring(req, "general"))
        h += self.get_twocol_bool_row_true_false(
            req, "g1", self.wxstring(req, "G1"))
        h += self.pd_b_text(req, "G1b")
        for i in range(1, Icd10SpecPD.N_GENERAL_1 + 1):
            h += self.get_twocol_bool_row_true_false(
                req, "g1_" + str(i), self.wxstring(req, "G1_" + str(i)))
        for i in range(2, Icd10SpecPD.N_GENERAL + 1):
            h += self.get_twocol_bool_row_true_false(
                req, "g" + str(i), self.wxstring(req, "G" + str(i)))

        # Paranoid, etc.
        h += self.standard_pd_html(req, "paranoid", Icd10SpecPD.N_PARANOID)
        h += self.standard_pd_html(req, "schizoid", Icd10SpecPD.N_SCHIZOID)
        h += self.standard_pd_html(req, "dissocial", Icd10SpecPD.N_DISSOCIAL)

        # EUPD is special
        h += self.pd_heading(req, "eu_pd_title")
        h += self.pd_skiprow(req, "eu")
        h += self.pd_general_criteria_bits(req)
        h += self.pd_subheading(req, "eu_pd_i_title")
        h += self.pd_b_text(req, "eu_pd_i_B")
        for i in range(1, Icd10SpecPD.N_EUPD_I + 1):
            h += self.pd_basic_row(req, "eu", i)
        h += self.pd_subheading(req, "eu_pd_b_title")
        h += self.pd_b_text(req, "eu_pd_b_B")
        for i in range(Icd10SpecPD.N_EUPD_I + 1, Icd10SpecPD.N_EU + 1):
            h += self.pd_basic_row(req, "eu", i)

        # Back to plain ones
        h += self.standard_pd_html(req, "histrionic", Icd10SpecPD.N_HISTRIONIC)
        h += self.standard_pd_html(req, "anankastic", Icd10SpecPD.N_ANANKASTIC)
        h += self.standard_pd_html(req, "anxious", Icd10SpecPD.N_ANXIOUS)
        h += self.standard_pd_html(req, "dependent", Icd10SpecPD.N_DEPENDENT)

        # Done
        h += """
            </table>
        """ + ICD10_COPYRIGHT_DIV
        return h
