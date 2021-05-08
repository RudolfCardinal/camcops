## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/exported_task.mako

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

def listview(req, objects, route_name, description):
    parts = []
    for obj in objects:
        url = req.route_url(route_name, _query={ViewParam.ID: obj.id})
        parts.append('<a href="{url}">{description} {id}</a>'.format(
            url=url, description=description, id=obj.id))
    return "<br>".join(parts)

%>

<%include file="db_user_info.mako"/>

<h1>${ _("Individual task export attempt") }</h1>

<table>
    <tr>
        <th>Export ID</th>
        <td>${ et.id }</td>
    </tr>
    <tr>
        <th>Export recipient ID</th>
        <td>
            <a href="${ req.route_url(
                            Routes.VIEW_EXPORT_RECIPIENT,
                            _query={ViewParam.ID: et.recipient_id}
                        ) | n }">ExportRecipient ${ et.recipient_id }</a>
        </td>
    </tr>
    <tr>
        <th>Base table</th>
        <td>${ et.basetable }</td>
    </tr>
    <tr>
        <th>Task server PK</th>
        <td>
            ${ et.task_server_pk }
            (<a href="${ req.route_url(
                            Routes.TASK,
                            _query={
                                ViewParam.TABLE_NAME: et.basetable,
                                ViewParam.SERVER_PK: et.task_server_pk,
                                ViewParam.VIEWTYPE: ViewArg.HTML
                            }
                        ) | n }">${ _("View task") }</a>)
        </td>
    </tr>
    <tr>
        <th>Start at (UTC)</th>
        <td>${ et.start_at_utc }</td>
    </tr>
    <tr>
        <th>Finish at (UTC)</th>
        <td>${ et.finish_at_utc }</td>
    </tr>
    <tr>
        <th>Success?</th>
        <td>${ et.success }</td>
    </tr>
    <tr>
        <th>Failure reasons</th>
        <td>${ "<br>".join(escape(reason) for reason in et.failure_reasons) | n }</td>
    </tr>
    <tr>
        <th>Cancelled?</th>
        <td>${ et.cancelled }</td>
    </tr>
    <tr>
        <th>Cancelled at (UTC)</th>
        <td>${ et.cancelled_at_utc or "" }</td>
    </tr>
    <tr>
        <th>E-mails</th>
        <td>${ listview(req, et.emails, Routes.VIEW_EXPORTED_TASK_EMAIL,
                        "ExportedTaskEmail") | n }</td>
    </tr>
    <tr>
        <th>Files</th>
        <td>${ listview(req, et.filegroups, Routes.VIEW_EXPORTED_TASK_FILE_GROUP,
                        "ExportedTaskFileGroup") | n }</td>
    </tr>
    <tr>
        <th>HL7 messages</th>
        <td>${ listview(req, et.hl7_messages, Routes.VIEW_EXPORTED_TASK_HL7_MESSAGE,
                        "ExportedTaskHL7Message") | n }</td>
    </tr>
</table>

<%include file="to_main_menu.mako"/>
