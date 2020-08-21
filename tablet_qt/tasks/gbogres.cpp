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

// By Joe Kearney, Rudolf Cardinal.

#include "gbogres.h"
#include "maths/mathfunc.h"
#include "lib/datetime.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/qudatetime.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/quflowcontainer.h"
#include "questionnairelib/quhorizontalcontainer.h"
#include "questionnairelib/quhorizontalline.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
#include "taskxtra/gbocommon.h"

using mathfunc::noneNullOrEmpty;
using stringfunc::strseq;

const QString GboGReS::GBOGRES_TABLENAME("gbogres");

const QString FN_DATE("date");  // NB SQL keyword too; doesn't matter
const QString FN_GOAL_1_DESC("goal_1_description");
const QString FN_GOAL_2_DESC("goal_2_description");
const QString FN_GOAL_3_DESC("goal_3_description");
const QString FN_GOAL_OTHER("other_goals");
const QString FN_COMPLETED_BY("completed_by");
const QString FN_COMPLETED_BY_OTHER("completed_by_other");

const QString TAG_OTHER("other");


void initializeGboGReS(TaskFactory& factory)
{
    static TaskRegistrar<GboGReS> registered(factory);
}


GboGReS::GboGReS(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, GBOGRES_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addField(FN_DATE, QVariant::Date);
    addField(FN_GOAL_1_DESC, QVariant::String);
    addField(FN_GOAL_2_DESC, QVariant::String);
    addField(FN_GOAL_3_DESC, QVariant::String);
    addField(FN_GOAL_OTHER, QVariant::String);
    addField(FN_COMPLETED_BY, QVariant::Int);
    addField(FN_COMPLETED_BY_OTHER, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.

    // Extra initialization:
    if (load_pk == dbconst::NONEXISTENT_PK) {
        setValue(FN_DATE, datetime::nowDate(), false);
    }
}

// ============================================================================
// Class info
// ============================================================================

QString GboGReS::shortname() const
{
    return "GBO-GReS";
}


QString GboGReS::longname() const
{
    return tr("Goal-Based Outcomes – 1 – Goal Record Sheet");
}


QString GboGReS::description() const
{
    return tr("For recording goals of therapy.");
}


QString GboGReS::xstringTaskname() const
{
    return "gbo";
}


// ============================================================================
// Instance info
// ============================================================================

bool GboGReS::isComplete() const
{
    if (anyValuesNullOrEmpty({FN_DATE, FN_GOAL_1_DESC, FN_COMPLETED_BY})) {
        return false;
    }
    if (value(FN_COMPLETED_BY) == gbocommon::AGENT_OTHER
            && valueIsNullOrEmpty(FN_COMPLETED_BY_OTHER)) {
        return false;
    }
    return true;
}


QStringList GboGReS::summary() const
{
    return QStringList{
        QString("Date: <b>%1</b>.").arg(
                             datetime::dateToIso(valueDate(FN_DATE))),
        QString("Goals set: <b>%1</b>%2.").arg(numGoalsDescription(),
                                               extraGoalsDescription()),
    };
}


QStringList GboGReS::detail() const
{
    QStringList detail = summary();

    int i = 0;
    for (auto field : {FN_GOAL_1_DESC, FN_GOAL_2_DESC, FN_GOAL_3_DESC}) {
        ++i;
        if (!valueIsNullOrEmpty(field)) {
            detail.push_back(QString("Goal %1: <b>%2</b>.").arg(
                                 QString::number(i),
                                 value(field).toString()));
        }
    }
    if (!valueIsNullOrEmpty(FN_GOAL_OTHER)) {
        detail.push_back(QString("Extra goals: <b>%1</b>")
                         .arg(value(FN_GOAL_OTHER).toString()));
    }

    detail.push_back(QString("Completed by: <b>%1</b>.").arg(completedBy()));

    return detail;
}


OpenableWidget* GboGReS::editor(const bool read_only)
{
    const NameValueOptions completed_by_options = NameValueOptions{
        { xstring("agent_1"), gbocommon::AGENT_PATIENT },
        { xstring("agent_2"), gbocommon::AGENT_PARENT_CARER },
        { xstring("agent_3"), gbocommon::AGENT_CLINICIAN },
        { xstring("agent_4"), gbocommon::AGENT_OTHER }
    };
    const QString goal_desc = xstring("goal_desc");

    QuPagePtr page(new QuPage{
        (new QuHorizontalContainer{
            new QuHeading(xstring("date")),
           (new QuDateTime(fieldRef(FN_DATE))
               )->setMode(QuDateTime::DefaultDate)
                ->setOfferNowButton(true),
        }),
        (new QuText(xstring("gres_stem")))->setBold(true),
        new QuSpacer(),

        new QuHeading(xstring("goal_1")),
        new QuText(goal_desc),
        new QuTextEdit(fieldRef(FN_GOAL_1_DESC)),
        new QuSpacer(),

        new QuHeading(xstring("goal_2")),
        new QuText(goal_desc),
        new QuTextEdit(fieldRef(FN_GOAL_2_DESC, false)),
        new QuSpacer(),

        new QuHeading(xstring("goal_3")),
        new QuText(goal_desc),
        new QuTextEdit(fieldRef(FN_GOAL_3_DESC, false)),
        new QuSpacer(),

        (new QuText(xstring("goal_other")))->setBold(true),
        new QuTextEdit(fieldRef(FN_GOAL_OTHER, false)),
        new QuSpacer(),

        (new QuText(xstring("completed_by")))->setBold(true),
        (new QuMcq(fieldRef(FN_COMPLETED_BY), completed_by_options))
                        ->setHorizontal(true)
                        ->setAsTextButton(true),
        (new QuTextEdit(fieldRef(FN_COMPLETED_BY_OTHER), false))->addTag(TAG_OTHER),

        new QuSpacer(),
        new QuHorizontalLine(),
        new QuSpacer(),
        (new QuText(xstring("copyright")))->setItalic()
    });

    page->setTitle(longname());

    m_questionnaire = new Questionnaire(m_app, {page});
    m_questionnaire->setReadOnly(read_only);

    connect(fieldRef(FN_COMPLETED_BY).data(), &FieldRef::valueChanged,
            this, &GboGReS::updateMandatory);
    updateMandatory();

    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

void GboGReS::updateMandatory()
{
    const bool required = valueInt(FN_COMPLETED_BY) == gbocommon::AGENT_OTHER;
    fieldRef(FN_COMPLETED_BY_OTHER)->setMandatory(required);
    if (!m_questionnaire) {
        return;
    }
    m_questionnaire->setVisibleByTag(TAG_OTHER, required);
}


QString GboGReS::numGoalsDescription() const
{
    int goal_n = 0;

    for (auto field : {FN_GOAL_1_DESC, FN_GOAL_2_DESC, FN_GOAL_3_DESC}) {
        if (!valueIsNullOrEmpty(field)) {
            ++goal_n;
        }
    }

    return QString::number(goal_n);
}


QString GboGReS::extraGoalsDescription() const
{
    QString extra = "";
    if (!valueIsNullOrEmpty(FN_GOAL_OTHER)) {
        extra = " <i>(with additional goals set)</i>";
    }
    return extra;
}


QString GboGReS::completedBy() const
{
    return gbocommon::agentDescription(valueInt(FN_COMPLETED_BY),
                                       valueString(FN_COMPLETED_BY_OTHER));
}
