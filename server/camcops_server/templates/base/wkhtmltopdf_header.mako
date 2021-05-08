## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/base/wkhtmltopdf_header.mako

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

USED TO MAKE SEPARATE HEADER HTML FILES FOR WKHTMLTOPDF.
WORKS IN CONJUNCTION WITH wkhtmltopdf_footer.mako

</%doc>

<%inherit file="base.mako"/>

<%block name="css">
    <%include file="css_wkhtmltopdf.mako"/>
</%block>

<%block name="extra_head_start">
    ${ parent.extra_head_start() | n }

    <script nonce="${ request.nonce | n }">
        document.addEventListener("DOMContentLoaded", subst, false);
        // the function itself is defined in wkhtmltopdf_footer.mako
    </script>
</%block>

<div>
    ${ inner_text | n }
</div>
