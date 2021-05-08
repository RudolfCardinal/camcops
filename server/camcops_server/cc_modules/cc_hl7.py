#!/usr/bin/env python

# noinspection HttpUrlsUsage
"""
camcops_server/cc_modules/cc_hl7.py

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

**Core HL7 functions, e.g. to build HL7 messages.**

General HL7 sources:

- https://python-hl7.readthedocs.org/en/latest/
- http://www.interfaceware.com/manual/v3gen_python_library_details.html
- http://www.interfaceware.com/hl7_video_vault.html#how
- http://www.interfaceware.com/hl7-standard/hl7-segments.html
- https://www.hl7.org/special/committees/vocab/v26_appendix_a.pdf
- https://www.ncbi.nlm.nih.gov/pmc/articles/PMC130066/

To consider

- batched messages (HL7 batching protocol);
  https://docs.oracle.com/cd/E23943_01/user.1111/e23486/app_hl7batching.htm
- note: DG1 segment = diagnosis

Basic HL7 message structure:

- can package into HL7 2.X message as encapsulated PDF;
  https://www.hl7standards.com/blog/2007/11/27/pdf-attachment-in-hl7-message/
- message ORU^R01
  https://www.corepointhealth.com/resource-center/hl7-resources/hl7-messages
- MESSAGES: http://www.interfaceware.com/hl7-standard/hl7-messages.html
- OBX segment = observation/result segment;
  https://www.corepointhealth.com/resource-center/hl7-resources/hl7-obx-segment;
  http://www.interfaceware.com/hl7-standard/hl7-segment-OBX.html
- SEGMENTS:
  https://www.corepointhealth.com/resource-center/hl7-resources/hl7-segments
- ED field (= encapsulated data);
  http://www.interfaceware.com/hl7-standard/hl7-fields.html
- base-64 encoding

We can then add an option for structure (XML), HTML, PDF export.

"""

import base64
import logging
import socket
from typing import List, Optional, Tuple, TYPE_CHECKING, Union

from cardinal_pythonlib.datetimefunc import format_datetime
from cardinal_pythonlib.logs import BraceStyleAdapter
import hl7
from pendulum import Date, DateTime as Pendulum

from camcops_server.cc_modules.cc_constants import DateFormat, FileType
from camcops_server.cc_modules.cc_simpleobjects import HL7PatientIdentifier

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_request import CamcopsRequest
    from camcops_server.cc_modules.cc_simpleobjects import TaskExportOptions
    from camcops_server.cc_modules.cc_task import Task

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Constants
# =============================================================================

# STRUCTURE OF HL7 MESSAGES
# MESSAGE = list of segments, separated by carriage returns
SEGMENT_SEPARATOR = "\r"
# SEGMENT = list of fields (= composites), separated by pipes
FIELD_SEPARATOR = "|"
# FIELD (= COMPOSITE) = string, or list of components separated by carets
COMPONENT_SEPARATOR = "^"
# Component = string, or lists of subcomponents separated by ampersands
SUBCOMPONENT_SEPARATOR = "&"
# Subcomponents must be primitive data types (i.e. strings).
# ... http://www.interfaceware.com/blog/hl7-composites/

REPETITION_SEPARATOR = "~"
ESCAPE_CHARACTER = "\\"

# Fields are specified in terms of DATA TYPES:
# http://www.corepointhealth.com/resource-center/hl7-resources/hl7-data-types

# Some of those are COMPOSITE TYPES:
# http://amisha.pragmaticdata.com/~gunther/oldhtml/composites.html#COMPOSITES


# =============================================================================
# HL7 helper functions
# =============================================================================

def get_mod11_checkdigit(strnum: str) -> str:
    # noinspection HttpUrlsUsage
    """
    Input: string containing integer. Output: MOD11 check digit (string).

    See:

    - http://www.mexi.be/documents/hl7/ch200025.htm
    - https://stackoverflow.com/questions/7006109
    - http://www.pgrocer.net/Cis51/mod11.html
    """
    total = 0
    multiplier = 2  # 2 for units digit, increases to 7, then resets to 2
    try:
        for i in reversed(range(len(strnum))):
            total += int(strnum[i]) * multiplier
            multiplier += 1
            if multiplier == 8:
                multiplier = 2
        c = str(11 - (total % 11))
        if c == "11":
            c = "0"
        elif c == "10":
            c = "X"
        return c
    except (TypeError, ValueError):
        # garbage in...
        return ""


