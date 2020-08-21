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

#include "lynalliamlife.h"
#include "common/textconst.h"
#include "lib/stringfunc.h"
#include "lib/version.h"
#include "maths/mathfunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevalueoptions.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quhorizontalline.h"
#include "questionnairelib/qulineeditinteger.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::countTrue;
using mathfunc::scorePhrase;
using stringfunc::strnum;
using stringfunc::strseq;


const QString LynallIamLife::LYNALL_IAM_LIFE_TABLENAME("lynall_iam_life");

const int N_QUESTIONS = 14;
const QVector<int> SPECIAL_SEVERITY_QUESTIONS{14};
const QVector<int> SPECIAL_FREQUENCY_QUESTIONS{1, 2, 3, 8};
const QVector<int> FREQUENCY_AS_PERCENT_QUESTIONS{1, 2, 8};

const QString QPREFIX("q");
const QString QSUFFIX_MAIN("_main");
const QString QSUFFIX_SEVERITY("_severity");
const QString QSUFFIX_FREQUENCY("_frequency");

const QString TAG_PREFIX("t");


void initializeLynallIamLife(TaskFactory& factory)
{
    static TaskRegistrar<LynallIamLife> registered(factory);
}


LynallIamLife::LynallIamLife(
        CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, LYNALL_IAM_LIFE_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addFields(strseq(QPREFIX, 1, N_QUESTIONS, QSUFFIX_MAIN), QVariant::Bool);
    addFields(strseq(QPREFIX, 1, N_QUESTIONS, QSUFFIX_SEVERITY), QVariant::Int);
    addFields(strseq(QPREFIX, 1, N_QUESTIONS, QSUFFIX_FREQUENCY), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString LynallIamLife::shortname() const
{
    return "Lynall_IAM_Life";
}


QString LynallIamLife::longname() const
{
    return tr("Lynall M-E — IAM — Life events");
}


QString LynallIamLife::description() const
{
    return tr("Life events questionnaire for IAM immunopsychiatry study, "
              "based on the List of Threatening Experiences (LTE).");
}


Version LynallIamLife::minimumServerVersion() const
{
    return Version(2, 3, 6);
}


// ============================================================================
// Instance info
// ============================================================================

bool LynallIamLife::isComplete() const
{
    for (int q = 1; q <= N_QUESTIONS; ++q) {
        const QVariant value_main = value(qfieldnameMain(q));
        if (value_main.isNull()) {
            return false;
        }
        if (!value_main.toBool()) {
            continue;
        }
        if (valueIsNull(qfieldnameSeverity(q)) ||
                valueIsNull(qfieldnameFrequency(q))) {
            return false;
        }
    }
    return true;
}


QStringList LynallIamLife::summary() const
{
    return QStringList{
        scorePhrase(
            tr("Number of categories endorsed"),
            nCategoriesEndorsed(),
            N_QUESTIONS
        ),
    };
}


QStringList LynallIamLife::detail() const
{
    return summary() + QStringList{
        scorePhrase(
            tr("Severity score"),
            severityScore(),
            N_QUESTIONS * 3
        ),
    };
}


OpenableWidget* LynallIamLife::editor(const bool read_only)
{
    const QString q_generic_severity = xstring("q_generic_severity");
    const QString q_generic_frequency = xstring("q_generic_frequency");
    const NameValueOptions options_yn = CommonOptions::yesNoBoolean();
    const NameValueOptions options_severity{
        {xstring("severity_a3"), 3},
        {xstring("severity_a2"), 2},
        {xstring("severity_a1"), 1},
    };
    const NameValueOptions options_frequency_pct{
        {"0%", 0},
        {"20%", 20},
        {"40%", 40},
        {"60%", 60},
        {"80%", 80},
        {"100%", 100},
    };
    const int min_n_events = 0;
    // ... could well argue for 1, but 0 is the answer if an under-18 takes
    // this!
    const int max_n_events = std::numeric_limits<int>::max();

    QuPagePtr page((new QuPage())->setTitle(xstring("title")));

    for (int q = 1; q <= N_QUESTIONS; ++q) {
        const QString q_main = xstring(strnum(QPREFIX, q, QSUFFIX_MAIN));
        const QString q_severity = SPECIAL_SEVERITY_QUESTIONS.contains(q)
                ? xstring(strnum(QPREFIX, q, QSUFFIX_SEVERITY))
                : q_generic_severity;
        const QString q_frequency = SPECIAL_FREQUENCY_QUESTIONS.contains(q)
                ? xstring(strnum(QPREFIX, q, QSUFFIX_FREQUENCY))
                : q_generic_frequency;
        const QString fn_main = qfieldnameMain(q);
        const QString fn_severity = qfieldnameSeverity(q);
        const QString fn_frequency = qfieldnameFrequency(q);
        const QString tag = tagExtras(q);

        if (q > 1) {
            page->addElement(new QuSpacer());
            page->addElement(new QuHorizontalLine());
            page->addElement(new QuSpacer());
        }

        // Main question/answer
        page->addElement((new QuText(q_main))->setBold());
        page->addElement((new QuMcq(fieldRef(fn_main), options_yn))
                         ->setHorizontal());

        // Severity question/answer
        page->addElement((new QuText(q_severity))->addTag(tag));
        page->addElement((new QuMcq(fieldRef(fn_severity), options_severity))
                         ->setHorizontal()
                         ->addTag(tag));

        // Frequency question/answer
        page->addElement((new QuText(q_frequency))->addTag(tag));
        if (FREQUENCY_AS_PERCENT_QUESTIONS.contains(q)) {
            page->addElement(
                (new QuMcq(fieldRef(fn_frequency), options_frequency_pct))
                    ->setHorizontal()
                    ->addTag(tag));
        } else {
            page->addElement(
                (new QuLineEditInteger(fieldRef(fn_frequency),
                                       min_n_events, max_n_events))
                    ->setHint("")
                    ->addTag(tag)
            );
        }

        // Signals
        connect(fieldRef(fn_main).data(), &FieldRef::valueChanged,
                this, &LynallIamLife::updateMandatory);
    }

    m_questionnaire = new Questionnaire(m_app, {page});
    m_questionnaire->setType(QuPage::PageType::Patient);
    m_questionnaire->setReadOnly(read_only);

    updateMandatory();

    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int LynallIamLife::nCategoriesEndorsed() const
{
    return countTrue(values(strseq(QPREFIX, 1, N_QUESTIONS, QSUFFIX_MAIN)));
}


int LynallIamLife::severityScore() const
{
    int total = 0;
    for (int q = 1; q <= N_QUESTIONS; ++q) {
        const QVariant value_main = value(qfieldnameMain(q));
        if (value_main.toBool()) {
            total += valueInt(qfieldnameSeverity(q));
        }
    }
    return total;
}


QString LynallIamLife::qfieldnameMain(const int qnum) const
{
    return strnum(QPREFIX, qnum, QSUFFIX_MAIN);
}


QString LynallIamLife::qfieldnameSeverity(const int qnum) const
{
    return strnum(QPREFIX, qnum, QSUFFIX_SEVERITY);
}


QString LynallIamLife::qfieldnameFrequency(const int qnum) const
{
    return strnum(QPREFIX, qnum, QSUFFIX_FREQUENCY);
}


QString LynallIamLife::tagExtras(const int qnum) const
{
    return strnum(TAG_PREFIX, qnum);
}



// ============================================================================
// Signal handlers
// ============================================================================

void LynallIamLife::updateMandatory()
{
    if (!m_questionnaire) {
        return;
    }
    for (int q = 1; q <= N_QUESTIONS; ++q) {
        const bool show_extra = valueBool(qfieldnameMain(q));
        m_questionnaire->setVisibleByTag(tagExtras(q), show_extra);
    }
}
