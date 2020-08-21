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

#include "gbogras.h"
#include "maths/mathfunc.h"
#include "lib/datetime.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/qudatetime.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quflowcontainer.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/quhorizontalline.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/quslider.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
#include "taskxtra/gbocommon.h"

using mathfunc::anyNullOrEmpty;
using stringfunc::strseq;

const QString GboGRaS::GBOGRAS_TABLENAME("gbogras");

const QString FN_DATE("date");
const QString FN_RATE_GOAL_1("rate_goal_1");
const QString FN_RATE_GOAL_2("rate_goal_2");
const QString FN_RATE_GOAL_3("rate_goal_3");
const QString FN_GOAL_1_DESC("goal_1_description");
const QString FN_GOAL_2_DESC("goal_2_description");
const QString FN_GOAL_3_DESC("goal_3_description");
const QString FN_GOAL_1_PROGRESS("goal_1_progress");
const QString FN_GOAL_2_PROGRESS("goal_2_progress");
const QString FN_GOAL_3_PROGRESS("goal_3_progress");
const QString FN_COMPLETED_BY("completed_by");
const QString FN_COMPLETED_BY_OTHER("completed_by_other");

const QString TAG_OTHER("other");


// ============================================================================
// Helper functions
// ============================================================================

QString getGoalTag(const int goalnum)
{
    return QString("goal%1").arg(goalnum);
}


// ============================================================================
// Registration
// ============================================================================

void initializeGboGRaS(TaskFactory& factory)
{
    static TaskRegistrar<GboGRaS> registered(factory);
}


// ============================================================================
// Constructor
// ============================================================================

GboGRaS::GboGRaS(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, GBOGRAS_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addField(FN_DATE, QVariant::Date);
    addField(FN_RATE_GOAL_1, QVariant::Bool);
    addField(FN_RATE_GOAL_2, QVariant::Bool);
    addField(FN_RATE_GOAL_3, QVariant::Bool);
    addField(FN_GOAL_1_DESC, QVariant::String);
    addField(FN_GOAL_2_DESC, QVariant::String);
    addField(FN_GOAL_3_DESC, QVariant::String);
    addField(FN_GOAL_1_PROGRESS, QVariant::Int);
    addField(FN_GOAL_2_PROGRESS, QVariant::Int);
    addField(FN_GOAL_3_PROGRESS, QVariant::Int);
    addField(FN_COMPLETED_BY, QVariant::Int);
    addField(FN_COMPLETED_BY_OTHER, QVariant::String);
    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.

    if (load_pk == dbconst::NONEXISTENT_PK) {
        setValue(FN_DATE, datetime::nowDate(), false);
    }
}


// ============================================================================
// Class info
// ============================================================================

QString GboGRaS::shortname() const
{
    return "GBO-GRaS";
}


QString GboGRaS::longname() const
{
    return tr("Goal-Based Outcomes – 3 – Goal Rating Sheet");
}


QString GboGRaS::description() const
{
    return tr("For recording progress towards the goals of therapy "
              "(up to 3 goals).");
}


QString GboGRaS::xstringTaskname() const
{
    return "gbo";
}


// ============================================================================
// Instance info
// ============================================================================

bool GboGRaS::isComplete() const
{
    if (anyValuesNullOrEmpty({FN_DATE, FN_COMPLETED_BY})) {
        return false;
    }
    if (value(FN_COMPLETED_BY) == gbocommon::AGENT_OTHER &&
            valueIsNullOrEmpty(FN_COMPLETED_BY_OTHER)) {
        return false;
    }
    int n_goals_completed = 0;

    auto goalOK = [this, &n_goals_completed]
                  (const QString& fn_rate,  const QString& fn_desc,
                   const QString& fn_progress) -> bool {
        if (valueBool(fn_rate)) {
            ++n_goals_completed;
            if (anyValuesNullOrEmpty({fn_desc, fn_progress})) {
                return false;
            }
        }
        return true;
    };

    if (!goalOK(FN_RATE_GOAL_1, FN_GOAL_1_DESC, FN_GOAL_1_PROGRESS)) {
        return false;
    }
    if (!goalOK(FN_RATE_GOAL_2, FN_GOAL_2_DESC, FN_GOAL_2_PROGRESS)) {
        return false;
    }
    if (!goalOK(FN_RATE_GOAL_3, FN_GOAL_3_DESC, FN_GOAL_3_PROGRESS)) {
        return false;
    }
    return n_goals_completed > 0;
}


QStringList GboGRaS::summary() const
{
    QStringList lines;

    auto doGoal = [this, &lines]
                  (const int goalnum, const QString& fn_rate,
                   const QString& fn_progress) -> void {
        if (!valueBool(fn_rate)) {
            return;
        }
        QString line = QString("Goal <b>%1</b>: progress <b>%2</b>/%3.").arg(
                    QString::number(goalnum),
                    prettyValue(fn_progress),
                    QString::number(gbocommon::PROGRESS_MAX));
        lines.append(line);
    };

    lines.append(QString("Date: <b>%1</b>.").arg(
                     datetime::dateToIso(valueDate(FN_DATE))));
    doGoal(1, FN_RATE_GOAL_1, FN_GOAL_1_PROGRESS);
    doGoal(2, FN_RATE_GOAL_2, FN_GOAL_2_PROGRESS);
    doGoal(3, FN_RATE_GOAL_3, FN_GOAL_3_PROGRESS);
    return lines;
}


