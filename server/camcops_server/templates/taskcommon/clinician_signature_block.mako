## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/taskcommon/clinician_signature_block.mako

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

## Genuinely static, so we can cache it:
<%page cached="True" cache_region="local" cache_key="clinician_signature_block.mako"/>

<div>
    <table class="noborder">
        <tr class="signature_label">
            <td class="signature_label" style="width:33%">
                ${ _("Signature of author/validator") }
            </td>
            <td class="signature_label" style="width:33%">
                ${ _("Print name") }
            </td>
            <td class="signature_label" style="width:33%">
                ${ _("Date and time") }
            </td>
        </tr>
        <tr class="signature">
            ## ... can't get "height" to work in table; only seems to like line-height; for
            ## which, you need some text, hence the &nbsp;
            ## https://stackoverflow.com/questions/6398172/setting-table-row-height-in-css
            <td class="signature">&nbsp;</td>
            <td class="signature">&nbsp;</td>
            <td class="signature">&nbsp;</td>
        </tr>
    </table>
</div>
