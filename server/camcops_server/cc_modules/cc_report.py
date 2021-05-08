#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_report.py

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

**CamCOPS reports.**

"""

import logging
from abc import ABC
from typing import (Any, Callable, Dict, List, Optional, Sequence,
                    Type, TYPE_CHECKING, Union)

from cardinal_pythonlib.classes import all_subclasses, classproperty
from cardinal_pythonlib.datetimefunc import format_datetime
from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.pyramid.responses import (
    OdsResponse, TsvResponse, XlsxResponse,
)
from deform.form import Form
from pyramid.httpexceptions import HTTPBadRequest
from pyramid.renderers import render_to_response
from pyramid.response import Response
from sqlalchemy.engine.result import ResultProxy
from sqlalchemy.orm.query import Query
from sqlalchemy.sql.elements import ColumnElement
from sqlalchemy.sql.expression import and_, column, func, select
from sqlalchemy.sql.selectable import SelectBase

# import as LITTLE AS POSSIBLE; this is used by lots of modules
from camcops_server.cc_modules.cc_constants import (
    DateFormat,
    DEFAULT_ROWS_PER_PAGE,
)
from camcops_server.cc_modules.cc_db import FN_CURRENT, TFN_WHEN_CREATED
from camcops_server.cc_modules.cc_pyramid import (
    CamcopsPage,
    PageUrl,
    ViewArg,
    ViewParam,
)
from camcops_server.cc_modules.cc_tsv import TsvCollection, TsvPage

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_forms import (  # noqa: F401
        ReportParamForm,
        ReportParamSchema,
    )
    from camcops_server.cc_modules.cc_request import CamcopsRequest  # noqa: E501,F401
    from camcops_server.cc_modules.cc_task import Task  # noqa: F401

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Other constants
# =============================================================================

class PlainReportType(object):
    """
    Simple class to hold the results of a plain report.
    """
    def __init__(self, rows: Sequence[Sequence[Any]],
                 column_names: Sequence[str]) -> None:
        self.rows = rows
        self.column_names = column_names


# =============================================================================
# Report class
# =============================================================================

class Report(object):
    """
    Abstract base class representing a report.

    If you are writing a report, you must override these attributes:

    - :meth:`report_id`
    - :meth:`report_title`
    - One combination of:

      - (simplest) :meth:`get_query` OR :meth:`get_rows_colnames`
      - (for multi-page results) :meth:`render_html` and :meth:`get_tsv_pages`
      - (manual control) all ``render_*`` functions

    See the explanations of each.
    """

    template_name = "report.mako"

    # -------------------------------------------------------------------------
    # Attributes that must be provided
    # -------------------------------------------------------------------------
    # noinspection PyMethodParameters
    @classproperty
    def report_id(cls) -> str:
        """
        Returns a identifying string, unique to this report, used in the HTML
        report selector.
        """
        raise NotImplementedError("implement in subclass")

    @classmethod
    def title(cls, req: "CamcopsRequest") -> str:
        """
        Descriptive title for display purposes.
        """
        raise NotImplementedError("implement in subclass")

    # noinspection PyMethodParameters
    @classproperty
    def superuser_only(cls) -> bool:
        """
        If ``True`` (the default), only superusers may run the report.
        You must explicitly override this property to permit others.
        """
        return True

    @classmethod
    def get_http_query_keys(cls) -> List[str]:
        """
        Returns the keys used for the HTTP GET query. They include details of:

        - which report?
        - how to view it?
        - pagination options
        - report-specific configuration details from
          :func:`get_specific_http_query_keys`.
        """
        return [
            ViewParam.REPORT_ID,
            ViewParam.VIEWTYPE,
            ViewParam.ROWS_PER_PAGE,
            ViewParam.PAGE,
        ] + cls.get_specific_http_query_keys()

    @classmethod
    def get_specific_http_query_keys(cls) -> List[str]:
        """
        Additional HTTP GET query keys used by this report. Override to add
        custom ones.
        """
        return []

    def get_query(self, req: "CamcopsRequest") \
            -> Union[None, SelectBase, Query]:
        """
        Overriding this function is one way of providing a report. (The other
        is :func:`get_rows_colnames`.)

        To override this function, return the SQLAlchemy Base :class:`Select`
        statement or the SQLAlchemy ORM :class:`Query` to execute the report.

        Parameters are passed in via the request.
        """
        return None

    def get_rows_colnames(self, req: "CamcopsRequest") \
            -> Optional[PlainReportType]:
        """
        Overriding this function is one way of providing a report. (The other
        is :func:`get_query`.)

        To override this function, return a :class:`PlainReportType` with
        column names and row content.
        """
        return None

    @staticmethod
    def get_paramform_schema_class() -> Type["ReportParamSchema"]:
        """
        Returns the class used as the Colander schema for the form that
        configures the report. By default, this is a simple form that just
        offers a choice of output format, but you can provide a more
        extensive one (an example being in
        :class:`camcops_server.tasks.diagnosis.DiagnosisFinderReportBase`.
        """
        from camcops_server.cc_modules.cc_forms import ReportParamSchema  # delayed import  # noqa
        return ReportParamSchema

    def get_form(self, req: "CamcopsRequest") -> Form:
        """
        Returns a Colander form to configure the report. The default usually
        suffices, and it will use the schema specified in
        :func:`get_paramform_schema_class`.
        """
        from camcops_server.cc_modules.cc_forms import ReportParamForm  # delayed import  # noqa
        schema_class = self.get_paramform_schema_class()
        return ReportParamForm(request=req, schema_class=schema_class)

    @staticmethod
    def get_test_querydict() -> Dict[str, Any]:
        """
        What this function returns is used as the specimen Colander
        ``appstruct`` for unit tests. The default is an empty dictionary.
        """
        return {}

    @staticmethod
    def add_task_report_filters(wheres: List[ColumnElement]) -> None:
        """
        Adds any restrictions required to a list of SQLAlchemy Core ``WHERE``
        clauses.

        Override this (or provide additional filters and call this) to provide
        global filters to queries used to create reports.

        Used by :class:`DateTimeFilteredReportMixin`, etc.

        The presumption is that the thing being filtered is an instance of
        :class:`camcops_server.cc_modules.cc_task.Task`.

        Args:
            wheres:
                list of SQL ``WHERE`` conditions, each represented as an
                SQLAlchemy :class:`ColumnElement`. This list is modifed in
                place. The caller will need to apply the final list to the
                query.
        """
        # noinspection PyPep8
        wheres.append(
            column(FN_CURRENT) == True  # noqa: E712
        )

    # -------------------------------------------------------------------------
    # Common functionality: classmethods
    # -------------------------------------------------------------------------

    @classmethod
    def all_subclasses(cls) -> List[Type["Report"]]:
        """
        Get all report subclasses, except those not implementing their
        ``report_id`` property. Optionally, sort by their title.
        """
        # noinspection PyTypeChecker
        classes = all_subclasses(cls)  # type: List[Type["Report"]]
        instantiated_report_classes = []  # type: List[Type["Report"]]
        for reportcls in classes:
            if reportcls.__name__ == 'TestReport':
                continue

            try:
                _ = reportcls.report_id
                instantiated_report_classes.append(reportcls)
            except NotImplementedError:
                # This is a subclass of Report, but it's still an abstract
                # class; skip it.
                pass
        return instantiated_report_classes

    # -------------------------------------------------------------------------
    # Common functionality: default Response
    # -------------------------------------------------------------------------

    def get_response(self, req: "CamcopsRequest") -> Response:
        """
        Return the report content itself, as an HTTP :class:`Response`.
        """
        # Check the basic parameters
        report_id = req.get_str_param(ViewParam.REPORT_ID)

        if report_id != self.report_id:
            raise HTTPBadRequest("Error - request directed to wrong report!")

        # viewtype = req.get_str_param(ViewParam.VIEWTYPE, ViewArg.HTML,
        #                              lower=True)
        # ... NO; for a Deform radio button, the request contains parameters
        # like
        #   ('__start__', 'viewtype:rename'),
        #   ('deformField2', 'tsv'),
        #   ('__end__', 'viewtype:rename')
        # ... so we need to ask the appstruct instead.
        # This is a bit different from how we manage trackers/CTVs, where we
        # recode the appstruct to a URL.
        #
        # viewtype = appstruct.get(ViewParam.VIEWTYPE)  # type: str
        #
        # Ah, no... that fails with pagination of reports. Let's redirect
        # things to the HTTP query, as for trackers/audit!

        viewtype = req.get_str_param(ViewParam.VIEWTYPE, ViewArg.HTML,
                                     lower=True)
        # Run the report (which may take additional parameters from the
        # request)
        # Serve the result
        if viewtype == ViewArg.HTML:
            return self.render_html(req=req)

        if viewtype == ViewArg.ODS:
            return self.render_ods(req=req)

        if viewtype == ViewArg.TSV:
            return self.render_tsv(req=req)

        if viewtype == ViewArg.XLSX:
            return self.render_xlsx(req=req)

        raise HTTPBadRequest("Bad viewtype")

    def render_html(self, req: "CamcopsRequest") -> Response:
        rows_per_page = req.get_int_param(ViewParam.ROWS_PER_PAGE,
                                          DEFAULT_ROWS_PER_PAGE)
        page_num = req.get_int_param(ViewParam.PAGE, 1)

        plain_report = self._get_plain_report(req)

        page = CamcopsPage(collection=plain_report.rows,
                           page=page_num,
                           items_per_page=rows_per_page,
                           url_maker=PageUrl(req),
                           request=req)

        return self.render_single_page_html(
            req=req,
            column_names=plain_report.column_names,
            page=page
        )

    def render_tsv(self, req: "CamcopsRequest") -> TsvResponse:
        filename = self.get_filename(req, ViewArg.TSV)

        # By default there is only one page. If there are more,
        # we only output the first
        page = self.get_tsv_pages(req)[0]

        return TsvResponse(body=page.get_tsv(), filename=filename)

    def render_xlsx(self, req: "CamcopsRequest") -> XlsxResponse:
        filename = self.get_filename(req, ViewArg.XLSX)
        tsvcoll = self.get_tsv_collection(req)
        content = tsvcoll.as_xlsx()

        return XlsxResponse(body=content, filename=filename)

    def render_ods(self, req: "CamcopsRequest") -> OdsResponse:
        filename = self.get_filename(req, ViewArg.ODS)
        tsvcoll = self.get_tsv_collection(req)
        content = tsvcoll.as_ods()

        return OdsResponse(body=content, filename=filename)

    def get_tsv_collection(self, req: "CamcopsRequest") -> TsvCollection:
        tsvcoll = TsvCollection()
        tsvcoll.add_pages(self.get_tsv_pages(req))

        return tsvcoll

    def get_tsv_pages(self, req: "CamcopsRequest") -> List[TsvPage]:
        plain_report = self._get_plain_report(req)

        page = self.get_tsv_page(name=self.title(req),
                                 column_names=plain_report.column_names,
                                 rows=plain_report.rows)
        return [page]

    @staticmethod
    def get_tsv_page(name: str,
                     column_names: Sequence[str],
                     rows: Sequence[Sequence[Any]]) -> TsvPage:
        keyed_rows = [dict(zip(column_names, r)) for r in rows]
        page = TsvPage(name=name, rows=keyed_rows)

        return page

    def get_filename(self, req: "CamcopsRequest", viewtype: str) -> str:
        extension_dict = {
            ViewArg.ODS: 'ods',
            ViewArg.TSV: 'tsv',
            ViewArg.XLSX: 'xlsx',
        }

        if viewtype not in extension_dict:
            raise HTTPBadRequest("Unsupported viewtype")

        extension = extension_dict.get(viewtype)

        return (
            "CamCOPS_" +
            self.report_id +
            "_" +
            format_datetime(req.now, DateFormat.FILENAME) +
            "." +
            extension
        )

    def render_single_page_html(self,
                                req: "CamcopsRequest",
                                column_names: Sequence[str],
                                page: CamcopsPage) -> Response:
        """
        Converts a paginated report into an HTML response.

        If you wish, you can override this for more report customization.
        """
        return render_to_response(
            self.template_name,
            dict(title=self.title(req),
                 page=page,
                 column_names=column_names,
                 report_id=self.report_id),
            request=req
        )

    def _get_plain_report(self, req: "CamcopsRequest") -> PlainReportType:
        """
        Uses :meth:`get_query`, or if absent, :meth:`get_rows_colnames`, to
        fetch data. Returns a "single-page" type report, in the form of a
        :class:`PlainReportType`.
        """
        statement = self.get_query(req)
        if statement is not None:
            rp = req.dbsession.execute(statement)  # type: ResultProxy
            column_names = rp.keys()
            rows = rp.fetchall()

            plain_report = PlainReportType(rows=rows,
                                           column_names=column_names)
        else:
            plain_report = self.get_rows_colnames(req)
            if plain_report is None:
                raise NotImplementedError(
                    "Report did not implement either of get_query()"
                    " or get_rows_colnames()")

        return plain_report


class PercentageSummaryReportMixin(object):
    """
    Mixin to be used with :class:`Report`.
    """
    @classproperty
    def task_class(self) -> Type["Task"]:
        raise NotImplementedError("implement in subclass")

    def get_percentage_summaries(self,
                                 req: "CamcopsRequest",
                                 column_dict: Dict[str, str],
                                 num_answers: int,
                                 cell_format: str = "{}",
                                 min_answer: int = 0) -> List[List[str]]:
        """
        Provides a summary of each question, x% of people said each response.
        """
        rows = []

        for column_name, question in column_dict.items():
            """
            e.g. SELECT COUNT(col) FROM perinatal_poem WHERE col IS NOT NULL
            """
            wheres = [
                column(column_name).isnot(None)
            ]

            # noinspection PyUnresolvedReferences
            self.add_task_report_filters(wheres)

            # noinspection PyUnresolvedReferences
            total_query = (
                select([func.count(column_name)])
                .select_from(self.task_class.__table__)
                .where(and_(*wheres))
            )

            total_responses = req.dbsession.execute(total_query).fetchone()[0]

            row = [question] + [total_responses] + [""] * num_answers

            """
            e.g.
            SELECT total_responses,col, ((100 * COUNT(col)) / total_responses)
            FROM perinatal_poem WHERE col is not NULL
            GROUP BY col
            """
            # noinspection PyUnresolvedReferences
            query = (
                select([
                    column(column_name),
                    ((100 * func.count(column_name))/total_responses)
                ])
                .select_from(self.task_class.__table__)
                .where(and_(*wheres))
                .group_by(column_name)
            )

            # row output is:
            #      0              1               2              3
            # +----------+-----------------+--------------+--------------+----
            # | question | total responses | % 1st answer | % 2nd answer | ...
            # +----------+-----------------+--------------+--------------+----
            for result in req.dbsession.execute(query):
                col = 2 + (result[0] - min_answer)
                row[col] = cell_format.format(result[1])

            rows.append(row)

        return rows


class DateTimeFilteredReportMixin(object):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.start_datetime = None  # type: Optional[str]
        self.end_datetime = None  # type: Optional[str]

    @staticmethod
    def get_paramform_schema_class() -> Type["ReportParamSchema"]:
        from camcops_server.cc_modules.cc_forms import DateTimeFilteredReportParamSchema  # delayed import  # noqa
        return DateTimeFilteredReportParamSchema

    @classmethod
    def get_specific_http_query_keys(cls) -> List[str]:
        # noinspection PyUnresolvedReferences
        return super().get_specific_http_query_keys() + [
            ViewParam.START_DATETIME,
            ViewParam.END_DATETIME,
        ]

    def get_response(self, req: "CamcopsRequest") -> Response:
        self.start_datetime = format_datetime(
            req.get_datetime_param(ViewParam.START_DATETIME),
            DateFormat.ERA
        )
        self.end_datetime = format_datetime(
            req.get_datetime_param(ViewParam.END_DATETIME),
            DateFormat.ERA
        )

        # noinspection PyUnresolvedReferences
        return super().get_response(req)

    def add_task_report_filters(self, wheres: List[ColumnElement]) -> None:
        """
        Adds any restrictions required to a list of SQLAlchemy Core ``WHERE``
        clauses.

        See :meth:`Report.add_task_report_filters`.

        Args:
            wheres:
                list of SQL ``WHERE`` conditions, each represented as an
                SQLAlchemy :class:`ColumnElement`. This list is modifed in
                place. The caller will need to apply the final list to the
                query.
        """
        # noinspection PyUnresolvedReferences
        super().add_task_report_filters(wheres)

        if self.start_datetime is not None:
            wheres.append(
                column(TFN_WHEN_CREATED) >= self.start_datetime
            )

        if self.end_datetime is not None:
            wheres.append(
                column(TFN_WHEN_CREATED) < self.end_datetime
            )


class ScoreDetails(object):
    """
    Represents a type of score whose progress we want to track over time.
    """
    def __init__(self,
                 name: str,
                 scorefunc: Callable[["Task"], Union[None, int, float]],
                 minimum: int,
                 maximum: int,
                 higher_score_is_better: bool = False) -> None:
        """
        Args:
            name:
                human-friendly name of this score
            scorefunc:
                function that can be called with a task instance as its
                sole parameter and which will return a numerical score (or
                ``None``)
            minimum:
                minimum possible value of this score (for display purposes)
            maximum:
                maximum possible value of this score (for display purposes)
            higher_score_is_better:
                is a higher score a better thing?
        """
        self.name = name
        self.scorefunc = scorefunc
        self.minimum = minimum
        self.maximum = maximum
        self.higher_score_is_better = higher_score_is_better

    def calculate_improvement(self,
                              first_score: float,
                              latest_score: float) -> float:
        """
        Improvement is positive.

        So if higher scores are better, returns ``latest - first``; otherwise
        returns ``first - latest``.
        """
        if self.higher_score_is_better:
            return latest_score - first_score
        else:
            return first_score - latest_score


class AverageScoreReport(DateTimeFilteredReportMixin, Report, ABC):
    """
    Used by MAAS, CORE-10 and PBQ to report average scores and progress
    """
    template_name = "average_score_report.mako"

    def __init__(self, *args, via_index: bool = True, **kwargs) -> None:
        """
        Args:
            via_index:
                set this to ``False`` for unit test when you don't want to
                have to build a dummy task index.
        """
        super().__init__(*args, **kwargs)
        self.via_index = via_index

    # noinspection PyMethodParameters
    @classproperty
    def superuser_only(cls) -> bool:
        return False

    # noinspection PyMethodParameters
    @classproperty
    def task_class(cls) -> Type["Task"]:
        raise NotImplementedError(
            "Report did not implement task_class"
        )

    # noinspection PyMethodParameters
    @classmethod
    def scoretypes(cls, req: "CamcopsRequest") -> List[ScoreDetails]:
        raise NotImplementedError(
            "Report did not implement 'scoretypes'"
        )

    @staticmethod
    def no_data_value() -> Any:
        """
        The value used for a "no data" cell.

        The only reason this is accessible outside this class is for unit
        testing.
        """
        return ""

    def render_html(self, req: "CamcopsRequest") -> Response:
        tsv_pages = self.get_tsv_pages(req)
        return render_to_response(
            self.template_name,
            dict(title=self.title(req),
                 mainpage=tsv_pages[0],
                 datepage=tsv_pages[1],
                 report_id=self.report_id),
            request=req
        )

    def get_tsv_pages(self, req: "CamcopsRequest") -> List[TsvPage]:
        """
        We use an SQLAlchemy ORM, rather than Core, method. Why?

        - "Patient equality" is complex (e.g. same patient_id on same device,
          or a shared ID number, etc.) -- simplicity via Patient.__eq__.
        - Facilities "is task complete?" checks, and use of Python
          calculations.
        """
        _ = req.gettext
        from camcops_server.cc_modules.cc_taskcollection import (
            TaskCollection,
            task_when_created_sorter,
        )  # delayed import
        from camcops_server.cc_modules.cc_taskfilter import TaskFilter  # delayed import  # noqa

        # Which tasks?
        taskfilter = TaskFilter()
        taskfilter.task_types = [self.task_class.__tablename__]
        taskfilter.start_datetime = self.start_datetime
        taskfilter.end_datetime = self.end_datetime
        taskfilter.complete_only = True

        # Get tasks
        collection = TaskCollection(
            req=req,
            taskfilter=taskfilter,
            current_only=True,
            via_index=self.via_index,
        )
        all_tasks = collection.all_tasks

        # Get all distinct patients
        patients = set(t.patient for t in all_tasks)
        # log.critical("all_tasks: {}", all_tasks)
        # log.critical("patients: {}", [str(p) for p in patients])

        scoretypes = self.scoretypes(req)
        n_scoretypes = len(scoretypes)

        # Sum first/last/progress scores by patient
        sum_first_by_score = [0] * n_scoretypes
        sum_last_by_score = [0] * n_scoretypes
        sum_improvement_by_score = [0] * n_scoretypes
        n_first = 0
        n_last = 0  # also n_progress
        for patient in patients:
            # Find tasks for this patient
            patient_tasks = [t for t in all_tasks if t.patient == patient]
            assert patient_tasks, f"No tasks for patient {patient}"
            # log.critical("For patient {}, tasks: {}", patient, patient_tasks)
            # Find first and last task (last may be absent)
            patient_tasks.sort(key=task_when_created_sorter)
            first = patient_tasks[0]
            n_first += 1
            if len(patient_tasks) > 1:
                last = patient_tasks[-1]
                n_last += 1
            else:
                last = None

            # Obtain first/last scores and progress
            for scoreidx, scoretype in enumerate(scoretypes):
                firstscore = scoretype.scorefunc(first)
                # Scores should not be None, because all tasks are complete.
                sum_first_by_score[scoreidx] += firstscore
                if last:
                    lastscore = scoretype.scorefunc(last)
                    sum_last_by_score[scoreidx] += lastscore
                    improvement = scoretype.calculate_improvement(
                        firstscore, lastscore)
                    sum_improvement_by_score[scoreidx] += improvement

        # Format output
        column_names = [
            _("Number of initial records"),
            _("Number of latest subsequent records"),
        ]
        row = [n_first, n_last]
        no_data = self.no_data_value()
        for scoreidx, scoretype in enumerate(scoretypes):
            # Calculations
            if n_first == 0:
                avg_first = no_data
            else:
                avg_first = sum_first_by_score[scoreidx] / n_first
            if n_last == 0:
                avg_last = no_data
                avg_improvement = no_data
            else:
                avg_last = sum_last_by_score[scoreidx] / n_last
                avg_improvement = sum_improvement_by_score[scoreidx] / n_last

            # Columns and row data
            column_names += [
                f"{scoretype.name} ({scoretype.minimum}–{scoretype.maximum}): "
                f"{_('First')}",
                f"{scoretype.name} ({scoretype.minimum}–{scoretype.maximum}): "
                f"{_('Latest')}",
                f"{scoretype.name}: {_('Improvement')}",
            ]
            row += [avg_first, avg_last, avg_improvement]

        # Create and return report
        mainpage = self.get_tsv_page(
            name=self.title(req),
            column_names=column_names,
            rows=[row]
        )
        datepage = self.get_tsv_page(
            name=_("Date filters"),
            column_names=[_("Start date"), _("End date")],
            rows=[[str(self.start_datetime), str(self.end_datetime)]]
        )
        return [mainpage, datepage]


# =============================================================================
# Report framework
# =============================================================================

def get_all_report_classes(req: "CamcopsRequest") -> List[Type["Report"]]:
    """
    Returns all :class:`Report` (sub)classes, i.e. all report types.
    """
    classes = Report.all_subclasses()
    classes.sort(key=lambda c: c.title(req))
    return classes


def get_report_instance(report_id: str) -> Optional[Report]:
    """
    Creates an instance of a :class:`Report`, given its ID (name), or return
    ``None`` if the ID is invalid.
    """
    if not report_id:
        return None
    for cls in Report.all_subclasses():
        if cls.report_id == report_id:
            return cls()
    return None
