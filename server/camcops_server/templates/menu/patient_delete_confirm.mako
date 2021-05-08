## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/patient_delete_confirm.mako

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

<h1>${ _("Delete patient entirely: FINAL STEP") }</h1>

<div class="warning">
    ${ _("This operation is irrevocable!") }
</div>
<div class="warning">
    ${ _("IT WILL PERMANENTLY DELETE THE PATIENT AND ALL ASSOCIATED TASKS "
         "from the group that you specify.") }
</div>

<div class="important">
    ${ n_patient_instances }
    ${ _("patient records (current and/or old) will be deleted.") }
</div>

<div class="warning">
    ${ _("This is the final step. ARE YOU SURE YOU WANT TO DELETE THE "
         "PATIENT AND ALL THE FOLLOWING TASKS?") }
</div>

<%include file="view_tasks_table.mako" args="tasks=tasks"/>

<div class="important">
    ${ len(tasks) } ${ _("tasks will be deleted.") }
</div>

<h1>${ _("Proceed to deletion?") }</h1>

${ form | n }

<%include file="to_main_menu.mako"/>
