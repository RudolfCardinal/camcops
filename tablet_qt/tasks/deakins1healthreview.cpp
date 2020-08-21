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

// This task doesn't bother with XML downloads or translation; it's hard-coded.

#include "deakins1healthreview.h"
#include "core/camcopsapp.h"
#include "common/textconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qulineeditinteger.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qumcqgridsingleboolean.h"
#include "questionnairelib/qumultipleresponse.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
#include "tasks/gmcpq.h"  // for ethnicity options
using mathfunc::noneNull;

const QString DeakinS1HealthReview::DEAKIN_S1_HEALTHREVIEW_TABLENAME(
        "deakin_1_healthreview");

const QString ETHNICITY("ethnicity");
const QString ETHNICITY_TEXT("ethnicity_text");
const QString ETHNICITY_OTHER_DETAILS("ethnicity_other_details");

const QString HANDEDNESS("handedness");

const QString EDUCATION("education");

const QString ALLERGIES("allergies");
const QString ALLERGY_ASTHMA("allergy_asthma");
const QString ALLERGY_POLLEN_DUST("allergy_pollen_dust");
const QString ALLERGY_DERMATITIS("allergy_dermatitis");
const QString ALLERGY_FOOD("allergy_food");
const QString ALLERGY_DANDER("allergy_dander");
const QString ALLERGY_OTHER("allergy_other");
const QString ALLERGY_DETAILS("allergy_details");

const QString VACCINATIONS_LAST3MONTHS("vaccinations_last3months");
const QString VACCINATION_DETAILS("vaccination_details");

const QString INFECTIONS_LAST3MONTHS("infections_last3months");
const QString INFECTION_RECENT_RESPIRATORY("infection_recent_respiratory");
const QString INFECTION_RECENT_GASTROENTERITIS("infection_recent_gastroenteritis");
const QString INFECTION_RECENT_URINARY("infection_recent_urinary");
const QString INFECTION_RECENT_SEXUAL("infection_recent_sexual");
const QString INFECTION_RECENT_HEPATITIS("infection_recent_hepatitis");
const QString INFECTION_RECENT_OTHER("infection_recent_other");
const QString INFECTION_RECENT_DETAILS("infection_recent_details");

const QString INFECTIONS_CHRONIC("infections_chronic");
const QString INFECTION_CHRONIC_RESPIRATORY("infection_chronic_respiratory");
const QString INFECTION_CHRONIC_GASTROENTERITIS("infection_chronic_gastroenteritis");
const QString INFECTION_CHRONIC_URINARY("infection_chronic_urinary");
const QString INFECTION_CHRONIC_SEXUAL("infection_chronic_sexual");
const QString INFECTION_CHRONIC_HEPATITIS("infection_chronic_hepatitis");
const QString INFECTION_CHRONIC_OTHER("infection_chronic_other");
const QString INFECTION_CHRONIC_DETAILS("infection_chronic_details");

const QString IMMUNE_DISORDERS("immune_disorders");
const QString IMMUNITY_MS("immunity_ms");
const QString IMMUNITY_SLE("immunity_sle");
const QString IMMUNITY_ARTHRITIS("immunity_arthritis");
const QString IMMUNITY_HIV("immunity_hiv");
const QString IMMUNITY_GRAVES("immunity_graves");
const QString IMMUNITY_DIABETES("immunity_diabetes");
const QString IMMUNITY_OTHER("immunity_other");
const QString IMMUNITY_DETAILS("immunity_details");

const QString FAMILY_HISTORY("family_history");
const QString FAMILYHISTORY_MS("familyhistory_ms");
const QString FAMILYHISTORY_SLE("familyhistory_sle");
const QString FAMILYHISTORY_ARTHRITIS("familyhistory_arthritis");
const QString FAMILYHISTORY_GRAVES("familyhistory_graves");
const QString FAMILYHISTORY_DIABETES("familyhistory_diabetes");
const QString FAMILYHISTORY_PSYCHOSIS_SZ("familyhistory_psychosis_sz");
const QString FAMILYHISTORY_BIPOLAR("familyhistory_bipolar");
const QString FAMILYHISTORY_DETAILS("familyhistory_details");

const QString HEALTH_ANYTHING_ELSE("health_anything_else");
const QString HEALTH_ANYTHING_ELSE_DETAILS("health_anything_else_details");

const QString DRUG_HISTORY("drug_history");
const QString FIRST_ANTIPSYCHOTIC_MEDICATION("first_antipsychotic_medication");

