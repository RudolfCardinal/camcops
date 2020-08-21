..  docs/source/tasks/das28.rst

..  Copyright (C) 2012-2020 Rudolf Cardinal (rudolf@pobox.com).
    .
    This file is part of CamCOPS.
    .
    CamCOPS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    .
    CamCOPS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
    .
    You should have received a copy of the GNU General Public License
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.


.. _das28:

Disease Activity Score-28 (DAS28)
---------------------------------

.. include:: include_data_collection_plus_local_upgrade.rst

Disease activity score for rheumatoid arthritis. The 28 refers to the number
of joints in which disease activity is rated (bilaterally: shoulder, elbow,
wrist, knee, 5 × MCP, 5 × PIP).

For each joint, "swollen?" and "tender?" are asked; there is a visual analogue
rating scale (100 mm in length) for arthritis activity, plus an inflammatory
marker. In the original DAS28 (sometimes termed the DAS28-ESR), this was ESR
(erythrocyte sedimentation rate). An alternative is the CRP (C-reactive
protein), in the DAS28-CRP.

- t28 = number of tender joints, range [0, 28].

- sw28 = number of swollen joints, range [0, 28].

- The CRP units are mg/L and in the range [0, 300].

- The ESR units are mm/hr and in the range [1, 300].

- The VAS (visual analogue scale) units are mm, range [0, 100]. Low is inactive
  arthritis; high is extremely active arthritis.

DAS28 score = 0.56 × √(t28) + 0.28 × √(sw28) + 0.70 × ln(ESR) + 0.014 × VAS

DAS28-CRP score = 0.56 × √(t28) + 0.28 × √(sw28) + 0.36 × ln(CRP + 1) + 0.014 ×
VAS + 0.96.

These are the "4-variable" DAS scales (tenderness, swelling, inflammatory
marker, VAS). There are different 3-variables scales too, without the VAS, not
implemented here.


Intellectual property rights
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- Copyright © 1995, Prof. dr. Piet L.C.M. van Riel (as per
  https://eprovide.mapi-trust.org/instruments/disease-activity-score-c-reactive-protein#contact_and_conditions_of_use).


History
~~~~~~~

- Prevoo ML, van 't Hof MA, Kuper HH, van Leeuwen MA, van de Putte LB, van
  Riel PL (1995).
  Modified disease activity scores that include twenty-eight-joint counts.
  Development and validation in a prospective longitudinal study of patients
  with rheumatoid arthritis.
  *Arthritis Rheum.* 38: 44-8.
  https://www.ncbi.nlm.nih.gov/pubmed/7818570

  [Original version using ESR.]

- Fransen J, Welsing PMJ, de Keijzer RMH, van Riel PLCM (2003).
  Disease activity scores using C-reactive protein: CRP may replace ESR in the
  assessment of RA disease activity.
  *Ann Rheum Dis* 62(Suppl. 1): 151.
  https://www.researchgate.net/publication/291893932_Disease_activity_scores_using_C-reactive_protein_CRP_may_replace_ESR_in_the_assessment_of_RA_disease_activity.
  (That says 2004, but vol. 62 supplement 2 was in 2003 according to
  https://www.ncbi.nlm.nih.gov/pmc/journals/149/ so 2003 probably correct.)

  [DAS-28-CRP source, according to Hensor et al. 2010 as below.]

- Fleischmann RM, van der Heijde D, Gardiner PV, Szumski A, Marshall L, Bananis
  E (2017).
  DAS28-CRP and DAS28-ESR cut-offs for high disease activity in rheumatoid
  arthritis are not interchangeable.
  *RMD Open* 3: e000382.
  https://doi.org/10.1136/rmdopen-2016-000382.
  https://www.ncbi.nlm.nih.gov/pubmed/28255449.

  [Helpful review comparing DAS28-ESR and DAS28-CRP.]

- Hensor EM, Emery P, Bingham SJ, Conaghan PG; YEAR Consortium (2010).
  Discrepancies in categorizing rheumatoid arthritis patients by DAS-28(ESR)
  and DAS-28(CRP): can they be reduced?
  *Rheumatology (Oxford)* 49: 1521-9.
  https://doi.org/10.1093/rheumatology/keq117.
  https://www.ncbi.nlm.nih.gov/pubmed/20435650.

  [Useful history.]


Source
~~~~~~

- https://eprovide.mapi-trust.org/instruments/disease-activity-score-c-reactive-protein#contact_and_conditions_of_use

- https://www.das28.nl/

  ... the Excel file ``dasculators.xls`` (created 2003-01-13) provides the
  following rules:

  - Tender joint count range [0, 28].
  - Swollen joint count range [0, 28].
  - CRP integer only; CRP in range [1, 300] according to the warning when you
    enter invalid values, but actually [0, 300] accepted.
  - ESR integer only; ESR in range [1, 300].
  - VAS general health integer only; range [0, 100].

- DAS-ESR cutoffs as per:

  Anderson J, Caplan L, Yazdany J, Robbins ML, Neogi T, Michaud K, Saag KG,
  O'Dell JR, Kazi S (2012).
  Rheumatoid arthritis disease activity measures: American College of
  Rheumatology recommendations for use in clinical practice.
  *Arthritis Care Res* 64: 640-7.
  https://www.ncbi.nlm.nih.gov/pubmed/22473918;
  https://doi.org/10.1002/acr.21649;
  https://onlinelibrary.wiley.com/doi/full/10.1002/acr.21649.

- DAS-CRP cutoffs as per:

  Fleischmann RM, van der Heijde D, Gardiner PV, Szumski A, Marshall L, Bananis
  E (2017).
  DAS28-CRP and DAS28-ESR cut-offs for high disease activity in rheumatoid
  arthritis are not interchangeable.
  *RMD Open* 3: e000382.
  https://www.ncbi.nlm.nih.gov/pubmed/28255449;
  https://doi.org/10.1136/rmdopen-2016-000382;
  https://rmdopen.bmj.com/content/3/1/e000382.
