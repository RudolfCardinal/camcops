## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/tasks/apeq_cpft_perinatal_report.mako

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

<%inherit file="report.mako"/>

<%block name="css">
${ parent.css() | n }

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

<%block name="results">

    <p>
        %if start_datetime:
            ${ _("Created" ) } <b>&ge; ${ start_datetime }</b>.
        %endif
        %if end_datetime:
            ${ _("Created") } <b>&lt; ${ end_datetime }</b>.
        %endif
    </p>

    <h2>${ _("Main questions") }</h2>

    <%include file="table.mako" args="column_headings=main_column_headings, rows=main_rows"/>

    <h2>${ _("Friends and family question") }</h2>

    <%include file="table.mako" args="column_headings=ff_column_headings, rows=ff_rows"/>

    <h3>${ _("Reasons given for the above responses") }</h3>

    <%include file="table.mako" args="column_headings=[], rows=ff_why_rows, table_class='ff-why-table'"/>

    <h2>${ _("Comments") }</h2>
    %for comment in comments:
       <blockquote>
           <p>${ comment }</p>
       </blockquote>
    %endfor

</%block>
