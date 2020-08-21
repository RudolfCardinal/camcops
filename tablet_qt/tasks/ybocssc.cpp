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

#include "ybocssc.h"
#include "common/textconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/questionnairefunc.h"
#include "questionnairelib/qugridcontainer.h"
#include "questionnairelib/qulineedit.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"

const QString YbocsSc::YBOCSSC_TABLENAME("ybocssc");

const QString SC_PREFIX("sc_");
const QString SUFFIX_CURRENT("_current");
const QString SUFFIX_PAST("_past");
const QString SUFFIX_PRINCIPAL("_principal");
const QString SUFFIX_OTHER("_other");
const QString SUFFIX_DETAIL("_detail");
const QStringList GROUPS{
    "obs_aggressive",
    "obs_contamination",
    "obs_sexual",
    "obs_hoarding",
    "obs_religious",
    "obs_symmetry",
    "obs_misc",
    "obs_somatic",
    "com_cleaning",
    "com_checking",
    "com_repeat",
    "com_counting",
    "com_arranging",
    "com_hoarding",
    "com_misc",
};
const QStringList ITEMS{
    "obs_aggressive_harm_self",
    "obs_aggressive_harm_others",
    "obs_aggressive_imagery",
    "obs_aggressive_obscenities",
    "obs_aggressive_embarrassing",
    "obs_aggressive_impulses",
    "obs_aggressive_steal",
    "obs_aggressive_accident",
    "obs_aggressive_responsible",
    "obs_aggressive_other",

    "obs_contamination_bodily_waste",
    "obs_contamination_dirt",
    "obs_contamination_environmental",
    "obs_contamination_household",
    "obs_contamination_animals",
    "obs_contamination_sticky",
    "obs_contamination_ill",
    "obs_contamination_others_ill",
    "obs_contamination_feeling",
    "obs_contamination_other",

    "obs_sexual_forbidden",
    "obs_sexual_children_incest",
    "obs_sexual_homosexuality",
    "obs_sexual_to_others",
    "obs_sexual_other",

    "obs_hoarding_other",

    "obs_religious_sacrilege",
    "obs_religious_morality",
    "obs_religious_other",

    "obs_symmetry_with_magical",
    "obs_symmetry_without_magical",

    "obs_misc_know_remember",
    "obs_misc_fear_saying",
    "obs_misc_fear_not_saying",
    "obs_misc_fear_losing",
    "obs_misc_intrusive_nonviolent_images",
    "obs_misc_intrusive_sounds",
    "obs_misc_bothered_sounds",
    "obs_misc_numbers",
    "obs_misc_colours",
    "obs_misc_superstitious",
    "obs_misc_other",

    "obs_somatic_illness",
    "obs_somatic_appearance",
    "obs_somatic_other",

    "com_cleaning_handwashing",
    "com_cleaning_toileting",
    "com_cleaning_cleaning_items",
    "com_cleaning_other_contaminant_avoidance",
    "com_cleaning_other",

    "com_checking_locks_appliances",
    "com_checking_not_harm_others",
    "com_checking_not_harm_self",
    "com_checking_nothing_bad_happens",
    "com_checking_no_mistake",
    "com_checking_somatic",
    "com_checking_other",

    "com_repeat_reread_rewrite",
    "com_repeat_routine",
    "com_repeat_other",

    "com_counting_other",

    "com_arranging_other",

    "com_hoarding_other",

    "com_misc_mental_rituals",
    "com_misc_lists",
    "com_misc_tell_ask",
    "com_misc_touch",
    "com_misc_blink_stare",
    "com_misc_prevent_harm_self",
    "com_misc_prevent_harm_others",
    "com_misc_prevent_terrible",
    "com_misc_eating_ritual",
    "com_misc_superstitious",
    "com_misc_trichotillomania",
    "com_misc_self_harm",
    "com_misc_other",
};


void initializeYbocsSc(TaskFactory& factory)
{
    static TaskRegistrar<YbocsSc> registered(factory);
}


