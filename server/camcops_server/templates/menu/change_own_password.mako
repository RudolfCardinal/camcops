## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/change_own_password.mako

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

<%inherit file="base_web_form.mako"/>

<%include file="db_user_info.mako"/>

%if expired:
    <div class="important">
        ${ _("Your password has expired and must be changed.") }
    </div>
%endif

<h1>${ _("Change your password") }</h1>

${form | n}

<div>
    ${ _("Choose strong passphrases.") }
    ${ _("See") }
    <a href="https://www.ncsc.gov.uk/blog-post/three-random-words-or-thinkrandom-0">
        https://www.ncsc.gov.uk/blog-post/three-random-words-or-thinkrandom-0</a>.
</div>
<div>
    ${ _("Minimum password length is {} characters.").format(min_pw_length) }
</div>
