## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/tasks/apeq_cpft_perinatal_report.mako

===============================================================================

    Copyright (C) 2012-2019 Rudolf Cardinal (rudolf@pobox.com).

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
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.

===============================================================================

</%doc>

<%inherit file="base_web.mako"/>
<%block name="css">
${parent.css()}

h2, h3 {
    margin-top: 20px;
}

.table-cell {
    text-align: right;
}

.table-cell.col-0 {
    text-align: initial;
}

.ff-why-table > tbody > tr > .col-1 {
    text-align: initial;
}
</%block>

<%!
from camcops_server.cc_modules.cc_pyramid import Routes, ViewParam
%>


<%include file="db_user_info.mako"/>

<h1>${ title | h }</h1>

<h2>${_("Main questions")}</h2>

<%include file="table.mako" args="column_headings=main_column_headings, rows=main_rows"/>

<h2>${_("Friends / family questions")}</h2>

<%include file="table.mako" args="column_headings=ff_column_headings, rows=ff_rows"/>

<h3>${_("Reasons given for the above responses")}</h3>

<%include file="table.mako" args="column_headings=[], rows=ff_why_rows, table_class='ff-why-table'"/>

<h2>${_("Comments")}</h2>
%for comment in comments:
   <blockquote>
       <p>${comment | h}</p>
   </blockquote>
%endfor
<div>
    <a href="${ request.route_url(Routes.OFFER_REPORT, _query={ViewParam.REPORT_ID: report_id}) }">${_("Re-configure report")}</a>
</div>
<div>
    <a href="${request.route_url(Routes.REPORTS_MENU)}">${_("Return to reports menu")}</a>
</div>
<%include file="to_main_menu.mako"/>