def make_msh_segment(message_datetime: Pendulum,
                     message_control_id: str) -> hl7.Segment:
    """
    Creates an HL7 message header (MSH) segment.

    - MSH: https://www.hl7.org/documentcenter/public/wg/conf/HL7MSH.htm

    - We're making an ORU^R01 message = unsolicited result.

      - ORU = Observational Report - Unsolicited
      - ORU^R01 = Unsolicited transmission of an observation message
      - https://www.corepointhealth.com/resource-center/hl7-resources/hl7-oru-message
      - https://www.hl7kit.com/joomla/index.php/hl7resources/examples/107-orur01
    """  # noqa

    segment_id = "MSH"
    encoding_characters = (COMPONENT_SEPARATOR + REPETITION_SEPARATOR +
                           ESCAPE_CHARACTER + SUBCOMPONENT_SEPARATOR)
    sending_application = "CamCOPS"
    sending_facility = ""
    receiving_application = ""
    receiving_facility = ""
    date_time_of_message = format_datetime(message_datetime,
                                           DateFormat.HL7_DATETIME)
    security = ""
    message_type = hl7.Field(COMPONENT_SEPARATOR, [
        "ORU",  # message type ID = Observ result/unsolicited
        "R01"   # trigger event ID = ORU/ACK - Unsolicited transmission
                # of an observation message
    ])
    processing_id = "P"  # production (processing mode: current)
    version_id = "2.3"  # HL7 version
    sequence_number = ""
    continuation_pointer = ""
    accept_acknowledgement_type = ""
    application_acknowledgement_type = "AL"  # always
    country_code = ""
    character_set = "UNICODE UTF-8"
    # http://wiki.hl7.org/index.php?title=Character_Set_used_in_v2_messages
    principal_language_of_message = ""

    fields = [
        segment_id,
        # field separator inserted automatically; HL7 standard considers it a
        # field but the python-hl7 processor doesn't when it parses
        encoding_characters,
        sending_application,
        sending_facility,
        receiving_application,
        receiving_facility,
        date_time_of_message,
        security,
        message_type,
        message_control_id,
        processing_id,
        version_id,
        sequence_number,
        continuation_pointer,
        accept_acknowledgement_type,
        application_acknowledgement_type,
        country_code,
        character_set,
        principal_language_of_message,
    ]
    segment = hl7.Segment(FIELD_SEPARATOR, fields)
    return segment


