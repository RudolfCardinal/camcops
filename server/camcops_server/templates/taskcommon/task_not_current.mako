## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/taskcommon/task_not_current.mako

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

<%page args="task: Task"/>

<%!

from cardinal_pythonlib.datetimefunc import format_datetime
from camcops_server.cc_modules.cc_constants import DateFormat

%>

<div class="warning">
    %if task.pk is None:
        ${ _("WARNING! This is NOT a valid record. It has a blank primary key "
             "and is therefore nonsensical (and only useful for software testing).") }
    %else:
        ${ _("WARNING! This is NOT a current record.") }<br>
        %if task._successor_pk is not None:
            ${ _("It was MODIFIED at") }
            ${ format_datetime(task._when_removed_exact, DateFormat.LONG_DATETIME_SECONDS) }.
        %elif task._manually_erased:
            ${ _("It was MANUALLY ERASED at") }
            ${ format_datetime(task._manually_erased_at, DateFormat.LONG_DATETIME_SECONDS) }
            by ${ task.get_manually_erasing_user_username() }.
        %else:
            ${ _("It was DELETED at") }
            ${ format_datetime(task._when_removed_exact, DateFormat.LONG_DATETIME_SECONDS) }.
        %endif
    %endif
</div>