QStringList GboGRaS::detail() const
{
    return summary();
}


OpenableWidget* GboGRaS::editor(const bool read_only)
{
    // ------------------------------------------------------------------------
    // Define options
    // ------------------------------------------------------------------------

    const NameValueOptions completed_by_options = NameValueOptions{
        { xstring("agent_1"), gbocommon::AGENT_PATIENT },
        { xstring("agent_2"), gbocommon::AGENT_PARENT_CARER },
        // not 3: clinician
        { xstring("agent_4"), gbocommon::AGENT_OTHER },
    };

    QMap<int, QString> slider_tick_map;
    for (int r = gbocommon::PROGRESS_MIN; r <= gbocommon::PROGRESS_MAX; ++r) {
        QString label;
        switch (r) {
        case 0:
        case 5:
        case 10:
            label = xstring(QString("gras_anchor_%1").arg(r));
            break;
        default:
            break;
        }
        label = label + "\n" + QString::number(r);
        slider_tick_map[r] = label;
    }

    // ------------------------------------------------------------------------
    // Starting elements
    // ------------------------------------------------------------------------

    QVector<QuElement*> elements{
        new QuFlowContainer{
                new QuHeading(xstring("date")),
                (new QuDateTime(fieldRef(FN_DATE)))
                    ->setMode(QuDateTime::DefaultDate)
                    ->setOfferNowButton(true),
        },
        (new QuText(xstring("gras_question")))
                ->setBig()
                ->setBold()
                ->setItalic(),
        new QuText(xstring("gras_instruction")),
        (new QuText(xstring("progress_explanation")))->setItalic(),
        new QuSpacer(),
    };

    // ------------------------------------------------------------------------
    // Goal rating elements
    // ------------------------------------------------------------------------

    auto addGoal = [this, &elements, &slider_tick_map]
                   (const int goalnum, const QString& fn_rate,
                    const QString& fn_desc,
                    const QString& fn_progress) -> void {
        const QString tag = getGoalTag(goalnum);
        // I tried with a QuMcqGrid but a slider is much better at an evenly
        // distributed set of responses where some have (textually lengthy)
        // anchor points.
        QuSlider* slider = new QuSlider(fieldRef(fn_progress),
                                        gbocommon::PROGRESS_MIN,
                                        gbocommon::PROGRESS_MAX);
        slider->setTickPosition(QSlider::TicksBothSides);
        slider->setTickLabelPosition(QSlider::TicksAbove);
        slider->addTag(tag);
        slider->setTickLabels(slider_tick_map);
        slider->setBigStep(1);
        elements.append({
            new QuHeading(xstring(QString("goal_rating_heading_%1").arg(goalnum))),
            (new QuBoolean(xstring("rate_goal"), fieldRef(fn_rate))),
            (new QuText(xstring("gras_desc_instruction")))->addTag(tag),
            (new QuTextEdit(fieldRef(fn_desc)))->addTag(tag),
            (new QuText(xstring("gras_rate_instruction")))->addTag(tag),
            slider,
            (new QuSpacer())->addTag(tag),
        });
        connect(fieldRef(fn_rate).data(), &FieldRef::valueChanged,
                this, &GboGRaS::updateMandatory);
    };

    addGoal(1, FN_RATE_GOAL_1, FN_GOAL_1_DESC, FN_GOAL_1_PROGRESS);
    addGoal(2, FN_RATE_GOAL_2, FN_GOAL_2_DESC, FN_GOAL_2_PROGRESS);
    addGoal(3, FN_RATE_GOAL_3, FN_GOAL_3_DESC, FN_GOAL_3_PROGRESS);

    // ------------------------------------------------------------------------
    // Closing elements
    // ------------------------------------------------------------------------

    elements.append({
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

    // ------------------------------------------------------------------------
    // Page, questionnaire, other setup
    // ------------------------------------------------------------------------

    m_page = new QuPage(elements);

    connect(fieldRef(FN_COMPLETED_BY).data(), &FieldRef::valueChanged,
            this, &GboGRaS::updateMandatory);

    m_page->setTitle(longname());

    m_questionnaire = new Questionnaire(m_app, {m_page});
    m_questionnaire->setReadOnly(read_only);

    updateMandatory();

    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

void GboGRaS::updateMandatory()
{
    const bool other = valueInt(FN_COMPLETED_BY) == gbocommon::AGENT_OTHER;
    fieldRef(FN_COMPLETED_BY_OTHER)->setMandatory(other);
    if (!m_questionnaire || !m_page) {
        return;
    }
    m_questionnaire->setVisibleByTag(TAG_OTHER, other);

    int n_goals = 0;
    auto doGoal = [this, &n_goals](const int goalnum,
                                   const QString& fn_rate) -> void {
        const QString tag = getGoalTag(goalnum);
        const bool rate = valueBool(fn_rate);
        m_questionnaire->setVisibleByTag(tag, rate);
        if (rate) {
            ++n_goals;
        }
    };
    doGoal(1, FN_RATE_GOAL_1);
    doGoal(2, FN_RATE_GOAL_2);
    doGoal(3, FN_RATE_GOAL_3);
    // We need at least one goal:
    m_page->blockProgress(n_goals == 0);
}
