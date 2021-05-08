## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/dump_sql_offer.mako

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

<%!
from camcops_server.cc_modules.cc_pyramid import Routes, ViewArg, ViewParam
%>

<%include file="db_user_info.mako"/>

<h1>${ _("Research data dump in SQL format") }</h1>

<h2>${ _("Explanation") }</h2>
<div>
    <ul>
        <li>
            ${ _("This research dump takes some subset of data to which you "
                 "have access, and builds a new database from it. That "
                 "database is served to you in SQLite format; you can choose "
                 "binary or SQL format.") }
        </li>
        <li>
            ${ _("The records are restricted to ‘current’ tasks, and some "
                 "irrelevant, administrative, and security-related columns "
                 "are removed.") }
        </li>
        <li>
            ${ _("Summary information (such as total scores) is automatically "
                 "added, for convenience.") }
        </li>
        <li>${ _("Foreign key constraints are removed.") }</li>
        <li>${ _("You can load and explore the binary database like this:") }
            <pre>$ sqlite3 CamCOPS_dump_SOME_DATE.sqlite3
sqlite> .tables
sqlite> pragma table_info('patient');
sqlite> select * from patient;</pre>
        </li>
        <li>${ _("The SQLite format is widely supported; see, for example, the") }
            <a href="https://cran.r-project.org/web/packages/RSQLite/index.html">
                RSQLite</a> ${ _("package for") }
            <a href="https://www.r-project.org/">R</a>.
        </li>
    </ul>
</div>

<h2>${ _("Choose SQL dump parameters") }</h2>

${ form | n }

<%include file="to_main_menu.mako"/>
