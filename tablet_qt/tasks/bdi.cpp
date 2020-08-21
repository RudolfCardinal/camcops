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

#include "bdi.h"
#include "common/appstrings.h"
#include "common/textconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/questionwithonefield.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 21;
const int MAX_SCORE = N_QUESTIONS * 3;
const QString QPREFIX("q");

const int SUICIDALITY_QNUM = 9;  // Q9 in all versions of the BDI (I, IA, II)
const QString SUICIDALITY_FN = "q9";  // fieldname

const QVector<int> CUSTOM_SOMATIC_KHANDAKER_BDI_II_QNUMS{
    4, 15, 16, 18, 19, 20, 21};

const QString Bdi::BDI_TABLENAME("bdi");
const QString FN_BDI_SCALE("bdi_scale");

const QString SCALE_BDI_I("BDI-I");
const QString SCALE_BDI_IA("BDI-IA");
const QString SCALE_BDI_II("BDI-II");

const QStringList BDI_I_QUESTION_TOPICS{
    // from Beck 1988, https://doi.org/10.1016/0272-7358(88)90050-5
    "",  // index zero
    "mood",  // a
    "pessimism",  // b
    "sense of failure",  // c
    "lack of satisfaction",  // d
    "guilt feelings",  // e
    "sense of punishment",  // f
    "self-dislike",  // g
    "self-accusation",  // h
    "suicidal wishes",  // i
    "crying",  // j
    "irritability",  // k
    "social withdrawal",  // l
    "indecisiveness",  // m
    "distortion of body image",  // n
    "work inhibition",  // o
    "sleep disturbance",  // p
    "fatigability",  // q
    "loss of appetite",  // r
    "weight loss",  // s
    "somatic preoccupation",  // t
    "loss of libido",  // u
};
const QStringList BDI_IA_QUESTION_TOPICS = {
    // from [Beck1996b]
    "",  // index zero
    "sadness",  // 1
    "pessimism",
    "sense of failure",
    "self-dissatisfaction",
    "guilt",  // 5
    "punishment",
    "self-dislike",
    "self-accusations",
    "suicidal ideas",
    "crying",  // 10
    "irritability",
    "social withdrawal",
    "indecisiveness",
    "body image change",
    "work difficulty",  // 15
    "insomnia",
    "fatigability",
    "loss of appetite",
    "weight loss",
    "somatic preoccupation",  // 20
    "loss of libido",
};
const QStringList BDI_II_QUESTION_TOPICS = {
    // from https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5889520/;
    // also https://www.ncbi.nlm.nih.gov/pubmed/10100838;
    // also [Beck1996b]
    // matches BDI-II paper version
    "",  // index zero
    "sadness",  // 1
    "pessimism",
    "past failure",
    "loss of pleasure",
    "guilty feelings",  // 5
    "punishment feelings",
    "self-dislike",
    "self-criticalness",
    "suicidal thoughts or wishes",
    "crying",  // 10
    "agitation",
    "loss of interest",
    "indecisiveness",
    "worthlessness",
    "loss of energy",  // 15
    "changes in sleeping pattern",  // decrease or increase
    "irritability",
    "changes in appetite",  // decrease or increase
    "concentration difficulty",
    "tiredness or fatigue",  // 20
    "loss of interest in sex",
};


void initializeBdi(TaskFactory& factory)
{
    static TaskRegistrar<Bdi> registered(factory);
}


