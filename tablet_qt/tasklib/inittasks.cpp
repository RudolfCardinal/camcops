/*
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
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.
*/

#include "tasklib/inittasks.h"

#include "tasks/ace3.h"
#include "tasks/aims.h"
#include "tasks/apeqcpftperinatal.h"
#include "tasks/apeqpt.h"
#include "tasks/asdas.h"
#include "tasks/audit.h"
#include "tasks/auditc.h"

#include "tasks/badls.h"
#include "tasks/bdi.h"
#include "tasks/bmi.h"
#include "tasks/bprs.h"
#include "tasks/bprse.h"

#include "tasks/cage.h"
#include "tasks/cape42.h"
#include "tasks/caps.h"
#include "tasks/cardinalexpdetthreshold.h"
#include "tasks/cardinalexpectationdetection.h"
#include "tasks/cbir.h"
#include "tasks/cecaq3.h"
#include "tasks/cesd.h"
#include "tasks/cesdr.h"
#include "tasks/cgi.h"
#include "tasks/cgii.h"
#include "tasks/cgisch.h"
#include "tasks/chit.h"
#include "tasks/cisr.h"
#include "tasks/ciwa.h"
#include "tasks/contactlog.h"
#include "tasks/copebrief.h"
#include "tasks/core10.h"
#include "tasks/cpftlpsdischarge.h"
#include "tasks/cpftlpsreferral.h"
#include "tasks/cpftlpsresetresponseclock.h"
#include "tasks/ctqsf.h"

#include "tasks/dad.h"
#include "tasks/das28.h"
#include "tasks/dast.h"
#include "tasks/deakins1healthreview.h"
#include "tasks/demoquestionnaire.h"
#include "tasks/demqol.h"
#include "tasks/demqolproxy.h"
#include "tasks/diagnosisicd9cm.h"
#include "tasks/diagnosisicd10.h"
#include "tasks/distressthermometer.h"

#include "tasks/elixhauserci.h"
#include "tasks/epds.h"
#include "tasks/eq5d5l.h"
#include "tasks/esspri.h"

#include "tasks/fast.h"
#include "tasks/factg.h"
#include "tasks/fft.h"
#include "tasks/frs.h"

#include "tasks/gad7.h"
#include "tasks/gaf.h"
#include "tasks/gbogras.h"
#include "tasks/gbogres.h"
#include "tasks/gbogpc.h"
#include "tasks/gds15.h"
#include "tasks/gmcpq.h"

#include "tasks/hads.h"
#include "tasks/hadsrespondent.h"
#include "tasks/hama.h"
#include "tasks/hamd.h"
#include "tasks/hamd7.h"
#include "tasks/honos.h"
#include "tasks/honos65.h"
#include "tasks/honosca.h"

#include "tasks/icd10depressive.h"
#include "tasks/icd10manic.h"
#include "tasks/icd10mixed.h"
#include "tasks/icd10schizophrenia.h"
#include "tasks/icd10schizotypal.h"
#include "tasks/icd10specpd.h"
#include "tasks/ided3d.h"
#include "tasks/iesr.h"
#include "tasks/ifs.h"
#include "tasks/irac.h"

#include "tasks/khandakerinsightmedical.h"
#include "tasks/khandakermojomedical.h"
#include "tasks/khandakermojomedicationtherapy.h"
#include "tasks/khandakermojosociodemographics.h"
#include "tasks/kirby.h"

#include "tasks/lynalliammedical.h"
#include "tasks/lynalliamlife.h"

#include "tasks/maas.h"
#include "tasks/mast.h"
#include "tasks/mdsupdrs.h"
#include "tasks/mfi20.h"
#include "tasks/moca.h"

#include "tasks/nart.h"
#include "tasks/npiq.h"

#include "tasks/ors.h"

#include "tasks/panss.h"
#include "tasks/patientsatisfaction.h"
#include "tasks/pbq.h"
#include "tasks/pcl5.h"
#include "tasks/pclc.h"
#include "tasks/pclm.h"
#include "tasks/pcls.h"
#include "tasks/pdss.h"
#include "tasks/perinatalpoem.h"
#include "tasks/photo.h"
#include "tasks/photosequence.h"
#include "tasks/phq9.h"
#include "tasks/phq15.h"
#include "tasks/progressnote.h"
#include "tasks/pswq.h"
#include "tasks/psychiatricclerking.h"

#include "tasks/qolbasic.h"
#include "tasks/qolsg.h"

#include "tasks/rand36.h"
#include "tasks/referrersatisfactiongen.h"
#include "tasks/referrersatisfactionspec.h"

#include "tasks/sfmpq2.h"
#include "tasks/shaps.h"
#include "tasks/slums.h"
#include "tasks/smast.h"
#include "tasks/srs.h"
#include "tasks/suppsp.h"
#include "tasks/swemwbs.h"

