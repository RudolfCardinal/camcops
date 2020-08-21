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

#include "moca.h"
#include "common/textconst.h"
#include "lib/datetime.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/qucanvas.h"
#include "questionnairelib/qucountdown.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quimage.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using mathfunc::scorePhrase;
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;
using stringfunc::standardResult;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 28;
const int MAX_SCORE = 30;
const QString QPREFIX("q");

const QString Moca::MOCA_TABLENAME("moca");

const QString IMAGE_PATH("moca/path.png");
const QString IMAGE_CUBE("moca/cube.png");
const QString IMAGE_CLOCK("moca/clock.png");
const QString IMAGE_ANIMALS("moca/animals.png");

const QString EDUCATION12Y_OR_LESS("education12y_or_less");
const QString TRAILPICTURE_BLOBID("trailpicture_blobid");
const QString CUBEPICTURE_BLOBID("cubepicture_blobid");
const QString CLOCKPICTURE_BLOBID("clockpicture_blobid");

const int N_REG_RECALL = 5;
const QString REGISTER_TRIAL1_PREFIX("register_trial1_");
const QString REGISTER_TRIAL2_PREFIX("register_trial2_");
const QString RECALL_CATEGORY_CUE_PREFIX("recall_category_cue_");
const QString RECALL_MC_CUE_PREFIX("recall_mc_cue_");

const QString COMMENTS("comments");

const int NORMAL_IF_GEQ = 26;  // cutoff: normal if score >= this

const QString RECALL_TAG_PREFIX("recall");
const QString SKIP_LABEL("skip");
const QString CATEGORY_RECALL_PAGE_TAG("cr");
const QString MC_RECALL_PAGE_TAG("mc");


void initializeMoca(TaskFactory& factory)
{
    static TaskRegistrar<Moca> registered(factory);
}


