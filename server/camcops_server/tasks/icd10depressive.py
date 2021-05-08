#!/usr/bin/env python

"""
camcops_server/tasks/icd10depressive.py

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
import cardinal_pythonlib.rnc_web as ws
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Boolean, Date, Integer, UnicodeText

from camcops_server.cc_modules.cc_constants import (
    CssClass,
    DateFormat,
    ICD10_COPYRIGHT_DIV,
)
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_html import (
    answer,
    get_present_absent_none,
    heading_spanning_two_columns,
    tr,
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
# Icd10Depressive
# =============================================================================

class Icd10Depressive(TaskHasClinicianMixin, TaskHasPatientMixin, Task):
    """
    Server implementation of the ICD10-DEPR task.
    """
    __tablename__ = "icd10depressive"
    shortname = "ICD10-DEPR"

    mood = CamcopsColumn(
        "mood", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Depressed mood to a degree that is definitely abnormal "
                "for the individual, present for most of the day and  almost "
                "every day, largely uninfluenced by circumstances, and "
                "sustained for at least 2 weeks."
    )
    anhedonia = CamcopsColumn(
        "anhedonia", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Loss of interest or pleasure in activities  that are "
                "normally pleasurable."
    )
    energy = CamcopsColumn(
        "energy", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Decreased energy or increased fatiguability."
    )

    sleep = CamcopsColumn(
        "sleep", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Sleep disturbance of any type."
    )
    worth = CamcopsColumn(
        "worth", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Loss of confidence and self-esteem."
    )
    appetite = CamcopsColumn(
        "appetite", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Change in appetite (decrease or increase) with "
                "corresponding weight change."
    )
    guilt = CamcopsColumn(
        "guilt", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Unreasonable feelings of self-reproach or excessive and "
                "inappropriate guilt."
    )
    concentration = CamcopsColumn(
        "concentration", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Complaints or evidence of diminished ability to think "
                "or concentrate, such as indecisiveness or vacillation."
    )
    activity = CamcopsColumn(
        "activity", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Change in psychomotor activity, with agitation or "
                "retardation (either subjective or objective)."
    )
    death = CamcopsColumn(
        "death", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Recurrent thoughts of death or suicide, or any "
                "suicidal behaviour."
    )

    somatic_anhedonia = CamcopsColumn(
        "somatic_anhedonia", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Marked loss of interest or pleasure in activities that "
                "are normally pleasurable"
    )
    somatic_emotional_unreactivity = CamcopsColumn(
        "somatic_emotional_unreactivity", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Lack of emotional reactions to events or "
                "activities that normally produce an emotional response"
    )
    somatic_early_morning_waking = CamcopsColumn(
        "somatic_early_morning_waking", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Waking in the morning 2 hours or more before "
                "the usual time"
    )
    somatic_mood_worse_morning = CamcopsColumn(
        "somatic_mood_worse_morning", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Depression worse in the morning"
    )
    somatic_psychomotor = CamcopsColumn(
        "somatic_psychomotor", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Objective evidence of marked psychomotor retardation or "
                "agitation (remarked on or reported by other people)"
    )
    somatic_appetite = CamcopsColumn(
        "somatic_appetite", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Marked loss of appetite"
    )
    somatic_weight = CamcopsColumn(
        "somatic_weight", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Weight loss (5 percent or more of body weight in the past "
                "month)"
        # 2017-08-24: AVOID A PERCENT SYMBOL (%) FOR NOW; SEE THIS BUG:
        # https://bitbucket.org/zzzeek/sqlalchemy/issues/4052/comment-attribute-causes-crash-during  # noqa
    )
    somatic_libido = CamcopsColumn(
        "somatic_libido", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Marked loss of libido"
    )

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
                "impossible).")
    delusions_other = CamcopsColumn(
        "delusions_other", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Delusions (of any other kind)."
    )
    stupor = CamcopsColumn(
        "stupor", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Depressive stupor."
    )

    date_pertains_to = CamcopsColumn(
        "date_pertains_to", Date,
        comment="Date the assessment pertains to"
    )
    comments = Column(
        "comments", UnicodeText,
        comment="Clinician's comments"
    )
    duration_at_least_2_weeks = CamcopsColumn(
        "duration_at_least_2_weeks", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Depressive episode lasts at least 2 weeks?"
    )
    severe_clinically = CamcopsColumn(
        "severe_clinically", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Clinical impression of severe depression, in a "
                "patient unwilling or unable to describe many symptoms in "
                "detail"
    )

    CORE_NAMES = ["mood", "anhedonia", "energy"]
    ADDITIONAL_NAMES = ["sleep", "worth", "appetite", "guilt", "concentration",
                        "activity", "death"]
    SOMATIC_NAMES = [
        "somatic_anhedonia", "somatic_emotional_unreactivity",
        "somatic_early_morning_waking", "somatic_mood_worse_morning",
        "somatic_psychomotor", "somatic_appetite",
        "somatic_weight", "somatic_libido"
    ]
    PSYCHOSIS_NAMES = [
        "hallucinations_schizophrenic", "hallucinations_other",
        "delusions_schizophrenic", "delusions_other",
        "stupor"
    ]

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _(
            "ICD-10 symptomatic criteria for a depressive episode "
            "(as in e.g. F06.3, F25, F31, F32, F33)"
        )

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        infolist = [CtvInfo(
            content="Pertains to: {}. Category: {}.".format(
                format_datetime(self.date_pertains_to, DateFormat.LONG_DATE),
                self.get_full_description(req)
            )
        )]
        if self.comments:
            infolist.append(CtvInfo(content=ws.webify(self.comments)))
        return infolist

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="n_core",
                coltype=Integer(),
                value=self.n_core(),
                comment="Number of core diagnostic symptoms (/3)"),
            SummaryElement(
                name="n_additional",
                coltype=Integer(),
                value=self.n_additional(),
                comment="Number of additional diagnostic symptoms (/7)"),
            SummaryElement(
                name="n_total",
                coltype=Integer(),
                value=self.n_total(),
                comment="Total number of diagnostic symptoms (/10)"),
            SummaryElement(
                name="n_somatic",
                coltype=Integer(),
                value=self.n_somatic(),
                comment="Number of somatic syndrome symptoms (/8)"),
            SummaryElement(
                name="category",
                coltype=SummaryCategoryColType,
                value=self.get_full_description(req),
                comment="Diagnostic category"),
            SummaryElement(
                name="psychosis_or_stupor",
                coltype=Boolean(),
                value=self.is_psychotic_or_stupor(),
                comment="Psychotic symptoms or stupor present?"),
        ]

    # Scoring
    def n_core(self) -> int:
        return self.count_booleans(self.CORE_NAMES)

    def n_additional(self) -> int:
        return self.count_booleans(self.ADDITIONAL_NAMES)

    def n_total(self) -> int:
        return self.n_core() + self.n_additional()

    def n_somatic(self) -> int:
        return self.count_booleans(self.SOMATIC_NAMES)

    def main_complete(self) -> bool:
        return (
            self.duration_at_least_2_weeks is not None and
            self.all_fields_not_none(self.CORE_NAMES) and
            self.all_fields_not_none(self.ADDITIONAL_NAMES)
        ) or bool(
            self.severe_clinically
        )

    # Meets criteria? These also return null for unknown.
    def meets_criteria_severe_psychotic_schizophrenic(self) -> Optional[bool]:
        x = self.meets_criteria_severe_ignoring_psychosis()
        if not x:
            return x
        if self.stupor or self.hallucinations_other or self.delusions_other:
            return False  # that counts as F32.3
        if (self.stupor is None or self.hallucinations_other is None or
                self.delusions_other is None):
            return None  # might be F32.3
        if self.hallucinations_schizophrenic or self.delusions_schizophrenic:
            return True
        if (self.hallucinations_schizophrenic is None or
                self.delusions_schizophrenic is None):
            return None
        return False

    def meets_criteria_severe_psychotic_icd(self) -> Optional[bool]:
        x = self.meets_criteria_severe_ignoring_psychosis()
        if not x:
            return x
        if self.stupor or self.hallucinations_other or self.delusions_other:
            return True
        if (self.stupor is None or
                self.hallucinations_other is None or
                self.delusions_other is None):
            return None
        return False

    def meets_criteria_severe_nonpsychotic(self) -> Optional[bool]:
        x = self.meets_criteria_severe_ignoring_psychosis()
        if not x:
            return x
        if self.any_fields_none(self.PSYCHOSIS_NAMES):
            return None
        return self.count_booleans(self.PSYCHOSIS_NAMES) == 0

    def meets_criteria_severe_ignoring_psychosis(self) -> Optional[bool]:
        if self.severe_clinically:
            return True
        if self.duration_at_least_2_weeks is not None \
                and not self.duration_at_least_2_weeks:
            return False  # too short
        if self.n_core() >= 3 and self.n_total() >= 8:
            return True
        if not self.main_complete():
            return None  # addition of more information might increase severity
        return False

    def meets_criteria_moderate(self) -> Optional[bool]:
        if self.severe_clinically:
            return False  # too severe
        if self.duration_at_least_2_weeks is not None \
                and not self.duration_at_least_2_weeks:
            return False  # too short
        if self.n_core() >= 3 and self.n_total() >= 8:
            return False  # too severe; that's severe
        if not self.main_complete():
            return None  # addition of more information might increase severity
        if self.n_core() >= 2 and self.n_total() >= 6:
            return True
        return False

    def meets_criteria_mild(self) -> Optional[bool]:
        if self.severe_clinically:
            return False  # too severe
        if self.duration_at_least_2_weeks is not None \
                and not self.duration_at_least_2_weeks:
            return False  # too short
        if self.n_core() >= 2 and self.n_total() >= 6:
            return False  # too severe; that's moderate
        if not self.main_complete():
            return None  # addition of more information might increase severity
        if self.n_core() >= 2 and self.n_total() >= 4:
            return True
        return False

    def meets_criteria_none(self) -> Optional[bool]:
        if self.severe_clinically:
            return False  # too severe
        if self.duration_at_least_2_weeks is not None \
                and not self.duration_at_least_2_weeks:
            return True  # too short for depression
        if self.n_core() >= 2 and self.n_total() >= 4:
            return False  # too severe
        if not self.main_complete():
            return None  # addition of more information might increase severity
        return True

    def meets_criteria_somatic(self) -> Optional[bool]:
        t = self.n_somatic()
        u = self.n_fields_none(self.SOMATIC_NAMES)
        if t >= 4:
            return True
        elif t + u < 4:
            return False
        else:
            return None

    def get_somatic_description(self, req: CamcopsRequest) -> str:
        s = self.meets_criteria_somatic()
        if s is None:
            return self.wxstring(req, "category_somatic_unknown")
        elif s:
            return self.wxstring(req, "category_with_somatic")
        else:
            return self.wxstring(req, "category_without_somatic")

    def get_main_description(self, req: CamcopsRequest) -> str:
        if self.meets_criteria_severe_psychotic_schizophrenic():
            return self.wxstring(req, "category_severe_psychotic_schizophrenic")

        elif self.meets_criteria_severe_psychotic_icd():
            return self.wxstring(req, "category_severe_psychotic")

        elif self.meets_criteria_severe_nonpsychotic():
            return self.wxstring(req, "category_severe_nonpsychotic")

        elif self.meets_criteria_moderate():
            return self.wxstring(req, "category_moderate")

        elif self.meets_criteria_mild():
            return self.wxstring(req, "category_mild")

        elif self.meets_criteria_none():
            return self.wxstring(req, "category_none")

        else:
            return req.sstring(SS.UNKNOWN)

    def get_full_description(self, req: CamcopsRequest) -> str:
        skip_somatic = self.main_complete() and self.meets_criteria_none()
        return (
            self.get_main_description(req) +
            ("" if skip_somatic else " " + self.get_somatic_description(req))
        )

    def is_psychotic_or_stupor(self) -> Optional[bool]:
        if self.count_booleans(self.PSYCHOSIS_NAMES) > 0:
            return True
        elif self.all_fields_not_none(self.PSYCHOSIS_NAMES) > 0:
            return False
        else:
            return None

    def is_complete(self) -> bool:
        return (
            self.date_pertains_to is not None and
            self.main_complete() and
            self.field_contents_valid()
        )

    def text_row(self, req: CamcopsRequest, wstringname: str) -> str:
        return heading_spanning_two_columns(self.wxstring(req, wstringname))

    def row_true_false(self, req: CamcopsRequest, fieldname: str) -> str:
        return self.get_twocol_bool_row_true_false(
            req, fieldname, self.wxstring(req, "" + fieldname))

    def row_present_absent(self, req: CamcopsRequest, fieldname: str) -> str:
        return self.get_twocol_bool_row_present_absent(
            req, fieldname, self.wxstring(req, "" + fieldname))

    def get_task_html(self, req: CamcopsRequest) -> str:
        h = """
            {clinician_comments}
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {date_pertains_to}
                    {category}
                    {n_core}
                    {n_total}
                    {n_somatic}
                    {psychotic_symptoms_or_stupor}
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
                format_datetime(self.date_pertains_to, DateFormat.LONG_DATE,
                                default=None)
            ),
            category=tr_qa(
                req.sstring(SS.CATEGORY) + " <sup>[1,2]</sup>",
                self.get_full_description(req)
            ),
            n_core=tr(
                self.wxstring(req, "n_core"),
                answer(self.n_core()) + " / 3"
            ),
            n_total=tr(
                self.wxstring(req, "n_total"),
                answer(self.n_total()) + " / 10"
            ),
            n_somatic=tr(
                self.wxstring(req, "n_somatic"),
                answer(self.n_somatic()) + " / 8"
            ),
            psychotic_symptoms_or_stupor=tr(
                self.wxstring(req, "psychotic_symptoms_or_stupor") +
                " <sup>[2]</sup>",
                answer(get_present_absent_none(
                    req, self.is_psychotic_or_stupor()))
            ),
            icd10_symptomatic_disclaimer=req.wappstring(
                AS.ICD10_SYMPTOMATIC_DISCLAIMER),
        )

        h += self.text_row(req, "duration_text")
        h += self.row_true_false(req, "duration_at_least_2_weeks")

        h += self.text_row(req, "core")
        for x in self.CORE_NAMES:
            h += self.row_present_absent(req, x)

        h += self.text_row(req, "additional")
        for x in self.ADDITIONAL_NAMES:
            h += self.row_present_absent(req, x)

        h += self.text_row(req, "clinical_text")
        h += self.row_true_false(req, "severe_clinically")

        h += self.text_row(req, "somatic")
        for x in self.SOMATIC_NAMES:
            h += self.row_present_absent(req, x)

        h += self.text_row(req, "psychotic")
        for x in self.PSYCHOSIS_NAMES:
            h += self.row_present_absent(req, x)

        extradetail = [
            f"n_core() = {self.n_core()}",
            f"n_additional() = {self.n_additional()}",
            f"n_total() = {self.n_total()}",
            f"n_somatic() = {self.n_somatic()}",
            f"main_complete() = {self.main_complete()}",
            f"meets_criteria_severe_psychotic_schizophrenic() = {self.meets_criteria_severe_psychotic_schizophrenic()}",  # noqa
            f"meets_criteria_severe_psychotic_icd() = {self.meets_criteria_severe_psychotic_icd()}",  # noqa
            f"meets_criteria_severe_nonpsychotic() = {self.meets_criteria_severe_nonpsychotic()}",  # noqa
            f"meets_criteria_severe_ignoring_psychosis() = {self.meets_criteria_severe_ignoring_psychosis()}",  # noqa
            f"meets_criteria_moderate() = {self.meets_criteria_moderate()}",
            f"meets_criteria_mild() = {self.meets_criteria_mild()}",
            f"meets_criteria_none() = {self.meets_criteria_none()}",
            f"meets_criteria_somatic() = {self.meets_criteria_somatic()}",
        ]

        h += f"""
            </table>
            <div class="{CssClass.HEADING}">Working</div>
            <div class="{CssClass.EXTRADETAIL2}">
                <pre>{"<br>".join(ws.webify(f"‣ {x}") for x in extradetail)}</pre>
            </div>
            <div class="{CssClass.FOOTNOTES}">
                [1] Mild depression requires ≥2 core symptoms and ≥4 total
                diagnostic symptoms.
                Moderate depression requires ≥2 core and ≥6 total.
                Severe depression requires 3 core and ≥8 total.
                All three require a duration of ≥2 weeks.
                In addition, the diagnosis of severe depression is allowed with
                a clinical impression of “severe” in a patient unable/unwilling
                to describe symptoms in detail.
                [2] ICD-10 nonpsychotic severe depression requires severe
                depression without hallucinations/delusions/depressive stupor.
                ICD-10 psychotic depression requires severe depression plus
                hallucinations/delusions other than those that are “typically
                schizophrenic”, or stupor.
                ICD-10 does not clearly categorize severe depression with only
                schizophreniform psychotic symptoms;
                however, such symptoms can occur in severe depression with
                psychosis (e.g. Tandon R &amp; Greden JF, 1987, PMID 2884810).
                Moreover, psychotic symptoms can occur in mild/moderate
                depression (Maj M et al., 2007, PMID 17915981).
            </div>
            {ICD10_COPYRIGHT_DIV}
        """  # noqa
        return h
