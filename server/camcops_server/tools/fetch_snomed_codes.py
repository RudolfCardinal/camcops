#!/usr/bin/env python

"""
camcops_server/tools/fetch_snomed_codes.py

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

Assists a user who has the necessary permission to look up SNOMED CT
identifiers from a SNOMED server (e.g. of their national provider).

References:

- https://ihtsdo.github.io/sct-snapshot-rest-api/api.html
- https://termbrowser.nhs.uk/

"""

import argparse
from collections import OrderedDict
import json
import logging
import requests
import sys
from typing import Dict, List, Optional, Set, Tuple, Union

from cardinal_pythonlib.argparse_func import ShowAllSubparserHelpAction
from cardinal_pythonlib.json.typing_helpers import JsonValueType
from cardinal_pythonlib.logs import main_only_quicksetup_rootlogger
from cardinal_pythonlib.rate_limiting import rate_limited
from cardinal_pythonlib.snomed import SnomedConcept

from camcops_server.cc_modules.cc_snomed import SnomedLookup

log = logging.getLogger(__name__)


# =============================================================================
# Constants
# =============================================================================

DEFAULT_URL = "https://termbrowser.nhs.uk/sct-browser-api/snomed"
DEFAULT_EDITION = "uk-edition"
DEFAULT_RELEASE = "v20191001"
DEFAULT_LANGUAGE = "english"
DEFAULT_RATE_LIMIT_HZ = 1  # be nice
DEFAULT_OUTPUT_XML_FILENAME = "camcops_tasks_snomed.xml"

HTTP_200_OK = 200

DISCLAIMER_1 = """
SNOMED Clinical Terms® (SNOMED CT®) is owned by the International Health
Terminology Standards Development Organization (IHTSDO), trading as SNOMED
International. All rights reserved.

Some people/organizations in some countries are authorized to use SNOMED CT
identifiers for data creation systems. Others are not. See e.g.

    https://snomed.org/licensing
    https://www.snomed.org/snomed-ct/get-snomed
    https://termbrowser.nhs.uk

This tool will use a SNOMED REST API server of your choice to look for
relevant SNOMED CT identifiers. Use it ONLY if you have permission (and have
paid any necessary fees). The authors of CamCOPS disclaim all responsibility
for your decisions.

Are you authorized to proceed?
"""
ANSWER_1 = "yes"
DISCLAIMER_2 = """
Are you sure? You take all legal responsibility for the consequences.
"""
ANSWER_2 = "sure"

# NB no newline allowed at start
XML_HEADER = """<?xml version="1.0" encoding="UTF-8"?>
<snomed_concepts>

    <!--
    =============================================================================
    Explanation
    =============================================================================

    SNOMED-CT codes for CamCOPS tasks.

    Each is in a <lookup> node.

    The "name" attribute is the CamCOPS string name for this lookup; see
        camcops_server.cc_modules.cc_snomed.SnomedLookup.

    Each lookup should (in this file) contain exactly one <concept>.
    (Multiple concepts per lookup are permitted elsewhere, e.g. ICD-9/ICD-10.)

    Within the <concept> nodes there should be:
      <id>: SNOMED-CT concept identifier (integer)
      <term>: name, or SNOMED-CT associated term (description) (string)
        ... there are often synonyms.
        ... prefer the Fully Specified Nname, FSN (with the type at the end)

    Find them at e.g.:
        https://termbrowser.nhs.uk/
    but reproduction of the SNOMED CT identifiers is conditional on licensing
    arrangements and these identifiers are not in (or supplied with) CamCOPS.

    Explanation for each ID is at:
        https://snomedbrowser.com/Codes/Details/XXX

    Here's a template:

    <lookup name="XXX"><concept><id>XXX</id><term>XXX</term></concept></lookup>

    -->
"""

XML_FOOTER = """
</snomed_concepts>
"""


class SemanticTypes(object):
    DIS = "disorder"
    FIND = "finding"
    OBJ = "physical object"
    OBS = "observable entity"
    PROC = "procedure"
    REC = "record artifact"
    SCALE = "assessment scale"
    QV = "qualifier value"


SL = SnomedLookup
ST = SemanticTypes


ASSESS = "Assessment using"
EXAM = "examination"
MEASURE = "Measurement of"
NEG_SCREEN = "Negative screening for"
ON = "on"
POS_SCREEN = "Positive screening for"
RATING_SCALE = "rating scale"
SCALE = "scale"
SCREEN = "screening using"
SCORE = "score"
TOTSCORE = "total score"

ACE_R = "Addenbrooke's cognitive examination revised"
AIMS = "Abnormal Involuntary Movement Scale"
AUDIT = "Alcohol Use Disorders Identification Test"
AUDITC = "Alcohol Use Disorders Identification Test - Consumption"
BADLS = "Bristol activities of daily living scale"
BDI = "Beck depression inventory"
BDI_II = "Beck Depression Inventory II"
BMI = "Body mass index"
CAGE = "Cutting down, Annoyance by criticism, Guilty feeling and Eye-openers Questionnaire"  # noqa
CIWA_1 = "Clinical Institute Withdrawal Assessment for Alcohol scale, revised"
CIWA_2 = "Clinical Institute Withdrawal of Alcohol Scale, revised"
CIWA_3 = "Clinical Institute Withdrawal Assessment of Alcohol Scale, revised"
CORE10_1 = "Clinical Outcomes in Routine Evaluation - 10"
CORE10_2 = "Clinical Outcomes in Routine Evaluation 10"
EQ5D5L = "EuroQol five dimension five level"
FAST = "Fast Alcohol Screening Test"
GAD7 = "Generalized anxiety disorder 7 item"
GDS15 = "Geriatric depression scale short form"
HADS = "Hospital Anxiety and Depression scale"
HDRS = "Hamilton Rating Scale for Depression"

