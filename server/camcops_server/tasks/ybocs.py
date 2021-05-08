#!/usr/bin/env python

"""
camcops_server/tasks/ybocs.py

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

from typing import Any, Dict, List, Tuple, Type

from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Boolean, Integer, UnicodeText

from camcops_server.cc_modules.cc_constants import (
    CssClass,
    DATA_COLLECTION_UNLESS_UPGRADED_DIV,
)
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_html import (
    answer,
    get_ternary,
    subheading_spanning_four_columns,
    tr,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    BIT_CHECKER,
    CamcopsColumn,
    PermittedValueChecker,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# Y-BOCS
# =============================================================================

class YbocsMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Ybocs'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        cls.TARGET_COLUMNS = []  # type: List[Column]
        for target in ["obsession", "compulsion", "avoidance"]:
            for n in range(1, cls.NTARGETS + 1):
                fname = f"target_{target}_{n}"
                col = Column(
                    fname, UnicodeText,
                    comment=f"Target symptoms: {target} {n}"
                )
                setattr(cls, fname, col)
                cls.TARGET_COLUMNS.append(col)
        for qnumstr, maxscore, comment in cls.QINFO:
            fname = "q" + qnumstr
            setattr(
                cls,
                fname,
                CamcopsColumn(
                    fname, Integer,
                    permitted_value_checker=PermittedValueChecker(
                        minimum=0, maximum=maxscore),
                    comment=f"Q{qnumstr}, {comment} "
                            f"(0-{maxscore}, higher worse)"
                )
            )
        super().__init__(name, bases, classdict)


class Ybocs(TaskHasClinicianMixin, TaskHasPatientMixin, Task,
            metaclass=YbocsMetaclass):
    """
    Server implementation of the Y-BOCS task.
    """
    __tablename__ = "ybocs"
    shortname = "Y-BOCS"
    provides_trackers = True

    NTARGETS = 3
    QINFO = [  # number, max score, minimal comment
        ('1',  4, "obsessions: time"),
        ('1b', 4, "obsessions: obsession-free interval"),
        ('2',  4, "obsessions: interference"),
        ('3',  4, "obsessions: distress"),
        ('4',  4, "obsessions: resistance"),
        ('5',  4, "obsessions: control"),
        ('6',  4, "compulsions: time"),
        ('6b', 4, "compulsions: compulsion-free interval"),
        ('7',  4, "compulsions: interference"),
        ('8',  4, "compulsions: distress"),
        ('9',  4, "compulsions: resistance"),
        ('10', 4, "compulsions: control"),
        ('11', 4, "insight"),
        ('12', 4, "avoidance"),
        ('13', 4, "indecisiveness"),
        ('14', 4, "overvalued responsibility"),
        ('15', 4, "slowness"),
        ('16', 4, "doubting"),
        ('17', 6, "global severity"),
        ('18', 6, "global improvement"),
        ('19', 3, "reliability"),
    ]
    QUESTION_FIELDS = ["q" + x[0] for x in QINFO]
    SCORED_QUESTIONS = strseq("q", 1, 10)
    OBSESSION_QUESTIONS = strseq("q", 1, 5)
    COMPULSION_QUESTIONS = strseq("q", 6, 10)
    MAX_TOTAL = 40
    MAX_OBS = 20
    MAX_COM = 20

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Yale–Brown Obsessive Compulsive Scale")

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [
            TrackerInfo(
                value=self.total_score(),
                plot_label="Y-BOCS total score (lower is better)",
                axis_label=f"Total score (out of {self.MAX_TOTAL})",
                axis_min=-0.5,
                axis_max=self.MAX_TOTAL + 0.5
            ),
            TrackerInfo(
                value=self.obsession_score(),
                plot_label="Y-BOCS obsession score (lower is better)",
                axis_label=f"Total score (out of {self.MAX_OBS})",
                axis_min=-0.5,
                axis_max=self.MAX_OBS + 0.5
            ),
            TrackerInfo(
                value=self.compulsion_score(),
                plot_label="Y-BOCS compulsion score (lower is better)",
                axis_label=f"Total score (out of {self.MAX_COM})",
                axis_min=-0.5,
                axis_max=self.MAX_COM + 0.5
            ),
        ]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total_score",
                coltype=Integer(),
                value=self.total_score(),
                comment=f"Total score (/ {self.MAX_TOTAL})"
            ),
            SummaryElement(
                name="obsession_score",
                coltype=Integer(),
                value=self.obsession_score(),
                comment=f"Obsession score (/ {self.MAX_OBS})"
            ),
            SummaryElement(
                name="compulsion_score",
                coltype=Integer(),
                value=self.compulsion_score(),
                comment=f"Compulsion score (/ {self.MAX_COM})"
            ),
        ]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        t = self.total_score()
        o = self.obsession_score()
        c = self.compulsion_score()
        return [CtvInfo(content=(
            "Y-BOCS total score {t}/{mt} (obsession {o}/{mo}, "
            "compulsion {c}/{mc})".format(
                t=t, mt=self.MAX_TOTAL,
                o=o, mo=self.MAX_OBS,
                c=c, mc=self.MAX_COM,
            )
        ))]

    def total_score(self) -> int:
        return self.sum_fields(self.SCORED_QUESTIONS)

    def obsession_score(self) -> int:
        return self.sum_fields(self.OBSESSION_QUESTIONS)

    def compulsion_score(self) -> int:
        return self.sum_fields(self.COMPULSION_QUESTIONS)

    def is_complete(self) -> bool:
        return (
            self.field_contents_valid() and
            self.all_fields_not_none(self.SCORED_QUESTIONS)
        )

    def get_task_html(self, req: CamcopsRequest) -> str:
        target_symptoms = ""
        for col in self.TARGET_COLUMNS:
            target_symptoms += tr(col.comment, answer(getattr(self, col.name)))
        q_a = ""
        for qi in self.QINFO:
            fieldname = "q" + qi[0]
            value = getattr(self, fieldname)
            q_a += tr(
                self.wxstring(req, fieldname + "_title"),
                answer(self.wxstring(req, fieldname + "_a" + str(value), value)
                       if value is not None else None)
            )
        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    <tr>
                        <td>Total score</td>
                        <td>{answer(self.total_score())} /
                            {self.MAX_TOTAL}</td>
                    </td>
                    <tr>
                        <td>Obsession score</td>
                        <td>{answer(self.obsession_score())} /
                            {self.MAX_OBS}</td>
                    </td>
                    <tr>
                        <td>Compulsion score</td>
                        <td>{answer(self.compulsion_score())} /
                            {self.MAX_COM}</td>
                    </td>
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Target symptom</th>
                    <th width="50%">Detail</th>
                </tr>
                {target_symptoms}
            </table>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer</th>
                </tr>
                {q_a}
            </table>
            {DATA_COLLECTION_UNLESS_UPGRADED_DIV}
        """


