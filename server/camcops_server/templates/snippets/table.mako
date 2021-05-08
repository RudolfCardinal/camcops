## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/snippets/table.mako

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
<%page args="column_headings, rows, table_class=None, escape_cells=True"/>
<table
%if table_class:
class="${ table_class | n }"
%endif
>
    <tr>
        %for c in column_headings:
            <th>${ c }</th>
        %endfor
    </tr>
    %for row in rows:
        <tr>
            %for (col_index,val) in enumerate(row):
                <td class="table-cell col-${col_index | n }">
                    %if escape_cells:
                        ${ val }
                    %else:
                        ${ val | n }
                    %endif
                </td>
            %endfor
        </tr>
    %endfor
</table>
