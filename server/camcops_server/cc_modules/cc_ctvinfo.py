#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_ctvinfo.py

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

**Information class used by the clinical text viewer (CTV) function.**

"""


# =============================================================================
# CtvInfo
# =============================================================================

class CtvInfo(object):
    """
    Snippet of information for incorporating into a CTV.
    """
    def __init__(self,
                 heading: str = None,
                 subheading: str = None,
                 description: str = None,
                 content: str = None,
                 skip_if_no_content: bool = True):
        """
        Args:
            heading: optional text used for heading
            subheading: optional text used for subheading
            description: optional text used for field description
            content: text
            skip_if_no_content: if True, no other fields will be printed
                unless content evaluates to True

        These will be NOT webified by the ClinicalTextView class, meaning

        (a) do it yourself if it's necessary, and
        (b) you can pass HTML formatting.
        """
        self.heading = heading
        self.subheading = subheading
        self.description = description
        self.content = content
        self.skip_if_no_content = skip_if_no_content


CTV_INCOMPLETE = [CtvInfo(description="Incomplete", skip_if_no_content=False)]
