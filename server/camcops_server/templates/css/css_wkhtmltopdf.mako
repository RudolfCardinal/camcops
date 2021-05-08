## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/css/css_wkhtmltopdf.mako

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

<%namespace file="def_css_constants.mako" import="_get_css_varargs"/>
<%

# Note the important difference between <% %> and <%! %>
# http://docs.makotemplates.org/en/latest/syntax.html#python-blocks
# http://docs.makotemplates.org/en/latest/syntax.html#module-level-blocks

va = _get_css_varargs("wkhtmltopdf_header_footer")

%>

<%page expression_filter="n"/>
## ... everything here is trusted.

body {
    font-family: Arial, Helvetica, sans-serif;
    font-size: ${va.MAINFONTSIZE};  /* absolute */
    line-height: ${va.SMALLLINEHEIGHT};
    padding: 0;
    margin: 0;  /* use header-spacing / footer-spacing instead */
}
div {
    font-size: ${va.SMALLFONTSIZE};  /* relative */
}

## http://stackoverflow.com/questions/11447672/fix-wkhtmltopdf-headers-clipping-content  # noqa
