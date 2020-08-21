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

#if 0

#include "ctqsf.h"
#include "lib/stringfunc.h"
#include "lib/version.h"
#include "tasklib/taskfactory.h"
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 28;
const QString QPREFIX("q");

const QString Ctqsf::CTQSF_TABLENAME("ctqsf");


void initializeCtqsf(TaskFactory& factory)
{
    static TaskRegistrar<Ctqsf> registered(factory);
}


Ctqsf::Ctqsf(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, CTQSF_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Ctqsf::shortname() const
{
    return "CTQ-SF";
}


QString Ctqsf::longname() const
{
    return tr("Childhood Trauma Questionnaire, Short Form");
}


QString Ctqsf::description() const
{
    return tr("28-item self-rating scale of adverse childhood experiences.");
}


Version Ctqsf::minimumServerVersion() const
{
    return Version(2, 3, 3);
}


// ============================================================================
// Instance info
// ============================================================================

bool Ctqsf::isComplete() const
{
    return !anyValuesNull(strseq(QPREFIX, FIRST_Q, N_QUESTIONS));
}


QStringList Ctqsf::summary() const
{
    return QStringList{
        // ***
    };
}


QStringList Ctqsf::detail() const
{
    const QString spacer = " ";
    QStringList lines = completenessInfo();
    // *** lines
    lines.append("");
    lines += summary();
    return lines;
}


OpenableWidget* Ctqsf::editor(const bool read_only)
{
    // ***
    return nullptr;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

#endif
