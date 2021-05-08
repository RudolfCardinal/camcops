#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_validators.py

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

**String validators and the like.**

All functions starting ``validate_`` do nothing if the input is good, and raise
:exc:`ValueError` if it's bad, with a descriptive error (you can use ``str()``
on the exception).

All validators take a
:class:`camcops_server.cc_modules.cc_request.CamcopsRequest` parameter, for
internationalized error messages.

WARNING: even the error messages shouldn't contain the error-producing strings.
"""

import ipaddress
import logging
import re
from typing import Callable, List, Optional, TYPE_CHECKING
import urllib.parse

from cardinal_pythonlib.logs import BraceStyleAdapter
from colander import EMAIL_RE

from camcops_server.cc_modules.cc_constants import (
    MINIMUM_PASSWORD_LENGTH,
    StringLengths,
)
from camcops_server.cc_modules.cc_password import password_prohibited

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_request import CamcopsRequest

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Typing constants
# =============================================================================

STRING_VALIDATOR_TYPE = Callable[[str, Optional["CamcopsRequest"]], None]
# ... string validators raise ValueError if the string is invalid


# =============================================================================
# Raising exceptions: sometimes internationalized, sometimes not
# =============================================================================

def dummy_gettext(x: str) -> str:
    """
    Returns the input directly.
    """
    return x


# =============================================================================
# Regex manipulation
# =============================================================================

def anchor(expression: str,
           anchor_start: bool = True,
           anchor_end: bool = True) -> str:
    """
    Adds start/end anchors.
    """
    start = "^" if anchor_start else ""
    end = "$" if anchor_end else ""
    return f"{start}{expression}{end}"


def zero_or_more(expression: str) -> str:
    """
    Regex for zero or more copies.
    """
    return f"{expression}*"


def one_or_more(expression: str) -> str:
    """
    Regex for one or more copies.
    """
    return f"{expression}+"


def min_max_copies(
        expression: str,
        max_count: int,
        min_count: int = 1) -> str:
    """
    Given a regex expression, permit it a minimum/maximum number of times. For
    example, for a regex group ``x``, produce ``x{min,max}``.

    Be very careful if you use ``min_count == 0`` -- without other
    restrictions, your regex may match an empty string.
    """
    assert 0 <= min_count <= max_count
    return f"{expression}{{{min_count},{max_count}}}"


def describe_regex_permitted_char(
        expression: str,
        req: Optional["CamcopsRequest"] = None,
        invalid_prefix: bool = True) -> str:
    """
    Describes the characters permitted in a regular expression character
    selector -- as long as it's simple! This won't handle arbitrary regexes.
    """
    assert expression.startswith("[") and expression.endswith("]")
    content = expression[1:-1]  # strip off surrounding []
    permitted = []  # type: List[str]
    length = len(content)
    _ = req.gettext if req else dummy_gettext
    i = 0
    while i < length:
        if content[i] == "\\":
            # backslash preceding another character: regex code or escaped char
            assert i + 1 < length, f"Bad escaping in {expression!r}"
            escaped = content[i + 1]
            if escaped == "w":
                permitted.append(_("word character"))
            elif escaped == "W":
                permitted.append(_("non-word character"))
            elif escaped == "d":
                permitted.append(_("digit"))
            elif escaped == "D":
                permitted.append(_("non-digit"))
            elif escaped == "s":
                permitted.append(_("whitespace"))
            elif escaped == "S":
                permitted.append(_("non-whitespace"))
            else:
                permitted.append(repr(escaped))
            i += 2
        elif i + 1 < length and content[i + 1] == "-":
            # range like A-Z
            assert i + 2 < length, f"Bad range specification in {expression!r}"
            permitted.append(content[i:i + 3])
            i += 3
        else:
            char = content[i]
            if char == ".":
                permitted.append(_("any character"))
            else:
                permitted.append(repr(char))
            i += 1
    description = ", ".join(permitted)
    prefix = _("Invalid string.") + " " if invalid_prefix else ""
    return prefix + _("Permitted characters:") + " " + description


def describe_regex_permitted_char_length(
        expression: str,
        max_length: int,
        min_length: int = 1,
        req: Optional["CamcopsRequest"] = None) -> str:
    """
    Describes a valid string by permitted characters and length.
    """
    _ = req.gettext if req else dummy_gettext
    return(
        _("Invalid string.") +
        " " +
        _("Minimum length = {}. Maximum length = {}.").format(
            min_length, max_length
        ) +
        " " +
        describe_regex_permitted_char(expression, req, invalid_prefix=False)
    )


# =============================================================================
# Generic validation functions
# =============================================================================

def validate_by_char_and_length(
        x: str,
        permitted_char_expression: str,
        max_length: int,
        min_length: int = 1,
        req: Optional["CamcopsRequest"] = None,
        flags: int = 0) -> None:
    """
    Validate a string based on permitted characters and length.
    """
    regex = re.compile(
        anchor(min_max_copies(
            expression=permitted_char_expression,
            min_count=min_length,
            max_count=max_length
        )),
        flags=flags
    )
    if not regex.match(x):
        raise ValueError(describe_regex_permitted_char_length(
            permitted_char_expression,
            min_length=min_length, max_length=max_length, req=req
        ))


# =============================================================================
# Generic strings
# =============================================================================

ALPHA_CHAR = "[A-Za-z]"

ALPHANUM_UNDERSCORE_CHAR = "[A-Za-z0-9_]"
ALPHANUM_UNDERSCORE_REGEX = re.compile(
    anchor(one_or_more(ALPHANUM_UNDERSCORE_CHAR))
)

ALPHANUM_UNDERSCORE_HYPHEN_CHAR = r"[A-Za-z0-9_\-]"
ALPHANUM_UNDERSCORE_HYPHEN_DOT_CHAR = r"[A-Za-z0-9_\-\.]"
ALPHANUM_COMMA_UNDERSCORE_HYPHEN_BRACE_CHAR = r"[A-Za-z0-9,_\-\{\}]"
ALPHANUM_UNDERSCORE_HYPHEN_SPACE_CHAR = r"[A-Za-z0-9_\- ]"

HUMAN_NAME_CHAR_UNICODE = r"[\w\-'’ \.]"
# \w is a word character; with the re.UNICODE flag, that includes accented
# characters. Then we allow hyphen, plain apostrophe, Unicode apostrophe,
# space, dot.
HUMAN_MANDATORY_CHAR_REGEX = re.compile(r"\w+", re.UNICODE)
# ... for "at least one word character somewhere"


# -----------------------------------------------------------------------------
# Level 1. Computer-style simple strings with no spaces.
# -----------------------------------------------------------------------------

def validate_alphanum(x: str,
                      req: Optional["CamcopsRequest"] = None) -> None:
    """
    Validates a generic alphanumeric string.
    """
    if not x.isalnum():
        _ = req.gettext if req else dummy_gettext
        raise ValueError(_("Invalid alphanumeric string"))


def validate_alphanum_underscore(
        x: str,
        req: Optional["CamcopsRequest"] = None) -> None:
    """
    Validates a string that can be alphanumeric or contain an underscore.
    """
    if not ALPHANUM_UNDERSCORE_REGEX.match(x):
        raise ValueError(
            describe_regex_permitted_char(ALPHANUM_UNDERSCORE_CHAR, req)
        )


# -----------------------------------------------------------------------------
# Level 2. Human-style simple strings, allowing spaces but only minimal
# punctuation.
# -----------------------------------------------------------------------------

# ... see specific validators.

# -----------------------------------------------------------------------------
# Level 3. Human-style strings, such as people's names; may involve accented
# characters, spaces, some punctuation; may be used as Python or SQL search
# literals (with suitable precautions).
# -----------------------------------------------------------------------------

# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# 3(a). Human names
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

def validate_human_name(
        x: str,
        req: Optional["CamcopsRequest"] = None,
        min_length: int = 0,
        max_length: int = StringLengths.PATIENT_NAME_MAX_LEN) -> None:
    """
    Accepts spaces, accents, etc.

    This is hard. See
    https://stackoverflow.com/questions/888838/regular-expression-for-validating-names-and-surnames
    """  # noqa
    validate_by_char_and_length(
        x,
        permitted_char_expression=HUMAN_NAME_CHAR_UNICODE,
        min_length=min_length,
        max_length=max_length,
        req=req
    )
    if not HUMAN_MANDATORY_CHAR_REGEX.match(x):
        _ = req.gettext if req else dummy_gettext
        raise ValueError("Names require at least one 'word' character")


# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# 3(c). Search terms for simple near-alphanumeric SQL content, allowing
# wildcards.
# ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

RESTRICTED_SQL_SEARCH_LITERAL_CHAR = r"[A-Za-z0-9\- _%]"
# ... hyphens are meaningful in regexes, so escape it


def validate_restricted_sql_search_literal(
        x: str,
        req: Optional["CamcopsRequest"] = None,
        min_length: int = 0,
        max_length: int = StringLengths.SQL_SEARCH_LITERAL_MAX_LENGTH) -> None:
    """
    Validates a string that can be fairly broad, and can do SQL finding via
    wildcards such as ``%`` and ``_``, but should be syntactically safe in
    terms of HTML etc. It does not permit arbitrary strings; it's a subset of
    what might be possible in SQL.
    """
    validate_by_char_and_length(
        x,
        permitted_char_expression=RESTRICTED_SQL_SEARCH_LITERAL_CHAR,
        min_length=min_length,
        max_length=max_length,
        req=req
    )


# -----------------------------------------------------------------------------
# Level 4. Infinitely worrying.
# -----------------------------------------------------------------------------

# noinspection PyUnusedLocal
def validate_anything(x: str,
                      req: Optional["CamcopsRequest"] = None) -> None:
    """
    Lets anything through. May be unwise.
    """
    pass


# =============================================================================
# Specific well-known computer formats
# =============================================================================

# -----------------------------------------------------------------------------
# Base 64 encoding
# -----------------------------------------------------------------------------

# BASE64_REGEX = re.compile(
#     "^(?:[A-Za-z0-9+/]{4})*(?:[A-Za-z0-9+/]{2}==|[A-Za-z0-9+/]{3}=)?$"
#     # https://stackoverflow.com/questions/475074/regex-to-parse-or-validate-base64-data  # noqa
# )


# -----------------------------------------------------------------------------
# Email addresses
# -----------------------------------------------------------------------------

EMAIL_RE_COMPILED = re.compile(EMAIL_RE)


def validate_email(email: str,
                   req: Optional["CamcopsRequest"] = None) -> None:
    """
    Validate an e-mail address.

    Is this a valid e-mail address?

    We use the same validation system as our web form (which uses Colander's
    method plus a length constraint).
    """
    if (len(email) > StringLengths.EMAIL_ADDRESS_MAX_LEN or
            not EMAIL_RE_COMPILED.match(email)):
        _ = req.gettext if req else dummy_gettext
        raise ValueError(_("Invalid e-mail address"))


# -----------------------------------------------------------------------------
# IP addresses
# -----------------------------------------------------------------------------

def validate_ip_address(
        x: str,
        req: Optional["CamcopsRequest"] = None) -> None:
    """
    Validates an IP address.
    """
    # https://stackoverflow.com/questions/3462784/check-if-a-string-matches-an-ip-address-pattern-in-python  # noqa
    try:
        ipaddress.ip_address(x)
    except ValueError:
        _ = req.gettext if req else dummy_gettext
        raise ValueError(_("Invalid IP address"))


# -----------------------------------------------------------------------------
# URLs
# -----------------------------------------------------------------------------

# Per https://mathiasbynens.be/demo/url-regex, using @stephenhay's regex but
# restricted further.
VALID_REDIRECT_URL_REGEX = re.compile(r"^https?://[^\s/$.?#].[^\s]*$")


def validate_any_url(url: str,
                     req: Optional["CamcopsRequest"] = None) -> None:
    """
    Validates a URL. If valid, returns the URL; if not, returns ``default``.
    See https://stackoverflow.com/questions/22238090/validating-urls-in-python

    However, avoid this one. For example, a URL such as
    xxhttps://127.0.0.1:8088/ can trigger Chrome to launch ``xdg-open``.
    """
    log.warning("Avoid this validator! It allows open-this-file URLs!")
    result = urllib.parse.urlparse(url)
    if not result.scheme or not result.netloc:
        _ = req.gettext if req else dummy_gettext
        raise ValueError(_("Invalid URL"))


def validate_redirect_url(url: str,
                          req: Optional["CamcopsRequest"] = None) -> None:
    """
    Validates a URL. If valid, returns the URL; if not, returns ``default``.
    See https://stackoverflow.com/questions/22238090/validating-urls-in-python
    """
    if not VALID_REDIRECT_URL_REGEX.match(url):
        _ = req.gettext if req else dummy_gettext
        raise ValueError(_("Invalid redirection URL"))


# =============================================================================
# CamCOPS system-oriented names
# =============================================================================

# -----------------------------------------------------------------------------
# Group names
# -----------------------------------------------------------------------------

def validate_group_name(name: str,
                        req: Optional["CamcopsRequest"] = None) -> None:
    """
    Is the string a valid group name?

    Group descriptions can be anything, but group names shouldn't have odd
    characters in -- this greatly facilitates config file handling etc. (for
    example: no spaces, no commas).
    """
    validate_by_char_and_length(
        name,
        permitted_char_expression=ALPHANUM_UNDERSCORE_HYPHEN_CHAR,
        min_length=StringLengths.GROUP_NAME_MIN_LEN,
        max_length=StringLengths.GROUP_NAME_MAX_LEN,
        req=req
    )


# -----------------------------------------------------------------------------
# Usernames
# -----------------------------------------------------------------------------

def validate_username(name: str,
                      req: Optional["CamcopsRequest"] = None) -> None:
    """
    Is the string a valid user name?
    """
    validate_by_char_and_length(
        name,
        permitted_char_expression=ALPHANUM_COMMA_UNDERSCORE_HYPHEN_BRACE_CHAR,
        min_length=StringLengths.USERNAME_CAMCOPS_MIN_LEN,
        max_length=StringLengths.USERNAME_CAMCOPS_MAX_LEN,
        req=req
    )


# -----------------------------------------------------------------------------
# Devices
# -----------------------------------------------------------------------------

def validate_device_name(
        x: str, req: Optional["CamcopsRequest"] = None) -> None:
    """
    Validate a client device name -- the computer-oriented one, not the
    friendly one.
    """
    validate_by_char_and_length(
        x,
        permitted_char_expression=ALPHANUM_COMMA_UNDERSCORE_HYPHEN_BRACE_CHAR,
        min_length=1,
        max_length=StringLengths.DEVICE_NAME_MAX_LEN,
        req=req
    )


# -----------------------------------------------------------------------------
# Export recipients
# -----------------------------------------------------------------------------

def validate_export_recipient_name(
        x: str, req: Optional["CamcopsRequest"] = None) -> None:
    validate_by_char_and_length(
        x,
        permitted_char_expression=ALPHANUM_UNDERSCORE_CHAR,
        min_length=StringLengths.EXPORT_RECIPIENT_NAME_MIN_LEN,
        max_length=StringLengths.EXPORT_RECIPIENT_NAME_MAX_LEN,
        req=req
    )


# -----------------------------------------------------------------------------
# Passwords
# -----------------------------------------------------------------------------

def validate_new_password(
        x: str, req: Optional["CamcopsRequest"] = None) -> None:
    """
    Validate a proposed new password. Enforce our password policy.
    """
    _ = req.gettext if req else dummy_gettext
    if not x or not x.strip():
        raise ValueError(_("Passwords can't be blank"))
    if len(x) < MINIMUM_PASSWORD_LENGTH:
        raise ValueError(
            _("Passwords can't be shorter than {} characters").format(
                MINIMUM_PASSWORD_LENGTH
            )
        )
    # No maximum length, because we store a hash.
    # No other character limitations.
    if password_prohibited(x):
        raise ValueError(_("That password is used too commonly; try again"))


# -----------------------------------------------------------------------------
# HL7
# -----------------------------------------------------------------------------

def validate_hl7_id_type(
        x: str, req: Optional["CamcopsRequest"] = None) -> None:
    """
    Validate HL7 Identifier Type.
    """
    validate_by_char_and_length(
        x,
        permitted_char_expression=ALPHANUM_UNDERSCORE_HYPHEN_SPACE_CHAR,
        min_length=0,
        max_length=StringLengths.HL7_ID_TYPE_MAX_LEN,
        req=req
    )


def validate_hl7_aa(
        x: str, req: Optional["CamcopsRequest"] = None) -> None:
    """
    Validate HL7 Assigning Authority.
    """
    validate_by_char_and_length(
        x,
        permitted_char_expression=ALPHANUM_UNDERSCORE_HYPHEN_SPACE_CHAR,
        min_length=0,
        max_length=StringLengths.HL7_AA_MAX_LEN,
        req=req
    )


# -----------------------------------------------------------------------------
# Task table names
# -----------------------------------------------------------------------------

TASK_TABLENAME_REGEX = re.compile(
    anchor(ALPHA_CHAR, anchor_start=True, anchor_end=False) +
    # ... don't start with a number
    # ... and although tables can and do start with underscores, task tables
    #     don't.
    anchor(
        min_max_copies(ALPHANUM_UNDERSCORE_CHAR,
                       min_count=0,
                       max_count=StringLengths.TABLENAME_MAX_LEN - 1),
        anchor_start=False,
        anchor_end=True
    )
)


def validate_task_tablename(
        x: str,
        req: Optional["CamcopsRequest"] = None) -> None:
    """
    Validates a string that could be a task tablename.
    """
    if not TASK_TABLENAME_REGEX.match(x):
        _ = req.gettext if req else dummy_gettext
        raise ValueError(_(
            "Task table names must start with a letter, and contain only "
            "contain alphanumeric characters (A-Z, a-z, 0-9) or "
            "underscores (_)."
        ))
