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

#include "icd10specpd.h"
#include "common/appstrings.h"
#include "common/textconst.h"
#include "lib/convert.h"
#include "lib/datetime.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/qudatetime.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
using datetime::shortDate;
using mathfunc::allTrue;
using mathfunc::anyFalse;
using mathfunc::countTrue;
using mathfunc::noneNull;
using stringfunc::standardResult;
using stringfunc::strnum;
using stringfunc::strseq;
using uifunc::yesNoUnknown;

const QString Icd10SpecPD::ICD10SPECPD_TABLENAME("icd10specpd");

const int N_GENERAL = 6;
const int N_GENERAL_1 = 4;
const int N_PARANOID = 7;
const int N_SCHIZOID = 9;
const int N_DISSOCIAL = 6;
const int N_EU = 10;
const int N_EUPD_I = 5;
const int N_HISTRIONIC = 6;
const int N_ANANKASTIC = 8;
const int N_ANXIOUS = 5;
const int N_DEPENDENT = 6;

const QString G_PREFIX("g");
const QString G1_PREFIX("g1_");
const QString PARANOID_PREFIX("paranoid");
const QString SCHIZOID_PREFIX("schizoid");
const QString DISSOCIAL_PREFIX("dissocial");
const QString EU_PREFIX("eu");
const QString HISTRIONIC_PREFIX("histrionic");
const QString ANANKASTIC_PREFIX("anankastic");
const QString ANXIOUS_PREFIX("anxious");
const QString DEPENDENT_PREFIX("dependent");

const QString DATE_PERTAINS_TO("date_pertains_to");
const QString COMMENTS("comments");

const QString SKIP_PARANOID("skip_paranoid");
const QString SKIP_SCHIZOID("skip_schizoid");
const QString SKIP_DISSOCIAL("skip_dissocial");
const QString SKIP_EU("skip_eu");
const QString SKIP_HISTRIONIC("skip_histrionic");
const QString SKIP_ANANKASTIC("skip_anankastic");
const QString SKIP_ANXIOUS("skip_anxious");
const QString SKIP_DEPENDENT("skip_dependent");

const QString OTHER_PD_PRESENT("other_pd_present");  // new in v2.0.0
const QString VIGNETTE("vignette");


void initializeIcd10SpecPD(TaskFactory& factory)
{
    static TaskRegistrar<Icd10SpecPD> registered(factory);
}


