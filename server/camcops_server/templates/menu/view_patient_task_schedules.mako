## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/view_patient_task_schedules.mako

===============================================================================

    Copyright (C) 2012-2019 Rudolf Cardinal (rudolf@pobox.com).

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
from urllib.parse import quote, urlencode

from cardinal_pythonlib.datetimefunc import format_datetime

from camcops_server.cc_modules.cc_constants import DateFormat
from camcops_server.cc_modules.cc_pyramid import Routes, ViewArg, ViewParam
%>

<%include file="db_user_info.mako"/>
<div>
    ${ _("CamCOPS server location:") }
    ${ req.route_url( Routes.CLIENT_API ) | n }
</div>

<h1>${ _("Patient Task Schedules") }</h1>

<div>${ page.pager() | n }</div>

<table>
    <tr>
        <th>${ _("Patient") }</th>
        <th>${ _("Identifiers") }</th>
        <th>${ _("Access key") }</th>
        <th>${ _("Task schedules") }</th>
        <th>${ _("Edit patient") }</th>
        <th>${ _("Delete patient") }</th>
    </tr>
%for patient in page:
    <tr>
        <td>
            <b>${ patient.get_surname_forename_upper() }</b>
            (${ patient.get_sex_verbose() },
            ${ format_datetime(patient.dob, DateFormat.SHORT_DATE, default="?") })
        </td>
        <td>
            %for idobj in patient.idnums:
                ${ idobj.short_description(request) }:&nbsp;${ idobj.idnum_value }.
                <br>
            %endfor
        </td>
        <td>
            ${ patient.uuid_as_proquint }
        </td>
        <td>
            %for pts in patient.task_schedules:
                <a href="${ req.route_url(
                                 Routes.VIEW_PATIENT_TASK_SCHEDULE,
                                 _query={
                                     ViewParam.PATIENT_TASK_SCHEDULE_ID: pts.id
                                 }) | n }">${ pts.task_schedule.name }</a>
            %if patient.email:
                [<a href="${ pts.mailto_url(req) | n }">${ _("Email") }</a>]<br>
            %endif
            %endfor
        </td>
        <td>
            <a href="${ req.route_url(
                             Routes.EDIT_SERVER_CREATED_PATIENT,
                             _query={
                                 ViewParam.SERVER_PK: patient.pk
                             }) | n }">${ _("Edit") }</a>
        </td>
        <td>
            <a href="${ req.route_url(
                             Routes.DELETE_SERVER_CREATED_PATIENT,
                             _query={
                                 ViewParam.SERVER_PK: patient.pk
                             }) | n }">${ _("Delete") }</a>
        </td>
    </tr>
%endfor
</table>

<div>${ page.pager() | n }</div>

<div>
    <a href="${ req.route_url(Routes.ADD_PATIENT) | n }">${ _("Add a patient") }</a>
</div>
<div>
    <a href="${ request.route_url(Routes.VIEW_TASK_SCHEDULES) | n }">
        ${ _("Manage task schedules") }</a>
</div>

<%include file="to_main_menu.mako"/>
