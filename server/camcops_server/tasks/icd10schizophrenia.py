#!/usr/bin/env python

"""
camcops_server/tasks/icd10schizophrenia.py

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
from cardinal_pythonlib.typetests import is_false
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Boolean, Date, UnicodeText

from camcops_server.cc_modules.cc_constants import (
    CssClass,
    DateFormat,
    ICD10_COPYRIGHT_DIV,
)
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_html import (
    get_true_false_none,
    heading_spanning_two_columns,
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
# Icd10Schizophrenia
# =============================================================================

class Icd10Schizophrenia(TaskHasClinicianMixin, TaskHasPatientMixin, Task):
    """
    Server implementation of the ICD10-SZ task.
    """
    __tablename__ = "icd10schizophrenia"
    shortname = "ICD10-SZ"

    passivity_bodily = CamcopsColumn(
        "passivity_bodily", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Passivity: delusions of control, influence, or "
                "passivity, clearly referred to body or limb movements..."
    )
    passivity_mental = CamcopsColumn(
        "passivity_mental", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="(passivity) ... or to specific thoughts, actions, or "
                "sensations."
    )
    hv_commentary = CamcopsColumn(
        "hv_commentary", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Hallucinatory voices giving a running commentary on the "
                "patient's behaviour"
    )
    hv_discussing = CamcopsColumn(
        "hv_discussing", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Hallucinatory voices discussing the patient among "
                "themselves"
    )
    hv_from_body = CamcopsColumn(
        "hv_from_body", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Other types of hallucinatory voices coming from some "
                "part of the body"
    )
    delusions = CamcopsColumn(
        "delusions", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Delusions: persistent delusions of other kinds that are "
                "culturally inappropriate and completely impossible, such as "
                "religious or political identity, or superhuman powers and "
                "abilities (e.g. being able to control the weather, or being "
                "in communication with aliens from another world)."
    )
    delusional_perception = CamcopsColumn(
        "delusional_perception", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Delusional perception [a normal perception, "
                "delusionally interpreted]"
    )
    thought_echo = CamcopsColumn(
        "thought_echo", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Thought echo [hearing one's own thoughts aloud, just "
                "before, just after, or simultaneously with the thought]"
    )
    thought_withdrawal = CamcopsColumn(
        "thought_withdrawal", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Thought withdrawal [the feeling that one's thoughts "
                "have been removed by an outside agency]"
    )
    thought_insertion = CamcopsColumn(
        "thought_insertion", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Thought insertion [the feeling that one's thoughts have "
                "been placed there from outside]"
    )
    thought_broadcasting = CamcopsColumn(
        "thought_broadcasting", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Thought broadcasting [the feeling that one's thoughts "
                "leave oneself and are diffused widely, or are audible to "
                "others, or that others think the same thoughts in unison]"
    )

    hallucinations_other = CamcopsColumn(
        "hallucinations_other", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Hallucinations: persistent hallucinations in any "
                "modality, when accompanied either by fleeting or half-formed "
                "delusions without clear affective content, or by persistent "
                "over-valued ideas, or when occurring every day for weeks or "
                "months on end."
    )
    thought_disorder = CamcopsColumn(
        "thought_disorder", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Thought disorder: breaks or interpolations in the train "
                "of thought, resulting in incoherence or irrelevant speech, "
                "or neologisms."
    )
    catatonia = CamcopsColumn(
        "catatonia", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Catatonia: catatonic behaviour, such as excitement, "
                "posturing, or waxy flexibility, negativism, mutism, and "
                "stupor."
    )

    negative = CamcopsColumn(
        "negative", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Negative symptoms: 'negative' symptoms such as marked "
                "apathy, paucity of speech, and blunting or incongruity of "
                "emotional responses, usually resulting in social withdrawal "
                "and lowering of social performance; it must be clear that "
                "these are not due to depression or to neuroleptic "
                "medication."
    )

    present_one_month = CamcopsColumn(
        "present_one_month", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Symptoms in groups A-C present for most of the time "
                "during an episode of psychotic illness lasting for at least "
                "one month (or at some time during most of the days)."
    )

    also_manic = CamcopsColumn(
        "also_manic", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Also meets criteria for manic episode (F30)?"
    )
    also_depressive = CamcopsColumn(
        "also_depressive", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Also meets criteria for depressive episode (F32)?"
    )
    if_mood_psychosis_first = CamcopsColumn(
        "if_mood_psychosis_first", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="If the patient also meets criteria for manic episode "
                "(F30) or depressive episode (F32), the criteria listed above "
                "must have been met before the disturbance of mood developed."
    )

    not_organic_or_substance = CamcopsColumn(
        "not_organic_or_substance", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="The disorder is not attributable to organic brain "
                "disease (in the sense of F0), or to alcohol- or drug-related "
                "intoxication, dependence or withdrawal."
    )

    behaviour_change = CamcopsColumn(
        "behaviour_change", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="A significant and consistent change in the overall "
                "quality of some aspects of personal behaviour, manifest as "
                "loss of interest, aimlessness, idleness, a self-absorbed "
                "attitude, and social withdrawal."
    )
    performance_decline = CamcopsColumn(
        "performance_decline", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Marked decline in social, scholastic, or occupational "
                "performance."
    )

    subtype_paranoid = CamcopsColumn(
        "subtype_paranoid", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="PARANOID (F20.0): dominated by delusions or hallucinations."
    )
    subtype_hebephrenic = CamcopsColumn(
        "subtype_hebephrenic", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="HEBEPHRENIC (F20.1): dominated by affective changes "
                "(shallow, flat, incongruous, or inappropriate affect) and "
                "either pronounced thought disorder or aimless, disjointed "
                "behaviour is present."
    )
    subtype_catatonic = CamcopsColumn(
        "subtype_catatonic", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="CATATONIC (F20.2): psychomotor disturbances dominate "
                "(such as stupor, mutism, excitement, posturing, negativism, "
                "rigidity, waxy flexibility, command automatisms, or verbal "
                "perseveration)."
    )
    subtype_undifferentiated = CamcopsColumn(
        "subtype_undifferentiated", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="UNDIFFERENTIATED (F20.3): schizophrenia with active "
                "psychosis fitting none or more than one of the above three "
                "types."
    )
    subtype_postschizophrenic_depression = CamcopsColumn(
        "subtype_postschizophrenic_depression", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="POST-SCHIZOPHRENIC DEPRESSION (F20.4): in which a depressive "
                "episode has developed for at least 2 weeks following a "
                "schizophrenic episode within the last 12 months and in which "
                "schizophrenic symptoms persist but are not as prominent as "
                "the depression."
    )
    subtype_residual = CamcopsColumn(
        "subtype_residual", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="RESIDUAL (F20.5): in which previous psychotic episodes "
                "of schizophrenia have given way to a chronic condition with "
                "'negative' symptoms of schizophrenia for at least 1 year."
    )
    subtype_simple = CamcopsColumn(
        "subtype_simple", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="SIMPLE SCHIZOPHRENIA (F20.6), in which 'negative' "
                "symptoms (C) with a change in personal behaviour (D) develop "
                "for at least one year without any psychotic episodes (no "
                "symptoms from groups A or B or other hallucinations or "
                "well-formed delusions), and with a marked decline in social, "
                "scholastic, or occupational performance."
    )
    subtype_cenesthopathic = CamcopsColumn(
        "subtype_cenesthopathic", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="CENESTHOPATHIC (within OTHER F20.8): body image "
                "aberration (e.g. desomatization, loss of bodily boundaries, "
                "feelings of body size change) or abnormal bodily sensations "
                "(e.g. numbness, stiffness, feeling strange, "
                "depersonalization, or sensations of pain, temperature, "
                "electricity, heaviness, lightness, or discomfort when "
                "touched) dominate."
    )

    date_pertains_to = Column(
        "date_pertains_to", Date,
        comment="Date the assessment pertains to"
    )
    comments = Column(
        "comments", UnicodeText,
        comment="Clinician's comments"
    )

    A_NAMES = [
        "passivity_bodily", "passivity_mental",
        "hv_commentary", "hv_discussing", "hv_from_body",
        "delusions", "delusional_perception",
        "thought_echo", "thought_withdrawal", "thought_insertion",
        "thought_broadcasting"
    ]
    B_NAMES = ["hallucinations_other", "thought_disorder", "catatonia"]
    C_NAMES = ["negative"]
    D_NAMES = ["present_one_month"]
    E_NAMES = ["also_manic", "also_depressive", "if_mood_psychosis_first"]
    F_NAMES = ["not_organic_or_substance"]
    G_NAMES = ["behaviour_change", "performance_decline"]
    H_NAMES = [
        "subtype_paranoid", "subtype_hebephrenic", "subtype_catatonic",
        "subtype_undifferentiated", "subtype_postschizophrenic_depression",
        "subtype_residual", "subtype_simple", "subtype_cenesthopathic"
    ]

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("ICD-10 criteria for schizophrenia (F20)")

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        c = self.meets_general_criteria()
        if c is None:
            category = "Unknown if met or not met"
        elif c:
            category = "Met"
        else:
            category = "Not met"
        infolist = [CtvInfo(
            content=(
                "Pertains to: {}. General criteria for "
                "schizophrenia: {}.".format(
                    format_datetime(self.date_pertains_to,
                                    DateFormat.LONG_DATE),
                    category
                )
            )
        )]
        if self.comments:
            infolist.append(CtvInfo(content=ws.webify(self.comments)))
        return infolist

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="meets_general_criteria",
                coltype=Boolean(),
                value=self.meets_general_criteria(),
                comment="Meets general criteria for paranoid/hebephrenic/"
                        "catatonic/undifferentiated schizophrenia "
                        "(F20.0-F20.3)?"),
        ]

    # Meets criteria? These also return null for unknown.
    def meets_general_criteria(self) -> Optional[bool]:
        t_a = self.count_booleans(Icd10Schizophrenia.A_NAMES)
        u_a = self.n_fields_none(Icd10Schizophrenia.A_NAMES)
        t_b = (
            self.count_booleans(Icd10Schizophrenia.B_NAMES) +
            self.count_booleans(Icd10Schizophrenia.C_NAMES)
        )
        u_b = (
            self.n_fields_none(Icd10Schizophrenia.B_NAMES) +
            self.n_fields_none(Icd10Schizophrenia.C_NAMES)
        )
        if t_a + u_a < 1 and t_b + u_b < 2:
            return False
        if self.present_one_month is not None and not self.present_one_month:
            return False
        if ((self.also_manic or self.also_depressive) and
                is_false(self.if_mood_psychosis_first)):
            return False
        if is_false(self.not_organic_or_substance):
            return False
        if ((t_a >= 1 or t_b >= 2) and
                self.present_one_month and
                (
                    (is_false(self.also_manic) and
                        is_false(self.also_depressive)) or
                    self.if_mood_psychosis_first
                ) and
                self.not_organic_or_substance):
            return True
        return None

    def is_complete(self) -> bool:
        return (
            self.date_pertains_to is not None and
            self.meets_general_criteria() is not None and
            self.field_contents_valid()
        )

    def heading_row(self, req: CamcopsRequest, wstringname: str,
                    extra: str = None) -> str:
        return heading_spanning_two_columns(
            self.wxstring(req, wstringname) + (extra or "")
        )

    def text_row(self, req: CamcopsRequest, wstringname: str) -> str:
        return subheading_spanning_two_columns(self.wxstring(req, wstringname))

    def row_true_false(self, req: CamcopsRequest, fieldname: str) -> str:
        return self.get_twocol_bool_row_true_false(
            req, fieldname, self.wxstring(req, fieldname))

    def row_present_absent(self, req: CamcopsRequest, fieldname: str) -> str:
        return self.get_twocol_bool_row_present_absent(
            req, fieldname, self.wxstring(req, fieldname))

    def get_task_html(self, req: CamcopsRequest) -> str:
        h = """
            {clinician_comments}
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {date_pertains_to}
                    {meets_general_criteria}
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                {comments}
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
            meets_general_criteria=tr_qa(
                self.wxstring(req, "meets_general_criteria") + " <sup>[1]</sup>",  # noqa
                get_true_false_none(req, self.meets_general_criteria())
            ),
            comments=self.wxstring(req, "comments"),
        )

        h += self.heading_row(req, "core", " <sup>[2]</sup>")
        for x in Icd10Schizophrenia.A_NAMES:
            h += self.row_present_absent(req, x)

        h += self.heading_row(req, "other_positive")
        for x in Icd10Schizophrenia.B_NAMES:
            h += self.row_present_absent(req, x)

        h += self.heading_row(req, "negative_title")
        for x in Icd10Schizophrenia.C_NAMES:
            h += self.row_present_absent(req, x)

        h += self.heading_row(req, "other_criteria")
        for x in Icd10Schizophrenia.D_NAMES:
            h += self.row_true_false(req, x)
        h += self.text_row(req, "duration_comment")
        for x in Icd10Schizophrenia.E_NAMES:
            h += self.row_true_false(req, x)
        h += self.text_row(req, "affective_comment")
        for x in Icd10Schizophrenia.F_NAMES:
            h += self.row_true_false(req, x)

        h += self.heading_row(req, "simple_title")
        for x in Icd10Schizophrenia.G_NAMES:
            h += self.row_present_absent(req, x)

        h += self.heading_row(req, "subtypes")
        for x in Icd10Schizophrenia.H_NAMES:
            h += self.row_present_absent(req, x)

        h += f"""
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] All of:
                    (a) at least one core symptom, or at least two of the other
                        positive or negative symptoms;
                    (b) present for a month (etc.);
                    (c) if also manic/depressed, schizophreniform psychosis
                        came first;
                    (d) not attributable to organic brain disease or
                        psychoactive substances.
                [2] Symptom definitions from:
                    (a) Oyebode F (2008). Sims’ Symptoms in the Mind: An
                        Introduction to Descriptive Psychopathology. Fourth
                        edition, Saunders, Elsevier, Edinburgh.
                    (b) Pawar AV &amp; Spence SA (2003), PMID 14519605.
            </div>
            {ICD10_COPYRIGHT_DIV}
        """
        return h
