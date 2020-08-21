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

#include "elixhauserci.h"
#include "common/textconst.h"
#include "common/uiconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/qubutton.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;

const QString ElixhauserCI::ELIXHAUSERCI_TABLENAME("elixhauserci");

const QStringList FIELDNAMES{
    "congestive_heart_failure",
    "cardiac_arrhythmias",
    "valvular_disease",
    "pulmonary_circulation_disorders",
    "peripheral_vascular_disorders",

    "hypertension_uncomplicated",
    "hypertension_complicated",
    "paralysis",
    "other_neurological_disorders",
    "chronic_pulmonary_disease",

    "diabetes_uncomplicated",
    "diabetes_complicated",
    "hypothyroidism",
    "renal_failure",
    "liver_disease",

    "peptic_ulcer_disease_exc_bleeding",
    "aids_hiv",
    "lymphoma",
    "metastatic_cancer",
    "solid_tumor_without_metastasis",

    "rheumatoid_arthritis_collagen_vascular_diseases",
    "coagulopathy",
    "obesity",
    "weight_loss",
    "fluid_electrolyte_disorders",

    "blood_loss_anemia",
    "deficiency_anemia",
    "alcohol_abuse",
    "drug_abuse",
    "psychoses",

    "depression",
};
const int MAX_SCORE = FIELDNAMES.length();

void initializeElixhauserCI(TaskFactory& factory)
{
    static TaskRegistrar<ElixhauserCI> registered(factory);
}


ElixhauserCI::ElixhauserCI(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, ELIXHAUSERCI_TABLENAME, false, true, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    for (const QString& fieldname : FIELDNAMES) {
        addField(fieldname, QVariant::Bool);
    }

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString ElixhauserCI::shortname() const
{
    return "ElixhauserCI";
}


QString ElixhauserCI::longname() const
{
    return tr("Elixhauser Comorbidity Index");
}


QString ElixhauserCI::description() const
{
    return tr("31-item clinician-rated comorbidity catalogue.");
}


// ============================================================================
// Instance info
// ============================================================================

bool ElixhauserCI::isComplete() const
{
    return noValuesNull(FIELDNAMES);
}


QStringList ElixhauserCI::summary() const
{
    return QStringList{totalScorePhrase(totalScore(), MAX_SCORE)};
}


QStringList ElixhauserCI::detail() const
{
    using uifunc::yesNoNull;
    QStringList lines = completenessInfo();
    for (const QString& fieldname : FIELDNAMES) {
        const QVariant& v = value(fieldname);
        lines.append(QString("%1: <b>%2</b>").arg(fieldname, yesNoNull(v)));
    }
    lines.append("");
    lines += summary();
    return lines;
}


OpenableWidget* ElixhauserCI::editor(const bool read_only)
{
    const QString& title = longname();
    QuPagePtr clinicianpage = getClinicianDetailsPage();
    clinicianpage->setTitle(title + " " + textconst.page() + " 1");

    QuPagePtr mainpage(new QuPage());
    mainpage->setTitle(title + " " + textconst.page() + " 2");
    mainpage->addElement(new QuText(xstring("instruction")));
    auto all_absent_button = new QuButton(
        xstring("mark_all_unmarked_absent"),
        std::bind(&ElixhauserCI::markAllUnmarkedAbsent, this)
    );
    mainpage->addElement(all_absent_button);
    mainpage->addElement(
        new QuSpacer(QSize(uiconst::BIGSPACE, uiconst::BIGSPACE))
    );

    m_fieldrefs.clear();

    for (const QString& fieldname : FIELDNAMES) {
        const QString& description = xstring(fieldname);
        FieldRefPtr field = fieldRef(fieldname);
        QuBoolean* element = new QuBoolean(description, field);
        m_fieldrefs.append(field);
        element->setAsTextButton();
        mainpage->addElement(element);
    }

    QVector<QuPagePtr> pages{clinicianpage, mainpage};
    m_questionnaire = new Questionnaire(m_app, pages);
    m_questionnaire->setType(QuPage::PageType::Clinician);
    m_questionnaire->setReadOnly(read_only);
    return m_questionnaire;
}

void ElixhauserCI::markAllUnmarkedAbsent()
{
    for (const FieldRefPtr& field : m_fieldrefs) {
        if (field->value().isNull()) {
            field->setValue(false);
        }
    }
}

// ============================================================================
// Task-specific calculations
// ============================================================================

int ElixhauserCI::totalScore() const
{
    return sumInt(values(FIELDNAMES));
}