HONOSCA_1 = "Health of the Nation Outcome Scale for Children and Adolescents"
HONOSCA_2 = "Health of the Nation Outcome Scales for Children and Adolescents"
HONOSWA_1 = "Health of the Nation Outcome Scale for working age adults"
HONOSWA_2 = "Health of the Nation Outcome Scales for working age adults"
HONOS65_1 = "Health of the Nation Outcome Scale for older adults"
HONOS65_2 = "Health of the Nation Outcome Scales 65+"
HONOSWA_S1 = "overactive, aggressive, disruptive or agitated behaviour"
HONOSWA_S2 = "non-accidental self-injury"
HONOSWA_S3 = "problem drinking or drug-taking"
HONOSWA_S4 = "cognitive problems"
HONOSWA_S5 = "physical illness or disability problems"
HONOSWA_S6 = "problems associated with hallucinations and delusions"
HONOSWA_S7 = "problems with depressed mood"
HONOSWA_S8 = "other mental and behavioural problems"
HONOSWA_S9 = "problems with relationships"
HONOSWA_S10 = "problems with activities of daily living"
HONOSWA_S11 = "problems with living conditions"
HONOSWA_S12 = "problems with occupation and activities"

IESR = "Impact of event scale revised"
MAST = "Michigan Alcoholism Screening Test"
MOCA = "Montreal cognitive assessment"
NART = "National Adult Reading Test"
PDSS = "Panic disorder severity scale"
PHQ9 = "Patient Health Questionnaire 9"
PHQ9W = "Patient Health Questionnaire Nine Item"
PHQ15 = "Patient Health Questionnaire 15"
PSWQ = "Penn State worry questionnaire"
WEMWBS1 = "Warwick Edinburgh Mental Well Being Scale"
WEMWBS2 = "Warwick-Edinburgh Mental Well-Being Scale"
SWEMWBS1 = "Short Warwick Edinburgh Mental Well-Being Scale"
SWEMWBS2 = "Short Warwick-Edinburgh Mental Well-Being Scale"
WSAS = "Improving Access to Psychological Therapies programme Work and Social Adjustment Scale"  # noqa

