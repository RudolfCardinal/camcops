#!/usr/bin/env python

r"""
camcops_server/cc_modules/cc_snomed.py

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

**CamCOPS code to support SNOMED-CT.**

Note that the licensing arrangements for SNOMED-CT mean that the actual codes
must be separate (and not part of the CamCOPS code). See the documentation.

Some tests:

.. code-block:: python

    import logging
    from cardinal_pythonlib.logs import main_only_quicksetup_rootlogger
    from camcops_server.cc_modules.cc_request import get_command_line_request
    from camcops_server.cc_modules.cc_snomed import *
    from camcops_server.tasks.phq9 import Phq9
    main_only_quicksetup_rootlogger(level=logging.DEBUG)
    req = get_command_line_request()

    # ---------------------------------------------------------------------
    # Read the XML, etc.
    # ---------------------------------------------------------------------

    print(VALID_SNOMED_LOOKUPS)
    concepts = get_all_snomed_concepts(req.config.snomed_xml_filename)

    # ---------------------------------------------------------------------
    # Test a task, and loading SNOMED data from XML via the CamCOPS config
    # ---------------------------------------------------------------------

    phq9 = Phq9()
    print("\n".join(str(x) for x in phq9.get_snomed_codes(req)))
    phq9.q1 = 2
    phq9.q2 = 2
    phq9.q3 = 2
    phq9.q4 = 2
    phq9.q5 = 2
    phq9.q6 = 2
    phq9.q7 = 2
    phq9.q8 = 2
    phq9.q9 = 2
    phq9.q10 = 2
    print("\n".join(repr(x) for x in phq9.get_snomed_codes(req)))
    print("\n".join(str(x) for x in phq9.get_snomed_codes(req)))

Note diagnostic coding maps:

- https://www.nlm.nih.gov/research/umls/mapping_projects/icd9cm_to_snomedct.html
- https://www.nlm.nih.gov/research/umls/mapping_projects/snomedct_to_icd10cm.html

Other testing:

.. code-block:: bash

    camcops_server dev_cli --verbose

.. code-block:: python

    from camcops_server.cc_modules.cc_snomed import *

    athena_concepts = get_athena_concepts(config.athena_concept_tsv_filename)
    relationships = get_athena_concept_relationships(config.athena_concept_relationship_tsv_filename)
    rel_ids = set(r.relationship_id for r in relationships)
    icd9, icd10 = get_icd9cm_icd10_snomed_concepts(config.athena_concept_tsv_filename, config.athena_concept_relationship_tsv_filename)

    ac = get_athena_concepts(config.athena_concept_tsv_filename, vocabulary_ids=[AthenaVocabularyId.SNOMED], concept_codes=["4303690"])

Regarding temporal coding:

- SNOMED CT has some concepts relating to time (e.g. "time aspect", "single
  point in time", "date of birth", "date of event", "time (property)", "time
  (attribute)"). HOWEVER, note that SNOMED-CT isn't intended for recording
  event dates:

  - https://confluence.ihtsdotools.org/display/DOCRSG/3.5.+Safely+representing+the+context+of+recorded+codes

    "Contextual information that is not represented by SNOMED CT

    Clinical statements that contain SNOMED CT Concept representations will
    be associated with some information which is not intended to be
    represented using SNOMED CT:

    - Dates, times of an activity of recording and activity;
    - Quantitative information including ranges and durations;
    - Identifiers or names of authors, providers of information or other
      parties involved in a recorded activity.
    - SNOMED CT is not intended to represent this information. Appropriate
      constructs in a standardized or proprietary record architecture
      should be used to relate this information to SNOMED CT encoded
      clinical statements."

  - https://confluence.ihtsdotools.org/display/DOCRSG/3.4.+Record+architectures%2C+structures+and+semantics

    "Using SNOMED CT in standard architectures

    ...

    Each health record component has the potential to include:

    - Dates and times of actual and planned events.
    - Associations with people, organizations, devices and other entities
      that participate or are used in relations to a recorded event or
      plan.
    - Codes or other representations that name or provide the semantic
      information container, link, or statement:
      
      - SNOMED CT fulfills this role in a structured health record.

    - Additional data including text, numeric values, images and other
      digital data."

- That explains why the grammar has no explanation of how to represent
  dates/times, perhaps!

"""  # noqa

from collections import OrderedDict
import csv
import logging
from typing import Dict, List, Optional, Set, Tuple, Union
import xml.etree.cElementTree as ElementTree

from cardinal_pythonlib.athena_ohdsi import (
    AthenaConceptRow,
    AthenaRelationshipId,
    AthenaVocabularyId,
    get_athena_concept_relationships,
    get_athena_concepts,
)
from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.reprfunc import simple_repr
# noinspection PyUnresolvedReferences
from cardinal_pythonlib.snomed import (  # noqa: F401
    SnomedAttribute,
    SnomedAttributeGroup,
    SnomedAttributeSet,
    SnomedConcept as SnomedConceptCardinalPythonlib,
    SnomedExpression as SnomedExpressionCardinalPythonlib,
    SnomedFocusConcept,
    SnomedRefinement,
    SnomedValue,
)

from camcops_server.cc_modules.cc_cache import cache_region_static, fkg
from camcops_server.cc_modules.cc_xml import XmlDataTypes, XmlElement

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Constants
# =============================================================================

# -----------------------------------------------------------------------------
# Internal
# -----------------------------------------------------------------------------

SNOMED_XML_NAME = "snomed_ct_expression"

# -----------------------------------------------------------------------------
# ICD-9-CM; from the client: camcops --print_icd9_codes
# -----------------------------------------------------------------------------

CLIENT_ICD9CM_CODES = set("""
290.0 290.1 290.10 290.11 290.12 290.13 290.2 290.20 290.21 290.3 290.4 290.40
290.41 290.42 290.43 290.8 290.9 291.0 291.1 291.2 291.3 291.4 291.5 291.8
291.81 291.82 291.89 291.9 292.0 292.1 292.11 292.12 292.2 292.8 292.81 292.82
292.83 292.84 292.85 292.89 292.9 293.0 293.1 293.8 293.81 293.82 293.83 293.84
293.89 293.9 294.0 294.1 294.10 294.11 294.2 294.20 294.21 294.8 294.9 295.0
295.00 295.01 295.02 295.03 295.04 295.05 295.1 295.10 295.11 295.12 295.13
295.14 295.15 295.2 295.20 295.21 295.22 295.23 295.24 295.25 295.3 295.30
295.31 295.32 295.33 295.34 295.35 295.4 295.40 295.41 295.42 295.43 295.44
295.45 295.5 295.50 295.51 295.52 295.53 295.54 295.55 295.6 295.60 295.61
295.62 295.63 295.64 295.65 295.7 295.70 295.71 295.72 295.73 295.74 295.75
295.8 295.80 295.81 295.82 295.83 295.84 295.85 295.9 295.90 295.91 295.92
295.93 295.94 295.95 296.0 296.00 296.01 296.02 296.03 296.04 296.05 296.06
296.1 296.10 296.11 296.12 296.13 296.14 296.15 296.16 296.2 296.20 296.21
296.22 296.23 296.24 296.25 296.26 296.3 296.30 296.31 296.32 296.33 296.34
296.35 296.36 296.4 296.40 296.41 296.42 296.43 296.44 296.45 296.46 296.5
296.50 296.51 296.52 296.53 296.54 296.55 296.56 296.6 296.60 296.61 296.62
296.63 296.64 296.65 296.66 296.7 296.8 296.80 296.81 296.82 296.89 296.9
296.90 296.99 297.0 297.1 297.2 297.3 297.8 297.9 298.0 298.1 298.2 298.3 298.4
298.8 298.9 299.0 299.00 299.01 299.1 299.10 299.11 299.8 299.80 299.81 299.9
299.90 299.91 300.0 300.00 300.01 300.02 300.09 300.1 300.10 300.11 300.12
300.13 300.14 300.15 300.16 300.19 300.2 300.20 300.21 300.22 300.23 300.29
300.3 300.4 300.5 300.6 300.7 300.8 300.81 300.82 300.89 300.9 301.0 301.1
301.10 301.11 301.12 301.13 301.2 301.20 301.21 301.22 301.3 301.4 301.5 301.50
301.51 301.59 301.6 301.7 301.8 301.81 301.82 301.83 301.84 301.89 301.9 302.0
302.1 302.2 302.3 302.4 302.5 302.50 302.51 302.52 302.53 302.6 302.7 302.70
302.71 302.72 302.73 302.74 302.75 302.76 302.79 302.8 302.81 302.82 302.83
302.84 302.85 302.89 302.9 303.0 303.00 303.01 303.02 303.03 303.0x 303.9
303.90 303.91 303.92 303.93 304.0 304.00 304.01 304.02 304.03 304.1 304.10
304.11 304.12 304.13 304.2 304.20 304.21 304.22 304.23 304.3 304.30 304.31
304.32 304.33 304.4 304.40 304.41 304.42 304.43 304.5 304.50 304.51 304.52
304.53 304.6 304.60 304.61 304.62 304.63 304.7 304.70 304.71 304.72 304.73
304.8 304.80 304.81 304.82 304.83 304.9 304.90 304.91 304.92 304.93 305.0
305.00 305.01 305.02 305.03 305.0x 305.1 305.10 305.11 305.12 305.13 305.2
305.20 305.21 305.22 305.23 305.2x 305.3 305.30 305.31 305.32 305.33 305.3x
305.4 305.40 305.41 305.42 305.43 305.4x 305.5 305.50 305.51 305.52 305.53
305.5x 305.6 305.60 305.61 305.62 305.63 305.6x 305.7 305.70 305.71 305.72
305.73 305.7x 305.8 305.80 305.81 305.82 305.83 305.8x 305.9 305.90 305.91
305.92 305.93 305.9x 306.0 306.1 306.2 306.3 306.4 306.50 306.51 306.52 306.53
306.59 306.6 306.7 306.8 306.9 307.0 307.1 307.2 307.20 307.21 307.22 307.23
307.3 307.4 307.40 307.41 307.42 307.43 307.44 307.45 307.46 307.47 307.48
307.49 307.5 307.50 307.51 307.52 307.53 307.54 307.59 307.6 307.7 307.8 307.80
307.81 307.89 307.9 308.0 308.1 308.2 308.3 308.4 308.9 309.0 309.1 309.2
309.21 309.22 309.23 309.24 309.28 309.29 309.3 309.4 309.8 309.81 309.82
309.83 309.89 309.9 310.0 310.1 310.2 310.8 310.81 310.89 310.9 311 312.0
312.00 312.01 312.02 312.03 312.1 312.10 312.11 312.12 312.13 312.2 312.20
312.21 312.22 312.23 312.3 312.30 312.31 312.32 312.33 312.34 312.35 312.39
312.4 312.8 312.81 312.82 312.89 312.9 313.0 313.1 313.2 313.21 313.22 313.23
313.3 313.8 313.81 313.82 313.83 313.89 313.9 314.0 314.00 314.01 314.1 314.2
314.8 314.9 315.0 315.00 315.01 315.02 315.09 315.1 315.2 315.3 315.31 315.32
315.34 315.35 315.39 315.4 315.5 315.8 315.9 316 317 318.0 318.1 318.2 319
V71.09
""".split())

# -----------------------------------------------------------------------------
# ICD-10; from the client: camcops --print_icd10_codes
# -----------------------------------------------------------------------------