const QString RECREATIONAL_DRUG_IN_LAST_3_MONTHS("recreational_drug_in_last_3_months");
const QString RECDRUG_TOBACCO_FREQUENCY("recdrug_tobacco_frequency");
const QString RECDRUG_TOBACCO_CIGSPERWEEK("recdrug_tobacco_cigsperweek");
const QString RECDRUG_TOBACCO_PREVHEAVY("recdrug_tobacco_prevheavy");
const QString RECDRUG_CANNABIS_FREQUENCY("recdrug_cannabis_frequency");
const QString RECDRUG_CANNABIS_JOINTSPERWEEK("recdrug_cannabis_jointsperweek");
const QString RECDRUG_CANNABIS_PREVHEAVY("recdrug_cannabis_prevheavy");
const QString RECDRUG_ALCOHOL_FREQUENCY("recdrug_alcohol_frequency");
const QString RECDRUG_ALCOHOL_UNITSPERWEEK("recdrug_alcohol_unitsperweek");
const QString RECDRUG_ALCOHOL_PREVHEAVY("recdrug_alcohol_prevheavy");
const QString RECDRUG_MDMA_FREQUENCY("recdrug_mdma_frequency");
const QString RECDRUG_MDMA_PREVHEAVY("recdrug_mdma_prevheavy");
const QString RECDRUG_COCAINE_FREQUENCY("recdrug_cocaine_frequency");
const QString RECDRUG_COCAINE_PREVHEAVY("recdrug_cocaine_prevheavy");
const QString RECDRUG_CRACK_FREQUENCY("recdrug_crack_frequency");
const QString RECDRUG_CRACK_PREVHEAVY("recdrug_crack_prevheavy");
const QString RECDRUG_HEROIN_FREQUENCY("recdrug_heroin_frequency");
const QString RECDRUG_HEROIN_PREVHEAVY("recdrug_heroin_prevheavy");
const QString RECDRUG_METHADONE_FREQUENCY("recdrug_methadone_frequency");
const QString RECDRUG_METHADONE_PREVHEAVY("recdrug_methadone_prevheavy");
const QString RECDRUG_AMPHETAMINES_FREQUENCY("recdrug_amphetamines_frequency");
const QString RECDRUG_AMPHETAMINES_PREVHEAVY("recdrug_amphetamines_prevheavy");
const QString RECDRUG_BENZODIAZEPINES_FREQUENCY("recdrug_benzodiazepines_frequency");
const QString RECDRUG_BENZODIAZEPINES_PREVHEAVY("recdrug_benzodiazepines_prevheavy");
const QString RECDRUG_KETAMINE_FREQUENCY("recdrug_ketamine_frequency");
const QString RECDRUG_KETAMINE_PREVHEAVY("recdrug_ketamine_prevheavy");
const QString RECDRUG_LEGALHIGHS_FREQUENCY("recdrug_legalhighs_frequency");
const QString RECDRUG_LEGALHIGHS_PREVHEAVY("recdrug_legalhighs_prevheavy");
const QString RECDRUG_INHALANTS_FREQUENCY("recdrug_inhalants_frequency");
const QString RECDRUG_INHALANTS_PREVHEAVY("recdrug_inhalants_prevheavy");
const QString RECDRUG_HALLUCINOGENS_FREQUENCY("recdrug_hallucinogens_frequency");
const QString RECDRUG_HALLUCINOGENS_PREVHEAVY("recdrug_hallucinogens_prevheavy");
const QString RECDRUG_DETAILS("recdrug_details");
const QString RECDRUG_PREVHEAVY("recdrug_prevheavy");
const QString RECDRUG_PREVHEAVY_DETAILS("recdrug_prevheavy_details");

const QString MRI_CLAUSTROPHOBIC("mri_claustrophobic");
const QString MRI_DIFFICULTY_LYING_1_HOUR("mri_difficulty_lying_1_hour");
const QString MRI_NONREMOVABLE_METAL("mri_nonremovable_metal");
const QString MRI_METAL_FROM_OPERATIONS("mri_metal_from_operations");
const QString MRI_TATTOOS_NICOTINE_PATCHES("mri_tattoos_nicotine_patches");
const QString MRI_WORKED_WITH_METAL("mri_worked_with_metal");
const QString MRI_PREVIOUS_BRAIN_SCAN("mri_previous_brain_scan");
const QString MRI_PREVIOUS_BRAIN_SCAN_DETAILS("mri_previous_brain_scan_details");
const QString OTHER_RELEVANT_THINGS("other_relevant_things");
const QString OTHER_RELEVANT_THINGS_DETAILS("other_relevant_things_details");
const QString WILLING_TO_PARTICIPATE_IN_FURTHER_STUDIES("willing_to_participate_in_further_studies");

const QString STR_DETAILS_IF_YES("If you answered YES, please give details:");
const QString STR_DETAILS("Details:");
const QString TICK_ANY_THAT_APPLY("Tick any that apply:");
const QStringList DRUGLIST{  // order is important
    "tobacco",
    "cannabis",
    "alcohol",
    "Ecstasy (MDMA)",
    "cocaine",
    "crack cocaine",
    "amphetamines",
    "heroin",
    "methadone (heroin substitute)",
    "benzodiazepines",
    "ketamine",
    "legal highs (e.g. Salvia)",
    "inhalants",
    "hallucinogens",
};
const QStringList INFECTIONLIST{  // order is important
    "respiratory infection",
    "gastroenteritis",
    "urinary tract infection",
    "sexually transmitted infection",
    "hepatitis",
    "other"
};
const QString PT_ETHNICITY("eth");
const QString PT_ALLERGY("all");
const QString PT_VACCINES("vac");
const QString PT_ACUTE_INFECTIONS("acinf");
const QString PT_CHRONIC_INFECTIONS("chinf");
const QString PT_IMMUNE("imm");
const QString PT_FH("fh");
const QString PT_HEALTH_OTHER("ho");
const QString PT_RECDRUGS("recdrug");
const QString PT_MRI("mri");
const QString ET_ETHNICITY_OTHER("eth_other");
const QString ET_ALLERGY("all");
const QString ET_VACCINES("vacc");
const QString ET_ACUTE_INFECTIONS("acinf");
const QString ET_CHRONIC_INFECTIONS("chinf");
const QString ET_IMMUNE("imm");
const QString ET_FH("fh");
const QString ET_HEALTH_OTHER("ho");
const QString ET_RECDRUGS("recdrug");
const QString ET_PREVSCAN("prevscan");
const QString ET_OTHERDETAILS("otherdetails");


void initializeDeakinS1HealthReview(TaskFactory& factory)
{
    static TaskRegistrar<DeakinS1HealthReview> registered(factory);
}