CCMAP = OrderedDict([
    (SL.MASS, ("Mass, a measure of quantity of matter", ST.QV, " (property)")),
    (SL.LENGTH, ("Length", ST.QV, " property")),
    (SL.UNIT_OF_MEASURE, ("Unit of measure", ST.QV)),
    (SL.KILOGRAM, ("kilogram", ST.QV)),
    (SL.METRE, ("meter", ST.QV)),
    (SL.CENTIMETRE, ("Centimeter", ST.QV)),
    (SL.KG_PER_SQ_M, ("Kilogram/square meter", ST.QV)),

    (SL.ACE_R_SCALE, (ACE_R, ST.SCALE)),
    (SL.ACE_R_SUBSCALE_ATTENTION_ORIENTATION, (f"{ACE_R} attention and orientation subscale", ST.SCALE)),  # noqa
    (SL.ACE_R_SUBSCALE_FLUENCY, (f"{ACE_R} fluency subscale", ST.SCALE)),
    (SL.ACE_R_SUBSCALE_LANGUAGE, (f"{ACE_R} language subscale", ST.SCALE)),
    (SL.ACE_R_SUBSCALE_MEMORY, (f"{ACE_R} memory subscale", ST.SCALE)),
    (SL.ACE_R_SUBSCALE_VISUOSPATIAL, (f"{ACE_R} visuospatial subscale", ST.SCALE)),  # noqa
    (SL.ACE_R_SCORE, (f"{ACE_R} - score", ST.OBS)),
    (SL.ACE_R_SUBSCORE_ATTENTION_ORIENTATION, (f"{ACE_R} - attention and orientation subscore", ST.OBS)),  # noqa
    (SL.ACE_R_SUBSCORE_FLUENCY, (f"{ACE_R} - fluency subscore", ST.OBS)),
    (SL.ACE_R_SUBSCORE_LANGUAGE, (f"{ACE_R} - language subscore", ST.OBS)),
    (SL.ACE_R_SUBSCORE_MEMORY, (f"{ACE_R} - memory subscore", ST.OBS)),
    (SL.ACE_R_SUBSCORE_VISUOSPATIAL, (f"{ACE_R} - visuospatial subscore", ST.OBS)),  # noqa
    (SL.ACE_R_PROCEDURE_ASSESSMENT, (f"{ASSESS} {ACE_R}", ST.PROC)),
    (SL.ACE_R_PROCEDURE_ASSESSMENT_SUBSCALE_ATTENTION_ORIENTATION, (f"{ASSESS} {ACE_R} attention and orientation subscale", ST.PROC)),  # noqa
    (SL.ACE_R_PROCEDURE_ASSESSMENT_SUBSCALE_FLUENCY, (f"{ASSESS} {ACE_R} fluency subscale", ST.PROC)),  # noqa
    (SL.ACE_R_PROCEDURE_ASSESSMENT_SUBSCALE_LANGUAGE, (f"{ASSESS} {ACE_R} language subscale", ST.PROC)),  # noqa
    (SL.ACE_R_PROCEDURE_ASSESSMENT_SUBSCALE_MEMORY, (f"{ASSESS} {ACE_R} memory subscale", ST.PROC)),  # noqa
    (SL.ACE_R_PROCEDURE_ASSESSMENT_SUBSCALE_VISUOSPATIAL, (f"{ASSESS} {ACE_R} visuospatial subscale", ST.PROC)),  # noqa

    (SL.AIMS_SCALE, (f"{AIMS}", ST.SCALE)),
    (SL.AIMS_TOTAL_SCORE, (f"{AIMS} {TOTSCORE}", ST.OBS)),
    (SL.AIMS_PROCEDURE_ASSESSMENT, (f"{ASSESS} {AIMS}", ST.PROC)),

    (SL.AUDIT_SCALE, (f"{AUDIT}", ST.SCALE)),
    (SL.AUDIT_SCORE, (f"{AUDIT} {SCORE}", ST.OBS)),
    (SL.AUDIT_PROCEDURE_ASSESSMENT, (f"{ASSESS} {AUDIT}", ST.PROC)),
    (SL.AUDITC_SCALE, ("Alcohol use disorder identification test consumption questionnaire", ST.SCALE)),  # noqa
    (SL.AUDITC_SCORE, (f"{AUDITC} {SCORE}", ST.OBS)),
    (SL.AUDITC_PROCEDURE_ASSESSMENT, (f"{ASSESS} {AUDITC}", ST.PROC)),

    (SL.BADLS_SCALE, (f"{BADLS}", ST.SCALE)),
    (SL.BADLS_SCORE, (f"{BADLS} {SCORE}", ST.OBS)),
    (SL.BADLS_PROCEDURE_ASSESSMENT, (f"{ASSESS} {BADLS}", ST.PROC)),

    (SL.BDI_SCALE, (f"{BDI}", ST.SCALE)),
    (SL.BDI_SCORE, (f"{BDI} {SCORE}", ST.OBS)),
    (SL.BDI_PROCEDURE_ASSESSMENT, (f"{ASSESS} {BDI}", ST.PROC)),
    (SL.BDI_II_SCORE, (f"{BDI_II} {SCORE}", ST.OBS)),
    (SL.BDI_II_PROCEDURE_ASSESSMENT, (f"{ASSESS} {BDI_II}", ST.PROC)),

    (SL.BMI_OBSERVABLE, (f"{BMI}", ST.OBS)),
    (SL.BMI_PROCEDURE_MEASUREMENT, (f"{MEASURE} {BMI}", ST.PROC)),
    (SL.BODY_HEIGHT_OBSERVABLE, ("Body height measure", ST.OBS)),
    (SL.BODY_WEIGHT_OBSERVABLE, ("Body weight", ST.OBS)),
    (SL.WAIST_CIRCUMFERENCE_PROCEDURE_MEASUREMENT, (f"{MEASURE} circumference of waist", ST.PROC)),  # noqa
    (SL.WAIST_CIRCUMFERENCE_OBSERVABLE, ("Waist circumference", ST.OBS)),

    (SL.BPRS1962_SCALE, ("Brief psychiatric rating scale - 1962", ST.SCALE)),

    (SL.CAGE_SCALE, ("Cage questionnaire", ST.SCALE)),
    (SL.CAGE_SCORE, (f"{CAGE} {TOTSCORE}", ST.OBS)),
    (SL.CAGE_PROCEDURE_ASSESSMENT, (f"{ASSESS} {CAGE}", ST.PROC)),

    (SL.CIWA_AR_SCALE, (f"{CIWA_1}", ST.SCALE)),
    (SL.CIWA_AR_SCORE, (f"{CIWA_2} {SCORE}", ST.OBS)),
    (SL.CIWA_AR_PROCEDURE_ASSESSMENT, (f"{ASSESS} {CIWA_3}", ST.PROC)),

    (SL.PROGRESS_NOTE_PROCEDURE, ("Chart review by physician, update", ST.PROC)),  # noqa
    (SL.CLINICAL_NOTE, ("Clinical note", ST.REC)),

    (SL.PHOTOGRAPH_PROCEDURE, ("Medical photography", ST.PROC)),
    (SL.PHOTOGRAPH_PHYSICAL_OBJECT, ("Photograph", ST.OBJ)),

    (SL.PSYCHIATRIC_ASSESSMENT_PROCEDURE, (f"Psychiatric diagnostic interview, examination, history, mental status and disposition", ST.PROC)),  # noqa

    (SL.PSYCLERK_REASON_FOR_REFERRAL, ("Reason for referral", ST.REC)),
    (SL.PSYCLERK_PRESENTING_ISSUE, ("Presenting complaints or issues", ST.REC)),  # NOQA
    (SL.PSYCLERK_SYSTEMS_REVIEW, ("Review of systems", ST.REC)),
    (SL.PSYCLERK_COLLATERAL_HISTORY, ("History obtained from third party", ST.FIND)),  # noqa

    (SL.PSYCLERK_PAST_MEDICAL_SURGICAL_MENTAL_HEALTH_HISTORY, ("Relevant past medical, surgical and mental health history", ST.REC)),  # noqa
    (SL.PSYCLERK_PROCEDURES, ("Procedures", ST.REC)),
    (SL.PSYCLERK_ALLERGIES_ADVERSE_REACTIONS, ("Allergies and adverse reaction", ST.REC)),  # noqa
    (SL.PSYCLERK_MEDICATIONS_MEDICAL_DEVICES, ("Medications and medical devices", ST.REC)),  # noqa
    (SL.PSYCLERK_DRUG_SUBSTANCE_USE, ("Drug/substance use", ST.REC)),
    (SL.PSYCLERK_FAMILY_HISTORY, ("Family history", ST.REC)),
    (SL.PSYCLERK_DEVELOPMENTAL_HISTORY, ("Child developmental detail", ST.OBS)),  # noqa
    (SL.PSYCLERK_SOCIAL_PERSONAL_HISTORY, ("Social / personal history observable", ST.OBS)),  # noqa
    (SL.PSYCLERK_PERSONALITY, ("Personality, function", ST.OBS)),
    (SL.PSYCLERK_PRISON_RECORD_CRIMINAL_ACTIVITY, ("Prison record and criminal activity details", ST.OBS)),  # noqa
    (SL.PSYCLERK_SOCIAL_HISTORY_BASELINE, ("Social history baseline", ST.OBS)),

    (SL.PSYCLERK_MSE_APPEARANCE, ("Appearance", ST.OBS)),
    (SL.PSYCLERK_MSE_BEHAVIOUR, ("Behavior observable", ST.OBS)),
    (SL.PSYCLERK_MSE_SPEECH, ("Speech observable", ST.OBS)),
    (SL.PSYCLERK_MSE_MOOD, ("Mood, function", ST.OBS)),
    (SL.PSYCLERK_MSE_AFFECT, ("Affect, function", ST.OBS)),
    (SL.PSYCLERK_MSE_THOUGHT, ("Thinking, function", ST.OBS)),
    (SL.PSYCLERK_MSE_PERCEPTION, ("Perception, function", ST.OBS)),
    (SL.PSYCLERK_MSE_COGNITION, ("Cognitive functions", ST.OBS)),
    (SL.PSYCLERK_MSE_INSIGHT, ("Insight, function", ST.OBS)),

    (SL.PSYCLERK_PHYSEXAM_GENERAL, (f"General {EXAM} {ST.FIND}", ST.OBS)),
    (SL.PSYCLERK_PHYSEXAM_CARDIOVASCULAR, (f"Cardiovascular {EXAM} {ST.FIND}", ST.OBS)),  # noqa
    (SL.PSYCLERK_PHYSEXAM_RESPIRATORY, (f"Respiratory {EXAM} {ST.FIND}", ST.OBS)),  # noqa
    (SL.PSYCLERK_PHYSEXAM_ABDOMINAL, (f"Abdominal {EXAM} {ST.FIND}", ST.OBS)),
    (SL.PSYCLERK_PHYSEXAM_NEUROLOGICAL, (f"Neurology {EXAM} {ST.FIND}", ST.OBS)),  # noqa

    (SL.PSYCLERK_ASSESSMENT_SCALES, ("Assessment scales", ST.REC)),
    (SL.PSYCLERK_INVESTIGATIONS_RESULTS, ("Investigation results", ST.REC)),

    (SL.PSYCLERK_SAFETY_ALERTS, ("Safety alerts", ST.REC)),
    (SL.PSYCLERK_RISK_ASSESSMENT, ("Clinical risk assessment", ST.REC)),
    (SL.PSYCLERK_RELEVANT_LEGAL_INFORMATION, ("Legal information", ST.REC)),

    (SL.PSYCLERK_CURRENT_PROBLEMS, ("Problems and issues", ST.REC)),
    (SL.PSYCLERK_PATIENT_CARER_CONCERNS, ("Patient and carer concerns", ST.REC)),  # noqa
    (SL.PSYCLERK_CLINICAL_NARRATIVE, ("Clinical narrative", ST.REC)),
    (SL.PSYCLERK_MANAGEMENT_PLAN, ("Clinical management plan", ST.REC)),
    (SL.PSYCLERK_INFORMATION_GIVEN, ("Information given", ST.REC)),

    (SL.CORE10_SCALE, (f"{CORE10_1}", ST.SCALE)),
    (SL.CORE10_SCORE, (f"{CORE10_2} clinical {SCORE}", ST.OBS)),
    (SL.CORE10_PROCEDURE_ASSESSMENT, (f"{ASSESS} {CORE10_2}", ST.PROC)),

    (SL.DAST_SCALE, ("Drug abuse screening test", ST.SCALE)),

    (SL.EQ5D5L_SCALE, (f"{EQ5D5L} {SCALE}", ST.SCALE)),
    (SL.EQ5D5L_INDEX_VALUE, (f"{EQ5D5L} index value", ST.OBS)),
    (SL.EQ5D5L_PAIN_DISCOMFORT_SCORE, (f"{EQ5D5L} pain discomfort {SCORE}", ST.OBS)),  # noqa
    (SL.EQ5D5L_USUAL_ACTIVITIES_SCORE, (f"{EQ5D5L} usual activities {SCORE}", ST.OBS)),  # noqa
    (SL.EQ5D5L_ANXIETY_DEPRESSION_SCORE, (f"{EQ5D5L} anxiety depression {SCORE}", ST.OBS)),  # noqa
    (SL.EQ5D5L_MOBILITY_SCORE, (f"{EQ5D5L} mobility {SCORE}", ST.OBS)),
    (SL.EQ5D5L_SELF_CARE_SCORE, (f"{EQ5D5L} self-care {SCORE}", ST.OBS)),
    (SL.EQ5D5L_PROCEDURE_ASSESSMENT, (f"{ASSESS} {EQ5D5L} health outcome measure", ST.PROC)),  # noqa

    (SL.FAST_SCALE, (f"{FAST}", ST.SCALE)),
    (SL.FAST_SCORE, (f"{FAST} {SCORE}", ST.OBS)),
    (SL.FAST_PROCEDURE_ASSESSMENT, (f"{ASSESS} {FAST}", ST.PROC)),

    (SL.GAD7_SCALE, (f"{GAD7} {SCALE}", ST.SCALE)),
    (SL.GAD7_SCORE, (f"{GAD7} {SCORE}", ST.OBS)),
    (SL.GAD7_PROCEDURE_ASSESSMENT, (f"{ASSESS} {GAD7} {SCORE}", ST.PROC)),

    (SL.GAF_SCALE, (f"Global assessment of functioning - 1993 Diagnostic and Statistical Manual of Mental Disorders, Fourth Edition adaptation", ST.SCALE)),  # noqa

    (SL.GDS15_SCALE, (f"{GDS15}", ST.SCALE)),
    (SL.GDS15_SCORE, (f"{GDS15} {SCORE}", ST.OBS)),
    (SL.GDS15_PROCEDURE_ASSESSMENT, (f"{ASSESS} {GDS15}", ST.PROC)),

    (SL.HADS_SCALE, (f"{HADS}", ST.SCALE)),
    (SL.HADS_ANXIETY_SCORE, (f"{HADS}: anxiety {SCORE}", ST.OBS)),
    (SL.HADS_DEPRESSION_SCORE, (f"{HADS}: depression {SCORE}", ST.OBS)),
    (SL.HADS_PROCEDURE_ASSESSMENT, (f"{ASSESS} {HADS}", ST.PROC)),

    (SL.HAMD_SCALE, (f"{HDRS}", ST.SCALE)),
    (SL.HAMD_SCORE, (f"{HDRS} {SCORE}", ST.OBS)),
    (SL.HAMD_PROCEDURE_ASSESSMENT, (f"{ASSESS} {HDRS}", ST.PROC)),

    (SL.HONOSCA_SCALE, (f"{HONOSCA_1}", ST.SCALE)),
    (SL.HONOSCA_SECTION_A_SCALE, (f"{HONOSCA_2} section A", ST.SCALE)),
    (SL.HONOSCA_SECTION_B_SCALE, (f"{HONOSCA_2} section B", ST.SCALE)),
    (SL.HONOSCA_SCORE, (f"{HONOSCA_1} {SCORE}", ST.OBS)),
    (SL.HONOSCA_SECTION_A_SCORE, (f"{HONOSCA_1} section A {SCORE}", ST.OBS)),
    (SL.HONOSCA_SECTION_B_SCORE, (f"{HONOSCA_1} section B {SCORE}", ST.OBS)),
    (SL.HONOSCA_SECTION_A_PLUS_B_SCORE, (f"{HONOSCA_2} section A and B total {SCORE}", ST.OBS)),  # noqa
    (SL.HONOSCA_PROCEDURE_ASSESSMENT, (f"{ASSESS} {HONOSCA_2}", ST.PROC)),
    #
    (SL.HONOS65_SCALE, (f"{HONOS65_1}", ST.SCALE)),
    (SL.HONOS65_SCORE, (f"{HONOS65_1} {SCORE}", ST.OBS)),
    (SL.HONOS65_PROCEDURE_ASSESSMENT, (f"{ASSESS} {HONOS65_2} {RATING_SCALE}", ST.PROC)),  # noqa
    #
    (SL.HONOSWA_SCALE, (f"{HONOSWA_1}", ST.SCALE)),
    (SL.HONOSWA_SUBSCALE_1_OVERACTIVE, (f"{HONOSWA_2} {RATING_SCALE} 1 - {HONOSWA_S1}", ST.SCALE)),  # noqa
    (SL.HONOSWA_SUBSCALE_2_SELFINJURY, (f"{HONOSWA_2} {RATING_SCALE} 2 - {HONOSWA_S2}", ST.SCALE)),  # noqa
    (SL.HONOSWA_SUBSCALE_3_SUBSTANCE, (f"{HONOSWA_2} {RATING_SCALE} 3 - {HONOSWA_S3}", ST.SCALE)),  # noqa
    (SL.HONOSWA_SUBSCALE_4_COGNITIVE, (f"{HONOSWA_2} {RATING_SCALE} 4 - {HONOSWA_S4}", ST.SCALE)),  # noqa
    (SL.HONOSWA_SUBSCALE_5_PHYSICAL, (f"{HONOSWA_2} {RATING_SCALE} 5 - {HONOSWA_S5}", ST.SCALE)),  # noqa
    (SL.HONOSWA_SUBSCALE_6_PSYCHOSIS, (f"{HONOSWA_2} {RATING_SCALE} 6 - {HONOSWA_S6}", ST.SCALE)),  # noqa
    (SL.HONOSWA_SUBSCALE_7_DEPRESSION, (f"{HONOSWA_2} {RATING_SCALE} 7 - {HONOSWA_S7}", ST.SCALE)),  # noqa
    (SL.HONOSWA_SUBSCALE_8_OTHERMENTAL, (f"{HONOSWA_2} {RATING_SCALE} 8 - {HONOSWA_S8}", ST.SCALE)),  # noqa
    (SL.HONOSWA_SUBSCALE_9_RELATIONSHIPS, (f"{HONOSWA_2} {RATING_SCALE} 9 - {HONOSWA_S9}", ST.SCALE)),  # noqa
    (SL.HONOSWA_SUBSCALE_10_ADL, (f"{HONOSWA_2} {RATING_SCALE} 10 - {HONOSWA_S10}", ST.SCALE)),  # noqa
    (SL.HONOSWA_SUBSCALE_11_LIVINGCONDITIONS, (f"{HONOSWA_2} {RATING_SCALE} 11 - {HONOSWA_S11}", ST.SCALE)),  # noqa
    (SL.HONOSWA_SUBSCALE_12_OCCUPATION, (f"{HONOSWA_2} {RATING_SCALE} 12 - {HONOSWA_S12}", ST.SCALE)),  # noqa
    (SL.HONOSWA_SCORE, (f"{HONOSWA_1} {SCORE}", ST.OBS)),
    (SL.HONOSWA_1_OVERACTIVE_SCORE, (f"{HONOSWA_2} {RATING_SCALE} 1 {SCORE} - {HONOSWA_S1}", ST.OBS)),  # noqa
    (SL.HONOSWA_2_SELFINJURY_SCORE, (f"{HONOSWA_2} {RATING_SCALE} 2 {SCORE} - {HONOSWA_S2}", ST.OBS)),  # noqa
    (SL.HONOSWA_3_SUBSTANCE_SCORE, (f"{HONOSWA_2} {RATING_SCALE} 3 {SCORE} - {HONOSWA_S3}", ST.OBS)),  # noqa
    (SL.HONOSWA_4_COGNITIVE_SCORE, (f"{HONOSWA_2} {RATING_SCALE} 4 {SCORE} - {HONOSWA_S4}", ST.OBS)),  # noqa
    (SL.HONOSWA_5_PHYSICAL_SCORE, (f"{HONOSWA_2} {RATING_SCALE} 5 {SCORE} - {HONOSWA_S5}", ST.OBS)),  # noqa
    (SL.HONOSWA_6_PSYCHOSIS_SCORE, (f"{HONOSWA_2} {RATING_SCALE} 6 {SCORE} - {HONOSWA_S6}", ST.OBS)),  # noqa
    (SL.HONOSWA_7_DEPRESSION_SCORE, (f"{HONOSWA_2} {RATING_SCALE} 7 {SCORE} - {HONOSWA_S7}", ST.OBS)),  # noqa
    (SL.HONOSWA_8_OTHERMENTAL_SCORE, (f"{HONOSWA_2} {RATING_SCALE} 8 {SCORE} - {HONOSWA_S8}", ST.OBS)),  # noqa
    (SL.HONOSWA_9_RELATIONSHIPS_SCORE, (f"{HONOSWA_2} {RATING_SCALE} 9 {SCORE} - {HONOSWA_S9}", ST.OBS)),  # noqa
    (SL.HONOSWA_10_ADL_SCORE, (f"{HONOSWA_2} {RATING_SCALE} 10 {SCORE} - {HONOSWA_S10}", ST.OBS)),  # noqa
    (SL.HONOSWA_11_LIVINGCONDITIONS_SCORE, (f"{HONOSWA_2} {RATING_SCALE} 11 {SCORE} - {HONOSWA_S11}", ST.OBS)),  # noqa
    (SL.HONOSWA_12_OCCUPATION_SCORE, (f"{HONOSWA_2} {RATING_SCALE} 12 {SCORE} - {HONOSWA_S12}", ST.OBS)),  # noqa
    (SL.HONOSWA_PROCEDURE_ASSESSMENT, (f"{ASSESS} {HONOSWA_2}", ST.PROC)),

    (SL.IESR_SCALE, (f"{IESR}", ST.SCALE)),
    (SL.IESR_SCORE, (f"{IESR} {SCORE}", ST.OBS)),
    (SL.IESR_PROCEDURE_ASSESSMENT, (f"{ASSESS} {IESR}", ST.PROC)),

    (SL.MAST_SCALE, (f"{MAST}", ST.SCALE)),
    (SL.MAST_SCORE, (f"{MAST} {TOTSCORE}", ST.OBS)),
    (SL.MAST_PROCEDURE_ASSESSMENT, (f"{ASSESS} {MAST}", ST.PROC)),

    (SL.SMAST_SCALE, (f"Short {MAST}", ST.SCALE)),

    (SL.UPDRS_SCALE, ("Unified Parkinsons disease rating scale score", ST.SCALE)),  # noqa

    (SL.MOCA_SCALE, (f"{MOCA}", ST.SCALE)),
    (SL.MOCA_SCORE, (f"{MOCA} {SCORE}", ST.OBS)),
    (SL.MOCA_PROCEDURE_ASSESSMENT, (f"{ASSESS} {MOCA}", ST.PROC)),

    (SL.NART_SCALE, (f"{NART}", ST.SCALE)),
    (SL.NART_SCORE, (f"{NART} {SCORE}", ST.OBS)),
    (SL.NART_PROCEDURE_ASSESSMENT, (f"{ASSESS} {NART}", ST.PROC)),

    (SL.PANSS_SCALE, ("Positive and negative syndrome scale", ST.SCALE)),

    (SL.PDSS_SCALE, (f"{PDSS}", ST.SCALE)),
    (SL.PDSS_SCORE, (f"{PDSS} {SCORE}", ST.OBS)),

    (SL.PHQ9_FINDING_NEGATIVE_SCREENING_FOR_DEPRESSION, (f"{NEG_SCREEN} depression {ON} {PHQ9}", ST.FIND)),  # noqa
    (SL.PHQ9_FINDING_POSITIVE_SCREENING_FOR_DEPRESSION, (f"{POS_SCREEN} depression {ON} {PHQ9}", ST.FIND)),  # noqa
    (SL.PHQ9_SCORE, (f"{PHQ9W} {SCORE}", ST.OBS)),
    (SL.PHQ9_PROCEDURE_DEPRESSION_SCREENING, (f"Depression {SCREEN} {PHQ9W} {SCORE}", ST.PROC)),  # noqa
    (SL.PHQ9_SCALE, (f"{PHQ9}", ST.SCALE)),

    (SL.PHQ15_SCORE, (f"{PHQ15} {SCORE}", ST.OBS)),
    (SL.PHQ15_PROCEDURE, (f"{ASSESS} {PHQ15}", ST.PROC)),
    (SL.PHQ15_SCALE, (f"{PHQ15}", ST.SCALE)),

    (SL.PSWQ_SCALE, (f"{PSWQ}", ST.SCALE)),
    (SL.PSWQ_SCORE, (f"{PSWQ} {SCORE}", ST.OBS)),
    (SL.PSWQ_PROCEDURE_ASSESSMENT, (f"{ASSESS} {PSWQ}", ST.PROC)),

    (SL.QOL_SCALE, ("Quality of life scale", ST.SCALE)),

    (SL.WEMWBS_SCALE, (f"{WEMWBS1}", ST.SCALE)),
    (SL.WEMWBS_SCORE, (f"{WEMWBS1} {SCORE}", ST.OBS)),
    (SL.WEMWBS_PROCEDURE_ASSESSMENT, (f"{ASSESS} {WEMWBS2}", ST.PROC)),
    #
    (SL.SWEMWBS_SCALE, (f"{SWEMWBS2}", ST.SCALE)),
    (SL.SWEMWBS_SCORE, (f"{SWEMWBS1} {SCORE}", ST.OBS)),
    (SL.SWEMWBS_PROCEDURE_ASSESSMENT, (f"{ASSESS} {SWEMWBS2}", ST.PROC)),

    (SL.WSAS_SCALE, (f"{WSAS}", ST.SCALE)),
    (SL.WSAS_SCORE, (f"{WSAS} {SCORE}", ST.OBS)),
    (SL.WSAS_WORK_SCORE, (f"{WSAS} - work {SCORE}", ST.OBS)),
    (SL.WSAS_RELATIONSHIPS_SCORE, (f"{WSAS} - relationships {SCORE}", ST.OBS)),
    (SL.WSAS_HOME_MANAGEMENT_SCORE, (f"{WSAS} - home management {SCORE}", ST.OBS)),  # noqa
    (SL.WSAS_SOCIAL_LEISURE_SCORE, (f"{WSAS} - social leisure activities {SCORE}", ST.OBS)),  # noqa
    (SL.WSAS_PRIVATE_LEISURE_SCORE, (f"{WSAS} - private leisure activities {SCORE}", ST.OBS)),  # noqa
    (SL.WSAS_PROCEDURE_ASSESSMENT, (f"{ASSESS} {WSAS}", ST.PROC)),

])  # type: Dict[str, Tuple]


