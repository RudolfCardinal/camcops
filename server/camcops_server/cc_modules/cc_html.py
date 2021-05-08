#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_html.py

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

**Basic HTML creation functions.**

"""

import base64
from typing import Any, Callable, List, Optional, TYPE_CHECKING, Union

import cardinal_pythonlib.rnc_web as ws

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_text import SS

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_request import CamcopsRequest


# =============================================================================
# HTML elements
# =============================================================================

def table_row(columns: List[str],
              classes: List[str] = None,
              colspans: List[Union[str, int]] = None,
              colwidths: List[str] = None,
              default: str = "",
              heading: bool = False) -> str:
    """
    Make HTML table row.

    Args:
        columns: contents of HTML table columns
        classes: optional CSS classes, one for each column
        colspans: ``colspan`` values for each column
        colwidths: ``width`` values for each column
        default: content to use if a ``column`` value is None
        heading: use ``<th>`` rather than ``<td>`` for contents?

    Returns:
        the ``<tr>...</tr>`` string
    """
    n = len(columns)

    if not classes or len(classes) != n:
        # blank, or duff (in which case ignore)
        classes = [""] * n
    else:
        classes = [(f' class="{x}"' if x else '') for x in classes]

    if not colspans or len(colspans) != n:
        # blank, or duff (in which case ignore)
        colspans = [""] * n
    else:
        colspans = [(f' colspan="{x}"' if x else '') for x in colspans]

    if not colwidths or len(colwidths) != n:
        # blank, or duff (in which case ignore)
        colwidths = [""] * n
    else:
        colwidths = [
            (f' width="{x}"' if x else '')
            for x in colwidths
        ]

    celltype = "th" if heading else "td"
    rows = "".join([
        (
            f"<{celltype}{classes[i]}{colspans[i]}{colwidths[i]}>"
            f"{default if columns[i] is None else columns[i]}"
            f"</{celltype}>"
        )
        for i in range(n)
    ])
    return f"<tr>{rows}</tr>\n"


def div(content: str, div_class: str = "") -> str:
    """
    Make simple HTML div.
    """
    class_str = f' class="{div_class}"' if div_class else ''
    return f"""
        <div{class_str}>
            {content}
        </div>
    """


def table(content: str, table_class: str = "") -> str:
    """
    Make simple HTML table.
    """
    class_str = f' class="{table_class}"' if table_class else ''
    return f"""
        <table{class_str}>
            {content}
        </table>
    """


def tr(*args, tr_class: str = "", literal: bool = False) -> str:
    """
    Make simple HTML table data row.

    Args:
        *args: Set of columns data.
        literal: Treat elements as literals with their own ``<td> ... </td>``,
            rather than things to be encapsulated.
        tr_class: table row class
    """
    if literal:
        elements = args
    else:
        elements = [td(x) for x in args]
    tr_class = f' class="{tr_class}"' if tr_class else ''
    contents = "".join(elements)
    return f"<tr{tr_class}>{contents}</tr>\n"


def td(contents: Any, td_class: str = "", td_width: str = "") -> str:
    """
    Make simple HTML table data ``<td>...</td>`` cell.
    """
    td_class = f' class="{td_class}"' if td_class else ''
    td_width = f' width="{td_width}"' if td_width else ''
    return f"<td{td_class}{td_width}>{contents}</td>\n"


def th(contents: Any, th_class: str = "", th_width: str = "") -> str:
    """
    Make simple HTML table header ``<th>...</th>`` cell.
    """
    th_class = f' class="{th_class}"' if th_class else ''
    th_width = f' width="{th_width}"' if th_width else ''
    return f"<th{th_class}{th_width}>{contents}</th>\n"


def tr_qa(q: str,
          a: Any,
          default: str = "?",
          default_for_blank_strings: bool = False) -> str:
    """
    Make HTML two-column data row (``<tr>...</tr>``), with the right-hand
    column formatted as an answer.
    """
    return tr(q, answer(a, default=default,
                        default_for_blank_strings=default_for_blank_strings))


def heading_spanning_two_columns(s: str) -> str:
    """
    HTML table heading row spanning 2 columns.
    """
    return tr_span_col(s, cols=2, tr_class=CssClass.HEADING)


def subheading_spanning_two_columns(s: str, th_not_td: bool = False) -> str:
    """
    HTML table subheading row spanning 2 columns.
    """
    return tr_span_col(s, cols=2, tr_class=CssClass.SUBHEADING,
                       th_not_td=th_not_td)


def subheading_spanning_three_columns(s: str, th_not_td: bool = False) -> str:
    """
    HTML table subheading row spanning 3 columns.
    """
    return tr_span_col(s, cols=3, tr_class=CssClass.SUBHEADING,
                       th_not_td=th_not_td)


def subheading_spanning_four_columns(s: str, th_not_td: bool = False) -> str:
    """
    HTML table subheading row spanning 4 columns.
    """
    return tr_span_col(s, cols=4, tr_class=CssClass.SUBHEADING,
                       th_not_td=th_not_td)


def bold(x: str) -> str:
    """
    Applies HTML bold.
    """
    return f"<b>{x}</b>"


def italic(x: str) -> str:
    """
    Applies HTML italic.
    """
    return f"<i>{x}</i>"


def identity(x: Any) -> Any:
    """
    Returns argument unchanged.
    """
    return x


def bold_webify(x: str) -> str:
    """
    Webifies the string, then makes it bold.
    """
    return bold(ws.webify(x))


def sub(x: str) -> str:
    """
    Applies HTML subscript.
    """
    return f"<sub>{x}</sub>"


def sup(x: str) -> str:
    """
    Applies HTML superscript.
    """
    return f"<sup>{x}</sup>"


def answer(x: Any,
           default: str = "?",
           default_for_blank_strings: bool = False,
           formatter_answer: Callable[[str], str] = bold_webify,
           formatter_blank: Callable[[str], str] = italic) -> str:
    """
    Formats answer in bold, or the default value if None.

    Avoid the word "None" for the default, e.g.
    "Score indicating likelihood of abuse: None"... may be misleading!
    Prefer "?" instead.
    """
    if x is None:
        return formatter_blank(default)
    if default_for_blank_strings and not x and isinstance(x, str):
        return formatter_blank(default)
    return formatter_answer(x)


def tr_span_col(x: str,
                cols: int = 2,
                tr_class: str = "",
                td_class: str = "",
                th_not_td: bool = False) -> str:
    """
    HTML table data row spanning several columns.

    Args:
        x: Data.
        cols: Number of columns to span.
        tr_class: CSS class to apply to tr.
        td_class: CSS class to apply to td.
        th_not_td: make it a th, not a td.
    """
    cell = "th" if th_not_td else "td"
    tr_cl = f' class="{tr_class}"' if tr_class else ""
    td_cl = f' class="{td_class}"' if td_class else ""
    return f'<tr{tr_cl}><{cell} colspan="{cols}"{td_cl}>{x}</{cell}></tr>'


def get_data_url(mimetype: str, data: Union[bytes, memoryview]) -> str:
    """
    Takes data (in binary format) and returns a data URL as per RFC 2397
    (https://tools.ietf.org/html/rfc2397), such as:

    .. code-block:: none

        data:MIMETYPE;base64,B64_ENCODED_DATA
    """
    return f"data:{mimetype};base64,{base64.b64encode(data).decode('ascii')}"


def get_embedded_img_tag(mimetype: str, data: Union[bytes, memoryview]) -> str:
    """
    Takes a binary image and its MIME type, and produces an HTML tag of the
    form:

    .. code-block:: none

        <img src="DATA_URL">
    """
    return f'<img src={get_data_url(mimetype, data)}>'


# =============================================================================
# Field formatting
# =============================================================================

def get_yes_no(req: "CamcopsRequest", x: Any) -> str:
    """
    'Yes' if x else 'No'
    """
    return req.sstring(SS.YES) if x else req.sstring(SS.NO)


def get_yes_no_none(req: "CamcopsRequest", x: Any) -> Optional[str]:
    """
    Returns 'Yes' for True, 'No' for False, or None for None.
    """
    if x is None:
        return None
    return get_yes_no(req, x)


def get_yes_no_unknown(req: "CamcopsRequest", x: Any) -> str:
    """
    Returns 'Yes' for True, 'No' for False, or '?' for None.
    """
    if x is None:
        return "?"
    return get_yes_no(req, x)


def get_true_false(req: "CamcopsRequest", x: Any) -> str:
    """
    'True' if x else 'False'
    """
    return req.sstring(SS.TRUE) if x else req.sstring(SS.FALSE)


def get_true_false_none(req: "CamcopsRequest", x: Any) -> Optional[str]:
    """
    Returns 'True' for True, 'False' for False, or None for None.
    """
    if x is None:
        return None
    return get_true_false(req, x)


def get_true_false_unknown(req: "CamcopsRequest", x: Any) -> str:
    """
    Returns 'True' for True, 'False' for False, or '?' for None.
    """
    if x is None:
        return "?"
    return get_true_false(req, x)


def get_present_absent(req: "CamcopsRequest", x: Any) -> str:
    """
    'Present' if x else 'Absent'
    """
    return req.sstring(SS.PRESENT) if x else req.sstring(SS.ABSENT)


def get_present_absent_none(req: "CamcopsRequest", x: Any) -> Optional[str]:
    """
    Returns 'Present' for True, 'Absent' for False, or None for None.
    """
    if x is None:
        return None
    return get_present_absent(req, x)


def get_present_absent_unknown(req: "CamcopsRequest", x: str) -> str:
    """
    Returns 'Present' for True, 'Absent' for False, or '?' for None.
    """
    if x is None:
        return "?"
    return get_present_absent(req, x)


def get_ternary(x: Any,
                value_true: Any = True,
                value_false: Any = False,
                value_none: Any = None) -> Any:
    """
    Returns ``value_none`` if ``x`` is ``None``, ``value_true`` if it's truthy,
    or ``value_false`` if it's falsy.
    """
    if x is None:
        return value_none
    if x:
        return value_true
    return value_false


def get_correct_incorrect_none(x: Any) -> Optional[str]:
    """
    Returns None if ``x`` is None, "Correct" if it's truthy, or "Incorrect" if
    it's falsy.
    """
    return get_ternary(x, "Correct", "Incorrect", None)