DeakinS1HealthReview::DeakinS1HealthReview(
        CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, DEAKIN_S1_HEALTHREVIEW_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addField(ETHNICITY, QVariant::Int);
    addField(ETHNICITY_TEXT, QVariant::String);  // SEEMS TO BE UNUSED!
    addField(ETHNICITY_OTHER_DETAILS, QVariant::String);

    addField(HANDEDNESS, QVariant::String);

    addField(EDUCATION, QVariant::String);

    addField(ALLERGIES, QVariant::Bool);
    addField(ALLERGY_ASTHMA, QVariant::Bool);
    addField(ALLERGY_POLLEN_DUST, QVariant::Bool);
    addField(ALLERGY_DERMATITIS, QVariant::Bool);
    addField(ALLERGY_FOOD, QVariant::Bool);
    addField(ALLERGY_DANDER, QVariant::Bool);
    addField(ALLERGY_OTHER, QVariant::Bool);
    addField(ALLERGY_DETAILS, QVariant::String);

    addField(VACCINATIONS_LAST3MONTHS, QVariant::Bool);
    addField(VACCINATION_DETAILS, QVariant::String);

    addField(INFECTIONS_LAST3MONTHS, QVariant::Bool);
    addField(INFECTION_RECENT_RESPIRATORY, QVariant::Bool);
    addField(INFECTION_RECENT_GASTROENTERITIS, QVariant::Bool);
    addField(INFECTION_RECENT_URINARY, QVariant::Bool);
    addField(INFECTION_RECENT_SEXUAL, QVariant::Bool);
    addField(INFECTION_RECENT_HEPATITIS, QVariant::Bool);
    addField(INFECTION_RECENT_OTHER, QVariant::Bool);
    addField(INFECTION_RECENT_DETAILS, QVariant::String);

    addField(INFECTIONS_CHRONIC, QVariant::Bool);
    addField(INFECTION_CHRONIC_RESPIRATORY, QVariant::Bool);
    addField(INFECTION_CHRONIC_GASTROENTERITIS, QVariant::Bool);
    addField(INFECTION_CHRONIC_URINARY, QVariant::Bool);
    addField(INFECTION_CHRONIC_SEXUAL, QVariant::Bool);
    addField(INFECTION_CHRONIC_HEPATITIS, QVariant::Bool);
    addField(INFECTION_CHRONIC_OTHER, QVariant::Bool);
    addField(INFECTION_CHRONIC_DETAILS, QVariant::String);

    addField(IMMUNE_DISORDERS, QVariant::Bool);
    addField(IMMUNITY_MS, QVariant::Bool);
    addField(IMMUNITY_SLE, QVariant::Bool);
    addField(IMMUNITY_ARTHRITIS, QVariant::Bool);
    addField(IMMUNITY_HIV, QVariant::Bool);
    addField(IMMUNITY_GRAVES, QVariant::Bool);
    addField(IMMUNITY_DIABETES, QVariant::Bool);
    addField(IMMUNITY_OTHER, QVariant::Bool);
    addField(IMMUNITY_DETAILS, QVariant::String);

    addField(FAMILY_HISTORY, QVariant::Bool);
    addField(FAMILYHISTORY_MS, QVariant::Bool);
    addField(FAMILYHISTORY_SLE, QVariant::Bool);
    addField(FAMILYHISTORY_ARTHRITIS, QVariant::Bool);
    addField(FAMILYHISTORY_GRAVES, QVariant::Bool);
    addField(FAMILYHISTORY_DIABETES, QVariant::Bool);
    addField(FAMILYHISTORY_PSYCHOSIS_SZ, QVariant::Bool);
    addField(FAMILYHISTORY_BIPOLAR, QVariant::Bool);
    addField(FAMILYHISTORY_DETAILS, QVariant::String);

    addField(HEALTH_ANYTHING_ELSE, QVariant::Bool);
    addField(HEALTH_ANYTHING_ELSE_DETAILS, QVariant::String);

    addField(DRUG_HISTORY, QVariant::String);
    addField(FIRST_ANTIPSYCHOTIC_MEDICATION, QVariant::String);

    addField(RECREATIONAL_DRUG_IN_LAST_3_MONTHS, QVariant::Bool);
    addField(RECDRUG_TOBACCO_FREQUENCY, QVariant::Int);
    addField(RECDRUG_TOBACCO_CIGSPERWEEK, QVariant::Int);
    addField(RECDRUG_TOBACCO_PREVHEAVY, QVariant::Bool);
    addField(RECDRUG_CANNABIS_FREQUENCY, QVariant::Int);
    addField(RECDRUG_CANNABIS_JOINTSPERWEEK, QVariant::Int);
    addField(RECDRUG_CANNABIS_PREVHEAVY, QVariant::Bool);
    addField(RECDRUG_ALCOHOL_FREQUENCY, QVariant::Int);
    addField(RECDRUG_ALCOHOL_UNITSPERWEEK, QVariant::Int);
    addField(RECDRUG_ALCOHOL_PREVHEAVY, QVariant::Bool);
    addField(RECDRUG_MDMA_FREQUENCY, QVariant::Int);
    addField(RECDRUG_MDMA_PREVHEAVY, QVariant::Bool);
    addField(RECDRUG_COCAINE_FREQUENCY, QVariant::Int);
    addField(RECDRUG_COCAINE_PREVHEAVY, QVariant::Bool);
    addField(RECDRUG_CRACK_FREQUENCY, QVariant::Int);
    addField(RECDRUG_CRACK_PREVHEAVY, QVariant::Bool);
    addField(RECDRUG_HEROIN_FREQUENCY, QVariant::Int);
    addField(RECDRUG_HEROIN_PREVHEAVY, QVariant::Bool);
    addField(RECDRUG_METHADONE_FREQUENCY, QVariant::Int);
    addField(RECDRUG_METHADONE_PREVHEAVY, QVariant::Bool);
    addField(RECDRUG_AMPHETAMINES_FREQUENCY, QVariant::Int);
    addField(RECDRUG_AMPHETAMINES_PREVHEAVY, QVariant::Bool);
    addField(RECDRUG_BENZODIAZEPINES_FREQUENCY, QVariant::Int);
    addField(RECDRUG_BENZODIAZEPINES_PREVHEAVY, QVariant::Bool);
    addField(RECDRUG_KETAMINE_FREQUENCY, QVariant::Int);
    addField(RECDRUG_KETAMINE_PREVHEAVY, QVariant::Bool);
    addField(RECDRUG_LEGALHIGHS_FREQUENCY, QVariant::Int);
    addField(RECDRUG_LEGALHIGHS_PREVHEAVY, QVariant::Bool);
    addField(RECDRUG_INHALANTS_FREQUENCY, QVariant::Int);
    addField(RECDRUG_INHALANTS_PREVHEAVY, QVariant::Bool);
    addField(RECDRUG_HALLUCINOGENS_FREQUENCY, QVariant::Int);
    addField(RECDRUG_HALLUCINOGENS_PREVHEAVY, QVariant::Bool);
    addField(RECDRUG_DETAILS, QVariant::String);
    addField(RECDRUG_PREVHEAVY, QVariant::Bool);
    addField(RECDRUG_PREVHEAVY_DETAILS, QVariant::String);

    addField(MRI_CLAUSTROPHOBIC, QVariant::Bool);
    addField(MRI_DIFFICULTY_LYING_1_HOUR, QVariant::Bool);
    addField(MRI_NONREMOVABLE_METAL, QVariant::Bool);
    addField(MRI_METAL_FROM_OPERATIONS, QVariant::Bool);
    addField(MRI_TATTOOS_NICOTINE_PATCHES, QVariant::Bool);
    addField(MRI_WORKED_WITH_METAL, QVariant::Bool);
    addField(MRI_PREVIOUS_BRAIN_SCAN, QVariant::Bool);
    addField(MRI_PREVIOUS_BRAIN_SCAN_DETAILS, QVariant::String);
    addField(OTHER_RELEVANT_THINGS, QVariant::Bool);
    addField(OTHER_RELEVANT_THINGS_DETAILS, QVariant::String);
    addField(WILLING_TO_PARTICIPATE_IN_FURTHER_STUDIES, QVariant::Bool);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString DeakinS1HealthReview::shortname() const
{
    return "Deakin_S1_HealthReview";
}


QString DeakinS1HealthReview::longname() const
{
    return tr("Deakin JB — Antibody-mediated psychosis study — health review");
}


QString DeakinS1HealthReview::description() const
{
    return tr("Health review for antibody-mediated psychosis study.");
}


QString DeakinS1HealthReview::infoFilenameStem() const
{
    return "deakin_s1_healthreview";
}


// ============================================================================
// Instance info
// ============================================================================

bool DeakinS1HealthReview::isComplete() const
{
    return noneNull(values(QStringList{
       ETHNICITY,
       HANDEDNESS,
       EDUCATION,
       ALLERGIES,
       VACCINATIONS_LAST3MONTHS,
       INFECTIONS_LAST3MONTHS,
       INFECTIONS_CHRONIC,
       IMMUNE_DISORDERS,
       HEALTH_ANYTHING_ELSE,
       RECREATIONAL_DRUG_IN_LAST_3_MONTHS,
       RECDRUG_PREVHEAVY,
       MRI_CLAUSTROPHOBIC,
       MRI_DIFFICULTY_LYING_1_HOUR,
       MRI_NONREMOVABLE_METAL,
       MRI_METAL_FROM_OPERATIONS,
       MRI_TATTOOS_NICOTINE_PATCHES,
       MRI_WORKED_WITH_METAL,
       MRI_PREVIOUS_BRAIN_SCAN,
       OTHER_RELEVANT_THINGS,
       WILLING_TO_PARTICIPATE_IN_FURTHER_STUDIES,
    }));
}


QStringList DeakinS1HealthReview::summary() const
{
    return QStringList{TextConst::noSummarySeeFacsimile()};
}


QStringList DeakinS1HealthReview::detail() const
{
    return completenessInfo() + summary();
}


OpenableWidget* DeakinS1HealthReview::editor(const bool read_only)
{
    const NameValueOptions yn_options = CommonOptions::yesNoBoolean();
    const NameValueOptions handedness_options = NameValueOptions{
        {"Left hand", "L"},
        {"Right hand", "R"},
    };
    const NameValueOptions education_options = NameValueOptions{
        {"None", "none"},
        {"CSE", "cse"},
        {"GCSE", "gcse"},
        {"O-Level", "olevel"},
        {"A-Level", "alevel"},
        {"Vocational qualification, NVQ — full time", "nvq_fulltime"},
        {"Vocational qualification, NVQ — part time", "nvq_parttime"},
        {"Degree qualification — diploma", "degree_diploma"},
        {"Degree qualification — bachelor’s", "degree_bachelor"},
        {"Degree qualification — other", "degree_other"},
        {"Postgraduate qualification — master’s", "postgrad_masters"},
        {"Postgraduate qualification — PhD", "postgrad_phd"},
    };

    auto text = [](const QString& text) -> QuElement* {
        return new QuText(text);
    };
    auto boldtext = [](const QString& text) -> QuElement* {
        return (new QuText(text))->setBold();
    };
    auto mcq = [this](const QString& fieldname,
                      const NameValueOptions& options) -> QuElement* {
        return new QuMcq(fieldRef(fieldname), options);
    };
    auto mcqhoriz = [this](const QString& fieldname,
                           const NameValueOptions& options) -> QuElement* {
        return (new QuMcq(fieldRef(fieldname), options))->setHorizontal(true);
    };
    auto qf = [this](const QString& fieldname,
                     const QString& question) -> QuestionWithOneField {
        return QuestionWithOneField(fieldRef(fieldname), question);
    };
    auto q2f = [this](const QString& fieldname1,
                      const QString& fieldname2,
                      const QString& question) -> QuestionWithTwoFields {
        return QuestionWithTwoFields(question, fieldRef(fieldname1),
                                     fieldRef(fieldname2));
    };
    auto yn = [this, &yn_options](const QString& fieldname) -> QuElement* {
        return (new QuMcq(fieldRef(fieldname), yn_options))
                ->setHorizontal(true);
    };
    auto lineedit = [this](const QString& fieldname) -> QuElement* {
        return new QuLineEdit(fieldRef(fieldname));
    };
    auto multiline = [this](const QString& fieldname,
                            bool mandatory = true) -> QuElement* {
        return new QuTextEdit(fieldRef(fieldname, mandatory));
    };
    auto intedit = [this](const QString& fieldname) -> QuElement* {
        return (new QuLineEditInteger(fieldRef(fieldname), 0, 1000))
                ->setHint("");
    };
    auto watch = [this](const QString& fieldname) -> void {
        connect(fieldRef(fieldname).data(), &FieldRef::valueChanged,
                this, &DeakinS1HealthReview::updateMandatory);
    };

    QVector<QuPagePtr> pages;

    pages.append(QuPagePtr((new QuPage{
        boldtext("Please enter your ethnicity:"),
        mcq(ETHNICITY, GmcPq::ethnicityOptions(m_app)),
        text(m_app.xstring(GmcPq::GMCPQ_TABLENAME, "ethnicity_other_s")),
        lineedit(ETHNICITY_OTHER_DETAILS),
    })->setTitle("Ethnicity")));
    watch(ETHNICITY);

    pages.append(QuPagePtr((new QuPage{
        boldtext("I prefer to use my:"),
        mcqhoriz(HANDEDNESS, handedness_options),
    })->setTitle("Handedness")));

    pages.append(QuPagePtr((new QuPage{
        boldtext("Please enter your highest level of education, or nearest "
                 "equivalent:"),
        mcqhoriz(EDUCATION, education_options),
    })->setTitle("Education")));

    QuMultipleResponse* mr_allergies = new QuMultipleResponse{
        qf(ALLERGY_ASTHMA, "asthma"),
        qf(ALLERGY_POLLEN_DUST, "pollen/dust"),
        qf(ALLERGY_DERMATITIS, "dermatitis"),
        qf(ALLERGY_FOOD, "food allergy"),
        qf(ALLERGY_DANDER, "animal dander"),
        qf(ALLERGY_OTHER, "other"),
    };
    pages.append(QuPagePtr((new QuPage{
        boldtext("Do you have any allergies?"),
        yn(ALLERGIES),
        boldtext(STR_DETAILS_IF_YES),
        mr_allergies,
        text(STR_DETAILS),
        multiline(ALLERGY_DETAILS),
    })->setTitle("Allergies")));
    watch(ALLERGIES);
    watch(ALLERGY_OTHER);
    connect(this, &DeakinS1HealthReview::setAllergyMinimum,
            mr_allergies, &QuMultipleResponse::setMinimumAnswers);

    pages.append(QuPagePtr((new QuPage{
        boldtext("Have you had any vaccinations or inoculations in the last 3 "
                 "months?"),
        yn(VACCINATIONS_LAST3MONTHS),
        boldtext(STR_DETAILS_IF_YES),
        text(STR_DETAILS),
        multiline(VACCINATION_DETAILS),
    })->setTitle("Recent vaccinations")));
    connect(fieldRef(VACCINATIONS_LAST3MONTHS).data(), &FieldRef::valueChanged,
            this, &DeakinS1HealthReview::updateMandatory);

    QuMultipleResponse* mr_recent_infection = new QuMultipleResponse{
        qf(INFECTION_RECENT_RESPIRATORY, INFECTIONLIST[0]),
        qf(INFECTION_RECENT_GASTROENTERITIS, INFECTIONLIST[1]),
        qf(INFECTION_RECENT_URINARY, INFECTIONLIST[2]),
        qf(INFECTION_RECENT_SEXUAL, INFECTIONLIST[3]),
        qf(INFECTION_RECENT_HEPATITIS, INFECTIONLIST[4]),
        qf(INFECTION_RECENT_OTHER, INFECTIONLIST[5]),
    };
    pages.append(QuPagePtr((new QuPage{
        boldtext("Have you had any infectious diseases in the last 3 months?"),
        yn(INFECTIONS_LAST3MONTHS),
        boldtext(STR_DETAILS_IF_YES),
        mr_recent_infection,
        text(STR_DETAILS),
        multiline(INFECTION_RECENT_DETAILS),
    })->setTitle("Recent infections")));
    watch(INFECTIONS_LAST3MONTHS);
    watch(INFECTION_RECENT_OTHER);
    connect(this, &DeakinS1HealthReview::setRecentInfectionsMinimum,
            mr_recent_infection, &QuMultipleResponse::setMinimumAnswers);

    QuMultipleResponse* mr_chronic_infection = new QuMultipleResponse{
        qf(INFECTION_CHRONIC_RESPIRATORY, INFECTIONLIST[0]),
        qf(INFECTION_CHRONIC_GASTROENTERITIS, INFECTIONLIST[1]),
        qf(INFECTION_CHRONIC_URINARY, INFECTIONLIST[2]),
        qf(INFECTION_CHRONIC_SEXUAL, INFECTIONLIST[3]),
        qf(INFECTION_CHRONIC_HEPATITIS, INFECTIONLIST[4]),
        qf(INFECTION_CHRONIC_OTHER, INFECTIONLIST[5]),
    };
    pages.append(QuPagePtr((new QuPage{
        boldtext("Are you currently experiencing or have you ever experienced "
                 "any chronic infections?"),
        yn(INFECTIONS_CHRONIC),
        boldtext(STR_DETAILS_IF_YES),
        mr_chronic_infection,
        text(STR_DETAILS),
        multiline(INFECTION_CHRONIC_DETAILS),
    })->setTitle("Chronic infections")));
    watch(INFECTIONS_CHRONIC);
    watch(INFECTION_CHRONIC_OTHER);
    connect(this, &DeakinS1HealthReview::setChronicInfectionsMinimum,
            mr_chronic_infection, &QuMultipleResponse::setMinimumAnswers);

    QuMultipleResponse* mr_immune = new QuMultipleResponse{
        qf(IMMUNITY_MS, "multiple sclerosis"),
        qf(IMMUNITY_SLE, "lupus"),
        qf(IMMUNITY_ARTHRITIS, "arthritis"),
        qf(IMMUNITY_HIV, "HIV/AIDS"),
        qf(IMMUNITY_GRAVES, "Graves’ disease"),
        qf(IMMUNITY_DIABETES, "diabetes"),
        qf(IMMUNITY_OTHER, "other"),
    };
    pages.append(QuPagePtr((new QuPage{
        boldtext("Do you have any immune disorders?"),
        yn(IMMUNE_DISORDERS),
        boldtext(STR_DETAILS_IF_YES),
        mr_immune,
        text(STR_DETAILS),
        multiline(IMMUNITY_DETAILS),
    })->setTitle("Immune disorders")));
    watch(IMMUNE_DISORDERS);
    watch(IMMUNITY_OTHER);
    connect(this, &DeakinS1HealthReview::setImmuneMinimum,
            mr_immune, &QuMultipleResponse::setMinimumAnswers);

    QuMultipleResponse* mr_fh_immune = new QuMultipleResponse{
        qf(FAMILYHISTORY_MS, "multiple sclerosis"),
        qf(FAMILYHISTORY_SLE, "lupus"),
        qf(FAMILYHISTORY_ARTHRITIS, "arthritis"),
        qf(FAMILYHISTORY_GRAVES, "Graves’ disease"),
        qf(FAMILYHISTORY_PSYCHOSIS_SZ, "psychosis/schizophrenia"),
        qf(FAMILYHISTORY_BIPOLAR, "mania/bipolar affective disorder"),
    };
    pages.append(QuPagePtr((new QuPage{
        boldtext("Does anyone in your family have any of the disorders listed "
                 "below?"),
        yn(FAMILY_HISTORY),
        boldtext(STR_DETAILS_IF_YES),
        mr_fh_immune,
        text(STR_DETAILS),
        multiline(FAMILYHISTORY_DETAILS),
    })->setTitle("Family history")));
    watch(FAMILY_HISTORY);
    connect(this, &DeakinS1HealthReview::setFHImmuneMinimum,
            mr_fh_immune, &QuMultipleResponse::setMinimumAnswers);

    pages.append(QuPagePtr((new QuPage{
        boldtext("Is there any other information about your general health "
                 "that we should know?"),
        yn(HEALTH_ANYTHING_ELSE),
        boldtext(STR_DETAILS_IF_YES),
        multiline(HEALTH_ANYTHING_ELSE_DETAILS),
    })->setTitle("Other aspects of health")));

    pages.append(QuPagePtr((new QuPage{
        boldtext("If you are taking prescribed medication please list below:"),
        multiline(DRUG_HISTORY, false),
        boldtext("If you are taking antipsychotic medication, when did you "
                 "first take a medication of this kind?"),
        multiline(FIRST_ANTIPSYCHOTIC_MEDICATION, false),
    })->setTitle("Medication")));

    pages.append(QuPagePtr((new QuPage{
        boldtext("Please answer the following questions about any history you "
                 "may have with drug taking. It is very important that you "
                 "are honest, because this history may affect your blood "
                 "sample. Previous drug taking will not necessarily exclude "
                 "you, and all information will be kept completely "
                 "confidential."),
        boldtext("Have you taken any recreational drugs in the last 3 months? "
                 "(Recreational drugs include drugs used only occasionally "
                 "without being dependent on them.)"),
        yn(RECREATIONAL_DRUG_IN_LAST_3_MONTHS),
        boldtext("Have you ever had a period of very heavy use of any of the "
                 "drugs listed below?"),
        text(DRUGLIST.join(", ")),
        yn(RECDRUG_PREVHEAVY),
        boldtext("If you answered YES to either question, please give details "
                 "(A–E below)."),
        boldtext("(A) Please use the grid below to specify which drugs you "
                 "used in the past 3 months, and how often."),
        boldtext("(B) If you have ever had a period of very heavy use of any "
                 "of these drugs, please tick its “Previous heavy use?” box."),
        (new QuMcqGridSingleBoolean(
            QVector<QuestionWithTwoFields>{
                q2f(RECDRUG_TOBACCO_FREQUENCY, RECDRUG_TOBACCO_PREVHEAVY, DRUGLIST[0]),
                q2f(RECDRUG_CANNABIS_FREQUENCY, RECDRUG_CANNABIS_PREVHEAVY, DRUGLIST[1]),
                q2f(RECDRUG_ALCOHOL_FREQUENCY, RECDRUG_ALCOHOL_PREVHEAVY, DRUGLIST[2]),

                q2f(RECDRUG_MDMA_FREQUENCY, RECDRUG_MDMA_PREVHEAVY, DRUGLIST[3]),
                q2f(RECDRUG_COCAINE_FREQUENCY, RECDRUG_COCAINE_PREVHEAVY, DRUGLIST[4]),
                q2f(RECDRUG_CRACK_FREQUENCY, RECDRUG_CRACK_PREVHEAVY, DRUGLIST[5]),
                q2f(RECDRUG_AMPHETAMINES_FREQUENCY, RECDRUG_AMPHETAMINES_PREVHEAVY, DRUGLIST[6]),

                q2f(RECDRUG_HEROIN_FREQUENCY, RECDRUG_HEROIN_PREVHEAVY, DRUGLIST[7]),
                q2f(RECDRUG_METHADONE_FREQUENCY, RECDRUG_METHADONE_PREVHEAVY, DRUGLIST[8]),
                q2f(RECDRUG_BENZODIAZEPINES_FREQUENCY, RECDRUG_BENZODIAZEPINES_PREVHEAVY, DRUGLIST[9]),

                q2f(RECDRUG_KETAMINE_FREQUENCY, RECDRUG_KETAMINE_PREVHEAVY, DRUGLIST[10]),
                q2f(RECDRUG_LEGALHIGHS_FREQUENCY, RECDRUG_LEGALHIGHS_PREVHEAVY, DRUGLIST[11]),
                q2f(RECDRUG_INHALANTS_FREQUENCY, RECDRUG_INHALANTS_PREVHEAVY, DRUGLIST[12]),
                q2f(RECDRUG_HALLUCINOGENS_FREQUENCY, RECDRUG_HALLUCINOGENS_PREVHEAVY, DRUGLIST[13]),
            },
            NameValueOptions{
                {"Did not use", 0},
                {"Occasionally", 1},
                {"Monthly", 2},
                {"Weekly", 3},
                {"Daily", 4},
            },
            "Previous heavy use?"  // boolean label
        ))->setSubtitles(QVector<McqGridSubtitle>{
            McqGridSubtitle(3, ""),
            McqGridSubtitle(7, ""),
            McqGridSubtitle(10, ""),
        }),
        boldtext("(C) Please give any further details of your recreational "
                 "drug use in the previous 3 months:"),
        multiline(RECDRUG_DETAILS),
        boldtext("(D) If you have used tobacco, cannabis, or alcohol in the "
                 "last 3 months, please give the quantities:"),
        text("Tobacco – cigarettes per week:"),
        intedit(RECDRUG_TOBACCO_CIGSPERWEEK),
        text("Cannabis – joints per week:"),
        intedit(RECDRUG_CANNABIS_JOINTSPERWEEK),
        text("Alcohol – units per week:"),
        intedit(RECDRUG_ALCOHOL_UNITSPERWEEK),
        boldtext("(E) If you have had a period of very heavy drug use, please "
                 "give details about when this was and how long you used the "
                 "drug heavily:"),
        multiline(RECDRUG_PREVHEAVY_DETAILS),
    })->setTitle("Recreational drug use")));
    watch(RECREATIONAL_DRUG_IN_LAST_3_MONTHS);
    watch(RECDRUG_PREVHEAVY);

    pages.append(QuPagePtr((new QuPage{
        new QuMcqGrid(QVector<QuestionWithOneField>{
            qf(MRI_CLAUSTROPHOBIC,
               "Are you claustrophobic, or have difficulties in small spaces "
               "(e.g. lifts, confined spaces)?"),
            qf(MRI_DIFFICULTY_LYING_1_HOUR,
               "Would you have any difficulties with lying down for 1 hour "
               "(e.g. problems with your back, neck, bladder, etc.)?"),
            qf(MRI_NONREMOVABLE_METAL,
               "Is there any metal in your body which is not removable (e.g. "
               "piercings, splinters, etc.)?"),
            qf(MRI_METAL_FROM_OPERATIONS,
               "Have you ever had any operations where metal has been left in "
               "your body?"),
            qf(MRI_TATTOOS_NICOTINE_PATCHES,
               "Do you have any tattoos or nicotine patches?"),
            qf(MRI_WORKED_WITH_METAL,
               "Have you ever worked with metal (e.g. as a machinist, "
               "metalworker, etc.)?"),
            qf(MRI_PREVIOUS_BRAIN_SCAN,
               "Have you ever had any form of brain scan before? If so, "
               "please give details below."),
            qf(OTHER_RELEVANT_THINGS,
               "Are there any points you feel may be relevant to your "
               "participation in the study? If so, please give details "
               "below."),
        }, yn_options),
        text("Details of previous brain scans, if applicable:"),
        multiline(MRI_PREVIOUS_BRAIN_SCAN_DETAILS, false),
        text("Any other points you feel may be relevant to your "
             "participation, if applicable:"),
        multiline(OTHER_RELEVANT_THINGS_DETAILS, false),
        text("Finally:"),
        new QuMcqGrid(QVector<QuestionWithOneField>{
            qf(WILLING_TO_PARTICIPATE_IN_FURTHER_STUDIES,
               "Would you be willing to participate in further studies run by "
               "our department?"),
        }, yn_options),
    })->setTitle("Questions related to MRI scanning")));

    pages.append(QuPagePtr((new QuPage{
        boldtext(TextConst::thankYou()),
    })->setTitle(TextConst::finished())));

    updateMandatory();

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Signal handlers
// ============================================================================

void DeakinS1HealthReview::updateMandatory()
{
    // This could be more efficient with lots of signal handlers, but...

    fieldRef(ETHNICITY_OTHER_DETAILS)->setMandatory(
                GmcPq::ethnicityOther(valueInt(ETHNICITY)));

    emit setAllergyMinimum(valueInt(ALLERGIES));
    fieldRef(ALLERGY_DETAILS)->setMandatory(valueBool(ALLERGY_OTHER));

    fieldRef(VACCINATION_DETAILS)->setMandatory(
                valueBool(VACCINATIONS_LAST3MONTHS));

    emit setRecentInfectionsMinimum(valueInt(INFECTIONS_LAST3MONTHS));
    fieldRef(INFECTION_RECENT_DETAILS)->setMandatory(
                valueBool(INFECTION_RECENT_OTHER));

    emit setChronicInfectionsMinimum(valueInt(INFECTIONS_CHRONIC));
    fieldRef(INFECTION_CHRONIC_DETAILS)->setMandatory(
                valueBool(INFECTION_CHRONIC_OTHER));

    emit setImmuneMinimum(valueInt(IMMUNE_DISORDERS));
    fieldRef(IMMUNITY_DETAILS)->setMandatory(valueBool(IMMUNITY_OTHER));

    emit setFHImmuneMinimum(valueInt(FAMILY_HISTORY));
    fieldRef(FAMILYHISTORY_DETAILS)->setMandatory(valueBool(FAMILY_HISTORY));

    fieldRef(HEALTH_ANYTHING_ELSE_DETAILS)->setMandatory(
                valueBool(HEALTH_ANYTHING_ELSE));

    fieldRef(HEALTH_ANYTHING_ELSE_DETAILS)->setMandatory(
                valueBool(HEALTH_ANYTHING_ELSE));

    const bool recent_drugs = valueBool(RECREATIONAL_DRUG_IN_LAST_3_MONTHS);
    const bool heavy_drugs = valueBool(RECDRUG_PREVHEAVY);
    const bool drugs = recent_drugs || heavy_drugs;

    fieldRef(RECDRUG_TOBACCO_FREQUENCY)->setMandatory(drugs);
    fieldRef(RECDRUG_TOBACCO_PREVHEAVY)->setMandatory(drugs);
    fieldRef(RECDRUG_CANNABIS_FREQUENCY)->setMandatory(drugs);
    fieldRef(RECDRUG_CANNABIS_PREVHEAVY)->setMandatory(drugs);
    fieldRef(RECDRUG_ALCOHOL_FREQUENCY)->setMandatory(drugs);
    fieldRef(RECDRUG_ALCOHOL_PREVHEAVY)->setMandatory(drugs);

    fieldRef(RECDRUG_MDMA_FREQUENCY)->setMandatory(drugs);
    fieldRef(RECDRUG_MDMA_PREVHEAVY)->setMandatory(drugs);
    fieldRef(RECDRUG_COCAINE_FREQUENCY)->setMandatory(drugs);
    fieldRef(RECDRUG_COCAINE_PREVHEAVY)->setMandatory(drugs);
    fieldRef(RECDRUG_CRACK_FREQUENCY)->setMandatory(drugs);
    fieldRef(RECDRUG_CRACK_PREVHEAVY)->setMandatory(drugs);
    fieldRef(RECDRUG_AMPHETAMINES_FREQUENCY)->setMandatory(drugs);
    fieldRef(RECDRUG_AMPHETAMINES_PREVHEAVY)->setMandatory(drugs);

    fieldRef(RECDRUG_HEROIN_FREQUENCY)->setMandatory(drugs);
    fieldRef(RECDRUG_HEROIN_PREVHEAVY)->setMandatory(drugs);
    fieldRef(RECDRUG_METHADONE_FREQUENCY)->setMandatory(drugs);
    fieldRef(RECDRUG_METHADONE_PREVHEAVY)->setMandatory(drugs);
    fieldRef(RECDRUG_BENZODIAZEPINES_FREQUENCY)->setMandatory(drugs);
    fieldRef(RECDRUG_BENZODIAZEPINES_PREVHEAVY)->setMandatory(drugs);

    fieldRef(RECDRUG_KETAMINE_FREQUENCY)->setMandatory(drugs);
    fieldRef(RECDRUG_KETAMINE_PREVHEAVY)->setMandatory(drugs);
    fieldRef(RECDRUG_LEGALHIGHS_FREQUENCY)->setMandatory(drugs);
    fieldRef(RECDRUG_LEGALHIGHS_PREVHEAVY)->setMandatory(drugs);
    fieldRef(RECDRUG_INHALANTS_FREQUENCY)->setMandatory(drugs);
    fieldRef(RECDRUG_INHALANTS_PREVHEAVY)->setMandatory(drugs);
    fieldRef(RECDRUG_HALLUCINOGENS_FREQUENCY)->setMandatory(drugs);
    fieldRef(RECDRUG_HALLUCINOGENS_PREVHEAVY)->setMandatory(drugs);

    fieldRef(RECDRUG_DETAILS)->setMandatory(recent_drugs);

    fieldRef(RECDRUG_TOBACCO_CIGSPERWEEK)->setMandatory(recent_drugs);
    fieldRef(RECDRUG_TOBACCO_CIGSPERWEEK)->setMandatory(recent_drugs);
    fieldRef(RECDRUG_TOBACCO_CIGSPERWEEK)->setMandatory(recent_drugs);

    fieldRef(RECDRUG_PREVHEAVY_DETAILS)->setMandatory(heavy_drugs);
}
