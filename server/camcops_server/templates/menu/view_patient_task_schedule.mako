## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/view_patient_task_schedule.mako

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
from cardinal_pythonlib.datetimefunc import format_datetime

from camcops_server.cc_modules.cc_constants import DateFormat
from camcops_server.cc_modules.cc_pyramid import Routes, ViewArg, ViewParam
%>

<%include file="db_user_info.mako"/>

<h1>
    ${ _("Scheduled tasks for {patient} on schedule: {schedule}").format(
            patient=patient_descriptor, schedule=schedule_name) }
</h1>

<table>
    <tr>
        <th>${ _("Task") }</th>
        <th>${ _("Due from") }</th>
        <th>${ _("Due by") }</th>
        <th>${ _("Created") }</th>
        <th>${ _("Complete") }</th>
        <th>${ _("View") }</th>
        <th>${ _("Print/save") }</th>
    </tr>
%for task_info in task_list:
    <%
        td_attributes = ""
        if task_info.task and not task_info.task.is_complete():
            td_attributes = "class='incomplete'"
    %>
    <tr>
        <td>
            ${ task_info.shortname }
        </td>
        <td>
            %if task_info.start_datetime:
            ${ format_datetime(task_info.start_datetime, DateFormat.SHORT_DATETIME_NO_TZ) }
            %endif
        </td>
        <td>
            %if task_info.end_datetime:
            ${ format_datetime(task_info.end_datetime, DateFormat.SHORT_DATETIME_NO_TZ) }
            %endif
        </td>
        <td>
            %if task_info.is_anonymous:
               —
            %elif task_info.task:
               ${ format_datetime(task_info.task.when_created, DateFormat.SHORT_DATETIME_NO_TZ) }
            %endif
        </td>
        <td>
            %if task_info.is_anonymous:
               —
            %elif task_info.task:
               ${ task_info.task.is_complete() }
            %endif
        </td>
        <td ${ td_attributes | n }>
            %if task_info.is_anonymous:
               —
            %elif task_info.task:
                <a href="${ req.route_url(
                                Routes.TASK,
                                _query={
                                    ViewParam.TABLE_NAME: task_info.task.tablename,
                                    ViewParam.SERVER_PK: task_info.task.pk,
                                    ViewParam.VIEWTYPE: ViewArg.HTML,
                                }) | n }">HTML</a>
            %endif
        </td>
        <td ${ td_attributes }>
            %if task_info.is_anonymous:
               —
            %elif task_info.task:
                <a href="${ req.route_url(
                                Routes.TASK,
                                _query={
                                    ViewParam.TABLE_NAME: task_info.task.tablename,
                                    ViewParam.SERVER_PK: task_info.task.pk,
                                    ViewParam.VIEWTYPE: ViewArg.PDF,
                                }) | n }">PDF</a>
            %endif
        </td>
    </tr>
%endfor
</table>

<div>
    <a href="${ req.route_url(Routes.VIEW_PATIENT_TASK_SCHEDULES) | n }">
        ${ _("Manage scheduled tasks for patients") }</a>
    <%include file="to_main_menu.mako"/>
</div>