# =============================================================================
# Exceptions
# =============================================================================

class ConceptNotFound(Exception):
    pass


# =============================================================================
# Fetch SNOMED CT codes from an API provider.
# =============================================================================

class SnomedApiInfo(object):
    """
    Represents information about a SNOMED API.
    """
    def __init__(self,
                 base_url: str = DEFAULT_URL,
                 edition: str = DEFAULT_EDITION,
                 release: str = DEFAULT_RELEASE,
                 language: str = DEFAULT_LANGUAGE,
                 rate_limit_hz: float = DEFAULT_RATE_LIMIT_HZ,
                 json_indent: int = 4,
                 json_sort_keys: bool = True) -> None:
        """
        Args:
            base_url: REST API server base URL
            edition: SNOMED CT edition
            release: SNOMED CT release (version)
            language: language to use
            rate_limit_hz: maximum request rate
            json_indent: cosmetic: indent for JSON formatting
            json_sort_keys: cosmetic: sort keys for JSON formatting?
        """
        self.base_url = base_url
        self.edition = edition
        self.release = release
        self.language = language
        self.rate_limit_hz = rate_limit_hz
        self.json_indent = json_indent
        self.json_sort_keys = json_sort_keys

        self.main_url = f"{base_url}/{edition}/{release}"
        self.get_result = rate_limited(rate_limit_hz)(self._get_result)

    def format_json(self, json_object: JsonValueType) -> str:
        """
        Pretty-formats a JSON object and returns it as a string.
        """
        return json.dumps(json_object, indent=self.json_indent,
                          sort_keys=self.json_sort_keys)

    def _get_result(self, url: str,
                    params: Dict[str, str] = None) -> Optional[JsonValueType]:
        """
        Asks the REST API server for details and returns them as a JSON object.

        Args:
            url: URL
            params: query parameters

        Returns:
            a JSON object (e.g. literal, list, dictionary)

        Note that the final function used is a rate-limited version of this.
        """
        log.debug(f"Fetching from {url}")
        response = requests.get(url, params)
        if response.status_code != HTTP_200_OK:
            log.warning(f"Response was: {response}")
            return None
        json_object = response.json()
        log.debug(f"Response was:\n{self.format_json(json_object)}")
        return response.json()

    def get_concept_by_id(self, sctid: Union[int, str]) -> SnomedConcept:
        """
        Finds a concept by identifer a SNOMED API provider. You must be
        entitled to use information from that provider.

        Args:
            sctid: SNOMED CT identifier (in string format)

        Returns:
            a :class:`SnomedConcept`

        Raises:
            :exc:`ConceptNotFound` if not found

        """
        url = f"{self.main_url}/concepts/{sctid}"
        json_object = self.get_result(url)
        if not isinstance(json_object, dict):
            raise ConceptNotFound(f"No result for {sctid!r}.")
        identifier = int(json_object["conceptId"])
        term = json_object["defaultTerm"]
        concept = SnomedConcept(identifier=identifier, term=term)
        log.info(concept)
        return concept

    def get_concept_by_term(self,
                            term: str,
                            semantic_area: str = None,
                            suffix: str = "",
                            limit: int = 1000) -> SnomedConcept:
        """
        Finds a concept by name from a SNOMED API provider. You must be
        entitled to use information from that provider.

        Args:
            term: SNOMED CT term name
            semantic_area: SNOMED CT semantic area
            suffix: special suffix: search without it, match with it
            limit: maximum number of hits per search (if you don't specify,
                you'll get a default of 100, at least in the NHS)

        Returns:
            a :class:`SnomedConcept`

        Raises:
            :exc:`ConceptNotFound` if not found
        """
        url = f"{self.main_url}/descriptions"
        fsn = f"{term}{suffix} ({semantic_area})"
        params = {
            "query": term,
            # Searching for an FSN does not work.

            "mode": "fullText",
            # With mode="fullText", you get e.g. "Lupus pneumonia" as a hit for
            # "pneumonia". With mode="regex", you don't, but proper regex
            # characters such as ^ don't seem to work; no hits come back.
            # I'm not sure the regex mode works.

            "language": self.language,
            # ... required for "fullText" mode, according to the docs
            # (I'm not so sure it is!)

            "statusFilter": "activeOnly",

            "returnLimit": str(limit),
            # "returnLimit": "10",  # "more than one" is a problem for us
            # ... but can't get the search to restrict properly, so we need
            # to fetch all and check.
        }
        if semantic_area:
            params["semanticFilter"] = semantic_area
        log.debug(f"Query params: {params}")
        json_object = self.get_result(url, params)
        if not isinstance(json_object, dict):
            raise ConceptNotFound(f"No result for {term!r}.")
        matches = json_object["matches"]
        if not isinstance(matches, list):
            raise ConceptNotFound(f"Bad 'matches' object for {term!r}.")
        if len(matches) == 0:
            raise ConceptNotFound(f"No matches for {term!r}.")
        log.debug(f"Found {len(matches)} hits for {term!r}")
        for match in matches:
            assert isinstance(match, dict)
            # found_term = match["term"]
            found_identifier = int(match["conceptId"])
            found_fsn = match["fsn"]
            if found_fsn.lower() == fsn.lower():  # case-insensitive
                concept = SnomedConcept(identifier=found_identifier,
                                        term=found_fsn)
                log.info(concept)
                return concept
        raise ConceptNotFound(f"Multiple hits but no proper match for {fsn}")


