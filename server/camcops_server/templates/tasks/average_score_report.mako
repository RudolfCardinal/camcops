## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/tasks/average_score_report.mako

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

## <%page args="title: str, report_id: str, mainpage: TsvPage, datepage: TsvPage"/>

<%inherit file="report.mako"/>

<%!
from camcops_server.cc_modules.cc_pyramid import Routes, ViewArg, ViewParam
%>

<%block name="results">

    <div>
        Only complete tasks are considered. If date filters are applied, only
        tasks within the date range are considered.
    </div>

    <%include file="table.mako" args="column_headings=mainpage.headings, rows=mainpage.plainrows"/>

    <div>
        If date filters are applied, only tasks within the date range are
        considered.
    </div>

    <%include file="table.mako" args="column_headings=datepage.headings, rows=datepage.plainrows"/>

</%block>
