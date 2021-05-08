#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_formatter.py

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

**Safe alternative to str.format() that rejects anything not in the list of
  allowed keys.**

"""

from string import Formatter

from typing import Any, Mapping, Sequence, Tuple


class SafeFormatter(Formatter):
    """
    Safe alternative to ``str.format()`` that rejects anything not in the list
    of allowed keys.

    Basic usage:

    .. code-block:: python

        from camcops_server.cc_modules.cc_formatter import SafeFormatter

        f = SafeFormatter(["a", "b"])

        f.format("a={a}, b={b}", a=1, b=2)  # OK
        f.format("a={a.__class__}", a=1)  # raises KeyError
        f.format("a={a}, b={b}, c={c}", a=1, b=2, c=3)  # raises KeyError
    """

    def __init__(self, allowed_keys: Sequence[str]) -> None:
        """
        Args:
            allowed_keys:
                Keys that are permitted within a brace-delimited format string.
        """
        self._allowed_keys = allowed_keys
        super().__init__()

    def get_valid_parameters_string(self) -> str:
        """
        Returns a string, such as ``{a}, {b}``, that enumerates the parameters
        allowed (e.g. for user help).
        """
        return ", ".join(f"{{{k}}}" for k in self._allowed_keys)

    def get_field(self, field_name: str, args: Sequence[Any],
                  kwargs: Mapping[str, Any]) -> Tuple[Any, str]:
        """
        Overrides :meth:`Formatter.get_field` (q.v.).

        Args:
            field_name:
                name of the field to be looked up
            args:
                positional arguments passed to :meth:`format` (not including
                the format string)
            kwargs:
                keyword arguments passed to :meth:`format`

        Returns:
            tuple: ``(obj, arg_used)`` where ``obj`` is the object that's been
            looked up, and ``arg_used`` is the argument it came from

        Raises:
            - :exc:`KeyError` if the field_name is disallowed
        """
        # print(f"field_name={field_name!r}, args={args!r}, kwargs={kwargs!r}")
        if field_name not in self._allowed_keys:
            raise KeyError(field_name)

        return super().get_field(field_name, args, kwargs)

    def validate(self, format_string: str) -> None:
        """
        Checks a format string for validity.

        Args:
            format_string:
                string to check

        Raises:
            - :exc:`KeyError` for unknown key
            - :exc:`ValueError` for unmatched ``{``

        """
        test_dict = {k: "" for k in self._allowed_keys}

        self.format(format_string, **test_dict)
