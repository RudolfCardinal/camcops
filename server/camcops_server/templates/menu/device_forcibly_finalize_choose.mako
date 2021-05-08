## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/device_forcibly_finalize_choose.mako

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

<h1>${ _("Forcibly finalize a device") }</h1>

<h2>${ _("Step 1: choose a device") }</h2>

<div class="important">
    ${ _(
        "This process marks all records from a particular device "
        "(e.g. tablet, or desktop client) as final, so the device can no "
        "longer alter them. If you do this and the client re-uploads "
        "records, they will be created as fresh tasks, so only "
        "force-finalize devices that are no longer in use and to which you "
        "no longer have access.") }
</div>

${ form | n }

<%include file="to_main_menu.mako"/>