CLIENT_ICD10_CODES = set("""
F00 F00.0 F00.00 F00.000 F00.001 F00.002 F00.01 F00.010 F00.011 F00.012 F00.02
F00.020 F00.021 F00.022 F00.03 F00.030 F00.031 F00.032 F00.04 F00.040 F00.041
F00.042 F00.1 F00.10 F00.100 F00.101 F00.102 F00.11 F00.110 F00.111 F00.112
F00.12 F00.120 F00.121 F00.122 F00.13 F00.130 F00.131 F00.132 F00.14 F00.140
F00.141 F00.142 F00.2 F00.20 F00.200 F00.201 F00.202 F00.21 F00.210 F00.211
F00.212 F00.22 F00.220 F00.221 F00.222 F00.23 F00.230 F00.231 F00.232 F00.24
F00.240 F00.241 F00.242 F00.9 F00.90 F00.900 F00.901 F00.902 F00.91 F00.910
F00.911 F00.912 F00.92 F00.920 F00.921 F00.922 F00.93 F00.930 F00.931 F00.932
F00.94 F00.940 F00.941 F00.942 F01 F01.0 F01.00 F01.000 F01.001 F01.002 F01.01
F01.010 F01.011 F01.012 F01.02 F01.020 F01.021 F01.022 F01.03 F01.030 F01.031
F01.032 F01.04 F01.040 F01.041 F01.042 F01.1 F01.10 F01.100 F01.101 F01.102
F01.11 F01.110 F01.111 F01.112 F01.12 F01.120 F01.121 F01.122 F01.13 F01.130
F01.131 F01.132 F01.14 F01.140 F01.141 F01.142 F01.2 F01.20 F01.200 F01.201
F01.202 F01.21 F01.210 F01.211 F01.212 F01.22 F01.220 F01.221 F01.222 F01.23
F01.230 F01.231 F01.232 F01.24 F01.240 F01.241 F01.242 F01.3 F01.30 F01.300
F01.301 F01.302 F01.31 F01.310 F01.311 F01.312 F01.32 F01.320 F01.321 F01.322
F01.33 F01.330 F01.331 F01.332 F01.34 F01.340 F01.341 F01.342 F01.8 F01.80
F01.800 F01.801 F01.802 F01.81 F01.810 F01.811 F01.812 F01.82 F01.820 F01.821
F01.822 F01.83 F01.830 F01.831 F01.832 F01.84 F01.840 F01.841 F01.842 F01.9
F01.90 F01.900 F01.901 F01.902 F01.91 F01.910 F01.911 F01.912 F01.92 F01.920
F01.921 F01.922 F01.93 F01.930 F01.931 F01.932 F01.94 F01.940 F01.941 F01.942
F02 F02.0 F02.00 F02.000 F02.001 F02.002 F02.01 F02.010 F02.011 F02.012 F02.02
F02.020 F02.021 F02.022 F02.03 F02.030 F02.031 F02.032 F02.04 F02.040 F02.041
F02.042 F02.1 F02.10 F02.100 F02.101 F02.102 F02.11 F02.110 F02.111 F02.112
F02.12 F02.120 F02.121 F02.122 F02.13 F02.130 F02.131 F02.132 F02.14 F02.140
F02.141 F02.142 F02.2 F02.20 F02.200 F02.201 F02.202 F02.21 F02.210 F02.211
F02.212 F02.22 F02.220 F02.221 F02.222 F02.23 F02.230 F02.231 F02.232 F02.24
F02.240 F02.241 F02.242 F02.3 F02.30 F02.300 F02.301 F02.302 F02.31 F02.310
F02.311 F02.312 F02.32 F02.320 F02.321 F02.322 F02.33 F02.330 F02.331 F02.332
F02.34 F02.340 F02.341 F02.342 F02.4 F02.40 F02.400 F02.401 F02.402 F02.41
F02.410 F02.411 F02.412 F02.42 F02.420 F02.421 F02.422 F02.43 F02.430 F02.431
F02.432 F02.44 F02.440 F02.441 F02.442 F02.8 F02.80 F02.800 F02.801 F02.802
F02.81 F02.810 F02.811 F02.812 F02.82 F02.820 F02.821 F02.822 F02.83 F02.830
F02.831 F02.832 F02.84 F02.840 F02.841 F02.842 F03 F03.0 F03.00 F03.000 F03.001
F03.002 F03.01 F03.010 F03.011 F03.012 F03.02 F03.020 F03.021 F03.022 F03.03
F03.030 F03.031 F03.032 F03.04 F03.040 F03.041 F03.042 F04 F05 F05.0 F05.1
F05.8 F05.9 F06 F06.0 F06.1 F06.2 F06.3 F06.4 F06.5 F06.6 F06.7 F06.8 F06.9 F07
F07.0 F07.1 F07.2 F07.8 F07.9 F09 F10 F10.0 F10.00 F10.01 F10.02 F10.03 F10.04
F10.05 F10.06 F10.07 F10.1 F10.2 F10.20 F10.200 F10.201 F10.202 F10.21 F10.22
F10.23 F10.24 F10.241 F10.242 F10.25 F10.26 F10.3 F10.30 F10.31 F10.4 F10.40
F10.41 F10.5 F10.50 F10.51 F10.52 F10.53 F10.54 F10.55 F10.56 F10.6 F10.7
F10.70 F10.71 F10.72 F10.73 F10.74 F10.75 F10.8 F10.9 F11 F11.0 F11.00 F11.01
F11.02 F11.03 F11.04 F11.05 F11.06 F11.1 F11.2 F11.20 F11.200 F11.201 F11.202
F11.21 F11.22 F11.23 F11.24 F11.241 F11.242 F11.25 F11.26 F11.3 F11.30 F11.31
F11.4 F11.40 F11.41 F11.5 F11.50 F11.51 F11.52 F11.53 F11.54 F11.55 F11.56
F11.6 F11.7 F11.70 F11.71 F11.72 F11.73 F11.74 F11.75 F11.8 F11.9 F12 F12.0
F12.00 F12.01 F12.02 F12.03 F12.04 F12.05 F12.06 F12.1 F12.2 F12.20 F12.200
F12.201 F12.202 F12.21 F12.22 F12.23 F12.24 F12.241 F12.242 F12.25 F12.26 F12.3
F12.30 F12.31 F12.4 F12.40 F12.41 F12.5 F12.50 F12.51 F12.52 F12.53 F12.54
F12.55 F12.56 F12.6 F12.7 F12.70 F12.71 F12.72 F12.73 F12.74 F12.75 F12.8 F12.9
F13 F13.0 F13.00 F13.01 F13.02 F13.03 F13.04 F13.05 F13.06 F13.1 F13.2 F13.20
F13.200 F13.201 F13.202 F13.21 F13.22 F13.23 F13.24 F13.241 F13.242 F13.25
F13.26 F13.3 F13.30 F13.31 F13.4 F13.40 F13.41 F13.5 F13.50 F13.51 F13.52
F13.53 F13.54 F13.55 F13.56 F13.6 F13.7 F13.70 F13.71 F13.72 F13.73 F13.74
F13.75 F13.8 F13.9 F14 F14.0 F14.00 F14.01 F14.02 F14.03 F14.04 F14.05 F14.06
F14.1 F14.2 F14.20 F14.200 F14.201 F14.202 F14.21 F14.22 F14.23 F14.24 F14.241
F14.242 F14.25 F14.26 F14.3 F14.30 F14.31 F14.4 F14.40 F14.41 F14.5 F14.50
F14.51 F14.52 F14.53 F14.54 F14.55 F14.56 F14.6 F14.7 F14.70 F14.71 F14.72
F14.73 F14.74 F14.75 F14.8 F14.9 F15 F15.0 F15.00 F15.01 F15.02 F15.03 F15.04
F15.05 F15.06 F15.1 F15.2 F15.20 F15.200 F15.201 F15.202 F15.21 F15.22 F15.23
F15.24 F15.241 F15.242 F15.25 F15.26 F15.3 F15.30 F15.31 F15.4 F15.40 F15.41
F15.5 F15.50 F15.51 F15.52 F15.53 F15.54 F15.55 F15.56 F15.6 F15.7 F15.70
F15.71 F15.72 F15.73 F15.74 F15.75 F15.8 F15.9 F16 F16.0 F16.00 F16.01 F16.02
F16.03 F16.04 F16.05 F16.06 F16.1 F16.2 F16.20 F16.200 F16.201 F16.202 F16.21
F16.22 F16.23 F16.24 F16.241 F16.242 F16.25 F16.26 F16.3 F16.30 F16.31 F16.4
F16.40 F16.41 F16.5 F16.50 F16.51 F16.52 F16.53 F16.54 F16.55 F16.56 F16.6
F16.7 F16.70 F16.71 F16.72 F16.73 F16.74 F16.75 F16.8 F16.9 F17 F17.0 F17.00
F17.01 F17.02 F17.03 F17.04 F17.05 F17.06 F17.1 F17.2 F17.20 F17.200 F17.201
F17.202 F17.21 F17.22 F17.23 F17.24 F17.241 F17.242 F17.25 F17.26 F17.3 F17.30
F17.31 F17.4 F17.40 F17.41 F17.5 F17.50 F17.51 F17.52 F17.53 F17.54 F17.55
F17.56 F17.6 F17.7 F17.70 F17.71 F17.72 F17.73 F17.74 F17.75 F17.8 F17.9 F18
F18.0 F18.00 F18.01 F18.02 F18.03 F18.04 F18.05 F18.06 F18.1 F18.2 F18.20
F18.200 F18.201 F18.202 F18.21 F18.22 F18.23 F18.24 F18.241 F18.242 F18.25
F18.26 F18.3 F18.30 F18.31 F18.4 F18.40 F18.41 F18.5 F18.50 F18.51 F18.52
F18.53 F18.54 F18.55 F18.56 F18.6 F18.7 F18.70 F18.71 F18.72 F18.73 F18.74
F18.75 F18.8 F18.9 F19 F19.0 F19.00 F19.01 F19.02 F19.03 F19.04 F19.05 F19.06
F19.1 F19.2 F19.20 F19.200 F19.201 F19.202 F19.21 F19.22 F19.23 F19.24 F19.241
F19.242 F19.25 F19.26 F19.3 F19.30 F19.31 F19.4 F19.40 F19.41 F19.5 F19.50
F19.51 F19.52 F19.53 F19.54 F19.55 F19.56 F19.6 F19.7 F19.70 F19.71 F19.72
F19.73 F19.74 F19.75 F19.8 F19.9 F20 F20.0 F20.1 F20.2 F20.3 F20.4 F20.5 F20.6
F20.8 F20.9 F21 F22 F22.0 F22.8 F22.9 F23 F23.0 F23.1 F23.2 F23.3 F23.8 F23.9
F23.90 F23.91 F24 F25 F25.0 F25.00 F25.01 F25.1 F25.10 F25.11 F25.2 F25.20
F25.21 F25.8 F25.80 F25.81 F25.9 F25.90 F25.91 F28 F29 F30 F30.0 F30.1 F30.2
F30.20 F30.21 F30.8 F30.9 F31 F31.0 F31.1 F31.2 F31.20 F31.21 F31.3 F31.30
F31.31 F31.4 F31.5 F31.50 F31.51 F31.6 F31.7 F31.8 F31.9 F32 F32.0 F32.00
F32.01 F32.1 F32.10 F32.11 F32.2 F32.3 F32.30 F32.31 F32.8 F32.9 F33 F33.0
F33.00 F33.01 F33.1 F33.10 F33.11 F33.2 F33.3 F33.30 F33.31 F33.4 F33.8 F33.9
F34 F34.0 F34.1 F34.8 F34.9 F38 F38.0 F38.00 F38.1 F38.10 F38.8 F39 F40 F40.0
F40.00 F40.01 F40.1 F40.2 F40.8 F40.9 F41 F41.0 F41.00 F41.01 F41.1 F41.2 F41.3
F41.8 F41.9 F42 F42.0 F42.1 F42.2 F42.8 F42.9 F43 F43.0 F43.00 F43.01 F43.02
F43.1 F43.2 F43.20 F43.21 F43.22 F43.23 F43.24 F43.25 F43.28 F43.8 F43.9 F44
F44.0 F44.1 F44.2 F44.3 F44.4 F44.5 F44.6 F44.7 F44.8 F44.80 F44.81 F44.82
F44.88 F44.9 F45 F45.0 F45.1 F45.2 F45.3 F45.30 F45.31 F45.32 F45.33 F45.34
F45.38 F45.4 F45.8 F45.9 F48 F48.0 F48.1 F48.8 F48.9 F50 F50.0 F50.1 F50.2
F50.3 F50.4 F50.5 F50.8 F50.9 F51 F51.0 F51.1 F51.2 F51.3 F51.4 F51.5 F51.8
F51.9 F52 F52.0 F52.1 F52.10 F52.11 F52.2 F52.3 F52.4 F52.5 F52.6 F52.7 F52.8
F52.9 F53 F53.0 F53.1 F53.8 F53.9 F54 F55 F59 F60 F60.0 F60.1 F60.2 F60.3
F60.30 F60.31 F60.4 F60.5 F60.6 F60.7 F60.8 F60.9 F61 F62 F62.0 F62.1 F62.8
F62.9 F63 F63.0 F63.1 F63.2 F63.3 F63.8 F63.9 F64 F64.0 F64.1 F64.2 F64.8 F64.9
F65 F65.0 F65.1 F65.2 F65.3 F65.4 F65.5 F65.6 F65.8 F65.9 F66 F66.0 F66.1 F66.2
F66.8 F66.9 F66.90 F66.91 F66.92 F66.98 F68 F68.0 F68.1 F68.8 F69 F70 F70.0
F70.1 F70.8 F70.9 F71 F71.0 F71.1 F71.8 F71.9 F72 F72.0 F72.1 F72.8 F72.9 F73
F73.0 F73.1 F73.8 F73.9 F78 F78.0 F78.1 F78.8 F78.9 F79 F79.0 F79.1 F79.8 F79.9
F80 F80.0 F80.1 F80.2 F80.3 F80.8 F80.9 F81 F81.0 F81.1 F81.2 F81.3 F81.8 F81.9
F82 F83 F84 F84.0 F84.1 F84.10 F84.11 F84.12 F84.2 F84.3 F84.4 F84.5 F84.8
F84.9 F88 F89 F90 F90.0 F90.1 F90.8 F90.9 F91 F91.0 F91.1 F91.2 F91.3 F91.8
F91.9 F92 F92.0 F92.8 F92.9 F93 F93.0 F93.1 F93.2 F93.3 F93.8 F93.80 F93.9 F94
F94.0 F94.1 F94.2 F94.8 F94.9 F95 F95.0 F95.1 F95.2 F95.8 F95.9 F98 F98.0
F98.00 F98.01 F98.02 F98.1 F98.10 F98.11 F98.12 F98.2 F98.3 F98.4 F98.40 F98.41
F98.42 F98.5 F98.6 F98.8 F98.9 F99 R40 R40.0 R40.1 R40.2 R41 R41.0 R41.1 R41.2
R41.3 R41.8 R42 R43 R43.0 R43.1 R43.2 R43.8 R44 R44.0 R44.1 R44.2 R44.3 R44.8
R45 R45.0 R45.1 R45.2 R45.3 R45.4 R45.5 R45.6 R45.7 R45.8 R46 R46.0 R46.1 R46.2
R46.3 R46.4 R46.5 R46.6 R46.7 R46.8 X60 X60.0 X60.00 X60.01 X60.02 X60.03
X60.04 X60.08 X60.09 X60.1 X60.10 X60.11 X60.12 X60.13 X60.14 X60.18 X60.19
X60.2 X60.20 X60.21 X60.22 X60.23 X60.24 X60.28 X60.29 X60.3 X60.30 X60.31
X60.32 X60.33 X60.34 X60.38 X60.39 X60.4 X60.40 X60.41 X60.42 X60.43 X60.44
X60.48 X60.49 X60.5 X60.50 X60.51 X60.52 X60.53 X60.54 X60.58 X60.59 X60.6
X60.60 X60.61 X60.62 X60.63 X60.64 X60.68 X60.69 X60.7 X60.70 X60.71 X60.72
X60.73 X60.74 X60.78 X60.79 X60.8 X60.80 X60.81 X60.82 X60.83 X60.84 X60.88
X60.89 X60.9 X60.90 X60.91 X60.92 X60.93 X60.94 X60.98 X60.99 X61 X61.0 X61.00
X61.01 X61.02 X61.03 X61.04 X61.08 X61.09 X61.1 X61.10 X61.11 X61.12 X61.13
X61.14 X61.18 X61.19 X61.2 X61.20 X61.21 X61.22 X61.23 X61.24 X61.28 X61.29
X61.3 X61.30 X61.31 X61.32 X61.33 X61.34 X61.38 X61.39 X61.4 X61.40 X61.41
X61.42 X61.43 X61.44 X61.48 X61.49 X61.5 X61.50 X61.51 X61.52 X61.53 X61.54
X61.58 X61.59 X61.6 X61.60 X61.61 X61.62 X61.63 X61.64 X61.68 X61.69 X61.7
X61.70 X61.71 X61.72 X61.73 X61.74 X61.78 X61.79 X61.8 X61.80 X61.81 X61.82
X61.83 X61.84 X61.88 X61.89 X61.9 X61.90 X61.91 X61.92 X61.93 X61.94 X61.98
X61.99 X62 X62.0 X62.00 X62.01 X62.02 X62.03 X62.04 X62.08 X62.09 X62.1 X62.10
X62.11 X62.12 X62.13 X62.14 X62.18 X62.19 X62.2 X62.20 X62.21 X62.22 X62.23
X62.24 X62.28 X62.29 X62.3 X62.30 X62.31 X62.32 X62.33 X62.34 X62.38 X62.39
X62.4 X62.40 X62.41 X62.42 X62.43 X62.44 X62.48 X62.49 X62.5 X62.50 X62.51
X62.52 X62.53 X62.54 X62.58 X62.59 X62.6 X62.60 X62.61 X62.62 X62.63 X62.64
X62.68 X62.69 X62.7 X62.70 X62.71 X62.72 X62.73 X62.74 X62.78 X62.79 X62.8
X62.80 X62.81 X62.82 X62.83 X62.84 X62.88 X62.89 X62.9 X62.90 X62.91 X62.92
X62.93 X62.94 X62.98 X62.99 X63 X63.0 X63.00 X63.01 X63.02 X63.03 X63.04 X63.08
X63.09 X63.1 X63.10 X63.11 X63.12 X63.13 X63.14 X63.18 X63.19 X63.2 X63.20
X63.21 X63.22 X63.23 X63.24 X63.28 X63.29 X63.3 X63.30 X63.31 X63.32 X63.33
X63.34 X63.38 X63.39 X63.4 X63.40 X63.41 X63.42 X63.43 X63.44 X63.48 X63.49
X63.5 X63.50 X63.51 X63.52 X63.53 X63.54 X63.58 X63.59 X63.6 X63.60 X63.61
X63.62 X63.63 X63.64 X63.68 X63.69 X63.7 X63.70 X63.71 X63.72 X63.73 X63.74
X63.78 X63.79 X63.8 X63.80 X63.81 X63.82 X63.83 X63.84 X63.88 X63.89 X63.9
X63.90 X63.91 X63.92 X63.93 X63.94 X63.98 X63.99 X64 X64.0 X64.00 X64.01 X64.02
X64.03 X64.04 X64.08 X64.09 X64.1 X64.10 X64.11 X64.12 X64.13 X64.14 X64.18
X64.19 X64.2 X64.20 X64.21 X64.22 X64.23 X64.24 X64.28 X64.29 X64.3 X64.30
X64.31 X64.32 X64.33 X64.34 X64.38 X64.39 X64.4 X64.40 X64.41 X64.42 X64.43
X64.44 X64.48 X64.49 X64.5 X64.50 X64.51 X64.52 X64.53 X64.54 X64.58 X64.59
X64.6 X64.60 X64.61 X64.62 X64.63 X64.64 X64.68 X64.69 X64.7 X64.70 X64.71
X64.72 X64.73 X64.74 X64.78 X64.79 X64.8 X64.80 X64.81 X64.82 X64.83 X64.84
X64.88 X64.89 X64.9 X64.90 X64.91 X64.92 X64.93 X64.94 X64.98 X64.99 X65 X65.0
X65.00 X65.01 X65.02 X65.03 X65.04 X65.08 X65.09 X65.1 X65.10 X65.11 X65.12
X65.13 X65.14 X65.18 X65.19 X65.2 X65.20 X65.21 X65.22 X65.23 X65.24 X65.28
X65.29 X65.3 X65.30 X65.31 X65.32 X65.33 X65.34 X65.38 X65.39 X65.4 X65.40
X65.41 X65.42 X65.43 X65.44 X65.48 X65.49 X65.5 X65.50 X65.51 X65.52 X65.53
X65.54 X65.58 X65.59 X65.6 X65.60 X65.61 X65.62 X65.63 X65.64 X65.68 X65.69
X65.7 X65.70 X65.71 X65.72 X65.73 X65.74 X65.78 X65.79 X65.8 X65.80 X65.81
X65.82 X65.83 X65.84 X65.88 X65.89 X65.9 X65.90 X65.91 X65.92 X65.93 X65.94
X65.98 X65.99 X66 X66.0 X66.00 X66.01 X66.02 X66.03 X66.04 X66.08 X66.09 X66.1
X66.10 X66.11 X66.12 X66.13 X66.14 X66.18 X66.19 X66.2 X66.20 X66.21 X66.22
X66.23 X66.24 X66.28 X66.29 X66.3 X66.30 X66.31 X66.32 X66.33 X66.34 X66.38
X66.39 X66.4 X66.40 X66.41 X66.42 X66.43 X66.44 X66.48 X66.49 X66.5 X66.50
X66.51 X66.52 X66.53 X66.54 X66.58 X66.59 X66.6 X66.60 X66.61 X66.62 X66.63
X66.64 X66.68 X66.69 X66.7 X66.70 X66.71 X66.72 X66.73 X66.74 X66.78 X66.79
X66.8 X66.80 X66.81 X66.82 X66.83 X66.84 X66.88 X66.89 X66.9 X66.90 X66.91
X66.92 X66.93 X66.94 X66.98 X66.99 X67 X67.0 X67.00 X67.01 X67.02 X67.03 X67.04
X67.08 X67.09 X67.1 X67.10 X67.11 X67.12 X67.13 X67.14 X67.18 X67.19 X67.2
X67.20 X67.21 X67.22 X67.23 X67.24 X67.28 X67.29 X67.3 X67.30 X67.31 X67.32
X67.33 X67.34 X67.38 X67.39 X67.4 X67.40 X67.41 X67.42 X67.43 X67.44 X67.48
X67.49 X67.5 X67.50 X67.51 X67.52 X67.53 X67.54 X67.58 X67.59 X67.6 X67.60
X67.61 X67.62 X67.63 X67.64 X67.68 X67.69 X67.7 X67.70 X67.71 X67.72 X67.73
X67.74 X67.78 X67.79 X67.8 X67.80 X67.81 X67.82 X67.83 X67.84 X67.88 X67.89
X67.9 X67.90 X67.91 X67.92 X67.93 X67.94 X67.98 X67.99 X68 X68.0 X68.00 X68.01
X68.02 X68.03 X68.04 X68.08 X68.09 X68.1 X68.10 X68.11 X68.12 X68.13 X68.14
X68.18 X68.19 X68.2 X68.20 X68.21 X68.22 X68.23 X68.24 X68.28 X68.29 X68.3
X68.30 X68.31 X68.32 X68.33 X68.34 X68.38 X68.39 X68.4 X68.40 X68.41 X68.42
X68.43 X68.44 X68.48 X68.49 X68.5 X68.50 X68.51 X68.52 X68.53 X68.54 X68.58
X68.59 X68.6 X68.60 X68.61 X68.62 X68.63 X68.64 X68.68 X68.69 X68.7 X68.70
X68.71 X68.72 X68.73 X68.74 X68.78 X68.79 X68.8 X68.80 X68.81 X68.82 X68.83
X68.84 X68.88 X68.89 X68.9 X68.90 X68.91 X68.92 X68.93 X68.94 X68.98 X68.99 X69
X69.0 X69.00 X69.01 X69.02 X69.03 X69.04 X69.08 X69.09 X69.1 X69.10 X69.11
X69.12 X69.13 X69.14 X69.18 X69.19 X69.2 X69.20 X69.21 X69.22 X69.23 X69.24
X69.28 X69.29 X69.3 X69.30 X69.31 X69.32 X69.33 X69.34 X69.38 X69.39 X69.4
X69.40 X69.41 X69.42 X69.43 X69.44 X69.48 X69.49 X69.5 X69.50 X69.51 X69.52
X69.53 X69.54 X69.58 X69.59 X69.6 X69.60 X69.61 X69.62 X69.63 X69.64 X69.68
X69.69 X69.7 X69.70 X69.71 X69.72 X69.73 X69.74 X69.78 X69.79 X69.8 X69.80
X69.81 X69.82 X69.83 X69.84 X69.88 X69.89 X69.9 X69.90 X69.91 X69.92 X69.93
X69.94 X69.98 X69.99 X70 X70.0 X70.00 X70.01 X70.02 X70.03 X70.04 X70.08 X70.09
X70.1 X70.10 X70.11 X70.12 X70.13 X70.14 X70.18 X70.19 X70.2 X70.20 X70.21
X70.22 X70.23 X70.24 X70.28 X70.29 X70.3 X70.30 X70.31 X70.32 X70.33 X70.34
X70.38 X70.39 X70.4 X70.40 X70.41 X70.42 X70.43 X70.44 X70.48 X70.49 X70.5
X70.50 X70.51 X70.52 X70.53 X70.54 X70.58 X70.59 X70.6 X70.60 X70.61 X70.62
X70.63 X70.64 X70.68 X70.69 X70.7 X70.70 X70.71 X70.72 X70.73 X70.74 X70.78
X70.79 X70.8 X70.80 X70.81 X70.82 X70.83 X70.84 X70.88 X70.89 X70.9 X70.90
X70.91 X70.92 X70.93 X70.94 X70.98 X70.99 X71 X71.0 X71.00 X71.01 X71.02 X71.03
X71.04 X71.08 X71.09 X71.1 X71.10 X71.11 X71.12 X71.13 X71.14 X71.18 X71.19
X71.2 X71.20 X71.21 X71.22 X71.23 X71.24 X71.28 X71.29 X71.3 X71.30 X71.31
X71.32 X71.33 X71.34 X71.38 X71.39 X71.4 X71.40 X71.41 X71.42 X71.43 X71.44
X71.48 X71.49 X71.5 X71.50 X71.51 X71.52 X71.53 X71.54 X71.58 X71.59 X71.6
X71.60 X71.61 X71.62 X71.63 X71.64 X71.68 X71.69 X71.7 X71.70 X71.71 X71.72
X71.73 X71.74 X71.78 X71.79 X71.8 X71.80 X71.81 X71.82 X71.83 X71.84 X71.88
X71.89 X71.9 X71.90 X71.91 X71.92 X71.93 X71.94 X71.98 X71.99 X72 X72.0 X72.00
X72.01 X72.02 X72.03 X72.04 X72.08 X72.09 X72.1 X72.10 X72.11 X72.12 X72.13
X72.14 X72.18 X72.19 X72.2 X72.20 X72.21 X72.22 X72.23 X72.24 X72.28 X72.29
X72.3 X72.30 X72.31 X72.32 X72.33 X72.34 X72.38 X72.39 X72.4 X72.40 X72.41
X72.42 X72.43 X72.44 X72.48 X72.49 X72.5 X72.50 X72.51 X72.52 X72.53 X72.54
X72.58 X72.59 X72.6 X72.60 X72.61 X72.62 X72.63 X72.64 X72.68 X72.69 X72.7
X72.70 X72.71 X72.72 X72.73 X72.74 X72.78 X72.79 X72.8 X72.80 X72.81 X72.82
X72.83 X72.84 X72.88 X72.89 X72.9 X72.90 X72.91 X72.92 X72.93 X72.94 X72.98
X72.99 X73 X73.0 X73.00 X73.01 X73.02 X73.03 X73.04 X73.08 X73.09 X73.1 X73.10
X73.11 X73.12 X73.13 X73.14 X73.18 X73.19 X73.2 X73.20 X73.21 X73.22 X73.23
X73.24 X73.28 X73.29 X73.3 X73.30 X73.31 X73.32 X73.33 X73.34 X73.38 X73.39
X73.4 X73.40 X73.41 X73.42 X73.43 X73.44 X73.48 X73.49 X73.5 X73.50 X73.51
X73.52 X73.53 X73.54 X73.58 X73.59 X73.6 X73.60 X73.61 X73.62 X73.63 X73.64
X73.68 X73.69 X73.7 X73.70 X73.71 X73.72 X73.73 X73.74 X73.78 X73.79 X73.8
X73.80 X73.81 X73.82 X73.83 X73.84 X73.88 X73.89 X73.9 X73.90 X73.91 X73.92
X73.93 X73.94 X73.98 X73.99 X74 X74.0 X74.00 X74.01 X74.02 X74.03 X74.04 X74.08
X74.09 X74.1 X74.10 X74.11 X74.12 X74.13 X74.14 X74.18 X74.19 X74.2 X74.20
X74.21 X74.22 X74.23 X74.24 X74.28 X74.29 X74.3 X74.30 X74.31 X74.32 X74.33
X74.34 X74.38 X74.39 X74.4 X74.40 X74.41 X74.42 X74.43 X74.44 X74.48 X74.49
X74.5 X74.50 X74.51 X74.52 X74.53 X74.54 X74.58 X74.59 X74.6 X74.60 X74.61
X74.62 X74.63 X74.64 X74.68 X74.69 X74.7 X74.70 X74.71 X74.72 X74.73 X74.74
X74.78 X74.79 X74.8 X74.80 X74.81 X74.82 X74.83 X74.84 X74.88 X74.89 X74.9
X74.90 X74.91 X74.92 X74.93 X74.94 X74.98 X74.99 X75 X75.0 X75.00 X75.01 X75.02
X75.03 X75.04 X75.08 X75.09 X75.1 X75.10 X75.11 X75.12 X75.13 X75.14 X75.18
X75.19 X75.2 X75.20 X75.21 X75.22 X75.23 X75.24 X75.28 X75.29 X75.3 X75.30
X75.31 X75.32 X75.33 X75.34 X75.38 X75.39 X75.4 X75.40 X75.41 X75.42 X75.43
X75.44 X75.48 X75.49 X75.5 X75.50 X75.51 X75.52 X75.53 X75.54 X75.58 X75.59
X75.6 X75.60 X75.61 X75.62 X75.63 X75.64 X75.68 X75.69 X75.7 X75.70 X75.71
X75.72 X75.73 X75.74 X75.78 X75.79 X75.8 X75.80 X75.81 X75.82 X75.83 X75.84
X75.88 X75.89 X75.9 X75.90 X75.91 X75.92 X75.93 X75.94 X75.98 X75.99 X76 X76.0
X76.00 X76.01 X76.02 X76.03 X76.04 X76.08 X76.09 X76.1 X76.10 X76.11 X76.12
X76.13 X76.14 X76.18 X76.19 X76.2 X76.20 X76.21 X76.22 X76.23 X76.24 X76.28
X76.29 X76.3 X76.30 X76.31 X76.32 X76.33 X76.34 X76.38 X76.39 X76.4 X76.40
X76.41 X76.42 X76.43 X76.44 X76.48 X76.49 X76.5 X76.50 X76.51 X76.52 X76.53
X76.54 X76.58 X76.59 X76.6 X76.60 X76.61 X76.62 X76.63 X76.64 X76.68 X76.69
X76.7 X76.70 X76.71 X76.72 X76.73 X76.74 X76.78 X76.79 X76.8 X76.80 X76.81
X76.82 X76.83 X76.84 X76.88 X76.89 X76.9 X76.90 X76.91 X76.92 X76.93 X76.94
X76.98 X76.99 X77 X77.0 X77.00 X77.01 X77.02 X77.03 X77.04 X77.08 X77.09 X77.1
X77.10 X77.11 X77.12 X77.13 X77.14 X77.18 X77.19 X77.2 X77.20 X77.21 X77.22
X77.23 X77.24 X77.28 X77.29 X77.3 X77.30 X77.31 X77.32 X77.33 X77.34 X77.38
X77.39 X77.4 X77.40 X77.41 X77.42 X77.43 X77.44 X77.48 X77.49 X77.5 X77.50
X77.51 X77.52 X77.53 X77.54 X77.58 X77.59 X77.6 X77.60 X77.61 X77.62 X77.63
X77.64 X77.68 X77.69 X77.7 X77.70 X77.71 X77.72 X77.73 X77.74 X77.78 X77.79
X77.8 X77.80 X77.81 X77.82 X77.83 X77.84 X77.88 X77.89 X77.9 X77.90 X77.91
X77.92 X77.93 X77.94 X77.98 X77.99 X78 X78.0 X78.00 X78.01 X78.02 X78.03 X78.04
X78.08 X78.09 X78.1 X78.10 X78.11 X78.12 X78.13 X78.14 X78.18 X78.19 X78.2
X78.20 X78.21 X78.22 X78.23 X78.24 X78.28 X78.29 X78.3 X78.30 X78.31 X78.32
X78.33 X78.34 X78.38 X78.39 X78.4 X78.40 X78.41 X78.42 X78.43 X78.44 X78.48
X78.49 X78.5 X78.50 X78.51 X78.52 X78.53 X78.54 X78.58 X78.59 X78.6 X78.60
X78.61 X78.62 X78.63 X78.64 X78.68 X78.69 X78.7 X78.70 X78.71 X78.72 X78.73
X78.74 X78.78 X78.79 X78.8 X78.80 X78.81 X78.82 X78.83 X78.84 X78.88 X78.89
X78.9 X78.90 X78.91 X78.92 X78.93 X78.94 X78.98 X78.99 X79 X79.0 X79.00 X79.01
X79.02 X79.03 X79.04 X79.08 X79.09 X79.1 X79.10 X79.11 X79.12 X79.13 X79.14
X79.18 X79.19 X79.2 X79.20 X79.21 X79.22 X79.23 X79.24 X79.28 X79.29 X79.3
X79.30 X79.31 X79.32 X79.33 X79.34 X79.38 X79.39 X79.4 X79.40 X79.41 X79.42
X79.43 X79.44 X79.48 X79.49 X79.5 X79.50 X79.51 X79.52 X79.53 X79.54 X79.58
X79.59 X79.6 X79.60 X79.61 X79.62 X79.63 X79.64 X79.68 X79.69 X79.7 X79.70
X79.71 X79.72 X79.73 X79.74 X79.78 X79.79 X79.8 X79.80 X79.81 X79.82 X79.83
X79.84 X79.88 X79.89 X79.9 X79.90 X79.91 X79.92 X79.93 X79.94 X79.98 X79.99 X80
X80.0 X80.00 X80.01 X80.02 X80.03 X80.04 X80.08 X80.09 X80.1 X80.10 X80.11
X80.12 X80.13 X80.14 X80.18 X80.19 X80.2 X80.20 X80.21 X80.22 X80.23 X80.24
X80.28 X80.29 X80.3 X80.30 X80.31 X80.32 X80.33 X80.34 X80.38 X80.39 X80.4
X80.40 X80.41 X80.42 X80.43 X80.44 X80.48 X80.49 X80.5 X80.50 X80.51 X80.52
X80.53 X80.54 X80.58 X80.59 X80.6 X80.60 X80.61 X80.62 X80.63 X80.64 X80.68
X80.69 X80.7 X80.70 X80.71 X80.72 X80.73 X80.74 X80.78 X80.79 X80.8 X80.80
X80.81 X80.82 X80.83 X80.84 X80.88 X80.89 X80.9 X80.90 X80.91 X80.92 X80.93
X80.94 X80.98 X80.99 X81 X81.0 X81.00 X81.01 X81.02 X81.03 X81.04 X81.08 X81.09
X81.1 X81.10 X81.11 X81.12 X81.13 X81.14 X81.18 X81.19 X81.2 X81.20 X81.21
X81.22 X81.23 X81.24 X81.28 X81.29 X81.3 X81.30 X81.31 X81.32 X81.33 X81.34
X81.38 X81.39 X81.4 X81.40 X81.41 X81.42 X81.43 X81.44 X81.48 X81.49 X81.5
X81.50 X81.51 X81.52 X81.53 X81.54 X81.58 X81.59 X81.6 X81.60 X81.61 X81.62
X81.63 X81.64 X81.68 X81.69 X81.7 X81.70 X81.71 X81.72 X81.73 X81.74 X81.78
X81.79 X81.8 X81.80 X81.81 X81.82 X81.83 X81.84 X81.88 X81.89 X81.9 X81.90
X81.91 X81.92 X81.93 X81.94 X81.98 X81.99 X82 X82.0 X82.00 X82.01 X82.02 X82.03
X82.04 X82.08 X82.09 X82.1 X82.10 X82.11 X82.12 X82.13 X82.14 X82.18 X82.19
X82.2 X82.20 X82.21 X82.22 X82.23 X82.24 X82.28 X82.29 X82.3 X82.30 X82.31
X82.32 X82.33 X82.34 X82.38 X82.39 X82.4 X82.40 X82.41 X82.42 X82.43 X82.44
X82.48 X82.49 X82.5 X82.50 X82.51 X82.52 X82.53 X82.54 X82.58 X82.59 X82.6
X82.60 X82.61 X82.62 X82.63 X82.64 X82.68 X82.69 X82.7 X82.70 X82.71 X82.72
X82.73 X82.74 X82.78 X82.79 X82.8 X82.80 X82.81 X82.82 X82.83 X82.84 X82.88
X82.89 X82.9 X82.90 X82.91 X82.92 X82.93 X82.94 X82.98 X82.99 X83 X83.0 X83.00
X83.01 X83.02 X83.03 X83.04 X83.08 X83.09 X83.1 X83.10 X83.11 X83.12 X83.13
X83.14 X83.18 X83.19 X83.2 X83.20 X83.21 X83.22 X83.23 X83.24 X83.28 X83.29
X83.3 X83.30 X83.31 X83.32 X83.33 X83.34 X83.38 X83.39 X83.4 X83.40 X83.41
X83.42 X83.43 X83.44 X83.48 X83.49 X83.5 X83.50 X83.51 X83.52 X83.53 X83.54
X83.58 X83.59 X83.6 X83.60 X83.61 X83.62 X83.63 X83.64 X83.68 X83.69 X83.7
X83.70 X83.71 X83.72 X83.73 X83.74 X83.78 X83.79 X83.8 X83.80 X83.81 X83.82
X83.83 X83.84 X83.88 X83.89 X83.9 X83.90 X83.91 X83.92 X83.93 X83.94 X83.98
X83.99 X84 X84.0 X84.00 X84.01 X84.02 X84.03 X84.04 X84.08 X84.09 X84.1 X84.10
X84.11 X84.12 X84.13 X84.14 X84.18 X84.19 X84.2 X84.20 X84.21 X84.22 X84.23
X84.24 X84.28 X84.29 X84.3 X84.30 X84.31 X84.32 X84.33 X84.34 X84.38 X84.39
X84.4 X84.40 X84.41 X84.42 X84.43 X84.44 X84.48 X84.49 X84.5 X84.50 X84.51
X84.52 X84.53 X84.54 X84.58 X84.59 X84.6 X84.60 X84.61 X84.62 X84.63 X84.64
X84.68 X84.69 X84.7 X84.70 X84.71 X84.72 X84.73 X84.74 X84.78 X84.79 X84.8
X84.80 X84.81 X84.82 X84.83 X84.84 X84.88 X84.89 X84.9 X84.90 X84.91 X84.92
X84.93 X84.94 X84.98 X84.99 Z00.4 Z03.2 Z71.1
""".split())


