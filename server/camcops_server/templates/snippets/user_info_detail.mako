## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/snippets/user_info_detail.mako

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

<%page args="user: User"/>

<%!

from camcops_server.cc_modules.cc_html import get_yes_no
from camcops_server.cc_modules.cc_pyramid import Routes, ViewArg, ViewParam

%>

<%namespace file="displayfunc.mako" import="one_per_line"/>

<h2>${ _("Core information") }</h2>

<table>
    <tr>
        <th>${ _("Username") }</th>
        <td>${ user.username }</td>
    </tr>
    <tr>
        <th>${ _("User ID") }</th>
        <td>${ user.id }</td>
    </tr>
    <tr>
        <th>${ _("Full name") }</th>
        <td>${ user.fullname or "" }</td>
    </tr>
    <tr>
        <th>${ _("E-mail address") }</th>
        <td>${ user.email or "" }</td>
    </tr>
    <tr>
        <th>${ _("Last login at (UTC)") }</th>
        <td>${ user.last_login_at_utc }</td>
    </tr>
    <tr>
        <th>${ _("Locked out?") }</th>
        <td>${ get_yes_no(request, user.is_locked_out(request)) }</td>
    </tr>
    <tr>
        <th>${ _("Last password change (UTC)") }</th>
        <td>${ user.last_password_change_utc }</td>
    </tr>
    <tr>
        <th>${ _("Must change password at next login?") }</th>
        <td>${ get_yes_no(request, user.must_change_password) }</td>
    </tr>
    <tr>
        <th>${ _("Agreed to terms of use at:") }</th>
        <td>${ user.when_agreed_terms_of_use }</td>
    </tr>
    <tr>
        <th>${ _("Language") }</th>
        <td>${ user.language }</td>
    </tr>
    <tr>
        <th>${ _("Superuser?") }</th>
        <td ${ ('class="important"' if user.superuser else "") | n }>
            ${ get_yes_no(request, user.superuser) }
        </td>
    </tr>
</table>

<h2>${ _("Summary of group membership information") }</h2>

<table>
    <tr>
        <th>${ _("May log in to web viewer?") }</th>
        <td>${ get_yes_no(request, user.may_use_webviewer) }</td>
    </tr>
    <tr>
        <th>${ _("May register new tablet devices?") }</th>
        <td>${ get_yes_no(request, user.may_register_devices) }</td>
    </tr>
    <tr>
        <th>${ _("Groups this user is a member of:") }</th>
        <td>
            <%
                groups = list(user.groups)
                groups.sort(key=lambda g: g.name)
            %>
            ${ one_per_line(g.name for g in groups) | n }
        </td>
    </tr>
    <tr>
        <th>${ _("Groups this user is an administrator for:") }</th>
        <td class="important">
            ${ one_per_line(g.name for g in user.groups_user_is_admin_for) | n }
        </td>
    </tr>
    <tr>
        <th>${ _("Groups this user can see data from:") }</th>
        <td>
            ${ one_per_line(g.name for g in user.groups_user_may_see) | n }
        </td>
    </tr>
    <tr>
        <th>${ _("Groups this user can see all patients from, when no task filters are applied? (For other groups, only anonymous tasks will be shown if no patient filters are applied.)") }</th>
        <td>
            ${ one_per_line(g.name for g in user.groups_user_may_see_all_pts_when_unfiltered) | n }
        </td>
    </tr>
    <tr>
        <th>${ _("Groups this user can upload into:") }</th>
        <td>
            ${ one_per_line(g.name for g in user.groups_user_may_upload_into) | n }
        </td>
    </tr>
    <tr>
        <th>${ _("Groups this user may add special notes to tasks for:") }</th>
        <td>
            ${ one_per_line(g.name for g in user.groups_user_may_add_special_notes) | n }
        </td>
    </tr>
    <tr>
        <th>${ _("Groups this user can dump data from:") }</th>
        <td>
            ${ one_per_line(g.name for g in user.groups_user_may_dump) | n }
        </td>
    </tr>
    <tr>
        <th>${ _("Groups this user can run reports on:") }</th>
        <td>
            ${ one_per_line(g.name for g in user.groups_user_may_report_on) | n }
        </td>
    </tr>
    <tr>
        <th>${ _("Group this user is currently set to upload into:") }</th>
        <td>
            %if user.upload_group:
                ${ user.upload_group.name }
            %else:
                <i>${ _("(None)") }</i>
            %endif
        </td>
    </tr>
</table>

<h2>${ _("Detailed group membership information") }</h2>

<table>
    <tr>
        ## TRANSLATOR: keep short; table heading in user_info_detail.mako
        <th>${ _("Group name") }</th>
        ## TRANSLATOR: keep short; table heading in user_info_detail.mako
        <th>${ _("Group ID") }</th>
        ## TRANSLATOR: keep short; table heading in user_info_detail.mako
        <th>${ _("Group administrator?") }</th>
        ## TRANSLATOR: keep short; table heading in user_info_detail.mako
        <th>${ _("May upload?") }</th>
        ## TRANSLATOR: keep short; table heading in user_info_detail.mako
        <th>${ _("May register devices?") }</th>
        ## TRANSLATOR: keep short; table heading in user_info_detail.mako
        <th>${ _("May use webviewer?") }</th>
        ## TRANSLATOR: keep short; table heading in user_info_detail.mako
        <th>${ _("View all pts when unfiltered?") }</th>
        ## TRANSLATOR: keep short; table heading in user_info_detail.mako
        <th>${ _("May dump?") }</th>
        ## TRANSLATOR: keep short; table heading in user_info_detail.mako
        <th>${ _("May run reports?") }</th>
        ## TRANSLATOR: keep short; table heading in user_info_detail.mako
        <th>${ _("May add notes?") }</th>
        %if req.user.superuser or req.user.authorized_as_groupadmin:
            <th>${ _("Edit") }</th>
        %endif
    </tr>
    %for ugm in sorted(list(user.user_group_memberships), key=lambda ugm: ugm.group.name):
        <tr>
            <td>${ ugm.group.name }</td>
            <td>${ ugm.group_id }</td>
            <td ${ ('class="important"' if ugm.groupadmin else "") | n }>
                ${ get_yes_no(request, ugm.groupadmin) }
            </td>
            <td>${ get_yes_no(request, ugm.may_upload) }</td>
            <td>${ get_yes_no(request, ugm.may_register_devices) }</td>
            <td>${ get_yes_no(request, ugm.may_use_webviewer) }</td>
            <td>${ get_yes_no(request, ugm.view_all_patients_when_unfiltered) }</td>
            <td>${ get_yes_no(request, ugm.may_dump_data) }</td>
            <td>${ get_yes_no(request, ugm.may_run_reports) }</td>
            <td>${ get_yes_no(request, ugm.may_add_notes) }</td>
            %if req.user.superuser or ugm.group_id in req.user.ids_of_groups_user_is_admin_for:
                <td><a href="${ req.route_url(
                                    Routes.EDIT_USER_GROUP_MEMBERSHIP,
                                    _query={ViewParam.USER_GROUP_MEMBERSHIP_ID: ugm.id}
                                ) | n }">${ _("Edit") }</a></td>
            %endif
        </tr>
    %endfor
</table>
