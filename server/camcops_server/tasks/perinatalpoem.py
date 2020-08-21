#!/usr/bin/env python

"""
camcops_server/tasks/perinatalpoem.py

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
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.

===============================================================================

"""

import re
from typing import Dict, Generator, List, Tuple, Type

from cardinal_pythonlib.classes import classproperty
import pendulum
from pyramid.renderers import render_to_response
from pyramid.response import Response
from sqlalchemy.sql.expression import and_, column, select
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Integer, UnicodeText

from camcops_server.cc_modules.cc_constants import CssClass

from camcops_server.cc_modules.cc_html import (
    get_yes_no_none,
    subheading_spanning_two_columns,
    tr_qa,
)
from camcops_server.cc_modules.cc_report import (
    DateTimeFilteredReportMixin,
    PercentageSummaryReportMixin,
    Report,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    ZERO_TO_ONE_CHECKER,
    ONE_TO_TWO_CHECKER,
    ONE_TO_FIVE_CHECKER,
    ONE_TO_FOUR_CHECKER,
)
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
)
from camcops_server.cc_modules.cc_text import SS
from camcops_server.cc_modules.cc_tsv import TsvPage
from camcops_server.cc_modules.cc_unittest import DemoDatabaseTestCase


# =============================================================================
# Perinatal-POEM
# =============================================================================

