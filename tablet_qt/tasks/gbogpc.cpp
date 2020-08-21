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

#include "gbogpc.h"
#include "maths/mathfunc.h"
#include "lib/datetime.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/qudatetime.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/quflowcontainer.h"
#include "questionnairelib/quhorizontalline.h"
#include "questionnairelib/qulineeditinteger.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
#include "taskxtra/gbocommon.h"

using mathfunc::anyNullOrEmpty;
using stringfunc::strseq;

const QString GboGPC::GBOGPC_TABLENAME("gbogpc");

const int MIN_SESSION = 1;
const int MAX_SESSION = 1000;
const int MIN_GOAL = 1;
const int MAX_GOAL = 3;  // 3? More to allow for the free-text other goals?
// Yes, 3 -- "This is one of up to three goals to track".

const QString FN_DATE("date");
const QString FN_SESSION("session");
const QString FN_GOAL_NUMBER("goal_number");
const QString FN_GOAL_DESCRIPTION("goal_description");
const QString FN_PROGRESS("progress");
const QString FN_WHOSE_GOAL("whose_goal");
const QString FN_WHOSE_GOAL_OTHER("whose_goal_other");

const QString TAG_OTHER("other");


void initializeGboGPC(TaskFactory& factory)
{
    static TaskRegistrar<GboGPC> registered(factory);
}


GboGPC::GboGPC(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, GBOGPC_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addField(FN_DATE, QVariant::Date);
    addField(FN_SESSION, QVariant::Int);
    addField(FN_GOAL_NUMBER, QVariant::Int);
    addField(FN_GOAL_DESCRIPTION, QVariant::String);
    addField(FN_PROGRESS, QVariant::Int);
    addField(FN_WHOSE_GOAL, QVariant::Int);
    addField(FN_WHOSE_GOAL_OTHER, QVariant::String);
    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.

    if (load_pk == dbconst::NONEXISTENT_PK) {
        setValue(FN_DATE, datetime::nowDate(), false);
    }
}


// ============================================================================
// Class info
// ============================================================================

QString GboGPC::shortname() const
{
    return "GBO-GPC";
}


QString GboGPC::longname() const
{
    return tr("Goal-Based Outcomes – 2 – Goal Progress Chart");
}


QString GboGPC::description() const
{
    return tr("For recording progress towards the goals of therapy "
              "(one goal at a time).");
}


QString GboGPC::xstringTaskname() const
{
    return "gbo";
}


// ============================================================================
// Instance info
// ============================================================================

bool GboGPC::isComplete() const
{
    if (anyValuesNullOrEmpty({FN_DATE, FN_SESSION, FN_GOAL_NUMBER,
                              FN_PROGRESS, FN_WHOSE_GOAL})) {
        return false;
    }
    if (value(FN_WHOSE_GOAL) == gbocommon::AGENT_OTHER &&
            valueIsNullOrEmpty(FN_WHOSE_GOAL_OTHER)) {
        return false;
    }
    return true;
}


QStringList GboGPC::summary() const
{
    QStringList lines;
    lines.append(QString("Date: <b>%1</b>.").arg(
                     datetime::dateToIso(valueDate(FN_DATE))));
    lines.append(QString("Goal: <b>%1</b>.").arg(prettyValue(FN_GOAL_NUMBER)));
    lines.append(QString("Progress: <b>%1</b>/%2.").arg(
                     prettyValue(FN_PROGRESS),
                     QString::number(gbocommon::PROGRESS_MAX)));
    return lines;
}


QStringList GboGPC::detail() const
{
    return summary();
}


OpenableWidget* GboGPC::editor(const bool read_only)
{
    const NameValueOptions whose_goal_options = NameValueOptions{
        { xstring("agent_1"), gbocommon::AGENT_PATIENT },
        { xstring("agent_2"), gbocommon::AGENT_PARENT_CARER },
        { xstring("agent_3"), gbocommon::AGENT_CLINICIAN },
        { xstring("agent_4"), gbocommon::AGENT_OTHER },
    };

    const NameValueOptions goal_number_options =
            NameValueOptions::makeNumbers(MIN_GOAL, MAX_GOAL);
    const NameValueOptions goal_progress_options =
            NameValueOptions::makeNumbers(gbocommon::PROGRESS_MIN,
                                          gbocommon::PROGRESS_MAX);

    QuPagePtr page(new QuPage{
        (new QuText(xstring("gpc_intro")))->setItalic(),
        new QuFlowContainer{
                new QuHeading(xstring("date")),
                (new QuDateTime(fieldRef(FN_DATE)))
                    ->setMode(QuDateTime::DefaultDate)
                    ->setOfferNowButton(true),
        },
        new QuFlowContainer{
            new QuHeading(xstring("session")),
            new QuLineEditInteger(fieldRef(FN_SESSION), MIN_SESSION, MAX_SESSION)
        },
        new QuFlowContainer{
            new QuHeading(xstring("goal_number")),
            (new QuMcq(fieldRef(FN_GOAL_NUMBER), goal_number_options))
                ->setHorizontal(true)
                ->setAsTextButton(true),
        },
        new QuFlowContainer{
            new QuHeading(xstring("goal")),
            new QuTextEdit(fieldRef(FN_GOAL_DESCRIPTION)),
        },

        (new QuText(xstring("progress")))->setBold(true),
        (new QuText(xstring("progress_explanation")))->setItalic(),
        (new QuMcq(fieldRef(FN_PROGRESS), goal_progress_options))
            ->setHorizontal(true)
            ->setAsTextButton(true),

        (new QuText(xstring("whose_goal")))->setBold(true),
        (new QuMcq(fieldRef(FN_WHOSE_GOAL), whose_goal_options))
            ->setHorizontal(true)
            ->setAsTextButton(true),
        (new QuTextEdit(fieldRef(FN_WHOSE_GOAL_OTHER)))->addTag(TAG_OTHER),

        new QuSpacer(),
        new QuHorizontalLine(),
        new QuSpacer(),
        (new QuText(xstring("copyright")))->setItalic()
    });

    connect(fieldRef(FN_WHOSE_GOAL).data(), &FieldRef::valueChanged,
            this, &GboGPC::updateMandatory);

    page->setTitle(longname());

    m_questionnaire = new Questionnaire(m_app, {page});
    m_questionnaire->setReadOnly(read_only);

    updateMandatory();

    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

void GboGPC::updateMandatory()
{
    const bool required = valueInt(FN_WHOSE_GOAL) == gbocommon::AGENT_OTHER;
    fieldRef(FN_WHOSE_GOAL_OTHER)->setMandatory(required);
    if (!m_questionnaire) {
        return;
    }
    m_questionnaire->setVisibleByTag(TAG_OTHER, required);
}
