#!/usr/bin/env python

"""
camcops_server/tasks/hads.py

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

from abc import ABC, ABCMeta
import logging
from typing import Any, Dict, List, Tuple, Type

from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.stringfunc import strseq
from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.sqltypes import Integer

from camcops_server.cc_modules.cc_constants import (
    CssClass,
    DATA_COLLECTION_UNLESS_UPGRADED_DIV,
)
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import add_multiple_columns
from camcops_server.cc_modules.cc_html import answer, tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_string import AS
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasPatientMixin,
    TaskHasRespondentMixin,
)
from camcops_server.cc_modules.cc_trackerhelpers import TrackerInfo

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# HADS (crippled unless upgraded locally) - base classes
# =============================================================================

class HadsMetaclass(DeclarativeMeta, ABCMeta):
    """
    We can't make this metaclass inherit from DeclarativeMeta.

    This works:

    .. :code-block:: python

        class MyTaskMetaclass(DeclarativeMeta):
            def __init__(cls, name, bases, classdict):
                # do useful stuff
                super().__init__(name, bases, classdict)

        class MyTask(Task, Base, metaclass=MyTaskMetaclass):
            __tablename__ = "mytask"

    ... but at the point that MyTaskMetaclass calls DeclarativeMeta.__init__,
    it registers "cls" (in this case MyTask) with the SQLAlchemy class
    registry. In this example, that's fine, because MyTask wants to be
    registered. But here it fails:

    .. :code-block:: python

        class OtherTaskMetaclass(DeclarativeMeta):
            def __init__(cls, name, bases, classdict):
                # do useful stuff
                super().__init__(name, bases, classdict)

        class Intermediate(Task, metaclass=OtherTaskMetaclass): pass

        class OtherTask(Intermediate, Base):
            __tablename__ = "othertask"

    ... and it fails because OtherTaskMetaclass calls DeclarativeMeta.__init__
    and this tries to register "Intermediate" with the SQLALchemy ORM.

    So, it's clear that OtherTaskMetaclass shouldn't derive from
    DeclarativeMeta. But if we make it derive from "object" instead, we get
    the error

    .. :code-block:: none

        TypeError: metaclass conflict: the metaclass of a derived class must
        be a (non-strict) subclass of the metaclasses of all its bases

    because OtherTask inherits from Base, whose metaclass is DeclarativeMeta,
    but there is another metaclass in the metaclass set that is incompatible
    with this.

    So, is solution that OtherTaskMetaclass should derive from "type" and then
    to use CooperativeMeta (q.v.) for OtherTask?

    No, that still seems to fail (and before any CooperativeMeta code is
    called) -- possibly that framework is for Python 2 only.

    See also
    https://blog.ionelmc.ro/2015/02/09/understanding-python-metaclasses/

    Alternative solution 1: make a new metaclass that pretends to inherit
    from HadsMetaclass and DeclarativeMeta.

    WENT WITH THIS ONE INITIALLY:

    .. :code-block:: python

        class HadsMetaclass(type):                      # METACLASS
            def __init__(cls: Type['HadsBase'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
            add_multiple_columns(...)

        class HadsBase(TaskHasPatientMixin, Task,       # INTERMEDIATE
                       metaclass=HadsMetaclass):
            ...

        class HadsBlendedMetaclass(HadsMetaclass, DeclarativeMeta):    # ODDITY
            # noinspection PyInitNewSignature
            def __init__(cls: Type[Union[HadsBase, DeclarativeMeta]],
                         name: str,
                         bases: Tuple[Type, ...],
                         classdict: Dict[str, Any]) -> None:
                HadsMetaclass.__init__(cls, name, bases, classdict)
                # ... will call DeclarativeMeta.__init__ via its
                #     super().__init__()

        class Hads(HadsBase,                            # ACTUAL TASK
                   metaclass=HadsBlendedMetaclass):
            __tablename__ = "hads"

    Alternative solution 2: continue to have the HadsMetaclass deriving from
    DeclarativeMeta, but add it in at the last stage.

    IGNORE THIS, NO LONGER TRUE:

    - ALL THIS SOMEWHAT REVISED to handle SQLAlchemy concrete inheritance
      (q.v.), with the rule that "the only things that inherit from Task are
      actual tasks"; Task then inherits from both AbstractConcreteBase and
      Base.

    SEE ALSO sqla_database_structure.txt

    FINAL ANSWER:

    - classes inherit in a neat chain from Base -> [+/- Task -> ...]
    - metaclasses inherit in a neat chain from DeclarativeMeta
    - abstract intermediates mark themselves with "__abstract__ = True"

    .. :code-block:: python

        class HadsMetaclass(DeclarativeMeta):           # METACLASS
            def __init__(cls: Type['HadsBase'],
                         name: str,
                         bases: Tuple[Type, ...],
                         classdict: Dict[str, Any]) -> None:
                add_multiple_columns(...)

        class HadsBase(TaskHasPatientMixin, Task,       # INTERMEDIATE
                       metaclass=HadsMetaclass):
            __abstract__ = True

        class Hads(HadsBase):
            __tablename__ = "hads"

    Yes, that's it. (Note that if you erroneously also add
    "metaclass=HadsMetaclass" on Hads, you get: "TypeError: metaclass conflict:
    the metaclass of a derived class must be a (non-strict) subclass of the
    metaclasses of all its bases.")

    UPDATE 2019-07-28:

    - To fix "class must implement all abstract methods" warning from PyCharm,
      add "ABCMeta" to superclass list of HadsMetaclass.

    """
    # noinspection PyInitNewSignature
    def __init__(cls: Type['HadsBase'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        add_multiple_columns(
            cls, "q", 1, cls.NQUESTIONS,
            minimum=0, maximum=3,
            comment_fmt="Q{n}: {s} (0-3)",
            comment_strings=[
                "tense", "enjoy usual", "apprehensive", "laugh", "worry",
                "cheerful", "relaxed", "slow", "butterflies", "appearance",
                "restless", "anticipate", "panic", "book/TV/radio"
            ]
        )
        super().__init__(name, bases, classdict)


class HadsBase(TaskHasPatientMixin, Task, ABC,
               metaclass=HadsMetaclass):
    """
    Server implementation of the HADS task.
    """
    __abstract__ = True
    provides_trackers = True

    NQUESTIONS = 14
    ANXIETY_QUESTIONS = [1, 3, 5, 7, 9, 11, 13]
    DEPRESSION_QUESTIONS = [2, 4, 6, 8, 10, 12, 14]
    TASK_FIELDS = strseq("q", 1, NQUESTIONS)
    MAX_ANX_SCORE = 21
    MAX_DEP_SCORE = 21

    def is_complete(self) -> bool:
        return (
            self.field_contents_valid() and
            self.all_fields_not_none(self.TASK_FIELDS)
        )

    def get_trackers(self, req: CamcopsRequest) -> List[TrackerInfo]:
        return [
            TrackerInfo(
                value=self.anxiety_score(),
                plot_label="HADS anxiety score",
                axis_label=f"Anxiety score (out of {self.MAX_ANX_SCORE})",
                axis_min=-0.5,
                axis_max=self.MAX_ANX_SCORE + 0.5,
            ),
            TrackerInfo(
                value=self.depression_score(),
                plot_label="HADS depression score",
                axis_label=f"Depression score (out of {self.MAX_DEP_SCORE})",
                axis_min=-0.5,
                axis_max=self.MAX_DEP_SCORE + 0.5
            ),
        ]

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(content=(
            f"anxiety score {self.anxiety_score()}/{self.MAX_ANX_SCORE}, "
            f"depression score {self.depression_score()}/{self.MAX_DEP_SCORE}"
        ))]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="anxiety", coltype=Integer(),
                value=self.anxiety_score(),
                comment=f"Anxiety score (/{self.MAX_ANX_SCORE})"),
            SummaryElement(
                name="depression", coltype=Integer(),
                value=self.depression_score(),
                comment=f"Depression score (/{self.MAX_DEP_SCORE})"),
        ]

    def score(self, questions: List[int]) -> int:
        fields = self.fieldnames_from_list("q", questions)
        return self.sum_fields(fields)

    def anxiety_score(self) -> int:
        return self.score(self.ANXIETY_QUESTIONS)

    def depression_score(self) -> int:
        return self.score(self.DEPRESSION_QUESTIONS)

    def get_task_html(self, req: CamcopsRequest) -> str:
        min_score = 0
        max_score = 3
        crippled = not self.extrastrings_exist(req)
        a = self.anxiety_score()
        d = self.depression_score()
        h = f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    <tr>
                        <td>{req.wappstring(AS.HADS_ANXIETY_SCORE)}</td>
                        <td>{answer(a)} / {self.MAX_ANX_SCORE}</td>
                    </tr>
                    <tr>
                        <td>{req.wappstring(AS.HADS_DEPRESSION_SCORE)}</td>
                        <td>{answer(d)} / 21</td>
                    </tr>
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                All questions are scored from 0–3
                (0 least symptomatic, 3 most symptomatic).
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer</th>
                </tr>
        """
        for n in range(1, self.NQUESTIONS + 1):
            if crippled:
                q = f"HADS: Q{n}"
            else:
                q = f"Q{n}. {self.wxstring(req, f'q{n}_stem')}"
            if n in self.ANXIETY_QUESTIONS:
                q += " (A)"
            if n in self.DEPRESSION_QUESTIONS:
                q += " (D)"
            v = getattr(self, "q" + str(n))
            if crippled or v is None or v < min_score or v > max_score:
                a = v
            else:
                a = f"{v}: {self.wxstring(req, f'q{n}_a{v}')}"
            h += tr_qa(q, a)
        h += """
            </table>
        """ + DATA_COLLECTION_UNLESS_UPGRADED_DIV
        return h


# =============================================================================
# Hads
# =============================================================================

class Hads(HadsBase):
    __tablename__ = "hads"
    shortname = "HADS"

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _(
            "Hospital Anxiety and Depression Scale (data collection only)")

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.HADS_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.HADS_SCALE),
                {
                    req.snomed(SnomedLookup.HADS_ANXIETY_SCORE): self.anxiety_score(),  # noqa
                    req.snomed(SnomedLookup.HADS_DEPRESSION_SCORE): self.depression_score(),  # noqa
                }
            ))
        return codes


# =============================================================================
# HadsRespondent
# =============================================================================

class HadsRespondent(TaskHasRespondentMixin, HadsBase):
    __tablename__ = "hads_respondent"
    shortname = "HADS-Respondent"
    extrastring_taskname = "hads"

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Hospital Anxiety and Depression Scale (data collection "
                 "only), non-patient respondent version")

    # No SNOMED codes; not for the patient!
