## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/base/base_pdf.mako

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

<%inherit file="base.mako"/>
<%namespace file="def_css_constants.mako" import="_get_css_varargs"/>

<%!
# import logging
from camcops_server.cc_modules.cc_constants import PDF_ENGINE
# log = logging.getLogger(__name__)
%>

## For CSS paged media:
<%block name="header_block">
    <div id="headerContent">
        <%block name="extra_header_content"/>
    </div>
</%block>

## For CSS paged media:
<%block name="footer_block">
    <div id="footerContent">
        ${ _("Page") } <pdf:pagenumber/> ${ _("of") } <pdf:pagecount/>.
        <%block name="extra_footer_content"/>
    </div>
</%block>

<%block name="logo">

    <%
    va = _get_css_varargs("pdf_portrait")
    # ... exact parameter doesn't matter; we only want PDF_LOGO_HEIGHT.
    %>

    %if PDF_ENGINE in ["pdfkit", "weasyprint"]:
        ## weasyprint: div with floating img does not work properly
        <div class="pdf_logo_header">
            <table>
                <tr>
                    <td class="image_td">
                        <img class="logo_left"
                             src="file://${ request.config.camcops_logo_file_absolute | n }"
                             alt="CamCOPS logo" />
                    </td>
                    <td class="centregap_td"></td>
                    <td class="image_td">
                        <img class="logo_right"
                             src="file://${ request.config.local_logo_file_absolute | n }"
                             alt="Local institutional logo" />
                    </td>
                </tr>
            </table>
        </div>

    %elif PDF_ENGINE in ["xhtml2pdf"]:
        ## xhtml2pdf: hard to get logos positioned any other way than within a table
        <div class="header">
            <table class="noborder">
                <tr class="noborder">
                    <td class="noborderphoto" style="width:45%">
                        <img src="file://${ request.config.camcops_logo_file_absolute | n }"
                             height="${ va.PDF_LOGO_HEIGHT | n }"
                             style="float:left"
                             alt="CamCOPS logo" />
                    </td>
                    <td class="noborderphoto" style="width:10%"></td>
                    <td class="noborderphoto" style="width:45%">
                        <img src="file://${ request.config.local_logo_file_absolute | n }"
                             height="${ va.PDF_LOGO_HEIGHT | n }"
                             style="float:right"
                             alt="Local institutional logo" />
                    </td>
                </tr>
            </table>
        </div>
    %else:
        MISSING_PDF_LOGO_BLOCK_UNKNOWN_ENGINE
    %endif

</%block>

${ next.body() | n }
