## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/special_note_delete.mako

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

<%inherit file="base_web_form.mako"/>

<%include file="db_user_info.mako"/>

<h1>${ _("Delete special note?") }</h1>

<div>
    ${ _("You are about to delete this note:") }
</div>

<table>
    <tr><th>note_id</th><td>${ sn.note_id }</td></tr>
    <tr><th>basetable</th><td>${ sn.basetable }</td></tr>
    <tr><th>task_id</th><td>${ sn.task_id }</td></tr>
    <tr><th>device_id</th><td>${ sn.device_id }</td></tr>
    <tr><th>era</th><td>${ sn.era }</td></tr>
    <tr><th>note_at</th><td>${ sn.note_at }</td></tr>
    <tr><th>user_id</th><td>${ sn.user_id }</td></tr>
    <tr><th>note</th><td>${ sn.note }</td></tr>
</table>

<p><i>${ _("The special note will vanish (though preserved in the database "
           "for auditing).") }</i></p>

${ form | n }

<%include file="to_main_menu.mako"/>
