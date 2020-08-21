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

#include "npiq.h"
#include "lib/convert.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quhorizontalline.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::countTrue;
using mathfunc::scorePhrase;
using stringfunc::standardResult;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 12;
const QString ENDORSED_PREFIX("endorsed");
const QString SEVERITY_PREFIX("severity");
const QString DISTRESS_PREFIX("distress");
const int MAX_ENDORSED = 12;
const int MAX_SEVERITY = 36;
const int MAX_DISTRESS = 60;

const QString NpiQ::NPIQ_TABLENAME("npiq");
const QString ELEMENT_TAG_PREFIX("q");
const QString PAGE_TAG_PREFIX("q");


void initializeNpiQ(TaskFactory& factory)
{
    static TaskRegistrar<NpiQ> registered(factory);
}


NpiQ::NpiQ(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, NPIQ_TABLENAME, false, false, true),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addFields(strseq(ENDORSED_PREFIX, FIRST_Q, N_QUESTIONS), QVariant::Bool);
    addFields(strseq(SEVERITY_PREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);
    addFields(strseq(DISTRESS_PREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString NpiQ::shortname() const
{
    return "NPI-Q";
}


QString NpiQ::longname() const
{
    return tr("Neuropsychiatry Inventory Questionnaire");
}


QString NpiQ::description() const
{
    return tr("12-item carer-rated scale for use in dementia.");
}


// ============================================================================
// Instance info
// ============================================================================

bool NpiQ::isComplete() const
{
    for (int i = FIRST_Q; i <= N_QUESTIONS; ++i) {
        if (!questionComplete(i)) {
            return false;
        }
    }
    return true;
}


QStringList NpiQ::summary() const
{
    return QStringList{
        scorePhrase("Endorsed", endorsedScore(), MAX_ENDORSED),
        scorePhrase("Severity", severityScore(), MAX_SEVERITY),
        scorePhrase("Distress", distressScore(), MAX_DISTRESS),
    };
}


QStringList NpiQ::detail() const
{
    QStringList lines = completenessInfo();
    for (int i = FIRST_Q; i <= N_QUESTIONS; ++i) {
        const QVariant endorsed = value(strnum(ENDORSED_PREFIX, i));
        QString msg = standardResult(xstring(strnum("t", i)),
                                     convert::prettyValue(endorsed),
                                     ": ", "");
        if (endorsed.toBool()) {
            msg += QString(" (severity <b>%1</b>, distress <b>%2</b>)")
                    .arg(prettyValue(strnum(SEVERITY_PREFIX, i)),
                         prettyValue(strnum(DISTRESS_PREFIX, i)));
        }
        msg += ".";
        lines.append(msg);
    }
    lines.append("");
    lines += summary();
    return lines;
}


OpenableWidget* NpiQ::editor(const bool read_only)
{
    const NameValueOptions options_yesno = CommonOptions::noYesBoolean();
    const NameValueOptions options_severity{
        {xstring("severity_1"), 1},
        {xstring("severity_2"), 2},
        {xstring("severity_3"), 3},
    };
    const NameValueOptions options_distress{
        {xstring("distress_0"), 0},
        {xstring("distress_1"), 1},
        {xstring("distress_2"), 2},
        {xstring("distress_3"), 3},
        {xstring("distress_4"), 4},
        {xstring("distress_5"), 5},
    };
    QVector<QuPagePtr> pages;

    auto text = [this](const QString& stringname) -> QuElement* {
        return new QuText(xstring(stringname));
    };
    auto boldtext = [this](const QString& stringname) -> QuElement* {
        return (new QuText(xstring(stringname)))->setBold(true);
    };
    auto addpage = [this, &pages,
                    &options_yesno, &options_severity, &options_distress,
                    &text, &boldtext](int q) -> void {
        const QString pagetitle = QString("NPI-Q (%1 / %2): %3")
                .arg(q)
                .arg(N_QUESTIONS)
                .arg(xstring(strnum("t", q)));
        const QString pagetag = strnum(PAGE_TAG_PREFIX, q);
        const QString tag = strnum(ELEMENT_TAG_PREFIX, q);
        FieldRefPtr endorsed_fr = fieldRef(strnum(ENDORSED_PREFIX, q));
        QuPagePtr page((new QuPage{
            text(strnum("q", q)),
            (new QuMcq(endorsed_fr, options_yesno))
                            ->setHorizontal(true),
            (new QuHorizontalLine())
                            ->addTag(tag),
            (boldtext("severity_instruction"))
                            ->addTag(tag),
            (new QuMcq(fieldRef(strnum(SEVERITY_PREFIX, q)), options_severity))
                            ->addTag(tag),
            (new QuHorizontalLine())
                            ->addTag(tag),
            (boldtext("distress_instruction"))
                            ->addTag(tag),
            (new QuMcq(fieldRef(strnum(DISTRESS_PREFIX, q)), options_distress))
                            ->addTag(tag),
        })->setTitle(pagetitle)->addTag(pagetag));
        pages.append(page);

        connect(endorsed_fr.data(), &FieldRef::valueChanged,
                std::bind(&NpiQ::updateMandatory, this, q));
    };

    pages.append(QuPagePtr((new QuPage{
        getRespondentQuestionnaireBlockRawPointer(true),
        text("instruction_1"),
        boldtext("instruction_2"),
        boldtext("instruction_3"),
        text("instruction_4"),
    })->setTitle(longname())));

    for (int n = FIRST_Q; n <= N_QUESTIONS; ++n) {
        addpage(n);
    }

    m_questionnaire = new Questionnaire(m_app, pages);
    m_questionnaire->setType(QuPage::PageType::Patient);
    m_questionnaire->setReadOnly(read_only);

    for (int n = FIRST_Q; n <= N_QUESTIONS; ++n) {
        updateMandatory(n);
    }

    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int NpiQ::endorsedScore() const
{
    return countTrue(values(strseq(ENDORSED_PREFIX, FIRST_Q, N_QUESTIONS)));
}


int NpiQ::distressScore() const
{
    int score = 0;
    for (int i = FIRST_Q; i <= N_QUESTIONS; ++i) {
        if (valueBool(strnum(ENDORSED_PREFIX, i))) {
            score += valueInt(strnum(DISTRESS_PREFIX, i));
        }
    }
    return score;
}


int NpiQ::severityScore() const
{
    int score = 0;
    for (int i = FIRST_Q; i <= N_QUESTIONS; ++i) {
        if (valueBool(strnum(ENDORSED_PREFIX, i))) {
            score += valueInt(strnum(SEVERITY_PREFIX, i));
        }
    }
    return score;
}


bool NpiQ::questionComplete(const int q) const
{
    const QVariant endorsed = value(strnum(ENDORSED_PREFIX, q));
    if (endorsed.isNull()) {
        return false;
    }
    if (endorsed.toBool()) {
        if (valueIsNull(strnum(DISTRESS_PREFIX, q)) ||
                valueIsNull(strnum(SEVERITY_PREFIX, q))) {
            return false;
        }
    }
    return true;
}


// ============================================================================
// Signal handlers
// ============================================================================

void NpiQ::updateMandatory(const int q)
{
    const bool endorsed = valueBool(strnum(ENDORSED_PREFIX, q));
    fieldRef(strnum(SEVERITY_PREFIX, q))->setMandatory(endorsed);
    fieldRef(strnum(DISTRESS_PREFIX, q))->setMandatory(endorsed);
    if (!m_questionnaire) {
        return;
    }
    const QString element_tag = strnum(ELEMENT_TAG_PREFIX, q);
    const QString page_tag = strnum(PAGE_TAG_PREFIX, q);
    m_questionnaire->setVisibleByTag(element_tag, endorsed, false, page_tag);
}