Moca::Moca(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, MOCA_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);
    addField(EDUCATION12Y_OR_LESS, QVariant::Int);
    addField(TRAILPICTURE_BLOBID, QVariant::Int);  // FK to BLOB table
    addField(CUBEPICTURE_BLOBID, QVariant::Int);  // FK to BLOB table
    addField(CLOCKPICTURE_BLOBID, QVariant::Int);  // FK to BLOB table
    addFields(strseq(REGISTER_TRIAL1_PREFIX, 1, N_REG_RECALL), QVariant::Int);
    addFields(strseq(REGISTER_TRIAL2_PREFIX, 1, N_REG_RECALL), QVariant::Int);
    addFields(strseq(RECALL_CATEGORY_CUE_PREFIX, 1, N_REG_RECALL), QVariant::Int);
    addFields(strseq(RECALL_MC_CUE_PREFIX, 1, N_REG_RECALL), QVariant::Int);
    addField(COMMENTS, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Moca::shortname() const
{
    return "MoCA";
}


QString Moca::longname() const
{
    return tr("Montreal Cognitive Assessment");
}


QString Moca::description() const
{
    return tr("30-point clinician-administered brief cognitive assessment.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Moca::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Moca::summary() const
{
    return QStringList{totalScorePhrase(totalScore(), MAX_SCORE)};
}


QStringList Moca::detail() const
{
    const int vsp = subScore(1, 5);
    const int naming = subScore(6, 8);
    const int attention = subScore(9, 12);
    const int language = subScore(13, 15);
    const int abstraction = subScore(16, 17);
    const int memory = subScore(18, 22);
    const int orientation = subScore(23, 28);
    const int totalscore = totalScore();
    const QString category = totalscore >= NORMAL_IF_GEQ ? TextConst::normal()
                                                         : TextConst::abnormal();
    QStringList reg1;
    QStringList reg2;
    QStringList recallcat;
    QStringList recallmc;
    const QString prefix_registered = xstring("registered");
    const QString prefix_recalled = xstring("recalled");
    const QString suffix_trial = xstring("trial");
    const QString suffix_cat_recall = xstring("category_recall_suffix");
    const QString suffix_mc_recall = xstring("mc_recall_suffix");
    for (int i = 1; i <= N_REG_RECALL; ++i) {
        QString this_q = xstring(strnum("memory_", i));
        reg1 += fieldSummary(
                    strnum(REGISTER_TRIAL1_PREFIX, i),
                    QString("%1 %2 (%3 1)").arg(prefix_registered, this_q,
                                                suffix_trial));
        reg2 += fieldSummary(
                    strnum(REGISTER_TRIAL2_PREFIX, i),
                    QString("%1 %2 (%3 2)").arg(prefix_registered, this_q,
                                                suffix_trial));
        recallcat += fieldSummary(
                    strnum(RECALL_CATEGORY_CUE_PREFIX, i),
                    QString("%1 %2 %3").arg(prefix_recalled, this_q,
                                            suffix_cat_recall));
        recallmc += fieldSummary(
                    strnum(RECALL_MC_CUE_PREFIX, i),
                    QString("%1 %2 %3").arg(prefix_recalled, this_q,
                                            suffix_mc_recall));
    }

    QStringList lines = completenessInfo();
    lines.append(fieldSummary(EDUCATION12Y_OR_LESS, xstring("education_s")));
    lines += fieldSummaries("q", "_s", " ", QPREFIX, 1, 8);
    lines += reg1;
    lines += reg2;
    lines += fieldSummaries("q", "_s", " ", QPREFIX, 9, 22);
    lines += recallcat;
    lines += recallmc;
    lines += fieldSummaries("q", "_s", " ", QPREFIX, 23, N_QUESTIONS);
    lines.append("");
    lines.append(scorePhrase(xstring("subscore_visuospatial"), vsp, 5));
    lines.append(scorePhrase(xstring("subscore_naming"), naming, 3));
    lines.append(scorePhrase(xstring("subscore_attention"), attention, 6));
    lines.append(scorePhrase(xstring("subscore_language"), language, 3));
    lines.append(scorePhrase(xstring("subscore_abstraction"), abstraction, 2));
    lines.append(scorePhrase(xstring("subscore_memory"), memory, 5));
    lines.append(scorePhrase(xstring("subscore_orientation"), orientation, 6));
    lines.append("");
    lines.append(standardResult(xstring("category"), category));
    lines.append("");
    lines += summary();
    return lines;
}


OpenableWidget* Moca::editor(const bool read_only)
{
    QVector<QuPagePtr> pages;
    const NameValueOptions education_options{
        {xstring("education_option0"), 0},
        {xstring("education_option1"), 1},
    };
    const NameValueOptions options_q12{
        {xstring("q12_option0"), 0},
        {xstring("q12_option1"), 1},
        {xstring("q12_option2"), 2},
        {xstring("q12_option3"), 3},
    };
    const NameValueOptions options_recalled{
        {TextConst::notRecalled(), 0},
        {TextConst::recalled(), 1},
    };
    const NameValueOptions options_corr_incorr = CommonOptions::incorrectCorrectInteger();
    const NameValueOptions options_yesno = CommonOptions::noYesInteger();
    const QString correct_date = "     " + datetime::nowDate().toString(datetime::LONG_DATE_FORMAT);
    const QString recalled = xstring("recalled");

    auto addpage = [&pages]
            (const QString& title,
            std::initializer_list<QuElement*> elements,
            QuPage::PageType type = QuPage::PageType::Inherit,
            bool allow_scroll = true) -> void {
        auto p = new QuPage(elements);
        p->setTitle(title);
        p->setType(type);
        if (!allow_scroll) {
            p->allowScroll(false);
        }
        pages.append(QuPagePtr(p));
    };
    auto text = [this](const QString& xstringname) -> QuElement* {
        return new QuText(xstring(xstringname));
    };
    auto boldtext = [this](const QString& xstringname) -> QuElement* {
        return (new QuText(xstring(xstringname)))->setBold();
    };
    auto italic = [](const QString& text) -> QuElement* {
        return (new QuText(text))->setItalic(true);
    };
    auto mcq = [this](const QString& fieldname,
                      const NameValueOptions& options,
                      bool mandatory = true) -> QuElement* {
        return new QuMcq(fieldRef(fieldname, mandatory), options);
    };
    auto grid1 = [this](const QString& fieldname_prefix,
                        const QString& question_prefix,
                        int first,
                        int last,
                        const NameValueOptions& options,
                        bool mandatory = true) -> QuElement* {
        QVector<QuestionWithOneField> qfields;
        for (int i = first; i <= last; ++i) {
            qfields.append(QuestionWithOneField(
                fieldRef(strnum(fieldname_prefix, i), mandatory),
                xstring(strnum(question_prefix, i))
            ));
        }
        Q_ASSERT(!qfields.empty());
        return new QuMcqGrid(qfields, options);
    };
    auto grid2 = [this](const QString& fieldname_prefix,
                        const QString& first_xstring_name,
                        const QString& xstring_prefix,
                        int first,
                        int last,
                        const NameValueOptions& options,
                        bool mandatory = true) -> QuElement* {
        QVector<QuestionWithOneField> qfields;
        for (int i = first; i <= last; ++i) {
            qfields.append(QuestionWithOneField(
                fieldRef(strnum(fieldname_prefix, i), mandatory),
                xstring(first_xstring_name) + " " +
                               xstring(strnum(xstring_prefix, i))
            ));
        }
        Q_ASSERT(!qfields.empty());
        return new QuMcqGrid(qfields, options);
    };
    auto viewblob = [this](const QString& blob_id_fieldname) -> QuElement* {
        FieldRefPtr fr = fieldRef(blob_id_fieldname, false, true, true);
        return new QuImage(fr);
    };
    auto canvas = [this](const QString& blob_id_fieldname,
                         const QString& image_filename) -> QuElement* {
        QuCanvas* c = new QuCanvas(
                    blobFieldRef(blob_id_fieldname, true),
                    uifunc::resourceFilename(image_filename));
        c->setAllowShrink(true);
        return c;
    };

    addpage(xstring("title_preamble"), {
        getClinicianQuestionnaireBlockRawPointer(),
        text("education_instructions"),
        mcq(EDUCATION12Y_OR_LESS, education_options),
    });

    addpage(xstring("title_prefix_singular") + " 1", {
        text("trail_instructions"),
        canvas(TRAILPICTURE_BLOBID, IMAGE_PATH),
    }, QuPage::PageType::Patient, false);

    addpage(xstring("title_prefix_singular") + " 2", {
        text("cube_instructions"),
        canvas(CUBEPICTURE_BLOBID, IMAGE_CUBE),
    }, QuPage::PageType::Patient, false);

    addpage(xstring("title_prefix_singular") + " 3–5", {
        text("clock_instructions"),
        canvas(CLOCKPICTURE_BLOBID, IMAGE_CLOCK),
    }, QuPage::PageType::Patient, false);

    addpage(xstring("title_prefix_plural") + " 6–8", {
        text("naming_instructions"),
        new QuImage(uifunc::resourceFilename(IMAGE_ANIMALS)),
    }, QuPage::PageType::ClinicianWithPatient);

    addpage(xstring("title_prefix_plural") + " 1–8 " + xstring("scoring"), {
        viewblob(TRAILPICTURE_BLOBID),
        viewblob(CUBEPICTURE_BLOBID),
        viewblob(CLOCKPICTURE_BLOBID),
        grid1(QPREFIX, "q", 1, 8, options_corr_incorr),
    });

    addpage(xstring("title_prefix_plural") + " " + xstring("title_memorize"), {
        text("memory_instruction1"),
        grid2(REGISTER_TRIAL1_PREFIX, "registered", "memory_",
              1, N_REG_RECALL, options_yesno),
        text("memory_instruction2"),
        grid2(REGISTER_TRIAL2_PREFIX, "registered", "memory_",
              1, N_REG_RECALL, options_yesno),
        text("memory_instruction3"),
    });

    addpage(xstring("title_prefix_plural") + " 9–12", {
        text("digit_forward_instructions"),
        grid1(QPREFIX, "q", 9, 9, options_corr_incorr),
        text("digit_backward_instructions"),
        grid1(QPREFIX, "q", 10, 10, options_corr_incorr),
        text("tapping_instructions"),
        grid1(QPREFIX, "q", 11, 11, options_corr_incorr),
        text("q12"),
        mcq(strnum(QPREFIX, 12), options_q12),
    });

    addpage(xstring("title_prefix_plural") + " 13–15", {
        text("repetition_instructions_1"),
        grid1(QPREFIX, "q", 13, 13, options_corr_incorr),
        text("repetition_instructions_2"),
        grid1(QPREFIX, "q", 14, 14, options_corr_incorr),
        text("fluency_instructions"),
        new QuCountdown(60),
        grid1(QPREFIX, "q", 15, 15, options_yesno),
    });

    addpage(xstring("title_prefix_plural") + " 16–17", {
        text("abstraction_instructions"),
        grid1(QPREFIX, "q", 16, 17, options_corr_incorr),
    });


    QVector<QuestionWithOneField> qf_recall;
    for (int i = 1; i <= N_REG_RECALL; ++i) {
        // Strings range from 1-5 but questions from 18-22.
        int qnum = i + 17;
        QString fieldname = strnum(QPREFIX, qnum);
        qf_recall.append(QuestionWithOneField(
                             fieldRef(fieldname),
                             recalled + " " + xstring(strnum("memory_", i))));
        connect(fieldRef(fieldname).data(), &FieldRef::valueChanged,
                this, &Moca::updateMandatory);
    }
    addpage(xstring("title_prefix_plural") + " 18–22", {
        text("recall_instructions"),
        new QuMcqGrid(qf_recall, options_recalled),
    });

    QVector<QuElement*> cat_elements;
    QVector<QuElement*> mc_elements;
    cat_elements.append(text("category_recall_instructions"));
    mc_elements.append(text("mc_recall_instructions"));
    for (int i = 1; i <= N_REG_RECALL; ++i) {
        QString tag = strnum(RECALL_TAG_PREFIX, i);
        cat_elements.append(grid1(RECALL_CATEGORY_CUE_PREFIX,
                                  "category_recall_", i, i, options_recalled)
                            ->addTag(tag));
        mc_elements.append(grid1(RECALL_MC_CUE_PREFIX,
                                 "mc_recall_", i, i, options_recalled)
                           ->addTag(tag));
        connect(fieldRef(strnum(RECALL_CATEGORY_CUE_PREFIX, i)).data(),
                &FieldRef::valueChanged,
                this, &Moca::updateMandatory);
    }
    cat_elements.append(boldtext("no_need_for_extra_recall")->addTag(SKIP_LABEL));
    mc_elements.append(boldtext("no_need_for_extra_recall")->addTag(SKIP_LABEL));

    QuPagePtr cat_recall_page((new QuPage(cat_elements))
        ->setTitle(xstring("title_prefix_plural") + " 18–22 " +
                   xstring("category_recall_suffix"))
        ->addTag(CATEGORY_RECALL_PAGE_TAG));
    pages.append(cat_recall_page);

    QuPagePtr mc_recall_page((new QuPage(mc_elements))
        ->setTitle(xstring("title_prefix_plural") + " 18–22 " +
                   xstring("mc_recall_suffix"))
        ->addTag(MC_RECALL_PAGE_TAG));
    pages.append(mc_recall_page);

    addpage(xstring("title_prefix_plural") + " 23–28", {
        text("orientation_instructions"),
        grid1(QPREFIX, "q", 23, 28, options_corr_incorr),
        italic(xstring("date_now_is")),
        italic(correct_date),
    });

    addpage(TextConst::examinerComments(), {
        new QuText(TextConst::examinerCommentsPrompt()),
        (new QuTextEdit(fieldRef(COMMENTS, false)))
                ->setHint(TextConst::examinerComments()),
    });

    m_questionnaire = new Questionnaire(m_app, pages);
    m_questionnaire->setType(QuPage::PageType::Clinician);
    m_questionnaire->setReadOnly(read_only);

    updateMandatory();

    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Moca::subScore(const int first, const int last) const
{
    return sumInt(values(strseq(QPREFIX, first, last)));
}


int Moca::totalScore() const
{
    // MOCA instructions:
    // - "The total possible score is 30 points"
    // - "TOTAL SCORE: Sum all subscores listed on the right-hand side. Add one
    //   point for an individual who has 12 years or fewer of formal education,
    //   for a possible maximum of 30 points."
    //
    // - The subscores add up to 30.
    // - So, presumably this means "add one point if you have <=12 years of
    //   education AND your score is less than 30", or equivalently "add one
    //   point... and take the maximum of (your score, 30)".

    int score = subScore(FIRST_Q, N_QUESTIONS);
    if (score < MAX_SCORE) {
        score += valueInt(EDUCATION12Y_OR_LESS);  // extra point for this
    }
    return score;
}


// ============================================================================
// Signal handlers
// ============================================================================

void Moca::updateMandatory()
{
    if (!m_questionnaire) {
        return;
    }
    int n_cat = 0;
    int n_mc = 0;
    for (int i = 1; i <= N_REG_RECALL; ++i) {
        const int qnum = i + 17;
        const QVariant v = value(strnum(QPREFIX, qnum));
        const bool cat_required = v.toInt() == 0;  // also true if NULL
        const QString recall_field = strnum(RECALL_CATEGORY_CUE_PREFIX, i);
        const QString tag = strnum(RECALL_TAG_PREFIX, i);
        fieldRef(recall_field)->setMandatory(cat_required);
        m_questionnaire->setVisibleByTag(tag, cat_required, false,
                                         CATEGORY_RECALL_PAGE_TAG);
        const bool mc_required = cat_required &&
                valueInt(strnum(RECALL_CATEGORY_CUE_PREFIX, i)) == 0;
        m_questionnaire->setVisibleByTag(tag, mc_required, false,
                                         MC_RECALL_PAGE_TAG);
        n_cat += cat_required;
        n_mc += mc_required;
    }
    const bool require_cat_skip_label = n_cat == 0;
    const bool require_mc_skip_label = n_mc == 0;
    m_questionnaire->setVisibleByTag(SKIP_LABEL, require_cat_skip_label, false,
                                     CATEGORY_RECALL_PAGE_TAG);
    m_questionnaire->setVisibleByTag(SKIP_LABEL, require_mc_skip_label, false,
                                     MC_RECALL_PAGE_TAG);

}
