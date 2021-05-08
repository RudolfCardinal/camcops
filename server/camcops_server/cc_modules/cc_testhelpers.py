#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_testhelpers.py

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

**Helper functions used during testing.**

"""

from typing import List, Type


def class_attribute_names(cls: Type,
                          exclude_underscore: bool = True,
                          exclude_double_underscore: bool = True) -> List[str]:
    """
    When given a class, returns the names of all its attributes, by default
    excluding those starting with single and double underscores.

    Used in particular to enumerate constants provided within a class.
    """
    attrs = []  # type: List[str]
    for x in cls.__dict__.keys():
        if exclude_underscore and x.startswith("_"):
            continue
        if exclude_double_underscore and x.startswith("__"):
            continue
        attrs.append(x)
    return attrs
