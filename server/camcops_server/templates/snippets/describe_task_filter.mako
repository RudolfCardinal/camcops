## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/snippets/current_session_filters.mako

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

<%page args="task_filter: TaskFilter"/>

<%!

from cardinal_pythonlib.datetimefunc import format_datetime
from markupsafe import escape
from camcops_server.cc_modules.cc_constants import DateFormat

%>

<%
    some_filter = False
%>

<div class="filters">
    ## Task types
    %if task_filter.task_types:
        ${ _("Task is one of:") } <b>${ ", ".join(task_filter.task_types) }</b>.
        <% some_filter = True %>
    %endif
    ## Patient
    %if task_filter.surname:
        ${ _("Surname") } = <b>${ task_filter.surnameh }</b>.
        <% some_filter = True %>
    %endif
    %if task_filter.forename:
        ${ _("Forename") } = <b>${ task_filter.forename }</b>.
        <% some_filter = True %>
    %endif
    %if task_filter.dob:
        ${ _("DOB") } = <b>${ format_datetime(task_filter.dob, DateFormat.SHORT_DATE) }</b>.
        <% some_filter = True %>
    %endif
    %if task_filter.sex:
        ${ _("Sex") } = <b>${ task_filter.sex }</b>.
        <% some_filter = True %>
    %endif
    %if task_filter.idnum_criteria:
        ${ _("ID numbers match one of:") }
        ${ ("; ".join("{which} = <b>{value}</b>".format(
                which=escape(request.get_id_shortdesc(iddef.which_idnum)),
                value=escape(iddef.idnum_value),
            ) for iddef in task_filter.idnum_criteria) + ".") | n }
        <% some_filter = True %>
    %endif
    ## Other
    %if task_filter.device_ids:
        ${ _("Uploading device is one of:") }
        <b>${ ", ".join(task_filter.get_device_names(request)) }</b>.
        <% some_filter = True %>
    %endif
    %if task_filter.adding_user_ids:
        ${ _("Adding user is one of:") }
        <b>${ ", ".join(task_filter.get_user_names(request)) }</b>.
        <% some_filter = True %>
    %endif
    %if task_filter.group_ids:
        ${ _("Group is one of:") }
        <b>${ ", ".join(task_filter.get_group_names(request)) }</b>.
        <% some_filter = True %>
    %endif
    %if task_filter.start_datetime:
        ${ _("Created") } <b>&ge; ${ task_filter.start_datetime }</b>.
        <% some_filter = True %>
    %endif
    %if task_filter.end_datetime:
        ${ _("Created") } <b>&le; ${ task_filter.end_datetime }</b>.
        <% some_filter = True %>
    %endif
    %if task_filter.text_contents:
        ${ _("Text contains one of:") }
        <b>${ ", ".join(repr(t) for t in task_filter.text_contents) }</b>.
        <% some_filter = True %>
    %endif
    %if task_filter.complete_only:
        <b>${ _("Restricted to “complete” tasks only.") }</b>
        <% some_filter = True %>
    %endif

    %if not some_filter:
        ${ _("[No filters.]") }
    %endif
</div>
