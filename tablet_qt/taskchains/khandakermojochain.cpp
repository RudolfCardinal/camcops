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

#include "khandakermojochain.h"
#include "tasks/chit.h"
#include "tasks/eq5d5l.h"
#include "tasks/khandakermojomedicationtherapy.h"
#include "tasks/khandakermojosociodemographics.h"
#include "tasks/mfi20.h"
#include "tasks/sfmpq2.h"
#include "tasks/shaps.h"
#include "tasks/suppsp.h"


KhandakerMojoChain::KhandakerMojoChain(CamcopsApp& app) :
    TaskChain(app, {
        KhandakerMojoSociodemographics::KHANDAKER2MOJOSOCIODEMOGRAPHICS_TABLENAME,
        KhandakerMojoMedicationTherapy::KHANDAKERMOJOMEDICATIONTHERAPY_TABLENAME,
        Eq5d5l::EQ5D5L_TABLENAME,
        Shaps::SHAPS_TABLENAME,
        Mfi20::MFI20_TABLENAME,
        Chit::CHIT_TABLENAME,
        Suppsp::SUPPSP_TABLENAME,
        Sfmpq2::SFMPQ2_TABLENAME,
        // Sequence as per Joel Parkinson to Rudolf Cardinal, 2019-10-22.
    }, TaskChain::CreationMethod::OnDemandOrAbort)  // as per JP 2019-10-22; also the default.
{
}
