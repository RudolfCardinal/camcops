/*
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
*/

#include "tasklib/inittasks.h"

#include "tasks/ace3.h"
#include "tasks/aims.h"
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
#include "tasks/cbir.h"
#include "tasks/cecaq3.h"
#include "tasks/cgi.h"
#include "tasks/cgii.h"
#include "tasks/cgisch.h"
#include "tasks/ciwa.h"
#include "tasks/contactlog.h"
#include "tasks/copebrief.h"
#include "tasks/cpftlpsdischarge.h"
#include "tasks/cpftlpsreferral.h"
#include "tasks/cpftlpsresetresponseclock.h"

#include "tasks/dad.h"
#include "tasks/dast.h"
#include "tasks/deakin1healthreview.h"
#include "tasks/demoquestionnaire.h"
#include "tasks/demqol.h"
#include "tasks/demqolproxy.h"
#include "tasks/diagnosisicd9cm.h"
#include "tasks/diagnosisicd10.h"
#include "tasks/distressthermometer.h"

#include "tasks/fast.h"
#include "tasks/fft.h"
#include "tasks/frs.h"

#include "tasks/gad7.h"
#include "tasks/gaf.h"
#include "tasks/gds15.h"
#include "tasks/gmcpq.h"

#include "tasks/hads.h"
#include "tasks/hama.h"
#include "tasks/hamd.h"
#include "tasks/hamd7.h"
#include "tasks/honos.h"
#include "tasks/honos65.h"
#include "tasks/honosca.h"

#include "tasks/phq9.h"


void InitTasks(TaskFactory& factory)
{
    // Change these lines to determine which tasks are available:

    initializeAce3(factory);
    initializeAims(factory);
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
    // *** initializeCardinalExpDetThreshold(factory); // *** COMPLEX
    // *** initializeCardinalExpectationDetection(factory); // *** COMPLEX
    initializeCbiR(factory);
    initializeCecaQ3(factory);
    initializeCgi(factory);
    initializeCgiI(factory);
    initializeCgiSch(factory);
    initializeCiwa(factory);
    initializeContactLog(factory);
    initializeCopeBrief(factory);
    initializeCPFTLPSDischarge(factory);
    initializeCPFTLPSReferral(factory);
    initializeCPFTLPSResetResponseClock(factory);

    initializeDad(factory);
    initializeDast(factory);
    initializeDeakin1HealthReview(factory);
    initializeDemoQuestionnaire(factory);
    initializeDemqol(factory);
    initializeDemqolProxy(factory);
    initializeDiagnosisIcd9CM(factory);
    initializeDiagnosisIcd10(factory);
    initializeDistressThermometer(factory);

    initializeFast(factory);
    initializeFft(factory);
    initializeFrs(factory);

    initializeGad7(factory);
    initializeGaf(factory);
    initializeGds15(factory);
    initializeGmcPq(factory);

    initializeHads(factory);
    initializeHamA(factory);
    initializeHamD(factory);
    initializeHamD7(factory);
    initializeHonos(factory);
    initializeHonos65(factory);
    initializeHonosca(factory);

    // *** initializeIcd10Depressive(factory);
    // *** initializeIcd10Manic(factory);
    // *** initializeIcd10Mixed(factory);
    // *** initializeIcd10Schizophrenia(factory);
    // *** initializeIcd10Schizotypal(factory);
    // *** initializeIcd10SpecPD(factory);
    // *** initializeIDED3D(factory); // *** COMPLEX
    // *** initializeIesr(factory);
    // *** initializeIfs(factory);
    // *** initializeIrac(factory);

    // *** initializeMast(factory);
    // *** initializeMdsUpdrs(factory);
    // *** initializeMoca(factory);

    // *** initializeNart(factory);
    // *** initializeNpiQ(factory);

    // *** initializePanss(factory);
    // *** initializePatientSatisfaction(factory);
    // *** initializePclC(factory);
    // *** initializePclM(factory);
    // *** initializePclS(factory);
    // *** initializePdss(factory);
    // *** initializePhoto(factory);
    // *** initializePhotoSequence(factory);
    initializePhq9(factory);
    // *** initializePhq15(factory);
    // *** initializeProgressNote(factory);
    // *** initializePswq(factory);
    // *** initializePsychiatricClerking(factory);

    // *** initializeQolBasic(factory);
    // *** initializeQolSG(factory); // *** COMPLEX

    // *** initializeRand36(factory);
    // *** initializeReferrerSatisfactionGen(factory);
    // *** initializeReferrerSatisfactionSpec(factory);

    // *** initializeSlums(factory);
    // *** initializeSmast(factory);
    // *** initializeSwemwbs(factory);

    // *** initializeWemwbs(factory);
    // *** initializeWsas(factory);

    // *** initializeYbocs(factory);
    // *** initializeYbocsSc(factory);

    // *** initializeZbi12(factory);


    // *** new: mini-ACE
    // *** new: other things in e-mail queue

    // *** discarded tasks - revitalize: ASRM
    // *** discarded tasks - revitalize: BARS
    // *** discarded tasks - revitalize: BFCRS
    // *** discarded tasks - revitalize: CSI
    // *** discarded tasks - revitalize: EPDS
    // *** discarded tasks - revitalize: FAB
    // *** discarded tasks - revitalize: GASS
    // *** discarded tasks - revitalize: LSHSA
    // *** discarded tasks - revitalize: LSHSLAROI2005
    // *** discarded tasks - revitalize: LUNSERS
    // *** discarded tasks - revitalize: MADRS
    // *** discarded tasks - revitalize: SAS
}
