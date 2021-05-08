#!/usr/bin/env python

"""
camcops_server/tasks/service_satisfaction.py

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

"""

from typing import Optional

from sqlalchemy.ext.declarative import declared_attr
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Integer, UnicodeText

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_html import tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    ZERO_TO_FOUR_CHECKER,
)
from camcops_server.cc_modules.cc_string import AS
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasPatientMixin,
)


# =============================================================================
# Abstract base class
# =============================================================================

class AbstractSatisfaction(object):
    # noinspection PyMethodParameters
    @declared_attr
    def service(cls) -> Column:
        return Column(
            "service", UnicodeText,
            comment="Clinical service being rated"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def rating(cls) -> Column:
        return CamcopsColumn(
            "rating", Integer,
            permitted_value_checker=ZERO_TO_FOUR_CHECKER,
            comment="Rating (0 very poor - 4 excellent)"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def good(cls) -> Column:
        return Column(
            "good", UnicodeText,
            comment="What has been good?"
        )

    # noinspection PyMethodParameters
    @declared_attr
    def bad(cls) -> Column:
        return Column(
            "bad", UnicodeText,
            comment="What could be improved?"
        )

    TASK_FIELDS = ["service", "rating", "good", "bad"]

    def is_complete(self) -> bool:
        # noinspection PyUnresolvedReferences
        return self.rating is not None and self.field_contents_valid()
        # ... self.field_contents_valid() is from Task, and we are a mixin

    def get_rating_text(self, req: CamcopsRequest) -> Optional[str]:
        ratingdict = {
            None: None,
            0: req.wappstring(AS.SATIS_RATING_A_PREFIX + "0"),
            1: req.wappstring(AS.SATIS_RATING_A_PREFIX + "1"),
            2: req.wappstring(AS.SATIS_RATING_A_PREFIX + "2"),
            3: req.wappstring(AS.SATIS_RATING_A_PREFIX + "3"),
            4: req.wappstring(AS.SATIS_RATING_A_PREFIX + "4"),
        }
        return get_from_dict(ratingdict, self.rating)

    def get_common_task_html(self,
                             req: CamcopsRequest,
                             rating_q: str,
                             good_q: str,
                             bad_q: str) -> str:
        if self.rating is not None:
            r = f"{self.rating}. {self.get_rating_text(req)}"
        else:
            r = None
        # noinspection PyUnresolvedReferences
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer</th>
                </tr>
                {tr_qa(req.wappstring(AS.SATIS_SERVICE_BEING_RATED),
                       self.service)}
                {tr_qa(f"{rating_q} {self.service}?", r)}
                {tr_qa(good_q, self.good)}
                {tr_qa(bad_q, self.bad)}
            </table>
        """
        # ... self.get_is_complete_tr() is from Task, and we are a mixin

    def get_task_html(self, req: CamcopsRequest) -> str:
        raise NotImplementedError("implement in subclass")


# =============================================================================
# PatientSatisfaction
# =============================================================================

class PatientSatisfaction(TaskHasPatientMixin, AbstractSatisfaction, Task):
    """
    Server implementation of the PatientSatisfaction task.
    """
    __tablename__ = "pt_satis"
    shortname = "PatientSatisfaction"

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Patient Satisfaction Scale")

    def get_task_html(self, req: CamcopsRequest) -> str:
        return self.get_common_task_html(
            req,
            req.wappstring(AS.SATIS_PT_RATING_Q),
            req.wappstring(AS.SATIS_GOOD_Q),
            req.wappstring(AS.SATIS_BAD_Q)
        )


# =============================================================================
# ReferrerSatisfactionGen
# =============================================================================

class ReferrerSatisfactionGen(AbstractSatisfaction, Task):
    """
    Server implementation of the ReferrerSatisfactionSurvey task.
    """
    __tablename__ = "ref_satis_gen"
    shortname = "ReferrerSatisfactionSurvey"

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Referrer Satisfaction Scale, survey")

    def get_task_html(self, req: CamcopsRequest) -> str:
        return self.get_common_task_html(
            req,
            req.wappstring(AS.SATIS_REF_GEN_RATING_Q),
            req.wappstring(AS.SATIS_GOOD_Q),
            req.wappstring(AS.SATIS_BAD_Q)
        )


# =============================================================================
# ReferrerSatisfactionSpec
# =============================================================================

class ReferrerSatisfactionSpec(TaskHasPatientMixin, AbstractSatisfaction,
                               Task):
    """
    Server implementation of the ReferrerSatisfactionSpecific task.
    """
    __tablename__ = "ref_satis_spec"
    shortname = "ReferrerSatisfactionSpecific"

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Referrer Satisfaction Scale, patient-specific")

    def get_task_html(self, req: CamcopsRequest) -> str:
        return self.get_common_task_html(
            req,
            req.wappstring(AS.SATIS_REF_SPEC_RATING_Q),
            req.wappstring(AS.SATIS_GOOD_Q),
            req.wappstring(AS.SATIS_BAD_Q)
        )
