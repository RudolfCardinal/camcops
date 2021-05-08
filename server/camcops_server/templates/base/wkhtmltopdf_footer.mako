## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/base/wkhtmltopdf_footer.mako

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

USED TO MAKE SEPARATE FOOTER HTML FILES FOR WKHTMLTOPDF.
WORKS IN CONJUNCTION WITH wkhtmltopdf_header.mako

</%doc>

<%inherit file="base.mako"/>

<%block name="css">
    <%include file="css_wkhtmltopdf.mako"/>
</%block>

<%block name="extra_head_start">
    ${ parent.extra_head_start() | n }

    <script nonce="${ request.nonce | n }">
        // Do not move this Javascript out into a file that's requested separately;
        // wkhtmltopdf will not be able to see it.

        // noinspection JSUnusedLocalSymbols
        function subst() {
            const vars = {};
            let x = document.location.search.substring(1).split('&'),
                    i,
                    z,
                    y,
                    j;
            for (i in x) {
                if (x.hasOwnProperty(i)) {
                    z = x[i].split('=', 2);
                    vars[z[0]] = decodeURI(z[1]);  // decodeURI() replaces unescape()
                }
            }
            x = ['frompage', 'topage', 'page', 'webpage', 'section',
                 'subsection','subsubsection'];
            for (i in x) {
                if (x.hasOwnProperty(i)) {
                    y = document.getElementsByClassName(x[i]);
                    for (j = 0; j < y.length; ++j) {
                        y[j].textContent = vars[x[i]];
                    }
                }
            }
        }

        document.addEventListener("DOMContentLoaded", subst, false);
    </script>
</%block>

<div>
    ${ _("Page") } <span class="page"></span> ${ _("of") } <span class="topage"></span>.
    ${ inner_text | n }
</div>
