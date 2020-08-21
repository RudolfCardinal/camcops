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

#pragma once

#include <QMap>
#include <QPair>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QVector>
class QString;

// Phase 1:

class BlobFieldRef;
using BlobFieldRefPtr = QSharedPointer<BlobFieldRef>;

class CamcopsApp;

class DatabaseManager;
using DatabaseManagerPtr = QSharedPointer<DatabaseManager>;

class DatabaseObject;
using DatabaseObjectPtr = QSharedPointer<DatabaseObject>;

class DiagnosticCodeSet;
using DiagnosticCodeSetPtr = QSharedPointer<DiagnosticCodeSet>;

class DiagnosisItemBase;
using DiagnosisItemBasePtr = QSharedPointer<DiagnosisItemBase>;

using Dict = QMap<QString, QString>;

class FieldRef;
using FieldRefPtr = QSharedPointer<FieldRef>;
using FieldRefPtrList = QVector<FieldRefPtr>;

class IdNumDescription;
using IdNumDescriptionPtr = QSharedPointer<IdNumDescription>;
using IdNumDescriptionPtrList = QVector<IdNumDescriptionPtr>;
using IdNumDescriptionConstPtr = QSharedPointer<const IdNumDescription>;

class KhandakerMojoTherapyItem;
using KhandakerMojoTherapyItemPtr = QSharedPointer<KhandakerMojoTherapyItem>;

class KhandakerMojoMedicationItem;
using KhandakerMojoMedicationItemPtr = QSharedPointer<KhandakerMojoMedicationItem>;

class Patient;
using PatientPtr = QSharedPointer<Patient>;
using PatientPtrList = QVector<PatientPtr>;

class PatientIdNum;
using PatientIdNumPtr = QSharedPointer<PatientIdNum>;
using PatientIdNumPtrList = QVector<PatientIdNumPtr>;

class PhotoSequencePhoto;
using PhotoSequencePhotoPtr = QSharedPointer<PhotoSequencePhoto>;

class QuElement;
using QuElementPtr = QSharedPointer<QuElement>;

class Questionnaire;
using QuestionnairePtr = QSharedPointer<Questionnaire>;

class QuPage;
using QuPagePtr = QSharedPointer<QuPage>;

using Record = QMap<QString, QVariant>;
using RecordList = QVector<Record>;

class StoredVar;
using StoredVarPtr = QSharedPointer<StoredVar>;

class Task;
using TaskPtr = QSharedPointer<Task>;
using TaskWeakPtr = QWeakPointer<Task>;
using TaskPtrList = QVector<TaskPtr>;

class TaskChain;
using TaskChainPtr = QSharedPointer<TaskChain>;

class TaskFactory;
using TaskFactoryPtr = QSharedPointer<TaskFactory>;

// Phase 2, using things from Phase 1:

using GridRowDefinition = QPair<QString, QuElementPtr>;
using GridRowDefinitionRawPtr = QPair<QString, QuElement*>;
