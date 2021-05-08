## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/view_ddl_choose_dialect.mako

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

<h1>${ _("View data definition language (DDL), in SQL") }</h1>

<div>
    ${ _("The server’s database is using dialect") }
    <code>${ current_dialect | n }</code> (${ current_dialect_description }).
</div>

${ form | n }

<%include file="to_main_menu.mako"/>
