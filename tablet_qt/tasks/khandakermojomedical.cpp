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

#include "khandakermojomedical.h"
#include "common/cssconst.h"
#include "common/textconst.h"
#include "lib/convert.h"
#include "lib/uifunc.h"
#include "lib/version.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/questionwithonefield.h"
#include "questionnairelib/qudatetime.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/qulineeditdouble.h"
#include "questionnairelib/qulineeditinteger.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qupage.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"


const QString KhandakerMojoMedical::KHANDAKERMOJOMEDICAL_TABLENAME(
    "khandaker_mojo_medical");

const QString Q_XML_PREFIX = "q_";
const QString Q_SUMMARY_XML_SUFFIX = "_s";

// Section 1: General Information
const QString FN_DIAGNOSIS("diagnosis");
const QString FN_DIAGNOSIS_DATE("diagnosis_date");
const QString FN_DIAGNOSIS_DATE_APPROXIMATE("diagnosis_date_approximate");
const QString FN_HAS_FIBROMYALGIA("has_fibromyalgia");
const QString FN_IS_PREGNANT("is_pregnant");
const QString FN_HAS_INFECTION_PAST_MONTH("has_infection_past_month");
const QString FN_HAD_INFECTION_TWO_MONTHS_PRECEDING("had_infection_two_months_preceding");
const QString FN_HAS_ALCOHOL_SUBSTANCE_DEPENDENCE("has_alcohol_substance_dependence");
const QString FN_SMOKING_STATUS("smoking_status");
const QString FN_ALCOHOL_UNITS_PER_WEEK("alcohol_units_per_week");

// Section 2: Medical History
const QString FN_DEPRESSION("depression");
const QString FN_BIPOLAR_DISORDER("bipolar_disorder");
const QString FN_SCHIZOPHRENIA("schizophrenia");
const QString FN_AUTISM("autism");
const QString FN_PTSD("ptsd");
const QString FN_ANXIETY("anxiety");
const QString FN_PERSONALITY_DISORDER("personality_disorder");
const QString FN_INTELLECTUAL_DISABILITY("intellectual_disability");
const QString FN_OTHER_MENTAL_ILLNESS("other_mental_illness");
const QString FN_OTHER_MENTAL_ILLNESS_DETAILS("other_mental_illness_details");
const QString FN_HOSPITALISED_IN_LAST_YEAR("hospitalised_in_last_year");
const QString FN_HOSPITALISATION_DETAILS("hospitalisation_details");

// Section 3: Family history
const QString FN_FAMILY_DEPRESSION("family_depression");
const QString FN_FAMILY_BIPOLAR_DISORDER("family_bipolar_disorder");
const QString FN_FAMILY_SCHIZOPHRENIA("family_schizophrenia");
const QString FN_FAMILY_AUTISM("family_autism");
const QString FN_FAMILY_PTSD("family_ptsd");
const QString FN_FAMILY_ANXIETY("family_anxiety");
const QString FN_FAMILY_PERSONALITY_DISORDER("family_personality_disorder");
const QString FN_FAMILY_INTELLECTUAL_DISABILITY("family_intellectual_disability");
const QString FN_FAMILY_OTHER_MENTAL_ILLNESS("family_other_mental_illness");
const QString FN_FAMILY_OTHER_MENTAL_ILLNESS_DETAILS("family_other_mental_illness_details");

const QStringList MANDATORY_FIELDNAMES{
    FN_DIAGNOSIS,
    FN_DIAGNOSIS_DATE,
    FN_HAS_FIBROMYALGIA,
    FN_IS_PREGNANT,
    FN_HAS_INFECTION_PAST_MONTH,
    FN_HAD_INFECTION_TWO_MONTHS_PRECEDING,
    FN_HAS_ALCOHOL_SUBSTANCE_DEPENDENCE,
    FN_SMOKING_STATUS,
    FN_ALCOHOL_UNITS_PER_WEEK,

    FN_DEPRESSION,
    FN_BIPOLAR_DISORDER,
    FN_SCHIZOPHRENIA,
    FN_AUTISM,
    FN_PTSD,
    FN_ANXIETY,
    FN_PERSONALITY_DISORDER,
    FN_INTELLECTUAL_DISABILITY,
    FN_OTHER_MENTAL_ILLNESS,
    FN_HOSPITALISED_IN_LAST_YEAR,

    FN_FAMILY_DEPRESSION,
    FN_FAMILY_BIPOLAR_DISORDER,
    FN_FAMILY_SCHIZOPHRENIA,
    FN_FAMILY_AUTISM,
    FN_FAMILY_PTSD,
    FN_FAMILY_ANXIETY,
    FN_FAMILY_PERSONALITY_DISORDER,
    FN_FAMILY_INTELLECTUAL_DISABILITY,
    FN_FAMILY_OTHER_MENTAL_ILLNESS,
};

