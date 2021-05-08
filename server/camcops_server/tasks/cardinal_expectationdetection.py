#!/usr/bin/env python

"""
camcops_server/tasks/cardinal_expectationdetection.py

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

import logging
from typing import Any, Dict, List, Optional, Sequence, Tuple, Type

from cardinal_pythonlib.logs import BraceStyleAdapter
from matplotlib.axes import Axes
import numpy
import scipy.stats  # http://docs.scipy.org/doc/scipy/reference/stats.html
from sqlalchemy.sql.schema import Column, ForeignKey
from sqlalchemy.sql.sqltypes import Float, Integer

from camcops_server.cc_modules.cc_constants import (
    CssClass,
    MatplotlibConstants,
    PlotDefaults,
)
from camcops_server.cc_modules.cc_db import (
    ancillary_relationship,
    GenericTabletRecordMixin,
    TaskDescendant,
)
from camcops_server.cc_modules.cc_html import (
    answer,
    div,
    get_yes_no_none,
    identity,
    italic,
    td,
    tr,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    PendulumDateTimeAsIsoTextColType,
)
from camcops_server.cc_modules.cc_sqlalchemy import Base
from camcops_server.cc_modules.cc_summaryelement import (
    ExtraSummaryTable,
    SummaryElement,
)
from camcops_server.cc_modules.cc_task import Task, TaskHasPatientMixin

log = BraceStyleAdapter(logging.getLogger(__name__))


CONVERT_0_P_TO = 0.001  # for Z-transformed ROC plot
CONVERT_1_P_TO = 0.999  # for Z-transformed ROC plot

NRATINGS = 5  # numbered 0-4 in the database
# -- to match DETECTION_OPTIONS.length in the original task
N_CUES = 8  # to match magic number in original task

ERROR_RATING_OUT_OF_RANGE = f"""
    <div class="{CssClass.ERROR}">Can't draw figure: rating out of range</div>
"""
WARNING_INSUFFICIENT_DATA = f"""
    <div class="{CssClass.WARNING}">Insufficient data</div>
"""
WARNING_RATING_MISSING = f"""
    <div class="{CssClass.WARNING}">One or more ratings are missing</div>
