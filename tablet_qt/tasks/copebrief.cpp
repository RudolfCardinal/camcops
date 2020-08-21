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

#include "copebrief.h"
#include "common/textconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quhorizontalline.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using stringfunc::bold;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 28;
const QString QPREFIX("q");
const int RELATIONSHIP_OTHER_CODE = 0;
const int RELATIONSHIPS_FIRST = 0;
const int RELATIONSHIPS_FIRST_NON_OTHER = 1;
const int RELATIONSHIPS_LAST = 9;

const QString CopeBrief::COPEBRIEF_TABLENAME("cope_brief");

const QString COMPLETED_BY_PATIENT("completed_by_patient");
const QString COMPLETED_BY("completed_by");
const QString RELATIONSHIP_TO_PATIENT("relationship_to_patient");
const QString RELATIONSHIP_TO_PATIENT_OTHER("relationship_to_patient_other");

const QString XSTRING_RELPREFIX("relationship_");

const QString TAG_RELATIONSHIP("rel");
const QString TAG_RELATIONSHIP_OTHER("rel_other");


void initializeCopeBrief(TaskFactory& factory)
{
    static TaskRegistrar<CopeBrief> registered(factory);
}


CopeBrief::CopeBrief(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, COPEBRIEF_TABLENAME, false, false, false),  // ... anon, clin, resp
    // There is a respondent, optionally, but the task handles this manually with more detail
    m_questionnaire(nullptr)
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);
    addField(COMPLETED_BY_PATIENT, QVariant::Bool);
    addField(COMPLETED_BY, QVariant::String);
    addField(RELATIONSHIP_TO_PATIENT, QVariant::Int);
    addField(RELATIONSHIP_TO_PATIENT_OTHER, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString CopeBrief::shortname() const
{
    return "COPE_brief";
}


QString CopeBrief::longname() const
{
    return tr("Brief COPE Inventory");
}


QString CopeBrief::description() const
{
    return tr("28-item brief measure of coping.");
}


QString CopeBrief::infoFilenameStem() const
{
    return "cope";
}


QString CopeBrief::xstringTaskname() const
{
    return "cope";
}


// ============================================================================
// Instance info
// ============================================================================

bool CopeBrief::isComplete() const
{
    return isCompleteResponder() &&
            noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList CopeBrief::summary() const
{
    return QStringList{getResponder()};
}


QStringList CopeBrief::detail() const
{
    return completenessInfo() + summary();
}


OpenableWidget* CopeBrief::editor(const bool read_only)
{
    const NameValueOptions main_options{
        {xstring("a0"), 0},
        {xstring("a1"), 1},
        {xstring("a2"), 2},
        {xstring("a3"), 3},
    };
    NameValueOptions relationship_options;
    for (int i = RELATIONSHIPS_FIRST_NON_OTHER; i <= RELATIONSHIPS_LAST; ++i) {
        relationship_options.append(NameValuePair(
                xstring(strnum(XSTRING_RELPREFIX, i)), i));
    }
    relationship_options.append(NameValuePair(
                xstring(strnum(XSTRING_RELPREFIX, RELATIONSHIP_OTHER_CODE)),
                RELATIONSHIP_OTHER_CODE));
    NameValueOptions yesno_options = CommonOptions::yesNoBoolean();

    FieldRefPtr fr_completed_by_patient = fieldRef(COMPLETED_BY_PATIENT);
    FieldRefPtr fr_relationship = fieldRef(RELATIONSHIP_TO_PATIENT);

    QVector<QuElement*> elements1{
        new QuText(QString("%1 (%2)?").arg(xstring("q_patient"),
                                           bold(getPatientName()))),
        (new QuMcq(fr_completed_by_patient, yesno_options))
                ->setHorizontal(true),
        (new QuText(xstring("q_completedby")))
                ->addTag(TAG_RELATIONSHIP),
        (new QuTextEdit(fieldRef(COMPLETED_BY, false)))
                ->addTag(TAG_RELATIONSHIP),
        (new QuText(xstring("q_relationship")))
                ->addTag(TAG_RELATIONSHIP),
        (new QuMcq(fr_relationship, relationship_options))
                ->addTag(TAG_RELATIONSHIP),
        (new QuText(xstring("q_relationship_other")))
                ->addTag(TAG_RELATIONSHIP_OTHER),
        (new QuTextEdit(fieldRef(RELATIONSHIP_TO_PATIENT_OTHER, false)))
                ->addTag(TAG_RELATIONSHIP_OTHER),
    };

    QVector<QuElement*> elements2{
        new QuText(xstring("instructions")),
    };
    for (int i = 1; i <= N_QUESTIONS; ++i) {
        elements2.append(new QuHorizontalLine());
        elements2.append((new QuText(QString("Q%1. %2")
                             .arg(i)
                             .arg(xstring(strnum("q", i)))))->setBold());
        elements2.append(new QuMcq(fieldRef(strnum(QPREFIX, i)),
                                   main_options));
    }

    QString commontitle = longname();
    QVector<QuPagePtr> pages{
        QuPagePtr((new QuPage(elements1))->setTitle(commontitle + " (1/2)")),
        QuPagePtr((new QuPage(elements2))->setTitle(commontitle + " (2/2)")),
    };

    connect(fr_completed_by_patient.data(), &FieldRef::valueChanged,
            this, &CopeBrief::completedByPatientChanged);
    connect(fr_relationship.data(), &FieldRef::valueChanged,
            this, &CopeBrief::relationshipChanged);

    m_questionnaire = new Questionnaire(m_app, pages);
    m_questionnaire->setType(QuPage::PageType::Patient);
    m_questionnaire->setReadOnly(read_only);

    completedByPatientChanged();

    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

bool CopeBrief::isCompleteResponder() const
{
    if (valueIsNull(COMPLETED_BY_PATIENT)) {
        return false;
    }
    if (valueBool(COMPLETED_BY_PATIENT)) {
        return true;
    }
    if (!valueBool(COMPLETED_BY) || valueIsNull(RELATIONSHIP_TO_PATIENT)) {
        return false;
    }
    if (valueInt(RELATIONSHIP_TO_PATIENT) == RELATIONSHIP_OTHER_CODE &&
            valueIsNullOrEmpty(RELATIONSHIP_TO_PATIENT_OTHER)) {
        return false;
    }
    return true;
}


QString CopeBrief::getResponder() const
{
    if (valueIsNull(COMPLETED_BY_PATIENT)) {
        return "?";
    }
    if (valueBool(COMPLETED_BY_PATIENT)) {
        return TextConst::patient();
    }

    QString c = valueString(COMPLETED_BY);
    if (c.isEmpty()) {
        c = "?";
    }

    QString r = "?";
    int relcode = valueInt(RELATIONSHIP_TO_PATIENT);
    if (relcode >= RELATIONSHIPS_FIRST && relcode <= RELATIONSHIPS_LAST) {
        r = xstring(strnum(XSTRING_RELPREFIX, relcode));
    }

    return QString("%1 (%2)").arg(c, r);
}


void CopeBrief::completedByPatientChanged()
{
    if (!m_questionnaire) {
        return;
    }
    const bool not_by_patient = valueIsFalseNotNull(COMPLETED_BY_PATIENT);
    fieldRef(COMPLETED_BY)->setMandatory(not_by_patient);
    fieldRef(RELATIONSHIP_TO_PATIENT)->setMandatory(not_by_patient);
    m_questionnaire->setVisibleByTag(TAG_RELATIONSHIP, not_by_patient, false);
    relationshipChanged();
}


void CopeBrief::relationshipChanged()
{
    if (!m_questionnaire) {
        return;
    }
    const bool need_other = valueIsFalseNotNull(COMPLETED_BY_PATIENT) &&
            !valueIsNull(RELATIONSHIP_TO_PATIENT) &&
            valueInt(RELATIONSHIP_TO_PATIENT) == RELATIONSHIP_OTHER_CODE;
    fieldRef(RELATIONSHIP_TO_PATIENT_OTHER)->setMandatory(need_other);
    m_questionnaire->setVisibleByTag(TAG_RELATIONSHIP_OTHER, need_other, false);
}
