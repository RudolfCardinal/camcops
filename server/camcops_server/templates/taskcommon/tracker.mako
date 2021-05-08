## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/taskcommon/tracker.mako

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

## <%page args="tracker: Tracker, viewtype: str, pdf_landscape: bool"/>
<%inherit file="tracker_ctv.mako"/>

<%block name="office_preamble">
    ${ _("Trackers use only information from tasks that are flagged CURRENT and COMPLETE.") }
</%block>

%if not tracker.collection.all_tasks:

    <div class="warning">
        ${ _("No tasks found for tracker.") }
    </div>

%elif not tracker.patient:

    <div class="warning">
        ${ _("No patient found for tracker.") }
    </div>

%else:

    %for cls in tracker.taskfilter.task_classes:
        <% instances = tracker.collection.tasks_for_task_class(cls) %>
        %if instances:
            <div class="taskheader">
                <b>${ instances[0].longname(req) } (${ instances[0].shortname })</b>
            </div>
            ${ tracker.get_all_plots_for_one_task_html(instances) | n }
        %endif
    %endfor

%endif
