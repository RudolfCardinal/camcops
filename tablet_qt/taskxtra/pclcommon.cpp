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

#include "pclcommon.h"
#include "core/camcopsapp.h"
#include "common/textconst.h"
#include "common/varconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
using mathfunc::countNull;
using mathfunc::noneNull;
using mathfunc::sumInt;
using mathfunc::totalScorePhrase;
using stringfunc::standardResult;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 17;
const int MAX_SCORE = 85;
const QString QPREFIX("q");

const QString EVENT("event");
const QString EVENTDATE("eventdate");


PclCommon::PclCommon(CamcopsApp& app,
                     DatabaseManager& db,
                     const QString& tablename,
                     const QString& xstring_prefix,
                     const bool specific_event,
                     const int load_pk) :
    Task(app, db, tablename, false, false, false),  // ... anon, clin, resp
    m_xstring_prefix(xstring_prefix),
    m_specific_event(specific_event)
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);
    if (m_specific_event) {
        addField(EVENT, QVariant::String);
        addField(EVENTDATE, QVariant::String);  // free text from subject
    }

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString PclCommon::description() const
{
    return tr("17-item self-report scale, based on DSM-IV-TR criteria.");
}


QString PclCommon::infoFilenameStem() const
{
    return "pcl";
}


QString PclCommon::xstringTaskname() const
{
    return "pcl";
}


// ============================================================================
// Instance info
// ============================================================================

bool PclCommon::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS))) &&
            (!m_specific_event || (
                 !valueIsNullOrEmpty(EVENT) &&
                 !valueIsNull(EVENTDATE)
            ));
}


QStringList PclCommon::summary() const
{
    return QStringList{
        totalScorePhrase(totalScore(), MAX_SCORE),
        standardResult(xstring("dsm_criteria_met"),
                       uifunc::yesNoUnknown(hasPtsd()))
    };
}


QStringList PclCommon::detail() const
{
    QStringList lines = completenessInfo();
    if (m_specific_event) {
        lines.append(fieldSummary(EVENT, xstring("s_event_s")));
        lines.append(fieldSummary(EVENTDATE, xstring("s_eventdate_s")));
    }
    lines.append("");
    lines += summary();
    return lines;
}


OpenableWidget* PclCommon::editor(const bool read_only)
{
    const NameValueOptions options{
        {xstring("option1"), 1},
        {xstring("option2"), 2},
        {xstring("option3"), 3},
        {xstring("option4"), 4},
        {xstring("option5"), 5},
    };

    QVector<QuestionWithOneField> qfields;
    for (int i = FIRST_Q; i <= N_QUESTIONS; ++i) {
        QString xstringname = strnum(i <= 8 ? m_xstring_prefix + "_q" : "q", i);
        qfields.append(QuestionWithOneField(xstring(xstringname),
                                            fieldRef(strnum(QPREFIX, i))));
    }

    QVector<QuElement*> elements;
    auto addtext = [this, &elements](const QString& xstringname,
                                     bool bold = false) -> void {
        auto text = new QuText(xstring(xstringname));
        if (bold) {
            text->setBold(true);
        }
        elements.append(text);
    };
    auto addedit = [this, &elements](const QString& fieldname,
                                     const QString& xstringname,
                                     bool mandatory = true) -> void {
        elements.append((new QuTextEdit(fieldRef(fieldname, mandatory)))
                     ->setHint(xstring(xstringname)));
    };

    if (m_specific_event) {
        addtext("s_event_prompt");
        addedit(EVENT, "s_event_hint");
        addtext("s_eventdate_prompt");
        addedit(EVENTDATE, "s_eventdate_hint");
    }
    addtext(m_xstring_prefix + "_instructions");
    const QVector<McqGridSubtitle> subtitles{
        {5, ""},
        {12, ""},
    };
    elements.append((new QuMcqGrid(qfields, options))->setSubtitles(subtitles));

    QuPagePtr page((new QuPage(elements))
                   ->setTitle(xstring(m_xstring_prefix + "_title")));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int PclCommon::totalScore() const
{
    return sumInt(values(strseq(QPREFIX, FIRST_Q, N_QUESTIONS)));
}


int PclCommon::numSymptomatic(const int first, const int last) const
{
    int total = 0;
    for (int i = first; i <= last; ++i) {
        if (valueInt(strnum(QPREFIX, i)) >= 3) {
            // 3 and above scores as "symptomatic":
            // http://www.mirecc.va.gov/docs/visn6/3_PTSD_CheckList_and_Scoring.pdf
            ++total;
        }
    }
    return total;
}


int PclCommon::numNull(const int first, const int last) const
{
    return countNull(values(strseq(QPREFIX, first, last)));
}


QVariant PclCommon::hasPtsd() const
{
    // PTSD = at least one "B" item
    //        and at least three "C" items
    //        and at least two "D" items:
    // http://www.mirecc.va.gov/docs/visn6/3_PTSD_CheckList_and_Scoring.pdf
    const int first_b = 1;
    const int last_b = 5;
    const int first_c = 6;
    const int last_c = 12;
    const int first_d = 13;
    const int last_d = 17;

    const int criterion_b = 1;
    const int criterion_c = 3;
    const int criterion_d = 2;

    int symptomatic_b = numSymptomatic(first_b, last_b);
    int symptomatic_c = numSymptomatic(first_c, last_c);
    int symptomatic_d = numSymptomatic(first_d, last_d);

    int null_b = numNull(first_b, last_b);
    int null_c = numNull(first_c, last_c);
    int null_d = numNull(first_d, last_d);

    if (symptomatic_b >= criterion_b &&
            symptomatic_c >= criterion_c &&
            symptomatic_d >= criterion_d) {
        return true;  // has PTSD
    }
    if (symptomatic_b + null_b >= criterion_b &&
            symptomatic_c + null_c >= criterion_c &&
            symptomatic_d + null_d >= criterion_d) {
        return QVariant();  // might have PTSD, depending on more info
    }
    return false;  // not PTSD
}