def make_pid_segment(
        forename: str,
        surname: str,
        dob: Date,
        sex: str,
        address: str,
        patient_id_list: List[HL7PatientIdentifier] = None) -> hl7.Segment:
    """
    Creates an HL7 patient identification (PID) segment.

    - https://www.corepointhealth.com/resource-center/hl7-resources/hl7-pid-segment
    - https://www.hl7.org/documentcenter/public/wg/conf/Msgadt.pdf (s5.4.8)

    - ID numbers...
      https://www.cdc.gov/vaccines/programs/iis/technical-guidance/downloads/hl7guide-1-4-2012-08.pdf
    """  # noqa

    patient_id_list = patient_id_list or []  # type: List[HL7PatientIdentifier]

    segment_id = "PID"
    set_id = ""

    # External ID
    patient_external_id = ""
    # ... this one is deprecated
    # http://www.j4jayant.com/articles/hl7/16-patient-id

    # Internal ID
    internal_id_element_list = []
    for i in range(len(patient_id_list)):
        if not patient_id_list[i].pid:
            continue
        ptidentifier = patient_id_list[i]
        pid = ptidentifier.pid
        check_digit = get_mod11_checkdigit(pid)
        check_digit_scheme = "M11"  # Mod 11 algorithm
        type_id = patient_id_list[i].id_type
        assigning_authority = patient_id_list[i].assigning_authority
        # Now, as per Table 4.6 "Extended composite ID" of
        # hl7guide-1-4-2012-08.pdf:
        internal_id_element = hl7.Field(COMPONENT_SEPARATOR, [
            pid,
            check_digit,
            check_digit_scheme,
            assigning_authority,
            type_id  # length "2..5" meaning 2-5
        ])
        internal_id_element_list.append(internal_id_element)
    patient_internal_id = hl7.Field(REPETITION_SEPARATOR,
                                    internal_id_element_list)

    # Alternate ID
    alternate_patient_id = ""
    # ... this one is deprecated
    # http://www.j4jayant.com/articles/hl7/16-patient-id

    patient_name = hl7.Field(COMPONENT_SEPARATOR, [
        forename,  # surname
        surname,  # forename
        "",  # middle initial/name
        "",  # suffix (e.g. Jr, III)
        "",  # prefix (e.g. Dr)
        "",  # degree (e.g. MD)
    ])
    mothers_maiden_name = ""
    date_of_birth = format_datetime(dob, DateFormat.HL7_DATE)
    alias = ""
    race = ""
    country_code = ""
    home_phone_number = ""
    business_phone_number = ""
    language = ""
    marital_status = ""
    religion = ""
    account_number = ""
    social_security_number = ""
    drivers_license_number = ""
    mother_identifier = ""
    ethnic_group = ""
    birthplace = ""
    birth_order = ""
    citizenship = ""
    veterans_military_status = ""

    fields = [
        segment_id,
        set_id,  # PID.1
        patient_external_id,  # PID.2
        patient_internal_id,  # known as "PID-3" or "PID.3"
        alternate_patient_id,  # PID.4
        patient_name,
        mothers_maiden_name,
        date_of_birth,
        sex,
        alias,
        race,
        address,
        country_code,
        home_phone_number,
        business_phone_number,
        language,
        marital_status,
        religion,
        account_number,
        social_security_number,
        drivers_license_number,
        mother_identifier,
        ethnic_group,
        birthplace,
        birth_order,
        citizenship,
        veterans_military_status,
    ]
    segment = hl7.Segment(FIELD_SEPARATOR, fields)
    return segment


# noinspection PyUnusedLocal
def make_obr_segment(task: "Task") -> hl7.Segment:
    # noinspection HttpUrlsUsage
    """
    Creates an HL7 observation request (OBR) segment.

    - http://hl7reference.com/HL7%20Specifications%20ORM-ORU.PDF
    - Required in ORU^R01 message:

      - https://www.corepointhealth.com/resource-center/hl7-resources/hl7-oru-message
      - https://www.corepointhealth.com/resource-center/hl7-resources/hl7-obr-segment
    """  # noqa

    segment_id = "OBR"
    set_id = "1"
    placer_order_number = "CamCOPS"
    filler_order_number = "CamCOPS"
    universal_service_id = hl7.Field(COMPONENT_SEPARATOR, [
        "CamCOPS",
        "CamCOPS psychiatric/cognitive assessment"
    ])
    # unused below here, apparently
    priority = ""
    requested_date_time = ""
    observation_date_time = ""
    observation_end_date_time = ""
    collection_volume = ""
    collector_identifier = ""
    specimen_action_code = ""
    danger_code = ""
    relevant_clinical_information = ""
    specimen_received_date_time = ""
    ordering_provider = ""
    order_callback_phone_number = ""
    placer_field_1 = ""
    placer_field_2 = ""
    filler_field_1 = ""
    filler_field_2 = ""
    results_report_status_change_date_time = ""
    charge_to_practice = ""
    diagnostic_service_section_id = ""
    result_status = ""
    parent_result = ""
    quantity_timing = ""
    result_copies_to = ""
    parent = ""
    transportation_mode = ""
    reason_for_study = ""
    principal_result_interpreter = ""
    assistant_result_interpreter = ""
    technician = ""
    transcriptionist = ""
    scheduled_date_time = ""
    number_of_sample_containers = ""
    transport_logistics_of_collected_samples = ""
    collectors_comment = ""
    transport_arrangement_responsibility = ""
    transport_arranged = ""
    escort_required = ""
    planned_patient_transport_comment = ""

    fields = [
        segment_id,
        set_id,
        placer_order_number,
        filler_order_number,
        universal_service_id,
        priority,
        requested_date_time,
        observation_date_time,
        observation_end_date_time,
        collection_volume,
        collector_identifier,
        specimen_action_code,
        danger_code,
        relevant_clinical_information,
        specimen_received_date_time,
        ordering_provider,
        order_callback_phone_number,
        placer_field_1,
        placer_field_2,
        filler_field_1,
        filler_field_2,
        results_report_status_change_date_time,
        charge_to_practice,
        diagnostic_service_section_id,
        result_status,
        parent_result,
        quantity_timing,
        result_copies_to,
        parent,
        transportation_mode,
        reason_for_study,
        principal_result_interpreter,
        assistant_result_interpreter,
        technician,
        transcriptionist,
        scheduled_date_time,
        number_of_sample_containers,
        transport_logistics_of_collected_samples,
        collectors_comment,
        transport_arrangement_responsibility,
        transport_arranged,
        escort_required,
        planned_patient_transport_comment,
    ]
    segment = hl7.Segment(FIELD_SEPARATOR, fields)
    return segment


