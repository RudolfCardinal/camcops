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

#include "mdsupdrs.h"
#include "common/textconst.h"
#include "maths/mathfunc.h"
#include "lib/roman.h"
#include "lib/stringfunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qulineeditdouble.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using stringfunc::strnum;

const QString MdsUpdrs::MDS_UPDRS_TABLENAME("mds_updrs");

// Part I
const QString Q1A("q1a");
const QString Q1_1("q1_1");
const QString Q1_2("q1_2");
const QString Q1_3("q1_3");
const QString Q1_4("q1_4");
const QString Q1_5("q1_5");
const QString Q1_6("q1_6");
const QString Q1_6A("q1_6a");
const QString Q1_7("q1_7");
const QString Q1_8("q1_8");
const QString Q1_9("q1_9");
const QString Q1_10("q1_10");
const QString Q1_11("q1_11");
const QString Q1_12("q1_12");
const QString Q1_13("q1_13");

// Part II
const QString Q2_1("q2_1");
const QString Q2_2("q2_2");
const QString Q2_3("q2_3");
const QString Q2_4("q2_4");
const QString Q2_5("q2_5");
const QString Q2_6("q2_6");
const QString Q2_7("q2_7");
const QString Q2_8("q2_8");
const QString Q2_9("q2_9");
const QString Q2_10("q2_10");
const QString Q2_11("q2_11");
const QString Q2_12("q2_12");
const QString Q2_13("q2_13");

// Part III
const QString Q3A("q3a");
const QString Q3B("q3b");
const QString Q3C("q3c");
const QString Q3C1("q3c1");
const QString Q3_1("q3_1");
const QString Q3_2("q3_2");
const QString Q3_3A("q3_3a");
const QString Q3_3B("q3_3b");
const QString Q3_3C("q3_3c");
const QString Q3_3D("q3_3d");
const QString Q3_3E("q3_3e");
const QString Q3_4A("q3_4a");
const QString Q3_4B("q3_4b");
const QString Q3_5A("q3_5a");
const QString Q3_5B("q3_5b");
const QString Q3_6A("q3_6a");
const QString Q3_6B("q3_6b");
const QString Q3_7A("q3_7a");
const QString Q3_7B("q3_7b");
const QString Q3_8A("q3_8a");
const QString Q3_8B("q3_8b");
const QString Q3_9("q3_9");
const QString Q3_10("q3_10");
const QString Q3_11("q3_11");
const QString Q3_12("q3_12");
const QString Q3_13("q3_13");
const QString Q3_14("q3_14");
const QString Q3_15A("q3_15a");
const QString Q3_15B("q3_15b");
const QString Q3_16A("q3_16a");
const QString Q3_16B("q3_16b");
const QString Q3_17A("q3_17a");
const QString Q3_17B("q3_17b");
const QString Q3_17C("q3_17c");
const QString Q3_17D("q3_17d");
const QString Q3_17E("q3_17e");
const QString Q3_18("q3_18");
const QString Q3_DYSKINESIA_PRESENT("q3_dyskinesia_present");
const QString Q3_DYSKINESIA_INTERFERED("q3_dyskinesia_interfered");
const QString Q3_HY_STAGE("q3_hy_stage");

// Part IV
const QString Q4_1("q4_1");
const QString Q4_2("q4_2");
const QString Q4_3("q4_3");
const QString Q4_4("q4_4");
const QString Q4_5("q4_5");
const QString Q4_6("q4_6");

