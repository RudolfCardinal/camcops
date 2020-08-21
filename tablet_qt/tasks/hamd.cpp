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

#include "hamd.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::scorePhrase;
using stringfunc::standardResult;
using stringfunc::strnum;

const int N_SCORED_QUESTIONS = 17;
const int MAX_SCORE = 52;

const QString HamD::HAMD_TABLENAME("hamd");
const QString QPREFIX("q");

const QString WHICH_Q16("whichq16");
const QString Q16A("q16a");
const QString Q16B("q16b");
struct HamdQInfo {
    QString name;
    int n_options;
    bool mandatory;
};
const QVector<HamdQInfo> QLIST{
    {"q1", 5, true},
    {"q2", 5, true},
    {"q3", 5, true},
    {"q4", 3, true},
    {"q5", 3, true},
    {"q6", 3, true},
    {"q7", 5, true},
    {"q8", 5, true},
    {"q9", 5, true},
    {"q10", 5, true},
    {"q11", 5, true},
    {"q12", 3, true},
    {"q13", 3, true},
    {"q14", 3, true},
    {"q15", 5, true},
    {WHICH_Q16, 2, true},
    {Q16A, 4, true},
    {Q16B, 4, true},
    {"q17", 3, true},
    {"q18a", 3, false},
    {"q18b", 3, false},
    {"q19", 5, false},
    {"q20", 4, false},
    {"q21", 3, false},
};


void initializeHamD(TaskFactory& factory)
{
    static TaskRegistrar<HamD> registered(factory);
}


HamD::HamD(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, HAMD_TABLENAME, false, true, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    for (auto qinfo : QLIST) {
        addField(qinfo.name, QVariant::Int);
    }

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString HamD::shortname() const
{
    return "HAM-D";
}


QString HamD::longname() const
{
    return tr("Hamilton Depression Rating Scale [HDRS/HAM-D/HRSD]");
}


QString HamD::description() const
{
    return tr("21-item professional-administered depression scale commonly "
              "used for monitoring and research.");
}


// ============================================================================
// Instance info
// ============================================================================

bool HamD::isComplete() const
{
    if (valueIsNull(WHICH_Q16)) {
        return false;
    }
    for (int i = 1; i <= N_SCORED_QUESTIONS; ++i) {
        if (i == 16) {
            if (valueIsNull(whichWeightVar())) {
                return false;
            }
        } else {
            if (valueIsNull(strnum(QPREFIX, i))) {
                return false;
            }
        }
    }
    return true;
}


QStringList HamD::summary() const
{
    return QStringList{
        scorePhrase(xstring("total_score"), totalScore(), MAX_SCORE)
    };
}


QStringList HamD::detail() const
{
    const int score = totalScore();
    const QString severity =
            score > 23
            ? xstring("severity_verysevere")
            : (score >= 19
               ? xstring("severity_severe")
               : (score >= 14
                  ? xstring("severity_moderate")
                  : score >= 8 ? xstring("severity_mild")
                               : xstring("severity_none")));
    QStringList lines = completenessInfo();
    for (auto info : QLIST) {
        lines.append(fieldSummary(info.name, xstring(info.name + "_s"), " "));
    }
    lines.append("");
    lines += summary();
    lines.append(standardResult(xstring("severity"), severity));
    return lines;
}


OpenableWidget* HamD::editor(const bool read_only)
{
    QVector<QuPagePtr> pages;

    auto addpage = [this, &pages](const HamdQInfo& info) -> void {
        NameValueOptions options;
        for (int i = 0; i < info.n_options; ++i) {
            const QString name = xstring(QString("%1_option%2").arg(info.name).arg(i));
            options.append(NameValuePair(name, i));
        }
        const QString pagetitle = xstring(QString("%1_title").arg(info.name));
        const QString question = xstring(QString("%1_question").arg(info.name));
        const QString fieldname = info.name;
        QuPagePtr page((new QuPage{
            new QuText(question),
            new QuMcq(fieldRef(fieldname, info.mandatory), options),
        })->setTitle(pagetitle)->addTag(info.name));
        pages.append(page);
    };

    pages.append(getClinicianDetailsPage());
    for (const HamdQInfo& info : QLIST) {
        addpage(info);
    }

    connect(fieldRef(WHICH_Q16).data(), &FieldRef::valueChanged,
            this, &HamD::chooseWeightPage);

    m_questionnaire = new Questionnaire(m_app, pages);
    m_questionnaire->setType(QuPage::PageType::Clinician);
    m_questionnaire->setReadOnly(read_only);
    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

QString HamD::whichWeightVar(const bool other) const
{
    if (other) {
        // backwards
        return valueInt(WHICH_Q16) == 0 ? Q16B : Q16A;
    }
    return valueInt(WHICH_Q16) == 0 ? Q16A : Q16B;
}


int HamD::totalScore() const
{
    int total = 0;
    for (int i = 1; i <= N_SCORED_QUESTIONS; ++i) {
        if (i == 16) {
            // Special weight question
            const int rawscore = valueInt(whichWeightVar());
            if (rawscore != 3) {
                // For weight questions, '3' means 'not measured' and is
                // not scored.
                total += rawscore;
            }
        } else {
            total += valueInt(strnum(QPREFIX, i));
        }
    }
    return total;
}


// ============================================================================
// Signal handlers
// ============================================================================

void HamD::chooseWeightPage()
{
    if (!m_questionnaire) {
        return;
    }
    const QString weightvar = whichWeightVar();
    const QString other = whichWeightVar(true);
    m_questionnaire->setPageSkip(weightvar, false, false);
    m_questionnaire->setPageSkip(other, true);
}
