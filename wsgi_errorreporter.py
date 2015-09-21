#!/usr/bin/python
# -*- encoding: utf8 -*-

# =============================================================================
# ErrorReportingMiddleware
# =============================================================================
# From: http://pylonsbook.com/en/1.0/the-web-server-gateway-interface-wsgi.html
# Modified to use six.StringIO
# Latest changes: 21 Sep 2015

import six
import sys
import cgitb


class ErrorReportingMiddleware(object):
    """WSGI middleware to produce cgitb traceback."""
    def __init__(self, app):
        self.app = app

    def format_exception(self, exc_info):
        dummy_file = six.StringIO()
        hook = cgitb.Hook(file=dummy_file)
        hook(*exc_info)
        return [dummy_file.getvalue()]

    def __call__(self, environ, start_response):
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