const QStringList EXTRAFIELDS{
    // Note re efficiency: QString is copy-on-write.
    // Part I
    Q1A,
    Q1_1,
    Q1_2,
    Q1_3,
    Q1_4,
    Q1_5,
    Q1_6,
    Q1_6A,
    Q1_7,
    Q1_8,
    Q1_9,
    Q1_10,
    Q1_11,
    Q1_12,
    Q1_13,
    // Part II
    Q2_1,
    Q2_2,
    Q2_3,
    Q2_4,
    Q2_5,
    Q2_6,
    Q2_7,
    Q2_8,
    Q2_9,
    Q2_10,
    Q2_11,
    Q2_12,
    Q2_13,
    // Part III
    Q3A,
    Q3B,
    Q3C,
    Q3C1,
    Q3_1,
    Q3_2,
    Q3_3A,
    Q3_3B,
    Q3_3C,
    Q3_3D,
    Q3_3E,
    Q3_4A,
    Q3_4B,
    Q3_5A,
    Q3_5B,
    Q3_6A,
    Q3_6B,
    Q3_7A,
    Q3_7B,
    Q3_8A,
    Q3_8B,
    Q3_9,
    Q3_10,
    Q3_11,
    Q3_12,
    Q3_13,
    Q3_14,
    Q3_15A,
    Q3_15B,
    Q3_16A,
    Q3_16B,
    Q3_17A,
    Q3_17B,
    Q3_17C,
    Q3_17D,
    Q3_17E,
    Q3_18,
    Q3_DYSKINESIA_PRESENT,
    Q3_DYSKINESIA_INTERFERED,
    Q3_HY_STAGE,
    // Part IV
    Q4_1,
    Q4_2,
    Q4_3,
    Q4_4,
    Q4_5,
    Q4_6,
};


void initializeMdsUpdrs(TaskFactory& factory)
{
    static TaskRegistrar<MdsUpdrs> registered(factory);
}


