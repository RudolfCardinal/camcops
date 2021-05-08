## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/tasks/diagnosis_finder_report.mako

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

<%block name="additional_report_above_results">
    <h2>${ _("Parameters:") }</h2>
    <div>
        ${ _("Which ID number type:") } ${idnum_desc }.<br>
        ${ _("Inclusion diagnoses:") } ${inclusion_dx }.<br>
        ${ _("Exclusion diagnoses:") } ${exclusion_dx }.<br>
        ${ _("Minimum age:") } ${age_minimum }.<br>
        ${ _("Maximum age:") } ${age_maximum }.
    </div>
    <h2>${ _("Results:") }</h2>
</%block>

<%block name="additional_report_below_menu">
    <h2>SQL:</h2>
    <div>
        ## no escaping required:
        <code>${sql | n }</code>
    </div>
</%block>
