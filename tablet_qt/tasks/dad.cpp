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

#include "dad.h"
#include "common/textconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qugridcell.h"
#include "questionnairelib/qugridcontainer.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;

const QString Dad::DAD_TABLENAME("dad");

const int YES = 1;
const int NO = 0;
const int NA = -99;
const QString HYGIENE("hygiene");
const QString DRESSING("dressing");
const QString CONTINENCE("continence");
const QString EATING("eating");
const QString MEALPREP("mealprep");
const QString TELEPHONE("telephone");
const QString OUTING("outing");
const QString FINANCE("finance");
const QString MEDICATIONS("medications");
const QString LEISURE("leisure");
const QStringList GROUPS{
    HYGIENE,
    DRESSING,
    CONTINENCE,
    EATING,
    MEALPREP,
    TELEPHONE,
    OUTING,
    FINANCE,
    MEDICATIONS,
    LEISURE,
};
const QString INIT("init");
const QString PLAN("plan");
const QString EXEC("exec");
const QStringList ITEMS{
    "hygiene_init_wash",
    "hygiene_init_teeth",
    "hygiene_init_hair",
    "hygiene_plan_wash",
    "hygiene_exec_wash",
    "hygiene_exec_hair",
    "hygiene_exec_teeth",

    "dressing_init_dress",
    "dressing_plan_clothing",
    "dressing_plan_order",
    "dressing_exec_dress",
    "dressing_exec_undress",

    "continence_init_toilet",
    "continence_exec_toilet",

    "eating_init_eat",
    "eating_plan_utensils",
    "eating_exec_eat",

    "mealprep_init_meal",
    "mealprep_plan_meal",
    "mealprep_exec_meal",

    "telephone_init_phone",
    "telephone_plan_dial",
    "telephone_exec_conversation",
    "telephone_exec_message",

    "outing_init_outing",
    "outing_plan_outing",
    "outing_exec_reach_destination",
    "outing_exec_mode_transportation",
    "outing_exec_return_with_shopping",

    "finance_init_interest",
    "finance_plan_pay_bills",
    "finance_plan_organise_correspondence",
    "finance_exec_handle_money",

    "medications_init_medication",
    "medications_exec_take_medications",

    "leisure_init_interest_leisure",
    "leisure_init_interest_chores",
    "leisure_plan_chores",
    "leisure_exec_complete_chores",
    "leisure_exec_safe_at_home",
};
const int LEFTCOL_STRETCH = 1;
const int RIGHTCOL_STRETCH = 2;


void initializeDad(TaskFactory& factory)
{
    static TaskRegistrar<Dad> registered(factory);
}


Dad::Dad(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, DAD_TABLENAME, false, true, true)  // ... anon, clin, resp
{
    for (const QString& item : ITEMS) {
        addField(item, QVariant::Int);
    }

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Dad::shortname() const
{
    return "DAD";
}


QString Dad::longname() const
{
    return tr("Disability Assessment for Dementia");
}


QString Dad::description() const
{
    return tr("40-item clinician-administered, carer-rated scale.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Dad::isComplete() const
{
    return noneNull(values(ITEMS));
}


QStringList Dad::summary() const
{
    QStringList lines;
    lines.append("Total: " + getScore(ITEMS) + ".");
    lines.append("BADL ACTIVITIES: "
                 "hygiene " + getScore(getItemsActivity(HYGIENE)) +
                 "; dressing " + getScore(getItemsActivity(DRESSING)) +
                 "; continence " + getScore(getItemsActivity(CONTINENCE)) +
                 "; eating " + getScore(getItemsActivity(EATING)) + ".");
    lines.append("BADL OVERALL: " + getScore(getItemsActivities({
                            HYGIENE, DRESSING, CONTINENCE, EATING})) + ".");
    lines.append("IADL ACTIVITIES: "
                 "mealprep " + getScore(getItemsActivity(MEALPREP)) +
                 "; telephone " + getScore(getItemsActivity(TELEPHONE)) +
                 "; outing " + getScore(getItemsActivity(OUTING)) +
                 "; finance " + getScore(getItemsActivity(FINANCE)) +
                 "; medications " + getScore(getItemsActivity(MEDICATIONS)) +
                 "; leisure " + getScore(getItemsActivity(LEISURE)) + ".");
    lines.append("BADL OVERALL: " + getScore(getItemsActivities({
                            MEALPREP, TELEPHONE, OUTING,
                            FINANCE, MEDICATIONS, LEISURE})) + ".");
    lines.append("PHASES: "
                 "initiation " + getScore(getItemsPhase(INIT)) +
                 "; planning/organisation " + getScore(getItemsPhase(PLAN)) +
                 "; execution/performance " + getScore(getItemsPhase(EXEC)) + ".");
    return lines;
}


QStringList Dad::detail() const
{
    return completenessInfo() + summary();
}


OpenableWidget* Dad::editor(const bool read_only)
{
    const NameValueOptions y_n_na_options{
        {CommonOptions::yes(), YES},
        {CommonOptions::no(), NO},
        {TextConst::notApplicable(), NA},
    };

    QuPagePtr page1 = getClinicianAndRespondentDetailsPage(false);

    QVector<QuElement*> elements{
        (new QuText(xstring("instruction_1") + " " +
                    getPatientName() + " " +
                    xstring("instruction_2")))->setBold(),
    };
    for (const QString& groupname : GROUPS) {
        elements.append((new QuText(xstring(groupname)))
                        ->setBold()
                        ->setItalic());
        auto grid = new QuGridContainer();
        int row = 0;
        grid->setColumnStretch(0, LEFTCOL_STRETCH);
        grid->setColumnStretch(1, RIGHTCOL_STRETCH);
        for (const QString& itemname : getItemsActivity(groupname)) {
            grid->addCell(QuGridCell(new QuText(xstring(itemname)), row, 0));
            grid->addCell(QuGridCell(
                (new QuMcq(fieldRef(itemname),
                           y_n_na_options))->setHorizontal(true),
                row, 1));
            ++row;
        }
        elements.append(grid);
    }

    QuPagePtr page2((new QuPage(elements))->setTitle(longname()));

    auto questionnaire = new Questionnaire(m_app, {page1, page2});
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

QStringList Dad::getItemsActivity(const QString& activity) const
{
    QStringList activity_items;
    for (const QString& item : ITEMS) {
        if (item.startsWith(activity)) {
            activity_items.append(item);
        }
    }
    return activity_items;
}


QStringList Dad::getItemsActivities(const QStringList& activities) const
{
    QStringList activity_items;
    for (const QString& item : ITEMS) {
        for (const QString& activity : activities) {
            if (item.startsWith(activity)) {
                activity_items.append(item);
            }
        }
    }
    return activity_items;
}


QStringList Dad::getItemsPhase(const QString& phase) const
{
    QStringList phase_items;
    for (const QString& item : ITEMS) {
        if (item.contains(phase)) {
            phase_items.append(item);
        }
    }
    return phase_items;
}


QString Dad::getScore(const QStringList& fieldnames) const
{
    const QVector<QVariant> v = values(fieldnames);
    const int score = mathfunc::countWhere(v, QVector<QVariant>{YES});
    const int possible = mathfunc::countWhereNot(v, QVector<QVariant>{QVariant(), NA});
    return mathfunc::scoreString(score, possible);
}