# =============================================================================
# Tests
# =============================================================================

def test_api(api: SnomedApiInfo,
             concept_ids: List[int] = None,
             terms: List[str] = None,
             semantic_area: str = None) -> None:
    """
    Examples as per
    https://confluence.ihtsdotools.org/display/DOCSTART/4.+SNOMED+CT+Basics

    Args:
        api: a :class:`SnomedApiInfo`.
        concept_ids: optional list of concept IDs to test
        terms: optional list of terms to test
        semantic_area: optional single semantic area to restrict to
    """
    concept_ids = concept_ids or []  # type: List[int]
    terms = terms or []  # type: List[str]

    log.warning("Running tests...")

    if not concept_ids and not terms:
        log.warning("Using default tests")

        log.warning("'Pneumonia (disorder)' by ID")
        api.get_concept_by_id("233604007")

        log.warning("'Lupus pneumonia (disorder)' by name")
        api.get_concept_by_term("Lupus pneumonia", SemanticTypes.DIS)

        log.warning("'Pneumonia (disorder)' by name")
        api.get_concept_by_term("Pneumonia", SemanticTypes.DIS)

    for cid in concept_ids:
        log.warning(str(cid))
        api.get_concept_by_id(cid)

    for term in terms:
        if semantic_area:
            log.warning(f"{term} ({semantic_area})")
        else:
            log.warning(f"{term}")
        api.get_concept_by_term(term, semantic_area)