const QStringList SUMMARY_FIELDNAMES{
    FN_HAS_FIBROMYALGIA,
    FN_IS_PREGNANT,
    FN_HAS_INFECTION_PAST_MONTH,
    FN_HAD_INFECTION_TWO_MONTHS_PRECEDING,
    FN_HAS_ALCOHOL_SUBSTANCE_DEPENDENCE,
};

// Maps "Other Y/N?" fields to "Other: please give details" fields
const QMap<QString, QString> DETAILS_FIELDS{
    {FN_OTHER_MENTAL_ILLNESS, FN_OTHER_MENTAL_ILLNESS_DETAILS},
    {FN_HOSPITALISED_IN_LAST_YEAR, FN_HOSPITALISATION_DETAILS},
    {FN_FAMILY_OTHER_MENTAL_ILLNESS, FN_FAMILY_OTHER_MENTAL_ILLNESS_DETAILS},
};

const int N_POSSIBLE_DIAGNOSES = 3;
const int N_SMOKING_STATUS_VALUES = 3;


void initializeKhandakerMojoMedical(TaskFactory& factory)
{
    static TaskRegistrar<KhandakerMojoMedical> registered(factory);
}


KhandakerMojoMedical::KhandakerMojoMedical(
        CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, KHANDAKERMOJOMEDICAL_TABLENAME,
         false, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr),
    m_fr_diagnosis_date(nullptr),
    m_fr_diagnosis_years(nullptr)
{
    // Section 1: General Information
    addField(FN_DIAGNOSIS, QVariant::Int);
    addField(FN_DIAGNOSIS_DATE, QVariant::Date);
    addField(FN_DIAGNOSIS_DATE_APPROXIMATE, QVariant::Bool);
    addField(FN_HAS_FIBROMYALGIA, QVariant::Bool);
    addField(FN_IS_PREGNANT, QVariant::Bool);
    addField(FN_HAS_INFECTION_PAST_MONTH, QVariant::Bool);
    addField(FN_HAD_INFECTION_TWO_MONTHS_PRECEDING, QVariant::Bool);
    addField(FN_HAS_ALCOHOL_SUBSTANCE_DEPENDENCE, QVariant::Bool);
    addField(FN_SMOKING_STATUS, QVariant::Int);
    addField(FN_ALCOHOL_UNITS_PER_WEEK, QVariant::Double);

    // Section 2: Medical History
    addField(FN_DEPRESSION, QVariant::Bool);
    addField(FN_BIPOLAR_DISORDER, QVariant::Bool);
    addField(FN_SCHIZOPHRENIA, QVariant::Bool);
    addField(FN_AUTISM, QVariant::Bool);
    addField(FN_PTSD, QVariant::Bool);
    addField(FN_ANXIETY, QVariant::Bool);
    addField(FN_PERSONALITY_DISORDER, QVariant::Bool);
    addField(FN_INTELLECTUAL_DISABILITY, QVariant::Bool);
    addField(FN_OTHER_MENTAL_ILLNESS, QVariant::Bool);
    addField(FN_OTHER_MENTAL_ILLNESS_DETAILS, QVariant::String);
    addField(FN_HOSPITALISED_IN_LAST_YEAR, QVariant::Bool);
    addField(FN_HOSPITALISATION_DETAILS, QVariant::String);

    // Section 3: Family history
    addField(FN_FAMILY_DEPRESSION, QVariant::Bool);
    addField(FN_FAMILY_BIPOLAR_DISORDER, QVariant::Bool);
    addField(FN_FAMILY_SCHIZOPHRENIA, QVariant::Bool);
    addField(FN_FAMILY_AUTISM, QVariant::Bool);
    addField(FN_FAMILY_PTSD, QVariant::Bool);
    addField(FN_FAMILY_ANXIETY, QVariant::Bool);
    addField(FN_FAMILY_PERSONALITY_DISORDER, QVariant::Bool);
    addField(FN_FAMILY_INTELLECTUAL_DISABILITY, QVariant::Bool);
    addField(FN_FAMILY_OTHER_MENTAL_ILLNESS, QVariant::Bool);
    addField(FN_FAMILY_OTHER_MENTAL_ILLNESS_DETAILS, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}



// ============================================================================
// Class info
// ============================================================================

QString KhandakerMojoMedical::shortname() const
{
    return "Khandaker_MOJO_Medical";
}


QString KhandakerMojoMedical::longname() const
{
    return tr("Khandaker GM — MOJO — Medical questionnaire");
}


QString KhandakerMojoMedical::description() const
{
    return tr("Medical questionnaire for MOJO study.");
}


// ============================================================================
// Instance info
// ============================================================================

bool KhandakerMojoMedical::isComplete() const
{
    for (const QString& fieldname: MANDATORY_FIELDNAMES) {
        if (valueIsNull(fieldname)) {
            return false;
        }

        if (DETAILS_FIELDS.contains(fieldname)) {
            if (valueBool(fieldname) &&
                    valueIsNullOrEmpty(DETAILS_FIELDS.value(fieldname))) {
                return false;
            }
        }
    }

    return true;
}


QStringList KhandakerMojoMedical::summary() const
{
    QStringList lines;
    QStringList medical_history;

    const QString fmt = QString("%1 <b>%2</b>");

    for (const QString& fieldname : SUMMARY_FIELDNAMES) {
        if (valueBool(fieldname)) {
            medical_history.append(
                xstring(Q_XML_PREFIX + fieldname + Q_SUMMARY_XML_SUFFIX)
            );
        }
    }

    if (medical_history.size() > 0) {
        lines.append(fmt.arg(xstring("answered_yes_to"),
                             medical_history.join(", ")));
    }

    lines.append(fmt.arg(xstring("q_diagnosis"), getDiagnosis()));

    return lines;
}


QString KhandakerMojoMedical::getDiagnosis() const
{
    const NameValueOptions options = getOptions(FN_DIAGNOSIS, N_POSSIBLE_DIAGNOSES);
    const QVariant diagnosis = value(FN_DIAGNOSIS);
    return options.nameFromValue(diagnosis, "?");
}


QStringList KhandakerMojoMedical::detail() const
{
    QStringList lines;

    for (const QString& fieldname : MANDATORY_FIELDNAMES) {
        lines.append(xstring(Q_XML_PREFIX + fieldname));
        lines.append(QString("<b>%1</b>").arg(prettyValue(fieldname)));

        if (DETAILS_FIELDS.contains(fieldname) && valueBool(fieldname)) {
            const QString details_fieldname = DETAILS_FIELDS.value(fieldname);
            lines.append(xstring(Q_XML_PREFIX + details_fieldname));
            lines.append(QString("<b>%1</b>").arg(
                             prettyValue(details_fieldname)));

        }
    }

    return completenessInfo() + lines;
}


OpenableWidget* KhandakerMojoMedical::editor(const bool read_only)
{
    QuPagePtr page(new QuPage);

    auto heading = [this, &page](const QString& xstringname) -> void {
        page->addElement((new QuText(xstring(xstringname)))->setBold(true));
    };

    auto textQuestion = [this, &page](const QString &fieldname) -> void {
        auto text = new QuText(xstring(Q_XML_PREFIX + fieldname));
        auto text_edit = new QuTextEdit(fieldRef(fieldname));
        auto spacer = new QuSpacer(QSize(uiconst::BIGSPACE,
                                         uiconst::BIGSPACE));

        text->addTag(fieldname);
        text_edit->addTag(fieldname);
        spacer->addTag(fieldname);

        page->addElement(text);
        page->addElement(text_edit);
        page->addElement(spacer);
    };

    auto multiChoiceQuestion = [this, &page](const QString &fieldname,
                                             int num_options) -> void {
        page->addElement(new QuText(xstring(Q_XML_PREFIX + fieldname)));

        FieldRefPtr fieldref = fieldRef(fieldname);
        QuMcq* mcq = new QuMcq(fieldref, getOptions(fieldname, num_options));
        mcq->setHorizontal(true);
        page->addElement(mcq);
        page->addElement(new QuSpacer(QSize(uiconst::BIGSPACE,
                                            uiconst::BIGSPACE)));
    };

    auto yesNoQuestion = [this, &page](const QString &fieldname) -> void {
        page->addElement(new QuText(xstring(Q_XML_PREFIX + fieldname)));

        FieldRefPtr fieldref = fieldRef(fieldname);
        QuMcq* mcq = new QuMcq(fieldref, CommonOptions::noYesBoolean());
        mcq->setHorizontal(true);
        page->addElement(mcq);
        page->addElement(new QuSpacer(QSize(uiconst::BIGSPACE,
                                            uiconst::BIGSPACE)));

    };

    auto doubleQuestion = [this, &page](const QString &fieldname,
                                        const double minimum,
                                        const double maximum,
                                        const QString &hint) -> void {
        page->addElement(new QuText(xstring(Q_XML_PREFIX + fieldname)));

        auto line_edit_double = new QuLineEditDouble(
            fieldRef(fieldname), minimum, maximum
        );
        line_edit_double->setHint(hint);

        page->addElement(line_edit_double);
        page->addElement(new QuSpacer(QSize(uiconst::BIGSPACE,
                                            uiconst::BIGSPACE)));
    };

    auto yesNoGrid = [this, &page](const QStringList fieldnames) -> void {
        QVector<QuestionWithOneField> field_pairs;

        for (const QString& fieldname : fieldnames) {
            const QString description = xstring(Q_XML_PREFIX + fieldname);
            field_pairs.append(QuestionWithOneField(description,
                                                    fieldRef(fieldname)));
        }

        auto grid = new QuMcqGrid(
            field_pairs, CommonOptions::noYesBoolean()
        );

        grid->setWidth(8, {1, 1});

        grid->setSubtitles({
            {5, ""},
            {10, ""},
        });

        page->addElement(grid);
    };

    page->setTitle(description());
    page->addElement(new QuHeading(xstring("title")));
    heading("general_information_title");

    multiChoiceQuestion(FN_DIAGNOSIS, N_POSSIBLE_DIAGNOSES);

    FieldRef::GetterFunction get_date = std::bind(
        &KhandakerMojoMedical::getDiagnosisDate, this);
    FieldRef::GetterFunction get_years = std::bind(
        &KhandakerMojoMedical::getDurationOfIllness, this);
    FieldRef::SetterFunction set_date = std::bind(
        &KhandakerMojoMedical::setDiagnosisDate, this, std::placeholders::_1);
    FieldRef::SetterFunction set_years = std::bind(
        &KhandakerMojoMedical::setDurationOfIllness, this, std::placeholders::_1);

    m_fr_diagnosis_date = FieldRefPtr(new FieldRef(get_date, set_date, true));
    m_fr_diagnosis_years = FieldRefPtr(new FieldRef(get_years, set_years, true));

    // We don't store duration of illness on the server
    page->addElement(new QuText(xstring("duration_of_illness")));
    page->addElement(new QuLineEditInteger(m_fr_diagnosis_years, 0, 150));
    page->addElement(new QuText(xstring("or")));
    page->addElement(new QuText(xstring(Q_XML_PREFIX + FN_DIAGNOSIS_DATE)));
    auto date_time = new QuDateTime(m_fr_diagnosis_date);
    date_time->setOfferNowButton(true);
    date_time->setMode(QuDateTime::Mode::DefaultDate);
    date_time->setMaximumDate(QDate::currentDate());
    page->addElement(date_time);
    page->addElement(new QuSpacer(QSize(uiconst::BIGSPACE, uiconst::BIGSPACE)));

    heading("medical_history_title");

    yesNoQuestion(FN_HAS_FIBROMYALGIA);
    yesNoQuestion(FN_IS_PREGNANT);
    yesNoQuestion(FN_HAS_INFECTION_PAST_MONTH);
    yesNoQuestion(FN_HAD_INFECTION_TWO_MONTHS_PRECEDING);
    yesNoQuestion(FN_HAS_ALCOHOL_SUBSTANCE_DEPENDENCE);
    multiChoiceQuestion(FN_SMOKING_STATUS, N_SMOKING_STATUS_VALUES);
    doubleQuestion(FN_ALCOHOL_UNITS_PER_WEEK, 0, 2000,
                   xstring("alcohol_units_hint"));

    page->addElement(new QuText(xstring("medical_history_subtitle")));
    yesNoGrid(
        {
            FN_DEPRESSION,
            FN_BIPOLAR_DISORDER,
            FN_SCHIZOPHRENIA,
            FN_AUTISM,
            FN_PTSD,
            FN_ANXIETY,
            FN_PERSONALITY_DISORDER,
            FN_INTELLECTUAL_DISABILITY,
            FN_OTHER_MENTAL_ILLNESS,
        }
    );

    textQuestion(FN_OTHER_MENTAL_ILLNESS_DETAILS);
    yesNoQuestion(FN_HOSPITALISED_IN_LAST_YEAR);
    textQuestion(FN_HOSPITALISATION_DETAILS);

    heading("family_history_title");

    page->addElement(new QuText(xstring("family_history_subtitle")));
    yesNoGrid(
        {
            FN_FAMILY_DEPRESSION,
            FN_FAMILY_BIPOLAR_DISORDER,
            FN_FAMILY_SCHIZOPHRENIA,
            FN_FAMILY_AUTISM,
            FN_FAMILY_PTSD,
            FN_FAMILY_ANXIETY,
            FN_FAMILY_PERSONALITY_DISORDER,
            FN_FAMILY_INTELLECTUAL_DISABILITY,
            FN_FAMILY_OTHER_MENTAL_ILLNESS,
        }
    );

    textQuestion(FN_FAMILY_OTHER_MENTAL_ILLNESS_DETAILS);

    for (auto fieldname : DETAILS_FIELDS.keys()) {
        FieldRefPtr fieldref = fieldRef(fieldname);

        connect(fieldref.data(), &FieldRef::valueChanged,
                this, &KhandakerMojoMedical::updateMandatory);
    }

    QVector<QuPagePtr> pages{page};

    m_questionnaire = new Questionnaire(m_app, pages);
    m_questionnaire->setType(QuPage::PageType::Patient);
    m_questionnaire->setReadOnly(read_only);

    updateMandatory();

    return m_questionnaire;
}

QVariant KhandakerMojoMedical::getDiagnosisDate() const
{
    return value(FN_DIAGNOSIS_DATE);
}

QVariant KhandakerMojoMedical::getDurationOfIllness() const
{
    return m_diagnosis_years;
}

bool KhandakerMojoMedical::setDiagnosisDate(const QVariant& value)
{
    const bool changed = setValue(FN_DIAGNOSIS_DATE, value);
    if (changed) {
        setValue(FN_DIAGNOSIS_DATE_APPROXIMATE, false);
        updateDurationOfIllness();
    }

    return changed;

}

bool KhandakerMojoMedical::setDurationOfIllness(const QVariant& value)
{
    Q_ASSERT(m_fr_diagnosis_years);
    const bool changed = value != m_diagnosis_years;
    if (changed) {
        m_diagnosis_years = value;
        setValue(FN_DIAGNOSIS_DATE_APPROXIMATE, true);
        updateDiagnosisDate();
    }

    return changed;
}


void KhandakerMojoMedical::updateDiagnosisDate()
{
    Q_ASSERT(m_fr_diagnosis_date);
    if (m_diagnosis_years.isNull()) {
        setValue(FN_DIAGNOSIS_DATE, QVariant());
    } else {
        const int years = m_diagnosis_years.toInt();
        setValue(FN_DIAGNOSIS_DATE, QDate::currentDate().addYears(-years));
    }
    m_fr_diagnosis_date->emitValueChanged();
}


void KhandakerMojoMedical::updateDurationOfIllness()
{
    Q_ASSERT(m_fr_diagnosis_years);
    const QVariant diagnosis_date = value(FN_DIAGNOSIS_DATE);
    if (diagnosis_date.isNull()) {
        m_diagnosis_years.clear();
    } else {
        const double days = diagnosis_date.toDate().daysTo(QDate::currentDate());
        m_diagnosis_years = static_cast<int>(floor(0.5 + days / 365.25));
    }
    m_fr_diagnosis_years->emitValueChanged();
}


QString KhandakerMojoMedical::getOptionName(const QString &fieldname,
                                            const int index) const
{
    return xstring(QString("%1_%2").arg(fieldname).arg(index));
}


NameValueOptions KhandakerMojoMedical::getOptions(const QString &fieldname,
                                                  const int num_options) const {
    NameValueOptions options;
    for (int i = 0; i < num_options; i++) {
        const QString name = getOptionName(fieldname, i);
        options.append(NameValuePair(name, i));
    }
    return options;
}


// ============================================================================
// Signal handlers
// ============================================================================

void KhandakerMojoMedical::updateMandatory()
{
    // This could be more efficient with lots of signal handlers, but...

    for (const auto& fieldname : DETAILS_FIELDS.keys()) {
        /*
        // Removed this, thus only showing details when "other Y" chosen
        if (valueIsNull(fieldname)) {
            continue;
        }
        */

        const bool mandatory = valueBool(fieldname);
        const QString details_fieldname = DETAILS_FIELDS.value(fieldname);
        fieldRef(details_fieldname)->setMandatory(mandatory);

        m_questionnaire->setVisibleByTag(details_fieldname, mandatory);
    }
}
