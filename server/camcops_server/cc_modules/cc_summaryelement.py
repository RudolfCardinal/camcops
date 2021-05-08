#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_summaryelement.py

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

**Classes to represent summary information created by tasks.**

For example, the PHQ9 task calculates a total score; that's part of its summary
information.

"""

from collections import OrderedDict
from typing import Any, Dict, List, Optional, Type, TYPE_CHECKING, Union

from cardinal_pythonlib.reprfunc import auto_repr
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.type_api import TypeEngine

from camcops_server.cc_modules.cc_db import TaskDescendant
from camcops_server.cc_modules.cc_tsv import TsvPage
from camcops_server.cc_modules.cc_xml import XmlElement

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_task import Task


# =============================================================================
# SummaryElement
# =============================================================================

class SummaryElement(object):
    """
    Returned by tasks to represent extra summary information that they
    calculate.

    Use this for extra information that can be added to a row represented by a
    task or its ancillary object.
    """
    def __init__(self,
                 name: str,
                 coltype: TypeEngine,
                 value: Any,
                 comment: str = None) -> None:
        """
        Args:
            name: column name
            coltype: SQLAlchemy column type; e.g. ``Integer()``,
                ``String(length=50)``
            value: value
            comment: explanatory comment
        """
        self.name = name
        self.coltype = coltype
        self.value = value
        self.comment = comment

    @property
    def decorated_comment(self) -> Optional[str]:
        return "(SUMMARY) " + self.comment if self.comment else None


# =============================================================================
# ExtraSummaryTable
# =============================================================================

class ExtraSummaryTable(TaskDescendant):
    """
    Additional summary information returned by a task.

    Use this to represent an entire table that doesn't have a 1:1 relationship
    with rows of a task or ancillary object.
    """
    def __init__(self,
                 tablename: str,
                 xmlname: str,
                 columns: List[Column],
                 rows: List[Union[Dict[str, Any], OrderedDict]],
                 task: "Task") -> None:
        """
        Args:
            tablename: name of the additional summary table
            xmlname: name of the XML tag to encapsulate this information
            columns: list of SQLAlchemy columns
            rows: list of rows, where each row is a dictionary mapping
                column names to values
            task: parent task (for cross-referencing in some kinds of export)
        """
        self.tablename = tablename
        self.xmlname = xmlname
        self.columns = columns
        self.rows = rows
        self.task = task

    def get_xml_element(self) -> XmlElement:
        """
        Returns an :class:`camcops_server.cc_modules.cc_xml.XmlElement`
        representing this summary table.
        """
        itembranches = []  # type: List[XmlElement]
        for valuedict in self.rows:
            leaves = []  # type: List[XmlElement]
            for k, v in valuedict.items():
                leaves.append(XmlElement(name=k, value=v))
            branch = XmlElement(name=self.tablename, value=leaves)
            itembranches.append(branch)
        return XmlElement(name=self.xmlname, value=itembranches)

    def get_tsv_page(self) -> TsvPage:
        """
        Returns an :class:`camcops_server.cc_modules.cc_tsv.TsvPage`
        representing this summary table.
        """
        return TsvPage(name=self.tablename, rows=self.rows)

    def __repr__(self) -> str:
        return auto_repr(self)

    # -------------------------------------------------------------------------
    # TaskDescendant overrides
    # -------------------------------------------------------------------------

    @classmethod
    def task_ancestor_class(cls) -> Optional[Type["Task"]]:
        return None

    def task_ancestor(self) -> Optional["Task"]:
        return self.task
