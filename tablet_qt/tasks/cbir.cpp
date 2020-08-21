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

#include "cbir.h"
#include "common/textconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgriddouble.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
using mathfunc::noneNull;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 45;

const QString CbiR::CBIR_TABLENAME("cbir");

const QString FN_FREQ_PREFIX("frequency");
const QString FN_DISTRESS_PREFIX("distress");
const QString FN_CONFIRM_BLANKS("confirm_blanks");
const QString FN_COMMENTS("comments");

const QString TAG_MAIN("m");
const QString TAG_BLANKS("b");


void initializeCbiR(TaskFactory& factory)
{
    static TaskRegistrar<CbiR> registered(factory);
}


CbiR::CbiR(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, CBIR_TABLENAME, false, false, true),  // ... anon, clin, resp
    m_confirmation_fr(nullptr)
{
    addFields(strseq(FN_FREQ_PREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);
    addFields(strseq(FN_DISTRESS_PREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);
    addField(FN_CONFIRM_BLANKS, QVariant::Bool);
    addField(FN_COMMENTS, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString CbiR::shortname() const
{
    return "CBI-R";
}


QString CbiR::longname() const
{
    return tr("Cambridge Behavioural Inventory, Revised");
}


QString CbiR::description() const
{
    return tr("45-item caregiver rating scale, applicable to dementias.");
}


// ============================================================================
// Instance info
// ============================================================================

bool CbiR::isComplete() const
{
    return isRespondentComplete() && isCompleteQuestions();
}


QStringList CbiR::summary() const
{
    return QStringList{respondentRelationship().toString()};
}


QStringList CbiR::detail() const
{
    return completenessInfo() + summary() +
            QStringList{TextConst::noDetailSeeFacsimile()};
}


OpenableWidget* CbiR::editor(const bool read_only)
{
    const NameValueOptions freq_options{
        {xstring("f0"), 0},
        {xstring("f1"), 1},
        {xstring("f2"), 2},
        {xstring("f3"), 3},
        {xstring("f4"), 4},
    };
    const NameValueOptions distress_options{
        {xstring("d0"), 0},
        {xstring("d1"), 1},
        {xstring("d2"), 2},
        {xstring("d3"), 3},
        {xstring("d4"), 4},
    };
    const QString basetitle = shortname();
    m_data_frs.clear();

    auto addblock =
            [this, &freq_options, &distress_options]
            (QuPagePtr page, const QString& xstringname, int first_q, int last_q)
            -> void {
        QVector<QuestionWithTwoFields> qfields;
        for (int q = first_q; q <= last_q; ++q) {
            FieldRefPtr fr_freq = fieldRef(strnum(FN_FREQ_PREFIX, q));
            FieldRefPtr fr_distress = fieldRef(strnum(FN_DISTRESS_PREFIX, q));
            connect(fr_freq.data(), &FieldRef::valueChanged,
                    this, &CbiR::dataChanged);
            connect(fr_distress.data(), &FieldRef::valueChanged,
                    this, &CbiR::dataChanged);
            m_data_frs.append(fr_freq);
            m_data_frs.append(fr_distress);
            qfields.append(QuestionWithTwoFields(xstring(strnum("q", q)),
                                                 fr_freq, fr_distress));
        }
        page->addElement((new QuMcqGridDouble(
            qfields, freq_options, distress_options))
                         ->setTitle(xstring(xstringname))
                         ->setStems(xstring("stem_frequency"),
                                    xstring("stem_distress")));
    };

    QuPagePtr page1((new QuPage{
        (new QuText(xstring("for_carer")))->setItalic(),
        getRespondentQuestionnaireBlockRawPointer(true),
    })->setTitle(basetitle + " (1/3)"));

    m_confirmation_fr = fieldRef(FN_CONFIRM_BLANKS);
    QuPagePtr page2((new QuPage{
        new QuText(xstring("instruction_1")),
        new QuText(xstring("instruction_2")),
        new QuText(xstring("instruction_3")),
    })->setTitle(basetitle + " (2/3)"));
    addblock(page2, "h_memory", 1, 8);
    addblock(page2, "h_everyday", 9, 13);
    addblock(page2, "h_selfcare", 14, 17);
    addblock(page2, "h_abnormalbehaviour", 18, 23);
    addblock(page2, "h_mood", 24, 27);
    addblock(page2, "h_beliefs", 28, 30);
    addblock(page2, "h_eating", 31, 34);
    addblock(page2, "h_sleep", 35, 36);
    addblock(page2, "h_stereotypy_motor", 37, 40);
    addblock(page2, "h_motivation", 41, 45);
    page2->addElement((new QuText(xstring("confirmblanks_q")))->setBold());
    page2->addElement(new QuBoolean(xstring("confirmblanks_a"),
                                    m_confirmation_fr));
    connect(m_confirmation_fr.data(), &FieldRef::valueChanged,
            this, &CbiR::confirmationChanged);

    QuPagePtr page3((new QuPage{
        (new QuTextEdit(fieldRef(FN_COMMENTS, false)))
                ->setHint(xstring("comments")),
        (new QuText(xstring("thanks")))->setBold(),
    })->setTitle(basetitle + " (3/3)"));

    auto questionnaire = new Questionnaire(
                m_app, {page1, page2, page3});
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    confirmationChanged();
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

bool CbiR::isCompleteQuestions() const
{
    return valueBool(FN_CONFIRM_BLANKS) ||
            (noneNull(values(strseq(FN_FREQ_PREFIX, FIRST_Q, N_QUESTIONS))) &&
            noneNull(values(strseq(FN_DISTRESS_PREFIX, FIRST_Q, N_QUESTIONS))));
}


// ============================================================================
// Signal handlers
// ============================================================================

void CbiR::dataChanged()
{
    Q_ASSERT(m_confirmation_fr);
    m_confirmation_fr->setMandatory(!dataComplete());
}


void CbiR::confirmationChanged()
{
    Q_ASSERT(m_confirmation_fr);
    const bool need_data = !m_confirmation_fr->valueBool();
    for (const FieldRefPtr& fr : m_data_frs) {
        fr->setMandatory(need_data);
    }
}


bool CbiR::dataComplete() const
{
    for (const FieldRefPtr& fr : m_data_frs) {
        if (!fr->complete()) {
            return false;
        }
    }
    return true;
}
