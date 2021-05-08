## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/forbidden.mako

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

<%inherit file="base_web.mako"/>

<%!
from camcops_server.cc_modules.cc_pyramid import Routes, ViewArg, ViewParam
%>

<%include file="db_user_info.mako"/>

<div class="error">
    %if request.user_id is None:
        <!--
            Not logged in.

            (OR ANOTHER POSSIBILITY: CamCOPS being offered over HTTP, which
            will cause cookies not to be saved, because they're marked to the
            client as being HTTPS-only cookies by default.)
       -->
        ${ _("You are not logged in (or your session has timed out).") }
        <div>
            Click
            <a href="${request.route_url(Routes.LOGIN, _query=querydict) | n }">
                here</a> to log in.
        </div>
    %else:
        <!-- Logged in, but permission denied. -->
        ${ _("You do not have permission to view this page. "
             "It may be restricted to administrators.") }
        <%include file="to_main_menu.mako"/>
    %endif
</div>