# =============================================================================
# Fetching codes for CamCOPS
# =============================================================================

def disclaim(question: str, required_answer: str) -> None:
    """
    Requires the user to agree to a statement. If they don't, the program
    exits.

    Args:
        question: question to be printed
        required_answer: answer that is required
    """
    print(question)
    answer = input(f"-- Enter {required_answer!r} to continue: ")
    if answer != required_answer:
        print("OK; exiting.")
        sys.exit(1)


def get_xml(camcops_name: str, concept: SnomedConcept) -> str:
    """
    Returns XML representing this concept.

    Args:
        camcops_name: CamCOPS name for the concept
        concept: SNOMED CT concept

    Returns:
        str: the XML
    """
    assert '"' not in camcops_name
    return (
        f'<lookup name="{camcops_name}">'
        f'<concept>'
        f'<id>{concept.identifier}</id>'
        f'<term>{concept.term}</term>'
        f'</concept>'
        f'</lookup>'
    )


def fetch_camcops_snomed_codes(api: SnomedApiInfo,
                               filename: str,
                               continue_on_error: bool = False) -> None:
    """
    Prints XML to stdout.

    Args:
        api: a :class:`SnomedApiInfo`.
        filename: name of XML file to write
        continue_on_error: carry on through failed lookups?
    """
    possible_types = [getattr(SemanticTypes, x)
                      for x in dir(SemanticTypes) if not x.startswith("_")]
    possible_names = [getattr(SnomedLookup, x)
                      for x in dir(SnomedLookup) if not x.startswith("_")]
    seen = set()  # type: Set[str]
    with open(filename, "wt") as f:
        print(XML_HEADER, file=f)
        for camcops_name, name_type in CCMAP.items():
            sname = name_type[0]
            stype = name_type[1]
            suffix = name_type[2] if len(name_type) >= 3 else ""
            assert camcops_name in possible_names, f"Bad name: {camcops_name}"
            assert stype in possible_types, f"Bad type: {stype}"
            try:
                concept = api.get_concept_by_term(sname, stype, suffix=suffix)
                xml = get_xml(camcops_name, concept)
                print(f"    {xml}", file=f)
                seen.add(camcops_name)
            except ConceptNotFound:
                if continue_on_error:
                    log.error(f"Could not find: {sname!r}, {stype!r}")
                else:
                    raise
        print(XML_FOOTER, file=f)
    missing = set(possible_names) - seen
    if missing:
        log.warning(f"Missing: {missing}")


