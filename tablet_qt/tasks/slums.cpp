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

#include "slums.h"
#include "common/colourdefs.h"
#include "common/textconst.h"
#include "lib/datetime.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/qucanvas.h"
#include "questionnairelib/qucountdown.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;
using stringfunc::standardResult;

const QString Slums::SLUMS_TABLENAME("slums");

const QString ALERT("alert");
const QString HIGHSCHOOLEDUCATION("highschooleducation");
const QString Q1("q1");  // scores 1
const QString Q2("q2");  // scores 1
const QString Q3("q3");  // scores 1
// Q4 is "please remember these [...] objects[...]"
const QString Q5A("q5a");  // scores 1
const QString Q5B("q5b");  // scores 2
const QString Q6("q6");  // scores 3
const QString Q7A("q7a");  // scores 1
const QString Q7B("q7b");  // scores 1
const QString Q7C("q7c");  // scores 1
const QString Q7D("q7d");  // scores 1
const QString Q7E("q7e");  // scores 1
// Q8a is not scored (the first backwards digit span)
const QString Q8B("q8b");  // scores 1
const QString Q8C("q8c");  // scores 1
const QString Q9A("q9a");  // scores 2
const QString Q9B("q9b");  // scores 2
const QString Q10A("q10a");  // scores 1
const QString Q10B("q10b");  // scores 1
const QString Q11A("q11a");  // scores 2
const QString Q11B("q11b");  // scores 2
const QString Q11C("q11c");  // scores 2
const QString Q11D("q11d");  // scores 2
const QStringList QLIST{
    Q1,
    Q2,
    Q3,
    Q5A,
    Q5B,
    Q6,
    Q7A,
    Q7B,
    Q7C,
    Q7D,
    Q7E,
    Q8B,
    Q8C,
    Q9A,
    Q9B,
    Q10A,
    Q10B,
    Q11A,
    Q11B,
    Q11C,
    Q11D,
};
const QString CLOCKPICTURE_BLOBID("clockpicture_blobid");
const QString SHAPESPICTURE_BLOBID("shapespicture_blobid");
const QString COMMENTS("comments");

const int MAX_SCORE = 30;
const int NORMAL_IF_GEQ_HIGHSCHOOL = 27;
const int MCI_IF_GEQ_HIGHSCHOOL = 21;
const int NORMAL_IF_GEQ_NO_HIGHSCHOOL = 25;
const int MCI_IF_GEQ_NO_HIGHSCHOOL = 20;
const int COUNTDOWN_TIME_S = 60;

const QString IMAGE_CIRCLE("slums/circle.png");
const QString IMAGE_SHAPES("slums/shapes.png");


void initializeSlums(TaskFactory& factory)
{
    static TaskRegistrar<Slums> registered(factory);
}


