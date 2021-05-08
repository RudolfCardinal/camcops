## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/users_view.mako

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
%>

<%include file="db_user_info.mako"/>

<h1>${ _("Users") }</h1>

<div>${ page.pager() | n }</div>

${form | n}

<table>
    <tr>
        <th>${ _("Username") }</th>
        <th>${ _("ID") }</th>
        <th>${ _("Flags") }</th>
        <th>${ _("Full name") }</th>
        <th>${ _("Email") }</th>
        <th>${ _("View details") }</th>
        <th>${ _("Edit") }</th>
        <th>${ _("Groups") }</th>
        <th>${ _("Upload group") }</th>
        <th>${ _("Change password") }</th>
        <th>${ _("Delete") }</th>
    </tr>
    %for user in page:
        <tr>
            <td>${ user.username }</td>
            <td>${ user.id }</td>
            <td>
                %if user.superuser:
                    <span class="important">${ _("Superuser.") }</span>
                %endif
                %if user.is_a_groupadmin:
                    <span class="important">${ _("Group administrator") }
                        (${ user.names_of_groups_user_is_admin_for_csv }).</span>
                %endif
                %if user.is_locked_out(request):
                    <span class="warning">${ _("Locked out;") }
                        <a href="${ req.route_url(
                                        Routes.UNLOCK_USER,
                                        _query={ViewParam.USER_ID: user.id}
                                    ) | n }">${ _("unlock") }</a>.</span>
                %endif
                %if user.auto_generated:
                    <span>${ _("Auto-generated") }</span>
                %endif
            </td>
            <td>${ user.fullname or "" }</td>
            <td>${ user.email or "" }</td>
            <td><a href="${ req.route_url(
                                Routes.VIEW_USER,
                                _query={ViewParam.USER_ID: user.id}
                            ) | n }">${ _("View") }</a></td>
            <td><a href="${ req.route_url(
                                Routes.EDIT_USER,
                                _query={ViewParam.USER_ID: user.id}
                            ) | n }">${ _("Edit") }</a></td>
            <td>
                %for i, ugm in enumerate(sorted(list(user.user_group_memberships), key=lambda ugm: ugm.group.name)):
                    %if i > 0:
                        <br>
                    %endif
                    ${ ugm.group.name }
                    %if req.user.may_administer_group(ugm.group_id):
                        [<a href="${ req.route_url(
                                        Routes.EDIT_USER_GROUP_MEMBERSHIP,
                                        _query={ViewParam.USER_GROUP_MEMBERSHIP_ID: ugm.id}
                                    ) | n }">${ _("Permissions") }</a>]
                    %endif
                %endfor
            </td>
            <td>
                ${ (escape(user.upload_group.name) if user.upload_group
                    else "<i>(None)</i>") | n }
                [<a href="${request.route_url(
                                Routes.SET_OTHER_USER_UPLOAD_GROUP,
                                _query={ViewParam.USER_ID: user.id}
                            ) | n }">${ _("change") }</a>]
            </td>
            <td><a href="${ req.route_url(
                                Routes.CHANGE_OTHER_PASSWORD,
                                _query={ViewParam.USER_ID: user.id}
                            ) | n }">${ _("Change password") }</a></td>
            <td><a href="${ req.route_url(
                                Routes.DELETE_USER,
                                _query={ViewParam.USER_ID: user.id}
                            ) | n }">${ _("Delete") }</a></td>
        </tr>
    %endfor
</table>

<div>${ page.pager() | n }</div>

<td><a href="${ req.route_url(Routes.ADD_USER) | n }">${ _("Add a user") }</a></td>

<%include file="to_main_menu.mako"/>