#include "tasks/wemwbs.h"
#include "tasks/wsas.h"

#include "tasks/ybocs.h"
#include "tasks/ybocssc.h"

#include "tasks/zbi12.h"


void InitTasks(TaskFactory& factory)
{
    // Change these lines to determine which tasks are available:

    initializeAce3(factory);
    initializeAims(factory);
    initializeAPEQCPFTPerinatal(factory);
    initializeApeqpt(factory);
    initializeAsdas(factory);
    initializeAudit(factory);
    initializeAuditC(factory);

    initializeBadls(factory);
    initializeBdi(factory);
    initializeBmi(factory);
    initializeBprs(factory);
    initializeBprsE(factory);

    initializeCage(factory);
    initializeCape42(factory);
    initializeCaps(factory);
    initializeCardinalExpDetThreshold(factory);
    initializeCardinalExpectationDetection(factory);
    initializeCbiR(factory);
    initializeCesd(factory);
    initializeCesdr(factory);
    initializeCecaQ3(factory);
    initializeCgi(factory);
    initializeCgiI(factory);
    initializeCgiSch(factory);
    initializeChit(factory);
    initializeCisr(factory);
    initializeCiwa(factory);
    initializeContactLog(factory);
    initializeCopeBrief(factory);
    initializeCore10(factory);
    initializeCPFTLPSDischarge(factory);
    initializeCPFTLPSReferral(factory);
    initializeCPFTLPSResetResponseClock(factory);
    // *** // initializeCtqsf(factory);

    initializeDad(factory);
    initializeDas28(factory);
    initializeDast(factory);
    initializeDeakinS1HealthReview(factory);
    initializeDemoQuestionnaire(factory);
    initializeDemqol(factory);
    initializeDemqolProxy(factory);
    initializeDiagnosisIcd9CM(factory);
    initializeDiagnosisIcd10(factory);
    initializeDistressThermometer(factory);

    initializeElixhauserCI(factory);
    initializeEpds(factory);
    initializeEq5d5l(factory);
    initializeEsspri(factory);

    initializeFactg(factory);
    initializeFast(factory);
    initializeFft(factory);
    initializeFrs(factory);

    initializeGad7(factory);
    initializeGaf(factory);
    initializeGboGRaS(factory);
    initializeGboGReS(factory);
    initializeGboGPC(factory);
    initializeGds15(factory);
    initializeGmcPq(factory);

    initializeHads(factory);
    initializeHadsRespondent(factory);
    initializeHamA(factory);
    initializeHamD(factory);
    initializeHamD7(factory);
    initializeHonos(factory);
    initializeHonos65(factory);
    initializeHonosca(factory);

    initializeIcd10Depressive(factory);
    initializeIcd10Manic(factory);
    initializeIcd10Mixed(factory);
    initializeIcd10Schizophrenia(factory);
    initializeIcd10Schizotypal(factory);
    initializeIcd10SpecPD(factory);
    initializeIDED3D(factory);
    initializeIesr(factory);
    initializeIfs(factory);
    initializeIrac(factory);

    initializeKhandakerInsightMedical(factory);
    initializeKhandakerMojoMedical(factory);
    initializeKhandakerMojoMedicationTherapy(factory);
    initializeKhandakerMojoSociodemographics(factory);
    initializeKirby(factory);

    initializeLynallIamMedical(factory);
    initializeLynallIamLife(factory);

    initializeMaas(factory);
    initializeMast(factory);
    initializeMdsUpdrs(factory);
    initializeMfi20(factory);
    initializeMoca(factory);

    initializeNart(factory);
    initializeNpiQ(factory);

    initializeOrs(factory);

    initializePanss(factory);
    initializePatientSatisfaction(factory);
    initializePbq(factory);
    initializePcl5(factory);
    initializePclC(factory);
    initializePclM(factory);
    initializePclS(factory);
    initializePdss(factory);
    initializePerinatalPoem(factory);
    initializePhoto(factory);
    initializePhotoSequence(factory);
    initializePhq9(factory);
    initializePhq15(factory);
    initializeProgressNote(factory);
    initializePswq(factory);
    initializePsychiatricClerking(factory);

    initializeQolBasic(factory);
    initializeQolSG(factory);

    initializeRand36(factory);
    initializeReferrerSatisfactionGen(factory);
    initializeReferrerSatisfactionSpec(factory);

    initializeSfmpq2(factory);
    initializeShaps(factory);
    initializeSlums(factory);
    initializeSmast(factory);
    initializeSrs(factory);
    initializeSuppsp(factory);
    initializeSwemwbs(factory);

    initializeWemwbs(factory);
    initializeWsas(factory);

    initializeYbocs(factory);
    initializeYbocsSc(factory);

    initializeZbi12(factory);
}
