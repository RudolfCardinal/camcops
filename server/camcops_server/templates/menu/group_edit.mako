## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/menu/group_edit.mako

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

<h1>${ _("Edit group") } ${ object.name }</h1>

${ form | n }

<div class="footnotes">
    ${ _("Policies define the minimum amount of information about a patient "
         "that must be present before records can be <i>uploaded</i> to the "
         "server or <i>finalized</i> (uploaded but also cleared off the "
         "source device).") | n }<br>

    ## TRANSLATOR: group_edit.mako, advice to user about policies
    ${ _("Policies are specified using") }
    <ul>
        ## TRANSLATOR: group_edit.mako, advice to user about policies
        <li>${ _("these logical operators:") }
            <code>AND</code>, <code>OR</code>, <code>NOT</code>;</li>

        ## TRANSLATOR: group_edit.mako, advice to user about policies
        <li>${ _("these precedence operators:") }
            <code>(</code>, <code>)</code>;</li>

        ## TRANSLATOR: group_edit.mako, advice to user about policies
        <li>${ _("these named fields:") }
            <code>forename</code>, <code>surname</code>,
            <code>sex</code>, <code>dob</code> (date of birth),
            <code>email</code>, <code>address</code>,
            <code>gp</code>, <code>otherdetails</code></li>

        ## TRANSLATOR: group_edit.mako, advice to user about policies
        <li>${ _("a specific ID number using") } “idnum<i>n</i>”;
            ## TRANSLATOR: group_edit.mako, advice to user about policies
            ${ _("for example, <code>idnum3</code> refers to ID number 3 as "
                 "defined by the server’s") | n }
            <a href="${ req.route_url(Routes.VIEW_SERVER_INFO) | n }">
                ${ _("ID numbers") }</a>;</li>

        ## TRANSLATOR: group_edit.mako, advice to user about policies
        <li>${ _("the token <code>anyidnum</code> as a shorthand to mean "
                 "“at least one of the server’s ID numbers is present”;") | n }</li>

        ## TRANSLATOR: group_edit.mako, advice to user about policies
        <li>${ _("the token <code>otheridnum</code> as a shorthand to mean "
                 "“at least one of the server’s ID numbers that is not "
                 "named in the policy”;") | n }</li>
    </ul>

    ## TRANSLATOR: group_edit.mako, advice to user about policies
    ${ _("The policies are case-insensitive.") }<br>

    ## TRANSLATOR: group_edit.mako, advice to user about policies
    ${ _("Examples:") }
    <ul>
        <li>
            ## TRANSLATOR: group_edit.mako, advice to user about policies
            ${ _("A liaison psychiatry department might operate in a mental "
                 "health environment using idnum1, and an acute hospital "
                 "environment using idnum2. It might want to allow uploads "
                 "using either ID number, for quick access to the CamCOPS "
                 "server version from the acute hospital environment, but "
                 "require idnum1 to be present before the data can be "
                 "finalized. It also wants full identification with "
                 "forename, surname, DOB, and sex. It could use an upload "
                 "policy of <code>forename AND surname AND dob AND sex AND "
                 "(idnum1 OR idnum2)</code> and a finalize policy of "
                 "<code>forename AND surname AND dob AND sex AND idnum1</code>.") | n }
        </li>
        <li>
            ## TRANSLATOR: group_edit.mako, advice to user about policies
            ${ _("In contrast, a research environment might want no "
                 "patient-identifying information, and a single research "
                 "pseudonym for that study. It might use an upload and a "
                 "finalize policy that are both <code>sex AND idnum1</code>.") | n }
        </li>
    </ul>
</div>

<%include file="to_view_all_groups.mako"/>
<%include file="to_main_menu.mako"/>
