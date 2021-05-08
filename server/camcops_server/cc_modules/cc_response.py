#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_response.py

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

**Implements a Pyramid Response object customized for CamCOPS.**

"""

from typing import TYPE_CHECKING

from pyramid.response import Response

from camcops_server.cc_modules.cc_baseconstants import (
    DEFORM_SUPPORTS_CSP_NONCE,
)

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_request import CamcopsRequest


class CamcopsResponse(Response):
    """
    Response class, inheriting from Pyramid's response.

    We do this mainly to set the HTTP ``Content-Security-Policy`` header to
    match the nonce set by the
    :class:``camcops_server.cc_modules.cc_request.CamcopsRequest``.

    However, once this class exists, it may as well set all the standard
    headers, rather than using additional middleware.
    """
    def __init__(self, camcops_request: "CamcopsRequest", **kwargs) -> None:
        super().__init__(**kwargs)
        nonce = camcops_request.nonce
        self.headers.update([
            # List of key, value tuples:

            # -----------------------------------------------------------------
            # Cache-Control: Caching
            # -----------------------------------------------------------------
            # NOT THIS:
            #       ("Cache-Control", "no-cache, no-store, must-revalidate"),
            # ... or we get a ZAP error "Incomplete or No Cache-control and
            #     Pragma HTTP Header Set"
            # ... note that Pragma is HTTP/1.0 and cache-control is HTTP/1.1,
            #     so you don't have to do both.
            # BUT that prevents caching of images (e.g. logos), and we don't
            # want that.
            #
            # The Pyramid @view_config decorator (or add_view function) takes
            # an "http_cache" parameter, as per
            # https://pyramid-pt-br.readthedocs.io/en/latest/api/config.html#pyramid.config.Configurator.add_view  # noqa
            # and it looks like viewderivers.py implements this. The default
            # does not set the HTTP "cache-control" header, explained at
            # https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Cache-Control  # noqa
            # The basic options are:
            # - 0: do not cache
            # - integer_seconds, or datetime.timedelta: lifespan
            # - tuple (lifespan, dictionary_of_extra_cache_control_details)
            # - tuple (None, dictionary_of_extra_cache_control_details)
            # We now set http_cache for all our views via view_config, as well
            # as the equivalent of using cache_max_age for add_static_view().
            #
            # However, this (as an additional Cache-Control header -- as well
            # as any "cache, it's static" or "don't cache" header) sorts out
            # any ZAP complaints:

            ("Cache-Control", 'no-cache="Set-Cookie, Set-Cookie2"'),

            # -----------------------------------------------------------------
            # Content-Security-Policy: Control resources that are permitted to
            # load, to mitigate against cross-site scripting attacks
            # -----------------------------------------------------------------
            # - Content-Security-Policy.
            # - Recommended by Falanx penetration testing.
            # - https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Security-Policy  # noqa
            # - Defaults from https://owasp.org/www-project-secure-headers/
            # - Re scripts: see https://csper.io/blog/no-more-unsafe-inline
            # - Re nonces (and in general): see
            #   https://stackoverflow.com/questions/42922784/what-s-the-purpose-of-the-html-nonce-attribute-for-script-and-style-elements  # noqa
            # - Note that multiple CSP headers combine to produce the most
            #   restrictive and can get confusing; best to use one. See
            #   https://chrisguitarguy.com/2019/07/05/working-with-multiple-content-security-policy-headers/  # noqa
            (
                "Content-Security-Policy",
                # A single string:
                (
                    # The secure policy:

                    "default-src 'self' data:; "
                    "object-src 'none'; "
                    "child-src 'self'; "

                    f"style-src 'nonce-{nonce}' 'self'; "
                    # ... meaning: allow inline CSS only if it is tagged with
                    # this nonce, via <style nonce="XXX">, or if it comes from
                    # our site ('self'). See
                    # https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Security-Policy/style-src  # noqa

                    # And similarly for scripts:
                    f"script-src 'nonce-{nonce}' 'self'; "
                    # ... "unsafe-eval" is currently required by deform.js, in
                    # addSequenceItem(). Deform stores prototype code and then
                    # clones it when you add a sequence item; this involves
                    # evaluation.

                    "frame-ancestors 'none'; "
                    "upgrade-insecure-requests; "
                    "block-all-mixed-content"
                ) if DEFORM_SUPPORTS_CSP_NONCE else (
                    # The less secure policy, for Deform:

                    "default-src 'self' data:; "
                    "object-src 'none'; "
                    "child-src 'self'; "

                    "style-src 'self' 'unsafe-inline'; "
                    "script-src 'self' 'unsafe-inline' 'unsafe-eval'; "

                    "frame-ancestors 'none'; "
                    "upgrade-insecure-requests; "
                    "block-all-mixed-content"
                )
            ),

            # -----------------------------------------------------------------
            # Strict-Transport-Security: Enforce HTTPS through the client.
            # -----------------------------------------------------------------
            # - In part this is by e.g. telling Google (and thus Chrome) that
            #   your site always uses HTTPS, to prevent HTTP-based spoofing.
            # - https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Strict-Transport-Security  # noqa
            # - Advice is at
            #   https://blog.qualys.com/vulnerabilities-research/2016/03/28/the-importance-of-a-proper-http-strict-transport-security-implementation-on-your-web-server  # noqa

            ("Strict-Transport-Security", "max-age=31536000"),  # = 1 year

            # -----------------------------------------------------------------
            # X-Content-Type-Options: Opt out of MIME type sniffing
            # -----------------------------------------------------------------
            # - Recommended by ZAP penetration testing.
            #   ... otherwise, the error is "X-Content-Type-Options Header
            #   Missing"
            # - https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/X-Content-Type-Options  # noqa

            ("X-Content-Type-Options", "nosniff"),

            # -----------------------------------------------------------------
            # X-Frame-Options: Prevent rendering within a frame
            # -----------------------------------------------------------------
            # - Recommended by ZAP penetration testing.
            #   ... otherwise, the error is "X-Frame-Options Header Not Set"
            # - https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/X-Frame-Options  # noqa

            ("X-Frame-Options", "DENY"),

            # -----------------------------------------------------------------
            # X-XSS-Protection: Check for cross-site scripting attacks
            # -----------------------------------------------------------------
            # - Recommended by Falanx penetration testing.
            # - https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/X-XSS-Protection  # noqa

            ("X-XSS-Protection", "1"),

        ])


def camcops_response_factory(request: "CamcopsRequest") -> Response:
    """
    Factory function to make a response object.
    """
    return CamcopsResponse(camcops_request=request)