# =============================================================================
# The SnomedConcept class, amended
# =============================================================================

class SnomedConcept(SnomedConceptCardinalPythonlib):
    @classmethod
    def create(cls, concept: SnomedConceptCardinalPythonlib) \
            -> "SnomedConcept":
        """
        Creates a CamCOPS-friendly
        :class:`camcops_server.cc_modules.cc_snomed.SnomedConcept` from a
        :class:`cardinal_pythonlib.snomed.SnomedConcept`.

        The result has extra methods.
        """
        return SnomedConcept(identifier=concept.identifier,
                             term=concept.term)

    def xml_element(self, longform: bool = True) -> XmlElement:
        """
        Returns a :class:`camcops_server.cc_modules.cc_xml.XmlElement` for this
        SNOMED-CT object.

        Args:
            longform: print SNOMED-CT concepts in long form?
        """
        return XmlElement(
            name=SNOMED_XML_NAME,
            value=self.as_string(longform),
            datatype=XmlDataTypes.STRING
        )


# =============================================================================
# The SnomedExpression class, amended
# =============================================================================

class SnomedExpression(SnomedExpressionCardinalPythonlib):
    def xml_element(self, longform: bool = True) -> XmlElement:
        """
        Returns a :class:`camcops_server.cc_modules.cc_xml.XmlElement` for this
        SNOMED-CT object.

        Args:
            longform: print SNOMED-CT expressions in long form?
        """
        return XmlElement(
            name=SNOMED_XML_NAME,
            value=self.as_string(longform),
            datatype=XmlDataTypes.STRING
        )


