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

#include "diagnosisicd10.h"
#include "db/ancillaryfunc.h"
#include "diagnosis/icd10.h"
#include "questionnairelib/qudiagnosticcode.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
#include "taskxtra/diagnosisicd10item.h"

const QString DiagnosisIcd10::DIAGNOSIS_ICD10_TABLENAME("diagnosis_icd10");


void initializeDiagnosisIcd10(TaskFactory& factory)
{
    static TaskRegistrar<DiagnosisIcd10> registered(factory);
}


DiagnosisIcd10::DiagnosisIcd10(CamcopsApp& app, DatabaseManager& db,
                               const int load_pk) :
    DiagnosisTaskBase(app, db, DIAGNOSIS_ICD10_TABLENAME, load_pk)
{
}


// ============================================================================
// Class info
// ============================================================================

QString DiagnosisIcd10::shortname() const
{
    return "Diagnosis_ICD10";
}


QString DiagnosisIcd10::longname() const
{
    return tr("Diagnostic coding (ICD-10)");
}


QString DiagnosisIcd10::description() const
{
    return tr("Diagnostic codes, using ICD-10 codes.");
}


QString DiagnosisIcd10::infoFilenameStem() const
{
    return "icd";
}


QString DiagnosisIcd10::xstringTaskname() const
{
    return Icd10::XSTRING_TASKNAME;
}


// ============================================================================
// Ancillary management
// ============================================================================

QStringList DiagnosisIcd10::ancillaryTables() const
{
    return QStringList{DiagnosisIcd10Item::DIAGNOSIS_ICD10_ITEM_TABLENAME};
}


QString DiagnosisIcd10::ancillaryTableFKToTaskFieldname() const
{
    return DiagnosisIcd10Item::FK_NAME;
}


void DiagnosisIcd10::loadAllAncillary(const int pk)
{
    const OrderBy order_by{{DiagnosisIcd10Item::SEQNUM, true}};
    ancillaryfunc::loadAncillary<DiagnosisIcd10Item, DiagnosisItemBasePtr>(
                m_items, m_app, m_db,
                DiagnosisIcd10Item::FK_NAME, order_by, pk);
}


QVector<DatabaseObjectPtr> DiagnosisIcd10::getAncillarySpecimens() const
{
    return QVector<DatabaseObjectPtr>{
        DatabaseObjectPtr(new DiagnosisIcd10Item(m_app, m_db)),
    };
}


// ============================================================================
// DiagnosisTaskBase extras
// ============================================================================

DiagnosticCodeSetPtr DiagnosisIcd10::makeCodeset() const
{
    return DiagnosticCodeSetPtr(new Icd10(m_app));
}


DiagnosisItemBasePtr DiagnosisIcd10::makeItem() const
{
    return DiagnosisItemBasePtr(new DiagnosisIcd10Item(
                                    pkvalueInt(), m_app, m_db));
}