def make_obx_segment(req: "CamcopsRequest",
                     task: "Task",
                     task_format: str,
                     observation_identifier: str,
                     observation_datetime: Pendulum,
                     responsible_observer: str,
                     export_options: "TaskExportOptions") -> hl7.Segment:
    # noinspection HttpUrlsUsage
    """
    Creates an HL7 observation result (OBX) segment.

    - http://www.hl7standards.com/blog/2006/10/18/how-do-i-send-a-binary-file-inside-of-an-hl7-message
    - http://www.hl7standards.com/blog/2007/11/27/pdf-attachment-in-hl7-message/
    - http://www.hl7standards.com/blog/2006/12/01/sending-images-or-formatted-documents-via-hl7-messaging/
    - https://www.hl7.org/documentcenter/public/wg/ca/HL7ClmAttIG.PDF
    - type of data:
      https://www.hl7.org/implement/standards/fhir/v2/0191/index.html
    - subtype of data:
      https://www.hl7.org/implement/standards/fhir/v2/0291/index.html
    """  # noqa

    segment_id = "OBX"
    set_id = str(1)

    source_application = "CamCOPS"
    if task_format == FileType.PDF:
        value_type = "ED"  # Encapsulated data (ED) field
        observation_value = hl7.Field(COMPONENT_SEPARATOR, [
            source_application,
            "Application",  # type of data
            "PDF",  # data subtype
            "Base64",  # base 64 encoding
            base64.standard_b64encode(task.get_pdf(req))  # data
        ])
    elif task_format == FileType.HTML:
        value_type = "ED"  # Encapsulated data (ED) field
        observation_value = hl7.Field(COMPONENT_SEPARATOR, [
            source_application,
            "TEXT",  # type of data
            "HTML",  # data subtype
            "A",  # no encoding (see table 0299), but need to escape
            escape_hl7_text(task.get_html(req))  # data
        ])
    elif task_format == FileType.XML:
        value_type = "ED"  # Encapsulated data (ED) field
        observation_value = hl7.Field(COMPONENT_SEPARATOR, [
            source_application,
            "TEXT",  # type of data
            "XML",  # data subtype
            "A",  # no encoding (see table 0299), but need to escape
            escape_hl7_text(task.get_xml(
                req,
                indent_spaces=0,
                eol="",
                options=export_options,
            ))  # data
        ])
    else:
        raise AssertionError(
            f"make_obx_segment: invalid task_format: {task_format}")

    observation_sub_id = ""
    units = ""
    reference_range = ""
    abnormal_flags = ""
    probability = ""
    nature_of_abnormal_test = ""
    observation_result_status = ""
    date_of_last_observation_normal_values = ""
    user_defined_access_checks = ""
    date_and_time_of_observation = format_datetime(
        observation_datetime, DateFormat.HL7_DATETIME)
    producer_id = ""
    observation_method = ""
    equipment_instance_identifier = ""
    date_time_of_analysis = ""

    fields = [
        segment_id,
        set_id,
        value_type,
        observation_identifier,
        observation_sub_id,
        observation_value,
        units,
        reference_range,
        abnormal_flags,
        probability,
        nature_of_abnormal_test,
        observation_result_status,
        date_of_last_observation_normal_values,
        user_defined_access_checks,
        date_and_time_of_observation,
        producer_id,
        responsible_observer,
        observation_method,
        equipment_instance_identifier,
        date_time_of_analysis,
    ]
    segment = hl7.Segment(FIELD_SEPARATOR, fields)
    return segment


