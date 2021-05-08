#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_tracker.py

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

**Trackers, showing numerical information over time, and clinical text views,
showing text that a clinician might care about.**

"""

import logging
from typing import Any, Dict, List, Optional, Set, Tuple, TYPE_CHECKING

from cardinal_pythonlib.datetimefunc import format_datetime
from cardinal_pythonlib.logs import BraceStyleAdapter
from pendulum import DateTime as Pendulum
from pyramid.renderers import render

from camcops_server.cc_modules.cc_audit import audit
from camcops_server.cc_modules.cc_constants import (
    CssClass,
    CSS_PAGED_MEDIA,
    DateFormat,
    MatplotlibConstants,
    PlotDefaults,
)
from camcops_server.cc_modules.cc_filename import get_export_filename
from camcops_server.cc_modules.cc_plot import matplotlib
from camcops_server.cc_modules.cc_pdf import pdf_from_html
from camcops_server.cc_modules.cc_pyramid import ViewArg, ViewParam
from camcops_server.cc_modules.cc_simpleobjects import TaskExportOptions
from camcops_server.cc_modules.cc_task import Task
from camcops_server.cc_modules.cc_taskcollection import (
    TaskCollection,
    TaskFilter,
    TaskSortMethod,
)
from camcops_server.cc_modules.cc_xml import (
    get_xml_document,
    XmlDataTypes,
    XmlElement,
)

import matplotlib.dates  # delayed until after the cc_plot import

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_patient import Patient  # noqa: F401
    from camcops_server.cc_modules.cc_patientidnum import PatientIdNum  # noqa: E501,F401
    from camcops_server.cc_modules.cc_request import CamcopsRequest  # noqa: E501,F401
    from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo  # noqa: E501,F401

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Constants
# =============================================================================

TRACKER_DATEFORMAT = "%Y-%m-%d"
WARNING_NO_PATIENT_FOUND = f"""
    <div class="{CssClass.WARNING}">
    </div>
"""
WARNING_DENIED_INFORMATION = f"""
    <div class="{CssClass.WARNING}">
        Other tasks exist for this patient that you do not have access to view.
    </div>