# =============================================================================
# The CamCOPS XML file format for SNOMED-CT
# =============================================================================

ROOT_TAG = "snomed_concepts"
CONCEPT_TAG = "concept"
LOOKUP_TAG = "lookup"
LOOKUP_NAME_ATTR = "name"
LOOKUP_ATTR = "lookup"
ID_TAG = "id"
TERM_TAG = "term"
AUTOGEN_COMMENT = (
    "Autogenerated XML (see camcops_server.cc_modules.cc_snomed.py); do not "
    "edit"
)


def get_snomed_concepts_from_xml(xml_filename: str) \
        -> Dict[str, Union[SnomedConcept, List[SnomedConcept]]]:
    """
    Reads in all SNOMED-CT concepts from an XML file according to the CamCOPS
    format.

    Args:
        xml_filename: XML filename to read

    Returns:
        dict: mapping each lookup code found to a list of
        :class:`SnomedConcept` objects

    """
    log.info("Reading SNOMED-CT XML file: {}", xml_filename)
    parser = ElementTree.XMLParser(encoding="UTF-8")
    tree = ElementTree.parse(xml_filename, parser=parser)
    root = tree.getroot()
    all_concepts = {}  # type: Dict[str, List[SnomedConcept]]
    find_lookup = "./{tag}[@{attr}]".format(tag=LOOKUP_TAG, attr=LOOKUP_NAME_ATTR)  # noqa
    find_concept = "./{tag}".format(tag=CONCEPT_TAG)
    find_id_in_concept = "./{tag}".format(tag=ID_TAG)
    find_name_in_concept = "./{tag}".format(tag=TERM_TAG)
    for lookuproot in root.findall(find_lookup):
        # Extract info from the XML
        lookupname = lookuproot.attrib.get(LOOKUP_NAME_ATTR)
        for conceptroot in lookuproot.findall(find_concept):
            id_node = conceptroot.find(find_id_in_concept)
            identifier = int(id_node.text)
            name_node = conceptroot.find(find_name_in_concept)
            name = name_node.text or ""
            # Stash it
            concept = SnomedConcept(identifier, name)
            concepts_for_lookup = all_concepts.setdefault(lookupname, [])
            concepts_for_lookup.append(concept)
    # Done
    return all_concepts