"""
PLAIN_ROC_TITLE = "ROC"
Z_ROC_TITLE = (
    f"ROC in Z coordinates (0/1 first mapped to "
    f"{CONVERT_0_P_TO}/{CONVERT_1_P_TO})"
)

AUDITORY = 0
VISUAL = 1


def a(x: Any) -> str:
    """Answer formatting for this task."""
    return answer(x, formatter_answer=identity, default="")


# =============================================================================
# Cardinal_ExpectationDetection
# =============================================================================

class ExpDetTrial(GenericTabletRecordMixin, TaskDescendant, Base):
    __tablename__ = "cardinal_expdet_trials"

    cardinal_expdet_id = Column(
        "cardinal_expdet_id", Integer,
        nullable=False,
        comment="FK to cardinal_expdet"
    )
    trial = Column(
        "trial", Integer,
        nullable=False,
        comment="Trial number (0-based)"
    )

    # Config determines these (via an autogeneration process):
    block = Column(
        "block", Integer,
        comment="Block number (0-based)"
    )
    group_num = Column(
        "group_num", Integer,
        comment="Group number (0-based)"
    )
    cue = Column(
        "cue", Integer,
        comment="Cue number (0-based)"
    )
    raw_cue_number = Column(
        "raw_cue_number", Integer,
        comment="Raw cue number (following counterbalancing) (0-based)"
    )
    target_modality = Column(
        "target_modality", Integer,
        comment="Target modality (0 auditory, 1 visual)"
    )
    target_number = Column(
        "target_number", Integer,
        comment="Target number (0-based)"
    )
    target_present = Column(
        "target_present", Integer,
        comment="Target present? (0 no, 1 yes)"
    )
    iti_length_s = Column(
        "iti_length_s", Float,
        comment="Intertrial interval (s)"
    )

    # Task determines these (on the fly):
    pause_given_before_trial = Column(
        "pause_given_before_trial", Integer,
        comment="Pause given before trial? (0 no, 1 yes)"
    )
    pause_start_time = Column(
        "pause_start_time", PendulumDateTimeAsIsoTextColType,
        comment="Pause start time (ISO-8601)"
    )
    pause_end_time = Column(
        "pause_end_time", PendulumDateTimeAsIsoTextColType,
        comment="Pause end time (ISO-8601)"
    )
    trial_start_time = Column(
        "trial_start_time", PendulumDateTimeAsIsoTextColType,
        comment="Trial start time (ISO-8601)"
    )
    cue_start_time = Column(
        "cue_start_time", PendulumDateTimeAsIsoTextColType,
        comment="Cue start time (ISO-8601)"
    )
    target_start_time = Column(
        "target_start_time", PendulumDateTimeAsIsoTextColType,
        comment="Target start time (ISO-8601)"
    )
    detection_start_time = Column(
        "detection_start_time", PendulumDateTimeAsIsoTextColType,
        comment="Detection response start time (ISO-8601)"
    )
    iti_start_time = Column(
        "iti_start_time", PendulumDateTimeAsIsoTextColType,
        comment="Intertrial interval start time (ISO-8601)"
    )
    iti_end_time = Column(
        "iti_end_time", PendulumDateTimeAsIsoTextColType,
        comment="Intertrial interval end time (ISO-8601)"
    )
    trial_end_time = Column(
        "trial_end_time", PendulumDateTimeAsIsoTextColType,
        comment="Trial end time (ISO-8601)"
    )

    # Subject decides these:
    responded = Column(
        "responded", Integer,
        comment="Responded? (0 no, 1 yes)"
    )
    response_time = Column(
        "response_time", PendulumDateTimeAsIsoTextColType,
        comment="Response time (ISO-8601)"
    )
    response_latency_ms = Column(
        "response_latency_ms", Integer,
        comment="Response latency (ms)"
    )
    rating = Column(
        "rating", Integer,
        comment="Rating (0 definitely not - 4 definitely)"
    )
    correct = Column(
        "correct", Integer,
        comment="Correct side of the middle rating? (0 no, 1 yes)"
    )
    points = Column(
        "points", Integer,
        comment="Points earned this trial"
    )
    cumulative_points = Column(
        "cumulative_points", Integer,
        comment="Cumulative points earned"
    )

    @classmethod
    def get_html_table_header(cls) -> str:
        return f"""
            <table class="{CssClass.EXTRADETAIL}">
                <tr>
                    <th>Trial# (0-based)</th>
                    <th>Block# (0-based)</th>
                    <th>Group# (0-based)</th>
                    <th>Cue</th>
                    <th>Raw cue</th>
                    <th>Target modality</th>
                    <th>Target#</th>
                    <th>Target present?</th>
                    <th>ITI (s)</th>
                    <th>Pause before trial?</th>
                </tr>
                <tr class="{CssClass.EXTRADETAIL2}">
                    <th>...</th>
                    <th>Pause start@</th>
                    <th>Pause end@</th>
                    <th>Trial start@</th>
                    <th>Cue@</th>
                    <th>Target@</th>
                    <th>Detection start@</th>
                    <th>ITI start@</th>
                    <th>ITI end@</th>
                    <th>Trial end@</th>
                </tr>
                <tr class="{CssClass.EXTRADETAIL2}">
                    <th>...</th>
                    <th>Responded?</th>
                    <th>Responded@</th>
                    <th>Response latency (ms)</th>
                    <th>Rating</th>
                    <th>Correct?</th>
                    <th>Points</th>
                    <th>Cumulative points</th>
                </tr>
        """

    # ratings: 0, 1 absent -- 2 don't know -- 3, 4 present
    def judged_present(self) -> Optional[bool]:
        if not self.responded:
            return None
        elif self.rating >= 3:
            return True
        else:
            return False

    def judged_absent(self) -> Optional[bool]:
        if not self.responded:
            return None
        elif self.rating <= 1:
            return True
        else:
            return False

    def didnt_know(self) -> Optional[bool]:
        if not self.responded:
            return None
        return self.rating == 2

    def get_html_table_row(self) -> str:
        return tr(
            a(self.trial),
            a(self.block),
            a(self.group_num),
            a(self.cue),
            a(self.raw_cue_number),
            a(self.target_modality),
            a(self.target_number),
            a(self.target_present),
            a(self.iti_length_s),
            a(self.pause_given_before_trial),
        ) + tr(
            "...",
            a(self.pause_start_time),
            a(self.pause_end_time),
            a(self.trial_start_time),
            a(self.cue_start_time),
            a(self.target_start_time),
            a(self.detection_start_time),
            a(self.iti_start_time),
            a(self.iti_end_time),
            a(self.trial_end_time),
            tr_class=CssClass.EXTRADETAIL2
        ) + tr(
            "...",
            a(self.responded),
            a(self.response_time),
            a(self.response_latency_ms),
            a(self.rating),
            a(self.correct),
            a(self.points),
            a(self.cumulative_points),
            tr_class=CssClass.EXTRADETAIL2
        )

    # -------------------------------------------------------------------------
    # TaskDescendant overrides
    # -------------------------------------------------------------------------

    @classmethod
    def task_ancestor_class(cls) -> Optional[Type["Task"]]:
        return CardinalExpectationDetection

    def task_ancestor(self) -> Optional["CardinalExpectationDetection"]:
        return CardinalExpectationDetection.get_linked(
            self.cardinal_expdet_id, self)


class ExpDetTrialGroupSpec(GenericTabletRecordMixin, TaskDescendant, Base):
    __tablename__ = "cardinal_expdet_trialgroupspec"

    cardinal_expdet_id = Column(
        "cardinal_expdet_id", Integer,
        nullable=False,
        comment="FK to cardinal_expdet"
    )
    group_num = Column(
        "group_num", Integer,
        nullable=False,
        comment="Group number (0-based)"
    )

    # Group spec
    cue = Column(
        "cue", Integer,
        comment="Cue number (0-based)"
    )
    target_modality = Column(
        "target_modality", Integer,
        comment="Target modality (0 auditory, 1 visual)"
    )
    target_number = Column(
        "target_number", Integer,
        comment="Target number (0-based)"
    )
    n_target = Column(
        "n_target", Integer,
        comment="Number of trials with target present"
    )
    n_no_target = Column(
        "n_no_target", Integer,
        comment="Number of trials with target absent"
    )

    DP = 3

    @classmethod
    def get_html_table_header(cls) -> str:
        return f"""
            <table class="{CssClass.EXTRADETAIL}">
                <tr>
                    <th>Group# (0-based)</th>
                    <th>Cue (0-based)</th>
                    <th>Target modality (0 auditory, 1 visual)</th>
                    <th>Target# (0-based)</th>
                    <th># target trials</th>
                    <th># no-target trials</th>
                </tr>
        """

    def get_html_table_row(self) -> str:
        return tr(
            a(self.group_num),
            a(self.cue),
            a(self.target_modality),
            a(self.target_number),
            a(self.n_target),
            a(self.n_no_target),
        )

    # -------------------------------------------------------------------------
    # TaskDescendant overrides
    # -------------------------------------------------------------------------

    @classmethod
    def task_ancestor_class(cls) -> Optional[Type["Task"]]:
        return CardinalExpectationDetection

    def task_ancestor(self) -> Optional["CardinalExpectationDetection"]:
        return CardinalExpectationDetection.get_linked(
            self.cardinal_expdet_id, self)


class CardinalExpectationDetection(TaskHasPatientMixin, Task):
    """
    Server implementation of the Cardinal_ExpDet task.
    """
    __tablename__ = "cardinal_expdet"
    shortname = "Cardinal_ExpDet"
    use_landscape_for_pdf = True

    # Config
    num_blocks = Column(
        "num_blocks", Integer,
        comment="Number of blocks"
    )
    stimulus_counterbalancing = Column(
        "stimulus_counterbalancing", Integer,
        comment="Stimulus counterbalancing condition"
    )
    is_detection_response_on_right = Column(
        "is_detection_response_on_right", Integer,
        comment='Is the "detection" response on the right? (0 no, 1 yes)'
    )
    pause_every_n_trials = Column(
        "pause_every_n_trials", Integer,
        comment="Pause every n trials"
    )
    # ... cue
    cue_duration_s = Column(
        "cue_duration_s", Float,
        comment="Cue duration (s)"
    )
    visual_cue_intensity = Column(
        "visual_cue_intensity", Float,
        comment="Visual cue intensity (0.0-1.0)"
    )
    auditory_cue_intensity = Column(
        "auditory_cue_intensity", Float,
        comment="Auditory cue intensity (0.0-1.0)"
    )
    # ... ISI
    isi_duration_s = Column(
        "isi_duration_s", Float,
        comment="Interstimulus interval (s)"
    )
    # .. target
    visual_target_duration_s = Column(
        "visual_target_duration_s", Float,
        comment="Visual target duration (s)"
    )
    visual_background_intensity = Column(
        "visual_background_intensity", Float,
        comment="Visual background intensity (0.0-1.0)"
    )
    visual_target_0_intensity = Column(
        "visual_target_0_intensity", Float,
        comment="Visual target 0 intensity (0.0-1.0)"
    )
    visual_target_1_intensity = Column(
        "visual_target_1_intensity", Float,
        comment="Visual target 1 intensity (0.0-1.0)"
    )
    auditory_background_intensity = Column(
        "auditory_background_intensity", Float,
        comment="Auditory background intensity (0.0-1.0)"
    )
    auditory_target_0_intensity = Column(
        "auditory_target_0_intensity", Float,
        comment="Auditory target 0 intensity (0.0-1.0)"
    )
    auditory_target_1_intensity = Column(
        "auditory_target_1_intensity", Float,
        comment="Auditory target 1 intensity (0.0-1.0)"
    )
    # ... ITI
    iti_min_s = Column(
        "iti_min_s", Float,
        comment="Intertrial interval minimum (s)"
    )
    iti_max_s = Column(
        "iti_max_s", Float,
        comment="Intertrial interval maximum (s)"
    )

    # Results
    aborted = Column(
        "aborted", Integer,
        comment="Was the task aborted? (0 no, 1 yes)"
    )
    finished = Column(
        "finished", Integer,
        comment="Was the task finished? (0 no, 1 yes)"
    )
    last_trial_completed = Column(
        "last_trial_completed", Integer,
        comment="Number of last trial completed"
    )

    # Relationships
    trials = ancillary_relationship(
        parent_class_name="CardinalExpectationDetection",
        ancillary_class_name="ExpDetTrial",
        ancillary_fk_to_parent_attr_name="cardinal_expdet_id",
        ancillary_order_by_attr_name="trial"
    )  # type: List[ExpDetTrial]
    groupspecs = ancillary_relationship(
        parent_class_name="CardinalExpectationDetection",
        ancillary_class_name="ExpDetTrialGroupSpec",
        ancillary_fk_to_parent_attr_name="cardinal_expdet_id",
        ancillary_order_by_attr_name="group_num"
    )  # type: List[ExpDetTrialGroupSpec]

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Cardinal RN – Expectation–Detection task")

    def is_complete(self) -> bool:
        return bool(self.finished)

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(name="final_score",
                           coltype=Integer(),
                           value=self.get_final_score()),
            SummaryElement(name="overall_p_detect_present",
                           coltype=Float(),
                           value=self.get_overall_p_detect_present()),
            SummaryElement(name="overall_p_detect_absent",
                           coltype=Float(),
                           value=self.get_overall_p_detect_absent()),
            SummaryElement(name="overall_c",
                           coltype=Float(),
                           value=self.get_overall_c()),
            SummaryElement(name="overall_d",
                           coltype=Float(),
                           value=self.get_overall_d()),
        ]

    def get_final_score(self) -> Optional[int]:
        trialarray = self.trials
        if not trialarray:
            return None
        return trialarray[-1].cumulative_points

    def get_group_html(self) -> str:
        grouparray = self.groupspecs
        html = ExpDetTrialGroupSpec.get_html_table_header()
        for g in grouparray:
            html += g.get_html_table_row()
        html += """</table>"""
        return html

    @staticmethod
    def get_c_dprime(h: Optional[float],
                     fa: Optional[float],
                     two_alternative_forced_choice: bool = False) \
            -> Tuple[Optional[float], Optional[float]]:
        if h is None or fa is None:
            return None, None
        # In this task, we're only presenting a single alternative.
        if fa == 0:
            fa = CONVERT_0_P_TO
        if fa == 1:
            fa = CONVERT_1_P_TO
        if h == 0:
            h = CONVERT_0_P_TO
        if h == 1:
            h = CONVERT_1_P_TO
        z_fa = scipy.stats.norm.ppf(fa)
        z_h = scipy.stats.norm.ppf(h)
        if two_alternative_forced_choice:
            dprime = (1.0 / numpy.sqrt(2)) * (z_h - z_fa)
        else:
            dprime = z_h - z_fa
        c = -0.5 * (z_h + z_fa)
        # c is zero when FA rate and miss rate (1 - H) are equal
        # c is negative when FA > miss
        # c is positive when miss > FA
        return c, dprime

    @staticmethod
    def get_sdt_values(count_stimulus: Sequence[int],
                       count_nostimulus: Sequence[int]) -> Dict:
        # Probabilities and cumulative probabilities
        sum_count_stimulus = numpy.sum(count_stimulus)
        sum_count_nostimulus = numpy.sum(count_nostimulus)
        if sum_count_stimulus == 0 or sum_count_nostimulus == 0:
            fa = []
            h = []
            z_fa = []
            z_h = []
        else:
            p_stimulus = count_stimulus / sum_count_stimulus
            p_nostimulus = count_nostimulus / sum_count_nostimulus
            # ... may produce a RuntimeWarning in case of division by zero
            cump_stimulus = numpy.cumsum(p_stimulus)  # hit rates
            cump_nostimulus = numpy.cumsum(p_nostimulus)  # false alarm rates
            # We're interested in all pairs except the last:
            fa = cump_stimulus[:-1]
            h = cump_nostimulus[:-1]
            # WHICH WAY ROUND YOU ASSIGN THESE DETERMINES THE ROC'S APPEARANCE.
            # However, it's arbitrary, in the sense that the left/right
            # assignment of the ratings is arbitrary. To make the ROC look
            # conventional (top left), assign this way round, so that "fa"
            # starts low and grows, and "h" starts high and falls. Hmm...
            fa[fa == 0] = CONVERT_0_P_TO
            fa[fa == 1] = CONVERT_1_P_TO
            h[h == 0] = CONVERT_0_P_TO
            h[h == 1] = CONVERT_1_P_TO
            z_fa = scipy.stats.norm.ppf(fa)
            z_h = scipy.stats.norm.ppf(h)

            # log.debug("p_stimulus: " + str(p_stimulus))
            # log.debug("p_nostimulus: " + str(p_nostimulus))
            # log.debug("cump_stimulus: " + str(cump_stimulus))
            # log.debug("cump_nostimulus: " + str(cump_nostimulus))

        # log.debug("h: " + str(h))
        # log.debug("fa: " + str(fa))
        # log.debug("z_h: " + str(z_h))
        # log.debug("z_fa: " + str(z_fa))
        return {
            "fa": fa,
            "h": h,
            "z_fa": z_fa,
            "z_h": z_h,
        }

    def plot_roc(self,
                 req: CamcopsRequest,
                 ax: Axes,
                 count_stimulus: Sequence[int],
                 count_nostimulus: Sequence[int],
                 show_x_label: bool,
                 show_y_label: bool,
                 plainroc: bool,
                 subtitle: str) -> None:
        extraspace = 0.05
        sdtval = self.get_sdt_values(count_stimulus, count_nostimulus)

        # Calculate d' for all pairs but the last
        if plainroc:
            x = sdtval["fa"]
            y = sdtval["h"]
            xlabel = "FA"
            ylabel = "H"
            ax.set_xlim(0 - extraspace, 1 + extraspace)
            ax.set_ylim(0 - extraspace, 1 + extraspace)
        else:
            x = sdtval["z_fa"]
            y = sdtval["z_h"]
            xlabel = "Z(FA)"
            ylabel = "Z(H)"
        # Plot
        ax.plot(
            x, y,
            marker=MatplotlibConstants.MARKER_PLUS,
            color=MatplotlibConstants.COLOUR_BLUE,
            linestyle=MatplotlibConstants.LINESTYLE_SOLID
        )
        ax.set_xlabel(xlabel if show_x_label else "", fontdict=req.fontdict)
        ax.set_ylabel(ylabel if show_y_label else "", fontdict=req.fontdict)
        ax.set_title(subtitle, fontdict=req.fontdict)
        req.set_figure_font_sizes(ax)

    @staticmethod
    def get_roc_info(trialarray: List[ExpDetTrial],
                     blocks: List[int],
                     groups: Optional[List[int]]) -> Dict:
        # Collect counts (Macmillan & Creelman p61)
        total_n = 0
        count_stimulus = numpy.zeros(NRATINGS)
        count_nostimulus = numpy.zeros(NRATINGS)
        rating_missing = False
        rating_out_of_range = False
        for t in trialarray:
            if t.rating is None:
                rating_missing = True
                continue
            if t.rating < 0 or t.rating >= NRATINGS:
                rating_out_of_range = True
                break
            if groups and t.group_num not in groups:
                continue
            if blocks and t.block not in blocks:
                continue
            total_n += 1
            if t.target_present:
                count_stimulus[t.rating] += 1
            else:
                count_nostimulus[t.rating] += 1
        return {
            "total_n": total_n,
            "count_stimulus": count_stimulus,
            "count_nostimulus": count_nostimulus,
            "rating_missing": rating_missing,
            "rating_out_of_range": rating_out_of_range,
        }

    def get_roc_figure_by_group(self,
                                req: CamcopsRequest,
                                trialarray: List[ExpDetTrial],
                                grouparray: List[ExpDetTrialGroupSpec],
                                plainroc: bool) -> str:
        if not trialarray or not grouparray:
            return WARNING_INSUFFICIENT_DATA
        figsize = (
            PlotDefaults.FULLWIDTH_PLOT_WIDTH * 2,
            PlotDefaults.FULLWIDTH_PLOT_WIDTH
        )
        html = ""
        fig = req.create_figure(figsize=figsize)
        warned = False
        for groupnum in range(len(grouparray)):
            ax = fig.add_subplot(2, 4, groupnum+1)
            # ... rows, cols, plotnum (in reading order from 1)
            rocinfo = self.get_roc_info(trialarray, [], [groupnum])
            if rocinfo["rating_out_of_range"]:
                return ERROR_RATING_OUT_OF_RANGE
            if rocinfo["rating_missing"] and not warned:
                html += WARNING_RATING_MISSING
                warned = True
            show_x_label = (groupnum > 3)
            show_y_label = (groupnum % 4 == 0)
            subtitle = f"Group {groupnum} (n = {rocinfo['total_n']})"
            self.plot_roc(
                req,
                ax,
                rocinfo["count_stimulus"],
                rocinfo["count_nostimulus"],
                show_x_label,
                show_y_label,
                plainroc,
                subtitle
            )
        title = PLAIN_ROC_TITLE if plainroc else Z_ROC_TITLE
        fontprops = req.fontprops
        fontprops.set_weight("bold")
        fig.suptitle(title, fontproperties=fontprops)
        html += req.get_html_from_pyplot_figure(fig)
        return html

    def get_roc_figure_firsthalf_lasthalf(self,
                                          req: CamcopsRequest,
                                          trialarray: List[ExpDetTrial],
                                          plainroc: bool) -> str:
        if not trialarray or not self.num_blocks:
            return WARNING_INSUFFICIENT_DATA
        figsize = (
            PlotDefaults.FULLWIDTH_PLOT_WIDTH,
            PlotDefaults.FULLWIDTH_PLOT_WIDTH / 2
        )
        html = ""
        fig = req.create_figure(figsize=figsize)
        warned = False
        for half in range(2):
            ax = fig.add_subplot(1, 2, half+1)
            # ... rows, cols, plotnum (in reading order from 1)
            blocks = list(range(half * self.num_blocks // 2,
                                self.num_blocks // (2 - half)))
            rocinfo = self.get_roc_info(trialarray, blocks, None)
            if rocinfo["rating_out_of_range"]:
                return ERROR_RATING_OUT_OF_RANGE
            if rocinfo["rating_missing"] and not warned:
                html += WARNING_RATING_MISSING
                warned = True
            show_x_label = True
            show_y_label = (half == 0)
            subtitle = "First half" if half == 0 else "Second half"
            self.plot_roc(
                req,
                ax,
                rocinfo["count_stimulus"],
                rocinfo["count_nostimulus"],
                show_x_label,
                show_y_label,
                plainroc,
                subtitle
            )
        title = PLAIN_ROC_TITLE if plainroc else Z_ROC_TITLE
        fontprops = req.fontprops
        fontprops.set_weight("bold")
        fig.suptitle(title, fontproperties=fontprops)
        html += req.get_html_from_pyplot_figure(fig)
        return html

    def get_trial_html(self) -> str:
        trialarray = self.trials
        html = ExpDetTrial.get_html_table_header()
        for t in trialarray:
            html += t.get_html_table_row()
        html += """</table>"""
        return html

    def get_task_html(self, req: CamcopsRequest) -> str:
        grouparray = self.groupspecs
        trialarray = self.trials
        # THIS IS A NON-EDITABLE TASK, so we *ignore* the problem
        # of matching to no-longer-current records.
        # (See PhotoSequence.py for a task that does it properly.)

        # Provide HTML
        # HTML
        h = f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                Putative assay of propensity to hallucinations.
            </div>
            <table class="{CssClass.TASKCONFIG}">
                <tr>
                    <th width="50%">Configuration variable</th>
                    <th width="50%">Value</th>
                </tr>
        """
        h += tr_qa("Number of blocks", self.num_blocks)
        h += tr_qa("Stimulus counterbalancing", self.stimulus_counterbalancing)
        h += tr_qa("“Detection” response on right?",
                   self.is_detection_response_on_right)
        h += tr_qa("Pause every <i>n</i> trials (0 = no pauses)",
                   self.pause_every_n_trials)
        h += tr_qa("Cue duration (s)", self.cue_duration_s)
        h += tr_qa("Visual cue intensity (0–1)", self.visual_cue_intensity)
        h += tr_qa("Auditory cue intensity (0–1)",
                   self.auditory_cue_intensity)
        h += tr_qa("ISI duration (s)", self.isi_duration_s)
        h += tr_qa("Visual target duration (s)", self.visual_target_duration_s)
        h += tr_qa("Visual background intensity",
                   self.visual_background_intensity)
        h += tr_qa("Visual target 0 (circle) intensity",
                   self.visual_target_0_intensity)
        h += tr_qa("Visual target 1 (“sun”) intensity",
                   self.visual_target_1_intensity)
        h += tr_qa("Auditory background intensity",
                   self.auditory_background_intensity)
        h += tr_qa("Auditory target 0 (tone) intensity",
                   self.auditory_target_0_intensity)
        h += tr_qa("Auditory target 1 (“moon”) intensity",
                   self.auditory_target_1_intensity)
        h += tr_qa("ITI minimum (s)", self.iti_min_s)
        h += tr_qa("ITI maximum (s)", self.iti_max_s)
        h += f"""
            </table>
            <table class="{CssClass.TASKDETAIL}">
                <tr><th width="50%">Measure</th><th width="50%">Value</th></tr>
        """
        h += tr_qa("Aborted?", get_yes_no_none(req, self.aborted))
        h += tr_qa("Finished?", get_yes_no_none(req, self.finished))
        h += tr_qa("Last trial completed", self.last_trial_completed)
        h += (
            """
                </table>
                <div>
                    Trial group specifications (one block is a full set of
                    all these trials):
                </div>
            """ +
            self.get_group_html() +
            """
                <div>
                    Detection probabilities by block and group (c &gt; 0 when
                    miss rate &gt; false alarm rate; c &lt; 0 when false alarm
                    rate &gt; miss rate):
                </div>
            """ +
            self.get_html_correct_by_group_and_block(trialarray) +
            "<div>Detection probabilities by block:</div>" +
            self.get_html_correct_by_block(trialarray) +
            "<div>Detection probabilities by group:</div>" +
            self.get_html_correct_by_group(trialarray) +
            """
                <div>
                    Detection probabilities by half and high/low association
                    probability:
                </div>
            """ +
            self.get_html_correct_by_half_and_probability(trialarray,
                                                          grouparray) +
            """
                <div>
                    Detection probabilities by block and high/low association
                    probability:
                </div>
            """ +
            self.get_html_correct_by_block_and_probability(trialarray,
                                                           grouparray) +
            """
                <div>
                    Receiver operating characteristic (ROC) curves by group:
                </div>
            """ +
            self.get_roc_figure_by_group(req, trialarray, grouparray, True) +
            self.get_roc_figure_by_group(req, trialarray, grouparray, False) +
            "<div>First-half/last-half ROCs:</div>" +
            self.get_roc_figure_firsthalf_lasthalf(req, trialarray, True) +
            "<div>Trial-by-trial results:</div>" +
            self.get_trial_html()
        )
        return h

    def get_html_correct_by_group_and_block(
            self,
            trialarray: List[ExpDetTrial]) -> str:
        if not trialarray:
            return div(italic("No trials"))
        html = f"""
            <table class="{CssClass.EXTRADETAIL}">
                <tr>
                    <th>Block</th>
        """
        for g in range(N_CUES):
            # Have spaces around | to allow web browsers to word-wrap
            html += f"""
                <th>Group {g} P(detected | present)</th>
                <th>Group {g} P(detected | absent)</th>
                <th>Group {g} c</th>
                <th>Group {g} d'</th>
            """
        html += """
                    </th>
                </tr>
        """
        for b in range(self.num_blocks):
            html += "<tr>" + td(str(b))
            for g in range(N_CUES):
                (p_detected_given_present,
                 p_detected_given_absent,
                 c,
                 dprime,
                 n_trials) = self.get_p_detected(trialarray, [b], [g])
                html += td(a(p_detected_given_present))
                html += td(a(p_detected_given_absent))
                html += td(a(c))
                html += td(a(dprime))
            html += "</tr>\n"
        html += """
            </table>
        """
        return html

    def get_html_correct_by_half_and_probability(
            self,
            trialarray: List[ExpDetTrial],
            grouparray: List[ExpDetTrialGroupSpec]) -> str:
        if (not trialarray) or (not grouparray):
            return div(italic("No trials or no groups"))
        n_target_highprob = max([x.n_target for x in grouparray])
        n_target_lowprob = min([x.n_target for x in grouparray])
        groups_highprob = [x.group_num
                           for x in grouparray
                           if x.n_target == n_target_highprob]
        groups_lowprob = [x.group_num
                          for x in grouparray
                          if x.n_target == n_target_lowprob]
        html = f"""
            <div><i>
                High probability groups (cues):
                {", ".join([str(x) for x in groups_highprob])}.\n
                Low probability groups (cues):
                {", ".join([str(x) for x in groups_lowprob])}.\n
            </i></div>
            <table class="{CssClass.EXTRADETAIL}">
                <tr>
                    <th>Half (0 first, 1 second)</th>
                    <th>Target probability given stimulus (0 low, 1 high)</th>
                    <th>P(detected | present)</th>
                    <th>P(detected | absent)</th>
                    <th>c</th>
                    <th>d'</th>
                </tr>
        """
        for half in [0, 1]:
            for prob in [0, 1]:
                blocks = list(range(half * self.num_blocks // 2,
                                    self.num_blocks // (2 - half)))
                groups = groups_lowprob if prob == 0 else groups_highprob
                (p_detected_given_present,
                 p_detected_given_absent,
                 c,
                 dprime,
                 n_trials) = self.get_p_detected(trialarray, blocks, groups)
                html += tr(
                    half,
                    a(prob),
                    a(p_detected_given_present),
                    a(p_detected_given_absent),
                    a(c),
                    a(dprime),
                )
        html += """
            </table>
        """
        return html

    def get_html_correct_by_block_and_probability(
            self,
            trialarray: List[ExpDetTrial],
            grouparray: List[ExpDetTrialGroupSpec]) -> str:
        if (not trialarray) or (not grouparray):
            return div(italic("No trials or no groups"))
        n_target_highprob = max([x.n_target for x in grouparray])
        n_target_lowprob = min([x.n_target for x in grouparray])
        groups_highprob = [x.group_num
                           for x in grouparray
                           if x.n_target == n_target_highprob]
        groups_lowprob = [x.group_num
                          for x in grouparray
                          if x.n_target == n_target_lowprob]
        html = f"""
            <div><i>
                High probability groups (cues):
                {", ".join([str(x) for x in groups_highprob])}.\n
                Low probability groups (cues):
                {", ".join([str(x) for x in groups_lowprob])}.\n
            </i></div>
            <table class="{CssClass.EXTRADETAIL}">
                <tr>
                    <th>Block (0-based)</th>
                    <th>Target probability given stimulus (0 low, 1 high)</th>
                    <th>P(detected | present)</th>
                    <th>P(detected | absent)</th>
                    <th>c</th>
                    <th>d'</th>
                </tr>
        """
        for b in range(self.num_blocks):
            for prob in [0, 1]:
                groups = groups_lowprob if prob == 0 else groups_highprob
                (p_detected_given_present,
                 p_detected_given_absent,
                 c,
                 dprime,
                 n_trials) = self.get_p_detected(trialarray, [b], groups)
                html += tr(
                    b,
                    prob,
                    a(p_detected_given_present),
                    a(p_detected_given_absent),
                    a(c),
                    a(dprime),
                )
        html += """
            </table>
        """
        return html

    def get_html_correct_by_group(self, trialarray: List[ExpDetTrial]) -> str:
        if not trialarray:
            return div(italic("No trials"))
        html = f"""
            <table class="{CssClass.EXTRADETAIL}">
                <tr>
                    <th>Group</th>
                    <th>P(detected | present)</th>
                    <th>P(detected | absent)</th>
                    <th>c</th>
                    <th>d'</th>
                </tr>
        """
        for g in range(N_CUES):
            (p_detected_given_present,
             p_detected_given_absent,
             c,
             dprime,
             n_trials) = self.get_p_detected(trialarray, None, [g])
            html += tr(
                g,
                a(p_detected_given_present),
                a(p_detected_given_absent),
                a(c),
                a(dprime),
            )
        html += """
            </table>
        """
        return html

    def get_html_correct_by_block(self, trialarray: List[ExpDetTrial]) -> str:
        if not trialarray:
            return div(italic("No trials"))
        html = f"""
            <table class="{CssClass.EXTRADETAIL}">
                <tr>
                    <th>Block</th>
                    <th>P(detected | present)</th>
                    <th>P(detected | absent)</th>
                    <th>c</th>
                    <th>d'</th>
                </tr>
        """
        for b in range(self.num_blocks):
            (p_detected_given_present,
             p_detected_given_absent,
             c,
             dprime,
             n_trials) = self.get_p_detected(trialarray, [b], None)
            html += tr(
                b,
                a(p_detected_given_present),
                a(p_detected_given_absent),
                a(c),
                a(dprime),
            )
        html += """
            </table>
        """
        return html

    def get_p_detected(self,
                       trialarray: List[ExpDetTrial],
                       blocks: Optional[List[int]],
                       groups: Optional[List[int]]) -> \
            Tuple[Optional[float], Optional[float],
                  Optional[float], Optional[float],
                  int]:
        n_present = 0
        n_absent = 0
        n_detected_given_present = 0
        n_detected_given_absent = 0
        n_trials = 0
        for t in trialarray:
            if (not t.responded or
                    (blocks is not None and t.block not in blocks) or
                    (groups is not None and t.group_num not in groups)):
                continue
            if t.target_present:
                n_present += 1
                if t.judged_present():
                    n_detected_given_present += 1
            else:
                n_absent += 1
                if t.judged_present():
                    n_detected_given_absent += 1
            n_trials += 1
        p_detected_given_present = (
            float(n_detected_given_present) / float(n_present)
        ) if n_present > 0 else None
        p_detected_given_absent = (
            float(n_detected_given_absent) / float(n_absent)
        ) if n_absent > 0 else None
        (c, dprime) = self.get_c_dprime(p_detected_given_present,
                                        p_detected_given_absent)
        # hits: p_detected_given_present
        # false alarms: p_detected_given_absent
        return (p_detected_given_present, p_detected_given_absent, c, dprime,
                n_trials)

    def get_extra_summary_tables(self, req: CamcopsRequest) \
            -> List[ExtraSummaryTable]:
        grouparray = self.groupspecs
        trialarray = self.trials
        trialarray_auditory = [x for x in trialarray
                               if x.target_modality == AUDITORY]
        blockprob_values = []  # type: List[Dict[str, Any]]
        halfprob_values = []  # type: List[Dict[str, Any]]

        if grouparray and trialarray:
            n_target_highprob = max([x.n_target for x in grouparray])
            n_target_lowprob = min([x.n_target for x in grouparray])
            groups_highprob = [x.group_num for x in grouparray
                               if x.n_target == n_target_highprob]
            groups_lowprob = [x.group_num for x in grouparray
                              if x.n_target == n_target_lowprob]
            for block in range(self.num_blocks):
                for target_probability_low_high in (0, 1):
                    groups = (
                        groups_lowprob if target_probability_low_high == 0
                        else groups_highprob
                    )
                    (p_detected_given_present,
                     p_detected_given_absent,
                     c, dprime,
                     n_trials) = self.get_p_detected(
                        trialarray, [block], groups)
                    (auditory_p_detected_given_present,
                     auditory_p_detected_given_absent,
                     auditory_c, auditory_dprime,
                     auditory_n_trials) = self.get_p_detected(
                        trialarray_auditory, [block], groups)
                    blockprob_values.append(dict(
                        cardinal_expdet_pk=self._pk,  # tablename_pk
                        n_blocks_overall=self.num_blocks,
                        block=block,
                        target_probability_low_high=target_probability_low_high,  # noqa
                        n_trials=n_trials,
                        p_detect_present=p_detected_given_present,
                        p_detect_absent=p_detected_given_absent,
                        c=c,
                        d=dprime,
                        auditory_n_trials=auditory_n_trials,
                        auditory_p_detect_present=auditory_p_detected_given_present,  # noqa
                        auditory_p_detect_absent=auditory_p_detected_given_absent,  # noqa
                        auditory_c=auditory_c,
                        auditory_d=auditory_dprime,
                    ))

            # Now another one...
            for half in range(2):
                blocks = list(range(half * self.num_blocks // 2,
                                    self.num_blocks // (2 - half)))
                for target_probability_low_high in (0, 1):
                    groups = (
                        groups_lowprob if target_probability_low_high == 0
                        else groups_highprob
                    )
                    (p_detected_given_present,
                     p_detected_given_absent,
                     c,
                     dprime,
                     n_trials) = self.get_p_detected(
                        trialarray, blocks, groups)
                    (auditory_p_detected_given_present,
                     auditory_p_detected_given_absent,
                     auditory_c, auditory_dprime,
                     auditory_n_trials) = self.get_p_detected(
                        trialarray_auditory, blocks, groups)
                    halfprob_values.append(dict(
                        cardinal_expdet_pk=self._pk,  # tablename_pk
                        half=half,
                        target_probability_low_high=target_probability_low_high,  # noqa
                        n_trials=n_trials,
                        p_detect_present=p_detected_given_present,
                        p_detect_absent=p_detected_given_absent,
                        c=c,
                        d=dprime,
                        auditory_n_trials=auditory_n_trials,
                        auditory_p_detect_present=auditory_p_detected_given_present,  # noqa
                        auditory_p_detect_absent=auditory_p_detected_given_absent,  # noqa
                        auditory_c=auditory_c,
                        auditory_d=auditory_dprime,
                    ))

        blockprob_table = ExtraSummaryTable(
            tablename="cardinal_expdet_blockprobs",
            xmlname="blockprobs",
            columns=self.get_blockprob_columns(),
            rows=blockprob_values,
            task=self
        )
        halfprob_table = ExtraSummaryTable(
            tablename="cardinal_expdet_halfprobs",
            xmlname="halfprobs",
            columns=self.get_halfprob_columns(),
            rows=halfprob_values,
            task=self
        )
        return [blockprob_table, halfprob_table]

    @staticmethod
    def get_blockprob_columns() -> List[Column]:
        # Must be a function, not a constant, because as SQLAlchemy builds the
        # tables, it assigns the Table object to each Column. Therefore, a
        # constant list works for the first request, but fails on subsequent
        # requests with e.g. "sqlalchemy.exc.ArgumentError: Column object 'id'
        # already assigned to Table 'cardinal_expdet_blockprobs'"
        return [
            Column("id", Integer, primary_key=True, autoincrement=True,
                   comment="Arbitrary PK"),
            Column("cardinal_expdet_pk", Integer,
                   ForeignKey("cardinal_expdet._pk"),
                   nullable=False,
                   comment="FK to the source table's _pk field"),
            Column("n_blocks_overall", Integer,
                   comment="Number of blocks (OVERALL)"),
            Column("block", Integer,
                   comment="Block number"),
            Column("target_probability_low_high", Integer,
                   comment="Target probability given stimulus "
                           "(0 low, 1 high)"),
            Column("n_trials", Integer,
                   comment="Number of trials in this condition"),
            Column("p_detect_present", Float,
                   comment="P(detect | present)"),
            Column("p_detect_absent", Float,
                   comment="P(detect | absent)"),
            Column("c", Float,
                   comment="c (bias; c > 0 when miss rate > false alarm rate; "
                           "c < 0 when false alarm rate > miss rate)"),
            Column("d", Float,
                   comment="d' (discriminability)"),
            Column("auditory_n_trials", Integer,
                   comment="Number of auditory trials in this condition"),
            Column("auditory_p_detect_present", Float,
                   comment="AUDITORY P(detect | present)"),
            Column("auditory_p_detect_absent", Float,
                   comment="AUDITORY P(detect | absent)"),
            Column("auditory_c", Float,
                   comment="AUDITORY c"),
            Column("auditory_d", Float,
                   comment="AUDITORY d'"),
        ]

    @staticmethod
    def get_halfprob_columns() -> List[Column]:
        return [
            Column("id", Integer, primary_key=True, autoincrement=True,
                   comment="Arbitrary PK"),
            Column("cardinal_expdet_pk", Integer,
                   ForeignKey("cardinal_expdet._pk"),
                   nullable=False,
                   comment="FK to the source table's _pk field"),
            Column("half", Integer,
                   comment="Half number"),
            Column("target_probability_low_high", Integer,
                   comment="Target probability given stimulus "
                           "(0 low, 1 high)"),
            Column("n_trials", Integer,
                   comment="Number of trials in this condition"),
            Column("p_detect_present", Float,
                   comment="P(detect | present)"),
            Column("p_detect_absent", Float,
                   comment="P(detect | absent)"),
            Column("c", Float,
                   comment="c (bias; c > 0 when miss rate > false alarm rate; "
                           "c < 0 when false alarm rate > miss rate)"),
            Column("d", Float,
                   comment="d' (discriminability)"),
            Column("auditory_n_trials", Integer,
                   comment="Number of auditory trials in this condition"),
            Column("auditory_p_detect_present", Float,
                   comment="AUDITORY P(detect | present)"),
            Column("auditory_p_detect_absent", Float,
                   comment="AUDITORY P(detect | absent)"),
            Column("auditory_c", Float,
                   comment="AUDITORY c"),
            Column("auditory_d", Float,
                   comment="AUDITORY d'"),
        ]

    def get_overall_p_detect_present(self) -> Optional[float]:
        trialarray = self.trials
        (p_detected_given_present,
         p_detected_given_absent,
         c,
         dprime,
         n_trials) = self.get_p_detected(trialarray, None, None)
        return p_detected_given_present

    def get_overall_p_detect_absent(self) -> Optional[float]:
        trialarray = self.trials
        (p_detected_given_present,
         p_detected_given_absent,
         c,
         dprime,
         n_trials) = self.get_p_detected(trialarray, None, None)
        return p_detected_given_absent

    def get_overall_c(self) -> Optional[float]:
        trialarray = self.trials
        (p_detected_given_present,
         p_detected_given_absent,
         c,
         dprime,
         n_trials) = self.get_p_detected(trialarray, None, None)
        return c

    def get_overall_d(self) -> Optional[float]:
        trialarray = self.trials
        (p_detected_given_present,
         p_detected_given_absent,
         c,
         dprime,
         n_trials) = self.get_p_detected(trialarray, None, None)
        return dprime