# =============================================================================
# Command-line entry point
# =============================================================================

def main() -> None:
    # noinspection PyTypeChecker
    parser = argparse.ArgumentParser(
        description=(
            "Fetch SNOMED CT codes from a SNOMED CT Snapshot REST API provider"
        ),
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        '--allhelp',
        action=ShowAllSubparserHelpAction,
        help='show help for all commands and exit')
    parser.add_argument(
        "--url", type=str, default=DEFAULT_URL,
        help="URL (protocol, host, API root path) for SNOMED-CT API"
    )
    parser.add_argument(
        "--edition", type=str, default=DEFAULT_EDITION,
        help="SNOMED CT edition"
    )
    parser.add_argument(
        "--release", type=str, default=DEFAULT_RELEASE,
        help="Snomed release identifier (version)"
    )
    parser.add_argument(
        "--language", type=str, default=DEFAULT_LANGUAGE,
        help="Language"
    )
    parser.add_argument(
        "--rate_limit_hz", type=float, default=DEFAULT_RATE_LIMIT_HZ,
        help="Maximum number of requests per second",
    )
    parser.add_argument(
        "--verbose", action="store_true",
        help="Be verbose"
    )

    subparsers = parser.add_subparsers(
        title="commands",
        help=(
            f"Specify one command. "
            f"Use '{parser.prog} <command> --help' for more help"
        ),
        dest="command",  # sorts out the help for the command being mandatory
    )
    subparsers.required = True  # requires a command

    test_parser = subparsers.add_parser(
        "test",
        help=(
            "Run tests. "
            "If you specify no arguments, default tests will be run."
        ),
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    test_parser.add_argument(
        "--concept_id", nargs="*", type=int,
        help="For tests: Concept ID(s) to test"
    )
    test_parser.add_argument(
        "--term", nargs="*", type=str,
        help="For tests: Term(s) to test"
    )
    test_parser.add_argument(
        "--semantic_area", type=str,
        help="For tests: semantic area to restrict to"
    )

    fetch_parser = subparsers.add_parser(
        "fetch",
        help="Fetch data relevant to CamCOPS and write to XML",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    fetch_parser.add_argument(
        "--outfile", type=str, default=DEFAULT_OUTPUT_XML_FILENAME,
        help="XML filename to write"
    )
    fetch_parser.add_argument(
        "--continue_on_error", action="store_true",
        help="Carry on despite errors"
    )

    args = parser.parse_args()
    main_only_quicksetup_rootlogger(level=logging.DEBUG if args.verbose
                                    else logging.INFO)

    api = SnomedApiInfo(base_url=args.url,
                        edition=args.edition,
                        release=args.release,
                        language=args.language,
                        rate_limit_hz=args.rate_limit_hz)

    if args.command == "test":
        test_api(api, concept_ids=args.concept_id, terms=args.term,
                 semantic_area=args.semantic_area)
    elif args.command == "fetch":
        disclaim(DISCLAIMER_1, ANSWER_1)
        disclaim(DISCLAIMER_2, ANSWER_2)
        fetch_camcops_snomed_codes(api, filename=args.outfile,
                                   continue_on_error=args.continue_on_error)


if __name__ == "__main__":
    main()
