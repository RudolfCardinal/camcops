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

#include "shaps.h"
#include "common/textconst.h"
#include "common/uiconst.h"
#include "db/databaseobject.h"
#include "lib/stringfunc.h"
#include "maths/mathfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::anyNull;
using mathfunc::countWhere;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 14;
const int MIN_SCORE = 0;
const int MAX_SCORE = N_QUESTIONS;

const QString QPREFIX("q");

const int STRONGLY_DISAGREE = 0;
const int DISAGREE = 1;
const int AGREE = 2;
const int STRONGLY_OR_DEFINITELY_AGREE = 3;

const QVector<QVariant> SCORING_RESPONSES{STRONGLY_DISAGREE, DISAGREE};
const QVector<int> REVERSE_QUESTIONS{2, 4, 5, 7, 9, 12, 14};


const QString Shaps::SHAPS_TABLENAME("shaps");

void initializeShaps(TaskFactory& factory)
{
    static TaskRegistrar<Shaps> registered(factory);
}


Shaps::Shaps(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, SHAPS_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addFields(fieldNames(), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Shaps::shortname() const
{
    return "SHAPS";
}


QString Shaps::longname() const
{
    return tr("Snaith–Hamilton Pleasure Scale");
}


QString Shaps::description() const
{
    return tr("A scale to measure hedonic tone.");
}


QStringList Shaps::fieldNames() const
{
    return strseq(QPREFIX, FIRST_Q, N_QUESTIONS);
}

// ============================================================================
// Instance info
// ============================================================================


bool Shaps::isComplete() const
{
    if (anyNull(values(fieldNames()))) {
        return false;
    }

    return true;
}


int Shaps::totalScore() const
{
    const QVector<QVariant> responses = values(fieldNames());
    return countWhere(responses, SCORING_RESPONSES);
}


int Shaps::scoreResponse(const QString& fieldname) const
{
    const QVariant response = value(fieldname);

    if (!response.isNull()) {
        if (SCORING_RESPONSES.contains(response)) {
            return 1;
        }
    }

    return 0;
}


QStringList Shaps::summary() const
{
    auto rangeScore = [](const QString& description, const int score,
                         const int min, const int max) {
        return QString("%1: <b>%2</b> [%3–%4].").arg(
                    description,
                    QString::number(score),
                    QString::number(min),
                    QString::number(max));
    };
    return QStringList{
        rangeScore(TextConst::totalScore(), totalScore(),
                   MIN_SCORE, MAX_SCORE),
    };
}


QStringList Shaps::detail() const
{
    QStringList lines = completenessInfo();

    for (int q_number = 1; q_number <= N_QUESTIONS; q_number++) {
        const QString fieldname = strnum(QPREFIX, q_number);
        lines.append(
            QString("%1. %2 %3 (%4)").arg(
                QString::number(q_number),
                xstring(fieldname),
                getAnswerText(q_number, fieldname),
                QString::number(scoreResponse(fieldname))
            )
        );
    }

    lines.append("");
    lines += summary();

    return lines;
}


QString Shaps::getAnswerText(int q_number, const QString& fieldname) const
{
    const QVariant response = value(fieldname);
    if (response.isNull()) {
        return "?";
    }
    switch (response.toInt()) {
    case STRONGLY_DISAGREE:
        return xstring("strongly_disagree");
    case DISAGREE:
        return xstring("disagree");
    case AGREE:
        return xstring("agree");
    case STRONGLY_OR_DEFINITELY_AGREE:
        return REVERSE_QUESTIONS.contains(q_number)
                ? xstring("definitely_agree")
                : xstring("strongly_disagree");
    default:
        return "?";
    }
}


OpenableWidget* Shaps::editor(const bool read_only)
{
    const NameValueOptions agreement_options{
        {xstring("strongly_disagree"), STRONGLY_DISAGREE},
        {xstring("disagree"), DISAGREE},
        {xstring("agree"), AGREE},
        {xstring("strongly_agree"), STRONGLY_OR_DEFINITELY_AGREE},
    };

    const NameValueOptions reverse_agreement_options{
        {xstring("definitely_agree"), STRONGLY_OR_DEFINITELY_AGREE},
        {xstring("agree"), AGREE},
        {xstring("disagree"), DISAGREE},
        {xstring("strongly_disagree"), STRONGLY_DISAGREE},
    };

    m_questionnaire = new Questionnaire(m_app);
    QuPagePtr page((new QuPage{
        new QuText(xstring("instructions")),
        new QuSpacer(QSize(uiconst::BIGSPACE, uiconst::BIGSPACE))
    })->setTitle(xstring("title_main")));

    m_questionnaire->addPage(page);

    for (int q_number = 1; q_number <= N_QUESTIONS; q_number++) {
        const NameValueOptions options = REVERSE_QUESTIONS.contains(q_number)
                ? reverse_agreement_options
                : agreement_options;

        const QString fieldname = strnum(QPREFIX, q_number);
        page->addElement(new QuText(QString("<b>%1. %2</b>").arg(
                                        QString::number(q_number),
                                        xstring(fieldname))));
        page->addElement(new QuMcq(fieldRef(fieldname), options));
        page->addElement(new QuSpacer(QSize(uiconst::BIGSPACE, uiconst::BIGSPACE)));
    }

    m_questionnaire->setType(QuPage::PageType::Patient);
    m_questionnaire->setReadOnly(read_only);

    return m_questionnaire;
}
