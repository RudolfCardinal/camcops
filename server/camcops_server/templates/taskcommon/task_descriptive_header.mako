## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/taskcommon/task_descriptive_header.mako

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

<%page args="task: Task, anonymise: bool"/>

<%!
from cardinal_pythonlib.datetimefunc import format_datetime
from camcops_server.cc_modules.cc_constants import DateFormat
from camcops_server.cc_modules.cc_html import answer
from camcops_server.cc_modules.cc_text import SS
%>

## ============================================================================
## Patient (or "anonymous" label)
## ============================================================================

%if task.has_patient:
    %if anonymise:
        <div class="warning">${ _("Patient details hidden at user’s request!") }</div>
    %else:
        %if task.patient:
            <%include file="patient.mako" args="patient=task.patient, anonymise=anonymise, viewtype=viewtype"/>
        %else:
            <div class="warning">${ _("Missing patient information!") }</div>
        %endif
    %endif
%else:
    <div class="patient">
        ${ req.sstring(SS.ANONYMOUS_TASK) }
    </div>
%endif

## ============================================================================
## Which task, and when created (+/- how old was the patient then)?
## ============================================================================

<div class="taskheader">
    <b>${ task.longname(req) } (${ task.shortname })</b><br>
    ${ _("Created:") } ${ answer(
                            format_datetime(task.when_created,
                                            DateFormat.LONG_DATETIME_WITH_DAY,
                                            default=None)
                          ) | n }
    %if not task.is_anonymous and task.patient:
        (${ _("patient aged") } ${ answer(
                                        task.patient.get_age_at(task.when_created),
                                        default_for_blank_strings=True
                                   ) | n })
    %endif
</div>
