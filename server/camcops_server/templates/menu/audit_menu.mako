## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/audit_menu.mako

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
from camcops_server.cc_modules.cc_pyramid import Routes
%>

<h2>${ _("Audit options") }</h2>

<h3>${ _("Access logs") }</h3>
<ul>
    <li><a href="${ request.route_url(Routes.OFFER_AUDIT_TRAIL) | n }">
        ${ _("Audit trail") }</a></li>
</ul>

<h3>${ _("Export logs") }</h3>
<ul>
    <li><a href="${ request.route_url(Routes.OFFER_EXPORTED_TASK_LIST) | n }">
        ${ _("Exported task log") }</a></li>
</ul>
