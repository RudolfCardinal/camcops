#!/usr/bin/env python

"""
camcops_server/tasks/kirby.py

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
import math
from typing import Dict, List, Optional, Type

import numpy as np
from numpy.linalg.linalg import LinAlgError
from scipy.stats.mstats import gmean
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Float, Integer
import statsmodels.api as sm
# noinspection PyProtectedMember
from statsmodels.discrete.discrete_model import BinaryResultsWrapper
from statsmodels.tools.sm_exceptions import PerfectSeparationError

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_db import (
    ancillary_relationship,
    GenericTabletRecordMixin,
    TaskDescendant,
)
from camcops_server.cc_modules.cc_html import (
    answer,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqlalchemy import Base
from camcops_server.cc_modules.cc_sqla_coltypes import (
    BoolColumn,
    CurrencyColType,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import Task, TaskHasPatientMixin

log = logging.getLogger(__name__)


# =============================================================================
# KirbyRewardPair
# =============================================================================

class KirbyRewardPair(object):
    """
    Represents a pair of rewards: a small immediate reward (SIR) and a large
    delayed reward (LDR).
    """
    def __init__(self,
                 sir: int,
                 ldr: int,
                 delay_days: int,
                 chose_ldr: bool = None,
                 currency: str = "£",
                 currency_symbol_first: bool = True) -> None:
        """
        Args:
            sir: amount of the small immediate reward (SIR)
            ldr: amount of the large delayed reward (LDR)
            delay_days: delay to the LDR, in days
            chose_ldr: if result also represented, did the subject choose the
                LDR?
            currency: currency symbol
            currency_symbol_first: symbol before amount?
        """
        self.sir = sir
        self.ldr = ldr
        self.delay_days = delay_days
        self.chose_ldr = chose_ldr
        self.currency = currency
        self.currency_symbol_first = currency_symbol_first

    def money(self, amount: int) -> str:
        """
        Returns a currency amount, formatted.
        """
        if self.currency_symbol_first:
            return f"{self.currency}{amount}"
        return f"{amount}{self.currency}"

    def sir_string(self, req: CamcopsRequest) -> str:
        """
        Returns a string representing the small immediate reward, e.g.
        "£10 today".
        """
        _ = req.gettext
        return _("{money} today").format(money=self.money(self.sir))

    def ldr_string(self, req: CamcopsRequest) -> str:
        """
        Returns a string representing the large delayed reward, e.g.
        "£50 in 200 days".
        """
        _ = req.gettext
        return _("{money} in {days} days").format(
            money=self.money(self.ldr),
            days=self.delay_days,
        )

    def question(self, req: CamcopsRequest) -> str:
        """
        The question posed for this reward pair.
        """
        _ = req.gettext
        return _("Would you prefer {sir}, or {ldr}?").format(
            sir=self.sir_string(req),
            ldr=self.ldr_string(req),
        )

    def answer(self, req: CamcopsRequest) -> str:
        """
        Returns the subject's answer, or "?".
        """
        if self.chose_ldr is None:
            return "?"
        return self.ldr_string(req) if self.chose_ldr else self.sir_string(req)

    def k_indifference(self) -> float:
        """
        Returns the value of k, the discounting parameter (units: days ^ -1)
        if the subject is indifferent between the two choices.

        For calculations see :ref:`kirby_mcq.rst <kirby_mcq>`.
        """
        a1 = self.sir
        a2 = self.ldr
        d2 = self.delay_days
        return (a2 - a1) / (a1 * d2)

    def choice_consistent(self, k: float) -> bool:
        """
        Was the choice consistent with the k value given?

        - If no choice has been recorded, returns false.

        - If the k value equals the implied indifference point exactly (meaning
          that the subject should not care), return true.
        """
        if self.chose_ldr is None:
            return False
        k_indiff = self.k_indifference()
        if math.isclose(k, k_indiff):
            # Subject is indifferent
            return True
        # WARNING: "self.chose_ldr == k < k_indiff" FAILS.
        # Python will evaluate this to "(self.chose_ldr == k) < k_indiff", and
        # despite that evaluating to "a bool < an int", that's legal; e.g.
        # "False < 4" evaluates to True.
        # Must be bracketed like this:
        return self.chose_ldr == (k < k_indiff)


# =============================================================================
# KirbyTrial
# =============================================================================

class KirbyTrial(GenericTabletRecordMixin, TaskDescendant, Base):
    __tablename__ = "kirby_mcq_trials"

    kirby_mcq_id = Column(
        "kirby_mcq_id", Integer,
        nullable=False,
        comment="FK to kirby_mcq"
    )
    trial = Column(
        "trial", Integer,
        nullable=False,
        comment="Trial number (1-based)"
    )
    sir = Column(
        "sir", Integer,
        comment="Small immediate reward"
    )
    ldr = Column(
        "ldr", Integer,
        comment="Large delayed reward"
    )
    delay_days = Column(
        "delay_days", Integer,
        comment="Delay in days"
    )
    currency = Column(
        "currency", CurrencyColType,
        comment="Currency symbol"
    )
    currency_symbol_first = BoolColumn(
        "currency_symbol_first",
        comment="Does the currency symbol come before the amount?"
    )
    chose_ldr = BoolColumn(
        "chose_ldr",
        comment="Did the subject choose the large delayed reward?"
    )

    def info(self) -> KirbyRewardPair:
        """
        Returns the trial information as a :class:`KirbyRewardPair`.
        """
        return KirbyRewardPair(
            sir=self.sir,
            ldr=self.ldr,
            delay_days=self.delay_days,
            chose_ldr=self.chose_ldr,
            currency=self.currency,
            currency_symbol_first=self.currency_symbol_first,
        )

    def answered(self) -> bool:
        """
        Has the subject answered this question?
        """
        return self.chose_ldr is not None

    # -------------------------------------------------------------------------
    # TaskDescendant overrides
    # -------------------------------------------------------------------------

    @classmethod
    def task_ancestor_class(cls) -> Optional[Type["Task"]]:
        return Kirby

    def task_ancestor(self) -> Optional["Kirby"]:
        return Kirby.get_linked(self.kirby_mcq_id, self)


# =============================================================================
# Kirby
# =============================================================================

class Kirby(TaskHasPatientMixin, Task):
    """
    Server implementation of the Kirby Monetary Choice Questionnaire task.
    """
    __tablename__ = "kirby_mcq"
    shortname = "KirbyMCQ"

    EXPECTED_N_TRIALS = 27

    # No fields beyond the basics.

    # Relationships
    trials = ancillary_relationship(
        parent_class_name="Kirby",
        ancillary_class_name="KirbyTrial",
        ancillary_fk_to_parent_attr_name="kirby_mcq_id",
        ancillary_order_by_attr_name="trial"
    )  # type: List[KirbyTrial]

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Kirby et al. 1999 Monetary Choice Questionnaire")

    def is_complete(self) -> bool:
        if len(self.trials) != self.EXPECTED_N_TRIALS:
            return False
        for t in self.trials:
            if not t.answered():
                return False
        return True

    def all_choice_results(self) -> List[KirbyRewardPair]:
        """
        Returns a list of :class:`KirbyRewardPair` objects, one for each
        answered question.
        """
        results = []  # type: List[KirbyRewardPair]
        for t in self.trials:
            if t.answered():
                results.append(t.info())
        return results

    @staticmethod
    def n_choices_consistent(k: float, results: List[KirbyRewardPair]) -> int:
        """
        Returns the number of choices that are consistent with the given k
        value.
        """
        n_consistent = 0
        for pair in results:
            if pair.choice_consistent(k):
                n_consistent += 1
        return n_consistent

    def k_kirby(self, results: List[KirbyRewardPair]) -> Optional[float]:
        """
        Returns k for a subject as determined using Kirby's (2000) method.
        See :ref:`kirby_mcq.rst <kirby_mcq>`.
        """
        # 1. For every k value assessed by the questions, establish the degree
        #    of consistency.
        consistency = {}  # type: Dict[float, int]
        for pair in results:
            k = pair.k_indifference()
            if k not in consistency:
                consistency[k] = self.n_choices_consistent(k, results)
        if not consistency:
            return None

        # 2. Restrict to the results that are equally and maximally consistent.
        max_consistency = max(consistency.values())
        good_k_values = [k for k, v in consistency.items()
                         if v == max_consistency]

        # 3. Take the geometric mean of those good k values.
        # noinspection PyTypeChecker
        subject_k = gmean(good_k_values)  # type: np.float64

        return float(subject_k)

    @staticmethod
    def k_wileyto(results: List[KirbyRewardPair]) -> Optional[float]:
        """
        Returns k for a subject as determined using Wileyto et al.'s (2004)
        method. See :ref:`kirby_mcq.rst <kirby_mcq>`.
        """
        if not results:
            return None
        n_predictors = 2
        n_observations = len(results)
        x = np.zeros((n_observations, n_predictors))
        y = np.zeros(n_observations)
        for i in range(n_observations):
            pair = results[i]
            a1 = pair.sir
            a2 = pair.ldr
            d2 = pair.delay_days
            predictor1 = 1 - (a2 / a1)
            predictor2 = d2
            x[i, 0] = predictor1
            x[i, 1] = predictor2
            y[i] = int(pair.chose_ldr)  # bool to int
        lr = sm.Logit(y, x)
        try:
            result = lr.fit()  # type: BinaryResultsWrapper
        except (LinAlgError,  # e.g. "singular matrix"
                PerfectSeparationError) as e:
            log.debug(f"sm.Logit error: {e}")
            return None
        coeffs = result.params
        beta1 = coeffs[0]
        beta2 = coeffs[1]
        try:
            k = beta2 / beta1
        except ZeroDivisionError:
            log.warning("Division by zero when calculating k = beta2 / beta1")
            return None
        return k

    # noinspection PyUnusedLocal
    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        results = self.all_choice_results()
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="k_kirby", coltype=Float(),
                value=self.k_kirby(results),
                comment="k (days^-1, Kirby 2000 method)"),
            SummaryElement(
                name="k_wileyto", coltype=Float(),
                value=self.k_wileyto(results),
                comment="k (days^-1, Wileyto 2004 method)"),
        ]

    def get_task_html(self, req: CamcopsRequest) -> str:
        dp = 6
        qlines = []  # type: List[str]
        for t in self.trials:
            info = t.info()
            qlines.append(
                tr_qa(f"{t.trial}. {info.question(req)} "
                      f"<i>(k<sub>indiff</sub> = "
                      f"{round(info.k_indifference(), dp)})</i>",
                      info.answer(req))
            )
        q_a = "\n".join(qlines)
        results = self.all_choice_results()
        k_kirby = self.k_kirby(results)
        if k_kirby is None:
            inv_k_kirby = None
        else:
            inv_k_kirby = int(round(1 / k_kirby))  # round to int
            # ... you'd think the int() was unnecessary but it is needed
            k_kirby = round(k_kirby, dp)
        k_wileyto = self.k_wileyto(results)
        if k_wileyto is None:
            inv_k_wileyto = None
        else:
            inv_k_wileyto = int(round(1 / k_wileyto))  # round to int
            k_wileyto = round(k_wileyto, dp)
        return f"""
          <div class="{CssClass.SUMMARY}">
            <table class="{CssClass.SUMMARY}">
              {self.get_is_complete_tr(req)}
              <tr>
                <td><i>k</i> (days<sup>–1</sup>, Kirby 2000 method)</td>
                <td>{answer(k_kirby)}
              </tr>
              <tr>
                <td>1/<i>k</i> (days, Kirby method): time to half value</td>
                <td>{answer(inv_k_kirby)}
              </tr>
              <tr>
                <td><i>k</i> (days<sup>–1</sup>, Wileyto et al. 2004 method)</td>
                <td>{answer(k_wileyto)}
              </tr>
              <tr>
                <td>1/<i>k</i> (days, Wileyto method): time to half value</td>
                <td>{answer(inv_k_wileyto)}
              </tr>
            </table>
          </div>
          <table class="{CssClass.TASKDETAIL}">
            <tr>
              <th width="75%">Question</th>
              <th width="25%">Answer</th>
            </tr>
            {q_a}
          </table>
        """  # noqa
