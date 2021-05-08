#!/usr/bin/env python

"""
camcops_server/tasks/nart.py

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

import math
from typing import Any, Dict, List, Optional, Tuple, Type

from sqlalchemy.ext.declarative import DeclarativeMeta
from sqlalchemy.sql.sqltypes import Boolean, Float

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_html import answer, td, tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_sqla_coltypes import (
    BIT_CHECKER,
    CamcopsColumn,
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)


WORDLIST = [  # Value is true/1 for CORRECT, false/0 for INCORRECT
    "chord",
    "ache",
    "depot",
    "aisle",
    "bouquet",
    "psalm",
    "capon",
    "deny",  # NB reserved word in SQL (auto-handled)
    "nausea",
    "debt",
    "courteous",
    "rarefy",
    "equivocal",
    "naive",  # accent required
    "catacomb",
    "gaoled",
    "thyme",
    "heir",
    "radix",
    "assignate",
    "hiatus",
    "subtle",
    "procreate",
    "gist",
    "gouge",
    "superfluous",
    "simile",
    "banal",
    "quadruped",
    "cellist",
    "facade",  # accent required
    "zealot",
    "drachm",
    "aeon",
    "placebo",
    "abstemious",
    "detente",  # accent required
    "idyll",
    "puerperal",
    "aver",
    "gauche",
    "topiary",
    "leviathan",
    "beatify",
    "prelate",
    "sidereal",
    "demesne",
    "syncope",
    "labile",
    "campanile"
]
ACCENTED_WORDLIST = list(WORDLIST)
# noinspection PyUnresolvedReferences
ACCENTED_WORDLIST[ACCENTED_WORDLIST.index("naive")] = "naïve"
ACCENTED_WORDLIST[ACCENTED_WORDLIST.index("facade")] = "façade"
ACCENTED_WORDLIST[ACCENTED_WORDLIST.index("detente")] = "détente"


# =============================================================================
# NART
# =============================================================================

class NartMetaclass(DeclarativeMeta):
    # noinspection PyInitNewSignature
    def __init__(cls: Type['Nart'],
                 name: str,
                 bases: Tuple[Type, ...],
                 classdict: Dict[str, Any]) -> None:
        for w in WORDLIST:
            setattr(
                cls,
                w,
                CamcopsColumn(
                    w, Boolean,
                    permitted_value_checker=BIT_CHECKER,
                    comment=f"Pronounced {w} correctly (0 no, 1 yes)"
                )
            )
        super().__init__(name, bases, classdict)


class Nart(TaskHasPatientMixin, TaskHasClinicianMixin, Task,
           metaclass=NartMetaclass):
    """
    Server implementation of the NART task.
    """
    __tablename__ = "nart"
    shortname = "NART"

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("National Adult Reading Test")

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        return [CtvInfo(
            content=(
                "NART predicted WAIS FSIQ {n_fsiq}, WAIS VIQ {n_viq}, "
                "WAIS PIQ {n_piq}, WAIS-R FSIQ {nw_fsiq}, "
                "WAIS-IV FSIQ {b_fsiq}, WAIS-IV GAI {b_gai}, "
                "WAIS-IV VCI {b_vci}, WAIS-IV PRI {b_pri}, "
                "WAIS_IV WMI {b_wmi}, WAIS-IV PSI {b_psi}".format(
                    n_fsiq=self.nelson_full_scale_iq(),
                    n_viq=self.nelson_verbal_iq(),
                    n_piq=self.nelson_performance_iq(),
                    nw_fsiq=self.nelson_willison_full_scale_iq(),
                    b_fsiq=self.bright_full_scale_iq(),
                    b_gai=self.bright_general_ability(),
                    b_vci=self.bright_verbal_comprehension(),
                    b_pri=self.bright_perceptual_reasoning(),
                    b_wmi=self.bright_working_memory(),
                    b_psi=self.bright_perceptual_speed(),
                )
            )
        )]

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="nelson_full_scale_iq",
                coltype=Float(),
                value=self.nelson_full_scale_iq(),
                comment="Predicted WAIS full-scale IQ (Nelson 1982)"),
            SummaryElement(
                name="nelson_verbal_iq",
                coltype=Float(),
                value=self.nelson_verbal_iq(),
                comment="Predicted WAIS verbal IQ (Nelson 1982)"),
            SummaryElement(
                name="nelson_performance_iq",
                coltype=Float(),
                value=self.nelson_performance_iq(),
                comment="Predicted WAIS performance IQ (Nelson 1982"),
            SummaryElement(
                name="nelson_willison_full_scale_iq",
                coltype=Float(),
                value=self.nelson_willison_full_scale_iq(),
                comment="Predicted WAIS-R full-scale IQ (Nelson & Willison 1991"),  # noqa
            SummaryElement(
                name="bright_full_scale_iq",
                coltype=Float(),
                value=self.bright_full_scale_iq(),
                comment="Predicted WAIS-IV full-scale IQ (Bright 2016)"),
            SummaryElement(
                name="bright_general_ability",
                coltype=Float(),
                value=self.bright_general_ability(),
                comment="Predicted WAIS-IV General Ability Index (Bright 2016)"),  # noqa
            SummaryElement(
                name="bright_verbal_comprehension",
                coltype=Float(),
                value=self.bright_verbal_comprehension(),
                comment="Predicted WAIS-IV Verbal Comprehension Index (Bright 2016)"),  # noqa
            SummaryElement(
                name="bright_perceptual_reasoning",
                coltype=Float(),
                value=self.bright_perceptual_reasoning(),
                comment="Predicted WAIS-IV Perceptual Reasoning Index (Bright 2016)"),  # noqa
            SummaryElement(
                name="bright_working_memory",
                coltype=Float(),
                value=self.bright_working_memory(),
                comment="Predicted WAIS-IV Working Memory Index (Bright 2016)"),  # noqa
            SummaryElement(
                name="bright_perceptual_speed",
                coltype=Float(),
                value=self.bright_perceptual_speed(),
                comment="Predicted WAIS-IV Perceptual Speed Index (Bright 2016)"),  # noqa
        ]

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none(WORDLIST) and
            self.field_contents_valid()
        )

    def n_errors(self) -> int:
        e = 0
        for w in WORDLIST:
            if getattr(self, w) is not None and not getattr(self, w):
                e += 1
        return e

    def get_task_html(self, req: CamcopsRequest) -> str:
        # Table rows for individual words
        q_a = ""
        nwords = len(WORDLIST)
        ncolumns = 3
        nrows = int(math.ceil(float(nwords)/float(ncolumns)))
        column = 0
        row = 0
        # x: word index (shown in top-to-bottom then left-to-right sequence)
        for unused_loopvar in range(nwords):
            x = (column * nrows) + row
            if column == 0:  # first column
                q_a += "<tr>"
            q_a += td(ACCENTED_WORDLIST[x])
            q_a += td(answer(getattr(self, WORDLIST[x])))
            if column == (ncolumns - 1):  # last column
                q_a += "</tr>"
                row += 1
            column = (column + 1) % ncolumns

        # Annotations
        nelson = "; Nelson 1982 <sup>[1]</sup>"
        nelson_willison = "; Nelson &amp; Willison 1991 <sup>[2]</sup>"
        bright = "; Bright 2016 <sup>[3]</sup>"

        # HTML
        h = """
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {tr_is_complete}
                    {tr_total_errors}

                    {nelson_full_scale_iq}
                    {nelson_verbal_iq}
                    {nelson_performance_iq}
                    {nelson_willison_full_scale_iq}

                    {bright_full_scale_iq}
                    {bright_general_ability}
                    {bright_verbal_comprehension}
                    {bright_perceptual_reasoning}
                    {bright_working_memory}
                    {bright_perceptual_speed}
                </table>
            </div>
            <div class="{CssClass.EXPLANATION}">
                Estimates premorbid IQ by pronunciation of irregular words.
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="16%">Word</th><th width="16%">Correct?</th>
                    <th width="16%">Word</th><th width="16%">Correct?</th>
                    <th width="16%">Word</th><th width="16%">Correct?</th>
                </tr>
                {q_a}
            </table>
            <div class="{CssClass.FOOTNOTES}">
                [1] Nelson HE (1982), <i>National Adult Reading Test (NART):
                    For the Assessment of Premorbid Intelligence in Patients
                    with Dementia: Test Manual</i>, NFER-Nelson, Windsor, UK.
                [2] Nelson HE, Wilson J (1991)
                    <i>National Adult Reading Test (NART)</i>,
                    NFER-Nelson, Windsor, UK; see [3].
                [3] Bright P et al (2016). The National Adult Reading Test:
                    restandardisation against the Wechsler Adult Intelligence
                    Scale—Fourth edition.
                    <a href="https://www.ncbi.nlm.nih.gov/pubmed/27624393">PMID
                    27624393</a>.
            </div>
            <div class="{CssClass.COPYRIGHT}">
                NART: Copyright © Hazel E. Nelson. Used with permission.
            </div>
        """.format(
            CssClass=CssClass,
            tr_is_complete=self.get_is_complete_tr(req),
            tr_total_errors=tr_qa("Total errors", self.n_errors()),
            nelson_full_scale_iq=tr_qa(
                "Predicted WAIS full-scale IQ = 127.7 – 0.826 × errors" + nelson,  # noqa
                self.nelson_full_scale_iq()
            ),
            nelson_verbal_iq=tr_qa(
                "Predicted WAIS verbal IQ = 129.0 – 0.919 × errors" + nelson,
                self.nelson_verbal_iq()
            ),
            nelson_performance_iq=tr_qa(
                "Predicted WAIS performance IQ = 123.5 – 0.645 × errors" +
                nelson,
                self.nelson_performance_iq()
            ),
            nelson_willison_full_scale_iq=tr_qa(
                "Predicted WAIS-R full-scale IQ "
                "= 130.6 – 1.24 × errors" + nelson_willison,
                self.nelson_willison_full_scale_iq()
            ),
            bright_full_scale_iq=tr_qa(
                "Predicted WAIS-IV full-scale IQ "
                "= 126.41 – 0.9775 × errors" + bright,
                self.bright_full_scale_iq()
            ),
            bright_general_ability=tr_qa(
                "Predicted WAIS-IV General Ability Index "
                "= 126.5 – 0.9656 × errors" + bright,
                self.bright_general_ability()
            ),
            bright_verbal_comprehension=tr_qa(
                "Predicted WAIS-IV Verbal Comprehension Index "
                "= 126.81 – 1.0745 × errors" + bright,
                self.bright_verbal_comprehension()
            ),
            bright_perceptual_reasoning=tr_qa(
                "Predicted WAIS-IV Perceptual Reasoning Index "
                "= 120.18 – 0.6242 × errors" + bright,
                self.bright_perceptual_reasoning()
            ),
            bright_working_memory=tr_qa(
                "Predicted WAIS-IV Working Memory Index "
                "= 120.53 – 0.7901 × errors" + bright,
                self.bright_working_memory()
            ),
            bright_perceptual_speed=tr_qa(
                "Predicted WAIS-IV Perceptual Speed Index "
                "= 114.53 – 0.5285 × errors" + bright,
                self.bright_perceptual_speed()
            ),
            q_a=q_a,
        )
        return h

    def predict(self, intercept: float, slope: float) -> Optional[float]:
        if not self.is_complete():
            return None
        return intercept + slope * self.n_errors()

    def nelson_full_scale_iq(self) -> Optional[float]:
        return self.predict(intercept=127.7, slope=-0.826)

    def nelson_verbal_iq(self) -> Optional[float]:
        return self.predict(intercept=129.0, slope=-0.919)

    def nelson_performance_iq(self) -> Optional[float]:
        return self.predict(intercept=123.5, slope=-0.645)

    def nelson_willison_full_scale_iq(self) -> Optional[float]:
        return self.predict(intercept=130.6, slope=-1.24)

    def bright_full_scale_iq(self) -> Optional[float]:
        return self.predict(intercept=126.41, slope=-0.9775)

    def bright_general_ability(self) -> Optional[float]:
        return self.predict(intercept=126.5, slope=-0.9656)

    def bright_verbal_comprehension(self) -> Optional[float]:
        return self.predict(intercept=126.81, slope=-1.0745)

    def bright_perceptual_reasoning(self) -> Optional[float]:
        return self.predict(intercept=120.18, slope=-0.6242)

    def bright_working_memory(self) -> Optional[float]:
        return self.predict(intercept=120.53, slope=-0.7901)

    def bright_perceptual_speed(self) -> Optional[float]:
        return self.predict(intercept=114.53, slope=-0.5285)

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        codes = [SnomedExpression(req.snomed(SnomedLookup.NART_PROCEDURE_ASSESSMENT))]  # noqa
        if self.is_complete():
            codes.append(SnomedExpression(
                req.snomed(SnomedLookup.NART_SCALE),
                {
                    # Best value debatable:
                    req.snomed(SnomedLookup.NART_SCORE): self.nelson_full_scale_iq(),  # noqa
                }
            ))
        return codes
