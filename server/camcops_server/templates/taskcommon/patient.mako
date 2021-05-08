## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/taskcommon/patient.mako

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

<%page args="patient: Patient, viewtype: str, include_special_notes: bool = True"/>

<div class="patient">
    <b>${ patient.get_surname_forename_upper() }</b>
        (${ patient.get_sex_verbose() })
        ${ patient.get_dob_html(req, longform=True) | n }
    %for pt_id_num in patient.idnums:
        <!-- ID${ pt_id_num.which_idnum } -->
        <br>${ pt_id_num.description(req) }: <b>${ pt_id_num.idnum_value }</b>
        %if not pt_id_num.is_fully_valid(req):
            <span class="invalid_id_number_foreground">[${ pt_id_num.why_invalid(req) }]</span>
        %endif
    %endfor
    %if patient.other:
        <br>${ _("Other details:") } <b>${ patient.other }</b>
    %endif
    %if patient.address:
        <br>${ _("Address:") } <b>${ patient.address }</b>
    %endif
    %if patient.gp:
        <br>${ _("GP:") } <b>${ patient.gp }</b>
    %endif
</div>

%if include_special_notes and patient.special_notes:
    <%include file="special_notes.mako" args="special_notes=patient.special_notes, title='PATIENT SPECIAL NOTES', viewtype=viewtype"/>
%endif
