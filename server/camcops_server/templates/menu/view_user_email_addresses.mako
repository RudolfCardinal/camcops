## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/view_user_email_addresses.mako

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
from markupsafe import escape
from camcops_server.cc_modules.cc_pyramid import Routes, ViewArg, ViewParam
%>

<%include file="db_user_info.mako"/>

<h1>${ _("Users’ e-mail addresses") }</h1>

<h2>${ _("Users that you manage who have missing e-mail addresses") }</h2>
<ul>
    %for user in query:
        %if not user.email:
            <li>${ user.username }</li>
        %endif
    %endfor
</ul>

<h2>${ _("E-mail addresses for users that you manage") }</h2>
<ul>
    <li>${ _("Click one to e-mail that user using your computer's e-mail client.") }</li>
    <li>${ _("Copy/paste the whole list into your e-mail client for a bulk e-mail.") }</li>
</ul>
<p>
%for user in query:
    %if user.email:
        <a href="mailto:${ user.email | n }">
            ${ user.fullname or user.username } &lt;${ user.email }&gt;</a><br>
    %endif
%endfor
</p>

<!-- There is no general way to do a "mailto:" link for multiple recipients;
see https://stackoverflow.com/questions/13765286/send-mail-to-multiple-receiver-with-html-mailto -->