MdsUpdrs::MdsUpdrs(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, MDS_UPDRS_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    // Part I
    addField(Q1A, QVariant::Int);
    addField(Q1_1, QVariant::Int);
    addField(Q1_2, QVariant::Int);
    addField(Q1_3, QVariant::Int);
    addField(Q1_4, QVariant::Int);
    addField(Q1_5, QVariant::Int);
    addField(Q1_6, QVariant::Int);
    addField(Q1_6A, QVariant::Int);
    addField(Q1_7, QVariant::Int);
    addField(Q1_8, QVariant::Int);
    addField(Q1_9, QVariant::Int);
    addField(Q1_10, QVariant::Int);
    addField(Q1_11, QVariant::Int);
    addField(Q1_12, QVariant::Int);
    addField(Q1_13, QVariant::Int);
    // Part II
    addField(Q2_1, QVariant::Int);
    addField(Q2_2, QVariant::Int);
    addField(Q2_3, QVariant::Int);
    addField(Q2_4, QVariant::Int);
    addField(Q2_5, QVariant::Int);
    addField(Q2_6, QVariant::Int);
    addField(Q2_7, QVariant::Int);
    addField(Q2_8, QVariant::Int);
    addField(Q2_9, QVariant::Int);
    addField(Q2_10, QVariant::Int);
    addField(Q2_11, QVariant::Int);
    addField(Q2_12, QVariant::Int);
    addField(Q2_13, QVariant::Int);
    // Part III
    addField(Q3A, QVariant::Bool);  // yes/no
    addField(Q3B, QVariant::Int);
    addField(Q3C, QVariant::Bool);  // yes/no
    addField(Q3C1, QVariant::Double);  // minutes
    addField(Q3_1, QVariant::Int);
    addField(Q3_2, QVariant::Int);
    addField(Q3_3A, QVariant::Int);
    addField(Q3_3B, QVariant::Int);
    addField(Q3_3C, QVariant::Int);
    addField(Q3_3D, QVariant::Int);
    addField(Q3_3E, QVariant::Int);
    addField(Q3_4A, QVariant::Int);
    addField(Q3_4B, QVariant::Int);
    addField(Q3_5A, QVariant::Int);
    addField(Q3_5B, QVariant::Int);
    addField(Q3_6A, QVariant::Int);
    addField(Q3_6B, QVariant::Int);
    addField(Q3_7A, QVariant::Int);
    addField(Q3_7B, QVariant::Int);
    addField(Q3_8A, QVariant::Int);
    addField(Q3_8B, QVariant::Int);
    addField(Q3_9, QVariant::Int);
    addField(Q3_10, QVariant::Int);
    addField(Q3_11, QVariant::Int);
    addField(Q3_12, QVariant::Int);
    addField(Q3_13, QVariant::Int);
    addField(Q3_14, QVariant::Int);
    addField(Q3_15A, QVariant::Int);
    addField(Q3_15B, QVariant::Int);
    addField(Q3_16A, QVariant::Int);
    addField(Q3_16B, QVariant::Int);
    addField(Q3_17A, QVariant::Int);
    addField(Q3_17B, QVariant::Int);
    addField(Q3_17C, QVariant::Int);
    addField(Q3_17D, QVariant::Int);
    addField(Q3_17E, QVariant::Int);
    addField(Q3_18, QVariant::Int);
    addField(Q3_DYSKINESIA_PRESENT, QVariant::Bool);
    addField(Q3_DYSKINESIA_INTERFERED, QVariant::Bool);
    addField(Q3_HY_STAGE, QVariant::Int);
    // Part IV
    addField(Q4_1, QVariant::Int);
    addField(Q4_2, QVariant::Int);
    addField(Q4_3, QVariant::Int);
    addField(Q4_4, QVariant::Int);
    addField(Q4_5, QVariant::Int);
    addField(Q4_6, QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString MdsUpdrs::shortname() const
{
    return "MDS-UPDRS";
}


QString MdsUpdrs::longname() const
{
    return tr("Movement Disorder Society-Sponsored Revision of the Unified "
              "Parkinson’s Disease Rating Scale");
}


QString MdsUpdrs::description() const
{
    return tr("Assessment of experiences of daily living and motor "
              "examination/complications.");
}


QString MdsUpdrs::infoFilenameStem() const
{
    return "mds";
}


// ============================================================================
// Instance info
// ============================================================================

bool MdsUpdrs::isComplete() const
{
    return noneNull(values(EXTRAFIELDS));
}


QStringList MdsUpdrs::summary() const
{
    return QStringList{TextConst::noSummarySeeFacsimile()};
}


QStringList MdsUpdrs::detail() const
{
    QStringList lines = completenessInfo();
    for (const QString& fieldname : EXTRAFIELDS) {
        lines.append(fieldSummary(fieldname, fieldname));
    }
    return lines;
}


OpenableWidget* MdsUpdrs::editor(const bool read_only)
{
    const NameValueOptions main_options{
        {tr("Normal"), 0},
        {tr("Slight"), 1},
        {tr("Mild"), 2},
        {tr("Moderate"), 3},
        {tr("Severe"), 4},
    };
    const NameValueOptions source_options{
        {tr("Patient"), 0},
        {tr("Caregiver"), 1},
        {tr("Patient and caregiver"), 2},
    };
    const NameValueOptions on_off_options{
        {TextConst::off(), 0},
        {TextConst::on(), 1},
    };
    const NameValueOptions hy_options{
        {"0", 0},
        {"1", 1},
        {"2", 2},
        {"3", 3},
        {"4", 4},
        {"5", 5},
    };
    QVector<QuPagePtr> pages;

    auto pagetitle = [this](int partnum) -> QString {
        return QString("%1 %2: %3")
                .arg(shortname(),
                     TextConst::part(),
                     roman::romanize(partnum));
    };
    auto text = [](const QString& text) -> QuElement* {
        return (new QuText(text))->setBold();
    };
    auto mcq = [this](const QString& fieldname,
                      const NameValueOptions& options,
                      bool mandatory = true) -> QuElement* {
        return new QuMcq(fieldRef(fieldname, mandatory), options);
    };
    auto grid = [this](const QString& fieldname_prefix,
                       const QString& question_prefix,
                       int first,
                       int last,
                       const NameValueOptions& options,
                       bool mandatory = true) -> QuElement* {
        QVector<QuestionWithOneField> qfields;
        for (int i = first; i <= last; ++i) {
            qfields.append(QuestionWithOneField(
                fieldRef(strnum(fieldname_prefix, i), mandatory),
                strnum(question_prefix, i)
            ));
        }
        return new QuMcqGrid(qfields, options);
    };
    auto part3grid = [this, &main_options]() -> QuElement* {
        const QString fieldname_prefix("q3_");
        const QString question_prefix("Part III, Q3.");
        const QStringList part3bits{
            "1", "2", "3a", "3b", "3c", "3d", "3e",
            "4a", "4b", "5a", "5b", "6a", "6b", "7a", "7b", "8a", "8b",
            "9", "10", "11", "12", "13", "14",
            "15a", "15b", "16a", "16b",
            "17a", "17b", "17c", "17d", "17e",
            "18"
        };
        const bool mandatory = true;
        QVector<QuestionWithOneField> qfields;
        for (const QString& suffix : part3bits) {
            qfields.append(QuestionWithOneField(
                fieldRef(fieldname_prefix + suffix, mandatory),
                question_prefix + suffix
            ));
        }
        return new QuMcqGrid(qfields, main_options);
    };
    auto doublevar = [this](const QString& fieldname,
                            bool mandatory = true) -> QuElement* {
        return new QuLineEditDouble(fieldRef(fieldname, mandatory),
                                    0,
                                    10000000,  // about 19 years, in minutes
                                    1);
    };

    pages.append(QuPagePtr((new QuPage{
        text("Part I, Q1a (information source for 1.1–1.6"),
        mcq(Q1A, source_options),
        grid("q1_", "Part I, Q1.", 1, 6, main_options),
        text("Part I, Q1.6a (information source for 1.7–1.13"),
        mcq(Q1_6A, source_options),
        grid("q1_", "Part I, Q1.", 7, 13, main_options),
    })->setTitle(pagetitle(1))));

    pages.append(QuPagePtr((new QuPage{
        grid("q2_", "Part II, Q2.", 1, 13, main_options),
    })->setTitle(pagetitle(2))));

    pages.append(QuPagePtr((new QuPage{
        text("Part III, Q3a (medication)"),
        mcq(Q3A, CommonOptions::noYesBoolean()),
        text("Part III, Q3b (clinical state)"),
        mcq(Q3B, on_off_options),
        text("Part III, Q3c (levodopa)"),
        mcq(Q3C, CommonOptions::noYesBoolean()),
        text("Q3c.1, minutes since last dose"),
        doublevar(Q3C1),
        part3grid(),
        text("q3_dyskinesia_present"),
        mcq(Q3_DYSKINESIA_PRESENT, CommonOptions::noYesBoolean()),
        text("q3_dyskinesia_interfered"),
        mcq(Q3_DYSKINESIA_INTERFERED, CommonOptions::noYesBoolean()),
        text("Hoehn & Yahr stage"),
        mcq(Q3_HY_STAGE, hy_options),
    })->setTitle(pagetitle(3))));

    pages.append(QuPagePtr((new QuPage{
        grid("q4_", "Part IV, Q4.", 1, 6, main_options),
    })->setTitle(pagetitle(1))));

    // We want Q3C.1 (time since last dose) to be mandatory when Q3C
    // (levodopa?) is true. So we can connect them directly:
    connect(fieldRef(Q3C).data(), &FieldRef::valueChanged,
            this, &MdsUpdrs::levodopaChanged);

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Signal handlers
// ============================================================================

void MdsUpdrs::levodopaChanged(const FieldRef* fieldref)
{
    if (!fieldref) {
        return;
    }
    const bool levodopa = fieldref->valueBool();
    fieldRef(Q3C1)->setMandatory(levodopa);
}