def write_snomed_concepts_to_xml(
        xml_filename: str,
        concepts: Dict[str, List[SnomedConcept]],
        comment: str = AUTOGEN_COMMENT) -> None:
    """
    Writes SNOMED-CT concepts to an XML file in the CamCOPS format.

    Args:
        xml_filename: XML filename to write
        concepts: dictionary mapping lookup codes to a list of
            :class:`SnomedConcept` objects
        comment: comment for XML file
    """
    # https://stackoverflow.com/questions/3605680/creating-a-simple-xml-file-using-python
    root = ElementTree.Element(ROOT_TAG)
    comment_element = ElementTree.Comment(comment)
    root.insert(0, comment_element)
    for lookup, concepts_for_lookup in concepts.items():
        l_el = ElementTree.SubElement(
            root, LOOKUP_TAG, {LOOKUP_NAME_ATTR: lookup})
        for concept in concepts_for_lookup:
            c_el = ElementTree.SubElement(l_el, CONCEPT_TAG)
            i_el = ElementTree.SubElement(c_el, ID_TAG)
            i_el.text = str(concept.identifier)
            n_el = ElementTree.SubElement(c_el, TERM_TAG)
            n_el.text = concept.term
    tree = ElementTree.ElementTree(root)
    log.info("Writing to {!r}", xml_filename)
    tree.write(xml_filename)


# =============================================================================
# SNOMED-CT concepts for CamCOPS tasks
# =============================================================================

# -----------------------------------------------------------------------------
# CamCOPS lookup codes for SNOMED-CT concepts
# -----------------------------------------------------------------------------

