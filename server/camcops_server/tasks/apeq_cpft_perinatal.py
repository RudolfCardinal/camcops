#!/usr/bin/env python

"""
camcops_server/tasks/apeq_cpft_perinatal.py

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

from typing import Dict, Generator, List, Optional, Tuple, Type

from cardinal_pythonlib.classes import classproperty

import pendulum
from pyramid.renderers import render_to_response
from pyramid.response import Response
from sqlalchemy.sql.expression import and_, column, select
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Integer, UnicodeText

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_html import tr_qa
from camcops_server.cc_modules.cc_pyramid import ViewParam
from camcops_server.cc_modules.cc_report import (
    DateTimeFilteredReportMixin,
    PercentageSummaryReportMixin,
    Report,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
    ZERO_TO_FIVE_CHECKER,
    ZERO_TO_TWO_CHECKER,
)
from camcops_server.cc_modules.cc_task import Task
from camcops_server.cc_modules.cc_tsv import TsvPage
from camcops_server.cc_modules.cc_unittest import DemoDatabaseTestCase


# =============================================================================
# APEQCPFTPerinatal
# =============================================================================

class APEQCPFTPerinatal(Task):
    """
    Server implementation of the APEQ-CPFT-Perinatal task.
    """
    __tablename__ = "apeq_cpft_perinatal"
    shortname = "APEQ-CPFT-Perinatal"

    FIRST_MAIN_Q = 1
    LAST_MAIN_Q = 6
    FN_QPREFIX = "q"
    MAIN_EXPLANATION = " (0 no, 1 yes to some extent, 2 yes)"

    q1 = CamcopsColumn(
        "q1", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Q1. Treated with respect/dignity" + MAIN_EXPLANATION
    )
    q2 = CamcopsColumn(
        "q2", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Q2. Felt listened to" + MAIN_EXPLANATION
    )
    q3 = CamcopsColumn(
        "q3", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Q3. Needs were understood" + MAIN_EXPLANATION
    )
    q4 = CamcopsColumn(
        "q4", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Q4. Given info about team" + MAIN_EXPLANATION
    )
    q5 = CamcopsColumn(
        "q5", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Q5. Family considered/included" + MAIN_EXPLANATION
    )
    q6 = CamcopsColumn(
        "q6", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Q6. Views on treatment taken into account" + MAIN_EXPLANATION
    )
    ff_rating = CamcopsColumn(
        "ff_rating", Integer,
        permitted_value_checker=ZERO_TO_FIVE_CHECKER,
        comment="How likely to recommend service to friends and family "
                "(0 don't know, 1 extremely unlikely, 2 unlikely, "
                "3 neither likely nor unlikely, 4 likely, 5 extremely likely)"
    )
    ff_why = Column(
        "ff_why", UnicodeText,
        comment="Why was friends/family rating given as it was?"
    )
    comments = Column(
        "comments", UnicodeText,
        comment="General comments"
    )

    REQUIRED_FIELDS = ["q1", "q2", "q3", "q4", "q5", "q6", "ff_rating"]

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Assessment Patient Experience Questionnaire for "
                 "CPFT Perinatal Services")

    def is_complete(self) -> bool:
        return self.all_fields_not_none(self.REQUIRED_FIELDS)

    def get_task_html(self, req: CamcopsRequest) -> str:
        options_main = {None: "?"}  # type: Dict[Optional[int], str]
        for o in range(0, 2 + 1):
            options_main[o] = self.wxstring(req, f"main_a{o}")
        options_ff = {None: "?"}  # type: Dict[Optional[int], str]
        for o in range(0, 5 + 1):
            options_ff[o] = self.wxstring(req, f"ff_a{o}")

        qlines = []  # type: List[str]
        for qnum in range(self.FIRST_MAIN_Q, self.LAST_MAIN_Q + 1):
            xstring_attr_name = f"q{qnum}"
            qlines.append(tr_qa(
                self.wxstring(req, xstring_attr_name),
                options_main.get(getattr(self, xstring_attr_name))))
        q_a = "".join(qlines)
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
                {tr_qa(self.wxstring(req, "q_ff_rating"),
                       options_ff.get(self.ff_rating))}
                {tr_qa(self.wxstring(req, "q_ff_why"),
                       self.ff_why or "")}
                {tr_qa(self.wxstring(req, "q_comments"),
                       self.comments or "")}
            </table>
        """

    def get_main_options(self, req: "CamcopsRequest") -> List[str]:
        options = []

        for n in range(0, 2 + 1):
            options.append(self.wxstring(req, f"main_a{n}"))

        return options

    def get_ff_options(self, req: "CamcopsRequest") -> List[str]:
        options = []

        for n in range(0, 5 + 1):
            options.append(self.wxstring(req, f"ff_a{n}"))

        return options