Bdi::Bdi(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, BDI_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_grid_i(nullptr),
    m_grid_ia(nullptr),
    m_grid_ii(nullptr)
{
    addField(FN_BDI_SCALE, QVariant::String);
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Bdi::shortname() const
{
    return "BDI";
}


QString Bdi::longname() const
{
    return tr("Beck Depression Inventory");
}


QString Bdi::description() const
{
    return tr("21-item self-report scale (for BDI, BDI-1A, BDI-II).");
}


// ============================================================================
// Instance info
// ============================================================================

bool Bdi::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


bool Bdi::isBdiII() const
{
    return valueString(FN_BDI_SCALE) == SCALE_BDI_II;
}


QStringList Bdi::summary() const
{
    using mathfunc::describeAsRanges;
    using stringfunc::bold;
    using stringfunc::strnumlist;

    const QString scale = valueString(FN_BDI_SCALE);
    const QVariant suicide_value = value(SUICIDALITY_FN);

    // Suicidal thoughts:
    QString suicide_description;
    if (suicide_value.isNull()) {
        suicide_description = "? (not completed)";
    } else {
        const int suicidality_score = suicide_value.toInt();
        suicide_description = QString::number(suicidality_score);
    }

    // Custom somatic score for Khandaker Insight study:
    QString somatic_text;
    if (isBdiII()) {
        const QStringList somatic_fieldnames = strnumlist(
                    QPREFIX, CUSTOM_SOMATIC_KHANDAKER_BDI_II_QNUMS);
        const QVector<QVariant> somatic_values = values(somatic_fieldnames);
        bool somatic_missing = false;
        int somatic_score = 0;
        for (const QVariant& v : somatic_values) {
            if (v.isNull()) {
                somatic_missing = true;
                break;
            }
            somatic_score += v.toInt();
        }
        somatic_text = somatic_missing
                ? "incomplete"
                : QString::number(somatic_score);
    } else {
        somatic_text = "N/A";  // not the BDI-II
    }

    QString suicidality_topic;
    if (scale == SCALE_BDI_I) {
        suicidality_topic = BDI_I_QUESTION_TOPICS.at(SUICIDALITY_QNUM);
    } else if (scale == SCALE_BDI_IA) {
        suicidality_topic = BDI_IA_QUESTION_TOPICS.at(SUICIDALITY_QNUM);
    } else if (scale == SCALE_BDI_II) {
        suicidality_topic = BDI_II_QUESTION_TOPICS.at(SUICIDALITY_QNUM);
    } else {
        suicidality_topic = "suicidality";
    }

    // Summary:
    return QStringList{
        QString("Scale: %1.").arg(bold(valueString(FN_BDI_SCALE))),
        totalScorePhrase(totalScore(), MAX_SCORE),
        // Q9 is suicidal ideation in all versions of the BDI (I, IA, II).
        QString("Q%1 (%2): %3.").arg(
                QString::number(SUICIDALITY_QNUM),
                suicidality_topic,
                bold(suicide_description)),
        QString("Custom somatic score for Insight study "
                "(sum of scores for questions %1 for BDI-II only): %2.").arg(
                describeAsRanges(CUSTOM_SOMATIC_KHANDAKER_BDI_II_QNUMS),
                bold(somatic_text)),
    };
}


QStringList Bdi::detail() const
{
    return summary() + completenessInfo();
}


OpenableWidget* Bdi::editor(const bool read_only)
{
    const NameValueOptions options{
        {"0", 0},
        {"1", 1},
        {"2", 2},
        {"3", 3},
    };
    const NameValueOptions scale_options{
        {"BDI (1961; BDI-I)", SCALE_BDI_I},
        {"BDI-IA (1978)", SCALE_BDI_IA},
        {"BDI-II (1996)", SCALE_BDI_II},
    };
    QVector<QuestionWithOneField> fields_i;
    QVector<QuestionWithOneField> fields_ia;
    QVector<QuestionWithOneField> fields_ii;
    const QString& question_prefix = TextConst::question();
    for (int n = FIRST_Q; n <= N_QUESTIONS; ++n) {
        const QString qstrnum = QString::number(n);
        const QString fieldname = QPREFIX + qstrnum;
        const QString& topic_i = BDI_I_QUESTION_TOPICS.at(n);
        const QString& topic_ia = BDI_IA_QUESTION_TOPICS.at(n);
        const QString& topic_ii = BDI_II_QUESTION_TOPICS.at(n);
        const QString working_prefix = question_prefix + " " + qstrnum + " (";
        const QString question_i = working_prefix + topic_i + ")";
        const QString question_ia = working_prefix + topic_ia + ")";
        const QString question_ii = working_prefix + topic_ii + ")";
        fields_i.append(QuestionWithOneField(fieldRef(fieldname), question_i));
        fields_ia.append(QuestionWithOneField(fieldRef(fieldname), question_ia));
        fields_ii.append(QuestionWithOneField(fieldRef(fieldname), question_ii));
    }

    m_grid_i = new QuMcqGrid(fields_i, options);
    m_grid_ia = new QuMcqGrid(fields_ia, options);
    m_grid_ii = new QuMcqGrid(fields_ii, options);
    m_grid_i->addTag(SCALE_BDI_I);
    m_grid_ia->addTag(SCALE_BDI_IA);
    m_grid_ii->addTag(SCALE_BDI_II);

    if (valueIsNullOrEmpty(FN_BDI_SCALE)) {
        // first edit; set default
        setValue(FN_BDI_SCALE, SCALE_BDI_II);
    }

    // Set initial visibility:
    scaleChanged();

    // Callback
    FieldRefPtr fr_scale = fieldRef(FN_BDI_SCALE);
    connect(fr_scale.data(), &FieldRef::valueChanged,
            this, &Bdi::scaleChanged);

    QuPagePtr page((new QuPage({
        (new QuText(appstring(appstrings::DATA_COLLECTION_ONLY)))->setBold(),
        new QuText(appstring(appstrings::BDI_WHICH_SCALE)),
        (new QuMcq(fr_scale, scale_options))
            ->setHorizontal(true)
            ->setAsTextButton(true),
        new QuText(TextConst::enterTheAnswers()),
        // Add all three grids; we'll swap between them.
        m_grid_i,
        m_grid_ia,
        m_grid_ii,
    }))->setTitle(shortname()));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Bdi::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


// ============================================================================
// Signal handlers
// ============================================================================

void Bdi::scaleChanged()
{
    if (!m_grid_i || !m_grid_ia || !m_grid_ii) {
        return;
    }
    const QString current_scale = valueString(FN_BDI_SCALE);
    // Initial visibility
    m_grid_i->setVisible(current_scale == SCALE_BDI_I);
    m_grid_ia->setVisible(current_scale == SCALE_BDI_IA);
    m_grid_ii->setVisible(current_scale == SCALE_BDI_II);
}
