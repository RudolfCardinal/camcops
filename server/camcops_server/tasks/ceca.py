#!/usr/bin/env python

"""
camcops_server/tasks/ceca.py

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

from typing import Any, Dict, List, Optional

import cardinal_pythonlib.rnc_web as ws
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Boolean, Float, Integer, UnicodeText

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_html import (
    answer,
    get_yes_no,
    get_yes_no_none,
    subheading_spanning_two_columns,
    tr,
    tr_qa,
)
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    BIT_CHECKER,
    CamcopsColumn,
    MIN_ZERO_CHECKER,
    ONE_TO_FOUR_CHECKER,
    ONE_TO_FIVE_CHECKER,
    PermittedValueChecker,
    ZERO_TO_TWO_CHECKER,
    ZERO_TO_THREE_CHECKER,
    ZERO_TO_FOUR_CHECKER,
    ZERO_TO_FIVE_CHECKER
)
from camcops_server.cc_modules.cc_summaryelement import SummaryElement
from camcops_server.cc_modules.cc_task import (
    get_from_dict,
    Task,
    TaskHasPatientMixin,
)


# =============================================================================
# CECA-Q3
# =============================================================================

FREQUENCY_COMMENT = "Frequency (0 never - 3 often)"


class CecaQ3(TaskHasPatientMixin, Task):
    """
    Server implementation of the CECA-Q3 task.
    """
    __tablename__ = "cecaq3"
    shortname = "CECA-Q3"

    # -------------------------------------------------------------------------
    # Section 1(A)
    # -------------------------------------------------------------------------
    s1a_motherfigure_birthmother = CamcopsColumn(
        "s1a_motherfigure_birthmother", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Raised by, maternal, birth mother?"
    )
    s1a_motherfigure_stepmother = CamcopsColumn(
        "s1a_motherfigure_stepmother", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Raised by, maternal, stepmother?"
    )
    s1a_motherfigure_femalerelative = CamcopsColumn(
        "s1a_motherfigure_femalerelative", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Raised by, maternal, female relative?"
    )
    s1a_motherfigure_femalerelative_detail = Column(
        "s1a_motherfigure_femalerelative_detail", UnicodeText,
        comment="Raised by, maternal, female relative, detail"
    )
    s1a_motherfigure_familyfriend = CamcopsColumn(
        "s1a_motherfigure_familyfriend", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Raised by, maternal, family friend?"
    )
    s1a_motherfigure_fostermother = CamcopsColumn(
        "s1a_motherfigure_fostermother", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Raised by, maternal, foster mother?"
    )
    s1a_motherfigure_adoptivemother = CamcopsColumn(
        "s1a_motherfigure_adoptivemother", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Raised by, maternal, adoptive mother?"
    )
    s1a_motherfigure_other = CamcopsColumn(
        "s1a_motherfigure_other", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Raised by, maternal, other?"
    )
    s1a_motherfigure_other_detail = Column(
        "s1a_motherfigure_other_detail", UnicodeText,
        comment="Raised by, maternal, other, detail"
    )
    s1a_fatherfigure_birthfather = CamcopsColumn(
        "s1a_fatherfigure_birthfather", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Raised by, paternal, birth father?"
    )
    s1a_fatherfigure_stepfather = CamcopsColumn(
        "s1a_fatherfigure_stepfather", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Raised by, paternal, stepfather?"
    )
    s1a_fatherfigure_malerelative = CamcopsColumn(
        "s1a_fatherfigure_malerelative", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Raised by, paternal, male relative?"
    )
    s1a_fatherfigure_malerelative_detail = Column(
        "s1a_fatherfigure_malerelative_detail", UnicodeText,
        comment="Raised by, paternal, male relative, detail"
    )
    s1a_fatherfigure_familyfriend = CamcopsColumn(
        "s1a_fatherfigure_familyfriend", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Raised by, paternal, family friend?"
    )
    s1a_fatherfigure_fosterfather = CamcopsColumn(
        "s1a_fatherfigure_fosterfather", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Raised by, paternal, foster father?"
    )
    s1a_fatherfigure_adoptivefather = CamcopsColumn(
        "s1a_fatherfigure_adoptivefather", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Raised by, paternal, adoptive father?"
    )
    s1a_fatherfigure_other = CamcopsColumn(
        "s1a_fatherfigure_other", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Raised by, paternal, other?"
    )
    s1a_fatherfigure_other_detail = Column(
        "s1a_fatherfigure_other_detail", UnicodeText,
        comment="Raised by, paternal, other, detail"
    )

    # -------------------------------------------------------------------------
    # Section 1(B)
    # -------------------------------------------------------------------------
    s1b_institution = CamcopsColumn(
        "s1b_institution", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="In institution before 17?"
    )
    s1b_institution_time_years = CamcopsColumn(
        "s1b_institution_time_years", Float,
        permitted_value_checker=MIN_ZERO_CHECKER,
        comment="In institution, time (years)"
    )

    # -------------------------------------------------------------------------
    # Section 1(C)
    # -------------------------------------------------------------------------
    s1c_mother_died = CamcopsColumn(
        "s1c_mother_died", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Mother died before 17?"
    )
    s1c_father_died = CamcopsColumn(
        "s1c_father_died", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Father died before 17?"
    )
    s1c_mother_died_subject_aged = CamcopsColumn(
        "s1c_mother_died_subject_aged", Float,
        permitted_value_checker=MIN_ZERO_CHECKER,
        comment="Age when mother died (years)"
    )
    s1c_father_died_subject_aged = CamcopsColumn(
        "s1c_father_died_subject_aged", Float,
        permitted_value_checker=MIN_ZERO_CHECKER,
        comment="Age when father died (years)"
    )
    s1c_separated_from_mother = CamcopsColumn(
        "s1c_separated_from_mother", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Separated from mother for >=1y before 17?"
    )
    s1c_separated_from_father = CamcopsColumn(
        "s1c_separated_from_father", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Separated from father for >=1y before 17?"
    )
    s1c_first_separated_from_mother_aged = CamcopsColumn(
        "s1c_first_separated_from_mother_aged", Float,
        permitted_value_checker=MIN_ZERO_CHECKER,
        comment="Maternal separation, age (years)"
    )
    s1c_first_separated_from_father_aged = CamcopsColumn(
        "s1c_first_separated_from_father_aged", Float,
        permitted_value_checker=MIN_ZERO_CHECKER,
        comment="Paternal separation, age (years)"
    )
    s1c_mother_how_long_first_separation_years = CamcopsColumn(
        "s1c_mother_how_long_first_separation_years", Float,
        permitted_value_checker=MIN_ZERO_CHECKER,
        comment="Maternal separation, how long first separation (y)"
    )
    s1c_father_how_long_first_separation_years = CamcopsColumn(
        "s1c_father_how_long_first_separation_years", Float,
        permitted_value_checker=MIN_ZERO_CHECKER,
        comment="Paternal separation, how long first separation (y)"
    )
    s1c_mother_separation_reason = CamcopsColumn(
        "s1c_mother_separation_reason", Integer,
        permitted_value_checker=PermittedValueChecker(minimum=1, maximum=6),
        comment="Maternal separation, reason "
                "(1 illness, 2 work, 3 divorce/separation, 4 never knew, "
                "5 abandoned, 6 other)"
    )
    s1c_father_separation_reason = CamcopsColumn(
        "s1c_father_separation_reason", Integer,
        permitted_value_checker=PermittedValueChecker(minimum=1, maximum=6),
        comment="Paternal separation, reason "
                "(1 illness, 2 work, 3 divorce/separation, 4 never knew, "
                "5 abandoned, 6 other)"
    )
    s1c_describe_experience = Column(
        "s1c_describe_experience", UnicodeText,
        comment="Loss of/separation from parent, description"
    )

    # -------------------------------------------------------------------------
    # Section 2(A)
    # -------------------------------------------------------------------------
    s2a_which_mother_figure = CamcopsColumn(
        "s2a_which_mother_figure", Integer,
        permitted_value_checker=PermittedValueChecker(minimum=0, maximum=5),
        comment="Mother figure, which one (0 none/skip, 1 birth mother, "
                "2 stepmother, 3 other relative, 4 other non-relative, "
                "5 other)"
    )
    s2a_which_mother_figure_other_detail = Column(
        "s2a_which_mother_figure_other_detail", UnicodeText,
        comment="Mother figure, other, detail"
    )
    s2a_q1 = CamcopsColumn(
        "s2a_q1", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Mother figure, difficult to please (1 no - 5 yes)"
    )
    s2a_q2 = CamcopsColumn(
        "s2a_q2", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Mother figure, concerned re my worries (1 no - 5 yes)"
    )
    s2a_q3 = CamcopsColumn(
        "s2a_q3", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Mother figure, interested re school (1 no - 5 yes)"
    )
    s2a_q4 = CamcopsColumn(
        "s2a_q4", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Mother figure, made me feel unwanted (1 no - 5 yes)"
    )
    s2a_q5 = CamcopsColumn(
        "s2a_q5", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Mother figure, better when upset (1 no - 5 yes)"
    )
    s2a_q6 = CamcopsColumn(
        "s2a_q6", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Mother figure, critical (1 no - 5 yes)"
    )
    s2a_q7 = CamcopsColumn(
        "s2a_q7", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Mother figure, unsupervised <10y (1 no - 5 yes)"
    )
    s2a_q8 = CamcopsColumn(
        "s2a_q8", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Mother figure, time to talk (1 no - 5 yes)"
    )
    s2a_q9 = CamcopsColumn(
        "s2a_q9", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Mother figure, nuisance (1 no - 5 yes)"
    )
    s2a_q10 = CamcopsColumn(
        "s2a_q10", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Mother figure, picked on unfairly (1 no - 5 yes)"
    )
    s2a_q11 = CamcopsColumn(
        "s2a_q11", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Mother figure, there if needed (1 no - 5 yes)"
    )
    s2a_q12 = CamcopsColumn(
        "s2a_q12", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Mother figure, interested in friends (1 no - 5 yes)"
    )
    s2a_q13 = CamcopsColumn(
        "s2a_q13", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Mother figure, concerned re whereabouts (1 no - 5 yes)"
    )
    s2a_q14 = CamcopsColumn(
        "s2a_q14", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Mother figure, cared when ill (1 no - 5 yes)"
    )
    s2a_q15 = CamcopsColumn(
        "s2a_q15", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Mother figure, neglected basic needs (1 no - 5 yes)"
    )
    s2a_q16 = CamcopsColumn(
        "s2a_q16", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Mother figure, preferred siblings (1 no - 5 yes)"
    )
    s2a_extra = Column(
        "s2a_extra", UnicodeText,
        comment="Mother figure, extra detail"
    )

    # -------------------------------------------------------------------------
    # Section 2(B)
    # -------------------------------------------------------------------------
    s2b_q1 = CamcopsColumn(
        "s2b_q1", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Mother figure, tease me (0 no - 2 yes)"
    )
    s2b_q2 = CamcopsColumn(
        "s2b_q2", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Mother figure, made me keep secrets (0 no - 2 yes)"
    )
    s2b_q3 = CamcopsColumn(
        "s2b_q3", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Mother figure, undermined confidence (0 no - 2 yes)"
    )
    s2b_q4 = CamcopsColumn(
        "s2b_q4", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Mother figure, contradictory (0 no - 2 yes)"
    )
    s2b_q5 = CamcopsColumn(
        "s2b_q5", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Mother figure, played on fears (0 no - 2 yes)"
    )
    s2b_q6 = CamcopsColumn(
        "s2b_q6", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Mother figure, liked to see me suffer (0 no - 2 yes)"
    )
    s2b_q7 = CamcopsColumn(
        "s2b_q7", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Mother figure, humiliated me (0 no - 2 yes)"
    )
    s2b_q8 = CamcopsColumn(
        "s2b_q8", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Mother figure, shamed me before others (0 no - 2 yes)"
    )
    s2b_q9 = CamcopsColumn(
        "s2b_q9", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Mother figure, rejecting (0 no - 2 yes)"
    )
    s2b_q10 = CamcopsColumn(
        "s2b_q10", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Mother figure, took things I cherished (0 no - 2 yes)"
    )
    s2b_q11 = CamcopsColumn(
        "s2b_q11", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Mother figure, eat disliked until sick (0 no - 2 yes)"
    )
    s2b_q12 = CamcopsColumn(
        "s2b_q12", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Mother figure, deprived light/food/company (0 no - 2 yes)"
    )
    s2b_q13 = CamcopsColumn(
        "s2b_q13", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Mother figure, wouldn't let me mix (0 no - 2 yes)"
    )
    s2b_q14 = CamcopsColumn(
        "s2b_q14", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Mother figure, obedience through guilt (0 no - 2 yes)"
    )
    s2b_q15 = CamcopsColumn(
        "s2b_q15", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Mother figure, threatened to hurt people dear to me "
                "(0 no - 2 yes)"
    )
    s2b_q16 = CamcopsColumn(
        "s2b_q16", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Mother figure, forced to break law (0 no - 2 yes)"
    )
    s2b_q17 = CamcopsColumn(
        "s2b_q17", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Mother figure, said wanted me dead (0 no - 2 yes)"
    )
    s2b_q1_frequency = CamcopsColumn(
        "s2b_q1_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s2b_q2_frequency = CamcopsColumn(
        "s2b_q2_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s2b_q3_frequency = CamcopsColumn(
        "s2b_q3_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s2b_q4_frequency = CamcopsColumn(
        "s2b_q4_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s2b_q5_frequency = CamcopsColumn(
        "s2b_q5_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s2b_q6_frequency = CamcopsColumn(
        "s2b_q6_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s2b_q7_frequency = CamcopsColumn(
        "s2b_q7_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s2b_q8_frequency = CamcopsColumn(
        "s2b_q8_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s2b_q9_frequency = CamcopsColumn(
        "s2b_q9_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s2b_q10_frequency = CamcopsColumn(
        "s2b_q10_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s2b_q11_frequency = CamcopsColumn(
        "s2b_q11_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s2b_q12_frequency = CamcopsColumn(
        "s2b_q12_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s2b_q13_frequency = CamcopsColumn(
        "s2b_q13_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s2b_q14_frequency = CamcopsColumn(
        "s2b_q14_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s2b_q15_frequency = CamcopsColumn(
        "s2b_q15_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s2b_q16_frequency = CamcopsColumn(
        "s2b_q16_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s2b_q17_frequency = CamcopsColumn(
        "s2b_q17_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s2b_age_began = CamcopsColumn(
        "s2b_age_began", Float,
        permitted_value_checker=MIN_ZERO_CHECKER,
        comment="Age these experienced began (years)"
    )
    s2b_extra = Column(
        "s2b_extra", UnicodeText,
        comment="Extra detail"
    )

    # -------------------------------------------------------------------------
    # Section 3(A)
    # -------------------------------------------------------------------------
    s3a_which_father_figure = CamcopsColumn(
        "s3a_which_father_figure", Integer,
        permitted_value_checker=ZERO_TO_FIVE_CHECKER,
        comment="Father figure, which one (0 none/skip, 1 birth father, "
                "2 stepfather, 3 other relative, 4 other non-relative, "
                "5 other)"
    )
    s3a_which_father_figure_other_detail = Column(
        "s3a_which_father_figure_other_detail", UnicodeText,
        comment="Father figure, other, detail"
    )
    s3a_q1 = CamcopsColumn(
        "s3a_q1", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Father figure, difficult to please (1 no - 5 yes)"
    )
    s3a_q2 = CamcopsColumn(
        "s3a_q2", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Father figure, concerned re my worries (1 no - 5 yes)"
    )
    s3a_q3 = CamcopsColumn(
        "s3a_q3", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Father figure, interested re school (1 no - 5 yes)"
    )
    s3a_q4 = CamcopsColumn(
        "s3a_q4", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Father figure, made me feel unwanted (1 no - 5 yes)"
    )
    s3a_q5 = CamcopsColumn(
        "s3a_q5", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Father figure, better when upset (1 no - 5 yes)"
    )
    s3a_q6 = CamcopsColumn(
        "s3a_q6", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Father figure, critical (1 no - 5 yes)"
    )
    s3a_q7 = CamcopsColumn(
        "s3a_q7", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Father figure, unsupervised <10y (1 no - 5 yes)"
    )
    s3a_q8 = CamcopsColumn(
        "s3a_q8", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Father figure, time to talk (1 no - 5 yes)"
    )
    s3a_q9 = CamcopsColumn(
        "s3a_q9", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Father figure, nuisance (1 no - 5 yes)"
    )
    s3a_q10 = CamcopsColumn(
        "s3a_q10", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Father figure, picked on unfairly (1 no - 5 yes)"
    )
    s3a_q11 = CamcopsColumn(
        "s3a_q11", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Father figure, there if needed (1 no - 5 yes)"
    )
    s3a_q12 = CamcopsColumn(
        "s3a_q12", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Father figure, interested in friends (1 no - 5 yes)"
    )
    s3a_q13 = CamcopsColumn(
        "s3a_q13", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Father figure, concerned re whereabouts (1 no - 5 yes)"
    )
    s3a_q14 = CamcopsColumn(
        "s3a_q14", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Father figure, cared when ill (1 no - 5 yes)"
    )
    s3a_q15 = CamcopsColumn(
        "s3a_q15", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Father figure, neglected basic needs (1 no - 5 yes)"
    )
    s3a_q16 = CamcopsColumn(
        "s3a_q16", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Father figure, preferred siblings (1 no - 5 yes)"
    )
    s3a_extra = Column(
        "s3a_extra", UnicodeText,
        comment="Father figure, extra detail"
    )

    # -------------------------------------------------------------------------
    # Section 3(B)
    # -------------------------------------------------------------------------
    s3b_q1 = CamcopsColumn(
        "s3b_q1", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Father figure, tease me (0 no - 2 yes)"
    )
    s3b_q2 = CamcopsColumn(
        "s3b_q2", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Father figure, made me keep secrets (0 no - 2 yes)"
    )
    s3b_q3 = CamcopsColumn(
        "s3b_q3", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Father figure, undermined confidence (0 no - 2 yes)"
    )
    s3b_q4 = CamcopsColumn(
        "s3b_q4", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Father figure, contradictory (0 no - 2 yes)"
    )
    s3b_q5 = CamcopsColumn(
        "s3b_q5", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Father figure, played on fears (0 no - 2 yes)"
    )
    s3b_q6 = CamcopsColumn(
        "s3b_q6", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Father figure, liked to see me suffer (0 no - 2 yes)"
    )
    s3b_q7 = CamcopsColumn(
        "s3b_q7", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Father figure, humiliated me (0 no - 2 yes)"
    )
    s3b_q8 = CamcopsColumn(
        "s3b_q8", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Father figure, shamed me before others (0 no - 2 yes)"
    )
    s3b_q9 = CamcopsColumn(
        "s3b_q9", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Father figure, rejecting (0 no - 2 yes)"
    )
    s3b_q10 = CamcopsColumn(
        "s3b_q10", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Father figure, took things I cherished (0 no - 2 yes)"
    )
    s3b_q11 = CamcopsColumn(
        "s3b_q11", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Father figure, eat disliked until sick (0 no - 2 yes)"
    )
    s3b_q12 = CamcopsColumn(
        "s3b_q12", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Father figure, deprived light/food/company (0 no - 2 yes)"
    )
    s3b_q13 = CamcopsColumn(
        "s3b_q13", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Father figure, wouldn't let me mix (0 no - 2 yes)"
    )
    s3b_q14 = CamcopsColumn(
        "s3b_q14", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Father figure, obedience through guilt (0 no - 2 yes)"
    )
    s3b_q15 = CamcopsColumn(
        "s3b_q15", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Father figure, threatened to hurt people dear to me "
                "(0 no - 2 yes)"
    )
    s3b_q16 = CamcopsColumn(
        "s3b_q16", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Father figure, forced to break law (0 no - 2 yes)"
    )
    s3b_q17 = CamcopsColumn(
        "s3b_q17", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Father figure, said wanted me dead (0 no - 2 yes)"
    )
    s3b_q1_frequency = CamcopsColumn(
        "s3b_q1_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s3b_q2_frequency = CamcopsColumn(
        "s3b_q2_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s3b_q3_frequency = CamcopsColumn(
        "s3b_q3_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s3b_q4_frequency = CamcopsColumn(
        "s3b_q4_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s3b_q5_frequency = CamcopsColumn(
        "s3b_q5_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s3b_q6_frequency = CamcopsColumn(
        "s3b_q6_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s3b_q7_frequency = CamcopsColumn(
        "s3b_q7_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s3b_q8_frequency = CamcopsColumn(
        "s3b_q8_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s3b_q9_frequency = CamcopsColumn(
        "s3b_q9_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s3b_q10_frequency = CamcopsColumn(
        "s3b_q10_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s3b_q11_frequency = CamcopsColumn(
        "s3b_q11_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s3b_q12_frequency = CamcopsColumn(
        "s3b_q12_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s3b_q13_frequency = CamcopsColumn(
        "s3b_q13_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s3b_q14_frequency = CamcopsColumn(
        "s3b_q14_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s3b_q15_frequency = CamcopsColumn(
        "s3b_q15_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s3b_q16_frequency = CamcopsColumn(
        "s3b_q16_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s3b_q17_frequency = CamcopsColumn(
        "s3b_q17_frequency", Integer,
        permitted_value_checker=ZERO_TO_THREE_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    s3b_age_began = CamcopsColumn(
        "s3b_age_began", Float,
        permitted_value_checker=MIN_ZERO_CHECKER,
        comment="Age these experienced began (years)"
    )
    s3b_extra = Column(
        "s3b_extra", UnicodeText,
        comment="Extra detail"
    )

    # -------------------------------------------------------------------------
    # Section 3(C)
    # -------------------------------------------------------------------------
    s3c_q1 = CamcopsColumn(
        "s3c_q1", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Responsibility (1 no - 5 yes)"
    )
    s3c_q2 = CamcopsColumn(
        "s3c_q2", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Housework (1 no - 5 yes)"
    )
    s3c_q3 = CamcopsColumn(
        "s3c_q3", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Look after young siblings (1 no - 5 yes)"
    )
    s3c_q4 = CamcopsColumn(
        "s3c_q4", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Cooking/cleaning (1 no - 5 yes)"
    )
    s3c_q5 = CamcopsColumn(
        "s3c_q5", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Miss school for domestic responsibilities (1 no - 5 yes)"
    )
    s3c_q6 = CamcopsColumn(
        "s3c_q6", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Miss seeing friends for domestic responsibilities "
                "(1 no - 5 yes)"
    )
    s3c_q7 = CamcopsColumn(
        "s3c_q7", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Parents said they couldn't cope (1 no - 5 yes)"
    )
    s3c_q8 = CamcopsColumn(
        "s3c_q8", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Parents looked to you for help (1 no - 5 yes)"
    )
    s3c_q9 = CamcopsColumn(
        "s3c_q9", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Parents coped if you were hurt/ill (1 no - 5 yes)"
    )
    s3c_q10 = CamcopsColumn(
        "s3c_q10", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Parents confided their problems (1 no - 5 yes)"
    )
    s3c_q11 = CamcopsColumn(
        "s3c_q11", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Parents relied on you for emotional support (1 no - 5 yes)"
    )
    s3c_q12 = CamcopsColumn(
        "s3c_q12", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Parents cried in front of you (1 no - 5 yes)"
    )
    s3c_q13 = CamcopsColumn(
        "s3c_q13", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Concerned/worried re parent (1 no - 5 yes)"
    )
    s3c_q14 = CamcopsColumn(
        "s3c_q14", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Tried to support/care for parent (1 no - 5 yes)"
    )
    s3c_q15 = CamcopsColumn(
        "s3c_q15", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Try to make parent smile when upset (1 no - 5 yes)"
    )
    s3c_q16 = CamcopsColumn(
        "s3c_q16", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Parents made you feel guilty for their sacrifices "
                "(1 no - 5 yes)"
    )
    s3c_q17 = CamcopsColumn(
        "s3c_q17", Integer,
        permitted_value_checker=ONE_TO_FIVE_CHECKER,
        comment="Had to keep secrets for parent (1 no - 5 yes)"
    )
    s3c_which_parent_cared_for = CamcopsColumn(
        "s3c_which_parent_cared_for", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment="Which parent did you have to provide care for (0 none, "
                "1 mother, 2 father, 3 both, 4 other)"
    )
    s3c_parent_mental_problem = CamcopsColumn(
        "s3c_parent_mental_problem", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Parent/s had emotional/mental health problems (0 no - 2 yes)"
    )
    s3c_parent_physical_problem = CamcopsColumn(
        "s3c_parent_physical_problem", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Parent/s had disability/physical illness (0 no - 2 yes)"
    )

    # -------------------------------------------------------------------------
    # Section 4(A)
    # -------------------------------------------------------------------------
    s4a_adultconfidant = CamcopsColumn(
        "s4a_adultconfidant", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Adult confidant?"
    )
    s4a_adultconfidant_mother = CamcopsColumn(
        "s4a_adultconfidant_mother", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Adult confidant, mother?"
    )
    s4a_adultconfidant_father = CamcopsColumn(
        "s4a_adultconfidant_father", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Adult confidant, father?"
    )
    s4a_adultconfidant_otherrelative = CamcopsColumn(
        "s4a_adultconfidant_otherrelative", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Adult confidant, other relative?"
    )
    s4a_adultconfidant_familyfriend = CamcopsColumn(
        "s4a_adultconfidant_familyfriend", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Adult confidant, family friend?"
    )
    s4a_adultconfidant_responsibleadult = CamcopsColumn(
        "s4a_adultconfidant_responsibleadult", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Adult confidant, teacher/vicar/etc.?"
    )
    s4a_adultconfidant_other = CamcopsColumn(
        "s4a_adultconfidant_other", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Adult confidant, other?"
    )
    s4a_adultconfidant_other_detail = Column(
        "s4a_adultconfidant_other_detail", UnicodeText,
        comment="Adult confidant, other, detail"
    )
    s4a_adultconfidant_additional = Column(
        "s4a_adultconfidant_additional", UnicodeText,
        comment="Adult confidant, additional notes"
    )

    # -------------------------------------------------------------------------
    # Section 4(B)
    # -------------------------------------------------------------------------
    s4b_childconfidant = CamcopsColumn(
        "s4b_childconfidant", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Child confidant?"
    )
    s4b_childconfidant_sister = CamcopsColumn(
        "s4b_childconfidant_sister", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Child confidant, sister?"
    )
    s4b_childconfidant_brother = CamcopsColumn(
        "s4b_childconfidant_brother", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Child confidant, brother?"
    )
    s4b_childconfidant_otherrelative = CamcopsColumn(
        "s4b_childconfidant_otherrelative", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Child confidant, other relative?"
    )
    s4b_childconfidant_closefriend = CamcopsColumn(
        "s4b_childconfidant_closefriend", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Child confidant, close friend?"
    )
    s4b_childconfidant_otherfriend = CamcopsColumn(
        "s4b_childconfidant_otherfriend", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Child confidant, other less close friend(s)?"
    )
    s4b_childconfidant_other = CamcopsColumn(
        "s4b_childconfidant_other", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Child confidant, other person?"
    )
    s4b_childconfidant_other_detail = Column(
        "s4b_childconfidant_other_detail", UnicodeText,
        comment="Child confidant, other person, detail"
    )
    s4b_childconfidant_additional = Column(
        "s4b_childconfidant_additional", UnicodeText,
        comment="Child confidant, additional notes"
    )

    # -------------------------------------------------------------------------
    # Section 4(C)
    # -------------------------------------------------------------------------
    s4c_closest_mother = CamcopsColumn(
        "s4c_closest_mother", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Two closest people include: mother?"
    )
    s4c_closest_father = CamcopsColumn(
        "s4c_closest_father", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Two closest people include: father?"
    )
    s4c_closest_sibling = CamcopsColumn(
        "s4c_closest_sibling", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Two closest people include: sibling?"
    )
    s4c_closest_otherrelative = CamcopsColumn(
        "s4c_closest_otherrelative", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Two closest people include: other relative?"
    )
    s4c_closest_adultfriend = CamcopsColumn(
        "s4c_closest_adultfriend", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Two closest people include: adult family friend?"
    )
    s4c_closest_childfriend = CamcopsColumn(
        "s4c_closest_childfriend", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Two closest people include: friend your age?"
    )
    s4c_closest_other = CamcopsColumn(
        "s4c_closest_other", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Two closest people include: other?"
    )
    s4c_closest_other_detail = Column(
        "s4c_closest_other_detail", UnicodeText,
        comment="Two closest people include: other, detail"
    )
    s4c_closest_additional = Column(
        "s4c_closest_additional", UnicodeText,
        comment="Two closest people include: additional notes"
    )

    # -------------------------------------------------------------------------
    # Section 5(C)
    # -------------------------------------------------------------------------
    s5c_physicalabuse = CamcopsColumn(
        "s5c_physicalabuse", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Physical abuse?"
    )
    s5c_abused_by_mother = CamcopsColumn(
        "s5c_abused_by_mother", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Physical abuse, by mother?"
    )
    s5c_abused_by_father = CamcopsColumn(
        "s5c_abused_by_father", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Physical abuse, by father?"
    )
    s5c_mother_abuse_age_began = CamcopsColumn(
        "s5c_mother_abuse_age_began", Float,
        comment="Physical abuse, by mother, age began (y)"
    )
    s5c_father_abuse_age_began = CamcopsColumn(
        "s5c_father_abuse_age_began", Float,
        comment="Physical abuse, by father, age began (y)"
    )
    s5c_mother_hit_more_than_once = CamcopsColumn(
        "s5c_mother_hit_more_than_once", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Physical abuse, by mother, hit on >1 occasion"
    )
    s5c_father_hit_more_than_once = CamcopsColumn(
        "s5c_father_hit_more_than_once", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Physical abuse, by father, hit on >1 occasion"
    )
    s5c_mother_hit_how = CamcopsColumn(
        "s5c_mother_hit_how", Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment="Physical abuse, by mother, hit how (1 belt/stick, "
                "2 punched/kicked, 3 hit with hand, 4 other)"
    )
    s5c_father_hit_how = CamcopsColumn(
        "s5c_father_hit_how", Integer,
        permitted_value_checker=ONE_TO_FOUR_CHECKER,
        comment="Physical abuse, by father, hit how (1 belt/stick, "
                "2 punched/kicked, 3 hit with hand, 4 other)"
    )
    s5c_mother_injured = CamcopsColumn(
        "s5c_mother_injured", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Physical abuse, by mother, injured?"
    )
    s5c_father_injured = CamcopsColumn(
        "s5c_father_injured", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Physical abuse, by father, injured?"
    )
    s5c_mother_out_of_control = CamcopsColumn(
        "s5c_mother_out_of_control", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Physical abuse, by mother, out of control?"
    )
    s5c_father_out_of_control = CamcopsColumn(
        "s5c_father_out_of_control", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Physical abuse, by father, out of control?"
    )
    s5c_parental_abuse_description = Column(
        "s5c_parental_abuse_description", UnicodeText,
        comment="Physical abuse, description"
    )
    s5c_abuse_by_nonparent = CamcopsColumn(
        "s5c_abuse_by_nonparent", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Physical abuse, by anyone else in household?"
    )
    s5c_nonparent_abuse_description = Column(
        "s5c_nonparent_abuse_description", UnicodeText,
        comment="Physical abuse, nonparent, description"
    )

    # -------------------------------------------------------------------------
    # Section 6
    # -------------------------------------------------------------------------
    s6_any_unwanted_sexual_experience = CamcopsColumn(
        # Prior to 2.1.0: was cctype="BOOL" on the server, but this gave
        # TINYINT(1), which can store -128 to 128. Corrected to Integer.
        "s6_any_unwanted_sexual_experience", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Any unwanted sexual experiences (0 no - 2 yes)"
    )
    s6_unwanted_intercourse = CamcopsColumn(
        # Prior to 2.1.0: was cctype="BOOL" on the server, but this gave
        # TINYINT(1), which can store -128 to 128. Corrected to Integer.
        "s6_unwanted_intercourse", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Unwanted intercourse before 17yo (0 no - 2 yes)"
    )
    s6_upsetting_sexual_adult_authority = CamcopsColumn(
        # Prior to 2.1.0: was cctype="BOOL" on the server, but this gave
        # TINYINT(1), which can store -128 to 128. Corrected to Integer.
        "s6_upsetting_sexual_adult_authority", Integer,
        permitted_value_checker=ZERO_TO_TWO_CHECKER,
        comment="Upsetting sexual experiences under 17yo with "
                "related adult or someone in authority (0 no - 2 yes)"
    )
    s6_first_age = CamcopsColumn(
        "s6_first_age", Float,
        permitted_value_checker=MIN_ZERO_CHECKER,
        comment="Sexual abuse, first experience, age it began"
    )
    s6_other_age = CamcopsColumn(
        "s6_other_age", Float,
        permitted_value_checker=MIN_ZERO_CHECKER,
        comment="Sexual abuse, other experience, age it began"
    )
    s6_first_person_known = CamcopsColumn(
        "s6_first_person_known", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Sexual abuse, first experience, knew the person?"
    )
    s6_other_person_known = CamcopsColumn(
        "s6_other_person_known", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Sexual abuse, other experience, knew the person?"
    )
    s6_first_relative = CamcopsColumn(
        "s6_first_relative", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Sexual abuse, first experience, person was a relative?"
    )
    s6_other_relative = CamcopsColumn(
        "s6_other_relative", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Sexual abuse, other experience, person was a relative?"
    )
    s6_first_in_household = CamcopsColumn(
        "s6_first_in_household", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Sexual abuse, first experience, person lived in household?"
    )
    s6_other_in_household = CamcopsColumn(
        "s6_other_in_household", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Sexual abuse, other experience, person lived in household?"
    )
    s6_first_more_than_once = CamcopsColumn(
        "s6_first_more_than_once", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Sexual abuse, first experience, happened more than once?"
    )
    s6_other_more_than_once = CamcopsColumn(
        "s6_other_more_than_once", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Sexual abuse, other experience, happened more than once?"
    )
    s6_first_touch_privates_subject = CamcopsColumn(
        "s6_first_touch_privates_subject", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Sexual abuse, first experience, touched your private parts?"
    )
    s6_other_touch_privates_subject = CamcopsColumn(
        "s6_other_touch_privates_subject", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Sexual abuse, other experience, touched your private parts?"
    )
    s6_first_touch_privates_other = CamcopsColumn(
        "s6_first_touch_privates_other", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Sexual abuse, first experience, touched their private parts?"
    )
    s6_other_touch_privates_other = CamcopsColumn(
        "s6_other_touch_privates_other", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Sexual abuse, other experience, touched their private parts?"
    )
    s6_first_intercourse = CamcopsColumn(
        "s6_first_intercourse", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Sexual abuse, first experience, sexual intercourse?"
    )
    s6_other_intercourse = CamcopsColumn(
        "s6_other_intercourse", Boolean,
        permitted_value_checker=BIT_CHECKER,
        comment="Sexual abuse, other experience, sexual intercourse?"
    )
    s6_unwanted_sexual_description = Column(
        "s6_unwanted_sexual_description", UnicodeText,
        comment="Sexual abuse, description"
    )

    # -------------------------------------------------------------------------
    # Final
    # -------------------------------------------------------------------------
    any_other_comments = CamcopsColumn(
        "any_other_comments", UnicodeText,
        comment="Any other comments"
    )

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Childhood Experience of Care and Abuse Questionnaire")

    def get_summaries(self, req: CamcopsRequest) -> List[SummaryElement]:
        return self.standard_task_summary_fields() + [
            SummaryElement(
                name="parental_loss_risk",
                coltype=Boolean(),
                value=self.parental_loss_risk(),
                comment="Parental loss risk factor?"),
            SummaryElement(
                name="parental_loss_high_risk",
                coltype=Boolean(),
                value=self.parental_loss_high_risk(),
                comment="Parental loss higher risk factor?"),
            SummaryElement(
                name="mother_antipathy",
                coltype=Integer(),
                value=self.mother_antipathy(),
                comment="Maternal antipathy score (8-40)"),
            SummaryElement(
                name="mother_neglect",
                coltype=Integer(),
                value=self.mother_neglect(),
                comment="Maternal neglect score (8-40)"),
            SummaryElement(
                name="mother_psychological_abuse",
                coltype=Integer(),
                value=self.mother_psychological_abuse(),
                comment="Maternal psychological abuse score (0-85)"),
            SummaryElement(
                name="father_antipathy",
                coltype=Integer(),
                value=self.father_antipathy(),
                comment="Paternal antipathy score (8-40)"),
            SummaryElement(
                name="father_neglect",
                coltype=Integer(),
                value=self.father_neglect(),
                comment="Paternal neglect score (8-40)"),
            SummaryElement(
                name="father_psychological_abuse",
                coltype=Integer(),
                value=self.father_psychological_abuse(),
                comment="Paternal psychological abuse score (0-85)"),
            SummaryElement(
                name="role_reversal",
                coltype=Integer(),
                value=self.role_reversal(),
                comment="Role reversal score (17-85)"),
            SummaryElement(
                name="physical_abuse_screen",
                coltype=Integer(),
                value=self.physical_abuse_screen(),
                comment="Physical abuse screen (0-1)"),
            SummaryElement(
                name="physical_abuse_severity_mother",
                coltype=Integer(),
                value=self.physical_abuse_severity_mother(),
                comment="Maternal physical abuse severity score (0-4)"),
            SummaryElement(
                name="physical_abuse_severity_father",
                coltype=Integer(),
                value=self.physical_abuse_severity_father(),
                comment="Paternal physical abuse severity score (0-4)"),
            SummaryElement(
                name="sexual_abuse_screen",
                coltype=Integer(),
                value=self.sexual_abuse_screen(),
                comment="Sexual abuse screen (0-3)"),
            SummaryElement(
                name="sexual_abuse_score_first",
                coltype=Integer(),
                value=self.sexual_abuse_score_first(),
                comment="First sexual abuse severity score (0-7)"),
            SummaryElement(
                name="sexual_abuse_score_other",
                coltype=Integer(),
                value=self.sexual_abuse_score_other(),
                comment="Other sexual abuse severity score (0-7)"),
        ]

    # -------------------------------------------------------------------------
    # Complete?
    # -------------------------------------------------------------------------

    def is_complete(self) -> bool:
        return (
            self.complete_1a() and
            self.complete_1b() and
            self.complete_1c() and
            self.complete_2a() and
            self.complete_2b() and
            self.complete_3a() and
            self.complete_3b() and
            self.complete_3c() and
            self.complete_4a() and
            self.complete_4b() and
            self.complete_4c() and
            self.complete_5() and
            self.complete_6() and
            self.field_contents_valid()
        )

    def is_at_least_one_field_true(self, fields: List[str]) -> bool:
        for f in fields:
            if getattr(self, f):
                return True
        return True

    def complete_1a(self) -> bool:
        if not self.is_at_least_one_field_true([
            "s1a_motherfigure_birthmother",
            "s1a_motherfigure_stepmother",
            "s1a_motherfigure_femalerelative",
            "s1a_motherfigure_familyfriend",
            "s1a_motherfigure_fostermother",
            "s1a_motherfigure_adoptivemother",
            "s1a_motherfigure_other",
            "s1a_fatherfigure_birthfather",
            "s1a_fatherfigure_stepfather",
            "s1a_fatherfigure_malerelative",
            "s1a_fatherfigure_familyfriend",
            "s1a_fatherfigure_fosterfather",
            "s1a_fatherfigure_adoptivefather",
            "s1a_fatherfigure_other",
        ]):
            return False
        if (self.s1a_motherfigure_other and
                not self.s1a_motherfigure_other_detail):
            return False
        if (self.s1a_motherfigure_femalerelative and
                not self.s1a_motherfigure_femalerelative_detail):
            return False
        if (self.s1a_fatherfigure_other and
                not self.s1a_fatherfigure_other_detail):
            return False
        if (self.s1a_fatherfigure_malerelative and
                not self.s1a_fatherfigure_malerelative_detail):
            return False
        return True

    def complete_1b(self) -> bool:
        if self.s1b_institution is None:
            return False
        if self.s1b_institution and self.s1b_institution_time_years is None:
            return False
        return True

    def complete_1c(self) -> bool:
        if self.s1c_mother_died is None or self.s1c_father_died is None:
            return False
        if self.s1c_mother_died and self.s1c_mother_died_subject_aged is None:
            return False
        if self.s1c_father_died and self.s1c_father_died_subject_aged is None:
            return False
        if (self.s1c_separated_from_mother is None or
                self.s1c_separated_from_father is None):
            return False
        if self.s1c_separated_from_mother:
            if self.any_fields_none(["s1c_first_separated_from_mother_aged",
                                     "s1c_mother_how_long_first_separation_years",  # noqa
                                     "s1c_mother_separation_reason"]):
                return False
        if self.s1c_separated_from_father:
            if self.any_fields_none(["s1c_first_separated_from_father_aged",
                                     "s1c_father_how_long_first_separation_years",  # noqa
                                     "s1c_father_separation_reason"]):
                return False
        return True

    def complete_2a(self) -> bool:
        if self.s2a_which_mother_figure is None:
            return False
        if self.s2a_which_mother_figure == 0:
            return True
        if (self.s2a_which_mother_figure == 5 and
                self.s2a_which_mother_figure_other_detail is None):
            return False
        for i in range(1, 16):  # not q16 (siblings)
            if getattr(self, "s2a_q" + str(i)) is None:
                return False
        return True

    def complete_2b(self) -> bool:
        abuse = False
        if self.s2a_which_mother_figure == 0:
            return True
        for i in range(1, 18):
            if getattr(self, "s2b_q" + str(i)) is None:
                return False
            if getattr(self, "s2b_q" + str(i)) != 0:
                abuse = True
                if getattr(self, "s2b_q" + str(i) + "_frequency") is None:
                    return False
        if abuse and self.s2b_age_began is None:
            return False
        return True

    def complete_3a(self):
        if self.s3a_which_father_figure is None:
            return False
        if self.s3a_which_father_figure == 0:
            return True
        if (self.s3a_which_father_figure == 5 and
                self.s3a_which_father_figure_other_detail is None):
            return False
        for i in range(1, 16):  # not q16 (siblings)
            if getattr(self, "s3a_q" + str(i)) is None:
                return False
        return True

    def complete_3b(self) -> bool:
        abuse = False
        if self.s3a_which_father_figure == 0:
            return True
        for i in range(1, 18):
            if getattr(self, "s3b_q" + str(i)) is None:
                return False
            if getattr(self, "s3b_q" + str(i)) != 0:
                abuse = True
                if getattr(self, "s3b_q" + str(i) + "_frequency") is None:
                    return False
        if abuse and self.s3b_age_began is None:
            return False
        return True

    def complete_3c(self) -> bool:
        return self.all_fields_not_none([
            "s3c_q1",
            "s3c_q2",
            "s3c_q3",
            "s3c_q4",
            "s3c_q5",
            "s3c_q6",
            "s3c_q7",
            "s3c_q8",
            "s3c_q9",
            "s3c_q10",
            "s3c_q11",
            "s3c_q12",
            "s3c_q13",
            "s3c_q14",
            "s3c_q15",
            "s3c_q16",
            "s3c_q17",
            "s3c_which_parent_cared_for",
            "s3c_parent_mental_problem",
            "s3c_parent_physical_problem"
        ])

    def complete_4a(self) -> bool:
        if self.s4a_adultconfidant is None:
            return False
        if not self.s4a_adultconfidant:
            return True
        if not self.is_at_least_one_field_true([
            "s4a_adultconfidant_mother",
            "s4a_adultconfidant_father",
            "s4a_adultconfidant_otherrelative",
            "s4a_adultconfidant_familyfriend",
            "s4a_adultconfidant_responsibleadult",
            "s4a_adultconfidant_other"
        ]):
            return False
        if self.s4a_adultconfidant_other \
                and not self.s4a_adultconfidant_other_detail:
            return False
        return True

    def complete_4b(self) -> bool:
        if self.s4b_childconfidant is None:
            return False
        if not self.s4b_childconfidant:
            return True
        if not self.is_at_least_one_field_true([
            "s4b_childconfidant_sister",
            "s4b_childconfidant_brother",
            "s4b_childconfidant_otherrelative",
            "s4b_childconfidant_closefriend",
            "s4b_childconfidant_otherfriend",
            "s4b_childconfidant_other"
        ]):
            return False
        if self.s4b_childconfidant_other \
                and not self.s4b_childconfidant_other_detail:
            return False
        return True

    def complete_4c(self) -> bool:
        n = 0
        if self.s4c_closest_mother:
            n += 1
        if self.s4c_closest_father:
            n += 1
        if self.s4c_closest_sibling:
            n += 1
        if self.s4c_closest_otherrelative:
            n += 1
        if self.s4c_closest_adultfriend:
            n += 1
        if self.s4c_closest_childfriend:
            n += 1
        if self.s4c_closest_other:
            n += 1
        if n < 2:
            return False
        if self.s4c_closest_other and not self.s4c_closest_other_detail:
            return False
        return True

    def complete_5(self) -> bool:
        if self.s5c_physicalabuse is None:
            return False
        if self.s5c_physicalabuse == 0:
            return True
        if (self.s5c_abused_by_mother is None or
                self.s5c_abused_by_father is None or
                self.s5c_abuse_by_nonparent is None):
            return False
        if self.s5c_abused_by_mother:
            if self.any_fields_none(["s5c_mother_abuse_age_began",
                                     "s5c_mother_hit_more_than_once",
                                     "s5c_mother_hit_how",
                                     "s5c_mother_injured",
                                     "s5c_mother_out_of_control"]):
                return False
        if self.s5c_abused_by_father:
            if self.any_fields_none(["s5c_father_abuse_age_began",
                                     "s5c_father_hit_more_than_once",
                                     "s5c_father_hit_how",
                                     "s5c_father_injured",
                                     "s5c_father_out_of_control"]):
                return False
        if (self.s5c_abuse_by_nonparent and
                not self.s5c_nonparent_abuse_description):
            return False
        return True

    def complete_6(self) -> bool:
        if (self.s6_any_unwanted_sexual_experience is None or
                self.s6_unwanted_intercourse is None or
                self.s6_upsetting_sexual_adult_authority is None):
            return False
        if (self.s6_any_unwanted_sexual_experience == 0 and
                self.s6_unwanted_intercourse == 0 and
                self.s6_upsetting_sexual_adult_authority == 0):
            return True
        if self.any_fields_none(["s6_first_age",
                                 "s6_first_person_known",
                                 "s6_first_relative",
                                 "s6_first_in_household",
                                 "s6_first_more_than_once",
                                 "s6_first_touch_privates_subject",
                                 "s6_first_touch_privates_other",
                                 "s6_first_intercourse"]):
            return False
        # no checks for "other experience"
        return True

    # -------------------------------------------------------------------------
    # Scoring
    # -------------------------------------------------------------------------

    def total_sum_abort_if_none(self, fields: List[str]) -> Optional[int]:
        total = 0
        for field in fields:
            value = getattr(self, field)
            if value is None:
                return None
            total += value
        return total

    def total_nonzero_scores_1_abort_if_none(self, fields: List[str]) \
            -> Optional[int]:
        total = 0
        for field in fields:
            value = getattr(self, field)
            if value is None:
                return None
            if value:
                total += 1
        return total

    def parental_loss_risk(self) -> bool:
        return bool(
            self.s1c_mother_died or
            self.s1c_father_died or
            self.s1c_separated_from_mother or
            self.s1c_separated_from_father
        )

    def parental_loss_high_risk(self) -> bool:
        return bool(
            self.s1c_separated_from_mother and (
                self.s1c_mother_separation_reason == 5 or
                self.s1c_mother_separation_reason == 6
            ) or
            self.s1c_separated_from_father and (
                self.s1c_father_separation_reason == 5 or
                self.s1c_father_separation_reason == 6
            )
        )

    def mother_antipathy(self) -> Optional[int]:
        if self.s2a_which_mother_figure == 0:
            return None
        total = 0
        for i in [1, 4, 6, 8, 9, 10, 11, 16]:
            score = getattr(self, "s2a_q" + str(i))
            if i == 16 and score is None:
                # Q16 is allowed to be blank (if no siblings)
                score = 0
            if score is None:
                return None
            if i in [8, 11]:
                score = 6 - score  # reverse
            total += score
        return total

    def father_antipathy(self) -> Optional[int]:
        if self.s3a_which_father_figure == 0:
            return None
        total = 0
        for i in [1, 4, 6, 8, 9, 10, 11, 16]:
            score = getattr(self, "s3a_q" + str(i))
            if i == 16 and score is None:
                # Q16 is allowed to be blank (if no siblings)
                score = 0
            if score is None:
                return None
            if i in [8, 11]:
                score = 6 - score  # reverse
            total += score
        return total

    def mother_neglect(self) -> Optional[int]:
        if self.s2a_which_mother_figure == 0:
            return None
        total = 0
        for i in [2, 3, 5, 7, 12, 13, 14, 15]:
            score = getattr(self, "s2a_q" + str(i))
            if score is None:
                return None
            if i in [2, 3, 5, 12, 13, 14]:
                score = 6 - score  # reverse
            total += score
        return total

    def father_neglect(self) -> Optional[int]:
        if self.s3a_which_father_figure == 0:
            return None
        total = 0
        for i in [2, 3, 5, 7, 12, 13, 14, 15]:
            score = getattr(self, "s3a_q" + str(i))
            if score is None:
                return None
            if i in [2, 3, 5, 12, 13, 14]:
                score = 6 - score  # reverse
            total += score
        return total

    def mother_psychological_abuse(self) -> Optional[int]:
        if self.s2a_which_mother_figure == 0:
            return None
        total = 0
        for i in range(1, 18):
            score = getattr(self, "s2b_q" + str(i))
            if score is None:
                return None
            total += score
            freqscore = getattr(self, "s2b_q" + str(i) + "_frequency")
            if score != 0 and freqscore is None:
                return None
            if freqscore is not None:
                total += freqscore
        return total

    def father_psychological_abuse(self) -> Optional[int]:
        if self.s3a_which_father_figure == 0:
            return None
        total = 0
        for i in range(1, 18):
            score = getattr(self, "s3b_q" + str(i))
            if score is None:
                return None
            total += score
            freqscore = getattr(self, "s3b_q" + str(i) + "_frequency")
            if score != 0 and freqscore is None:
                return None
            if freqscore is not None:
                total += freqscore
        return total

    def role_reversal(self) -> Optional[int]:
        total = 0
        for i in range(1, 18):
            score = getattr(self, "s3c_q" + str(i))
            if score is None:
                return None
            total += score
        return total

    def physical_abuse_screen(self) -> Optional[int]:
        fields = [
            "s5c_physicalabuse"
        ]
        return self.total_nonzero_scores_1_abort_if_none(fields)

    def physical_abuse_severity_mother(self) -> Optional[int]:
        if self.physical_abuse_screen() == 0:
            return 0
        if self.s5c_abused_by_mother == 0:
            return 0
        mainfields = [
            "s5c_mother_hit_more_than_once",
            "s5c_mother_injured",
            "s5c_mother_out_of_control"
        ]
        total = self.total_nonzero_scores_1_abort_if_none(mainfields)
        if total is None:
            return None
        if self.s5c_mother_hit_how is None:
            return None
        if self.s5c_mother_hit_how == 1 or self.s5c_mother_hit_how == 2:
            total += 1
        return total

    def physical_abuse_severity_father(self) -> Optional[int]:
        if self.physical_abuse_screen() == 0:
            return 0
        if self.s5c_abused_by_father == 0:
            return 0
        mainfields = [
            "s5c_father_hit_more_than_once",
            "s5c_father_injured",
            "s5c_father_out_of_control"
        ]
        total = self.total_nonzero_scores_1_abort_if_none(mainfields)
        if total is None:
            return None
        if self.s5c_father_hit_how is None:
            return None
        if self.s5c_father_hit_how == 1 or self.s5c_father_hit_how == 2:
            total += 1
        return total

    def sexual_abuse_screen(self) -> Optional[int]:
        fields = [
            "s6_any_unwanted_sexual_experience",
            "s6_unwanted_intercourse",
            "s6_upsetting_sexual_adult_authority"
        ]
        return self.total_nonzero_scores_1_abort_if_none(fields)

    def sexual_abuse_score_first(self) -> Optional[int]:
        if self.sexual_abuse_screen() == 0:
            return 0
        fields = [
            "s6_first_person_known",
            "s6_first_relative",
            "s6_first_in_household",
            "s6_first_more_than_once",
            "s6_first_touch_privates_subject",
            "s6_first_touch_privates_other",
            "s6_first_intercourse"
        ]
        return self.total_nonzero_scores_1_abort_if_none(fields)

    def sexual_abuse_score_other(self) -> Optional[int]:
        if self.sexual_abuse_screen() == 0:
            return 0
        fields = [
            "s6_other_person_known",
            "s6_other_relative",
            "s6_other_in_household",
            "s6_other_more_than_once",
            "s6_other_touch_privates_subject",
            "s6_other_touch_privates_other",
            "s6_other_intercourse"
        ]
        return self.total_nonzero_scores_1_abort_if_none(fields)

    # -------------------------------------------------------------------------
    # HTML
    # -------------------------------------------------------------------------

    def get_task_html(self, req: CamcopsRequest) -> str:

        def wxstring(wstringname: str) -> str:
            return self.wxstring(req, wstringname)

        def subheading_from_wstring(wstringname: str) -> str:
            return subheading_from_string(self.wxstring(req, wstringname))

        def subsubheading_from_wstring(wstringname: str) -> str:
            return subsubheading_from_string(self.wxstring(req, wstringname))

        def wstring_boolean(wstring: str, value: Any) -> str:
            return string_boolean_(req, self.wxstring(req, wstring), value)

        def wstring_numeric(wstring: str, value: Any) -> str:
            return string_numeric(self.wxstring(req, wstring), value)

        def wstring_string(wstring: str, value: str) -> str:
            return string_string(self.wxstring(req, wstring), value)

        def wstring_dict(wstring: str, value: Any, d: Dict) -> str:
            return string_dict(self.wxstring(req, wstring), value, d)

        def string_boolean(string: str, value: Any) -> str:
            return string_boolean_(req, string, value)

        separation_map = {
            None: None,
            1: "1 — " + wxstring("1c_separation_reason1"),
            2: "2 — " + wxstring("1c_separation_reason2"),
            3: "3 — " + wxstring("1c_separation_reason3"),
            4: "4 — " + wxstring("1c_separation_reason4"),
            5: "5 — " + wxstring("1c_separation_reason5"),
            6: "6 — " + wxstring("1c_separation_reason6"),
        }
        motherfigure_map = {
            None: None,
            0: "0 — " + wxstring("2a_which_option0"),
            1: "1 — " + wxstring("2a_which_option1"),
            2: "2 — " + wxstring("2a_which_option2"),
            3: "3 — " + wxstring("2a_which_option3"),
            4: "4 — " + wxstring("2a_which_option4"),
            5: "5 — " + wxstring("2a_which_option5"),
        }
        fatherfigure_map = {
            None: None,
            0: "0 — " + wxstring("3a_which_option0"),
            1: "1 — " + wxstring("3a_which_option1"),
            2: "2 — " + wxstring("3a_which_option2"),
            3: "3 — " + wxstring("3a_which_option3"),
            4: "4 — " + wxstring("3a_which_option4"),
            5: "5 — " + wxstring("3a_which_option5"),
        }
        no_yes_5way_map = {
            None: None,
            1: "1 — " + wxstring("options5way_notoyes_1"),
            2: "2 — (between not-at-all and unsure)",
            3: "3 — " + wxstring("options5way_notoyes_3"),
            4: "4 — (between unsure and yes-definitely)",
            5: "5 — " + wxstring("options5way_notoyes_5"),
        }
        no_yes_3way_map = {
            None: None,
            0: "0 — " + wxstring("options3way_noto_yes_0"),
            1: "1 — " + wxstring("options3way_noto_yes_1"),
            2: "2 — " + wxstring("options3way_noto_yes_2"),
        }
        frequency_map = {
            None: None,
            0: "0 — " + wxstring("optionsfrequency0"),
            1: "1 — " + wxstring("optionsfrequency1"),
            2: "2 — " + wxstring("optionsfrequency2"),
            3: "3 — " + wxstring("optionsfrequency3"),
        }
        parent_cared_for_map = {
            None: None,
            0: "0 — " + wxstring("3c_whichparentcaredfor_option0"),
            1: "1 — " + wxstring("3c_whichparentcaredfor_option1"),
            2: "2 — " + wxstring("3c_whichparentcaredfor_option2"),
            3: "3 — " + wxstring("3c_whichparentcaredfor_option3"),
            4: "4 — " + wxstring("3c_whichparentcaredfor_option4"),
        }
        hitting_map = {
            None: None,
            1: "1 — " + wxstring("5_hit_option_1"),
            2: "2 — " + wxstring("5_hit_option_2"),
            3: "3 — " + wxstring("5_hit_option_3"),
            4: "4 — " + wxstring("5_hit_option_4"),
        }
        html = (
            f"""
                <div class="{CssClass.SUMMARY}">
                    <table class="{CssClass.SUMMARY}">
            """ +
            self.get_is_complete_tr(req) +
            tr_qa("Parental loss risk factor? <sup>[1]</sup>",
                  get_yes_no(req, self.parental_loss_risk())) +
            tr_qa("Parental loss higher risk factor? <sup>[2]</sup>",
                  get_yes_no(req, self.parental_loss_high_risk())) +
            tr_qa("Maternal antipathy score (8–40) <sup>[3]</sup>",
                  self.mother_antipathy()) +
            tr_qa("Maternal neglect score (8–40) <sup>[3]</sup>",
                  self.mother_neglect()) +
            tr_qa("Maternal psychological abuse score (0–85) <sup>[4]</sup>",
                  self.mother_psychological_abuse()) +
            tr_qa("Paternal antipathy score (8–40) <sup>[3]</sup>",
                  self.father_antipathy()) +
            tr_qa("Paternal neglect score (8–40) <sup>[3]</sup>",
                  self.father_neglect()) +
            tr_qa("Paternal psychological abuse score (0–85) <sup>[4]</sup>",
                  self.father_psychological_abuse()) +
            tr_qa("Role reversal score (17–85) <sup>[5]</sup>",
                  self.role_reversal()) +
            tr_qa("Physical abuse screen (0–1) <sup>[6]</sup>",
                  self.physical_abuse_screen()) +
            tr_qa("Maternal physical abuse severity score (0–4) "
                  "<sup>[6]</sup>",
                  self.physical_abuse_severity_mother()) +
            tr_qa("Paternal physical abuse severity score (0–4) "
                  "<sup>[6]</sup>",
                  self.physical_abuse_severity_father()) +
            tr_qa("Sexual abuse screen (0–3) <sup>[7]</sup>",
                  self.sexual_abuse_screen()) +
            tr_qa("First sexual abuse severity score (0–7) <sup>[7]</sup>",
                  self.sexual_abuse_score_first()) +
            tr_qa("Other sexual abuse severity score (0–7) <sup>[7]</sup>",
                  self.sexual_abuse_score_other()) +
            f"""
                    </table>
                </div>
                <table class="{CssClass.TASKDETAIL}">
            """ +

            subheading_spanning_two_columns("1A: " +
                                            wxstring("1a_q")) +
            subsubheading_from_wstring("1a_motherfigures") +
            wstring_boolean("1a_mf_birthmother",
                            self.s1a_motherfigure_birthmother) +
            wstring_boolean("1a_mf_stepmother",
                            self.s1a_motherfigure_stepmother) +
            wstring_boolean("1a_mf_femalerelative",
                            self.s1a_motherfigure_femalerelative) +
            string_string("(Female relative details)",
                          self.s1a_motherfigure_femalerelative_detail) +
            wstring_boolean("1a_mf_familyfriend",
                            self.s1a_motherfigure_familyfriend) +
            wstring_boolean("1a_mf_fostermother",
                            self.s1a_motherfigure_fostermother) +
            wstring_boolean("1a_mf_adoptivemother",
                            self.s1a_motherfigure_adoptivemother) +
            wstring_boolean("other", self.s1a_motherfigure_other) +
            string_string("(Other, details)",
                          self.s1a_motherfigure_other_detail) +

            subsubheading_from_wstring("1a_fatherfigures") +
            wstring_boolean("1a_ff_birthfather",
                            self.s1a_fatherfigure_birthfather) +
            wstring_boolean("1a_ff_stepfather",
                            self.s1a_fatherfigure_stepfather) +
            wstring_boolean("1a_ff_malerelative",
                            self.s1a_fatherfigure_malerelative) +
            string_string("(Male relative details)",
                          self.s1a_fatherfigure_malerelative_detail) +
            wstring_boolean("1a_ff_familyfriend",
                            self.s1a_fatherfigure_familyfriend) +
            wstring_boolean("1a_ff_fosterfather",
                            self.s1a_fatherfigure_fosterfather) +
            wstring_boolean("1a_ff_adoptivefather",
                            self.s1a_fatherfigure_adoptivefather) +
            wstring_boolean("other", self.s1a_fatherfigure_other) +
            string_string("(Other, details)",
                          self.s1a_fatherfigure_other_detail) +

            subheading_from_string("1B: " + wxstring("1b_q")) +
            wstring_boolean("1b_q", self.s1b_institution) +
            wstring_numeric("1b_q_how_long", self.s1b_institution_time_years) +

            subheading_from_string("1C: " + wxstring("1c_heading")) +
            subsubheading_from_wstring("mother") +

            string_boolean("Mother died before age 17",
                           self.s1c_mother_died) +
            wstring_numeric("1c_parentdiedage",
                            self.s1c_mother_died_subject_aged) +
            string_boolean("Separated from mother for >1y",
                           self.s1c_separated_from_mother) +
            wstring_numeric("1c_age_first_separated",
                            self.s1c_first_separated_from_mother_aged) +
            wstring_numeric(
                "1c_how_long_separation",
                self.s1c_mother_how_long_first_separation_years) +
            wstring_dict("1c_separation_reason",
                         self.s1c_mother_separation_reason,
                         separation_map) +

            subsubheading_from_wstring("father") +
            string_boolean("Father died before age 17", self.s1c_father_died) +
            wstring_numeric("1c_parentdiedage",
                            self.s1c_father_died_subject_aged) +
            string_boolean("Separated from father for >1y",
                           self.s1c_separated_from_father) +
            wstring_numeric("1c_age_first_separated",
                            self.s1c_first_separated_from_father_aged) +
            wstring_numeric(
                "1c_how_long_separation",
                self.s1c_father_how_long_first_separation_years) +
            wstring_dict("1c_separation_reason",
                         self.s1c_father_separation_reason,
                         separation_map) +
            wstring_string("please_describe_experience",
                           self.s1c_describe_experience) +

            subheading_from_string("2A: " + wxstring("2a_heading")) +
            wstring_dict("2a_which",
                         self.s2a_which_mother_figure, motherfigure_map) +
            wstring_string("rnc_if_other_describe",
                           self.s2a_which_mother_figure_other_detail)
        )
        for i in range(1, 17):
            html += string_dict(str(i) + ". " + wxstring("2a_q" + str(i)),
                                getattr(self, "s2a_q" + str(i)),
                                no_yes_5way_map)
        html += (
            wstring_string("2a_add_anything", self.s2a_extra) +
            subheading_from_string("2B: " + wxstring("2b_heading"))
        )
        for i in range(1, 18):
            html += tr(
                str(i) + ". " + wxstring("2b_q" + str(i)),
                answer(get_from_dict(no_yes_3way_map,
                                     getattr(self, "s2b_q" + str(i)))) +
                " (" +
                answer(get_from_dict(
                    frequency_map,
                    getattr(self, "s2b_q" + str(i) + "_frequency"))) +
                ")"
            )
        html += (
            wstring_boolean("if_any_what_age", self.s2b_age_began) +
            wstring_string("is_there_more_you_want_to_say", self.s2b_extra) +

            subheading_from_string("3A: " + wxstring("3a_heading")) +
            wstring_dict("2a_which",
                         self.s3a_which_father_figure, fatherfigure_map) +
            wstring_string("rnc_if_other_describe",
                           self.s3a_which_father_figure_other_detail)
        )
        for i in range(1, 17):
            html += string_dict(
                str(i) + ". " + wxstring("3a_q" + str(i)),
                getattr(self, "s3a_q" + str(i)), no_yes_5way_map)
        html += (
            wstring_string("3a_add_anything", self.s3a_extra) +
            subheading_from_string("3B: " + wxstring("3b_heading"))
        )
        for i in range(1, 18):
            html += tr(
                str(i) + ". " + wxstring("3b_q" + str(i)),
                answer(get_from_dict(no_yes_3way_map,
                                     getattr(self, "s3b_q" + str(i)))) +
                " (" +
                answer(get_from_dict(
                    frequency_map,
                    getattr(self, "s3b_q" + str(i) + "_frequency"))) +
                ")"
            )
        html += (
            wstring_boolean("if_any_what_age", self.s3b_age_began) +
            wstring_string("is_there_more_you_want_to_say", self.s3b_extra) +
            subheading_from_string("3C: " + wxstring("3c_heading"))
        )
        for i in range(1, 18):
            html += string_dict(
                str(i) + ". " + wxstring("3c_q" + str(i)),
                getattr(self, "s3c_q" + str(i)), no_yes_5way_map)
        html += (
            wstring_dict("3c_which_parent_cared_for",
                         self.s3c_which_parent_cared_for,
                         parent_cared_for_map) +
            wstring_boolean("3c_parent_mental_problem",
                            self.s3c_parent_mental_problem) +
            wstring_boolean("3c_parent_physical_problem",
                            self.s3c_parent_physical_problem) +

            subheading_from_string("4: " + wxstring("4_heading")) +
            subsubheading_from_string("(Adult confidant)") +
            wstring_boolean("4a_q", self.s4a_adultconfidant) +
            subsubheading_from_wstring("4_if_so_who") +
            wstring_boolean("4a_option_mother",
                            self.s4a_adultconfidant_mother) +
            wstring_boolean("4a_option_father",
                            self.s4a_adultconfidant_father) +
            wstring_boolean("4a_option_relative",
                            self.s4a_adultconfidant_otherrelative) +
            wstring_boolean("4a_option_friend",
                            self.s4a_adultconfidant_familyfriend) +
            wstring_boolean("4a_option_responsibleadult",
                            self.s4a_adultconfidant_responsibleadult) +
            wstring_boolean("4a_option_other",
                            self.s4a_adultconfidant_other) +
            string_string("(Other, details)",
                          self.s4a_adultconfidant_other_detail) +
            wstring_string("4_note_anything",
                           self.s4a_adultconfidant_additional) +
            subsubheading_from_string("(Child confidant)") +
            wstring_boolean("4b_q", self.s4b_childconfidant) +
            subsubheading_from_wstring("4_if_so_who") +
            wstring_boolean("4b_option_sister",
                            self.s4b_childconfidant_sister) +
            wstring_boolean("4b_option_brother",
                            self.s4b_childconfidant_brother) +
            wstring_boolean("4b_option_relative",
                            self.s4b_childconfidant_otherrelative) +
            wstring_boolean("4b_option_closefriend",
                            self.s4b_childconfidant_closefriend) +
            wstring_boolean("4b_option_otherfriend",
                            self.s4b_childconfidant_otherfriend) +
            wstring_boolean("4b_option_other",
                            self.s4b_childconfidant_other) +
            string_string("(Other, details)",
                          self.s4b_childconfidant_other_detail) +
            wstring_string("4_note_anything",
                           self.s4b_childconfidant_additional) +
            subsubheading_from_wstring("4c_q") +
            wstring_boolean("4c_option_mother",
                            self.s4c_closest_mother) +
            wstring_boolean("4c_option_father",
                            self.s4c_closest_father) +
            string_boolean("4c_option_sibling",
                           self.s4c_closest_sibling) +
            wstring_boolean("4c_option_relative",
                            self.s4c_closest_otherrelative) +
            wstring_boolean("4c_option_adultfriend",
                            self.s4c_closest_adultfriend) +
            wstring_boolean("4c_option_youngfriend",
                            self.s4c_closest_childfriend) +
            wstring_boolean("4c_option_other", self.s4c_closest_other) +
            string_string("(Other, details)", self.s4c_closest_other_detail) +
            wstring_string("4_note_anything", self.s4c_closest_additional) +

            subheading_from_string("4: " + wxstring("5_heading")) +
            wstring_boolean("5_mainq", self.s5c_physicalabuse) +
            subsubheading_from_wstring("5_motherfigure") +
            wstring_boolean("5_did_this_person_hurt_you",
                            self.s5c_abused_by_mother) +
            wstring_numeric("5_how_old",
                            self.s5c_mother_abuse_age_began) +
            wstring_boolean("5_hit_more_than_once",
                            self.s5c_mother_hit_more_than_once) +
            wstring_dict("5_how_hit", self.s5c_mother_hit_how, hitting_map) +
            wstring_boolean("5_injured", self.s5c_mother_injured) +
            wstring_boolean("5_outofcontrol", self.s5c_mother_out_of_control) +
            subsubheading_from_wstring("5_fatherfigure") +
            wstring_boolean("5_did_this_person_hurt_you",
                            self.s5c_abused_by_father) +
            wstring_numeric("5_how_old", self.s5c_father_abuse_age_began) +
            wstring_boolean("5_hit_more_than_once",
                            self.s5c_father_hit_more_than_once) +
            wstring_dict("5_how_hit", self.s5c_father_hit_how, hitting_map) +
            wstring_boolean("5_injured", self.s5c_father_injured) +
            wstring_boolean("5_outofcontrol", self.s5c_father_out_of_control) +
            wstring_string("5_can_you_describe_1",
                           self.s5c_parental_abuse_description) +
            subsubheading_from_string("(Other in household)") +
            wstring_boolean("5_anyone_else", self.s5c_abuse_by_nonparent) +
            wstring_string("5_can_you_describe_2",
                           self.s5c_nonparent_abuse_description) +

            subheading_from_string("6: " + wxstring("6_heading")) +
            wstring_dict("6_any_unwanted",
                         self.s6_any_unwanted_sexual_experience,
                         no_yes_3way_map) +
            wstring_dict("6_intercourse",
                         self.s6_unwanted_intercourse, no_yes_3way_map) +
            wstring_dict("6_upset_adult_authority",
                         self.s6_upsetting_sexual_adult_authority,
                         no_yes_3way_map) +

            subsubheading_from_wstring("6_first_experience") +
            wstring_numeric("6_q1", self.s6_first_age) +
            wstring_boolean("6_q2", self.s6_first_person_known) +
            wstring_boolean("6_q3", self.s6_first_relative) +
            wstring_boolean("6_q4", self.s6_first_in_household) +
            wstring_boolean("6_q5", self.s6_first_more_than_once) +
            wstring_boolean("6_q6", self.s6_first_touch_privates_subject) +
            wstring_boolean("6_q7", self.s6_first_touch_privates_other) +
            wstring_boolean("6_q8", self.s6_first_intercourse) +

            subsubheading_from_wstring("6_other_experience") +
            wstring_numeric("6_q1", self.s6_other_age) +
            wstring_boolean("6_q2", self.s6_other_person_known) +
            wstring_boolean("6_q3", self.s6_other_relative) +
            wstring_boolean("6_q4", self.s6_other_in_household) +
            wstring_boolean("6_q5", self.s6_other_more_than_once) +
            wstring_boolean("6_q6", self.s6_other_touch_privates_subject) +
            wstring_boolean("6_q7", self.s6_other_touch_privates_other) +
            wstring_boolean("6_q8", self.s6_other_intercourse) +
            wstring_string("5_can_you_describe_1",
                           self.s6_unwanted_sexual_description) +

            subheading_from_wstring("any_other_comments") +
            wstring_string("any_other_comments", self.any_other_comments) +

            f"""
                </table>
                <div class="{CssClass.FOOTNOTES}">
                    [1] Death of mother/father before age 17 or continuous
                        separation of ≥1 year.
                    [2] Reason for loss ‘abandonment’ or ‘other’.
                    [3] Section 2A and 3A contain antipathy and neglect scales.
                        Antipathy scales: reverse items 8, 11;
                        then sum 1, 4, 6, 8–11, 16.
                        Neglect scales: reverse items 2, 3, 5, 12–14;
                        then sum 2, 3, 5, 7, 12–15.
                    [4] Psychological abuse scores (sections 2B, 3B):
                        sum of items 1–17 (yes=2, unsure=1, no=0) with
                        frequencies (never=0, once=1, rarely=2, often=3).
                        Any score &gt;1 is a risk indicator.
                    [5] Role reversal: sum of scores from section 3C.
                    [6] Physical abuse (section 4): first question (screen
                        item) is scored 0/1. For each parent, score 1 for
                        {{&gt;1 occasion, {{belt/stick/punch/kick}}, injured,
                        out-of-control}}.
                    [7] Sexual abuse (section 6): no=0, unsure=1, yes=1.
                    First three questions are the screen.
                </div>
            """
        )
        return html


# =============================================================================
# More helper functions
# =============================================================================

def subheading_from_string(s: str) -> str:
    return subheading_spanning_two_columns(s)


def subsubheading_from_string(s: str) -> str:
    return f'<tr><td></td><td class="{CssClass.SUBHEADING}">{s}</td></tr>'


def row(label: str, value: Any, default: str = "") -> str:
    return tr_qa(label, value, default=default)


def string_boolean_(req: CamcopsRequest, string: str, value: Any) -> str:
    return row(string, get_yes_no_none(req, value))


def string_numeric(string: str, value: Any) -> str:
    return row(string, value)


def string_string(string: str, value: str) -> str:
    return row(string, ws.webify(value))


def string_dict(string: str, value: Any, d: Dict) -> str:
    return row(string, get_from_dict(d, value))
