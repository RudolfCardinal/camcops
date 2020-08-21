..  docs/source/tasks/fast.rst

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

.. _fast:

Fast Alcohol Screening Test (FAST)
----------------------------------

History and guide
~~~~~~~~~~~~~~~~~

Original: Hodgson et al. (2002) [#hodgson2002]_.



Sources
~~~~~~~

- Hodgson et al. (2002) [#hodgson2002]_.

- NICE [#nicedefunct]_, Public Health England [#phegeneral]_ [#phefast]_.

- The FAST manual [#fastmanual]_.


Notes on the method of administration and scoring
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is slightly trickier than it appears at first glance -- there are a
couple of administration guidelines that differ very slightly, but the scoring
method is consistent.

**Development**

Hodgson et al. [#hodgson2002]_ developed the FAST from the AUDIT, seeking a
quick version. They had the aim that one question serves as a first filter.
AUDIT Q3 was found to be good for that. They modified it to use a threshold of
≥8 units for men. They found that AUDIT Q5, Q8, and Q10 were collectively the
best for a second-stage filter, with minor modifications.

The four FAST questions are therefore approximately as follows:

- FAST Q1: How often ≥6 units (female) or ≥8 units (male) on a single
  occasion in the last year? = AUDIT Q3, approximately, but not quite.
- FAST Q2: ... failed to do what was normally expected... = AUDIT Q5.
- FAST Q3: ... unable to remember... the night before... = AUDIT Q8.
- FAST Q4: ... relative/friend/doctor/health professional... concerned = AUDIT
  Q10.

The test is referred to as a two-stage screening test (e.g. Abstract).

**Administration**

There seems to be a slight difference in administration instructions between
different sources of the test.

The original paper [#hodgson2002]_ says (p62): *"AUDIT Question 3 [= FAST Q1]...
served as the best first filter... If the response is 'never'... then there is
no hazardous use. If the response is 'weekly'/'daily or almost daily'... then
there is probably hazardous use... this one questions classifies... 66%... with
an accuracy of 97%. Only the 34% of patients who responded 'less than monthly'
or 'monthly' to [AUDIT] Question 3 [FAST Q1] need to be asked further
questions"*.

The FAST manual says that *"a response must be obtained for each question... In
the case of self-completion, a staff member... should check that all questions
have been answered"* ([#fastmanual]_, page 6), although it is not necessary to
score all (depending on Q1). There is no suggestion that one should not
*answer* Q2-Q4 if certain answers are given to Q1. The paper version of the
test simply says *"For the following questions, please circle the answer..."*

Public Health England says that *"the test consists of only 4 questions... which
are asked in 2 stages"* [#phegeneral]_. It clarifies that one should *"only
answer the following questions [Q2-Q4] if the answer above [to Q1] is Never
(0), Less than monthly (1) or Monthly (2). Stop here if the answer is Weekly
(3) or Daily (4)"* ([#phefast]_, page 1).

I presume that PHE trimmed the administration procedure a little for speed,
without any change to the scoring methodology (see below).

**Scoring**

Hodgson et al. [#hodgson2002]_ considered two scoring methods (p64):

- Both methods decide "FAST negative" if the response to Q1 is "Never".
- Both methods decide "FAST positive" if the response to Q1 is "weekly" or
  "daily or almost daily".

Then:

- Method 1 (not preferred): FAST negative if (FAST) Q2 and Q3 are "Never" and
  Q4 is "No"; FAST positive for all others (i.e. positive for any hint of a
  problem).

- Method 2 (preferred): all questions are scored 0-4 and the result is FAST
  positive if the total score for all four questions is ≥3.

  - In more detail: questions Q1-Q3 have five options and are scored 0-4;
    question Q4 has three options, scored 0, 2, and 4.

The original paper prefers Method 2 [#hodgson2002]_.
The FAST manual uses Method 2 [#fastmanual]_.
The PHE scoring method is Method 2 [#phefast]_.
So that's clear.

*CamCOPS*

CamCOPS marks all four questions as mandatory, following the FAST manual, and
following the principle of collecting more data (given also some ambiguity) in
a patient-completed questionnaire. Note also that the original validation
involved a questionnaire in which participants saw and answered all four
questions.

Reviewed 2019-11-01. The scoring method was correct; see ``fast.cpp`` in the
:ref:`source code <sourcecode>` and
:func:`camcops_server.tasks.fast.Fast.is_positive`. Explanatory text added
here.


Intellectual property rights
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Believed to be in the public domain.

- Made freely available with encouragement to copy by the UK NHS Health
  Development Agency and the UK National Institute for Clinical Excellence
  [#nicedefunct]_ and subsequently by Public Health England [#phegeneral]_
  [#phefast]_ along with the AUDIT.

- Developed from the :ref:`AUDIT <audit>` (q.v.)


===============================================================================

.. rubric:: Footnotes

..  [#hodgson2002]
    Hodgson R, Alwyn T, John B, Thom B, Smith A (2002).
    The FAST Alcohol Screening Test.
    *Alcohol and Alcoholism* 37: 61-66.
    http://www.ncbi.nlm.nih.gov/pubmed/11825859

..  [#fastmanual]
    Hodgson R, Alwyn T, John B, Smith A, Newcombe R, Morgan C,
    Thom B, Hodgson R, Waller S [2006, by PDF date].
    Fast screening for alcohol problems: manual for the FAST Alcohol Screening
    Test [v3, by PDF document title].
    http://www.dldocs.stir.ac.uk/documents/fastmanual.pdf

..  [#nicedefunct]
    UK National Institute for Health and Care Excellence.
    Document previously at
    http://www.nice.org.uk/niceMedia/documents/manual_fastalcohol.pdf (e.g.
    2013), now gone (2019-11-01). See
    https://web.archive.org/web/2013*/http://www.nice.org.uk/niceMedia/documents/manual_fastalcohol.pdf.

..  [#phegeneral]
    Public Health England (2017).
    Alcohol use screening tests.
    https://www.gov.uk/government/publications/alcohol-use-screening-tests;
    https://www.gov.uk/government/publications/alcohol-use-screening-tests/guidance-on-the-5-alcohol-use-screening-tests;
    https://assets.publishing.service.gov.uk/government/uploads/system/uploads/attachment_data/file/684828/Fast_alcohol_use_screening_test__FAST__.pdf.

..  [#phefast]
    Public Health England (2017).
    Fast alcohol screening test (FAST).
    https://assets.publishing.service.gov.uk/government/uploads/system/uploads/attachment_data/file/684828/Fast_alcohol_use_screening_test__FAST__.pdf
