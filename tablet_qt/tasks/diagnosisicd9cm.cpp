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

#include "diagnosisicd9cm.h"
#include "db/ancillaryfunc.h"
#include "diagnosis/icd9cm.h"
#include "questionnairelib/qudiagnosticcode.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
#include "taskxtra/diagnosisicd9cmitem.h"

const QString DiagnosisIcd9CM::DIAGNOSIS_ICD9CM_TABLENAME("diagnosis_icd9cm");


void initializeDiagnosisIcd9CM(TaskFactory& factory)
{
    static TaskRegistrar<DiagnosisIcd9CM> registered(factory);
}


DiagnosisIcd9CM::DiagnosisIcd9CM(CamcopsApp& app, DatabaseManager& db,
                                 const int load_pk) :
    DiagnosisTaskBase(app, db, DIAGNOSIS_ICD9CM_TABLENAME, load_pk)
{
}


// ============================================================================
// Class info
// ============================================================================

QString DiagnosisIcd9CM::shortname() const
{
    return "Diagnosis_ICD9CM";
}


QString DiagnosisIcd9CM::longname() const
{
    return tr("Diagnostic coding (ICD-9-CM)");
}


QString DiagnosisIcd9CM::description() const
{
    return tr("Diagnostic codes, using ICD-9-CM/DSM-IV-TR codes.");
}


QString DiagnosisIcd9CM::infoFilenameStem() const
{
    return "icd";
}


QString DiagnosisIcd9CM::xstringTaskname() const
{
    return Icd9cm::XSTRING_TASKNAME;
}


// ============================================================================
// Ancillary management
// ============================================================================

QStringList DiagnosisIcd9CM::ancillaryTables() const
{
    return QStringList{DiagnosisIcd9CMItem::DIAGNOSIS_ICD9CM_ITEM_TABLENAME};
}


QString DiagnosisIcd9CM::ancillaryTableFKToTaskFieldname() const
{
    return DiagnosisIcd9CMItem::FK_NAME;
}


void DiagnosisIcd9CM::loadAllAncillary(const int pk)
{
    const OrderBy order_by{{DiagnosisIcd9CMItem::SEQNUM, true}};
    ancillaryfunc::loadAncillary<DiagnosisIcd9CMItem, DiagnosisItemBasePtr>(
                m_items, m_app, m_db,
                DiagnosisIcd9CMItem::FK_NAME, order_by, pk);
}


QVector<DatabaseObjectPtr> DiagnosisIcd9CM::getAncillarySpecimens() const
{
    return QVector<DatabaseObjectPtr>{
        DatabaseObjectPtr(new DiagnosisIcd9CMItem(m_app, m_db)),
    };
}


// ============================================================================
// DiagnosisTaskBase extras
// ============================================================================

DiagnosticCodeSetPtr DiagnosisIcd9CM::makeCodeset() const
{
    return DiagnosticCodeSetPtr(new Icd9cm(m_app));
}


DiagnosisItemBasePtr DiagnosisIcd9CM::makeItem() const
{
    return DiagnosisItemBasePtr(new DiagnosisIcd9CMItem(
                                    pkvalueInt(), m_app, m_db));
}
