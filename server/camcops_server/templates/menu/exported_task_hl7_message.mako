## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/exported_task_hl7_message.mako

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
from camcops_server.cc_modules.cc_pyramid import Routes, ViewArg, ViewParam
%>

<%include file="db_user_info.mako"/>

<h1>${ _("Individual HL7 message") }</h1>

<table>
    <tr>
        <th>HL7 message ID</th>
        <td>${ msg.id }</td>
    </tr>
    <tr>
        <th>Exported task ID</th>
        <td><a href="${ req.route_url(
                            Routes.VIEW_EXPORTED_TASK,
                            _query={ViewParam.ID: msg.exported_task_id}
                        ) | n }">ExportedTask ${ msg.exported_task_id }</a></td>
    </tr>
    <tr>
        <th>Sent at (UTC)</th>
        <td>${ msg.sent_at_utc }</td>
    </tr>
    <tr>
        <th>Reply at (UTC)</th>
        <td>${ msg.reply_at_utc }</td>
    </tr>
    <tr>
        <th>Success?</th>
        <td>${ msg.success }</td>
    </tr>
    <tr>
        <th>Failure reason</th>
        <td>${ msg.failure_reason }</td>
    </tr>
    <tr>
        <th>Message</th>
        <td><pre>${ msg.message or "" }</pre></td>
    </tr>
    <tr>
        <th>Reply</th>
        <td><pre>${ msg.reply or "" }</pre></td>
    </tr>
</table>

<%include file="to_main_menu.mako"/>
