#!/usr/bin/env python

"""
camcops_server/tasks/icd10manic.py

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

from typing import List, Optional

from cardinal_pythonlib.datetimefunc import format_datetime
from cardinal_pythonlib.typetests import is_false
import cardinal_pythonlib.rnc_web as ws
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Boolean, Date, UnicodeText

from camcops_server.cc_modules.cc_constants import (
    CssClass,
    DateFormat,
    ICD10_COPYRIGHT_DIV,
)
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_html import (
    get_present_absent_none,
    heading_spanning_two_columns,
    subheading_spanning_two_columns,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    BIT_CHECKER,
    CamcopsColumn,
    SummaryCategoryColType,
)
from camcops_server.cc_modules.cc_string import AS
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_text import SS


# =============================================================================
# Icd10Manic
# =============================================================================

class Icd10Manic(TaskHasClinicianMixin, TaskHasPatientMixin, Task):
    """
    Server implementation of the ICD10-MANIC task.
    """
    __tablename__ = "icd10manic"
    shortname = "ICD10-MANIC"

    mood_elevated = CamcopsColumn(
        "mood_elevated", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="The mood is 'elevated' [hypomania] or 'predominantly "
                "elevated [or] expansive' [mania] to a degree that is "
                "definitely abnormal for the individual concerned."
    )
    mood_irritable = CamcopsColumn(
        "mood_irritable", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="The mood is 'irritable' [hypomania] or 'predominantly "
                "irritable' [mania] to a degree that is definitely abnormal "
                "for the individual concerned."
    )

    distractible = CamcopsColumn(
        "distractible", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Difficulty in concentration or distractibility [from "
                "the criteria for hypomania]; distractibility or constant "
                "changes in activity or plans [from the criteria for mania]."
    )
    activity = CamcopsColumn(
        "activity", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Increased activity or physical restlessness."
    )
    sleep = CamcopsColumn(
        "sleep", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Decreased need for sleep."
    )
    talkativeness = CamcopsColumn(
        "talkativeness", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Increased talkativeness (pressure of speech)."
    )
    recklessness = CamcopsColumn(
        "recklessness", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Mild spending sprees, or other types of reckless or "
                "irresponsible behaviour [hypomania]; behaviour which is "
                "foolhardy or reckless and whose risks the subject does not "
                "recognize e.g. spending sprees, foolish enterprises, "
                "reckless driving [mania]."
    )
    social_disinhibition = CamcopsColumn(
        "social_disinhibition", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Increased sociability or over-familiarity [hypomania]; "
                "loss of normal social inhibitions resulting in behaviour "
                "which is inappropriate to the circumstances [mania]."
    )
    sexual = CamcopsColumn(
        "sexual", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Increased sexual energy [hypomania]; marked sexual "
                "energy or sexual indiscretions [mania]."
    )

    grandiosity = CamcopsColumn(
        "grandiosity", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Inflated self-esteem or grandiosity."
    )
    flight_of_ideas = CamcopsColumn(
        "flight_of_ideas", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Flight of ideas or the subjective experience of "
                "thoughts racing."
    )

    sustained4days = CamcopsColumn(
        "sustained4days", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Elevated/irritable mood sustained for at least 4 days."
    )
    sustained7days = CamcopsColumn(
        "sustained7days", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Elevated/irritable mood sustained for at least 7 days."
    )
    admission_required = CamcopsColumn(
        "admission_required", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Elevated/irritable mood severe enough to require "
                "hospital admission."
    )
    some_interference_functioning = CamcopsColumn(
        "some_interference_functioning", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Some interference with personal functioning "
                "in daily living."
    )
    severe_interference_functioning = CamcopsColumn(
        "severe_interference_functioning", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Severe interference with personal "
                "functioning in daily living."
    )

    perceptual_alterations = CamcopsColumn(
        "perceptual_alterations", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Perceptual alterations (e.g. subjective hyperacusis, "
                "appreciation of colours as specially vivid, etc.)."
    )  # ... not psychotic
    hallucinations_schizophrenic = CamcopsColumn(
        "hallucinations_schizophrenic", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Hallucinations that are 'typically schizophrenic' "
                "(hallucinatory voices giving a running commentary on the "
                "patient's behaviour, or discussing him between themselves, "
                "or other types of hallucinatory voices coming from some part "
                "of the body)."
    )
    hallucinations_other = CamcopsColumn(
        "hallucinations_other", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Hallucinations (of any other kind)."
    )
    delusions_schizophrenic = CamcopsColumn(
        "delusions_schizophrenic", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Delusions that are 'typically schizophrenic' (delusions "
                "of control, influence or passivity, clearly referred to body "
                "or limb movements or specific thoughts, actions, or "
                "sensations; delusional perception; persistent delusions of "
                "other kinds that are culturally inappropriate and completely "
                "impossible)."
    )
    delusions_other = CamcopsColumn(
        "delusions_other", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Delusions (of any other kind)."
    )

    date_pertains_to = Column(
        "date_pertains_to", Date,
        comment="Date the assessment pertains to"
    )
    comments = Column(
        "comments", UnicodeText,
        comment="Clinician's comments"
    )

    CORE_NAMES = ["mood_elevated", "mood_irritable"]
    HYPOMANIA_MANIA_NAMES = [
        "distractible", "activity", "sleep",
        "talkativeness", "recklessness", "social_disinhibition", "sexual"
    ]
    MANIA_NAMES = ["grandiosity", "flight_of_ideas"]
    OTHER_CRITERIA_NAMES = [
        "sustained4days", "sustained7days", "admission_required",
        "some_interference_functioning", "severe_interference_functioning"
    ]
    PSYCHOSIS_NAMES = [
        "perceptual_alterations",  # not psychotic
        "hallucinations_schizophrenic", "hallucinations_other",
        "delusions_schizophrenic", "delusions_other"
    ]

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _(
            "ICD-10 symptomatic criteria for a manic/hypomanic episode "
            "(as in e.g. F06.3, F25, F30, F31)"
        )

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        infolist = [CtvInfo(
            content="Pertains to: {}. Category: {}.".format(
                format_datetime(self.date_pertains_to, DateFormat.LONG_DATE),
                self.get_description(req)
            )
        )]
        if self.comments:
            infolist.append(CtvInfo(content=ws.webify(self.comments)))
        return infolist

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(name="category",
                           coltype=SummaryCategoryColType,
                           value=self.get_description(req),
                           comment="Diagnostic category"),
            SummaryElement(name="psychotic_symptoms",
                           coltype=Boolean(),
                           value=self.psychosis_present(),
                           comment="Psychotic symptoms present?"),
        ]

    # Meets criteria? These also return null for unknown.
    def meets_criteria_mania_psychotic_schizophrenic(self) -> Optional[bool]:
        x = self.meets_criteria_mania_ignoring_psychosis()
        if not x:
            return x
        if self.hallucinations_other or self.delusions_other:
            return False  # that counts as manic psychosis
        if self.hallucinations_other is None or self.delusions_other is None:
            return None  # might be manic psychosis
        if self.hallucinations_schizophrenic or self.delusions_schizophrenic:
            return True
        if (self.hallucinations_schizophrenic is None or
                self.delusions_schizophrenic is None):
            return None
        return False

    def meets_criteria_mania_psychotic_icd(self) -> Optional[bool]:
        x = self.meets_criteria_mania_ignoring_psychosis()
        if not x:
            return x
        if self.hallucinations_other or self.delusions_other:
            return True
        if self.hallucinations_other is None or self.delusions_other is None:
            return None
        return False

    def meets_criteria_mania_nonpsychotic(self) -> Optional[bool]:
        x = self.meets_criteria_mania_ignoring_psychosis()
        if not x:
            return x
        if (self.hallucinations_schizophrenic is None or
                self.delusions_schizophrenic is None or
                self.hallucinations_other is None or
                self.delusions_other is None):
            return None
        if (self.hallucinations_schizophrenic or
                self.delusions_schizophrenic or
                self.hallucinations_other or
                self.delusions_other):
            return False
        return True

    def meets_criteria_mania_ignoring_psychosis(self) -> Optional[bool]:
        # When can we say "definitely not"?
        if is_false(self.mood_elevated) and is_false(self.mood_irritable):
            return False
        if is_false(self.sustained7days) and is_false(self.admission_required):
            return False
        t = self.count_booleans(self.HYPOMANIA_MANIA_NAMES) + \
            self.count_booleans(self.MANIA_NAMES)
        u = self.n_fields_none(self.HYPOMANIA_MANIA_NAMES) + \
            self.n_fields_none(self.MANIA_NAMES)
        if self.mood_elevated and (t + u < 3):
            # With elevated mood, need at least 3 symptoms
            return False
        if is_false(self.mood_elevated) and (t + u < 4):
            # With only irritable mood, need at least 4 symptoms
            return False
        if is_false(self.severe_interference_functioning):
            return False
        # OK. When can we say "yes"?
        if ((self.mood_elevated or self.mood_irritable) and
                (self.sustained7days or self.admission_required) and
                ((self.mood_elevated and t >= 3) or
                    (self.mood_irritable and t >= 4)) and
                self.severe_interference_functioning):
            return True
        return None

    def meets_criteria_hypomania(self) -> Optional[bool]:
        # When can we say "definitely not"?
        if self.meets_criteria_mania_ignoring_psychosis():
            return False  # silly to call it hypomania if it's mania
        if is_false(self.mood_elevated) and is_false(self.mood_irritable):
            return False
        if is_false(self.sustained4days):
            return False
        t = self.count_booleans(self.HYPOMANIA_MANIA_NAMES)
        u = self.n_fields_none(self.HYPOMANIA_MANIA_NAMES)
        if t + u < 3:
            # Need at least 3 symptoms
            return False
        if is_false(self.some_interference_functioning):
            return False
        # OK. When can we say "yes"?
        if ((self.mood_elevated or self.mood_irritable) and
                self.sustained4days and
                t >= 3 and
                self.some_interference_functioning):
            return True
        return None

    def meets_criteria_none(self) -> Optional[bool]:
        h = self.meets_criteria_hypomania()
        m = self.meets_criteria_mania_ignoring_psychosis()
        if h or m:
            return False
        if is_false(h) and is_false(m):
            return True
        return None

    def psychosis_present(self) -> Optional[bool]:
        if (self.hallucinations_other or
                self.hallucinations_schizophrenic or
                self.delusions_other or
                self.delusions_schizophrenic):
            return True
        if (self.hallucinations_other is None or
                self.hallucinations_schizophrenic is None or
                self.delusions_other is None or
                self.delusions_schizophrenic is None):
            return None
        return False

    def get_description(self, req: CamcopsRequest) -> str:
        if self.meets_criteria_mania_psychotic_schizophrenic():
            return self.wxstring(req, "category_manic_psychotic_schizophrenic")
        elif self.meets_criteria_mania_psychotic_icd():
            return self.wxstring(req, "category_manic_psychotic")
        elif self.meets_criteria_mania_nonpsychotic():
            return self.wxstring(req, "category_manic_nonpsychotic")
        elif self.meets_criteria_hypomania():
            return self.wxstring(req, "category_hypomanic")
        elif self.meets_criteria_none():
            return self.wxstring(req, "category_none")
        else:
            return req.sstring(SS.UNKNOWN)

    def is_complete(self) -> bool:
        return (
            self.date_pertains_to is not None and
            self.meets_criteria_none() is not None and
            self.field_contents_valid()
        )

    def text_row(self, req: CamcopsRequest, wstringname: str) -> str:
        return heading_spanning_two_columns(self.wxstring(req, wstringname))

    def row_true_false(self, req: CamcopsRequest, fieldname: str) -> str:
        return self.get_twocol_bool_row_true_false(
            req, fieldname, self.wxstring(req, "" + fieldname))

    def get_task_html(self, req: CamcopsRequest) -> str:
        h = """
            {clinician_comments}
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {date_pertains_to}
                    {category}
                    {psychotic_symptoms}
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                {icd10_symptomatic_disclaimer}
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="80%">Question</th>
                    <th width="20%">Answer</th>
                </tr>
        """.format(
            clinician_comments=self.get_standard_clinician_comments_block(
                req, self.comments),
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            date_pertains_to=tr_qa(
                req.wappstring(AS.DATE_PERTAINS_TO),
                format_datetime(self.date_pertains_to,
                                DateFormat.LONG_DATE, default=None)
            ),
            category=tr_qa(
                req.sstring(SS.CATEGORY) + " <sup>[1,2]</sup>",
                self.get_description(req)
            ),
            psychotic_symptoms=tr_qa(
                self.wxstring(req, "psychotic_symptoms") + " <sup>[2]</sup>",
                get_present_absent_none(req, self.psychosis_present())
            ),
            icd10_symptomatic_disclaimer=req.wappstring(
                AS.ICD10_SYMPTOMATIC_DISCLAIMER),
        )

        h += self.text_row(req, "core")
        for x in self.CORE_NAMES:
            h += self.row_true_false(req, x)

        h += self.text_row(req, "hypomania_mania")
        for x in self.HYPOMANIA_MANIA_NAMES:
            h += self.row_true_false(req, x)

        h += self.text_row(req, "other_mania")
        for x in self.MANIA_NAMES:
            h += self.row_true_false(req, x)

        h += self.text_row(req, "other_criteria")
        for x in self.OTHER_CRITERIA_NAMES:
            h += self.row_true_false(req, x)

        h += subheading_spanning_two_columns(self.wxstring(req, "psychosis"))
        for x in self.PSYCHOSIS_NAMES:
            h += self.row_true_false(req, x)

        h += f"""
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Hypomania:
                    elevated/irritable mood
                    + sustained for ≥4 days
                    + at least 3 of the “other hypomania” symptoms
                    + some interference with functioning.
                Mania:
                    elevated/irritable mood
                    + sustained for ≥7 days or hospital admission required
                    + at least 3 of the “other mania/hypomania” symptoms
                      (4 if mood only irritable)
                    + severe interference with functioning.
                [2] ICD-10 nonpsychotic mania requires mania without
                    hallucinations/delusions.
                ICD-10 psychotic mania requires mania plus
                hallucinations/delusions other than those that are
                “typically schizophrenic”.
                ICD-10 does not clearly categorize mania with only
                schizophreniform psychotic symptoms; however, Schneiderian
                first-rank symptoms can occur in manic psychosis
                (e.g. Conus P et al., 2004, PMID 15337330.).
            </div>
            {ICD10_COPYRIGHT_DIV}
        """
        return h