def make_dg1_segment(set_id: int,
                     diagnosis_datetime: Pendulum,
                     coding_system: str,
                     diagnosis_identifier: str,
                     diagnosis_text: str,
                     alternate_coding_system: str = "",
                     alternate_diagnosis_identifier: str = "",
                     alternate_diagnosis_text: str = "",
                     diagnosis_type: str = "F",
                     diagnosis_classification: str = "D",
                     confidential_indicator: str = "N",
                     clinician_id_number: Union[str, int] = None,
                     clinician_surname: str = "",
                     clinician_forename: str = "",
                     clinician_middle_name_or_initial: str = "",
                     clinician_suffix: str = "",
                     clinician_prefix: str = "",
                     clinician_degree: str = "",
                     clinician_source_table: str = "",
                     clinician_assigning_authority: str = "",
                     clinician_name_type_code: str = "",
                     clinician_identifier_type_code: str = "",
                     clinician_assigning_facility: str = "",
                     attestation_datetime: Pendulum = None) \
        -> hl7.Segment:
    # noinspection HttpUrlsUsage
    """
    Creates an HL7 diagnosis (DG1) segment.

    Args:

    .. code-block:: none

        set_id: Diagnosis sequence number, starting with 1 (use higher numbers
            for >1 diagnosis).
        diagnosis_datetime: Date/time diagnosis was made.

        coding_system: E.g. "I9C" for ICD9-CM; "I10" for ICD10.
        diagnosis_identifier: Code.
        diagnosis_text: Text.

        alternate_coding_system: Optional alternate coding system.
        alternate_diagnosis_identifier: Optional alternate code.
        alternate_diagnosis_text: Optional alternate text.

        diagnosis_type: A admitting, W working, F final.
        diagnosis_classification: C consultation, D diagnosis, M medication,
            O other, R radiological scheduling, S sign and symptom,
            T tissue diagnosis, I invasive procedure not classified elsewhere.
        confidential_indicator: Y yes, N no

        clinician_id_number:              } Diagnosing clinician.
        clinician_surname:                }
        clinician_forename:               }
        clinician_middle_name_or_initial: }
        clinician_suffix:                 }
        clinician_prefix:                 }
        clinician_degree:                 }
        clinician_source_table:           }
        clinician_assigning_authority:    }
        clinician_name_type_code:         }
        clinician_identifier_type_code:   }
        clinician_assigning_facility:     }

        attestation_datetime: Date/time the diagnosis was attested.

    - http://www.mexi.be/documents/hl7/ch600012.htm
    - https://www.hl7.org/special/committees/vocab/V26_Appendix_A.pdf
    """

    segment_id = "DG1"
    try:
        int(set_id)
        set_id = str(set_id)
    except Exception:
        raise AssertionError("make_dg1_segment: set_id invalid")
    diagnosis_coding_method = ""
    diagnosis_code = hl7.Field(COMPONENT_SEPARATOR, [
        diagnosis_identifier,
        diagnosis_text,
        coding_system,
        alternate_diagnosis_identifier,
        alternate_diagnosis_text,
        alternate_coding_system,
    ])
    diagnosis_description = ""
    diagnosis_datetime = format_datetime(diagnosis_datetime,
                                         DateFormat.HL7_DATETIME)
    if diagnosis_type not in ["A", "W", "F"]:
        raise AssertionError("make_dg1_segment: diagnosis_type invalid")
    major_diagnostic_category = ""
    diagnostic_related_group = ""
    drg_approval_indicator = ""
    drg_grouper_review_code = ""
    outlier_type = ""
    outlier_days = ""
    outlier_cost = ""
    grouper_version_and_type = ""
    diagnosis_priority = ""

    try:
        clinician_id_number = (
            str(int(clinician_id_number))
            if clinician_id_number is not None else ""
        )
    except Exception:
        raise AssertionError("make_dg1_segment: diagnosing_clinician_id_number"
                             " invalid")
    if clinician_id_number:
        clinician_id_check_digit = get_mod11_checkdigit(clinician_id_number)
        clinician_checkdigit_scheme = "M11"  # Mod 11 algorithm
    else:
        clinician_id_check_digit = ""
        clinician_checkdigit_scheme = ""
    diagnosing_clinician = hl7.Field(COMPONENT_SEPARATOR, [
        clinician_id_number,
        clinician_surname or "",
        clinician_forename or "",
        clinician_middle_name_or_initial or "",
        clinician_suffix or "",
        clinician_prefix or "",
        clinician_degree or "",
        clinician_source_table or "",
        clinician_assigning_authority or "",
        clinician_name_type_code or "",
        clinician_id_check_digit or "",
        clinician_checkdigit_scheme or "",
        clinician_identifier_type_code or "",
        clinician_assigning_facility or "",
    ])

    if diagnosis_classification not in ["C", "D", "M", "O", "R", "S", "T",
                                        "I"]:
        raise AssertionError(
            "make_dg1_segment: diagnosis_classification invalid")
    if confidential_indicator not in ["Y", "N"]:
        raise AssertionError(
            "make_dg1_segment: confidential_indicator invalid")
    attestation_datetime = (
        format_datetime(attestation_datetime, DateFormat.HL7_DATETIME)
        if attestation_datetime else ""
    )

    fields = [
        segment_id,
        set_id,
        diagnosis_coding_method,
        diagnosis_code,
        diagnosis_description,
        diagnosis_datetime,
        diagnosis_type,
        major_diagnostic_category,
        diagnostic_related_group,
        drg_approval_indicator,
        drg_grouper_review_code,
        outlier_type,
        outlier_days,
        outlier_cost,
        grouper_version_and_type,
        diagnosis_priority,
        diagnosing_clinician,
        diagnosis_classification,
        confidential_indicator,
        attestation_datetime,
    ]
    segment = hl7.Segment(FIELD_SEPARATOR, fields)
    return segment


