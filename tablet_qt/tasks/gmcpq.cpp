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

#include "gmcpq.h"
#include "core/camcopsapp.h"
#include "common/textconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qulineedit.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qumultipleresponse.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using stringfunc::strnum;

const QString GmcPq::GMCPQ_TABLENAME("gmcpq");
const int N_ETHNICITY_OPTIONS = 16;

const QString DOCTOR("doctor");
const QString Q1("q1");
const QString Q2A("q2a");
const QString Q2B("q2b");
const QString Q2C("q2c");
const QString Q2D("q2d");
const QString Q2E("q2e");
const QString Q2F("q2f");
const QString Q2F_DETAILS("q2f_details");
const QString Q3("q3");
const QString Q4A("q4a");
const QString Q4B("q4b");
const QString Q4C("q4c");
const QString Q4D("q4d");
const QString Q4E("q4e");
const QString Q4F("q4f");
const QString Q4G("q4g");
const QString Q5A("q5a");
const QString Q5B("q5b");
const QString Q6("q6");
const QString Q7("q7");
const QString Q8("q8");
const QString Q9("q9");  // other comments
const QString Q10("q10");  // sex
const QString Q11("q11");
const QString Q12("q12");  // ethnicity
const QString Q12_DETAILS("q12_details");


void initializeGmcPq(TaskFactory& factory)
{
    static TaskRegistrar<GmcPq> registered(factory);
}