Slums::Slums(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, SLUMS_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addField(ALERT, QVariant::Int);
    addField(HIGHSCHOOLEDUCATION, QVariant::Int);
    addFields(QLIST, QVariant::Int);
    addField(CLOCKPICTURE_BLOBID, QVariant::Int);  // FK to BLOB table
    addField(SHAPESPICTURE_BLOBID, QVariant::Int);  // FK to BLOB table
    addField(COMMENTS, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Slums::shortname() const
{
    return "SLUMS";
}


QString Slums::longname() const
{
    return tr("St Louis University Mental Status");
}


QString Slums::description() const
{
    return tr("30-point clinician-administered brief cognitive assessment.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Slums::isComplete() const
{
    return noneNull(values({ALERT, HIGHSCHOOLEDUCATION})) &&
            noneNull(values(QLIST));
}


QStringList Slums::summary() const
{
    return QStringList{totalScorePhrase(totalScore(), MAX_SCORE)};
}


QStringList Slums::detail() const
{
    const int score = totalScore();
    const QString category =
        valueBool(HIGHSCHOOLEDUCATION)
            ? (score >= NORMAL_IF_GEQ_HIGHSCHOOL
               ? TextConst::normal()
               : (score >= MCI_IF_GEQ_HIGHSCHOOL
                  ? xstring("category_mci")
                  : xstring("category_dementia")))
            : (score >= NORMAL_IF_GEQ_NO_HIGHSCHOOL
               ? TextConst::normal()
               : (score >= MCI_IF_GEQ_NO_HIGHSCHOOL
                  ? xstring("category_mci")
                  : xstring("category_dementia")));

    QStringList lines = completenessInfo();
    lines.append(fieldSummaryYesNoNull(ALERT, xstring("alert_s")));
    lines.append(fieldSummaryYesNoNull(HIGHSCHOOLEDUCATION,
                                       xstring("highschool_s")));
    for (const QString& q : QLIST) {
        lines.append(fieldSummary(q, xstring(q)));
    }
    lines.append("");
    lines += summary();
    lines.append("");
    lines.append(standardResult(TextConst::category(), category));
    return lines;
}


OpenableWidget* Slums::editor(const bool read_only)
{
    auto qfields = [this]
            (const QVector<QPair<QString, QString>>& fieldnames_xstringnames,
             bool mandatory = true) ->
            QVector<QuestionWithOneField> {
        QVector<QuestionWithOneField> qf;
        for (const QPair<QString, QString>& fx : fieldnames_xstringnames) {
            qf.append(QuestionWithOneField(fieldRef(fx.first, mandatory),
                                           xstring(fx.second)));
        }
        return qf;
    };
    auto mcqgrid = [this](const QStringList& field_and_xstring_names,
                          const NameValueOptions& options,
                          bool mandatory = true) -> QuElement* {
        QVector<QuestionWithOneField> qfields;
        for (const QString& qx : field_and_xstring_names) {
            qfields.append(QuestionWithOneField(xstring(qx),
                                                fieldRef(qx, mandatory)));
        }
        return new QuMcqGrid(qfields, options);
    };
    auto textRaw = [](const QString& string) -> QuElement* {
        return new QuText(string);
    };
    auto text = [this, textRaw](const QString& stringname) -> QuElement* {
        return textRaw(xstring(stringname));
    };
    auto textRawItalic = [](const QString& string) -> QuElement* {
        return (new QuText(string))->setItalic();
    };
    auto textItalic = [this, textRawItalic](const QString& stringname) -> QuElement* {
        return textRawItalic(xstring(stringname));
    };
    auto canvas = [this](const QString& blob_id_fieldname,
                         const QString& image_filename) -> QuElement* {
        QuCanvas* c = new QuCanvas(
                    blobFieldRef(blob_id_fieldname, true),
                    uifunc::resourceFilename(image_filename));
        c->setBorderWidth(0);
        c->setBorderColour(QCOLOR_TRANSPARENT);
        c->setBackgroundColour(QCOLOR_TRANSPARENT);
        c->setAllowShrink(true);
        return c;
    };

    const QString plural = xstring("title_prefix_plural");
    const QString singular = xstring("title_prefix_singular");
    const QString scoring = xstring("scoring");
    const NameValueOptions incorrect_correct_options =
            CommonOptions::incorrectCorrectInteger();
    const NameValueOptions incorr_0_corr_2_options{
        {CommonOptions::incorrect(), 0},
        {CommonOptions::correct(), 2},  // NB different scoring
    };
    const NameValueOptions q6_options{
        {xstring("q6_option0"), 0},
        {xstring("q6_option1"), 1},
        {xstring("q6_option2"), 2},
        {xstring("q6_option3"), 3},
    };
    const NameValueOptions q7_options{
        {TextConst::notRecalled(), 0},
        {TextConst::recalled(), 1},
    };
    const QDateTime now = datetime::now();
    const QString correct_date = "     " + now.toString("dddd d MMMM yyyy");
    QVector<QuPagePtr> pages;

    pages.append(QuPagePtr((new QuPage{
        getClinicianQuestionnaireBlockRawPointer(),
        new QuMcqGrid(qfields({{ALERT, "q_alert"},
                               {HIGHSCHOOLEDUCATION, "q_highschool"}}),
                      CommonOptions::noYesInteger()),
    })->setTitle(xstring("title_preamble"))));

    pages.append(QuPagePtr((new QuPage{
        mcqgrid({Q1, Q2, Q3}, incorrect_correct_options),
        textItalic("date_now_is"),
        textRawItalic(correct_date),
    })->setTitle(plural + " 1–3")));

    pages.append(QuPagePtr((new QuPage{
        text("q4"),
    })->setTitle(singular + " 4")));

    pages.append(QuPagePtr((new QuPage{
        text("q5"),
        mcqgrid({Q5A}, incorrect_correct_options),
        mcqgrid({Q5B}, incorr_0_corr_2_options),
    })->setTitle(singular + " 5")));

    pages.append(QuPagePtr((new QuPage{
        text("q6"),
        new QuCountdown(COUNTDOWN_TIME_S),
        mcqgrid({Q6}, q6_options),
    })->setTitle(singular + " 6")));

    pages.append(QuPagePtr((new QuPage{
        text("q7"),
        mcqgrid({Q7A, Q7B, Q7C, Q7D, Q7E}, q7_options),
    })->setTitle(singular + " 7")));

    pages.append(QuPagePtr((new QuPage{
        text("q8"),
        mcqgrid({Q8B, Q8C}, incorrect_correct_options),
    })->setTitle(singular + " 8")));

    pages.append(QuPagePtr((new QuPage{
        text("q9"),
        canvas(CLOCKPICTURE_BLOBID, IMAGE_CIRCLE),
    })
        ->setTitle(singular + " 9")
        ->allowScroll(false)
        ->setType(QuPage::PageType::ClinicianWithPatient)));

    pages.append(QuPagePtr((new QuPage{
        mcqgrid({Q9A, Q9B}, incorr_0_corr_2_options),
    })->setTitle(singular + " 9 " + scoring)));

    pages.append(QuPagePtr((new QuPage{
        canvas(SHAPESPICTURE_BLOBID, IMAGE_SHAPES),
        text("q10_part1"),
        text("q10_part2"),
    })
        ->setTitle(singular + " 10")
        ->allowScroll(false)
        ->setType(QuPage::PageType::ClinicianWithPatient)));

    pages.append(QuPagePtr((new QuPage{
        mcqgrid({Q10A, Q10B}, incorrect_correct_options),
    })->setTitle(singular + " 10 " + scoring)));

    pages.append(QuPagePtr((new QuPage{
        text("q11"),
        mcqgrid({Q11A, Q11B, Q11C, Q11D}, incorr_0_corr_2_options),
    })->setTitle(singular + " 11")));

    pages.append(QuPagePtr((new QuPage{
        textRaw(TextConst::examinerComments()),
        (new QuTextEdit(fieldRef(COMMENTS, false)))
            ->setHint(TextConst::examinerCommentsPrompt()),
    })->setTitle(singular + " 12")));

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Slums::totalScore() const
{
    return sumInt(values(QLIST));
}
