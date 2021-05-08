#!/usr/bin/env python

"""
playing/testwsgi.py

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

**Test WSGI applications.**

"""

# http://pylonsbook.com/en/1.0/the-web-server-gateway-interface-wsgi.html

import cgitb
import os
import StringIO
import sys

import cardinal_pythonlib.rnc_db as rnc_db
import cardinal_pythonlib.rnc_web as ws


# =============================================================================
# Actual demo application
# =============================================================================

def connect_to_database(environ):
    # Specific to our setup. Select a database engine.
    camcops_db_name = environ.get("CAMCOPS_DB_NAME", "camcops")
    camcops_db_user = environ.get("CAMCOPS_DB_USER", "root")
    camcops_db_password = environ.get("CAMCOPS_DB_PASSWORD")
    camcops_db_server = environ.get("CAMCOPS_DB_SERVER", "localhost")
    camcops_db_port = int(environ.get("CAMCOPS_DB_PORT", "3306"))

    if camcops_db_password is None:
        raise Exception("No database password specified")
    db = rnc_db.DatabaseSupporter()
    db.connect_to_database_mysql(
        server=camcops_db_server,
        port=camcops_db_port,
        database=camcops_db_name,
        user=camcops_db_user,
        password=camcops_db_password
    )
    return db


def application_show_environment_test_database(environ, start_response):
    linebreak = "=" * 79 + "\n"

    status = '200 OK'
    if not environ['mod_wsgi.process_group']:
        output = 'mod_wsgi EMBEDDED MODE'
    else:
        output = 'mod_wsgi DAEMON MODE'

    output += "\n\nenviron parameter:\n" + linebreak
    for (k, v) in sorted(environ.iteritems()):
        output += str(k) + ": " + str(v) + "\n"

    output += "\nos.environ:\n" + linebreak
    for (k, v) in sorted(os.environ.iteritems()):
        output += str(k) + ": " + str(v) + "\n"

    output += "\nCGI form:\n" + linebreak
    form = ws.get_cgi_fieldstorage_from_wsgi_env(environ)
    for k in form.keys():
        output += "{0} = {1}\n".format(k, form.getvalue(k))

    output += "\nCGI value for 'test' field:\n" + linebreak
    output += "{}\n".format(ws.get_cgi_parameter_str(form, "test"))

    output += "\nCGI value for 'Test' field:\n" + linebreak
    output += "{}\n".format(ws.get_cgi_parameter_str(form, "Test"))

    # This successfully writes to the Apache log:
    sys.stderr.write("testwsgi.py: STARTING\n")

    # Let's not bother with a persistent database connection for now.
    # http://stackoverflow.com/questions/405352/mysql-connection-pooling-question-is-it-worth-it  # noqa

    test_database = False
    if test_database:
        db = connect_to_database(environ)
        output += "\nCONNECTED TO DATABASE\n" + linebreak
        output += (
            "Count: " +
            str(db.fetchvalue("SELECT COUNT(*) FROM expdetthreshold")) + "\n"
        )

    # Final output
    response_headers = [('Content-type', 'text/plain'),
                        ('Content-Length', str(len(output)))]
    start_response(status, response_headers)
    return [output]


# =============================================================================
# Wrapper to print errors
# =============================================================================

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

application = ErrorReportingMiddleware(
    application_show_environment_test_database)
