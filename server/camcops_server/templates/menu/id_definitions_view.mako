## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/id_definitions_view.mako

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

<h1>${ _("Identification (ID) numbers") }</h1>

<table>
    <tr>
        <th>${ _("ID number") }</th>
        <th>${ _("Description") }</th>
        <th>${ _("Short description") }</th>
        <th>${ _("Validation method") }</th>
        <th>${ _("HL7 ID Type") }</th>
        <th>${ _("HL7 Assigning Authority") }</th>
        <th>${ _("Edit") }</th>
        <th>${ _("Delete") }</th>
    </tr>
    %for iddef in idnum_definitions:
        <tr>
            <td>${ iddef.which_idnum }</td>
            <td>${ iddef.description }</td>
            <td>${ iddef.short_description }</td>
            <td>${ iddef.validation_method or "" }</td>
            <td>${ iddef.hl7_id_type or "" }</td>
            <td>${ iddef.hl7_assigning_authority or "" }</td>
            <td><a href="${request.route_url(
                                Routes.EDIT_ID_DEFINITION,
                                _query={ViewParam.WHICH_IDNUM: iddef.which_idnum
                            }) | n }">${ _("Edit") }</a></td>
            <td><a href="${request.route_url(
                                Routes.DELETE_ID_DEFINITION,
                                _query={ViewParam.WHICH_IDNUM: iddef.which_idnum
                            }) | n }">${ _("Delete") }</a></td>
        </tr>
    %endfor
</table>

<a href="${ request.route_url(Routes.ADD_ID_DEFINITION) | n }">
    ${ _("Add new ID number definition") }</a>

<%include file="to_main_menu.mako"/>