YbocsSc::YbocsSc(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, YBOCSSC_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    for (const QString& item : ITEMS) {
        addField(item + SUFFIX_CURRENT, QVariant::Bool);
        addField(item + SUFFIX_PAST, QVariant::Bool);
        addField(item + SUFFIX_PRINCIPAL, QVariant::Bool);
        if (item.endsWith(SUFFIX_OTHER)) {
            addField(item + SUFFIX_DETAIL, QVariant::String);
        }
    }

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString YbocsSc::shortname() const
{
    return "Y-BOCS-SC";
}


QString YbocsSc::longname() const
{
    return tr("Y-BOCS Symptom Checklist, 9/89 revision");
}


QString YbocsSc::description() const
{
    return tr("Symptom checklist (past, current, principal) for Y-BOCS.");
}


QString YbocsSc::infoFilenameStem() const
{
    return "ybocs";
}


QString YbocsSc::xstringTaskname() const
{
    return "ybocs";
}


// ============================================================================
// Instance info
// ============================================================================

bool YbocsSc::isComplete() const
{
    return true;
}


QStringList YbocsSc::summary() const
{
    return QStringList{TextConst::seeFacsimile()};
}


QStringList YbocsSc::detail() const
{
    return completenessInfo() + summary();
}


OpenableWidget* YbocsSc::editor(const bool read_only)
{
    auto text = [this](const QString& xstringname) -> QuElement* {
        return new QuText(xstring(xstringname));
    };
    auto textRaw = [](const QString& text) -> QuElement* {
        return new QuText(text);
    };
    auto boldtext = [this](const QString& xstringname) -> QuElement* {
        return (new QuText(xstring(xstringname)))->setBold(true);
    };
    auto booltext = [this](const QString& fieldname,
                           const QString& text,
                           bool mandatory = false) -> QuElement* {
        QuBoolean* b = new QuBoolean(text, fieldRef(fieldname, mandatory));
        b->setAsTextButton(true);
        return b;
    };

    QVector<QuElement*> elements{
        boldtext("sc_instruction_1"),
        text("sc_instruction_2"),
    };

    const QString& current = tr("Current");
    const QString& past = tr("Past");
    const QString& principal = tr("Principal");
    const QString& specify = tr("... specify:");
    QVector<QuGridCell> cells;
    int row = 0;
    for (const QString& group : GROUPS) {
        cells.append(QuGridCell(boldtext(SC_PREFIX + group), row, 0, 1, 5));
        ++row;
        for (const QString& item : ITEMS) {
            if (!item.startsWith(group)) {
                continue;
            }
            cells.append(QuGridCell(text(SC_PREFIX + item), row, 0, 1, 2));
            cells.append(QuGridCell(booltext(item + SUFFIX_CURRENT,
                                             current), row, 2));
            cells.append(QuGridCell(booltext(item + SUFFIX_PAST,
                                             past), row, 3));
            cells.append(QuGridCell(booltext(item + SUFFIX_PRINCIPAL,
                                             principal), row, 4));
            ++row;
            if (item.endsWith(SUFFIX_OTHER)) {
                cells.append(QuGridCell(textRaw(specify), row, 0));
                cells.append(QuGridCell(
                        new QuLineEdit(fieldRef(item + SUFFIX_DETAIL, false)),
                        row, 1, 1, 4));
                ++row;
            }
        }
    }
    auto container = new QuGridContainer(cells);
    container->setColumnStretch(0, 2);
    container->setColumnStretch(1, 3);
    container->setColumnStretch(2, 1);
    container->setColumnStretch(3, 1);
    container->setColumnStretch(4, 1);
    container->setFixedGrid(false);
    container->setExpandHorizontally(false);
    elements.append(container);

    QVector<QuPagePtr> pages;
    pages.append(getClinicianDetailsPage());
    pages.append(QuPagePtr((new QuPage(elements))
                           ->setTitle(xstring("sc_title"))));

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================