class SnomedLookup(object):
    """
    We're not allowed to embed SNOMED-CT codes in the CamCOPS code. Therefore,
    within CamCOPS, we use string constants represented in this class. If the
    local institution is allowed (e.g. in the UK, as below), then it can
    install additional data.

    - UK license:
      https://isd.digital.nhs.uk/trud3/user/guest/group/0/pack/26/subpack/101/licences

    - To find codes: https://termbrowser.nhs.uk/

    Abbreviations:

    - "Finding" is not abbreviated
    - "Obs" or "observable" is short for "observable entity"
    - "Procedure" is not abbreviated
    - "Scale" is short for "assessment scale"
    - "Situation" is not abbreviated

    Variable names are designed for clear code. Value strings are designed for
    clear XML that matches SNOMED-CT, in the format TASK_CONCEPTTYPE_NAME.

    """  # noqa
    # https://snomedbrowser.com/Codes/Details/XXX  # noqa

    # -------------------------------------------------------------------------
    # SNOMED-CT core concepts
    # -------------------------------------------------------------------------

    # Abstract physical quantities
    MASS = "mass"
    LENGTH = "length"

    # Saying "units"
    UNIT_OF_MEASURE = "unit_of_measure"

    # Base physical units
    KILOGRAM = "kilogram"
    METRE = "metre"
    CENTIMETRE = "centimetre"

    # Compound physical units
    KG_PER_SQ_M = "kilogram_per_square_metre"

    # -------------------------------------------------------------------------
    # Scales
    # -------------------------------------------------------------------------

    # ACE-R
    ACE_R_SCALE = "ace_r_scale"
    ACE_R_SUBSCALE_ATTENTION_ORIENTATION = "ace_r_subscale_attention_orientation"  # noqa
    ACE_R_SUBSCALE_FLUENCY = "ace_r_subscale_fluency"
    ACE_R_SUBSCALE_LANGUAGE = "ace_r_subscale_language"
    ACE_R_SUBSCALE_MEMORY = "ace_r_subscale_memory"
    ACE_R_SUBSCALE_VISUOSPATIAL = "ace_r_subscale_visuospatial"
    ACE_R_SCORE = "ace_r_observable_score"
    ACE_R_SUBSCORE_ATTENTION_ORIENTATION = "ace_r_observable_subscore_attention_orientation"  # noqa
    ACE_R_SUBSCORE_FLUENCY = "ace_r_observable_subscore_fluency"
    ACE_R_SUBSCORE_LANGUAGE = "ace_r_observable_subscore_language"
    ACE_R_SUBSCORE_MEMORY = "ace_r_observable_subscore_memory"
    ACE_R_SUBSCORE_VISUOSPATIAL = "ace_r_observable_subscore_visuospatial"
    ACE_R_PROCEDURE_ASSESSMENT = "ace_r_procedure_assessment"
    ACE_R_PROCEDURE_ASSESSMENT_SUBSCALE_ATTENTION_ORIENTATION = "ace_r_procedure_assessment_subscale_attention_orientation"  # noqa
    ACE_R_PROCEDURE_ASSESSMENT_SUBSCALE_FLUENCY = "ace_r_procedure_assessment_subscale_fluency"  # noqa
    ACE_R_PROCEDURE_ASSESSMENT_SUBSCALE_LANGUAGE = "ace_r_procedure_assessment_subscale_language"  # noqa
    ACE_R_PROCEDURE_ASSESSMENT_SUBSCALE_MEMORY = "ace_r_procedure_assessment_subscale_memory"  # noqa
    ACE_R_PROCEDURE_ASSESSMENT_SUBSCALE_VISUOSPATIAL = "ace_r_procedure_assessment_subscale_visuospatial"  # noqa

    # AIMS
    AIMS_SCALE = "aims_scale"
    AIMS_TOTAL_SCORE = "aims_observable_total_score"
    AIMS_PROCEDURE_ASSESSMENT = "aims_procedure_assessment"

    # AUDIT, AUDIT-C
    AUDIT_SCALE = "audit_scale"
    AUDIT_SCORE = "audit_observable_score"
    AUDIT_PROCEDURE_ASSESSMENT = "audit_procedure_assessment"
    AUDITC_SCALE = "auditc_scale"
    AUDITC_SCORE = "auditc_observable_score"
    AUDITC_PROCEDURE_ASSESSMENT = "auditc_procedure_assessment"

    # BADLS
    BADLS_SCALE = "badls_scale"
    BADLS_SCORE = "badls_observable_score"
    BADLS_PROCEDURE_ASSESSMENT = "badls_procedure_assessment"

    # BDI
    BDI_SCALE = "bdi_scale"
    BDI_SCORE = "bdi_observable_score"
    BDI_PROCEDURE_ASSESSMENT = "bdi_procedure_assessment"
    BDI_II_SCORE = "bdi_ii_observable_score"
    BDI_II_PROCEDURE_ASSESSMENT = "bdi_ii_procedure_assessment"

    # BMI
    BMI_OBSERVABLE = "bmi_observable"
    BMI_PROCEDURE_MEASUREMENT = "bmi_procedure_measurement"
    BODY_HEIGHT_OBSERVABLE = "body_height_observable"
    BODY_WEIGHT_OBSERVABLE = "body_weight_observable"
    WAIST_CIRCUMFERENCE_PROCEDURE_MEASUREMENT = "waist_circumference_procedure_measurement"  # noqa
    WAIST_CIRCUMFERENCE_OBSERVABLE = "waist_circumference_observable"

    # BPRS, BPRS-E
    BPRS1962_SCALE = "bprs1962_scale"
    # no observable/procedure

    # CAGE
    CAGE_SCALE = "cage_scale"
    CAGE_SCORE = "cage_observable_score"
    CAGE_PROCEDURE_ASSESSMENT = "cage_procedure_assessment"

    # CAPE-42: none
    # CAPS: none
    # Cardinal RN, Expectation-Detection: none
    # CBI-R: none
    # CECA-Q3: none
    # CESD, CESD-R: none
    # CGI, CGI-I, CGI-SCH: none
    # CIS-R: none

    # CIWA-Ar
    CIWA_AR_SCALE = "ciwa_ar_scale"
    CIWA_AR_SCORE = "ciwa_ar_observable_score"
    CIWA_AR_PROCEDURE_ASSESSMENT = "ciwa_ar_procedure_assessment"

    # Clinical: progress note
    PROGRESS_NOTE_PROCEDURE = "progress_note_procedure"
    CLINICAL_NOTE = "clinical_note"

    # Clinical: photograph
    PHOTOGRAPH_PROCEDURE = "photograph_procedure"
    PHOTOGRAPH_PHYSICAL_OBJECT = "photograph_physical_object"

    # Clinical: psychiatric clerking
    # ... "location": not obvious
    # ... "contact type" is an AoMRC heading, but I'm not sure the observable
    #     entity of "Initial contact type" is right.
    PSYCHIATRIC_ASSESSMENT_PROCEDURE = "psychiatric_assessment_procedure"

    PSYCLERK_REASON_FOR_REFERRAL = "psyclerk_reason_for_referral"
    PSYCLERK_PRESENTING_ISSUE = "psyclerk_presenting_issue"
    PSYCLERK_SYSTEMS_REVIEW = "psyclerk_systems_review"
    PSYCLERK_COLLATERAL_HISTORY = "psyclerk_collateral_history"

    PSYCLERK_PAST_MEDICAL_SURGICAL_MENTAL_HEALTH_HISTORY = "psyclerk_past_medical_surgical_mental_health_history"  # noqa
    PSYCLERK_PROCEDURES = "psyclerk_procedures"
    PSYCLERK_ALLERGIES_ADVERSE_REACTIONS = "psyclerk_allergies_adverse_reactions"  # noqa
    PSYCLERK_MEDICATIONS_MEDICAL_DEVICES = "psyclerk_medications_medical_devices"  # noqa
    PSYCLERK_DRUG_SUBSTANCE_USE = "psyclerk_drug_substance_use"
    PSYCLERK_FAMILY_HISTORY = "psyclerk_family_history"
    PSYCLERK_DEVELOPMENTAL_HISTORY = "psyclerk_developmental_history"
    PSYCLERK_SOCIAL_PERSONAL_HISTORY = "psyclerk_social_personal_history"
    PSYCLERK_PERSONALITY = "psyclerk_personality"
    PSYCLERK_PRISON_RECORD_CRIMINAL_ACTIVITY = "psyclerk_prison_record_criminal_activity"  # noqa
    PSYCLERK_SOCIAL_HISTORY_BASELINE = "psyclerk_social_history_baseline"

    PSYCLERK_MSE_APPEARANCE = "psyclerk_mse_appearance"
    PSYCLERK_MSE_BEHAVIOUR = "psyclerk_mse_behaviour"
    PSYCLERK_MSE_SPEECH = "psyclerk_mse_speech"
    PSYCLERK_MSE_MOOD = "psyclerk_mse_mood"
    PSYCLERK_MSE_AFFECT = "psyclerk_mse_affect"
    PSYCLERK_MSE_THOUGHT = "psyclerk_mse_thought"
    PSYCLERK_MSE_PERCEPTION = "psyclerk_mse_perception"
    PSYCLERK_MSE_COGNITION = "psyclerk_mse_cognition"
    PSYCLERK_MSE_INSIGHT = "psyclerk_mse_insight"

    PSYCLERK_PHYSEXAM_GENERAL = "psyclerk_physexam_general"
    PSYCLERK_PHYSEXAM_CARDIOVASCULAR = "psyclerk_physexam_cardiovascular"
    PSYCLERK_PHYSEXAM_RESPIRATORY = "psyclerk_physexam_respiratory"
    PSYCLERK_PHYSEXAM_ABDOMINAL = "psyclerk_physexam_abdominal"
    PSYCLERK_PHYSEXAM_NEUROLOGICAL = "psyclerk_physexam_neurological"

    PSYCLERK_ASSESSMENT_SCALES = "psyclerk_assessment_scales"
    PSYCLERK_INVESTIGATIONS_RESULTS = "psyclerk_investigations_results"

    PSYCLERK_SAFETY_ALERTS = "psyclerk_safety_alerts"
    PSYCLERK_RISK_ASSESSMENT = "psyclerk_risk_assessment"
    PSYCLERK_RELEVANT_LEGAL_INFORMATION = "psyclerk_relevant_legal_information"

    PSYCLERK_CURRENT_PROBLEMS = "psyclerk_current_problems"
    PSYCLERK_PATIENT_CARER_CONCERNS = "psyclerk_patient_carer_concerns"
    PSYCLERK_CLINICAL_NARRATIVE = "psyclerk_clinical_narrative"
    PSYCLERK_MANAGEMENT_PLAN = "psyclerk_management_plan"
    PSYCLERK_INFORMATION_GIVEN = "psyclerk_information_given"

    # CPFT/CUH LPS: none
    # COPE: none

    # CORE-10
    CORE10_SCALE = "core10_scale"
    CORE10_SCORE = "core10_observable_score"
    CORE10_PROCEDURE_ASSESSMENT = "core10_procedure_assessment"

    # DAD: none

    # DAST
    DAST_SCALE = "dast_scale"
    # I think the similarly named codes represent urine screening.

    # Deakin JB, antibody: none
    # Demo questionnaire: none
    # DEMQOL, DEMQOL-Proxy: none
    # Distress Thermometer: none

    # EQ-5D-5L
    EQ5D5L_SCALE = "eq5d5l_scale"
    EQ5D5L_INDEX_VALUE = "eq5d5l_observable_index_value"
    EQ5D5L_PAIN_DISCOMFORT_SCORE = "eq5d5l_observable_pain_discomfort_score"
    EQ5D5L_USUAL_ACTIVITIES_SCORE = "eq5d5l_observable_usual_activities_score"
    EQ5D5L_ANXIETY_DEPRESSION_SCORE = "eq5d5l_observable_anxiety_depression_score"  # noqa
    EQ5D5L_MOBILITY_SCORE = "eq5d5l_observable_mobility_score"
    EQ5D5L_SELF_CARE_SCORE = "eq5d5l_observable_self_care_score"
    EQ5D5L_PROCEDURE_ASSESSMENT = "eq5d5l_procedure_assessment"

    # FACT-G: none

    # FAST: none
    FAST_SCALE = "fast_scale"
    FAST_SCORE = "fast_observable_score"
    FAST_PROCEDURE_ASSESSMENT = "fast_procedure_assessment"

    # IRAC: none
    # FFT: none

    # Patient Satisfaction Scale
    # Not sure. See XML

    # Referrer Satisfaction Scale (patient-specific, survey): none
    # Frontotemporal Dementia Rating Scale: none

    # GAD-7
    GAD7_SCALE = "gad7_scale"
    GAD7_SCORE = "gad7_observable_score"
    GAD7_PROCEDURE_ASSESSMENT = "gad7_procedure_assessment"

    # GAF
    GAF_SCALE = "gaf_scale"
    # no observable/procedure

    # GDS-15
    GDS15_SCALE = "gds15_scale"
    GDS15_SCORE = "gds15_observable_score"
    GDS15_PROCEDURE_ASSESSMENT = "gds15_procedure_assessment"

    # UK GMC Patient Questionnaire: none

    # HADS, HADS-respondent
    HADS_SCALE = "hads_scale"
    HADS_ANXIETY_SCORE = "hads_observable_anxiety_score"
    HADS_DEPRESSION_SCORE = "hads_observable_depression_score"
    HADS_PROCEDURE_ASSESSMENT = "hads_procedure_assessment"

    # HAMA: none

    # HAMD
    HAMD_SCALE = "hamd_scale"
    HAMD_SCORE = "hamd_observable_score"
    HAMD_PROCEDURE_ASSESSMENT = "hamd_procedure_assessment"

    # HAMD-7: none

    # HoNOS, HoNOS-65+, HoNOSCA, etc. (there are others too)
    HONOSCA_SCALE = "honos_childrenadolescents_scale"
    HONOSCA_SECTION_A_SCALE = "honos_childrenadolescents_section_a_scale"
    HONOSCA_SECTION_B_SCALE = "honos_childrenadolescents_section_b_scale"
    HONOSCA_SCORE = "honos_childrenadolescents_observable_score"
    HONOSCA_SECTION_A_SCORE = "honos_childrenadolescents_observable_section_a_score"  # noqa
    HONOSCA_SECTION_B_SCORE = "honos_childrenadolescents_observable_section_b_score"  # noqa
    HONOSCA_SECTION_A_PLUS_B_SCORE = "honos_childrenadolescents_observable_section_a_plus_b_score"  # noqa
    HONOSCA_PROCEDURE_ASSESSMENT = "honos_childrenadolescents_procedure_assessment"  # noqa
    #
    HONOS65_SCALE = "honos_olderadults_scale"
    HONOS65_SCORE = "honos_olderadults_observable_score"
    HONOS65_PROCEDURE_ASSESSMENT = "honos_olderadults_procedure_assessment"
    #
    HONOSWA_SCALE = "honos_workingage_scale"
    HONOSWA_SUBSCALE_1_OVERACTIVE = "honos_workingage_subscale_1_overactive"
    HONOSWA_SUBSCALE_2_SELFINJURY = "honos_workingage_subscale_2_selfinjury"
    HONOSWA_SUBSCALE_3_SUBSTANCE = "honos_workingage_subscale_3_substance"
    HONOSWA_SUBSCALE_4_COGNITIVE = "honos_workingage_subscale_4_cognitive"
    HONOSWA_SUBSCALE_5_PHYSICAL = "honos_workingage_subscale_5_physical"
    HONOSWA_SUBSCALE_6_PSYCHOSIS = "honos_workingage_subscale_6_psychosis"
    HONOSWA_SUBSCALE_7_DEPRESSION = "honos_workingage_subscale_7_depression"
    HONOSWA_SUBSCALE_8_OTHERMENTAL = "honos_workingage_subscale_8_othermental"
    HONOSWA_SUBSCALE_9_RELATIONSHIPS = "honos_workingage_subscale_9_relationships"  # noqa
    HONOSWA_SUBSCALE_10_ADL = "honos_workingage_subscale_10_adl"
    HONOSWA_SUBSCALE_11_LIVINGCONDITIONS = "honos_workingage_subscale_11_livingconditions"  # noqa
    HONOSWA_SUBSCALE_12_OCCUPATION = "honos_workingage_subscale_12_occupation"
    HONOSWA_SCORE = "honos_workingage_observable_score"
    HONOSWA_1_OVERACTIVE_SCORE = "honos_workingage_observable_1_overactive_score"  # noqa
    HONOSWA_2_SELFINJURY_SCORE = "honos_workingage_observable_2_selfinjury_score"  # noqa
    HONOSWA_3_SUBSTANCE_SCORE = "honos_workingage_observable_3_substance_score"
    HONOSWA_4_COGNITIVE_SCORE = "honos_workingage_observable_4_cognitive_score"
    HONOSWA_5_PHYSICAL_SCORE = "honos_workingage_observable_5_physical_score"
    HONOSWA_6_PSYCHOSIS_SCORE = "honos_workingage_observable_6_psychosis_score"
    HONOSWA_7_DEPRESSION_SCORE = "honos_workingage_observable_7_depression_score"  # noqa
    HONOSWA_8_OTHERMENTAL_SCORE = "honos_workingage_observable_8_othermental_score"  # noqa
    HONOSWA_9_RELATIONSHIPS_SCORE = "honos_workingage_observable_9_relationships_score"  # noqa
    HONOSWA_10_ADL_SCORE = "honos_workingage_observable_10_adl_score"
    HONOSWA_11_LIVINGCONDITIONS_SCORE = "honos_workingage_observable_11_livingconditions_score"  # noqa
    HONOSWA_12_OCCUPATION_SCORE = "honos_workingage_observable_12_occupation_score"  # noqa
    HONOSWA_PROCEDURE_ASSESSMENT = "honos_workingage_procedure_assessment"

    # ICD-9-CM: see below; separate file

    # ICD-10: see below; separate file

    # IDED3D: none

    # IES-R
    IESR_SCALE = "iesr_scale"
    IESR_SCORE = "iesr_observable_score"
    IESR_PROCEDURE_ASSESSMENT = "iesr_procedure_assessment"

    # INECO Frontal Screening: none
    # Khandaker G, Insight: none

    # MAST, SMAST
    MAST_SCALE = "mast_scale"
    MAST_SCORE = "mast_observable_score"
    MAST_PROCEDURE_ASSESSMENT = "mast_procedure_assessment"

    SMAST_SCALE = "smast_scale"
    # SMAST: no observable/procedure

    # MDS-UPDRS
    UPDRS_SCALE = "updrs_scale"
    # MDS-UPDRS: no observable/procedure

    # MoCA
    MOCA_SCALE = "moca_scale"
    MOCA_SCORE = "moca_observable_score"
    MOCA_PROCEDURE_ASSESSMENT = "moca_procedure_assessment"

    # NART
    NART_SCALE = "nart_scale"
    NART_SCORE = "nart_observable_score"
    NART_PROCEDURE_ASSESSMENT = "nart_procedure_assessment"

    # NPI-Q: none

    # PANSS
    PANSS_SCALE = "panss_scale"
    # PANSS: no observable/procedure

    # PCL and variants: none

    # PDSS
    PDSS_SCALE = "pdss_scale"
    PDSS_SCORE = "pdss_observable_score"
    # PDSS: no procedure

    # "Perinatal" and "scale": none

    # PHQ-9
    PHQ9_FINDING_NEGATIVE_SCREENING_FOR_DEPRESSION = "phq9_finding_negative_screening_for_depression"  # noqa
    PHQ9_FINDING_POSITIVE_SCREENING_FOR_DEPRESSION = "phq9_finding_positive_screening_for_depression"  # noqa
    PHQ9_SCORE = "phq9_observable_score"
    PHQ9_PROCEDURE_DEPRESSION_SCREENING = "phq9_procedure_depression_screening"  # noqa
    PHQ9_SCALE = "phq9_scale"  # https://snomedbrowser.com/Codes/Details/758711000000105  # noqa

    # PHQ-15
    PHQ15_SCORE = "phq15_observable_score"
    PHQ15_PROCEDURE = "phq15_procedure_assessment"
    PHQ15_SCALE = "phq15_scale"

    # PIIOS or "parent infant scale": none

    # PSWQ
    PSWQ_SCALE = "pswq_scale"
    PSWQ_SCORE = "pswq_observable_score"
    PSWQ_PROCEDURE_ASSESSMENT = "pswq_procedure_assessment"

    # QOL-Basic, QOL-SG
    # Unsure, but there is this:
    QOL_SCALE = "qol_scale"
    # ... with no observable/procedure

    # RAND-36: none directly

    # SLUMS: none

    # WEMWBS, SWEMWBS
    WEMWBS_SCALE = "wemwbs_scale"
    WEMWBS_SCORE = "wemwbs_observable_score"
    WEMWBS_PROCEDURE_ASSESSMENT = "wemwbs_procedure_assessment"
    #
    SWEMWBS_SCALE = "swemwbs_scale"
    SWEMWBS_SCORE = "swemwbs_observable_score"
    SWEMWBS_PROCEDURE_ASSESSMENT = "swemwbs_procedure_assessment"

    # WSAS
    WSAS_SCALE = "wsas_scale"
    WSAS_SCORE = "wsas_observable_score"
    WSAS_WORK_SCORE = "wsas_observable_work_score"
    WSAS_RELATIONSHIPS_SCORE = "wsas_observable_relationships_score"
    WSAS_HOME_MANAGEMENT_SCORE = "wsas_observable_home_management_score"
    WSAS_SOCIAL_LEISURE_SCORE = "wsas_observable_social_leisure_score"
    WSAS_PRIVATE_LEISURE_SCORE = "wsas_observable_private_leisure_score"
    WSAS_PROCEDURE_ASSESSMENT = "wsas_procedure_assessment"

    # Y-BOCS, Y-BOCS-SC: none
    # ZBI: none


