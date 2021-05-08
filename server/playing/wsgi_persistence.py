#!/usr/bin/env python

"""
playing/wsgi_persistence.py

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

**Test WSGI persistence.**

"""

import base64
import http.cookies
import os


# noinspection PyUnusedLocal
def application(environ, start_response):
    status = '200 OK'
    output = 'Hello'.encode('utf-8')
    cookie = http.cookies.SimpleCookie()
    # No expiration date, making it a session cookie
    randbytes = os.urandom(10)
    randstring = base64.urlsafe_b64encode(randbytes).decode('ascii')
    cookie["randomness"] = randstring
    cookie_tuples = [
        ("Set-Cookie", morsel.OutputString())
        for morsel in cookie.values()
    ]
    response_headers = [('Content-type', 'text/plain; charset=utf-8'),
                        ('Content-Length', str(len(output)))] + cookie_tuples
    start_response(status, response_headers)
    return [output]

"""
Run with:
$ gunicorn wsgi_persistence:application

Check with:
$ wget http://127.0.0.1:8000 --server-response
"""