# =============================================================================
# Reports
# =============================================================================

class APEQCPFTPerinatalReport(DateTimeFilteredReportMixin, Report,
                              PercentageSummaryReportMixin):
    """
    Provides a summary of each question, x% of people said each response etc.
    Then a summary of the comments.
    """
    COL_Q = 0
    COL_TOTAL = 1
    COL_RESPONSE_START = 2

    COL_FF_WHY = 1

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.task = APEQCPFTPerinatal()  # dummy task, never written to DB

    @classproperty
    def task_class(self) -> Type["Task"]:
        return APEQCPFTPerinatal

    # noinspection PyMethodParameters
    @classproperty
    def report_id(cls) -> str:
        return "apeq_cpft_perinatal"

    @classmethod
    def title(cls, req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("APEQ CPFT Perinatal — Question summaries")

    # noinspection PyMethodParameters
    @classproperty
    def superuser_only(cls) -> bool:
        return False

    @classmethod
    def get_specific_http_query_keys(cls) -> List[str]:
        return [
            ViewParam.START_DATETIME,
            ViewParam.END_DATETIME,
        ]

    def render_html(self, req: "CamcopsRequest") -> Response:
        cell_format = "{0:.1f}%"

        return render_to_response(
            "apeq_cpft_perinatal_report.mako",
            dict(
                title=self.title(req),
                report_id=self.report_id,
                start_datetime=self.start_datetime,
                end_datetime=self.end_datetime,
                main_column_headings=self._get_main_column_headings(req),
                main_rows=self._get_main_rows(req, cell_format=cell_format),
                ff_column_headings=self._get_ff_column_headings(req),
                ff_rows=self._get_ff_rows(req, cell_format=cell_format),
                ff_why_rows=self._get_ff_why_rows(req),
                comments=self._get_comments(req)
            ),
            request=req
        )

    def get_tsv_pages(self, req: "CamcopsRequest") -> List[TsvPage]:
        _ = req.gettext

        main_page = self.get_tsv_page(
            name=_("Main questions"),
            column_names=self._get_main_column_headings(req),
            rows=self._get_main_rows(req)
        )
        ff_page = self.get_tsv_page(
            name=_("Friends and family question"),
            column_names=self._get_ff_column_headings(req),
            rows=self._get_ff_rows(req)
        )
        ff_why_page = self.get_tsv_page(
            name=_("Reasons given for the above responses"),
            column_names=[_("Response"), _("Reason")],
            rows=self._get_ff_why_rows(req)
        )
        comments_page = self.get_tsv_page(
            name=_("Comments"),
            column_names=[_("Comment")],
            rows=self._get_comment_rows(req)
        )

        return [main_page, ff_page, ff_why_page, comments_page]

    def _get_main_column_headings(self, req: "CamcopsRequest") -> List[str]:
        _ = req.gettext
        names = [_("Question"),
                 _("Total responses")] + self.task.get_main_options(req)

        return names

    def _get_main_rows(self, req: "CamcopsRequest",
                       cell_format: str = "{}") -> List[List[str]]:
        """
        Percentage of people who answered x for each question
        """
        column_dict = {}

        qnums = range(self.task.FIRST_MAIN_Q, self.task.LAST_MAIN_Q + 1)

        for qnum in qnums:
            column_name = f"{self.task.FN_QPREFIX}{qnum}"

            column_dict[column_name] = self.task.wxstring(req, column_name)

        return self.get_percentage_summaries(
            req,
            column_dict=column_dict,
            num_answers=3,
            cell_format=cell_format
        )

    def _get_ff_column_headings(self, req: "CamcopsRequest") -> List[str]:
        _ = req.gettext
        return [_("Question"),
                _("Total responses")] + self.task.get_ff_options(req)

    def _get_ff_rows(self, req: "CamcopsRequest",
                     cell_format: str = "{}") -> List[List[str]]:
        """
        Percentage of people who answered x for the friends/family question
        """
        return self.get_percentage_summaries(
            req,
            column_dict={
                "ff_rating": self.task.wxstring(
                    req,
                    f"{self.task.FN_QPREFIX}_ff_rating"
                )
            },
            num_answers=6,
            cell_format=cell_format
        )

    def _get_ff_why_rows(self, req: "CamcopsRequest") -> List[List[str]]:
        """
        Reasons for giving a particular answer to the friends/family question
        """

        options = self.task.get_ff_options(req)

        wheres = [
            column("ff_rating").isnot(None),
            column("ff_why").isnot(None)
        ]

        self.add_task_report_filters(wheres)

        # noinspection PyUnresolvedReferences
        query = (
            select([
                column("ff_rating"),
                column("ff_why")
            ])
            .select_from(self.task.__table__)
            .where(and_(*wheres))
            .order_by("ff_why")
        )

        rows = []

        for result in req.dbsession.execute(query).fetchall():
            rows.append([options[result[0]], result[1]])

        return rows

    def _get_comment_rows(self, req: "CamcopsRequest") -> List[Tuple[str]]:
        """
        A list of all the additional comments, as rows.
        """

        wheres = [
            column("comments").isnot(None)
        ]

        self.add_task_report_filters(wheres)

        # noinspection PyUnresolvedReferences
        query = (
            select([
                column("comments"),
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

class APEQCPFTPerinatalReportTestCase(DemoDatabaseTestCase):
    COL_Q = 0
    COL_TOTAL = 1
    COL_RESPONSE_START = 2

    COL_FF_WHY = 1

    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)
        self.id_sequence = self.get_id()

    def setUp(self) -> None:
        super().setUp()

        self.report = APEQCPFTPerinatalReport()

        # Really only needed for tests
        self.report.start_datetime = None
        self.report.end_datetime = None

    @staticmethod
    def get_id() -> Generator[int, None, None]:
        i = 1

        while True:
            yield i
            i += 1

    def create_task(self,
                    q1: Optional[int],
                    q2: Optional[int],
                    q3: Optional[int],
                    q4: Optional[int],
                    q5: Optional[int],
                    q6: Optional[int],
                    ff_rating: int,
                    ff_why: str = None,
                    comments: str = None,
                    era: str = None) -> None:
        task = APEQCPFTPerinatal()
        self.apply_standard_task_fields(task)
        task.id = next(self.id_sequence)
        task.q1 = q1
        task.q2 = q2
        task.q3 = q3
        task.q4 = q4
        task.q5 = q5
        task.q6 = q6
        task.ff_rating = ff_rating
        task.ff_why = ff_why
        task.comments = comments

        if era is not None:
            task.when_created = pendulum.parse(era)

        self.dbsession.add(task)


class APEQCPFTPerinatalReportTests(APEQCPFTPerinatalReportTestCase):
    def create_tasks(self) -> None:
        """
        Creates 20 tasks.
        Should give us:

        .. code-block:: none

            q1: 0 - 50%,
                1 - 25%
                2 - 25%
            q2: 1 - 100%
            q3: 0 - 5%
                1 - 20%
                2 - 75%
            q4: 0 - 10%
                1 - 40%
                2 - 50%
            q5: 0 - 15%
                1 - 55%
                2 - 30%
            q6: 1 - 50%
                2 - 50%
            ff: 0 - 25%
                1 - 10%
                2 - 15%
                3 - 10%
                4 - 5%
                5 - 35%

        """
        #                q1 q2 q3 q4 q5 q6 ff
        self.create_task(0, 1, 0, 0, 2, 2, 5, ff_why="ff_5_1")
        self.create_task(0, 1, 1, 0, 2, 2, 5, ff_why="ff_5_2",
                         comments="comments_2")
        self.create_task(0, 1, 1, 1, 2, 2, 5)
        self.create_task(0, 1, 1, 1, 2, 2, 5)
        self.create_task(0, 1, 1, 1, 2, 2, 5, comments="comments_5")

        self.create_task(0, 1, 2, 1, 2, 2, 5)
        self.create_task(0, 1, 2, 1, 1, 2, 5)
        self.create_task(0, 1, 2, 1, 1, 2, 4, ff_why="ff_4_1")
        self.create_task(0, 1, 2, 1, 1, 2, 3)
        self.create_task(0, 1, 2, 1, 1, 1, 3, ff_why="ff_3_1")

        self.create_task(1, 1, 2, 2, 1, 1, 2, ff_why="ff_2_1")
        self.create_task(1, 1, 2, 2, 1, 1, 2)
        self.create_task(1, 1, 2, 2, 1, 1, 2, ff_why="ff_2_2")
        self.create_task(1, 1, 2, 2, 1, 1, 1, ff_why="ff_1_1")
        self.create_task(1, 1, 2, 2, 1, 1, 1, ff_why="ff_1_2")

        self.create_task(2, 1, 2, 2, 1, 1, 0)
        self.create_task(2, 1, 2, 2, 1, 1, 0)
        self.create_task(2, 1, 2, 2, 0, None, 0)
        self.create_task(2, 1, 2, 2, 0, None, 0)
        self.create_task(2, 1, 2, 2, 0, 1, 0, comments="comments_20")

        self.dbsession.commit()

    def test_main_rows_contain_percentages(self) -> None:
        expected_q1 = [20, "50", "25", "25"]
        expected_q2 = [20, "", "100", ""]
        expected_q3 = [20, "5", "20", "75"]
        expected_q4 = [20, "10", "40", "50"]
        expected_q5 = [20, "15", "55", "30"]
        expected_q6 = [18, "", "50", "50"]

        main_rows = self.report._get_main_rows(self.req)

        self.assertEqual(main_rows[0][1:], expected_q1)
        self.assertEqual(main_rows[1][1:], expected_q2)
        self.assertEqual(main_rows[2][1:], expected_q3)
        self.assertEqual(main_rows[3][1:], expected_q4)
        self.assertEqual(main_rows[4][1:], expected_q5)
        self.assertEqual(main_rows[5][1:], expected_q6)

    def test_main_rows_formatted(self) -> None:
        expected_q1 = [20, "50.0%", "25.0%", "25.0%"]

        main_rows = self.report._get_main_rows(
            self.req, cell_format="{0:.1f}%"
        )

        self.assertEqual(main_rows[0][1:], expected_q1)

    def test_ff_rows_contain_percentages(self) -> None:
        expected_ff = [20, "25", "10", "15",
                       "10", "5", "35"]

        ff_rows = self.report._get_ff_rows(self.req)

        self.assertEqual(ff_rows[0][1:], expected_ff)

    def test_ff_rows_formatted(self) -> None:
        expected_ff = [20, "25.0%", "10.0%", "15.0%",
                       "10.0%", "5.0%", "35.0%"]

        ff_rows = self.report._get_ff_rows(self.req, cell_format="{0:.1f}%")

        self.assertEqual(ff_rows[0][1:], expected_ff)

    def test_ff_why_rows_contain_reasons(self) -> None:
        expected_reasons = [
            ["Extremely unlikely", "ff_1_1"],
            ["Extremely unlikely", "ff_1_2"],
            ["Unlikely", "ff_2_1"],
            ["Unlikely", "ff_2_2"],
            ["Neither likely nor unlikely", "ff_3_1"],
            ["Likely", "ff_4_1"],
            ["Extremely likely", "ff_5_1"],
            ["Extremely likely", "ff_5_2"],
        ]

        ff_why_rows = self.report._get_ff_why_rows(self.req)

        self.assertEqual(ff_why_rows, expected_reasons)

    def test_comments(self) -> None:
        expected_comments = [
            "comments_2", "comments_5", "comments_20",
        ]
        comments = self.report._get_comments(self.req)
        self.assertEqual(comments, expected_comments)


class APEQCPFTPerinatalReportDateRangeTests(APEQCPFTPerinatalReportTestCase):
    def create_tasks(self) -> None:
        self.create_task(1, 0, 0, 0, 0, 0, 0,
                         ff_why="ff why 1",
                         comments="comments 1",
                         era="2018-10-01T00:00:00.000000+00:00")
        self.create_task(0, 0, 0, 0, 0, 0, 2,
                         ff_why="ff why 2",
                         comments="comments 2",
                         era="2018-10-02T00:00:00.000000+00:00")
        self.create_task(0, 0, 0, 0, 0, 0, 2,
                         ff_why="ff why 3",
                         comments="comments 3",
                         era="2018-10-03T00:00:00.000000+00:00")
        self.create_task(0, 0, 0, 0, 0, 0, 2,
                         ff_why="ff why 4",
                         comments="comments 4",
                         era="2018-10-04T00:00:00.000000+00:00")
        self.create_task(1, 0, 0, 0, 0, 0, 0,
                         ff_why="ff why 5",
                         comments="comments 5",
                         era="2018-10-05T00:00:00.000000+00:00")
        self.dbsession.commit()

    def test_main_rows_filtered_by_date(self) -> None:
        self.report.start_datetime = "2018-10-02T00:00:00.000000+00:00"
        self.report.end_datetime = "2018-10-05T00:00:00.000000+00:00"

        rows = self.report._get_main_rows(self.req, cell_format="{0:.1f}%")
        q1_row = rows[0]

        # There should be three tasks included in the calculation.
        self.assertEqual(q1_row[self.COL_TOTAL], 3)

        # For question 1 all of them answered 0 so we would expect
        # 100%. If the results aren't being filtered we will get
        # 60%
        self.assertEqual(q1_row[self.COL_RESPONSE_START + 0], "100.0%")

    def test_ff_rows_filtered_by_date(self) -> None:
        self.report.start_datetime = "2018-10-02T00:00:00.000000+00:00"
        self.report.end_datetime = "2018-10-05T00:00:00.000000+00:00"

        rows = self.report._get_ff_rows(self.req, cell_format="{0:.1f}%")
        ff_row = rows[0]

        # There should be three tasks included in the calculation.
        self.assertEqual(ff_row[self.COL_TOTAL], 3)

        # For the ff question all of them answered 2 so we would expect
        # 100%. If the results aren't being filtered we will get
        # 60%
        self.assertEqual(ff_row[self.COL_RESPONSE_START + 2], "100.0%")

    def test_ff_why_row_filtered_by_date(self) -> None:
        self.report.start_datetime = "2018-10-02T00:00:00.000000+00:00"
        self.report.end_datetime = "2018-10-05T00:00:00.000000+00:00"

        rows = self.report._get_ff_why_rows(self.req)
        self.assertEqual(len(rows), 3)

        self.assertEqual(rows[0][self.COL_FF_WHY], "ff why 2")
        self.assertEqual(rows[1][self.COL_FF_WHY], "ff why 3")
        self.assertEqual(rows[2][self.COL_FF_WHY], "ff why 4")

    def test_comments_filtered_by_date(self) -> None:
        self.report.start_datetime = "2018-10-02T00:00:00.000000+00:00"
        self.report.end_datetime = "2018-10-05T00:00:00.000000+00:00"

        comments = self.report._get_comments(self.req)
        self.assertEqual(len(comments), 3)

        self.assertEqual(comments[0], "comments 2")
        self.assertEqual(comments[1], "comments 3")
        self.assertEqual(comments[2], "comments 4")