# -----------------------------------------------------------------------------
# Perform the lookup
# -----------------------------------------------------------------------------

VALID_SNOMED_LOOKUPS = set([getattr(SnomedLookup, k) for k in dir(SnomedLookup)
                            if not k.startswith("_")])


@cache_region_static.cache_on_arguments(function_key_generator=fkg)
def get_all_task_snomed_concepts(xml_filename: str) \
        -> Dict[str, SnomedConcept]:
    """
    Reads in all SNOMED-CT codes for CamCOPS tasks, from the custom CamCOPS XML
    file for this.

    Args:
        xml_filename: XML filename to read

    Returns:
        dict: maps lookup strings to :class:`SnomedConcept` objects

    """
    xml_concepts = get_snomed_concepts_from_xml(xml_filename)
    camcops_concepts = {}  # type: Dict[str, SnomedConcept]
    identifiers_seen = set()  # type: Set[int]
    for lookup, concepts in xml_concepts.items():
        # Check it
        if lookup not in VALID_SNOMED_LOOKUPS:
            log.debug("Ignoring unknown SNOMED-CT lookup: {!r}", lookup)
            continue
        assert len(concepts) == 1, (
            f"More than one SNOMED-CT concept for lookup: {lookup!r}"
        )
        concept = concepts[0]
        assert concept.identifier not in identifiers_seen, (
            f"Duplicate SNOMED-CT identifier: {concept.identifier!r}")
        identifiers_seen.add(concept.identifier)
        # Stash it
        camcops_concepts[lookup] = concept
    # Check if any are missing
    missing = sorted(list(VALID_SNOMED_LOOKUPS - set(camcops_concepts.keys())))
    if missing:
        raise ValueError(
            f"The following SNOMED-CT concepts required by CamCOPS are "
            f"missing from the XML ({xml_filename!r}): {missing!r}")
    # Done
    return camcops_concepts


# =============================================================================
# UMLS ICD-9-CM
# =============================================================================

class UmlsIcd9SnomedRow(object):
    """
    Simple information-holding class for a row of the ICD-9-CM TSV file, from
    https://www.nlm.nih.gov/research/umls/mapping_projects/icd9cm_to_snomedct.html.

    NOT CURRENTLY USED.
    """
    HEADER = [
        "ICD_CODE", "ICD_NAME", "IS_CURRENT_ICD", "IP_USAGE", "OP_USAGE",
        "AVG_USAGE", "IS_NEC", "SNOMED_CID", "SNOMED_FSN", "IS_1-1MAP",
        "CORE_USAGE", "IN_CORE",
    ]

    @staticmethod
    def to_float(x: str) -> Optional[float]:
        return None if x == "NULL" else float(x)

    def __init__(self,
                 icd_code: str,
                 icd_name: str,
                 is_current_icd: str,
                 ip_usage: str,
                 op_usage: str,
                 avg_usage: str,
                 is_nec: str,
                 snomed_cid: str,
                 snomed_fsn: str,
                 is_one_to_one_map: str,
                 core_usage: str,
                 in_core: str) -> None:
        """
        Argument order is important.

        Args:
            icd_code: ICD-9-CM code
            icd_name: Name of ICD-9-CM entity
            is_current_icd: ?
            ip_usage: ?
            op_usage: ?
            avg_usage: ?
            is_nec: ?
            snomed_cid: SNOMED-CT concept ID
            snomed_fsn: SNOMED-CT fully specified name
            is_one_to_one_map: ?; possibly always true in this dataset but not
                true in a broader dataset including things other than 1:1
                mappings?
            core_usage: ?
            in_core: ?
        """
        self.icd_code = icd_code
        self.icd_name = icd_name
        self.is_current_icd = bool(int(is_current_icd))
        self.ip_usage = self.to_float(ip_usage)
        self.op_usage = self.to_float(op_usage)
        self.avg_usage = self.to_float(avg_usage)
        self.is_nec = bool(int(is_nec))
        self.snomed_cid = int(snomed_cid)
        self.snomed_fsn = snomed_fsn
        self.is_one_to_one_map = bool(int(is_one_to_one_map))
        self.core_usage = self.to_float(core_usage)
        self.in_core = bool(int(in_core))

    def __repr__(self) -> str:
        return simple_repr(self, [
            "icd_code", "icd_name", "is_current_icd", "ip_usage", "op_usage",
            "avg_usage", "is_nec",
            "snomed_cid", "snomed_fsn",
            "is_one_to_one_map", "core_usage", "in_core",
        ])

    def snomed_concept(self) -> SnomedConcept:
        """
        Returns the associated SNOMED-CT concept.
        """
        return SnomedConcept(self.snomed_cid, self.snomed_fsn)

    def __str__(self) -> str:
        return (
            f"ICD-9-CM {self.icd_code} ({self.icd_name}) "
            f"-> SNOMED-CT {self.snomed_concept()}"
        )


@cache_region_static.cache_on_arguments(function_key_generator=fkg)
def get_all_icd9cm_snomed_concepts_from_umls(
        tsv_filename: str) -> Dict[str, SnomedConcept]:
    """
    Reads in all ICD-9-CM SNOMED-CT codes that are supported by the client,
    from the UMLS data file, from
    https://www.nlm.nih.gov/research/umls/mapping_projects/icd9cm_to_snomedct.html.

    Args:
        tsv_filename: TSV filename to read

    Returns:
        dict: maps lookup strings to :class:`SnomedConcept` objects

    NOT CURRENTLY USED.
    """
    log.info("Loading SNOMED-CT ICD-9-CM codes from file: {}", tsv_filename)
    concepts = {}  # type: Dict[str, SnomedConcept]
    with open(tsv_filename, 'r') as tsvin:
        reader = csv.reader(tsvin, delimiter="\t")
        header = next(reader, None)
        if header != UmlsIcd9SnomedRow.HEADER:
            raise ValueError(
                f"ICD-9-CM TSV file has unexpected header: {header!r}; "
                f"expected {UmlsIcd9SnomedRow.HEADER!r}")
        for row in reader:
            entry = UmlsIcd9SnomedRow(*row)
            if entry.icd_code not in CLIENT_ICD9CM_CODES:
                continue
            if not entry.is_one_to_one_map:
                continue
            if entry.icd_code in concepts:
                log.warning(
                    "Duplicate ICD-9-CM code found in SNOMED file {!r}: {!r}",
                    tsv_filename, entry.icd_code)
                continue
            concept = entry.snomed_concept()
            # log.debug("{}", entry)
            concepts[entry.icd_code] = concept
    missing = CLIENT_ICD9CM_CODES - set(concepts.keys())
    if missing:
        log.info("No SNOMED-CT codes for ICD-9-CM codes: {}",
                 ", ".join(sorted(missing)))
    return concepts


# =============================================================================
# UMLS ICD-10
# =============================================================================

