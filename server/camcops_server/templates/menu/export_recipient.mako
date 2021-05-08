## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/export_recipient.mako

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

</%doc>

<%inherit file="base_web.mako"/>

<%!
from markupsafe import escape
from camcops_server.cc_modules.cc_pyramid import Routes, ViewArg, ViewParam
%>

<%include file="db_user_info.mako"/>

<h1>${ _("Export recipient") }</h1>

<table>
    <!-- Identification, current -->
    <tr>
        <th>Recipient ID</th>
        <td>${ recipient.id }</td>
    </tr>
    <tr>
        <th>Name</th>
        <td>${ recipient.recipient_name or "" }</td>
    </tr>
    <tr>
        <th>Current?</th>
        <td>${ recipient.current }</td>
    </tr>

    <!-- How to export -->
    <tr>
        <th>How: Transmission method</th>
        <td>${ recipient.transmission_method or "" }</td>
    </tr>
    <tr>
        <th>How: Push?</th>
        <td>${ recipient.push }</td>
    </tr>
    <tr>
        <th>How: Task format</th>
        <td>${ recipient.task_format or "" }</td>
    </tr>
    <tr>
        <th>How: Include fields comments in XML?</th>
        <td>${ recipient.xml_field_comments }</td>
    </tr>

    <!-- What to export -->
    <tr>
        <th>What: All groups?</th>
        <td>${ recipient.all_groups }</td>
    </tr>
    <tr>
        <th>What: Groups</th>
        <td>${ ", ".join(str(gid) for gid in recipient.group_ids) }</td>
    </tr>
    <tr>
        <th>What: Start date/time (UTC)</th>
        <td>${ recipient.start_datetime_utc }</td>
    </tr>
    <tr>
        <th>What: End date/time (UTC)</th>
        <td>${ recipient.end_datetime_utc }</td>
    </tr>
    <tr>
        <th>What: Finalized tasks only?</th>
        <td>${ recipient.finalized_only }</td>
    </tr>
    <tr>
        <th>What: Include anonymous tasks?</th>
        <td>${ recipient.include_anonymous }</td>
    </tr>
    <tr>
        <th>What: Primary ID number type</th>
        <td>${ recipient.primary_idnum }</td>
    </tr>
    <tr>
        <th>What: Require primary ID number to be mandatory?</th>
        <td>${ recipient.require_idnum_mandatory }</td>
    </tr>

    <!-- Database -->
    <tr>
        <th>Database: URL</th>
        <td>${ recipient.db_url_obscuring_password or "" }</td>
    </tr>
    <tr>
        <th>Database: echo?</th>
        <td>${ recipient.db_echo }</td>
    </tr>
    <tr>
        <th>Database: include BLOBs?</th>
        <td>${ recipient.db_include_blobs }</td>
    </tr>
    <tr>
        <th>Database: add summaries?</th>
        <td>${ recipient.db_add_summaries }</td>
    </tr>
    <tr>
        <th>Database: patient ID per row?</th>
        <td>${ recipient.db_patient_id_per_row }</td>
    </tr>

    <!-- E-mail -->
    <tr>
        <th>E-mail: host</th>
        <td>${ recipient.email_host or "" }</td>
    </tr>
    <tr>
        <th>E-mail: port</th>
        <td>${ recipient.email_port }</td>
    </tr>
    <tr>
        <th>E-mail: use TLS?</th>
        <td>${ recipient.email_use_tls }</td>
    </tr>
    <tr>
        <th>E-mail: from</th>
        <td>${ recipient.email_from or "" }</td>
    </tr>
    <tr>
        <th>E-mail: sender</th>
        <td>${ recipient.email_sender or "" }</td>
    </tr>
    <tr>
        <th>E-mail: reply-to</th>
        <td>${ recipient.email_reply_to or "" }</td>
    </tr>
    <tr>
        <th>E-mail: to</th>
        <td>${ recipient.email_to or "" }</td>
    </tr>
    <tr>
        <th>E-mail: CC</th>
        <td>${ recipient.email_cc or "" }</td>
    </tr>
    <tr>
        <th>E-mail: BCC</th>
        <td>${ recipient.email_bcc or "" }</td>
    </tr>
    <tr>
        <th>E-mail: patient spec</th>
        <td>${ recipient.email_patient_spec or "" }</td>
    </tr>
    <tr>
        <th>E-mail: patient spec if anonymous</th>
        <td>${ recipient.email_patient_spec_if_anonymous or "" }</td>
    </tr>
    <tr>
        <th>E-mail: subject</th>
        <td>${ recipient.email_subject or "" }</td>
    </tr>
    <tr>
        <th>E-mail: treat body as HTML?</th>
        <td>${ recipient.email_body_as_html }</td>
    </tr>
    <tr>
        <th>E-mail: body</th>
        <td><pre>${ recipient.email_body or "" }</pre></td>
    </tr>
    <tr>
        <th>E-mail: keep message?</th>
        <td>${ recipient.email_keep_message }</td>
    </tr>

    <!-- File -->
    <tr>
        <th>File: patient spec</th>
        <td>${ recipient.file_patient_spec or "" }</td>
    </tr>
    <tr>
        <th>File: patient spec if anonymous</th>
        <td>${ recipient.file_patient_spec_if_anonymous or "" }</td>
    </tr>
    <tr>
        <th>File: filename spec</th>
        <td>${ recipient.file_filename_spec or "" }</td>
    </tr>
    <tr>
        <th>File: make directory?</th>
        <td>${ recipient.file_make_directory }</td>
    </tr>
    <tr>
        <th>File: overwrite files?</th>
        <td>${ recipient.file_overwrite_files }</td>
    </tr>
    <tr>
        <th>File: export RiO metadata?</th>
        <td>${ recipient.file_export_rio_metadata }</td>
    </tr>
    <tr>
        <th>File: script to run after export</th>
        <td>${ recipient.file_script_after_export or "" }</td>
    </tr>

    <!-- File/RiO -->
    <tr>
        <th>File: RiO metadata: which ID number is the RiO ID?</th>
        <td>${ recipient.rio_idnum }</td>
    </tr>
    <tr>
        <th>File: RiO metadata: name of automatic upload user</th>
        <td>${ recipient.rio_uploading_user or "" }</td>
    </tr>
    <tr>
        <th>File: RiO metadata: document type for RiO</th>
        <td>${ recipient.rio_document_type or "" }</td>
    </tr>

    <!-- HL7 -->
    <tr>
        <th>HL7: host</th>
        <td>${ recipient.hl7_host or "" }</td>
    </tr>
    <tr>
        <th>HL7: port</th>
        <td>${ recipient.hl7_port }</td>
    </tr>
    <tr>
        <th>HL7: ping first?</th>
        <td>${ recipient.hl7_ping_first }</td>
    </tr>
    <tr>
        <th>HL7: network timeout (ms)</th>
        <td>${ recipient.hl7_network_timeout_ms }</td>
    </tr>
    <tr>
        <th>HL7: keep messages?</th>
        <td>${ recipient.hl7_keep_message }</td>
    </tr>
    <tr>
        <th>HL7: keep reply?</th>
        <td>${ recipient.hl7_keep_reply }</td>
    </tr>
    <tr>
        <th>HL7 (DEBUG): divert to file?</th>
        <td>${ recipient.hl7_debug_divert_to_file }</td>
    </tr>
    <tr>
        <th>HL7 (DEBUG): treat diverted as sent?</th>
        <td>${ recipient.hl7_debug_treat_diverted_as_sent }</td>
    </tr>

</table>

<%include file="to_main_menu.mako"/>
