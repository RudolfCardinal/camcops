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

#include "audit.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::anyNull;
using mathfunc::noneNull;
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 10;
const int MAX_SCORE = N_QUESTIONS * 4;
const int STANDARD_CUTOFF = 8;
const QString QPREFIX("q");

const QString Audit::AUDIT_TABLENAME("audit");
const QString TAG_Q2TO3("q2to3");
const QString TAG_Q4TO8("q4to8");


void initializeAudit(TaskFactory& factory)
{
    static TaskRegistrar<Audit> registered(factory);
}


Audit::Audit(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, AUDIT_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Audit::shortname() const
{
    return "AUDIT";
}


QString Audit::longname() const
{
    return tr("Alcohol Use Disorders Identification Test");
}


QString Audit::description() const
{
    return tr("World Health Organization; "
              "10-item clinician-administered screening test.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Audit::isComplete() const
{
    if (anyNull(values({"q1", "q9", "q10"}))) {
        // Always need these three.
        return false;
    }
    if (valueInt("q1") == 0) {
        // Special limited-information completeness
        return true;
    }
    if (noneNull(values({"q2", "q3"})) &&
            valueInt("q2") + valueInt("q3") == 0) {
        // Special limited-information completeness
        return true;
    }
    // Otherwise, any null values cause problems
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


QStringList Audit::summary() const
{
    return QStringList{totalScorePhrase(totalScore(), MAX_SCORE)};
}


QStringList Audit::detail() const
{
    using stringfunc::bold;
    using uifunc::yesNo;
    const bool exceeds_standard_cutoff = totalScore() >= STANDARD_CUTOFF;
    const QString spacer = " ";
    QStringList lines = completenessInfo();
    lines += fieldSummaries("q", "_s", spacer, QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();
    lines.append("");
    lines.append(xstring("exceeds_standard_cutoff") + spacer +
                 bold(yesNo(exceeds_standard_cutoff)));
    return lines;
}


OpenableWidget* Audit::editor(const bool read_only)
{
    const NameValueOptions options1{
        {xstring("q1_option0"), 0},
        {xstring("q1_option1"), 1},
        {xstring("q1_option2"), 2},
        {xstring("q1_option3"), 3},
        {xstring("q1_option4"), 4},
    };
    const NameValueOptions options2{
        {xstring("q2_option0"), 0},
        {xstring("q2_option1"), 1},
        {xstring("q2_option2"), 2},
        {xstring("q2_option3"), 3},
        {xstring("q2_option4"), 4},
    };
    const NameValueOptions options3to8{
        {xstring("q3to8_option0"), 0},
        {xstring("q3to8_option1"), 1},
        {xstring("q3to8_option2"), 2},
        {xstring("q3to8_option3"), 3},
        {xstring("q3to8_option4"), 4},
    };
    const NameValueOptions options9to10{
        {xstring("q9to10_option0"), 0},
        {xstring("q9to10_option2"), 2},
        {xstring("q9to10_option4"), 4},
    };

    QVector<QuPagePtr> pages;

    pages.append(QuPagePtr((new QuPage{
        new QuText(xstring("instructions_1")),
        (new QuText(xstring("instructions_2")))->setBold(true),
        new QuText(xstring("instructions_3")),
        new QuText(xstring("instructions_4")),
        new QuText(xstring("instructions_5")),
    })->setType(QuPage::PageType::Clinician)->setTitle(shortname())));

    auto addPage = [this, &pages](int question,
                                  const NameValueOptions& options,
                                  const QString& tag = "") -> void {
        const QString titlename = QString("q%1_title").arg(question);
        const QString qname = QString("q%1_question").arg(question);
        const QString fieldname = QString("q%1").arg(question);
        QuPagePtr page((new QuPage{
                new QuText(xstring(qname)),
                new QuMcq(fieldRef(fieldname), options),
            })
                ->setTitle(xstring(titlename))
                ->setType(QuPage::PageType::Clinician)
        );
        if (!tag.isEmpty()) {
            page->addTag(tag);
        }
        pages.append(page);
    };

    addPage(1, options1);
    addPage(2, options2, TAG_Q2TO3);
    addPage(3, options3to8, TAG_Q2TO3);
    addPage(4, options3to8, TAG_Q4TO8);
    addPage(5, options3to8, TAG_Q4TO8);
    addPage(6, options3to8, TAG_Q4TO8);
    addPage(7, options3to8, TAG_Q4TO8);
    addPage(8, options3to8, TAG_Q4TO8);
    addPage(9, options9to10);
    addPage(10, options9to10);

    FieldRefPtr fr_q1 = fieldRef("q1");
    FieldRefPtr fr_q2 = fieldRef("q2");
    FieldRefPtr fr_q3 = fieldRef("q3");
    connect(fr_q1.data(), &FieldRef::valueChanged, this, &Audit::setPageSkip);
    connect(fr_q2.data(), &FieldRef::valueChanged, this, &Audit::setPageSkip);
    connect(fr_q3.data(), &FieldRef::valueChanged, this, &Audit::setPageSkip);

    m_questionnaire = new Questionnaire(m_app, pages);
    m_questionnaire->setType(QuPage::PageType::Clinician);
    m_questionnaire->setReadOnly(read_only);

    setPageSkip();  // after m_questionnaire is created

    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Audit::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


// ============================================================================
// Signal handlers
// ============================================================================

void Audit::setPageSkip()
{
    if (!m_questionnaire) {
        return;
    }
    const QVariant q1value = value("q1");
    const QVariant q2value = value("q2");
    const QVariant q3value = value("q3");
    const bool need2to3 = q1value.isNull() || q1value.toInt() != 0;
    const bool need4to8 = need2to3 && (q2value.isNull() ||
                                       q3value.isNull() ||
                                       q2value.toInt() != 0 ||
                                       q3value.toInt() != 0);
    m_questionnaire->setPageSkip(TAG_Q2TO3, !need2to3, false);
    m_questionnaire->setPageSkip(TAG_Q4TO8, !need4to8, true);
}