"""

DEBUG_TRACKER_TASK_INCLUSION = False  # should be False for production system


# =============================================================================
# Helper functions
# =============================================================================
# http://stackoverflow.com/questions/11788195

def consistency(req: "CamcopsRequest",
                values: List[Any],
                servervalue: Any = None,
                case_sensitive: bool = True) -> Tuple[bool, str]:
    """
    Checks for consistency in a set of values (e.g. names, dates of birth).
    (ID numbers are done separately via :func:`consistency_idnums`.)

    The list of values (with the ``servervalue`` appended, if not ``None``) is
    checked to ensure that it contains only one unique value (ignoring ``None``
    values or empty ``""`` values).

    Returns:
        the tuple ``consistent, msg``, where ``consistent`` is a bool and
        ``msg`` is a descriptive HTML message
    """
    if case_sensitive:
        vallist = [str(v) if v is not None else v for v in values]
        if servervalue is not None:
            vallist.append(str(servervalue))
    else:
        vallist = [str(v).upper() if v is not None else v for v in values]
        if servervalue is not None:
            vallist.append(str(servervalue).upper())
    # Replace "" with None, so we only have a single "not-present" value
    vallist = [None if x == "" else x for x in vallist]
    unique = list(set(vallist))
    _ = req.gettext
    if len(unique) == 0:
        return True, _("consistent (no values)")
    if len(unique) == 1:
        return True, f"{_('consistent')} ({unique[0]})"
    if len(unique) == 2:
        if None in unique:
            return True, (
                f"{_('consistent')} "
                f"({_('all blank or')} {unique[1 - unique.index(None)]})"
            )
    return False, (
        f"<b>{_('INCONSISTENT')} "
        f"({_('contains values')} {', '.join(unique)})</b>"
    )


def consistency_idnums(req: "CamcopsRequest",
                       idnum_lists: List[List["PatientIdNum"]]) \
        -> Tuple[bool, str]:
    """
    Checks the consistency of a set of :class:`PatientIdNum` objects.
    "Are all these records from the same patient?"

    Args:
        req:
            a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        idnum_lists:
            a list of lists (one per task/patient instance) of
            :class:`PatientIdNum` objects

    Returns:
        the tuple ``consistent, msg``, where ``consistent`` is a bool and
        ``msg`` is a descriptive HTML message

    """
    # 1. Generate "known", mapping which_idnum -> set of observed non-NULL
    # idnum_values
    known = {}  # type: Dict[int, Set[int]]
    for task_idnum_list in idnum_lists:
        for idnum in task_idnum_list:
            idnum_value = idnum.idnum_value
            if idnum_value is not None:
                which_idnum = idnum.which_idnum
                if which_idnum not in known:
                    known[which_idnum] = set()  # type: Set[int]
                known[which_idnum].add(idnum_value)

    # 2. For every observed which_idnum, was it observed in all tasks?
    present_in_all = {}  # type: Dict[int, bool]
    for which_idnum in known.keys():
        present_for_all_tasks = all(
            # "For all tasks..."
            (
                # "At least one ID number record relates to this which_idnum".
                any(idnum.which_idnum == which_idnum
                    and idnum.idnum_value is not None)
                for idnum in task_idnum_list
            )
            for task_idnum_list in idnum_lists
        )
        present_in_all[which_idnum] = present_for_all_tasks

    # 3. Summarize
    failures = []  # type: List[str]
    successes = []  # type: List[str]
    _ = req.gettext
    for which_idnum, encountered_values in known.items():
        value_str = ", ".join(str(v) for v in sorted(list(encountered_values)))
        if len(encountered_values) > 1:
            failures.append(
                f"idnum{which_idnum} {_('contains values')} {value_str}")
        else:
            if present_in_all[which_idnum]:
                successes.append(
                    f"idnum{which_idnum} {_('consistent')} ({value_str})")
            else:
                successes.append(
                    f"idnum{which_idnum} {_('all blank or')} {value_str}")
    if failures:
        return False, (
            f"<b>{_('INCONSISTENT')} ({'; '.join(failures + successes)})</b>"
        )
    else:
        return True, f"{_('consistent')} ({'; '.join(successes)})"


def format_daterange(start: Optional[Pendulum],
                     end: Optional[Pendulum]) -> str:
    """
    Textual representation of an inclusive-to-exclusive date range.

    Arguments are datetime values.
    """
    start_str = format_datetime(start, DateFormat.ISO8601_DATE_ONLY,
                                default="−∞")
    end_str = format_datetime(end, DateFormat.ISO8601_DATE_ONLY, default="+∞")
    return f"[{start_str}, {end_str})"


# =============================================================================
# ConsistencyInfo class
# =============================================================================

class ConsistencyInfo(object):
    """
    Represents ID consistency information about a set of tasks.
    """

    def __init__(self, req: "CamcopsRequest", tasklist: List[Task]) -> None:
        """
        Initialize values, from a list of task instances.
        """
        self.request = req
        self.consistent_forename, self.msg_forename = consistency(
            req,
            [task.get_patient_forename() for task in tasklist],
            servervalue=None, case_sensitive=False)
        self.consistent_surname, self.msg_surname = consistency(
            req,
            [task.get_patient_surname() for task in tasklist],
            servervalue=None, case_sensitive=False)
        self.consistent_dob, self.msg_dob = consistency(
            req,
            [task.get_patient_dob_first11chars() for task in tasklist])
        self.consistent_sex, self.msg_sex = consistency(
            req,
            [task.get_patient_sex() for task in tasklist])
        self.consistent_idnums, self.msg_idnums = consistency_idnums(
            req,
            [task.get_patient_idnum_objects() for task in tasklist])
        self.all_consistent = (
            self.consistent_forename and
            self.consistent_surname and
            self.consistent_dob and
            self.consistent_sex and
            self.consistent_idnums
        )

    def are_all_consistent(self) -> bool:
        """
        Is all the ID information consistent?
        """
        return self.all_consistent

    def get_description_list(self) -> List[str]:
        """
        Textual representation of ID information, indicating consistency or
        lack of it.
        """
        _ = self.request.gettext
        cons = [
            f"{_('Forename:')} {self.msg_forename}",
            f"{_('Surname:')} {self.msg_surname}",
            f"{_('DOB:')} {self.msg_dob}",
            f"{_('Sex:')} {self.msg_sex}",
            f"{_('ID numbers:')} {self.msg_idnums}",
        ]
        return cons

    def get_xml_root(self) -> XmlElement:
        """
        XML tree (as root :class:`camcops_server.cc_modules.cc_xml.XmlElement`)
        of consistency information.
        """
        branches = [
            XmlElement(
                name="all_consistent",
                value=self.are_all_consistent(),
                datatype="boolean"
            )
        ]
        for c in self.get_description_list():
            branches.append(XmlElement(
                name="consistency_check",
                value=c,
            ))
        return XmlElement(name="_consistency", value=branches)


# =============================================================================
# TrackerCtvCommon class:
# =============================================================================

class TrackerCtvCommon(object):
    """
    Base class for :class:`camcops_server.cc_modules.cc_tracker.Tracker` and
    :class:`camcops_server.cc_modules.cc_tracker.ClinicalTextView`.
    """

    def __init__(self,
                 req: "CamcopsRequest",
                 taskfilter: TaskFilter,
                 as_ctv: bool,
                 via_index: bool = True) -> None:
        """
        Initialize, fetching applicable tasks.
        """

        # Record input variables at this point (for URL regeneration)
        self.req = req
        self.taskfilter = taskfilter
        self.as_ctv = as_ctv
        assert taskfilter.tasks_with_patient_only

        self.collection = TaskCollection(
            req=req,
            taskfilter=taskfilter,
            sort_method_by_class=TaskSortMethod.CREATION_DATE_ASC,
            sort_method_global=TaskSortMethod.CREATION_DATE_ASC,
            via_index=via_index
        )
        all_tasks = self.collection.all_tasks
        if all_tasks:
            self.earliest = all_tasks[0].when_created
            self.latest = all_tasks[-1].when_created
            self.patient = all_tasks[0].patient
        else:
            self.earliest = None  # type: Optional[Pendulum]
            self.latest = None  # type: Optional[Pendulum]
            self.patient = None  # type: Optional[Patient]

        # Summary information
        self.summary = ""
        if DEBUG_TRACKER_TASK_INCLUSION:
            first = True
            for cls in self.taskfilter.task_classes:
                if not first:
                    self.summary += " // "
                self.summary += cls.tablename
                first = False
                task_instances = self.collection.tasks_for_task_class(cls)
                if not task_instances:
                    if DEBUG_TRACKER_TASK_INCLUSION:
                        self.summary += " (no instances)"
                    continue
                for task in task_instances:
                    if DEBUG_TRACKER_TASK_INCLUSION:
                        self.summary += f" / PK {task.pk}"
            self.summary += " ~~~ "
        self.summary += " — ".join([
            "; ".join([
                f"({task.tablename},{task.pk},"
                f"{task.get_patient_server_pk()})"
                for task in self.collection.tasks_for_task_class(cls)
            ])
            for cls in self.taskfilter.task_classes
        ])

        # Consistency information
        self.consistency_info = ConsistencyInfo(req, all_tasks)

    # -------------------------------------------------------------------------
    # Required for implementation
    # -------------------------------------------------------------------------

    def get_xml(self,
                indent_spaces: int = 4,
                eol: str = '\n',
                include_comments: bool = False) -> str:
        """
        Returns an XML representation.

        Args:
            indent_spaces: number of spaces to indent formatted XML
            eol: end-of-line string
            include_comments: include comments describing each field?

        Returns:
            an XML UTF-8 document representing our object.
        """
        raise NotImplementedError("implement in subclass")

    def _get_html(self) -> str:
        """
        Returns an HTML representation.
        """
        raise NotImplementedError("implement in subclass")

    def _get_pdf_html(self) -> str:
        """
        Returns HTML used for making PDFs.
        """
        raise NotImplementedError("implement in subclass")

    # -------------------------------------------------------------------------
    # XML view
    # -------------------------------------------------------------------------

    def _get_xml(self,
                 audit_string: str,
                 xml_name: str,
                 indent_spaces: int = 4,
                 eol: str = '\n',
                 include_comments: bool = False) -> str:
        """
        Returns an XML document representing this object.

        Args:
            audit_string: description used to audit access to this information
            xml_name: name of the root XML element
            indent_spaces: number of spaces to indent formatted XML
            eol: end-of-line string
            include_comments: include comments describing each field?

        Returns:
            an XML UTF-8 document representing the task.
        """
        iddef = self.taskfilter.get_only_iddef()
        if not iddef:
            raise ValueError("Tracker/CTV doesn't have a single ID number "
                             "criterion")
        branches = [
            self.consistency_info.get_xml_root(),
            XmlElement(
                name="_search_criteria",
                value=[
                    XmlElement(
                        name="task_tablename_list",
                        value=",".join(self.taskfilter.task_tablename_list)
                    ),
                    XmlElement(
                        name=ViewParam.WHICH_IDNUM,
                        value=iddef.which_idnum,
                        datatype=XmlDataTypes.INTEGER
                    ),
                    XmlElement(
                        name=ViewParam.IDNUM_VALUE,
                        value=iddef.idnum_value,
                        datatype=XmlDataTypes.INTEGER
                    ),
                    XmlElement(
                        name=ViewParam.START_DATETIME,
                        value=format_datetime(self.taskfilter.start_datetime,
                                              DateFormat.ISO8601),
                        datatype=XmlDataTypes.DATETIME
                    ),
                    XmlElement(
                        name=ViewParam.END_DATETIME,
                        value=format_datetime(self.taskfilter.end_datetime,
                                              DateFormat.ISO8601),
                        datatype=XmlDataTypes.DATETIME
                    ),
                ]
            )
        ]
        options = TaskExportOptions(xml_include_plain_columns=True,
                                    xml_include_calculated=True,
                                    include_blobs=False)
        for t in self.collection.all_tasks:
            branches.append(t.get_xml_root(self.req, options))
            audit(
                self.req,
                audit_string,
                table=t.tablename,
                server_pk=t.pk,
                patient_server_pk=t.get_patient_server_pk()
            )
        tree = XmlElement(name=xml_name, value=branches)
        return get_xml_document(
            tree,
            indent_spaces=indent_spaces,
            eol=eol,
            include_comments=include_comments
        )

    # -------------------------------------------------------------------------
    # HTML view
    # -------------------------------------------------------------------------

    def get_html(self) -> str:
        """
        Get HTML representing this object.
        """
        self.req.prepare_for_html_figures()
        return self._get_html()

    # -------------------------------------------------------------------------
    # PDF view
    # -------------------------------------------------------------------------

    def get_pdf_html(self) -> str:
        """
        Returns HTML to be made into a PDF representing this object.
        """
        self.req.prepare_for_pdf_figures()
        return self._get_pdf_html()

    def get_pdf(self) -> bytes:
        """
        Get PDF representing tracker/CTV.
        """
        req = self.req
        html = self.get_pdf_html()  # main content
        if CSS_PAGED_MEDIA:
            return pdf_from_html(req, html)
        else:
            return pdf_from_html(
                req,
                html=html,
                header_html=render(
                    "wkhtmltopdf_header.mako",
                    dict(inner_text=render("tracker_ctv_header.mako",
                                           dict(tracker=self),
                                           request=req)),
                    request=req
                ),
                footer_html=render(
                    "wkhtmltopdf_footer.mako",
                    dict(inner_text=render("tracker_ctv_footer.mako",
                                           dict(tracker=self),
                                           request=req)),
                    request=req
                ),
                extra_wkhtmltopdf_options={
                    "orientation": "Portrait"
                }
            )

    def suggested_pdf_filename(self) -> str:
        """
        Get suggested filename for tracker/CTV PDF.
        """
        cfg = self.req.config
        return get_export_filename(
            req=self.req,
            patient_spec_if_anonymous=cfg.patient_spec_if_anonymous,
            patient_spec=cfg.patient_spec,
            filename_spec=cfg.ctv_filename_spec if self.as_ctv else cfg.tracker_filename_spec,  # noqa
            filetype=ViewArg.PDF,
            is_anonymous=self.patient is None,
            surname=self.patient.get_surname() if self.patient else "",
            forename=self.patient.get_forename() if self.patient else "",
            dob=self.patient.get_dob() if self.patient else None,
            sex=self.patient.get_sex() if self.patient else None,
            idnum_objects=self.patient.get_idnum_objects() if self.patient else None,  # noqa
            creation_datetime=None,
            basetable=None,
            serverpk=None
        )


# =============================================================================
# Tracker class
# =============================================================================

class Tracker(TrackerCtvCommon):
    """
    Class representing a numerical tracker.
    """

    def __init__(self,
                 req: "CamcopsRequest",
                 taskfilter: TaskFilter,
                 via_index: bool = True) -> None:
        super().__init__(
            req=req,
            taskfilter=taskfilter,
            as_ctv=False,
            via_index=via_index
        )

    def get_xml(self,
                indent_spaces: int = 4,
                eol: str = '\n',
                include_comments: bool = False) -> str:
        return self._get_xml(
            audit_string="Tracker XML accessed",
            xml_name="tracker",
            indent_spaces=indent_spaces,
            eol=eol,
            include_comments=include_comments
        )

    def _get_html(self) -> str:
        return render("tracker.mako",
                      dict(tracker=self,
                           viewtype=ViewArg.HTML),
                      request=self.req)

    def _get_pdf_html(self) -> str:
        return render("tracker.mako",
                      dict(tracker=self,
                           pdf_landscape=False,
                           viewtype=ViewArg.PDF),
                      request=self.req)

    # -------------------------------------------------------------------------
    # Plotting
    # -------------------------------------------------------------------------

    def get_all_plots_for_one_task_html(self, tasks: List[Task]) -> str:
        """
        HTML for all plots for a given task type.
        """
        html = ""
        ntasks = len(tasks)
        if ntasks == 0:
            return html
        if not tasks[0].provides_trackers:
            # ask the first of the task instances
            return html
        alltrackers = [task.get_trackers(self.req) for task in tasks]
        datetimes = [task.get_creation_datetime() for task in tasks]
        ntrackers = len(alltrackers[0])
        # ... number of trackers supplied by the first task (and all tasks)
        for tracker in range(ntrackers):
            values = [
                alltrackers[tasknum][tracker].value
                for tasknum in range(ntasks)
            ]
            html += self.get_single_plot_html(
                datetimes, values,
                specimen_tracker=alltrackers[0][tracker]
            )
        for task in tasks:
            audit(self.req,
                  "Tracker data accessed",
                  table=task.tablename,
                  server_pk=task.pk,
                  patient_server_pk=task.get_patient_server_pk())
        return html

    def get_single_plot_html(self,
                             datetimes: List[Pendulum],
                             values: List[Optional[float]],
                             specimen_tracker: "TrackerInfo") -> str:
        """
        HTML for a single figure.
        """
        nonblank_values = [x for x in values if x is not None]
        # NB DIFFERENT to list(filter(None, values)), which implements the
        # test "if x", not "if x is not None" -- thus eliminating zero values!
        # We don't want that.
        if not nonblank_values:
            return ""

        plot_label = specimen_tracker.plot_label
        axis_label = specimen_tracker.axis_label
        axis_min = specimen_tracker.axis_min
        axis_max = specimen_tracker.axis_max
        axis_ticks = specimen_tracker.axis_ticks
        horizontal_lines = specimen_tracker.horizontal_lines
        horizontal_labels = specimen_tracker.horizontal_labels
        aspect_ratio = specimen_tracker.aspect_ratio

        figsize = (
            PlotDefaults.FULLWIDTH_PLOT_WIDTH,
            (1.0 / float(aspect_ratio)) * PlotDefaults.FULLWIDTH_PLOT_WIDTH
        )
        fig = self.req.create_figure(figsize=figsize)
        ax = fig.add_subplot(MatplotlibConstants.WHOLE_PANEL)
        x = [matplotlib.dates.date2num(t) for t in datetimes]
        datelabels = [dt.strftime(TRACKER_DATEFORMAT) for dt in datetimes]

        # Plot lines and markers (on top of lines)
        ax.plot(
            x,  # x
            values,  # y
            color=MatplotlibConstants.COLOUR_BLUE,  # line colour
            linestyle=MatplotlibConstants.LINESTYLE_SOLID,
            marker=MatplotlibConstants.MARKER_PLUS,  # point shape
            markeredgecolor=MatplotlibConstants.COLOUR_RED,  # point colour
            markerfacecolor=MatplotlibConstants.COLOUR_RED,  # point colour
            label=None,
            zorder=PlotDefaults.ZORDER_DATA_LINES_POINTS
        )

        # x axis
        ax.set_xlabel("Date/time", fontdict=self.req.fontdict)
        ax.set_xticks(x)
        ax.set_xticklabels(datelabels, fontdict=self.req.fontdict)
        if (self.earliest is not None and
                self.latest is not None and
                self.earliest != self.latest):
            xlim = matplotlib.dates.date2num((self.earliest, self.latest))
            margin = (2.5 / 95.0) * (xlim[1] - xlim[0])
            xlim[0] -= margin
            xlim[1] += margin
            ax.set_xlim(xlim)
        xlim = ax.get_xlim()
        fig.autofmt_xdate(rotation=90)
        # ... autofmt_xdate must be BEFORE twinx:
        # http://stackoverflow.com/questions/8332395
        if axis_ticks is not None and len(axis_ticks) > 0:
            tick_positions = [m.y for m in axis_ticks]
            tick_labels = [m.label for m in axis_ticks]
            ax.set_yticks(tick_positions)
            ax.set_yticklabels(tick_labels, fontdict=self.req.fontdict)

        # y axis
        ax.set_ylabel(axis_label, fontdict=self.req.fontdict)
        axis_min = (
            min(axis_min, min(nonblank_values))
            if axis_min is not None
            else min(nonblank_values)
        )
        axis_max = (
            max(axis_max, max(nonblank_values))
            if axis_max is not None
            else max(nonblank_values)
        )
        # ... the supplied values are stretched if the data are outside them
        # ... but min(something, None) is None, so beware
        # If we get something with no sense of scale whatsoever, then what
        # we do is arbitrary. Matplotlib does its own thing, but we could do:
        if axis_min == axis_max:
            if axis_min == 0:
                axis_min, axis_min = -1.0, 1.0
            else:
                singlevalue = axis_min
                axis_min = 0.9 * singlevalue
                axis_max = 1.1 * singlevalue
                if axis_min > axis_max:
                    axis_min, axis_max = axis_max, axis_min
        ax.set_ylim(axis_min, axis_max)

        # title
        ax.set_title(plot_label, fontdict=self.req.fontdict)

        # Horizontal lines
        stupid_jitter = 0.001
        if horizontal_lines is not None:
            for y in horizontal_lines:
                ax.plot(
                    xlim,  # x
                    [y, y + stupid_jitter],  # y
                    color=MatplotlibConstants.COLOUR_GREY_50,
                    linestyle=MatplotlibConstants.LINESTYLE_DOTTED,
                    zorder=PlotDefaults.ZORDER_PRESET_LINES
                )
                # PROBLEM: horizontal lines becoming invisible
                # (whether from ax.axhline or plot)

        # Horizontal labels
        if horizontal_labels is not None:
            label_left = xlim[0] + 0.01 * (xlim[1] - xlim[0])
            for lab in horizontal_labels:
                y = lab.y
                l_ = lab.label
                va = lab.vertical_alignment.value
                ax.text(
                    label_left,  # x
                    y,  # y
                    l_,  # text
                    verticalalignment=va,
                    # alpha=0.5,
                    # ... was "0.5" rather than 0.5, which led to a
                    # tricky-to-find "TypeError: a float is required" exception
                    # after switching to Python 3.
                    # ... and switched to grey colour with zorder on 2020-06-28
                    # after wkhtmltopdf 0.12.5 had problems rendering
                    # opacity=0.5 with SVG lines
                    color=MatplotlibConstants.COLOUR_GREY_50,
                    fontdict=self.req.fontdict,
                    zorder=PlotDefaults.ZORDER_PRESET_LABELS
                )

        self.req.set_figure_font_sizes(ax)

        fig.tight_layout()
        # ... stop the labels dropping off
        # (only works properly for LEFT labels...)

        # http://matplotlib.org/faq/howto_faq.html
        # ... tried it - didn't work (internal numbers change fine,
        # check the logger, but visually doesn't help)
        # - http://stackoverflow.com/questions/9126838
        # - http://matplotlib.org/examples/pylab_examples/finance_work2.html
        return self.req.get_html_from_pyplot_figure(fig) + "<br>"
        # ... extra line break for the PDF rendering


# =============================================================================
# ClinicalTextView class
# =============================================================================

class ClinicalTextView(TrackerCtvCommon):
    """
    Class representing a clinical text view.
    """

    def __init__(self,
                 req: "CamcopsRequest",
                 taskfilter: TaskFilter,
                 via_index: bool = True) -> None:
        super().__init__(
            req=req,
            taskfilter=taskfilter,
            as_ctv=True,
            via_index=via_index
        )

    def get_xml(self,
                indent_spaces: int = 4,
                eol: str = '\n',
                include_comments: bool = False) -> str:
        return self._get_xml(
            audit_string="Clinical text view XML accessed",
            xml_name="ctv",
            indent_spaces=indent_spaces,
            eol=eol,
            include_comments=include_comments
        )

    def _get_html(self) -> str:
        return render("ctv.mako",
                      dict(tracker=self,
                           viewtype=ViewArg.HTML),
                      request=self.req)

    def _get_pdf_html(self) -> str:
        return render("ctv.mako",
                      dict(tracker=self,
                           pdf_landscape=False,
                           viewtype=ViewArg.PDF),
                      request=self.req)