class PerinatalPoem(Task):
    """
    Server implementation of the Perinatal-POEM task.
    """
    __tablename__ = "perinatal_poem"
    shortname = "Perinatal-POEM"
    provides_trackers = False

    # Field names
    FN_QA_RESPONDENT = "qa"
    FN_QB_SERVICE_TYPE = "qb"
    FN_Q1A_MH_FIRST_CONTACT = "q1a"
    FN_Q1B_MH_DISCHARGE = "q1b"
    FN_Q2A_STAFF_DID_NOT_COMMUNICATE = "q2a"
    FN_Q2B_STAFF_GAVE_RIGHT_SUPPORT = "q2b"
    FN_Q2C_HELP_NOT_QUICK_ENOUGH = "q2c"
    FN_Q2D_STAFF_LISTENED = "q2d"
    FN_Q2E_STAFF_DID_NOT_INVOLVE_ME = "q2e"
    FN_Q2F_SERVICE_PROVIDED_INFO = "q2f"
    FN_Q2G_STAFF_NOT_SENSITIVE_TO_ME = "q2g"
    FN_Q2H_STAFF_HELPED_ME_UNDERSTAND = "q2h"
    FN_Q2I_STAFF_NOT_SENSITIVE_TO_BABY = "q2i"
    FN_Q2J_STAFF_HELPED_MY_CONFIDENCE = "q2j"
    FN_Q2K_SERVICE_INVOLVED_OTHERS_HELPFULLY = "q2k"
    FN_Q2L_I_WOULD_RECOMMEND_SERVICE = "q2l"
    FN_Q3A_UNIT_CLEAN = "q3a"
    FN_Q3B_UNIT_NOT_GOOD_PLACE_TO_RECOVER = "q3b"
    FN_Q3C_UNIT_DID_NOT_PROVIDE_ACTIVITIES = "q3c"
    FN_Q3D_UNIT_GOOD_PLACE_FOR_BABY = "q3d"
    FN_Q3E_UNIT_SUPPORTED_FAMILY_FRIENDS_CONTACT = "q3e"
    FN_Q3F_FOOD_NOT_ACCEPTABLE = "q3f"
    FN_GENERAL_COMMENTS = "general_comments"
    FN_FUTURE_PARTICIPATION = "future_participation"
    FN_CONTACT_DETAILS = "contact_details"

    # Response values
    VAL_QA_PATIENT = 1
    VAL_QA_PARTNER_OTHER = 2

    VAL_QB_INPATIENT = 1  # inpatient = MBU = mother and baby unit
    VAL_QB_COMMUNITY = 2

    VAL_Q1_VERY_WELL = 1
    VAL_Q1_WELL = 2
    VAL_Q1_UNWELL = 3
    VAL_Q1_VERY_UNWELL = 4
    VAL_Q1_EXTREMELY_UNWELL = 5
    _MH_KEY = (
        f"({VAL_Q1_VERY_WELL} very well, {VAL_Q1_WELL} well, "
        f"{VAL_Q1_UNWELL} unwell, {VAL_Q1_VERY_UNWELL} very unwell, "
        f"{VAL_Q1_EXTREMELY_UNWELL} extremely unwell)"
    )

    VAL_STRONGLY_AGREE = 1
    VAL_AGREE = 2
    VAL_DISAGREE = 3
    VAL_STRONGLY_DISAGREE = 4
    _AGREE_KEY = (
        f"({VAL_STRONGLY_AGREE} strongly agree, {VAL_AGREE} agree, "
        f"{VAL_DISAGREE} disagree, {VAL_STRONGLY_DISAGREE} strongly disagree)"
    )

    _INPATIENT_ONLY = "[Inpatient services only]"

    YES_INT = 1
    NO_INT = 0

    # -------------------------------------------------------------------------
    # Fields
    # -------------------------------------------------------------------------
    qa = CamcopsColumn(
        FN_QA_RESPONDENT, Integer,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
        comment=(
            f"Question A: Is the respondent the patient ({VAL_QA_PATIENT}) "
            f"or other ({VAL_QA_PARTNER_OTHER})?"
        )
    )
    qb = CamcopsColumn(
        FN_QB_SERVICE_TYPE, Integer,
        permitted_value_checker=ONE_TO_TWO_CHECKER,
        comment=(
            f"Question B: Was the service type inpatient [mother-and-baby "
            f"unit, MBU] ({VAL_QB_INPATIENT}) or "
            f"community ({VAL_QB_COMMUNITY})?"
        )
    )

    q1a = CamcopsColumn(
        FN_Q1A_MH_FIRST_CONTACT, Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment=f"Q1A: mental health at first contact {_MH_KEY}"
    )
    q1b = CamcopsColumn(
        FN_Q1B_MH_DISCHARGE, Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment=f"Q1B: mental health at discharge {_MH_KEY}"
    )

    q2a = CamcopsColumn(
        FN_Q2A_STAFF_DID_NOT_COMMUNICATE, Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment=f"Q2a: staff didn't communicate with others {_AGREE_KEY}"
    )
    q2b = CamcopsColumn(
        FN_Q2B_STAFF_GAVE_RIGHT_SUPPORT, Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment=f"Q2b: Staff gave right amount of support {_AGREE_KEY}"
    )
    q2c = CamcopsColumn(
        FN_Q2C_HELP_NOT_QUICK_ENOUGH, Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment=f"Q2c: Help not quick enough after referral {_AGREE_KEY}"
    )
    q2d = CamcopsColumn(
        FN_Q2D_STAFF_LISTENED, Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment=f"Q2d: Staff listened/understood {_AGREE_KEY}"
    )

    q2e = CamcopsColumn(
        FN_Q2E_STAFF_DID_NOT_INVOLVE_ME, Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment=f"Q2e: Staff didn't involve pt enough {_AGREE_KEY}"
    )
    q2f = CamcopsColumn(
        FN_Q2F_SERVICE_PROVIDED_INFO, Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment=f"Q2f: Service provided information {_AGREE_KEY}"
    )
    q2g = CamcopsColumn(
        FN_Q2G_STAFF_NOT_SENSITIVE_TO_ME, Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment=f"Q2g: Staff not very sensitive to pt {_AGREE_KEY}"
    )
    q2h = CamcopsColumn(
        FN_Q2H_STAFF_HELPED_ME_UNDERSTAND, Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment=f"Q2h: Staff helped understanding of illness {_AGREE_KEY}"
    )

    q2i = CamcopsColumn(
        FN_Q2I_STAFF_NOT_SENSITIVE_TO_BABY, Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment=f"Q2i: Staff not very sensitive to baby {_AGREE_KEY}"
    )
    q2j = CamcopsColumn(
        FN_Q2J_STAFF_HELPED_MY_CONFIDENCE, Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment=f"Q2j: Staff helped confidence re baby {_AGREE_KEY}"
    )
    q2k = CamcopsColumn(
        FN_Q2K_SERVICE_INVOLVED_OTHERS_HELPFULLY, Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment=f"Q2k: Service involved others helpfully {_AGREE_KEY}"
    )
    q2l = CamcopsColumn(
        FN_Q2L_I_WOULD_RECOMMEND_SERVICE, Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment=f"Q2l: Would recommend service {_AGREE_KEY}"
    )

    q3a = CamcopsColumn(
        FN_Q3A_UNIT_CLEAN, Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment=f"Q3a: MBU clean {_AGREE_KEY} {_INPATIENT_ONLY}"
    )
    q3b = CamcopsColumn(
        FN_Q3B_UNIT_NOT_GOOD_PLACE_TO_RECOVER, Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment=f"Q3b: MBU not a good place to recover "
                f"{_AGREE_KEY} {_INPATIENT_ONLY}"
    )
    q3c = CamcopsColumn(
        FN_Q3C_UNIT_DID_NOT_PROVIDE_ACTIVITIES, Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment=f"Q3c: MBU did not provide helpful activities "
                f"{_AGREE_KEY} {_INPATIENT_ONLY}"
    )
    q3d = CamcopsColumn(
        FN_Q3D_UNIT_GOOD_PLACE_FOR_BABY, Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment=f"Q3d: MBU a good place for baby to be with pt "
                f"{_AGREE_KEY} {_INPATIENT_ONLY}"
    )
    q3e = CamcopsColumn(
        FN_Q3E_UNIT_SUPPORTED_FAMILY_FRIENDS_CONTACT, Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment=f"Q3e: MBU supported contact with family/friends "
                f"{_AGREE_KEY} {_INPATIENT_ONLY}"
    )
    q3f = CamcopsColumn(
        FN_Q3F_FOOD_NOT_ACCEPTABLE, Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment=f"Q3f: Food not acceptable {_AGREE_KEY} {_INPATIENT_ONLY}"
    )

    general_comments = Column(
        FN_GENERAL_COMMENTS, UnicodeText,
        comment="General comments"
    )
    future_participation = CamcopsColumn(
        FN_FUTURE_PARTICIPATION, Integer,
        permitted_value_checker=ZERO_TO_ONE_CHECKER,
        comment=f"Willing to participate in future studies "
                f"({YES_INT} yes, {NO_INT} no)"
    )
    contact_details = Column(
        FN_CONTACT_DETAILS, UnicodeText,
        comment="Contact details"
    )

    # -------------------------------------------------------------------------
    # Fieldname collections
    # -------------------------------------------------------------------------
    REQUIRED_ALWAYS = [
        FN_QA_RESPONDENT,
        FN_QB_SERVICE_TYPE,
        FN_Q1A_MH_FIRST_CONTACT,
        FN_Q1B_MH_DISCHARGE,
        FN_Q2A_STAFF_DID_NOT_COMMUNICATE,
        FN_Q2B_STAFF_GAVE_RIGHT_SUPPORT,
        FN_Q2C_HELP_NOT_QUICK_ENOUGH,
        FN_Q2D_STAFF_LISTENED,
        FN_Q2E_STAFF_DID_NOT_INVOLVE_ME,
        FN_Q2F_SERVICE_PROVIDED_INFO,
        FN_Q2G_STAFF_NOT_SENSITIVE_TO_ME,
        FN_Q2H_STAFF_HELPED_ME_UNDERSTAND,
        FN_Q2I_STAFF_NOT_SENSITIVE_TO_BABY,
        FN_Q2J_STAFF_HELPED_MY_CONFIDENCE,
        FN_Q2K_SERVICE_INVOLVED_OTHERS_HELPFULLY,
        FN_Q2L_I_WOULD_RECOMMEND_SERVICE,
        # not FN_GENERAL_COMMENTS,
        FN_FUTURE_PARTICIPATION,
        # not FN_CONTACT_DETAILS,
    ]
    REQUIRED_INPATIENT = [
        FN_Q3A_UNIT_CLEAN,
        FN_Q3B_UNIT_NOT_GOOD_PLACE_TO_RECOVER,
        FN_Q3C_UNIT_DID_NOT_PROVIDE_ACTIVITIES,
        FN_Q3D_UNIT_GOOD_PLACE_FOR_BABY,
        FN_Q3E_UNIT_SUPPORTED_FAMILY_FRIENDS_CONTACT,
        FN_Q3F_FOOD_NOT_ACCEPTABLE,
    ]
    Q1_FIELDS = [
        FN_Q1A_MH_FIRST_CONTACT,
        FN_Q1B_MH_DISCHARGE,
    ]
    Q2_FIELDS = [
        FN_Q2A_STAFF_DID_NOT_COMMUNICATE,
        FN_Q2B_STAFF_GAVE_RIGHT_SUPPORT,
        FN_Q2C_HELP_NOT_QUICK_ENOUGH,
        FN_Q2D_STAFF_LISTENED,
        FN_Q2E_STAFF_DID_NOT_INVOLVE_ME,
        FN_Q2F_SERVICE_PROVIDED_INFO,
        FN_Q2G_STAFF_NOT_SENSITIVE_TO_ME,
        FN_Q2H_STAFF_HELPED_ME_UNDERSTAND,
        FN_Q2I_STAFF_NOT_SENSITIVE_TO_BABY,
        FN_Q2J_STAFF_HELPED_MY_CONFIDENCE,
        FN_Q2K_SERVICE_INVOLVED_OTHERS_HELPFULLY,
        FN_Q2L_I_WOULD_RECOMMEND_SERVICE,
    ]
    Q3_FIELDS = REQUIRED_INPATIENT

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Perinatal Patient-rated Outcome and Experience Measure")

    def was_inpatient(self) -> bool:
        return self.qb == self.VAL_QB_INPATIENT

    def respondent_not_patient(self) -> bool:
        return self.qa == self.VAL_QA_PARTNER_OTHER

    def offering_participation(self) -> bool:
        return self.future_participation == self.YES_INT

    def is_complete(self) -> bool:
        if self.any_fields_none(self.REQUIRED_ALWAYS):
            return False
        if (self.was_inpatient() and
                self.any_fields_none(self.REQUIRED_INPATIENT)):
            return False
        if not self.field_contents_valid():
            return False
        return True

    def get_qa_options(self, req: CamcopsRequest) -> List[str]:
        options = [self.wxstring(req, f"qa_a{o}") for o in range(
            self.VAL_QA_PATIENT,
            self.VAL_QA_PARTNER_OTHER + 1)]

        return options

    def get_qb_options(self, req: CamcopsRequest) -> List[str]:
        options = [self.wxstring(req, f"qb_a{o}") for o in range(
            self.VAL_QB_INPATIENT,
            self.VAL_QB_COMMUNITY + 1)]

        return options

    def get_q1_options(self, req: CamcopsRequest) -> List[str]:
        options = [self.wxstring(req, f"q1_a{o}") for o in range(
            self.VAL_Q1_VERY_WELL,
            self.VAL_Q1_EXTREMELY_UNWELL + 1)]

        return options

    def get_agree_options(self, req: CamcopsRequest) -> List[str]:
        options = [self.wxstring(req, f"agreement_a{o}") for o in range(
            self.VAL_STRONGLY_AGREE,
            self.VAL_STRONGLY_DISAGREE + 1)]

        return options

    @staticmethod
    def get_yn_options(req: CamcopsRequest) -> List[str]:
        return [req.sstring(SS.NO), req.sstring(SS.YES)]

    def get_task_html(self, req: CamcopsRequest) -> str:
        def loadvalues(_dict: Dict[int, str], _first: int, _last: int,
                       _xstringprefix: str) -> None:
            for val in range(_first, _last + 1):
                _dict[val] = (
                    f"{val} — {self.wxstring(req, f'{_xstringprefix}{val}')}"
                )

        respondent_dict = {}  # type: Dict[int, str]
        loadvalues(respondent_dict, self.VAL_QA_PATIENT,
                   self.VAL_QA_PARTNER_OTHER, "qa_a")
        service_dict = {}  # type: Dict[int, str]
        loadvalues(service_dict, self.VAL_QB_INPATIENT,
                   self.VAL_QB_COMMUNITY, "qb_a")
        mh_dict = {}  # type: Dict[int, str]
        loadvalues(mh_dict, self.VAL_Q1_VERY_WELL,
                   self.VAL_Q1_EXTREMELY_UNWELL, "q1_a")
        agree_dict = {}  # type: Dict[int, str]
        loadvalues(agree_dict, self.VAL_STRONGLY_AGREE,
                   self.VAL_STRONGLY_DISAGREE, "agreement_a")

        q_a_list = []  # type: List[str]

        def addqa(_fieldname: str, _valuedict: Dict[int, str]) -> None:
            xstringname = _fieldname + "_q"
            q_a_list.append(
                tr_qa(self.xstring(req, xstringname),  # not wxstring
                      get_from_dict(_valuedict, getattr(self, _fieldname)))
            )

        def subheading(_xstringname: str) -> None:
            q_a_list.append(subheading_spanning_two_columns(
                self.wxstring(req, _xstringname)))

        # Preamble
        addqa(self.FN_QA_RESPONDENT, respondent_dict)
        addqa(self.FN_QB_SERVICE_TYPE, service_dict)
        # The bulk
        subheading("q1_stem")
        for fieldname in self.Q1_FIELDS:
            addqa(fieldname, mh_dict)
        subheading("q2_stem")
        for fieldname in self.Q2_FIELDS:
            addqa(fieldname, agree_dict)
        if self.was_inpatient():
            subheading("q3_stem")
            for fieldname in self.Q3_FIELDS:
                addqa(fieldname, agree_dict)
        # General
        q_a_list.append(subheading_spanning_two_columns(
            req.sstring(SS.GENERAL)))
        q_a_list.append(tr_qa(
            self.wxstring(req, "general_comments_q"),
            self.general_comments
        ))
        q_a_list.append(tr_qa(
            self.wxstring(req, "participation_q"),
            get_yes_no_none(req, self.future_participation)
        ))
        if self.offering_participation():
            q_a_list.append(tr_qa(
                self.wxstring(req, "contact_details_q"),
                self.contact_details
            ))

        q_a = "\n".join(q_a_list)
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="60%">Question</th>
                    <th width="40%">Answer</th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
            </div>
        """

    # No SNOMED codes for Perinatal-POEM.


# =============================================================================
# Reports
# =============================================================================

class PerinatalPoemReportTableConfig(object):
    def __init__(self,
                 heading: str,
                 column_headings: List[str],
                 fieldnames: List[str],
                 min_answer: int = 0,
                 xstring_format: str = "{}_q") -> None:
        self.heading = heading
        self.column_headings = column_headings
        self.fieldnames = fieldnames
        self.min_answer = min_answer
        self.xstring_format = xstring_format


class PerinatalPoemReportTable(object):
    def __init__(self, req: "CamcopsRequest",
                 heading: str,
                 column_headings: List[str],
                 rows: List[List[str]]) -> None:
        _ = req.gettext
        self.heading = heading

        common_headings = [_("Question"), _("Total responses")]
        self.column_headings = common_headings + column_headings
        self.rows = rows


class PerinatalPoemReport(DateTimeFilteredReportMixin, Report,
                          PercentageSummaryReportMixin):
    """
    Provides a summary of each question, x% of people said each response etc.
    Then a summary of the comments.
    """
    HTML_TAG_RE = re.compile(r'<[^>]+>')

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.task = PerinatalPoem()  # dummy task, never written to DB

    @classproperty
    def task_class(self) -> Type["Task"]:
        return PerinatalPoem

    # noinspection PyMethodParameters
    @classproperty
    def report_id(cls) -> str:
        return "perinatal_poem"

    @classmethod
    def title(cls, req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Perinatal-POEM — Question summaries")

    # noinspection PyMethodParameters
    @classproperty
    def superuser_only(cls) -> bool:
        return False

    def render_html(self, req: "CamcopsRequest") -> Response:
        return render_to_response(
            "perinatal_poem_report.mako",
            dict(
                title=self.title(req),
                report_id=self.report_id,
                start_datetime=self.start_datetime,
                end_datetime=self.end_datetime,
                tables=self._get_html_tables(req),
                comments=self._get_comments(req)
            ),
            request=req
        )

    def get_tsv_pages(self, req: "CamcopsRequest") -> List[TsvPage]:
        _ = req.gettext

        pages = []

        for table in self._get_tsv_tables(req):
            pages.append(
                self.get_tsv_page(
                    name=table.heading,
                    column_names=table.column_headings,
                    rows=table.rows
                )
            )

        pages.append(
            self.get_tsv_page(
                name=_("Comments"),
                column_names=[_("Comment")],
                rows=self._get_comment_rows(req)
            )
        )

        return pages

    def _get_html_tables(
            self, req: "CamcopsRequest") -> List["PerinatalPoemReportTable"]:

        return [
            self._get_html_table(req, config)
            for config in self._get_table_configs(req)
        ]

    def _get_tsv_tables(
            self, req: "CamcopsRequest") -> List["PerinatalPoemReportTable"]:

        return [
            self._get_tsv_table(req, config)
            for config in self._get_table_configs(req)
        ]

    def _get_table_configs(
            self,
            req: "CamcopsRequest") -> List["PerinatalPoemReportTableConfig"]:
        return [
            PerinatalPoemReportTableConfig(
                heading=self.task.xstring(req, "qa_q"),
                column_headings=self.task.get_qa_options(req),
                fieldnames=["qa"],
                min_answer=1
            ),
            PerinatalPoemReportTableConfig(
                heading=self.task.xstring(req, "qb_q"),
                column_headings=self.task.get_qb_options(req),
                fieldnames=["qb"],
                min_answer=1
            ),
            PerinatalPoemReportTableConfig(
                heading=self.task.xstring(req, "q1_stem"),
                column_headings=self.task.get_q1_options(req),
                fieldnames=PerinatalPoem.Q1_FIELDS,
                min_answer=1
            ),
            PerinatalPoemReportTableConfig(
                heading=self.task.xstring(req, "q2_stem"),
                column_headings=self.task.get_agree_options(req),
                fieldnames=PerinatalPoem.Q2_FIELDS,
                min_answer=1
            ),
            PerinatalPoemReportTableConfig(
                heading=self.task.xstring(req, "q3_stem"),
                column_headings=self.task.get_agree_options(req),
                fieldnames=PerinatalPoem.Q3_FIELDS,
                min_answer=1
            ),
            PerinatalPoemReportTableConfig(
                heading=self.task.xstring(req, "participation_q"),
                column_headings=self.task.get_yn_options(req),
                fieldnames=["future_participation"],
                xstring_format="participation_q"
            ),
        ]

    def _get_html_table(
            self, req: "CamcopsRequest",
            config: PerinatalPoemReportTableConfig
    ) -> PerinatalPoemReportTable:
        column_dict = {}

        for fieldname in config.fieldnames:
            column_dict[fieldname] = self.task.xstring(
                req, config.xstring_format.format(fieldname)
            )

        rows = self.get_percentage_summaries(
            req,
            column_dict=column_dict,
            num_answers=len(config.column_headings),
            cell_format="{0:.1f}%",
            min_answer=config.min_answer
        )

        return PerinatalPoemReportTable(
            req,
            heading=config.heading,
            column_headings=config.column_headings,
            rows=rows
        )

    def _get_tsv_table(
            self, req: "CamcopsRequest",
            config: PerinatalPoemReportTableConfig
    ) -> PerinatalPoemReportTable:
        column_dict = {}

        for fieldname in config.fieldnames:
            column_dict[fieldname] = self._strip_tags(
                self.task.xstring(
                    req, config.xstring_format.format(fieldname)
                )
            )

        rows = self.get_percentage_summaries(
            req,
            column_dict=column_dict,
            num_answers=len(config.column_headings),
            min_answer=config.min_answer
        )

        return PerinatalPoemReportTable(
            req,
            heading=config.heading,
            column_headings=config.column_headings,
            rows=rows
        )

    def _strip_tags(self, text: str) -> str:
        return self.HTML_TAG_RE.sub('', text)

    def _get_comment_rows(self, req: "CamcopsRequest") -> List[Tuple[str]]:
        """
        A list of all the additional comments
        """

        wheres = [
            column("general_comments").isnot(None)
        ]

        self.add_task_report_filters(wheres)

        # noinspection PyUnresolvedReferences
        query = (
            select([
                column("general_comments"),
            ])
            .select_from(self.task.__table__)
            .where(and_(*wheres))
        )

        comment_rows = []

        for result in req.dbsession.execute(query).fetchall():
            comment_rows.append(result)

        return comment_rows

    def _get_comments(self, req: "CamcopsRequest") -> List[str]:
        """
        A list of all the additional comments.
        """
        return [x[0] for x in self._get_comment_rows(req)]


# =============================================================================
# Unit tests
# =============================================================================

class PerinatalPoemReportTestCase(DemoDatabaseTestCase):
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.id_sequence = self.get_id()

    def setUp(self) -> None:
        super().setUp()

        self.report = PerinatalPoemReport()

        # Really only needed for tests
        self.report.start_datetime = None
        self.report.end_datetime = None

    @staticmethod
    def get_id() -> Generator[int, None, None]:
        i = 1

        while True:
            yield i
            i += 1

    def create_task(self, **kwargs) -> None:
        task = PerinatalPoem()
        self.apply_standard_task_fields(task)
        task.id = next(self.id_sequence)

        era = kwargs.pop('era', None)
        if era is not None:
            task.when_created = pendulum.parse(era)

        for name, value in kwargs.items():
            setattr(task, name, value)

        self.dbsession.add(task)


class PerinatalPoemReportTests(PerinatalPoemReportTestCase):
    """
    Most of the base class tested in APEQCPFT Perinatal so just some basic
    sanity checking here
    """

    def create_tasks(self):
        self.create_task(general_comments="comment 1")
        self.create_task(general_comments="comment 2")
        self.create_task(general_comments="comment 3")

        self.dbsession.commit()

    def test_qa_rows_counts(self) -> None:
        tables = self.report._get_html_tables(self.req)

        rows = tables[0].rows

        self.assertEqual(len(rows), 1)
        self.assertEqual(len(rows[0]), 4)

    def test_qb_rows_counts(self) -> None:
        tables = self.report._get_html_tables(self.req)

        rows = tables[1].rows

        self.assertEqual(len(rows), 1)
        self.assertEqual(len(rows[0]), 4)

    def test_q1_rows_counts(self) -> None:
        tables = self.report._get_html_tables(self.req)

        rows = tables[2].rows

        self.assertEqual(len(rows), 2)
        self.assertEqual(len(rows[0]), 7)

    def test_q2_rows_counts(self) -> None:
        tables = self.report._get_html_tables(self.req)

        rows = tables[3].rows

        self.assertEqual(len(rows), 12)
        self.assertEqual(len(rows[0]), 6)

    def test_q3_rows_counts(self) -> None:
        tables = self.report._get_html_tables(self.req)

        rows = tables[4].rows

        self.assertEqual(len(rows), 6)
        self.assertEqual(len(rows[0]), 6)

    def test_participation_rows_counts(self) -> None:
        tables = self.report._get_html_tables(self.req)

        rows = tables[5].rows

        self.assertEqual(len(rows), 1)
        self.assertEqual(len(rows[0]), 4)

    def test_comments(self) -> None:
        expected_comments = [
            "comment 1", "comment 2", "comment 3",
        ]
        comments = self.report._get_comments(self.req)
        self.assertEqual(comments, expected_comments)


class PerinatalPoemReportDateRangeTests(PerinatalPoemReportTestCase):
    def create_tasks(self) -> None:
        self.create_task(general_comments="comments 1",
                         era="2018-10-01T00:00:00.000000+00:00")
        self.create_task(general_comments="comments 2",
                         era="2018-10-02T00:00:00.000000+00:00")
        self.create_task(general_comments="comments 3",
                         era="2018-10-03T00:00:00.000000+00:00")
        self.create_task(general_comments="comments 4",
                         era="2018-10-04T00:00:00.000000+00:00")
        self.create_task(general_comments="comments 5",
                         era="2018-10-05T00:00:00.000000+00:00")
        self.dbsession.commit()

    def test_comments_filtered_by_date(self) -> None:
        self.report.start_datetime = "2018-10-02T00:00:00.000000+00:00"
        self.report.end_datetime = "2018-10-05T00:00:00.000000+00:00"

        comments = self.report._get_comments(self.req)
        self.assertEqual(len(comments), 3)

        self.assertEqual(comments[0], "comments 2")
        self.assertEqual(comments[1], "comments 3")
        self.assertEqual(comments[2], "comments 4")