def escape_hl7_text(s: str) -> str:
    # noinspection HttpUrlsUsage
    """
    Escapes HL7 special characters.

    - http://www.mexi.be/documents/hl7/ch200034.htm
    - http://www.mexi.be/documents/hl7/ch200071.htm
    """
    esc_escape = ESCAPE_CHARACTER + ESCAPE_CHARACTER + ESCAPE_CHARACTER
    esc_fieldsep = ESCAPE_CHARACTER + "F" + ESCAPE_CHARACTER
    esc_componentsep = ESCAPE_CHARACTER + "S" + ESCAPE_CHARACTER
    esc_subcomponentsep = ESCAPE_CHARACTER + "T" + ESCAPE_CHARACTER
    esc_repetitionsep = ESCAPE_CHARACTER + "R" + ESCAPE_CHARACTER

    # Linebreaks:
    # http://www.healthintersections.com.au/?p=344
    # https://groups.google.com/forum/#!topic/ensemble-in-healthcare/wP2DWMeFrPA  # noqa
    # http://www.hermetechnz.com/documentation/sqlschema/index.html?hl7_escape_rules.htm  # noqa
    esc_linebreak = ESCAPE_CHARACTER + ".br" + ESCAPE_CHARACTER

    s = s.replace(ESCAPE_CHARACTER, esc_escape)  # this one first!
    s = s.replace(FIELD_SEPARATOR, esc_fieldsep)
    s = s.replace(COMPONENT_SEPARATOR, esc_componentsep)
    s = s.replace(SUBCOMPONENT_SEPARATOR, esc_subcomponentsep)
    s = s.replace(REPETITION_SEPARATOR, esc_repetitionsep)
    s = s.replace("\n", esc_linebreak)
    return s


