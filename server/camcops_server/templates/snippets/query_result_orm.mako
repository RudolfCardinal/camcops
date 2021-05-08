## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/snippets/query_result_orm.mako

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

Creates an HTML table from a query result (from SQLAlchemy ORM).

</%doc>

<%page args="attrnames, descriptions, orm_objects, null_html='<i>NULL</i>'"/>

<%!

from mako.filters import html_escape

def filter_value(value, null_html):
    if value is None:
        return null_html
    return html_escape(str(value))

def get_value(orm_object, attrname, null_html):
    value = getattr(orm_object, attrname)
    return filter_value(value, null_html)

%>

<table>
    <tr>
        %for desc in descriptions:
            <th>${ desc or "" }</th>
        %endfor
    </tr>
    %for orm_object in orm_objects:
        <tr>
            %for attrname in attrnames:
                <td>${ get_value(orm_object, attrname, null_html) | n }</td>
            %endfor
        </tr>
    %endfor
</table>
