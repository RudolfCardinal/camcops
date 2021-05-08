## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/report.mako

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

## <%page args="title: str, report_id: str, column_names: List[str], page: CamcopsPage"/>
<%inherit file="base_web.mako"/>

<%!
from camcops_server.cc_modules.cc_pyramid import Routes, ViewArg, ViewParam
%>

<%include file="db_user_info.mako"/>

<h1>${ title }</h1>

<%block name="results">

    <%block name="additional_report_above_results"></%block>

    <%block name="pager_above_results">
        <div>${ page.pager() | n }</div>
    </%block>

    <%block name="table">
        <%include file="table.mako" args="column_headings=column_names, rows=page"/>
    </%block>

    <%block name="pager_below_results">
        <div>${ page.pager() | n }</div>
    </%block>

    <%block name="additional_report_below_results"></%block>

</%block>

<div>
    <a href="${ request.route_url(
                    Routes.OFFER_REPORT,
                    _query={ViewParam.REPORT_ID: report_id}
                ) | n }">${ _("Re-configure report") }</a>
</div>
<div>
    <a href="${ request.route_url(Routes.REPORTS_MENU) | n }">
        ${ _("Return to reports menu") }</a>
</div>
<%include file="to_main_menu.mako"/>

<%block name="additional_report_below_menu"></%block>
