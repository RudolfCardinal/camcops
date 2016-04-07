#!/usr/bin/env python3
# icd10depressive.py

"""
    Copyright (C) 2012-2016 Rudolf Cardinal (rudolf@pobox.com).
    Department of Psychiatry, University of Cambridge.
    Funded by the Wellcome Trust.

    This file is part of CamCOPS.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
"""

import cardinal_pythonlib.rnc_web as ws
from cc_modules.cc_constants import (
    CTV_DICTLIST_INCOMPLETE,
    DATEFORMAT,
    ICD10_COPYRIGHT_DIV,
    PV,
)
from cc_modules.cc_dt import format_datetime_string
from cc_modules.cc_html import (
    answer,
    get_present_absent_none,
    heading_spanning_two_columns,
    tr,
    tr_qa,
)
from cc_modules.cc_string import WSTRING
from cc_modules.cc_task import Task


# =============================================================================
# Icd10Depressive
# =============================================================================

class Icd10Depressive(Task):
    CORE_FIELDSPECS = [
        dict(name="mood", cctype="BOOL", pv=PV.BIT,
             comment="Depressed mood to a degree that is definitely abnormal "
             "for the individual, present for most of the day and  almost "
             "every day, largely uninfluenced by circumstances, and sustained "
             "for at least 2 weeks."),
        dict(name="anhedonia", cctype="BOOL", pv=PV.BIT,
             comment="Loss of interest or pleasure in activities  that are "
             "normally pleasurable."),
        dict(name="energy", cctype="BOOL", pv=PV.BIT,
             comment="Decreased energy or increased fatiguability."),
    ]
    ADDITIONAL_FIELDSPECS = [
        dict(name="sleep", cctype="BOOL", pv=PV.BIT,
             comment="Sleep disturbance of any type."),
        dict(name="worth", cctype="BOOL", pv=PV.BIT,
             comment="Loss of confidence and self-esteem."),
        dict(name="appetite", cctype="BOOL", pv=PV.BIT,
             comment="Change in appetite (decrease or increase) with "
             "corresponding weight change."),
        dict(name="guilt", cctype="BOOL", pv=PV.BIT,
             comment="Unreasonable feelings of self-reproach or excessive and "
             "inappropriate guilt."),
        dict(name="concentration", cctype="BOOL", pv=PV.BIT,
             comment="Complaints or evidence of diminished ability to think "
             "or concentrate, such as indecisiveness or vacillation."),
        dict(name="activity", cctype="BOOL", pv=PV.BIT,
             comment="Change in psychomotor activity, with agitation or "
             "retardation (either subjective or objective)."),
        dict(name="death", cctype="BOOL", pv=PV.BIT,
             comment="Recurrent thoughts of death or suicide, or any "
             "suicidal behaviour."),
    ]
    SOMATIC_FIELDSPECS = [
        dict(name="somatic_anhedonia", cctype="BOOL", pv=PV.BIT,
             comment="Marked loss of interest or pleasure in activities that "
             "are normally pleasurable"),
        dict(name="somatic_emotional_unreactivity", cctype="BOOL",
             pv=PV.BIT, comment="Lack of emotional reactions to events or "
             "activities that normally produce an emotional response"),
        dict(name="somatic_early_morning_waking", cctype="BOOL",
             pv=PV.BIT, comment="Waking in the morning 2 hours or more before "
             "the usual time"),
        dict(name="somatic_mood_worse_morning", cctype="BOOL",
             pv=PV.BIT, comment="Depression worse in the morning"),
        dict(name="somatic_psychomotor", cctype="BOOL", pv=PV.BIT,
             comment="Objective evidence of marked psychomotor retardation or "
             "agitation (remarked on or reported by other people)"),
        dict(name="somatic_appetite", cctype="BOOL", pv=PV.BIT,
             comment="Marked loss of appetite"),
        dict(name="somatic_weight", cctype="BOOL", pv=PV.BIT,
             comment="Weight loss (5% or more of body weight in the past "
             "month)"),
        dict(name="somatic_libido", cctype="BOOL", pv=PV.BIT,
             comment="Marked loss of libido"),
    ]
    PSYCHOSIS_FIELDSPECS = [
        dict(name="hallucinations_schizophrenic", cctype="BOOL",
             pv=PV.BIT,
             comment="Hallucinations that are 'typically schizophrenic' "
             "(hallucinatory voices giving a running commentary on the "
             "patient's behaviour, or discussing him between themselves, or "
             "other types of hallucinatory voices coming from some part of "
             "the body)."),
        dict(name="hallucinations_other", cctype="BOOL", pv=PV.BIT,
             comment="Hallucinations (of any other kind)."),
        dict(name="delusions_schizophrenic", cctype="BOOL", pv=PV.BIT,
             comment="Delusions that are 'typically schizophrenic' (delusions "
             "of control, influence or passivity, clearly referred to body or "
             "limb movements or specific thoughts, actions, or sensations; "
             "delusional perception; persistent delusions of other kinds that "
             "are culturally inappropriate and completely impossible)."),
        dict(name="delusions_other", cctype="BOOL", pv=PV.BIT,
             comment="Delusions (of any other kind)."),
        dict(name="stupor", cctype="BOOL", pv=PV.BIT,
             comment="Depressive stupor."),
    ]
    CORE_NAMES = [x["name"] for x in CORE_FIELDSPECS]
    ADDITIONAL_NAMES = [x["name"] for x in ADDITIONAL_FIELDSPECS]
    SOMATIC_NAMES = [x["name"] for x in SOMATIC_FIELDSPECS]
    PSYCHOSIS_NAMES = [x["name"] for x in PSYCHOSIS_FIELDSPECS]

    tablename = "icd10depressive"
    shortname = "ICD10-DEPR"
    longname = (
        "ICD-10 symptomatic criteria for a depressive episode "
        "(as in e.g. F06.3, F25, F31, F32, F33)"
    )
    fieldspecs = (
        [
            dict(name="date_pertains_to", cctype="ISO8601",
                 comment="Date the assessment pertains to"),
            dict(name="comments", cctype="TEXT",
                 comment="Clinician's comments"),
            dict(name="duration_at_least_2_weeks", cctype="BOOL",
                 pv=PV.BIT,
                 comment="Depressive episode lasts at least 2 weeks?"),
            dict(name="severe_clinically", cctype="BOOL", pv=PV.BIT,
                 comment="Clinical impression of severe depression, in a "
                 "patient unwilling or unable to describe many symptoms in "
                 "detail"),
        ] +
        CORE_FIELDSPECS +
        ADDITIONAL_FIELDSPECS +
        SOMATIC_FIELDSPECS +
        PSYCHOSIS_FIELDSPECS
    )
    has_clinician = True

    def get_clinical_text(self):
        if not self.is_complete():
            return CTV_DICTLIST_INCOMPLETE
        dl = [
            {
                "content": "Pertains to: {}. Category: {}.".format(
                    format_datetime_string(self.date_pertains_to,
                                           DATEFORMAT.LONG_DATE),
                    self.get_full_description()
                )
            }
        ]
        if self.comments:
            dl.append({"content": ws.webify(self.comments)})
        return dl

    def get_summaries(self):
        return [
            self.is_complete_summary_field(),
            dict(name="n_core", cctype="INT", value=self.n_core(),
                 comment="Number of core diagnostic symptoms (/3)"),
            dict(name="n_additional", cctype="INT",
                 value=self.n_additional(),
                 comment="Number of additional diagnostic symptoms (/7)"),
            dict(name="n_total", cctype="INT", value=self.n_total(),
                 comment="Total number of diagnostic symptoms (/10)"),
            dict(name="n_somatic", cctype="INT", value=self.n_somatic(),
                 comment="Number of somatic syndrome symptoms (/8)"),
            dict(name="category", cctype="TEXT",
                 value=self.get_full_description(),
                 comment="Diagnostic category"),
            dict(name="psychosis_or_stupor", cctype="BOOL",
                 value=self.is_psychotic_or_stupor(),
                 comment="Psychotic symptoms or stupor present?"),
        ]

    # Scoring
    def n_core(self):
        return self.count_booleans(Icd10Depressive.CORE_NAMES)

    def n_additional(self):
        return self.count_booleans(Icd10Depressive.ADDITIONAL_NAMES)

    def n_total(self):
        return self.n_core() + self.n_additional()

    def n_somatic(self):
        return self.count_booleans(Icd10Depressive.SOMATIC_NAMES)

    def main_complete(self):
        return (
            self.duration_at_least_2_weeks is not None and
            self.are_all_fields_complete(Icd10Depressive.CORE_NAMES) and
            self.are_all_fields_complete(Icd10Depressive.ADDITIONAL_NAMES)
        ) or (
            self.severe_clinically
        )

    # Meets criteria? These also return null for unknown.
    def meets_criteria_severe_psychotic_schizophrenic(self):
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

    def meets_criteria_severe_psychotic_icd(self):
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

    def meets_criteria_severe_nonpsychotic(self):
        x = self.meets_criteria_severe_ignoring_psychosis()
        if not x:
            return x
        if not self.are_all_fields_complete(Icd10Depressive.PSYCHOSIS_NAMES):
            return None
        return (self.count_booleans(Icd10Depressive.PSYCHOSIS_NAMES) == 0)

    def meets_criteria_severe_ignoring_psychosis(self):
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

    def meets_criteria_moderate(self):
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

    def meets_criteria_mild(self):
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

    def meets_criteria_none(self):
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

    def meets_criteria_somatic(self):
        t = self.n_somatic()
        u = self.n_incomplete(Icd10Depressive.SOMATIC_NAMES)
        if t >= 4:
            return True
        elif t + u < 4:
            return False
        else:
            return None

    def get_somatic_description(self):
        s = self.meets_criteria_somatic()
        if s is None:
            return WSTRING("icd10depressive_category_somatic_unknown")
        elif s:
            return WSTRING("icd10depressive_category_with_somatic")
        else:
            return WSTRING("icd10depressive_category_without_somatic")

    def get_main_description(self):
        if self.meets_criteria_severe_psychotic_schizophrenic():
            return WSTRING(
                "icd10depressive_category_severe_psychotic_schizophrenic")

        elif self.meets_criteria_severe_psychotic_icd():
            return WSTRING("icd10depressive_category_severe_psychotic")

        elif self.meets_criteria_severe_nonpsychotic():
            return WSTRING("icd10depressive_category_severe_nonpsychotic")

        elif self.meets_criteria_moderate():
            return WSTRING("icd10depressive_category_moderate")

        elif self.meets_criteria_mild():
            return WSTRING("icd10depressive_category_mild")

        elif self.meets_criteria_none():
            return WSTRING("icd10depressive_category_none")

        else:
            return WSTRING("Unknown")

    def get_full_description(self):
        skipSomatic = self.main_complete() and self.meets_criteria_none()
        return (
            self.get_main_description() +
            ("" if skipSomatic else " " + self.get_somatic_description())
        )

    def is_psychotic_or_stupor(self):
        if self.count_booleans(Icd10Depressive.PSYCHOSIS_NAMES) > 0:
            return True
        elif self.are_all_fields_complete(Icd10Depressive.PSYCHOSIS_NAMES) > 0:
            return False
        else:
            return None

    def is_complete(self):
        return (
            self.date_pertains_to is not None and
            self.main_complete() and
            self.field_contents_valid()
        )

    def text_row(self, wstringname):
        return heading_spanning_two_columns(WSTRING(wstringname))

    def row_true_false(self, fieldname):
        return self.get_twocol_bool_row_true_false(
            fieldname, WSTRING("icd10depressive_" + fieldname))

    def row_present_absent(self, fieldname):
        return self.get_twocol_bool_row_present_absent(
            fieldname, WSTRING("icd10depressive_" + fieldname))

    def get_task_html(self):
        h = self.get_standard_clinician_comments_block(self.comments) + """
            <div class="summary">
                <table class="summary">
        """ + self.get_is_complete_tr()
        h += tr_qa(WSTRING("date_pertains_to"),
                   format_datetime_string(self.date_pertains_to,
                                          DATEFORMAT.LONG_DATE,
                                          default=None))
        h += tr_qa(WSTRING("category") + " <sup>[1,2]</sup>",
                   self.get_full_description())
        h += tr(WSTRING("icd10depressive_n_core"),
                answer(self.n_core()) + " / 3")
        h += tr(WSTRING("icd10depressive_n_total"),
                answer(self.n_total()) + " / 10")
        h += tr(WSTRING("icd10depressive_n_somatic"),
                answer(self.n_somatic()) + " / 8")
        h += tr(WSTRING("icd10depressive_psychotic_symptoms_or_stupor") +
                " <sup>[2]</sup>",
                answer(get_present_absent_none(self.is_psychotic_or_stupor())))
        h += """
                </table>
            </div>
            <div class="explanation">
        """
        h += WSTRING("icd10_symptomatic_disclaimer")
        h += """
            </div>
            <table class="taskdetail">
                <tr>
                    <th width="80%">Question</th>
                    <th width="20%">Answer</th>
                </tr>
        """

        h += self.text_row("icd10depressive_duration_text")
        h += self.row_true_false("duration_at_least_2_weeks")

        h += self.text_row("icd10depressive_core")
        for x in Icd10Depressive.CORE_NAMES:
            h += self.row_present_absent(x)

        h += self.text_row("icd10depressive_additional")
        for x in Icd10Depressive.ADDITIONAL_NAMES:
            h += self.row_present_absent(x)

        h += self.text_row("icd10depressive_clinical_text")
        h += self.row_true_false("severe_clinically")

        h += self.text_row("icd10depressive_somatic")
        for x in Icd10Depressive.SOMATIC_NAMES:
            h += self.row_present_absent(x)

        h += self.text_row("icd10depressive_psychotic")
        for x in Icd10Depressive.PSYCHOSIS_NAMES:
            h += self.row_present_absent(x)

        h += """
            </table>
            <div class="footnotes">
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
        """ + ICD10_COPYRIGHT_DIV
        return h
