## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/exported_task_email.mako

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

<h1>${ _("Exported task e-mail") }</h1>

<table>
    <tr>
        <th>ExportedTaskEmail ID</th>
        <td>${ ete.id }</td>
    </tr>
    <tr>
        <th>Exported task ID</th>
        <td><a href="${ req.route_url(
                            Routes.VIEW_EXPORTED_TASK,
                            _query={ViewParam.ID: ete.exported_task_id
                        }) | n }">ExportedTask ${ ete.exported_task_id }</a></td>
    </tr>
    <tr>
        <th>E-mail ID</th>
        <td><a href="${ req.route_url(
                            Routes.VIEW_EMAIL,
                            _query={ViewParam.ID: ete.email_id}
                        ) | n }">Email ${ ete.email_id } </a></td>
    </tr>
</table>

<%include file="to_main_menu.mako"/>
