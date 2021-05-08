## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/taskcommon/clinician.mako

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

<%page args="task: Task"/>

<div class="clinician">
    <table class="taskdetail">
        <tr>
            <td style="width:50%">${ _("Clinician’s specialty:") }</td>
            <td style="width:50%"><b>${ task.clinician_specialty }</b></td>
        </tr>
        <tr>
            <td>${ _("Clinician’s name:") }</td>
            <td><b>${ task.clinician_name }</b></td>
        </tr>
        <tr>
            <td>${ _("Clinician’s professional registration:") }</td>
            <td><b>${ task.clinician_professional_registration }</b></td>
        </tr>
        <tr>
            <td>${ _("Clinician’s post:") }</td>
            <td><b>${ task.clinician_post }</b></td>
        </tr>
        <tr>
            <td>${ _("Clinician’s service:") }</td>
            <td><b>${ task.clinician_service }</b></td>
        </tr>
        <tr>
            <td>${ _("Clinician’s contact details:") }</td>
            <td><b>${ task.clinician_contact_details }</b></td>
        </tr>
    </table>
</div>