def msg_is_successful_ack(msg: hl7.Message) -> Tuple[bool, Optional[str]]:
    """
    Checks whether msg represents a successful acknowledgement message.

    - http://hl7reference.com/HL7%20Specifications%20ORM-ORU.PDF
    """

    if msg is None:
        return False, "Reply is None"

    # Get segments (MSH, MSA)
    if len(msg) != 2:
        return False, f"Reply doesn't have 2 segments (has {len(msg)})"
    msh_segment = msg[0]
    msa_segment = msg[1]

    # Check MSH segment
    if len(msh_segment) < 9:
        return False, (
            f"First (MSH) segment has <9 fields (has {len(msh_segment)})"
        )
    msh_segment_id = msh_segment[0]
    msh_message_type = msh_segment[8]
    if msh_segment_id != ["MSH"]:
        return False, (
            f"First (MSH) segment ID is not 'MSH' (is {msh_segment_id})"
        )
    if msh_message_type != ["ACK"]:
        return False, (
            f"MSH message type is not 'ACK' (is {msh_message_type})"
        )

    # Check MSA segment
    if len(msa_segment) < 2:
        return False, (
            f"Second (MSA) segment has <2 fields (has {len(msa_segment)})"
        )
    msa_segment_id = msa_segment[0]
    msa_acknowledgment_code = msa_segment[1]
    if msa_segment_id != ["MSA"]:
        return False, (
            f"Second (MSA) segment ID is not 'MSA' (is {msa_segment_id})"
        )
    if msa_acknowledgment_code != ["AA"]:
        # AA for success, AE for error
        return False, (
            f"MSA acknowledgement code is not 'AA' "
            f"(is {msa_acknowledgment_code})"
        )

    return True, None


# =============================================================================
# MLLPTimeoutClient
# =============================================================================
# Modification of MLLPClient from python-hl7, to allow timeouts and failure.

SB = '\x0b'  # <SB>, vertical tab
EB = '\x1c'  # <EB>, file separator
CR = '\x0d'  # <CR>, \r
FF = '\x0c'  # <FF>, new page form feed

RECV_BUFFER = 4096


class MLLPTimeoutClient(object):
    """
    Class for MLLP TCP/IP transmission that implements timeouts.
    """

    def __init__(self, host: str, port: int, timeout_ms: int = None) -> None:
        """Creates MLLP client and opens socket."""
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        timeout_s = float(timeout_ms) / float(1000) \
            if timeout_ms is not None else None
        self.socket.settimeout(timeout_s)
        self.socket.connect((host, port))
        self.encoding = "utf-8"

    def __enter__(self):
        """
        For use with "with" statement.
        """
        return self

    # noinspection PyUnusedLocal
    def __exit__(self, exc_type, exc_val, traceback):
        """
        For use with "with" statement.
        """
        self.close()

    def close(self):
        """
        Release the socket connection.
        """
        self.socket.close()

    def send_message(self, message: Union[str, hl7.Message]) \
            -> Tuple[bool, Optional[str]]:
        """
        Wraps a string or :class:`hl7.Message` in a MLLP container
        and sends the message to the server.

        Returns ``success, ack_msg``.
        """
        if isinstance(message, hl7.Message):
            message = str(message)
        # wrap in MLLP message container
        data = SB + message + CR + EB + CR
        # ... the CR immediately after the message is my addition, because
        # HL7 Inspector otherwise says: "Warning: last segment have no segment
        # termination char 0x0d !" (sic).
        return self.send(data.encode(self.encoding))

    def send(self, data: bytes) -> Tuple[bool, Optional[str]]:
        """
        Low-level, direct access to the ``socket.send`` function (data must be
        already wrapped in an MLLP container). Blocks until the server
        returns.

        Returns ``success, ack_msg``.
        """
        # upload the data
        self.socket.send(data)
        # wait for the ACK/NACK
        try:
            ack_msg = self.socket.recv(RECV_BUFFER).decode(self.encoding)
            return True, ack_msg
        except socket.timeout:
            return False, None
