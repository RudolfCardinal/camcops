## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/taskcommon/special_notes.mako

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

<%page args="special_notes: List[SpecialNote], title: str, viewtype: str"/>

<%!

from camcops_server.cc_modules.cc_convert import br_html
from camcops_server.cc_modules.cc_pyramid import Routes, ViewArg, ViewParam

%>

<div class="specialnote">
    <b>${ title }</b><br>
    %for idx, sn in enumerate(special_notes):
        %if idx > 0:
            <br>
        %endif
        [${ sn.note_at or "?" }, ${ sn.get_username() or "?" }]<br>
        <b>${ sn.note | n,br_html }</b>
        %if viewtype == ViewArg.HTML and sn.user_may_delete_specialnote(req.user):
            <br>[<a href="${ req.route_url(
                                Routes.DELETE_SPECIAL_NOTE,
                                _query={ViewParam.NOTE_ID: sn.note_id}
                             ) | n }">${ _("Delete special note") }</a>]
        %endif
    %endfor
</div>
