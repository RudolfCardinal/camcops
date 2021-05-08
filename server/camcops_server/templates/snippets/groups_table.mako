## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/snippets/groups_table.mako

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

<%page args="groups_page, valid_which_idnums, with_edit: bool"/>
<%!
from camcops_server.cc_modules.cc_pyramid import Routes, ViewArg, ViewParam
from markupsafe import escape
%>
<%namespace file="displayfunc.mako" import="one_per_line"/>

<div>${ groups_page.pager(show_if_single_page=False) | n }</div>

<table>
    <tr>
        <th>${ _("Group name") }</th>
        <th>${ _("Group ID") }</th>
        <th>${ _("Description") }</th>
        <th>${ _("Groups this group is allowed to see, in addition to itself") }</th>
        <th>${ _("Upload ID policy") }</th>
        <th>${ _("Principal (single necessary) ID number required by Upload policy") }</th>
        <th>${ _("Finalize ID policy") }</th>
        <th>${ _("Principal (single necessary) ID number required by Finalize policy") }</th>
        <th>${ _("Members") }</th>
        %if with_edit:
            <th>${ _("Edit") }</th>
            <th>${ _("Delete") }</th>
        %endif
    </tr>
    %for group in groups_page:
        <%
            upload_tk = group.tokenized_upload_policy()  # type: TokenizedPolicy
            finalize_tk = group.tokenized_finalize_policy()  # type: TokenizedPolicy
            upload_valid = upload_tk.is_valid(valid_which_idnums)
            finalize_valid = finalize_tk.is_valid(valid_which_idnums)
            critical_upload_id = upload_tk.find_critical_single_numerical_id()
            critical_finalize_id = finalize_tk.find_critical_single_numerical_id()
            users = list(group.regular_users)
        %>
        <tr>
            <td>${ group.name }</td>
            <td>${ group.id }</td>
            <td>${ group.description or "" }</td>
            <td>
                ${ one_per_line(g.name for g in group.can_see_other_groups) | n }
            </td>

            <td
                %if not upload_valid:
                    class="warning"
                %endif
                >
                ${ (escape(group.upload_policy) if group.upload_policy else "<i>None</i>") | n }
            </td>

            <td>${ critical_upload_id }</td>

            <td
                %if not finalize_valid:
                    class="warning"
                %endif
                >
                ${ (escape(group.finalize_policy) if group.finalize_policy else "<i>None</i>") | n }
            </td>

            <td>${ critical_finalize_id }</td>

            <td>
                ${ (", ".join(sorted(u.username if u is not None else "<DATA_ERROR_NULL_USER>"
                                     for u in users))) }
            </td>

            %if with_edit:
                <td><a href="${ req.route_url(
                                    Routes.EDIT_GROUP,
                                    _query={ViewParam.GROUP_ID: group.id}
                                ) | n }">${ _("Edit") }</a></td>
                <td><a href="${ req.route_url(
                                    Routes.DELETE_GROUP,
                                    _query={ViewParam.GROUP_ID: group.id}
                                ) | n }">${ _("Delete") }</a></td>
            %endif
        </tr>
    %endfor
</table>

<div>${ groups_page.pager(show_if_single_page=False) | n }</div>

<div class="footnotes">
    ${ _("Colour in a policy column means that an ID policy is not valid "
         "(syntactically, because it refers to ID numbers that do not exist, "
         "or because it's less restrictive than the tablet's minimum ID policy).") }
</div>
