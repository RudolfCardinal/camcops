#!/usr/bin/env python

"""
playing/test_webview.py

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

**Test the webview.**

"""

import camcopswebview  # module that does the real work

# =============================================================================
# CGI method - no longer used
# =============================================================================

# A two-script method:
# (a) better exception reporting to the HTTP client --- SUPERSEDED by import
#     cgitb
# (b) imported modules are automatically compiled, so load faster
# (c) Apache not quite so fussy about ownership of the script that gets edited
#     more

# camcopswebview.main()

# =============================================================================
# Wrapper to print errors
# =============================================================================

import sys
import cgitb
import StringIO


class ErrorReportingMiddleware(object):
    def __init__(self, app):
        self.app = app

    @staticmethod
    def format_exception(exc_info):
        dummy_file = StringIO.StringIO()
        hook = cgitb.Hook(file=dummy_file)
        hook(*exc_info)
        return [dummy_file.getvalue()]

    def __call__(self, environ, start_response):
        # noinspection PyBroadException
        try:
            return self.app(environ, start_response)
        except:
            exc_info = sys.exc_info()
            start_response(
                '500 Internal Server Error',
                [('content-type', 'text/html')],
                exc_info
            )
            return self.format_exception(exc_info)


# =============================================================================
# WSGI entry point
# =============================================================================

application = ErrorReportingMiddleware(camcopswebview.application)
