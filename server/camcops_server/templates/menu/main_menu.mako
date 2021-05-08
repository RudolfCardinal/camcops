## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/main_menu.mako

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

<%include file="db_user_info.mako" args="offer_main_menu=False"/>

## TRANSLATOR: Mako comment
<h1>${ _("CamCOPS web view: Main menu") }</h1>

<h3>${ _("Tasks, trackers, and clinical text views") }</h3>
<ul>
    <li><a href="${ request.route_url(
                        Routes.SET_FILTERS,
                        _query={
                            ViewParam.REDIRECT_URL: request.route_url(Routes.HOME)
                        }
                    ) | n }">${ _("Filter tasks") }</a></li>
    <li><a href="${request.route_url(
                        Routes.VIEW_TASKS,
                        _query={ViewParam.VIA_INDEX: True}
                    ) | n }">${ _("View tasks") }</a></li>
    <li><a href="${ request.route_url(Routes.CHOOSE_TRACKER) | n }">
        ${ _("Trackers for numerical information") }</a></li>
    <li><a href="${ request.route_url(Routes.CHOOSE_CTV) | n }">
        ${ _("Clinical text views") }</a></li>
</ul>

%if authorized_to_dump:
    <h3>${ _("Research views") }</h3>
    <ul>
        <li><a href="${ request.route_url(Routes.OFFER_BASIC_DUMP) | n }">
            ${ _("Basic research dump (fields and summaries)") }</a></li>
        <li><a href="${ request.route_url(Routes.OFFER_SQL_DUMP) | n }">
            ${ _("Advanced research dump (SQL or database)") }</a></li>
        <li><a href="${ request.route_url(Routes.VIEW_DDL) | n }">
            ${ _("Inspect table definitions") }</a>
        <li><a href="${ request.route_url(Routes.DOWNLOAD_AREA) | n }">
            ${ _("Download area") }</a>
    </ul>
%endif

%if authorized_for_reports:
    <h3>${ _("Reports") }</h3>
    <ul>
        <li><a href="${ request.route_url(Routes.REPORTS_MENU) | n }">
            ${ _("Run reports") }</a></li>
    </ul>
%endif

%if authorized_as_groupadmin:
    <h3>${ _("Group administrator options") }</h3>
    <ul>
        <li><a href="${ request.route_url(Routes.VIEW_ALL_USERS) | n }">
            ${ _("User management" ) }</a></li>
        <li><a href="${ request.route_url(Routes.VIEW_USER_EMAIL_ADDRESSES) | n }">
            ${ _("E-mail addresses of your users") }</a></li>
        <li><a href="${ request.route_url(Routes.FORCIBLY_FINALIZE) | n }">
            ${ _("Forcibly finalize records for a device") }</a></li>
        <li><a href="${ request.route_url(Routes.DELETE_PATIENT) | n }">
            ${ _("Delete patient entirely") }</a></li>
        <li><a href="${ request.route_url(Routes.VIEW_PATIENT_TASK_SCHEDULES) | n }">
            ${ _("Manage scheduled tasks for patients") }</a></li>
    </ul>
%endif

%if authorized_as_superuser:
    <h3>${ _("Superuser options") }</h3>
    <ul>
        <li><a href="${ request.route_url(Routes.VIEW_GROUPS) | n }">
            ${ _("Group management") }</a></li>
        <li><a href="${ request.route_url(Routes.AUDIT_MENU) | n }">
            ${ _("Audit menu") }</a></li>
        <li><a href="${ request.route_url(Routes.VIEW_ID_DEFINITIONS) | n }">
            ${ _("ID number definition management") }</a></li>
        <li><a href="${ request.route_url(Routes.EDIT_SERVER_SETTINGS) | n }">
            ${ _("Edit server settings") }</a></li>
        <li><a href="${ request.route_url(Routes.DEVELOPER) | n }">
            ${ _("Developer test pages") }</a></li>
    </ul>
%endif

<h3>${ _("Settings") }</h3>
<ul>
    <li><a href="${ request.route_url(Routes.SET_OWN_USER_UPLOAD_GROUP) | n }">
        ${ _("Choose group into which to upload data") }</a></li>
    <li><a href="${ request.route_url(Routes.VIEW_OWN_USER_INFO) | n }">
        ${ _("View your user settings") }</a></li>
    <li><a href="${ request.route_url(Routes.VIEW_SERVER_INFO) | n }">
        ${ _("View server information") }</a></li>
    %if request.camcops_session.username:
        <li><a href="${ request.route_url(
                            Routes.CHANGE_OWN_PASSWORD,
                            username=request.camcops_session.username
                        ) | n }">${ _("Change password") }</a></li>
    %else:
        <li class="warning">${ _("No username!") }</li>
    %endif
</ul>

<h3>${ _("Help") }</h3>
<ul>
    <li><a href="${ request.url_camcops_docs | n }">
        ${ _("CamCOPS documentation") }</a></li>
</ul>

<div class="office">
    ${ _("The time is") } ${ now }.
    ${ _("Server version:") } ${ server_version }.
    ${ _("See") } <a href="${ camcops_url | n }">${ camcops_url }</a>
    ${ _("for more information on CamCOPS.") }
    ${ _("Clicking on the CamCOPS logo will return you to the main menu.") }
</div>
