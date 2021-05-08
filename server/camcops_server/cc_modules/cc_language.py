#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_language.py

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

**Constants for languages/internationalization.**

This represents known languages (or, strictly, locales). Compare
``language.cpp`` on the client.

At present we don't make this user-configurable and arbitrary (e.g. via an XML
file to define languages), largely because it would a little complexity for the
user, and because there's not much point in adding a language to the server
without adding it for the client, too -- and the client needs recompiling.

Note that both Python locales (see e.g. ``locale.getlocale()``) and Qt use
underscores -- e.g. ``en_GB`` -- so we will too.

"""

# =============================================================================
# Languages
# =============================================================================

DANISH = "da_DK"
ENGLISH_UK = "en_GB"

POSSIBLE_LOCALES_WITH_DESCRIPTIONS = (
    # Default locale should be first (for auto-selecting <select> HTML).
    (ENGLISH_UK, "English (UK)"),  # DEFAULT LOCALE
    (DANISH, "Dansk"),
)


# =============================================================================
# Other constants
# =============================================================================

DEFAULT_LOCALE = ENGLISH_UK
GETTEXT_DOMAIN = "camcops"  # don't alter this
POSSIBLE_LOCALES = [_[0] for _ in POSSIBLE_LOCALES_WITH_DESCRIPTIONS]
