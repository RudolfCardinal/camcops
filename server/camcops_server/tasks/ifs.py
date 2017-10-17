#!/usr/bin/env python
# camcops_server/tasks/ifs.py

"""
===============================================================================
    Copyright (C) 2012-2017 Rudolf Cardinal (rudolf@pobox.com).

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

from typing import Any, Dict, List, Tuple, Type

from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.sqltypes import Boolean, Float, Integer

from ..cc_modules.cc_constants import (
    DATA_COLLECTION_UNLESS_UPGRADED_DIV,
    INVALID_VALUE,
)
from ..cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from ..cc_modules.cc_html import (
    answer,
    get_correct_incorrect_none,
    td,
    tr,
    tr_qa,
)
from ..cc_modules.cc_request import CamcopsRequest
from ..cc_modules.cc_sqla_coltypes import (
    BIT_CHECKER,
    CamcopsColumn,
    ZERO_TO_ONE_CHECKER,
    ZERO_TO_TWO_CHECKER,
    ZERO_TO_THREE_CHECKER,
)
from ..cc_modules.cc_summaryelement import SummaryElement
from ..cc_modules.cc_task import (
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)
from ..cc_modules.cc_trackerhelpers import TrackerInfo


# =============================================================================
# IFS
# =============================================================================

class IfsMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Ifs'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        for seqlen in cls.Q4_DIGIT_LENGTHS:
            fname1 = "q4_len{}_1".format(seqlen)
            fname2 = "q4_len{}_2".format(seqlen)
            setattr(
                cls,
                fname1,
                CamcopsColumn(
                    fname1, Boolean,
                    permitted_value_checker=BIT_CHECKER,
                    comment="Q4. Digits backward, length {}, trial 1".format(
                        seqlen)
                )
            )
            setattr(
                cls,
                fname2,
                CamcopsColumn(
                    fname2, Boolean,
                    permitted_value_checker=BIT_CHECKER,
                    comment="Q4. Digits backward, length {}, trial 2".format(
                        seqlen)
                )
            )
        for n in cls.Q6_SEQUENCE_NUMS:
            fname = "q6_seq{}".format(n)
            setattr(
                cls,
                fname,
                CamcopsColumn(
                    fname, Integer,
                    permitted_value_checker=BIT_CHECKER,
                    comment="Q6. Spatial working memory, sequence {}".format(n)
                )
            )
        for n in cls.Q7_PROVERB_NUMS:
            fname = "q7_proverb{}".format(n)
            setattr(
                cls,
                fname,
                CamcopsColumn(
                    fname, Float,
                    permitted_value_checker=ZERO_TO_ONE_CHECKER,
                    comment="Q7. Proverb {} (1 = correct explanation, "
                            "0.5 = example, 0 = neither)".format(n)
                )
            )
        for n in cls.Q8_SENTENCE_NUMS:
            fname = "q8_sentence{}".format(n)
            setattr(
                cls,
                fname,
                CamcopsColumn(
                    fname, Integer,
                    permitted_value_checker=ZERO_TO_TWO_CHECKER,
                    comment="Q8. Hayling, sentence {}".format(n)
                )
            )
        super().__init__(name, bases, classdict)


class Ifs(TaskHasPatientMixin, TaskHasClinicianMixin, Task,
          metaclass=IfsMetaclass):
    __tablename__ = "ifs"
    shortname = "IFS"
    longname = "INECO Frontal Screening"
    provides_trackers = True

    q1 = CamcopsColumn(
        "q1", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment="Q1. Motor series (motor programming)"
    )
    q2 = CamcopsColumn(
        "q2", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment="Q2. Conflicting instructions (interference sensitivity)"
    )
    q3 = CamcopsColumn(
        "q3", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment="Q3. Go/no-go (inhibitory control)"
    )
    q5 = CamcopsColumn(
        "q5", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Q5. Verbal working memory"
    )

    Q4_DIGIT_LENGTHS = list(range(2, 7 + 1))
    Q6_SEQUENCE_NUMS = list(range(1, 4 + 1))
    Q7_PROVERB_NUMS = list(range(1, 3 + 1))
    Q8_SENTENCE_NUMS = list(range(1, 3 + 1))
    SIMPLE_Q = (
        ["q1", "q2", "q3", "q5"] +
        ["q6_seq{}".format(n) for n in Q6_SEQUENCE_NUMS] +
        ["q7_proverb{}".format(n) for n in Q7_PROVERB_NUMS] +
        ["q8_sentence{}".format(n) for n in Q8_SENTENCE_NUMS]
    )
    MAX_TOTAL = 30
    MAX_WM = 10

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        scoredict = self.get_score()
        return [
            TrackerInfo(
                value=scoredict['total'],
                plot_label="IFS total score (higher is better)",
                axis_label="Total score (out of {})".format(self.MAX_TOTAL),
                axis_min=-0.5,
                axis_max=self.MAX_TOTAL + 0.5
            ),
            TrackerInfo(
                value=scoredict['wm'],
                plot_label="IFS working memory index (higher is better)",
                axis_label="Total score (out of {})".format(self.MAX_WM),
                axis_min=-0.5,
                axis_max=self.MAX_WM + 0.5
            ),
        ]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        scoredict = self.get_score()
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="total",
                coltype=Float(),
                value=scoredict['total'],
                comment="Total (out of {}, higher better)".format(
                    self.MAX_TOTAL)),
            SummaryElement(
                name="wm",
                coltype=Integer(),
                value=scoredict['wm'],
                comment="Working memory index (out of {}; "
                        "sum of Q4 + Q6".format(self.MAX_WM)),
        ]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        scoredict = self.get_score()
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=(
                "Total: {t}/{tmax}; working memory index {w}/{wmax}".format(
                    t=scoredict['total'],
                    tmax=self.MAX_TOTAL,
                    w=scoredict['wm'],
                    wmax=self.MAX_WM,
                )
            )
        )]

    def get_score(self) -> Dict:
        q1 = getattr(self, "q1", 0) or 0
        q2 = getattr(self, "q2", 0) or 0
        q3 = getattr(self, "q3", 0) or 0
        q4 = 0
        for seqlen in self.Q4_DIGIT_LENGTHS:
            val1 = getattr(self, "q4_len{}_1".format(seqlen))
            val2 = getattr(self, "q4_len{}_2".format(seqlen))
            if val1 or val2:
                q4 += 1
            if not val1 and not val2:
                break
        q5 = getattr(self, "q5", 0) or 0
        q6 = self.sum_fields(["q6_seq" + str(s) for s in range(1, 4 + 1)])
        q7 = self.sum_fields(["q7_proverb" + str(s) for s in range(1, 3 + 1)])
        q8 = self.sum_fields(["q8_sentence" + str(s) for s in range(1, 3 + 1)])
        total = q1 + q2 + q3 + q4 + q5 + q6 + q7 + q8
        wm = q4 + q6  # working memory index (though not verbal)
        return dict(
            total=total,
            wm=wm
        )

    def is_complete(self) -> bool:
        if not self.field_contents_valid():
            return False
        if not self.are_all_fields_complete(self.SIMPLE_Q):
            return False
        for seqlen in self.Q4_DIGIT_LENGTHS:
            val1 = getattr(self, "q4_len{}_1".format(seqlen))
            val2 = getattr(self, "q4_len{}_2".format(seqlen))
            if val1 is None or val2 is None:
                return False
            if not val1 and not val2:
                return True  # all done
        return True

    def get_simple_tr_qa(self, req: CamcopsRequest, qprefix: str) -> str:
        q = self.wxstring(req, qprefix + "_title")
        val = getattr(self, qprefix)
        if val is not None:
            a = self.wxstring(req, qprefix + "_a" + str(val))
        else:
            a = None
        return tr_qa(q, a)

    def get_task_html(self, req: CamcopsRequest) -> str:
        scoredict = self.get_score()
        h = """
            <div class="summary">
                <table class="summary">
                    {complete_tr}
                    <tr>
                        <td>Total (higher better)</td>
                        <td>{total} / {tmax}</td>
                    </td>
                    <tr>
                        <td>Working memory index <sup>1</sup></td>
                        <td>{wm} / {wmax}</td>
                    </td>
                </table>
            </div>
            <table class="taskdetail">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer</th>
                </tr>
        """.format(
            complete_tr=self.get_is_complete_tr(req),
            total=answer(scoredict['total']),
            tmax=self.MAX_TOTAL,
            wm=answer(scoredict['wm']),
            wmax=self.MAX_WM,
        )

        # Q1
        h += self.get_simple_tr_qa(req, "q1")
        # Q2
        h += self.get_simple_tr_qa(req, "q2")
        # Q3
        h += self.get_simple_tr_qa(req, "q3")
        # Q4
        h += tr(td(self.wxstring(req, "q4_title")),
                td("", td_class="subheading"),
                literal=True)
        required = True
        for n in self.Q4_DIGIT_LENGTHS:
            val1 = getattr(self, "q4_len{}_1".format(n))
            val2 = getattr(self, "q4_len{}_2".format(n))
            q = (
                "… " +
                self.wxstring(req, "q4_seq_len{}_1".format(n)) +
                " / " + self.wxstring(req, "q4_seq_len{}_2".format(n))
            )
            if required:
                score = 1 if val1 or val2 else 0
                a = (
                    answer(get_correct_incorrect_none(val1)) +
                    " / " + answer(get_correct_incorrect_none(val2)) +
                    " (scores {})".format(score)
                )
            else:
                a = ""
            h += tr(q, a)
            if not val1 and not val2:
                required = False
        # Q5
        h += self.get_simple_tr_qa(req, "q5")
        # Q6
        h += tr(td(self.wxstring(req, "q6_title")),
                td("", td_class="subheading"),
                literal=True)
        for n in self.Q6_SEQUENCE_NUMS:
            nstr = str(n)
            val = getattr(self, "q6_seq" + nstr)
            h += tr_qa("… " + self.wxstring(req, "q6_seq" + nstr), val)
        # Q7
        q7map = {
            None: None,
            1: self.wxstring(req, "q7_a_1"),
            0.5: self.wxstring(req, "q7_a_half"),
            0: self.wxstring(req, "q7_a_0"),
        }
        h += tr(td(self.wxstring(req, "q7_title")),
                td("", td_class="subheading"),
                literal=True)
        for n in self.Q7_PROVERB_NUMS:
            nstr = str(n)
            val = getattr(self, "q7_proverb" + nstr)
            a = q7map.get(val, INVALID_VALUE)
            h += tr_qa("… " + self.wxstring(req, "q7_proverb" + nstr), a)
        # Q8
        q8map = {
            None: None,
            2: self.wxstring(req, "q8_a2"),
            1: self.wxstring(req, "q8_a1"),
            0: self.wxstring(req, "q8_a0"),
        }
        h += tr(td(self.wxstring(req, "q8_title")),
                td("", td_class="subheading"),
                literal=True)
        for n in self.Q8_SENTENCE_NUMS:
            nstr = str(n)
            val = getattr(self, "q8_sentence" + nstr)
            a = q8map.get(val, INVALID_VALUE)
            h += tr_qa("… " + self.wxstring(req, "q8_sentence_" + nstr), a)

        h += """
            </table>
            <div class="footnotes">
                [1] Sum of scores for Q4 + Q6.
            </div>
        """ + DATA_COLLECTION_UNLESS_UPGRADED_DIV
        return h
