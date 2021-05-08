## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/taskcommon/tracker_ctv.mako

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

## <%page args="tracker: Tracker, pdf_landscape: bool"/>

<%!

from markupsafe import escape
from cardinal_pythonlib.datetimefunc import format_datetime
from camcops_server.cc_modules.cc_constants import CSS_PAGED_MEDIA, DateFormat
from camcops_server.cc_modules.cc_pyramid import Routes, ViewArg, ViewParam
from camcops_server.cc_modules.cc_tracker import format_daterange
from camcops_server.cc_modules.cc_version_string import CAMCOPS_SERVER_VERSION_STRING

def inherit_file(context):
    viewtype = context['viewtype']
    if viewtype == ViewArg.HTML:
        return "base_web.mako"
    elif viewtype == ViewArg.PDF:
        if CSS_PAGED_MEDIA:
            pdf_landscape = context['pdf_landscape']
            if pdf_landscape:
                return "base_pdf_landscape.mako"
            else:
                return "base_pdf_portrait.mako"
        else:
            return "base_pdf_no_paged_media.mako"
    else:
        raise ValueError("This template is only for HTML/PDF views")

%>

<%inherit file="${ inherit_file(context) }"/>
## ... don't use "| n" for that.

## ============================================================================
## For CSS paged media, extra headers
## ============================================================================

%if CSS_PAGED_MEDIA and viewtype == ViewArg.PDF:
    <%block name="extra_header_content">
        <%include file="tracker_ctv_header.mako" args="tracker=tracker"/>
    </%block>
    <%block name="extra_footer_content">
        <%include file="tracker_ctv_footer.mako" args="tracker=tracker"/>
    </%block>
%endif
## For non-paged media (i.e. wkhtmltopdf), the headers/footers are made separately.

## ============================================================================
## Header for tracker/CTV, including patient ID information
## ============================================================================

<div class="trackerheader">
    ${ _("Patient identified by:") }
    <b>${ ("; ".join(x.description(request)
           for x in tracker.taskfilter.idnum_criteria) + ".") }</b>
    ${ _("Date range for search:") }
    <b>${ format_daterange(tracker.taskfilter.start_datetime,
                           tracker.taskfilter.end_datetime) }</b>.
    ${ _("The tracker information will <b>only be valid</b> (i.e. will only "
         "be from only one patient!) if all contributing tablet devices use "
         "these identifiers consistently. The consistency check is below. "
         "The patient information shown below is taken from the first task "
         "used.") | n }
</div>

## Consistency
<%
    cons = tracker.consistency_info.get_description_list()
    if tracker.consistency_info.are_all_consistent():
        cons_class = "tracker_all_consistent"
        joiner = ". "
    else:
        cons_class = "warning"
        joiner = "<br>"
    consistency = joiner.join(escape(c) for c in cons)
%>
<div class="${ cons_class | n }">
    ${ consistency | n }
</div>

## Patient
%if tracker.patient:
    <%include file="patient.mako" args="patient=tracker.patient, viewtype=viewtype"/>
%else:
    <div class="warning">
        ${ _("No patient found, or the patient has no relevant tasks in the """
             "time period requested.") }
    </div>
%endif

## ============================================================================
## Main bit
## ============================================================================

${ next.body() | n }

## ============================================================================
## Office stuff
## ============================================================================

<div class="office">
    <%block name="office_preamble"/>

    ${ _("Requested tasks:") }
        ${ (", ".join(tracker.taskfilter.task_tablename_list)
            if tracker.taskfilter.task_classes else "None") }.
    ${ _("Sources (tablename, task server PK, patient server PK):") }
        ${ tracker.summary }.
    ${ _("Information retrieved from") }
        ${ request.application_url }
        (${ _("server version") } ${ CAMCOPS_SERVER_VERSION_STRING })
        ${ _("at:") }
        ${ format_datetime(request.now, DateFormat.SHORT_DATETIME_SECONDS) }.
</div>

## ============================================================================
## Navigation links, if applicable
## ============================================================================

%if viewtype == ViewArg.HTML:
    ## The XML version is available from the configuration view.
    ## Users might appreciate a direct shortcut to the PDF, though.
    <div class="navigation">
        ## Link to PDF version
        <a href="${ req.route_url(
                        Routes.CTV if tracker.as_ctv else Routes.TRACKER,
                        _query={
                            ViewParam.WHICH_IDNUM: tracker.taskfilter.idnum_criteria[0].which_idnum,
                            ViewParam.IDNUM_VALUE: tracker.taskfilter.idnum_criteria[0].idnum_value,
                            ViewParam.START_DATETIME: tracker.taskfilter.start_datetime,
                            ViewParam.END_DATETIME: tracker.taskfilter.end_datetime,
                            ViewParam.TASKS: tracker.taskfilter.task_tablename_list,
                            ViewParam.VIEWTYPE: ViewArg.PDF,
                        }) | n }">${ _("View PDF for printing/saving") }</a>
    </div>
%endif
