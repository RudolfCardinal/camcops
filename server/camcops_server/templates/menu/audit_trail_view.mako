## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/audit_trail_view.mako

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

from mako.filters import html_escape
from camcops_server.cc_modules.cc_pyramid import Routes, ViewArg, ViewParam

def trunc(value, truncate, truncate_at):
    if not value:
        return ""
    text = value[:truncate_at] if truncate else value
    return html_escape(text)


def filter_generic_value(value):
    if value is None:
        return ""
    return html_escape(str(value))


def get_username(audit_entry):
    if audit_entry.user is None or not audit_entry.user.username:
        return ""
    return html_escape(audit_entry.user.username)

%>

<%include file="db_user_info.mako"/>

<h1>${ _("Audit trail") }</h1>

%if conditions:
    <h2>${ _("Conditions") }</h2>
    ${ conditions }
%endif

<h2>${ _("Results") }</h2>

<div>${ page.pager() | n }</div>

<table>
    <tr>
        <th>${ _("ID") }</th>
        <th>${ _("When (UTC)") }</th>
        <th>${ _("Source") }</th>
        <th>${ _("Remote IP") }</th>
        <th>${ _("Username") }</th>
        <th>${ _("Device ID") }</th>
        <th>${ _("Table name") }</th>
        <th>${ _("Server PK") }</th>
        <th>${ _("Patient server PK") }</th>
        %if truncate:
            <th>${ _("Details (truncated)") }</th>
        %else:
            <th>${ _("Details") }</th>
        %endif
    </tr>
    %for audit in page:
        <tr>
            <td>${ audit.id }</td>
            <td>${ audit.when_access_utc }</td>
            <td>${ filter_generic_value(audit.source) | n }</td>
            <td>${ filter_generic_value(audit.remote_addr) | n }</td>
            <td>${ get_username(audit) }</td>
            <td>${ filter_generic_value(audit.device_id) | n }</td>
            <td>${ filter_generic_value(audit.table_name) | n }</td>
            <td>
                ${ filter_generic_value(audit.server_pk) | n }
                %if audit.server_pk:
                    (<a href="${ req.route_url(
                        Routes.TASK,
                        _query={
                            ViewParam.TABLE_NAME: audit.table_name,
                            ViewParam.SERVER_PK: audit.server_pk,
                            ViewParam.VIEWTYPE: ViewArg.HTML
                        }
                    ) | n }">${ _("View task") }</a>)
                %endif
            </td>
            <td>${ filter_generic_value(audit.patient_server_pk) | n }</td>
            <td>${ trunc(audit.details, truncate, truncate_at) | n }</td>
        </tr>
    %endfor
</table>

<div>${ page.pager() | n }</div>

<div>
    <a href="${ req.route_url(Routes.OFFER_AUDIT_TRAIL) | n }">
        ${ _("Choose different options") }
    </a>
</div>

<%include file="to_main_menu.mako"/>
