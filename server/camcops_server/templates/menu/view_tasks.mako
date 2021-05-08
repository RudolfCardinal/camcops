## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/view_tasks.mako

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

## <%page args="page: Page, head_form_html: str, no_patient_selected_and_user_restricted: bool, user: User"/>
<%inherit file="base_web_form.mako"/>

<%!

from camcops_server.cc_modules.cc_pyramid import Routes, ViewArg, ViewParam

%>

<%include file="db_user_info.mako"/>

<h1>${ _("Currently applicable filters") }</h1>

<%include file="describe_task_filter.mako" args="task_filter=request.camcops_session.get_task_filter()"/>

<div><a href="${ request.route_url(Routes.SET_FILTERS) | n }">
    ${ _("Set or clear filters") }</a></div>

<h1>${ _("Tasks") }</h1>

## Tasks-per-page form:
${ tpp_form | n }

${ refresh_form | n }

%if no_patient_selected_and_user_restricted:
    <div class="explanation">
        ${ _("Your user isn’t configured to view all patients’ records when "
             "no patient filters are applied, and none is. Records will only "
             "be shown if they are anonymous, or for groups that allow you to "
             "see all patients in these circumstances. Choose a specific "
             "patient to see their records.") }
    </div>
%endif
%if not user.superuser and not user.group_ids:
    <div class="warning">
        ${ _("Your administrator has not assigned you to any groups. You "
             "won’t be able to see any tasks.") }
    </div>
%endif

%if page.item_count == 0:

    <div class="important">
        ${ _("No tasks found for your search criteria!") }
    </div>

%else:

    <div>${ page.pager() | n }</div>

    <%include file="view_tasks_table.mako" args="tasks=page"/>

    <div>${ page.pager() | n }</div>

    <div class="footnotes">
        ${ _("Colour in the Patient column means that an ID policy is not "
             "yet satisfied. Colour in the Identifiers column means that an "
             "ID number is invalid. Colour in the Task Type column means the "
             "record is not current. Colour in the Created column means the "
             "task is ‘live’ on the tablet, not finalized (so patient and "
             "task details may change). Colour in the View/Print columns "
             "means the task is incomplete.") }
        ## NOT CURRENTLY: Colour in the Identifiers column means a conflict
        ## between the server’s and the tablet’s ID descriptions.
    </div>

%endif

<%include file="to_main_menu.mako"/>
