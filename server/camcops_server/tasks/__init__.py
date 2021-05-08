#!/usr/bin/env python

"""
camcops_server/tasks/__init__.py

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

from camcops_server.tasks.ace3 import Ace3
from camcops_server.tasks.aims import Aims
from camcops_server.tasks.apeq_cpft_perinatal import APEQCPFTPerinatal
from camcops_server.tasks.apeqpt import Apeqpt
from camcops_server.tasks.asdas import Asdas
from camcops_server.tasks.audit import Audit, AuditC

from camcops_server.tasks.badls import Badls
from camcops_server.tasks.basdai import Basdai
from camcops_server.tasks.bdi import Bdi
from camcops_server.tasks.bmi import Bmi
from camcops_server.tasks.bprs import Bprs
from camcops_server.tasks.bprse import Bprse

from camcops_server.tasks.cage import Cage
from camcops_server.tasks.cape42 import Cape42
from camcops_server.tasks.caps import Caps
from camcops_server.tasks.cardinal_expdetthreshold import (
    CardinalExpDetThreshold,
)
from camcops_server.tasks.cardinal_expectationdetection import (
    CardinalExpectationDetection,
)
from camcops_server.tasks.cbir import CbiR
from camcops_server.tasks.ceca import CecaQ3
from camcops_server.tasks.cesd import Cesd
from camcops_server.tasks.cesdr import Cesdr
from camcops_server.tasks.cgi_task import Cgi
from camcops_server.tasks.cgisch import CgiSch
from camcops_server.tasks.chit import Chit
from camcops_server.tasks.cisr import Cisr
from camcops_server.tasks.ciwa import Ciwa
from camcops_server.tasks.contactlog import ContactLog
from camcops_server.tasks.cope import CopeBrief
from camcops_server.tasks.core10 import Core10
from camcops_server.tasks.cpft_lps import (
    CPFTLPSReferral,
    CPFTLPSResetResponseClock,
    CPFTLPSDischarge,
)
# todo: # from camcops_server.tasks.ctqsf import Ctqsf

from camcops_server.tasks.dad import Dad
from camcops_server.tasks.das28 import Das28
from camcops_server.tasks.dast import Dast
from camcops_server.tasks.deakin_s1_healthreview import DeakinS1HealthReview
from camcops_server.tasks.demoquestionnaire import DemoQuestionnaire
from camcops_server.tasks.demqol import Demqol
from camcops_server.tasks.diagnosis import DiagnosisIcd9CM, DiagnosisIcd10
from camcops_server.tasks.distressthermometer import DistressThermometer

from camcops_server.tasks.elixhauserci import ElixhauserCI
from camcops_server.tasks.epds import Epds
from camcops_server.tasks.eq5d5l import Eq5d5l
from camcops_server.tasks.esspri import Esspri

from camcops_server.tasks.factg import Factg
from camcops_server.tasks.fast import Fast
from camcops_server.tasks.fft import Fft
from camcops_server.tasks.frs import Frs

from camcops_server.tasks.gad7 import Gad7
from camcops_server.tasks.gaf import Gaf
from camcops_server.tasks.gbo import Gbogres, Gbogpc
from camcops_server.tasks.gds import Gds15
from camcops_server.tasks.gmcpq import GMCPQ

from camcops_server.tasks.hads import Hads
from camcops_server.tasks.hama import Hama
from camcops_server.tasks.hamd import Hamd
from camcops_server.tasks.hamd7 import Hamd7
from camcops_server.tasks.honos import Honos, Honos65, Honosca

from camcops_server.tasks.icd10depressive import Icd10Depressive
from camcops_server.tasks.icd10manic import Icd10Manic
from camcops_server.tasks.icd10mixed import Icd10Mixed
from camcops_server.tasks.icd10schizophrenia import Icd10Schizophrenia
from camcops_server.tasks.icd10schizotypal import Icd10Schizotypal
from camcops_server.tasks.icd10specpd import Icd10SpecPD
from camcops_server.tasks.ided3d import IDED3D
from camcops_server.tasks.iesr import Iesr
from camcops_server.tasks.ifs import Ifs
from camcops_server.tasks.irac import Irac

from camcops_server.tasks.khandaker_insight_medical import (
    KhandakerInsightMedical,
)
from camcops_server.tasks.khandaker_mojo_medical import (
    KhandakerMojoMedical,
)
from camcops_server.tasks.khandaker_mojo_medicationtherapy import (
     KhandakerMojoMedicationTherapy,
)
from camcops_server.tasks.kirby_mcq import Kirby
from camcops_server.tasks.khandaker_mojo_sociodemographics import (
    KhandakerMojoSociodemographics,
)

from camcops_server.tasks.lynall_iam_medical import LynallIamMedicalHistory
from camcops_server.tasks.lynall_iam_life import LynallIamLifeEvents

from camcops_server.tasks.maas import Maas
from camcops_server.tasks.mast import Mast
from camcops_server.tasks.mds_updrs import MdsUpdrs
from camcops_server.tasks.mfi20 import Mfi20
from camcops_server.tasks.moca import Moca

from camcops_server.tasks.nart import Nart
from camcops_server.tasks.npiq import NpiQ

from camcops_server.tasks.ors import Ors

from camcops_server.tasks.panss import Panss
from camcops_server.tasks.pbq import Pbq
from camcops_server.tasks.pcl5 import Pcl5
from camcops_server.tasks.pcl import PclC, PclM, PclS
from camcops_server.tasks.pdss import Pdss
from camcops_server.tasks.perinatalpoem import PerinatalPoem
from camcops_server.tasks.photo import Photo, PhotoSequence
from camcops_server.tasks.phq9 import Phq9
from camcops_server.tasks.phq15 import Phq15
from camcops_server.tasks.progressnote import ProgressNote
from camcops_server.tasks.pswq import Pswq
from camcops_server.tasks.psychiatricclerking import PsychiatricClerking

from camcops_server.tasks.qolbasic import QolBasic
from camcops_server.tasks.qolsg import QolSG

from camcops_server.tasks.rand36 import Rand36
from camcops_server.tasks.rapid3 import Rapid3

from camcops_server.tasks.service_satisfaction import (
    PatientSatisfaction,
    ReferrerSatisfactionGen,
    ReferrerSatisfactionSpec,
)
from camcops_server.tasks.sfmpq2 import Sfmpq2
from camcops_server.tasks.shaps import Shaps
from camcops_server.tasks.slums import Slums
from camcops_server.tasks.smast import Smast
from camcops_server.tasks.srs import Srs
from camcops_server.tasks.suppsp import Suppsp

from camcops_server.tasks.wemwbs import Swemwbs, Wemwbs
from camcops_server.tasks.wsas import Wsas

from camcops_server.tasks.ybocs import Ybocs, YbocsSc

from camcops_server.tasks.zbi import Zbi12
