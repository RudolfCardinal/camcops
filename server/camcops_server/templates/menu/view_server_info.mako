## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/view_server_info.mako

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

<%include file="db_user_info.mako"/>

<h1>${ _("CamCOPS: information about this database/server") }</h1>

<h2>${ _("Identification (ID) numbers") }</h2>
<table>
    <tr>
        <th>${ _("ID number") }</th>
        <th>${ _("Description") }</th>
        <th>${ _("Short description") }</th>
    </tr>
    %for iddef in idnum_definitions:
        <tr>
            <td>${ iddef.which_idnum }</td>
            <td>${ iddef.description }</td>
            <td>${ iddef.short_description }</td>
        </tr>
    %endfor
</table>

<h2>${ _("Recent activity") }</h2>
<table>
    <tr>
        <th>${ _("Time-scale") }</th>
        <th>${ _("Number of active sessions") }</th>
    </tr>
    %for k, v in recent_activity.items():
        <tr>
            <td>${ k }</td>
            <td>${ v }</td>
        </tr>
    %endfor
</table>
<p>
    ${ _("Sessions time out after") } ${ session_timeout_minutes }
    ${ _("minutes; sessions older than this are periodically deleted.") }
</p>

<h2>${ _("Extra string families present") }</h2>
%for sf in string_families:
    ${ sf  }
    %if sf in restricted_tasks:
        <b>— restricted to groups: ${ ", ".join(restricted_tasks[sf]) }</b>
    %endif
    <br>

%endfor

<h2>${ _("All known tasks") }</h2>
<p>${ _("Format is: long name (short name; base table name).") }</p>
<pre>
    %for tc in all_task_classes:
${ tc.longname(req) } (${ tc.shortname }; ${ tc.tablename })
    %endfor
</pre>

<%include file="to_main_menu.mako"/>