class UmlsSnomedToIcd10Row(object):
    """
    Simple information-holding class for a row of the ICD-10-CM TSV file from
    https://www.nlm.nih.gov/research/umls/mapping_projects/snomedct_to_icd10cm.html.

    However, that is unhelpful (many to one).

    NOT CURRENTLY USED.
    """
    HEADER = [
        "id", "effectiveTime", "active", "moduleId", "refsetId",
        "referencedComponentId", "referencedComponentName", "mapGroup",
        "mapPriority", "mapRule", "mapAdvice", "mapTarget", "mapTargetName",
        "correlationId", "mapCategoryId", "mapCategoryName",
    ]
    MAP_GOOD = "MAP SOURCE CONCEPT IS PROPERLY CLASSIFIED"

    def __init__(self,
                 row_id: str,
                 effective_time: str,
                 active: str,
                 module_id: str,
                 refset_id: str,
                 referenced_component_id: str,
                 referenced_component_name: str,
                 map_group: str,
                 map_priority: str,
                 map_rule: str,
                 map_advice: str,
                 map_target: str,
                 map_target_name: str,
                 correlation_id: str,
                 map_category_id: str,
                 map_category_name: str) -> None:
        """
        Argument order is important.

        Args:
            row_id: UUID format or similar
            effective_time: date in YYYYMMDD format
            active: ?
            module_id: ?
            refset_id: ?
            referenced_component_id: SNOMED-CT concept ID
            referenced_component_name: SNOMED-CT concept name
            map_group: ?; e.g. 1
            map_priority: ? but e.g. 1, 2; correlates with map_rule
            map_rule: ?; e.g. "TRUE"; "OTHERWISE TRUE"
            map_advice: ?, but e.g. "ALWAYS F32.2" or "ALWAYS F32.2 |
                DESCENDANTS NOT EXHAUSTIVELY MAPPED"
            map_target: ICD-10 code
            map_target_name: ICD-10 name
            correlation_id: a SNOMED-CT concept for the mapping, e.g.
                447561005 = "SNOMED CT source code to target map code
                correlation not specified (foundation metadata concept)"
            map_category_id: a SNOMED-CT concept for the mapping, e.g.
                447637006 = "Map source concept is properly classified
                (foundation metadata concept)"
            map_category_name: SNOMED-CT name corresponding to map_category_id,
                e.g. "MAP SOURCE CONCEPT IS PROPERLY CLASSIFIED"
        """
        self.row_id = row_id
        self.effective_time = effective_time
        self.active = bool(int(active))
        self.module_id = int(module_id)
        self.refset_id = int(refset_id)
        self.referenced_component_id = int(referenced_component_id)
        self.referenced_component_name = referenced_component_name
        self.map_group = int(map_group)
        self.map_priority = int(map_priority)
        self.map_rule = map_rule
        self.map_advice = map_advice
        self.map_target = map_target
        self.map_target_name = map_target_name
        self.correlation_id = int(correlation_id)
        self.map_category_id = int(map_category_id)
        self.map_category_name = map_category_name

    def __repr__(self) -> str:
        return simple_repr(self, [
            "row_id", "effective_time", "active", "module_id", "refset_id",
            "referenced_component_id", "referenced_component_name",
            "map_group", "map_priority", "map_rule", "map_advice",
            "map_target", "map_target_name",
            "correlation_id", "map_category_id", "map_category_name",
        ])

    def snomed_concept(self) -> SnomedConcept:
        """
        Returns the associated SNOMED-CT concept.
        """
        return SnomedConcept(self.referenced_component_id,
                             self.referenced_component_name)

    @property
    def icd_code(self) -> str:
        return self.map_target

    @property
    def icd_name(self) -> str:
        return self.map_target_name

    def __str__(self) -> str:
        return (
            f"ICD-10 {self.icd_code} ({self.icd_name}) "
            f"-> SNOMED-CT {self.snomed_concept()}"
        )


def get_all_icd10_snomed_concepts_from_umls(
        tsv_filename: str) -> Dict[str, SnomedConcept]:
    """
    Reads in all ICD-10 SNOMED-CT codes that are supported by the client,
    from the UMLS data file, from
    https://www.nlm.nih.gov/research/umls/mapping_projects/snomedct_to_icd10cm.html.

    Args:
        tsv_filename: TSV filename to read

    Returns:
        dict: maps lookup strings to :class:`SnomedConcept` objects

    NOT CURRENTLY USED.
    """
    log.info("Loading SNOMED-CT ICD-10-CM codes from file: {}", tsv_filename)
    concepts = {}  # type: Dict[str, SnomedConcept]
    with open(tsv_filename, 'r') as tsvin:
        reader = csv.reader(tsvin, delimiter="\t")
        header = next(reader, None)
        if header != UmlsSnomedToIcd10Row.HEADER:
            raise ValueError(
                f"ICD-9-CM TSV file has unexpected header: {header!r}; "
                f"expected {UmlsSnomedToIcd10Row.HEADER!r}")
        for row in reader:
            entry = UmlsSnomedToIcd10Row(*row)
            if entry.icd_code not in CLIENT_ICD10_CODES:
                continue
            if entry.icd_code in concepts:
                log.warning(
                    "Duplicate ICD-10-CM code found in SNOMED file {!r}: {!r}",
                    tsv_filename, entry.icd_code)
                continue
            concept = entry.snomed_concept()
            # log.debug("{}", entry)
            concepts[entry.icd_code] = concept
    missing = CLIENT_ICD10_CODES - set(concepts.keys())
    if missing:
        log.info("No SNOMED-CT codes for ICD-10 codes: {}",
                 ", ".join(sorted(missing)))
    return concepts


# =============================================================================
# Athena OHDSI mapping
# =============================================================================

# -----------------------------------------------------------------------------
# Fetch ICD-9-CM and ICD-10 codes (shared for fewer passes through the files)
# -----------------------------------------------------------------------------

def get_icd9cm_icd10_snomed_concepts_from_athena(
        athena_concept_tsv_filename: str,
        athena_concept_relationship_tsv_filename: str) \
        -> Tuple[Dict[str, List[SnomedConcept]],
                 Dict[str, List[SnomedConcept]]]:
    """
    Takes Athena concept and concept-relationship files, and fetches details
    of SNOMED-CT code for all ICD-9-CM and ICD-10[-CM] codes used by CamCOPS.

    A bit of human review is required; this is probably preferable to using
    gensim or some other automatic similarity check.

    Args:
        athena_concept_tsv_filename:
            path to ``CONCEPT.csv`` (a tab-separated value file)
        athena_concept_relationship_tsv_filename:
            path to ``CONCEPT_RELATIONSHIP.csv`` (a tab-separated value file)

    Returns:
        tuple: ``icd9cm, icd10``, where each is a dictionary mapping ICD codes
        to a list of mapped :class:`SnomedConcept` objects.

    """
    athena_icd_concepts = get_athena_concepts(
        tsv_filename=athena_concept_tsv_filename,
        vocabulary_ids={AthenaVocabularyId.ICD9CM, AthenaVocabularyId.ICD10CM},
        concept_codes=CLIENT_ICD9CM_CODES | CLIENT_ICD10_CODES,
    )
    athena_icd_concepts.sort(key=lambda x: x.concept_code)
    relationships = get_athena_concept_relationships(
        tsv_filename=athena_concept_relationship_tsv_filename,
        concept_id_1_values=set(x.concept_id for x in athena_icd_concepts),
        relationship_id_values={AthenaRelationshipId.MAPS_TO}
    )
    athena_snomed_concepts = get_athena_concepts(
        tsv_filename=athena_concept_tsv_filename,
        vocabulary_ids={AthenaVocabularyId.SNOMED},
        concept_ids=set(x.concept_id_2 for x in relationships)
    )
    snomed_concepts_icd9 = OrderedDict()  # type: Dict[str, List[SnomedConcept]]  # noqa
    snomed_concepts_icd10 = OrderedDict()  # type: Dict[str, List[SnomedConcept]]  # noqa
    for icd in athena_icd_concepts:
        target = (
            snomed_concepts_icd9
            if icd.vocabulary_id == AthenaVocabularyId.ICD9CM
            else snomed_concepts_icd10
        )
        icd_code = icd.concept_code
        # log.debug("Processing icd = {}", icd)
        possible_snomed = []  # type: List[AthenaConceptRow]
        for rel in relationships:
            if rel.concept_id_1 != icd.concept_id:
                continue
            # log.debug("Processing rel = {}", rel)
            for snomed in athena_snomed_concepts:
                if snomed.concept_id != rel.concept_id_2:
                    continue
                # log.debug("Processing snomed = {}", snomed)
                possible_snomed.append(snomed)
        if possible_snomed:
            sclist = [
                SnomedConcept.create(s.snomed_concept())
                for s in possible_snomed
            ]
            sclist.sort(key=lambda sc: sc.identifier)
            log.debug("Mapping {} -> {}", icd, sclist)
            target[icd_code] = sclist
        else:
            log.debug("No SNOMED code found for {}", icd)
    return snomed_concepts_icd9, snomed_concepts_icd10


# -----------------------------------------------------------------------------
# Fetch codes from Athena data set and write them to CamCOPS XML
# -----------------------------------------------------------------------------

def send_athena_icd_snomed_to_xml(
        athena_concept_tsv_filename: str,
        athena_concept_relationship_tsv_filename: str,
        icd9_xml_filename: str,
        icd10_xml_filename) -> None:
    """
    Reads SNOMED-CT codes for ICD-9-CM and ICD10 from Athena OHDSI files, and
    writes

    Args:
        athena_concept_tsv_filename:
            path to ``CONCEPT.csv`` (a tab-separated value file)
        athena_concept_relationship_tsv_filename:
            path to ``CONCEPT_RELATIONSHIP.csv`` (a tab-separated value file)
        icd9_xml_filename:
            ICD-9 XML filename to write
        icd10_xml_filename:
            ICD-10 XML filename to write
    """
    icd9, icd10 = get_icd9cm_icd10_snomed_concepts_from_athena(
        athena_concept_tsv_filename=athena_concept_tsv_filename,
        athena_concept_relationship_tsv_filename=athena_concept_relationship_tsv_filename  # noqa
    )
    write_snomed_concepts_to_xml(icd9_xml_filename, icd9)
    write_snomed_concepts_to_xml(icd10_xml_filename, icd10)


# -----------------------------------------------------------------------------
# Fetch data from XML
# -----------------------------------------------------------------------------

def get_multiple_snomed_concepts_from_xml(xml_filename: str,
                                          valid_lookups: Set[str] = None,
                                          require_all: bool = False) \
        -> Dict[str, List[SnomedConcept]]:
    """
    Reads in all SNOMED-CT codes for ICD-9 or ICD-10, from the custom CamCOPS
    XML file for this (made by e.g. :func:`send_athena_icd_snomed_to_xml`).

    Args:
        xml_filename: XML filename to read
        valid_lookups: possible lookup values
        require_all: require that ``valid_lookups`` is truthy and that all
            values in it are present in the XML

    Returns:
        dict: maps lookup strings to lists of :class:`SnomedConcept` objects

    """
    valid_lookups = set(valid_lookups or [])  # type: Set[str]
    xml_concepts = get_snomed_concepts_from_xml(xml_filename)
    camcops_concepts = {}  # type: Dict[str, List[SnomedConcept]]
    for lookup, concepts in xml_concepts.items():
        # Check it
        if valid_lookups and lookup not in valid_lookups:
            log.debug("Ignoring unknown SNOMED-CT lookup: {!r}", lookup)
            continue
        # Stash it
        camcops_concepts[lookup] = concepts
    if require_all:
        # Check if any are missing
        assert valid_lookups, "require_all specified but valid_lookups missing"
    if valid_lookups:
        missing = sorted(list(valid_lookups - set(camcops_concepts.keys())))
        if missing:
            msg = (
                f'The following {"required " if require_all else ""}'
                f'SNOMED-CT concepts are missing from the XML '
                f'({xml_filename!r}): {missing!r}'
            )
            if require_all:
                log.critical(msg)
                raise ValueError(msg)
            else:
                log.debug(msg)
    # Done
    return camcops_concepts


@cache_region_static.cache_on_arguments(function_key_generator=fkg)
def get_icd9_snomed_concepts_from_xml(xml_filename: str) \
        -> Dict[str, List[SnomedConcept]]:
    """
    Reads in all ICD-9-CM SNOMED-CT codes from a custom CamCOPS XML file.

    Args:
        xml_filename: filename to read

    Returns:
        dict: maps ICD-9-CM codes to lists of :class:`SnomedConcept` objects
    """
    return get_multiple_snomed_concepts_from_xml(xml_filename,
                                                 CLIENT_ICD9CM_CODES)


@cache_region_static.cache_on_arguments(function_key_generator=fkg)
def get_icd10_snomed_concepts_from_xml(xml_filename: str) \
        -> Dict[str, List[SnomedConcept]]:
    """
    Reads in all ICD-10 SNOMED-CT codes from a custom CamCOPS XML file.

    Args:
        xml_filename: filename to read

    Returns:
        dict: maps ICD-10 codes to lists of :class:`SnomedConcept` objects
    """
    return get_multiple_snomed_concepts_from_xml(xml_filename,
                                                 CLIENT_ICD10_CODES)