# =============================================================================
# Y-BOCS-SC
# =============================================================================

class YbocsScMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['YbocsSc'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        for item in cls.ITEMS:
            setattr(
                cls,
                item + cls.SUFFIX_CURRENT,
                CamcopsColumn(
                    item + cls.SUFFIX_CURRENT, Boolean,
                    permitted_value_checker=BIT_CHECKER,
                    comment=item + " (current symptom)"
                )
            )
            setattr(
                cls,
                item + cls.SUFFIX_PAST,
                CamcopsColumn(
                    item + cls.SUFFIX_PAST, Boolean,
                    permitted_value_checker=BIT_CHECKER,
                    comment=item + " (past symptom)"
                )
            )
            setattr(
                cls,
                item + cls.SUFFIX_PRINCIPAL,
                CamcopsColumn(
                    item + cls.SUFFIX_PRINCIPAL, Boolean,
                    permitted_value_checker=BIT_CHECKER,
                    comment=item + " (principal symptom)"
                )
            )
            if item.endswith(cls.SUFFIX_OTHER):
                setattr(
                    cls,
                    item + cls.SUFFIX_DETAIL,
                    Column(
                        item + cls.SUFFIX_DETAIL, UnicodeText,
                        comment=item + " (details)"
                    )
                )
        super().__init__(name, bases, classdict)


class YbocsSc(TaskHasClinicianMixin, TaskHasPatientMixin, Task,
              metaclass=YbocsScMetaclass):
    """
    Server implementation of the Y-BOCS-SC task.
    """
    __tablename__ = "ybocssc"
    shortname = "Y-BOCS-SC"
    extrastring_taskname = "ybocs"  # shares with Y-BOCS

    SC_PREFIX = "sc_"
    SUFFIX_CURRENT = "_current"
    SUFFIX_PAST = "_past"
    SUFFIX_PRINCIPAL = "_principal"
    SUFFIX_OTHER = "_other"
    SUFFIX_DETAIL = "_detail"
    GROUPS = [
        "obs_aggressive",
        "obs_contamination",
        "obs_sexual",
        "obs_hoarding",
        "obs_religious",
        "obs_symmetry",
        "obs_misc",
        "obs_somatic",
        "com_cleaning",
        "com_checking",
        "com_repeat",
        "com_counting",
        "com_arranging",
        "com_hoarding",
        "com_misc"
    ]
    ITEMS = [
        "obs_aggressive_harm_self",
        "obs_aggressive_harm_others",
        "obs_aggressive_imagery",
        "obs_aggressive_obscenities",
        "obs_aggressive_embarrassing",
        "obs_aggressive_impulses",
        "obs_aggressive_steal",
        "obs_aggressive_accident",
        "obs_aggressive_responsible",
        "obs_aggressive_other",

        "obs_contamination_bodily_waste",
        "obs_contamination_dirt",
        "obs_contamination_environmental",
        "obs_contamination_household",
        "obs_contamination_animals",
        "obs_contamination_sticky",
        "obs_contamination_ill",
        "obs_contamination_others_ill",
        "obs_contamination_feeling",
        "obs_contamination_other",

        "obs_sexual_forbidden",
        "obs_sexual_children_incest",
        "obs_sexual_homosexuality",
        "obs_sexual_to_others",
        "obs_sexual_other",

        "obs_hoarding_other",

        "obs_religious_sacrilege",
        "obs_religious_morality",
        "obs_religious_other",

        "obs_symmetry_with_magical",
        "obs_symmetry_without_magical",

        "obs_misc_know_remember",
        "obs_misc_fear_saying",
        "obs_misc_fear_not_saying",
        "obs_misc_fear_losing",
        "obs_misc_intrusive_nonviolent_images",
        "obs_misc_intrusive_sounds",
        "obs_misc_bothered_sounds",
        "obs_misc_numbers",
        "obs_misc_colours",
        "obs_misc_superstitious",
        "obs_misc_other",

        "obs_somatic_illness",
        "obs_somatic_appearance",
        "obs_somatic_other",

        "com_cleaning_handwashing",
        "com_cleaning_toileting",
        "com_cleaning_cleaning_items",
        "com_cleaning_other_contaminant_avoidance",
        "com_cleaning_other",

        "com_checking_locks_appliances",
        "com_checking_not_harm_others",
        "com_checking_not_harm_self",
        "com_checking_nothing_bad_happens",
        "com_checking_no_mistake",
        "com_checking_somatic",
        "com_checking_other",

        "com_repeat_reread_rewrite",
        "com_repeat_routine",
        "com_repeat_other",

        "com_counting_other",

        "com_arranging_other",

        "com_hoarding_other",

        "com_misc_mental_rituals",
        "com_misc_lists",
        "com_misc_tell_ask",
        "com_misc_touch",
        "com_misc_blink_stare",
        "com_misc_prevent_harm_self",
        "com_misc_prevent_harm_others",
        "com_misc_prevent_terrible",
        "com_misc_eating_ritual",
        "com_misc_superstitious",
        "com_misc_trichotillomania",
        "com_misc_self_harm",
        "com_misc_other"
    ]

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Y-BOCS Symptom Checklist")

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        current_list = []
        past_list = []
        principal_list = []
        for item in self.ITEMS:
            if getattr(self, item + self.SUFFIX_CURRENT):
                current_list.append(item)
            if getattr(self, item + self.SUFFIX_PAST):
                past_list.append(item)
            if getattr(self, item + self.SUFFIX_PRINCIPAL):
                principal_list.append(item)
        return [
            CtvInfo(content=f"Current symptoms: {', '.join(current_list)}"),
            CtvInfo(content=f"Past symptoms: {', '.join(past_list)}"),
            CtvInfo(content=f"Principal symptoms: {', '.join(principal_list)}"),  # noqa
        ]

    # noinspection PyMethodOverriding
    @staticmethod
    def is_complete() -> bool:
        return True

    def get_task_html(self, req: CamcopsRequest) -> str:
        h = f"""
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="55%">Symptom</th>
                    <th width="15%">Current</th>
                    <th width="15%">Past</th>
                    <th width="15%">Principal</th>
                </tr>
        """
        for group in self.GROUPS:
            h += subheading_spanning_four_columns(
                self.wxstring(req, self.SC_PREFIX + group))
            for item in self.ITEMS:
                if not item.startswith(group):
                    continue
                h += tr(
                    self.wxstring(req, self.SC_PREFIX + item),
                    answer(get_ternary(getattr(self,
                                               item + self.SUFFIX_CURRENT),
                                       value_true="Current",
                                       value_false="",
                                       value_none="")),
                    answer(get_ternary(getattr(self,
                                               item + self.SUFFIX_PAST),
                                       value_true="Past",
                                       value_false="",
                                       value_none="")),
                    answer(get_ternary(getattr(self,
                                               item + self.SUFFIX_PRINCIPAL),
                                       value_true="Principal",
                                       value_false="",
                                       value_none="")),
                )
                if item.endswith(self.SUFFIX_OTHER):
                    h += f"""
                        <tr>
                            <td><i>Specify:</i></td>
                            <td colspan="3">{
                    answer(getattr(self, item + self.SUFFIX_DETAIL), "")}</td>
                        </tr>
                    """
        h += f"""
            </table>
            {DATA_COLLECTION_UNLESS_UPGRADED_DIV}
        """
        return h
