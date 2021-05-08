#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_text.py

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

**Text used by the server, and translated.**

"""

from enum import auto, Enum, unique
from typing import TYPE_CHECKING

from pendulum import Date

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_request import CamcopsRequest


TERMS_CONDITIONS_UPDATE_DATE = Date(2020, 6, 29)


@unique
class SS(Enum):
    """
    Server string enumeration. All elements have type ``<enum 'SS'>`` and
    ``isinstance(SS.ANSWER, SS)`` is true.
    """
    ABNORMAL = auto()
    ABSENT = auto()
    ABSENT_OR_MILD = auto()
    ANONYMOUS_TASK = auto()
    ANSWER = auto()
    AUDITORY = auto()

    CATEGORY = auto()

    DISCLAIMER_TITLE = auto()
    DISCLAIMER_SUBTITLE = auto()
    DISCLAIMER_CONTENT = auto()
    DISCLAIMER_AGREE = auto()

    EVENT = auto()

    FALSE = auto()
    FEMALE = auto()

    GENERAL = auto()

    IF_APPLICABLE = auto()

    LOCATION = auto()

    MALE = auto()
    MEETS_CRITERIA = auto()
    MILD = auto()
    MILD_TO_MODERATE = auto()
    MODERATE = auto()
    MODERATELY_SEVERE = auto()
    MODERATE_TO_SEVERE = auto()

    NA = auto()
    NO = auto()
    NONE = auto()
    NORMAL = auto()
    NOTE = auto()

    PRESENT = auto()

    QUESTION = auto()

    SEVERE = auto()
    SEX = auto()

    TOTAL_SCORE = auto()
    TRUE = auto()

    UNKNOWN = auto()

    VERY_SEVERE = auto()
    VISUAL = auto()
    VOLUME_0_TO_1 = auto()

    YES = auto()


def server_string(req: "CamcopsRequest", w: SS) -> str:
    """
    Returns a translated server string.

    Use this mechanism when the same string is re-used in several places in
    the server (but not by the client).

    Args:
        req:
            a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        w:
            which string? A :class:`camcops_server.cc_modules.cc_text.SS`
            enumeration value

    Returns:
        the translated string

    """
    _ = req.gettext

    if w == SS.ABNORMAL:
        return _("Abnormal")
    elif w == SS.ABSENT:
        return _("Absent")
    elif w == SS.ABSENT_OR_MILD:
        return _("Absent or mild")
    elif w == SS.ANONYMOUS_TASK:
        return _("Anonymous task")
    elif w == SS.ANSWER:
        return _("Answer")
    elif w == SS.AUDITORY:
        return _("Auditory")

    elif w == SS.CATEGORY:
        return _("Category")

    elif w == SS.DISCLAIMER_TITLE:
        return _("TERMS AND CONDITIONS OF USE")
    elif w == SS.DISCLAIMER_SUBTITLE:
        return _("You must agree to the following terms and conditions in "
                 "order to use CamCOPS.")
    elif w == SS.DISCLAIMER_CONTENT:
        return _(
            "1. By using the Cambridge Cognitive and Psychiatric Assessment "
            "Kit application or web interface (“CamCOPS”), you are agreeing "
            "in full to these Terms and Conditions of Use. If you do not "
            "agree to these terms, do not use the software.\n"
            "\n"
            "2. Content that is original to CamCOPS is licensed as follows.\n"
            "\n"
            "CamCOPS is free software: you can redistribute it and/or modify "
            "it under the terms of the GNU General Public License as "
            "published by the Free Software Foundation, either version 3 of "
            "the License, or (at your option) any later version.\n"
            "\n"
            "CamCOPS is distributed in the hope that it will be useful, but "
            "WITHOUT ANY WARRANTY; without even the implied warranty of "
            "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
            "GNU General Public License for more details.\n"
            "\n"
            "You should have received a copy of the GNU General Public "
            "License along with CamCOPS. If not, see "
            "https://www.gnu.org/licenses/.\n"
            "\n"
            "3. Content created by others and distributed with CamCOPS may be "
            "in the public domain, or distributed under other licenses or "
            "permissions. THERE MAY BE CRITERIA THAT APPLY TO YOU THAT MEAN "
            "YOU ARE NOT PERMITTED TO USE SPECIFIC TASKS. IT IS YOUR "
            "RESPONSIBILITY TO CHECK THAT YOU ARE LEGALLY ENTITLED TO USE "
            "EACH TASK. You agree that the authors of CamCOPS are not "
            "responsible for any consequences that arise from your use of an "
            "unauthorized task.\n\n 4. While efforts have been made to ensure "
            "that CamCOPS is reliable and accurate, you agree that the "
            "authors and distributors of CamCOPS are not responsible for "
            "errors, omissions, or defects in the content, nor liable for any "
            "direct, indirect, incidental, special and/or consequential "
            "damages, in whole or in part, resulting from your use or any "
            "user’s use of or reliance upon its content.\n"
            "\n"
            "5. Content contained in or accessed through CamCOPS should not "
            "be relied upon for medical purposes in any way. This software is "
            "not designed for use by the general public. If medical advice is "
            "required you should seek expert medical assistance. You agree "
            "that you will not rely on this software for any medical "
            "purpose.\n"
            "\n"
            "6. Regarding the European Union Council Directive 93/42/EEC of "
            "14 June 1993 concerning medical devices (amended by further "
            "directives up to and including Directive 2007/47/EC of 5 "
            "September 2007) (“Medical Devices Directive”): CamCOPS is not "
            "intended for the diagnosis and/or monitoring of human disease. "
            "If it is used for such purposes, it must be used EXCLUSIVELY FOR "
            "CLINICAL INVESTIGATIONS in an appropriate setting by persons "
            "professionally qualified to do so. It has NOT undergone a "
            "conformity assessment under the Medical Devices Directive, and "
            "thus cannot be marketed or put into service as a medical device. "
            "You agree that you will not use it as a medical device.\n"
            "\n"
            "7. THIS SOFTWARE IS DESIGNED FOR USE BY QUALIFIED CLINICIANS "
            "ONLY. BY CONTINUING TO USE THIS SOFTWARE YOU ARE CONFIRMING THAT "
            "YOU ARE A QUALIFIED CLINICIAN, AND THAT YOU RETAIN "
            "RESPONSIBILITY FOR DIAGNOSIS AND MANAGEMENT.\n"
            "\n"
            "8. The CamCOPS server uses a single secure cookie for session "
            "authentication. The cookie is not used for any other purpose. "
            "It is deleted when you finish your session (it is a session cookie). "  # noqa
            "By using a CamCOPS server, you agree to this use of cookies.\n"
            "\n"
            "These terms and conditions were last revised on 2020-10-12."
        )
        # ... should match textconst::TERMS_CONDITIONS in the C++ app.
        #
        # ENSURE YOU ALSO UPDATE TERMS_CONDITIONS_UPDATE_DATE (above).
        #
        # [OLD CONSTRAINT: ... but don't include hyperlinks; they break the XML
        # reader]
    elif w == SS.DISCLAIMER_AGREE:
        return _("I agree to these terms and conditions")

    elif w == SS.EVENT:
        return _("Event")

    elif w == SS.FALSE:
        return _("False")
    elif w == SS.FEMALE:
        return _("Female")

    elif w == SS.GENERAL:
        return _("General")

    elif w == SS.IF_APPLICABLE:
        return _("If applicable")

    elif w == SS.LOCATION:
        return _("Location")

    elif w == SS.MALE:
        return _("Male")
    elif w == SS.MEETS_CRITERIA:
        return _("Meets criteria?")
    elif w == SS.MILD:
        return _("Mild")
    elif w == SS.MILD_TO_MODERATE:
        return _("Mild to moderate")
    elif w == SS.MODERATE:
        return _("Moderate")
    elif w == SS.MODERATELY_SEVERE:
        return _("Moderately severe")
    elif w == SS.MODERATE_TO_SEVERE:
        return _("Moderate to severe")

    elif w == SS.NA:
        return _("N/A")
    elif w == SS.NO:
        return _("No")
    elif w == SS.NONE:
        return _("None")
    elif w == SS.NORMAL:
        return _("Normal")
    elif w == SS.NOTE:
        return _("Note")

    elif w == SS.PRESENT:
        return _("Present")

    elif w == SS.QUESTION:
        return _("Question")

    elif w == SS.SEVERE:
        return _("Severe")
    elif w == SS.SEX:
        return _("Sex")

    elif w == SS.TOTAL_SCORE:
        return _("Total score")
    elif w == SS.TRUE:
        return _("True")

    elif w == SS.UNKNOWN:
        return _("Unknown")

    elif w == SS.VERY_SEVERE:
        return _("Very severe")
    elif w == SS.VISUAL:
        return _("Visual")
    elif w == SS.VOLUME_0_TO_1:
        return _("Volume (0–1)")

    elif w == SS.YES:
        return _("Yes")

    raise ValueError("Bad value passed to server_string")