GmcPq::GmcPq(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, GMCPQ_TABLENAME, true, false, false)  // ... anon, clin, resp
{
    addField(DOCTOR, QVariant::String);
    addField(Q1, QVariant::Int);
    addField(Q2A, QVariant::Bool);
    addField(Q2B, QVariant::Bool);
    addField(Q2C, QVariant::Bool);
    addField(Q2D, QVariant::Bool);
    addField(Q2E, QVariant::Bool);
    addField(Q2F, QVariant::Bool);
    addField(Q2F_DETAILS, QVariant::String);
    addField(Q3, QVariant::Int);
    addField(Q4A, QVariant::Int);
    addField(Q4B, QVariant::Int);
    addField(Q4C, QVariant::Int);
    addField(Q4D, QVariant::Int);
    addField(Q4E, QVariant::Int);
    addField(Q4F, QVariant::Int);
    addField(Q4G, QVariant::Int);
    addField(Q5A, QVariant::Int);
    addField(Q5B, QVariant::Int);
    addField(Q6, QVariant::Bool);
    addField(Q7, QVariant::Bool);
    addField(Q8, QVariant::Bool);
    addField(Q9, QVariant::String);
    addField(Q10, QVariant::String);
    addField(Q11, QVariant::Int);
    addField(Q12, QVariant::Int);  // ethnicity
    addField(Q12_DETAILS, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString GmcPq::shortname() const
{
    return "GMC-PQ";
}


QString GmcPq::longname() const
{
    return tr("UK General Medical Council (GMC) Patient Questionnaire");
}


QString GmcPq::description() const
{
    return tr("Questionnaire for patients to provide anonymous feedback to "
              "their doctors.");
}


// ============================================================================
// Instance info
// ============================================================================

bool GmcPq::isComplete() const
{
    return noneNull(values(QStringList{
                               DOCTOR,
                               Q1,
                               // q2...?
                               Q3,
                               Q4A, Q4B, Q4C, Q4D, Q4E, Q4F, Q4G,
                               Q5A, Q5B,
                               Q6,
                               Q7,
                               Q8
    }));
}


QStringList GmcPq::summary() const
{
    return QStringList{fieldSummary(DOCTOR, xstring("q_doctor"), " ")};
}


QStringList GmcPq::detail() const
{
    const NameValueOptions q4options = optionsQ4();
    const NameValueOptions q5options = optionsQ5();
    QStringList lines = completenessInfo();
    const QString sp = " ";
    const QString co = ": ";
    lines.append(fieldSummary(DOCTOR, xstring("q_doctor"), sp));
    lines.append("");
    lines.append(fieldSummaryNameValueOptions(Q1, optionsQ1(), xstring("q1"), sp));
    lines.append(xstring("q2"));
    lines.append(fieldSummaryYesNoNull(Q2A, xstring("q2_a"), co));
    lines.append(fieldSummaryYesNoNull(Q2B, xstring("q2_b"), co));
    lines.append(fieldSummaryYesNoNull(Q2C, xstring("q2_c"), co));
    lines.append(fieldSummaryYesNoNull(Q2D, xstring("q2_d"), co));
    lines.append(fieldSummaryYesNoNull(Q2E, xstring("q2_e"), co));
    lines.append(fieldSummaryYesNoNull(Q2F, xstring("q2_f"), co));
    lines.append(fieldSummary(Q3, xstring("q3"), sp));
    lines.append(xstring("q4"));
    lines.append(fieldSummaryNameValueOptions(Q4A, q4options, xstring("q4_a"), co));
    lines.append(fieldSummaryNameValueOptions(Q4B, q4options, xstring("q4_b"), co));
    lines.append(fieldSummaryNameValueOptions(Q4C, q4options, xstring("q4_c"), co));
    lines.append(fieldSummaryNameValueOptions(Q4D, q4options, xstring("q4_d"), co));
    lines.append(fieldSummaryNameValueOptions(Q4E, q4options, xstring("q4_e"), co));
    lines.append(fieldSummaryNameValueOptions(Q4F, q4options, xstring("q4_f"), co));
    lines.append(fieldSummaryNameValueOptions(Q4G, q4options, xstring("q4_g"), co));
    lines.append(xstring("q5"));
    lines.append(fieldSummaryNameValueOptions(Q5A, q5options, xstring("q5_a"), co));
    lines.append(fieldSummaryNameValueOptions(Q5B, q5options, xstring("q5_b"), co));
    lines.append(fieldSummaryYesNoNull(Q6, xstring("q6"), co));
    lines.append(fieldSummaryYesNoNull(Q7, xstring("q7"), co));
    lines.append(fieldSummaryYesNoNull(Q8, xstring("q8"), sp));
    lines.append(fieldSummary(Q9, "9. " + xstring("q9_s"), co));
    lines.append(fieldSummary(Q10, "10. " + TextConst::sex(), co));
    lines.append(fieldSummaryNameValueOptions(Q11, optionsQ11(), xstring("q11"), sp));
    lines.append(fieldSummaryNameValueOptions(Q12, ethnicityOptions(m_app), xstring("q12"), sp));
    lines.append(fieldSummary(Q12_DETAILS, xstring("ethnicity_other_s"), co));
    return lines;
}


OpenableWidget* GmcPq::editor(const bool read_only)
{
    QVector<QuPagePtr> pages;
    const NameValueOptions yn_options = CommonOptions::yesNoBoolean();

    auto text = [this](const QString& xstringname) -> QuElement* {
        return new QuText(xstring(xstringname));
    };
    auto boldtext = [this](const QString& xstringname) -> QuElement* {
        return (new QuText(xstring(xstringname)))->setBold();
    };
    auto mcq = [this](const QString& fieldname,
                      const NameValueOptions& options) -> QuElement* {
        return new QuMcq(fieldRef(fieldname), options);
    };
    auto maketitle = [this](int page) -> QString {
        return xstring("titleprefix") + QString::number(page);
    };
    auto qf = [this](const QString& fieldname,
                     const QString& xstringname) -> QuestionWithOneField {
        return QuestionWithOneField(fieldRef(fieldname),
                                    xstring(xstringname));
    };
    auto yn = [this, &yn_options](const QString& fieldname) -> QuElement* {
        return new QuMcq(fieldRef(fieldname), yn_options);
    };

    pages.append(QuPagePtr((new QuPage{
        text("info1"),
        boldtext("please_enter_doctor"),
        new QuLineEdit(fieldRef(DOCTOR)),
        boldtext("info2"),
        text("q1"),
        mcq(Q1, optionsQ1()),
        boldtext("info3"),
    })->setTitle(maketitle(1))));

    pages.append(QuPagePtr((new QuPage{
        text("q2"),
        (new QuMultipleResponse{
            qf(Q2A, "q2_a"),
            qf(Q2B, "q2_b"),
            qf(Q2C, "q2_c"),
            qf(Q2D, "q2_d"),
            qf(Q2E, "q2_e"),
            qf(Q2F, "q2_f"),
        })->setMinimumAnswers(1),
        text("q2f_s"),
        new QuLineEdit(fieldRef(Q2F_DETAILS)),
    })->setTitle(maketitle(2))));

    pages.append(QuPagePtr((new QuPage{
        text("q3"),
        mcq(Q3, optionsQ3()),
    })->setTitle(maketitle(3))));

    pages.append(QuPagePtr((new QuPage{
        text("q4"),
        new QuMcqGrid(QVector<QuestionWithOneField>{qf(Q4A, "q4_a"),
                                                    qf(Q4B, "q4_b"),
                                                    qf(Q4C, "q4_c"),
                                                    qf(Q4D, "q4_d"),
                                                    qf(Q4E, "q4_e"),
                                                    qf(Q4F, "q4_f"),
                                                    qf(Q4G, "q4_g")},
                      optionsQ4()),
    })->setTitle(maketitle(4))));

    pages.append(QuPagePtr((new QuPage{
        text("q5"),
        new QuMcqGrid(QVector<QuestionWithOneField>{qf(Q5A, "q5_a"),
                                                    qf(Q5B, "q5_b")},
                      optionsQ5()),
    })->setTitle(maketitle(5))));

    pages.append(QuPagePtr((new QuPage{
        text("q6"),
        yn(Q6),
    })->setTitle(maketitle(6))));

    pages.append(QuPagePtr((new QuPage{
        text("q7"),
        yn(Q7),
    })->setTitle(maketitle(7))));

    pages.append(QuPagePtr((new QuPage{
        text("q8"),
        yn(Q8),
    })->setTitle(maketitle(8))));

    pages.append(QuPagePtr((new QuPage{
        text("q9"),
        text("q9_s"),
        new QuLineEdit(fieldRef(Q9, false)),
    })->setTitle(maketitle(9))));

    pages.append(QuPagePtr((new QuPage{
        text("q10"),
        mcq(Q10, CommonOptions::sexes()),
    })->setTitle(maketitle(10))));

    pages.append(QuPagePtr((new QuPage{
        text("q11"),
        mcq(Q11, optionsQ11()),
    })->setTitle(maketitle(11))));

    pages.append(QuPagePtr((new QuPage{
        text("q12"),
        mcq(Q12, ethnicityOptions(m_app)),
        text("ethnicity_other_s"),
        new QuLineEdit(fieldRef(Q12_DETAILS)),
    })->setTitle(maketitle(12))));

    pages.append(QuPagePtr((new QuPage{
        new QuText(TextConst::thankYou()),
    })->setTitle(TextConst::finished())));


    connect(fieldRef(Q2F).data(), &FieldRef::valueChanged,
            this, &GmcPq::updateMandatory);
    connect(fieldRef(Q12).data(), &FieldRef::valueChanged,
            this, &GmcPq::updateMandatory);
    updateMandatory();

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

NameValueOptions GmcPq::ethnicityOptions(CamcopsApp& app)
{
    const QString& xstring_taskname = GMCPQ_TABLENAME;
    NameValueOptions options;
    for (int i = 1; i <= N_ETHNICITY_OPTIONS; ++i) {
        options.append(NameValuePair(
            app.xstring(xstring_taskname, strnum("ethnicity_option", i)),
            i));
    }
    return options;
}


bool GmcPq::ethnicityOther(int ethnicity_code)
{
    return ethnicity_code == 3 ||
            ethnicity_code == 7 ||
            ethnicity_code == 11 ||
            ethnicity_code == 14 ||
            ethnicity_code == 16;
}


NameValueOptions GmcPq::optionsQ1() const
{
    return NameValueOptions{
        {xstring("q1_option1"), 1},
        {xstring("q1_option2"), 2},
        {xstring("q1_option3"), 3},
        {xstring("q1_option4"), 4},
    };
}


NameValueOptions GmcPq::optionsQ3() const
{
    return NameValueOptions{
        {xstring("q3_option1"), 1},
        {xstring("q3_option2"), 2},
        {xstring("q3_option3"), 3},
        {xstring("q3_option4"), 4},
        {xstring("q3_option5"), 5},
    };
}


NameValueOptions GmcPq::optionsQ4() const
{
    return NameValueOptions{
        {xstring("q4_option1"), 1},
        {xstring("q4_option2"), 2},
        {xstring("q4_option3"), 3},
        {xstring("q4_option4"), 4},
        {xstring("q4_option5"), 5},
        {xstring("q4_option0"), 0},
    };
}


NameValueOptions GmcPq::optionsQ5() const
{
    return NameValueOptions{
        {xstring("q5_option1"), 1},
        {xstring("q5_option2"), 2},
        {xstring("q5_option3"), 3},
        {xstring("q5_option4"), 4},
        {xstring("q5_option5"), 5},
        {xstring("q5_option0"), 0},
    };
}


NameValueOptions GmcPq::optionsQ11() const
{
    return NameValueOptions{
        {xstring("q11_option1"), 1},
        {xstring("q11_option2"), 2},
        {xstring("q11_option3"), 3},
        {xstring("q11_option4"), 4},
        {xstring("q11_option5"), 5},
    };
}


// ============================================================================
// Signal handlers
// ============================================================================

void GmcPq::updateMandatory()
{
    // This could be more efficient with lots of signal handlers, but...
    const bool need_q2f_details = valueBool(Q2F);
    const bool need_ethnicity_other = ethnicityOther(valueInt(Q12));

    fieldRef(Q2F_DETAILS)->setMandatory(need_q2f_details);
    fieldRef(Q12_DETAILS)->setMandatory(need_ethnicity_other);
}
