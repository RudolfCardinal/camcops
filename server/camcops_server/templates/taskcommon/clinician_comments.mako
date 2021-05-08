## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/taskcommon/clinician_comments.mako

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

<%page args="comment: str"/>

<div class="clinician">
    <table class="taskdetail">
        <tr>
            <td style="width:20%">${ _("Clinician’s comments:") }</td>
            <td style="width:80%">
                %if comment is None:
                    <i>${ _("None") }</i>
                %else:
                    <b>${ comment }</b>
                %endif
            </td>
        </tr>
    </table>
</div>
