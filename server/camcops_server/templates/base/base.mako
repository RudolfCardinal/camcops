## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/base/base.mako

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

<!DOCTYPE html> <!-- HTML 5 -->
<html lang="en">
    <head>
        <%block name="head">
            <%block name="title">
                <title>CamCOPS</title>
            </%block>
            <meta charset="utf-8">
            <%block name="extra_head_start"></%block>
            <link rel="icon" type="image/png" href="${ request.url_camcops_favicon | n }">
            <script nonce="${ request.nonce | n }">
                /* set "html.svg" if our browser supports SVG */
                // noinspection JSDeprecatedSymbols
                if (document.implementation.hasFeature(
                        "https://www.w3.org/TR/SVG11/feature#Image", "1.1")) {
                    document.documentElement.className = "svg";
                }
            </script>
            <style nonce="${ request.nonce | n }">
                <%block name="css"></%block>
            </style>
            <%block name="extra_head_end"></%block>
        </%block>
    </head>
    <body <%block name="body_tags"></%block>>
        <%block name="header_block"></%block>
        <%block name="footer_block"></%block>
        ## ... for CSS paged media
        <%block name="logo"></%block>

        ${ next.body() | n }

        <%block name="body_end"></%block>
    </body>
</html>