Icd10SpecPD::Icd10SpecPD(CamcopsApp& app, DatabaseManager& db,
                         const int load_pk) :
    Task(app, db, ICD10SPECPD_TABLENAME, false, true, false),  // ... anon, clin, resp
    m_fr_has_pd(nullptr)
{
    addFields(strseq(G_PREFIX, 1, N_GENERAL), QVariant::Bool);
    addFields(strseq(G1_PREFIX, 1, N_GENERAL_1), QVariant::Bool);
    addFields(strseq(PARANOID_PREFIX, 1, N_PARANOID), QVariant::Bool);
    addFields(strseq(SCHIZOID_PREFIX, 1, N_SCHIZOID), QVariant::Bool);
    addFields(strseq(DISSOCIAL_PREFIX, 1, N_DISSOCIAL), QVariant::Bool);
    addFields(strseq(EU_PREFIX, 1, N_EU), QVariant::Bool);
    addFields(strseq(HISTRIONIC_PREFIX, 1, N_HISTRIONIC), QVariant::Bool);
    addFields(strseq(ANANKASTIC_PREFIX, 1, N_ANANKASTIC), QVariant::Bool);
    addFields(strseq(ANXIOUS_PREFIX, 1, N_ANXIOUS), QVariant::Bool);
    addFields(strseq(DEPENDENT_PREFIX, 1, N_DEPENDENT), QVariant::Bool);
    addField(DATE_PERTAINS_TO, QVariant::Date);
    addField(COMMENTS, QVariant::String);
    addField(SKIP_PARANOID, QVariant::Bool);
    addField(SKIP_SCHIZOID, QVariant::Bool);
    addField(SKIP_DISSOCIAL, QVariant::Bool);
    addField(SKIP_EU, QVariant::Bool);
    addField(SKIP_HISTRIONIC, QVariant::Bool);
    addField(SKIP_ANANKASTIC, QVariant::Bool);
    addField(SKIP_ANXIOUS, QVariant::Bool);
    addField(SKIP_DEPENDENT, QVariant::Bool);
    addField(OTHER_PD_PRESENT, QVariant::Bool);
    addField(VIGNETTE, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.

    // Extra initialization:
    if (load_pk == dbconst::NONEXISTENT_PK) {
        setValue(DATE_PERTAINS_TO, datetime::nowDate(), false);
    }
}


// ============================================================================
// Class info
// ============================================================================

QString Icd10SpecPD::shortname() const
{
    return "ICD10-PD";
}


QString Icd10SpecPD::longname() const
{
    return tr("ICD-10 criteria for specific personality disorders (F60)");
}


QString Icd10SpecPD::description() const
{
    return TextConst::icd10();
}


QString Icd10SpecPD::infoFilenameStem() const
{
    return "icd";
}


// ============================================================================
// Instance info
// ============================================================================

bool Icd10SpecPD::isComplete() const
{
    return !valueIsNull(DATE_PERTAINS_TO) && (
        isPDExcluded() || (
            isCompleteGeneral() &&
            (valueBool(SKIP_PARANOID) || isCompleteParanoid()) &&
            (valueBool(SKIP_SCHIZOID) || isCompleteSchizoid()) &&
            (valueBool(SKIP_DISSOCIAL) || isCompleteDissocial()) &&
            (valueBool(SKIP_EU) || isCompleteEU()) &&
            (valueBool(SKIP_HISTRIONIC) || isCompleteHistrionic()) &&
            (valueBool(SKIP_ANANKASTIC) || isCompleteAnankastic()) &&
            (valueBool(SKIP_ANXIOUS) || isCompleteAnxious()) &&
            (valueBool(SKIP_DEPENDENT) || isCompleteDependent())
        )
    );
}


QStringList Icd10SpecPD::summary() const
{
    return QStringList{
        standardResult(appstring(appstrings::DATE_PERTAINS_TO),
                       shortDate(value(DATE_PERTAINS_TO))),
        standardResult(xstring("meets_general_criteria"),
                       yesNoUnknown(hasPD())),
    };
}


QStringList Icd10SpecPD::detail() const
{
    return completenessInfo() + QStringList{
        standardResult(appstring(appstrings::DATE_PERTAINS_TO),
                       shortDate(value(DATE_PERTAINS_TO))),
        fieldSummary(COMMENTS, TextConst::examinerComments()),
        standardResult(xstring("meets_general_criteria"),
                       yesNoUnknown(hasPD())),
        standardResult(xstring("paranoid_pd_title"),
                       yesNoUnknown(hasParanoidPD())),
        standardResult(xstring("schizoid_pd_title"),
                       yesNoUnknown(hasSchizoidPD())),
        standardResult(xstring("dissocial_pd_title"),
                       yesNoUnknown(hasDissocialPD())),
        standardResult(xstring("eu_pd_i_title"),
                       yesNoUnknown(hasEUPD_I())),
        standardResult(xstring("eu_pd_b_title"),
                       yesNoUnknown(hasEUPD_B())),
        standardResult(xstring("histrionic_pd_title"),
                       yesNoUnknown(hasHistrionicPD())),
        standardResult(xstring("anankastic_pd_title"),
                       yesNoUnknown(hasAnankasticPD())),
        standardResult(xstring("anxious_pd_title"),
                       yesNoUnknown(hasAnxiousPD())),
        standardResult(xstring("dependent_pd_title"),
                       yesNoUnknown(hasDependentPD())),
        standardResult(xstring("other_pd_title"),
                       yesNoUnknown(value(OTHER_PD_PRESENT))),
        standardResult(xstring("vignette"),
                       valueString(VIGNETTE)),
    };
}


OpenableWidget* Icd10SpecPD::editor(const bool read_only)
{
    const NameValueOptions options = CommonOptions::falseTrueBoolean();

    FieldRef::GetterFunction get_has_pd = std::bind(
                &Icd10SpecPD::getHasPDYesNoUnknown, this);
    FieldRef::SetterFunction set_has_pd = std::bind(
                &Icd10SpecPD::ignoreValue, this, std::placeholders::_1);
    m_fr_has_pd = FieldRefPtr(new FieldRef(get_has_pd, set_has_pd, false));

    auto text = [this](const QString& xstringname) -> QuElement* {
        return new QuText(xstring(xstringname));
    };
#if 0
    auto boldtext = [this](const QString& xstringname) -> QuElement* {
        return (new QuText(xstring(xstringname)))->setBold();
    };
#endif
    auto heading = [this](const QString& xstringname) -> QuElement* {
        return new QuHeading(xstring(xstringname));
    };
    auto gridbase = [this, &options]
            (const QStringList& fieldnames, const QStringList& xstringnames)
            -> QuElement* {
        Q_ASSERT(fieldnames.length() == xstringnames.length());
        QVector<QuestionWithOneField> qfields;
        const int numq = fieldnames.length();
        for (int i = 0; i < numq; ++i) {
            qfields.append(QuestionWithOneField(
                               xstring(xstringnames.at(i)),
                               fieldRef(fieldnames.at(i), false)));
        }
        const int n = options.size();
        const QVector<int> v(n, 1);
        return (new QuMcqGrid(qfields, options))
                ->setExpand(true)
                ->setWidth(n, v);
    };
    auto grid = [&gridbase]
            (const QString& prefix, int end, int start = 1) -> QuElement* {
        // Assumes the xstring name matches the fieldname (as it does)
        const QStringList field_xstring_names = strseq(prefix, start, end);
        return gridbase(field_xstring_names, field_xstring_names);
    };
    auto generalpage = [this, &text, &gridbase]() -> QuPagePtr {
        auto page = new QuPage();
        page->setTitle(xstring("general"));
        page->addElement(text("general"));
        page->addElement(gridbase({strnum(G_PREFIX, 1)}, {"G1"}));
        page->addElement(text("G1b"));
        page->addElement(gridbase(strseq(G1_PREFIX, 1, N_GENERAL_1),
                                  strseq("G1_", 1, N_GENERAL_1)));
        page->addElement(new QuText(TextConst::inAddition() + ":"));
        page->addElement(gridbase(strseq(G_PREFIX, 2, N_GENERAL),
                                  strseq("G", 2, N_GENERAL)));
        page->addElement(text("comments"));
        return QuPagePtr(page);
    };
    auto pdpage = [this, &grid, &text]
            (const QString& prefix,
             int n,
             const QString& skipfield,
             const QString& title_xstring,
             const QString& q_xstring,
             const QString& comment_xstring = "") -> QuPagePtr {
        auto page = new QuPage();
        page->setTitle(xstring(title_xstring));
        page->addElement(
            (new QuBoolean(xstring("skip_this_pd"),
                           fieldRef(skipfield, false)))->setAsTextButton(true));
        page->addElement(new QuText(xstring("general_criteria_must_be_met")));
        page->addElement((new QuText(m_fr_has_pd))->setBold());
        page->addElement(text(q_xstring));
        page->addElement(grid(prefix, n));
        if (!comment_xstring.isEmpty()) {
            page->addElement(text(comment_xstring));
        }
        return QuPagePtr(page);
    };
    auto eupdpage = [this, &grid, &text, &heading]() -> QuPagePtr {
        auto page = new QuPage();
        page->setTitle(xstring("eu_pd_title"));
        page->addElement(
            (new QuBoolean(xstring("skip_this_pd"),
                           fieldRef(SKIP_EU, false)))->setAsTextButton(true));
        page->addElement(text("general_criteria_must_be_met"));
        page->addElement(new QuText(m_fr_has_pd));
        page->addElement(heading("eu_pd_i_title"));
        page->addElement(text("eu_pd_i_B"));
        page->addElement(grid(EU_PREFIX, N_EUPD_I, 1));
        page->addElement(heading("eu_pd_b_title"));
        page->addElement(text("eu_pd_b_B"));
        page->addElement(grid(EU_PREFIX, N_EU, N_EUPD_I + 1));
        return QuPagePtr(page);
    };

    // Overview page
    QVector<QuPagePtr> pages{QuPagePtr((new QuPage{
        getClinicianQuestionnaireBlockRawPointer(),
        new QuText(appstring(appstrings::DATE_PERTAINS_TO)),
        (new QuDateTime(fieldRef(DATE_PERTAINS_TO)))
            ->setMode(QuDateTime::Mode::DefaultDate)
            ->setOfferNowButton(true),
        new QuText(TextConst::comments()),
        new QuTextEdit(fieldRef(COMMENTS, false)),
    })->setTitle(longname()))};

    // General criteria for personality disorders
    pages.append(generalpage());

    // Specific PDs
    pages.append(pdpage(PARANOID_PREFIX, N_PARANOID, SKIP_PARANOID,
                        "paranoid_pd_title", "paranoid_pd_B"));
    pages.append(pdpage(SCHIZOID_PREFIX, N_SCHIZOID, SKIP_SCHIZOID,
                        "schizoid_pd_title", "schizoid_pd_B"));
    pages.append(pdpage(DISSOCIAL_PREFIX, N_DISSOCIAL, SKIP_DISSOCIAL,
                        "dissocial_pd_title", "dissocial_pd_B",
                        "dissocial_pd_comments"));
    pages.append(eupdpage());  // EUPD is more complex
    pages.append(pdpage(HISTRIONIC_PREFIX, N_HISTRIONIC, SKIP_HISTRIONIC,
                        "histrionic_pd_title", "histrionic_pd_B",
                        "histrionic_pd_comments"));
    pages.append(pdpage(ANANKASTIC_PREFIX, N_ANANKASTIC, SKIP_ANANKASTIC,
                        "anankastic_pd_title", "anankastic_pd_B"));
    pages.append(pdpage(ANXIOUS_PREFIX, N_ANXIOUS, SKIP_ANXIOUS,
                        "anxious_pd_title", "anxious_pd_B"));
    pages.append(pdpage(DEPENDENT_PREFIX, N_DEPENDENT, SKIP_DEPENDENT,
                        "dependent_pd_title", "dependent_pd_B"));
    pages.append(QuPagePtr((new QuPage{
        text("other_pd_comments"),
        new QuBoolean(xstring("other_pd_title"), fieldRef(OTHER_PD_PRESENT)),
        text("vignette"),
        new QuTextEdit(fieldRef(VIGNETTE, false)),
    })->setTitle(xstring("other_pd_title"))));

    QStringList connected_fields = strseq(G_PREFIX, 1, N_GENERAL) +
            strseq(G1_PREFIX, 1, N_GENERAL_1) +
            QStringList{
                SKIP_PARANOID,
                SKIP_SCHIZOID,
                SKIP_DISSOCIAL,
                SKIP_EU,
                SKIP_HISTRIONIC,
                SKIP_ANANKASTIC,
                SKIP_ANXIOUS,
                SKIP_DEPENDENT,
                OTHER_PD_PRESENT,
            };
    for (const QString& fieldname : connected_fields) {
        connect(fieldRef(fieldname).data(), &FieldRef::valueChanged,
                this, &Icd10SpecPD::updateMandatory);
    }
    updateMandatory();

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

bool Icd10SpecPD::isPDExcluded() const
{
    const QVector<QVariant> g_values = values(strseq(G_PREFIX, 1, N_GENERAL));
    const QVector<QVariant> g1_values = values(strseq(G1_PREFIX, 1, N_GENERAL_1));
    return anyFalse(g_values) || (noneNull(g1_values) &&
                                  countTrue(g1_values) <= 1);
}


bool Icd10SpecPD::isCompleteGeneral() const
{
    return noneNull(values(strseq(G_PREFIX, 1, N_GENERAL))) &&
            noneNull(values(strseq(G1_PREFIX, 1, N_GENERAL_1)));
}


bool Icd10SpecPD::isCompleteParanoid() const
{
    return noneNull(values(strseq(PARANOID_PREFIX, 1, N_PARANOID)));
}


bool Icd10SpecPD::isCompleteSchizoid() const
{
    return noneNull(values(strseq(SCHIZOID_PREFIX, 1, N_SCHIZOID)));
}


bool Icd10SpecPD::isCompleteDissocial() const
{
    return noneNull(values(strseq(DISSOCIAL_PREFIX, 1, N_DISSOCIAL)));
}


bool Icd10SpecPD::isCompleteEU() const
{
    return noneNull(values(strseq(EU_PREFIX, 1, N_EU)));
}


bool Icd10SpecPD::isCompleteHistrionic() const
{
    return noneNull(values(strseq(HISTRIONIC_PREFIX, 1, N_HISTRIONIC)));
}


bool Icd10SpecPD::isCompleteAnankastic() const
{
    return noneNull(values(strseq(ANANKASTIC_PREFIX, 1, N_ANANKASTIC)));
}


bool Icd10SpecPD::isCompleteAnxious() const
{
    return noneNull(values(strseq(ANXIOUS_PREFIX, 1, N_ANXIOUS)));
}


bool Icd10SpecPD::isCompleteDependent() const
{
    return noneNull(values(strseq(DEPENDENT_PREFIX, 1, N_DEPENDENT)));
}


QVariant Icd10SpecPD::hasPD() const
{
    if (isPDExcluded()) {
        return false;
    }
    if (!isCompleteGeneral()) {
        return QVariant();
    }
    return allTrue(values(strseq(G_PREFIX, 1, N_GENERAL))) &&
            countTrue(values(strseq(G1_PREFIX, 1, N_GENERAL_1))) > 1;
}


QVariant Icd10SpecPD::hasParanoidPD() const
{
    const QVariant has_pd = hasPD();
    if (!has_pd.toBool()) {
        return has_pd;
    }
    if (!isCompleteParanoid()) {
        return QVariant();
    }
    return countTrue(values(strseq(PARANOID_PREFIX, 1, N_PARANOID))) >= 4;
}


QVariant Icd10SpecPD::hasSchizoidPD() const
{
    const QVariant has_pd = hasPD();
    if (!has_pd.toBool()) {
        return has_pd;
    }
    if (!isCompleteSchizoid()) {
        return QVariant();
    }
    return countTrue(values(strseq(SCHIZOID_PREFIX, 1, N_SCHIZOID))) >= 4;
}


QVariant Icd10SpecPD::hasDissocialPD() const
{
    const QVariant has_pd = hasPD();
    if (!has_pd.toBool()) {
        return has_pd;
    }
    if (!isCompleteDissocial()) {
        return QVariant();
    }
    return countTrue(values(strseq(DISSOCIAL_PREFIX, 1, N_DISSOCIAL))) >= 3;
}


QVariant Icd10SpecPD::hasEUPD_I() const
{
    const QVariant has_pd = hasPD();
    if (!has_pd.toBool()) {
        return has_pd;
    }
    if (!isCompleteEU()) {
        return QVariant();
    }
    return countTrue(values(strseq(EU_PREFIX, 1, N_EUPD_I))) >= 3 &&
            valueBool(strnum(EU_PREFIX, 2));
    // It is tempting to add "&& !hasEUPD_B()", on the basis that EUPD(B)
    // trumps EUPD(I), since one requires more symptoms for an EUPD(B)
    // diagnosis. However, that's not what the DCR-10 says (perhaps in error!);
    // it suggests that one can have both EUPD(B) and EUPD(I), if read
    // strictly.
}


QVariant Icd10SpecPD::hasEUPD_B() const
{
    const QVariant has_pd = hasPD();
    if (!has_pd.toBool()) {
        return has_pd;
    }
    if (!isCompleteEU()) {
        return QVariant();
    }
    return countTrue(values(strseq(EU_PREFIX, 1, N_EUPD_I))) >= 3 &&
            countTrue(values(strseq(EU_PREFIX, N_EUPD_I + 1, N_EU))) >= 2;
}


QVariant Icd10SpecPD::hasHistrionicPD() const
{
    const QVariant has_pd = hasPD();
    if (!has_pd.toBool()) {
        return has_pd;
    }
    if (!isCompleteHistrionic()) {
        return QVariant();
    }
    return countTrue(values(strseq(HISTRIONIC_PREFIX, 1, N_HISTRIONIC))) >= 4;
}


QVariant Icd10SpecPD::hasAnankasticPD() const
{
    const QVariant has_pd = hasPD();
    if (!has_pd.toBool()) {
        return has_pd;
    }
    if (!isCompleteAnankastic()) {
        return QVariant();
    }
    return countTrue(values(strseq(ANANKASTIC_PREFIX, 1, N_ANANKASTIC))) >= 4;
}


QVariant Icd10SpecPD::hasAnxiousPD() const
{
    const QVariant has_pd = hasPD();
    if (!has_pd.toBool()) {
        return has_pd;
    }
    if (!isCompleteAnxious()) {
        return QVariant();
    }
    return countTrue(values(strseq(ANXIOUS_PREFIX, 1, N_ANXIOUS))) >= 4;
}


QVariant Icd10SpecPD::hasDependentPD() const
{
    const QVariant has_pd = hasPD();
    if (!has_pd.toBool()) {
        return has_pd;
    }
    if (!isCompleteDependent()) {
        return QVariant();
    }
    return countTrue(values(strseq(DEPENDENT_PREFIX, 1, N_DEPENDENT))) >= 4;
}


// ============================================================================
// Signal handlers
// ============================================================================

void Icd10SpecPD::updateMandatory()
{
    auto set = [this](const QString& prefix, int n, bool mandatory) -> void {
        for (const QString& fieldname : strseq(prefix, 1, n)) {
            fieldRef(fieldname)->setMandatory(mandatory);
        }
    };

    const bool pd_excluded = isPDExcluded();
    const bool need_general = !pd_excluded;
    const bool need_paranoid = !(pd_excluded || valueBool(SKIP_PARANOID));
    const bool need_schizoid = !(pd_excluded || valueBool(SKIP_SCHIZOID));
    const bool need_dissocial = !(pd_excluded || valueBool(SKIP_DISSOCIAL));
    const bool need_eu = !(pd_excluded || valueBool(SKIP_EU));
    const bool need_histrionic = !(pd_excluded || valueBool(SKIP_HISTRIONIC));
    const bool need_anankastic = !(pd_excluded || valueBool(SKIP_ANANKASTIC));
    const bool need_anxious = !(pd_excluded || valueBool(SKIP_ANXIOUS));
    const bool need_dependent = !(pd_excluded || valueBool(SKIP_DEPENDENT));
    const bool need_other = !pd_excluded;
    const bool need_vignette = !pd_excluded && valueBool(OTHER_PD_PRESENT);

    set(G_PREFIX, N_GENERAL, need_general);
    set(G1_PREFIX, N_GENERAL_1, need_general);
    set(PARANOID_PREFIX, N_PARANOID, need_paranoid);
    set(SCHIZOID_PREFIX, N_SCHIZOID, need_schizoid);
    set(DISSOCIAL_PREFIX, N_DISSOCIAL, need_dissocial);
    set(EU_PREFIX, N_EU, need_eu);
    set(HISTRIONIC_PREFIX, N_HISTRIONIC, need_histrionic);
    set(ANANKASTIC_PREFIX, N_ANANKASTIC, need_anankastic);
    set(ANXIOUS_PREFIX, N_ANXIOUS, need_anxious);
    set(DEPENDENT_PREFIX, N_DEPENDENT, need_dependent);
    fieldRef(OTHER_PD_PRESENT)->setMandatory(need_other);
    fieldRef(VIGNETTE)->setMandatory(need_vignette);
}


QVariant Icd10SpecPD::getHasPDYesNoUnknown() const
{
    return yesNoUnknown(hasPD());
}


bool Icd10SpecPD::ignoreValue(const QVariant& value) const
{
    Q_UNUSED(value)
    return false;  // changed?
}
