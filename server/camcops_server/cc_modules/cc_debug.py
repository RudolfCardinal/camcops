#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_debug.py

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
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.

===============================================================================

**Debugging utilities**

"""

import cProfile
# import trace
from types import FrameType
from typing import Any, Callable, Set, Union

TraceFuncType = Callable[[FrameType, str, Any], Union[Callable, None]]
# ... returns a trace function (but we can't make the type definition
# recursive) or None.


# https://stackoverflow.com/questions/5375624/a-decorator-that-profiles-a-method-call-and-logs-the-profiling-result  # noqa
def profile(func):
    """
    Decorator to generate profiler output for slow code
    from camcops_server.cc_debug import profile.

    Add @profile to the function you want to profile.
    Will generate a file called <function name>.profile.

    Can be visualised with e.g. SnakeViz (pip install snakeviz)
    """

    def wrapper(*args, **kwargs):
        datafn = func.__name__ + ".profile"
        prof = cProfile.Profile()
        retval = prof.runcall(func, *args, **kwargs)
        prof.dump_stats(datafn)

        return retval

    return wrapper


# noinspection PyUnusedLocal
def trace_calls(frame: FrameType, event: str, arg: Any) \
        -> Union[TraceFuncType, None]:
    """
    A function that can be used as an argument to ``sys.settrace``. It prints
    details of every function called (filename, line number, function name).
    """
    # https://pymotw.com/2/sys/tracing.html
    # https://docs.python.org/3/library/sys.html#sys.settrace

    # Function calls only
    if event != "call":
        return
    co = frame.f_code
    filename = co.co_filename
    func_name = co.co_name
    line_no = frame.f_lineno
    print(f"- Call to {filename}:{line_no}:{func_name}")


def makefunc_trace_unique_calls(file_only: bool = False) -> TraceFuncType:
    """
    Creates a function that you can use as an argument to ``sys.settrace()``.
    When you execute a trace, it shows only new call to each function.

    Args:
        file_only:
            Shows files called only, not functions with line numbers.
    """
    called = set()  # type: Set[str]

    # noinspection PyUnusedLocal
    def _trace_calls(frame: FrameType, event: str, arg: Any) \
            -> Union[TraceFuncType, None]:
        nonlocal called
        if event != "call":
            return
        co = frame.f_code
        filename = co.co_filename
        if file_only:
            signature = filename
        else:
            func_name = co.co_name
            line_no = frame.f_lineno
            signature = f"{filename}:{line_no}:{func_name}"
        if signature not in called:
            print(f"- First call to {signature}")
            called.add(signature)

    return _trace_calls
