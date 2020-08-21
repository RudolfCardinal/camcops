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

#include "apeqcpftperinatal.h"
#include "lib/stringfunc.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
using stringfunc::strnum;
using stringfunc::strseq;


const QString APEQCPFTPerinatal::APEQCPFTPERINATAL_TABLENAME("apeq_cpft_perinatal");

const int FIRST_MAIN_Q = 1;
const int LAST_MAIN_Q = 6;
const QString FN_QPREFIX("q");

const QString FN_Q_FF_RATING("ff_rating");
const QString FN_Q_FF_WHY("ff_why");
const QString FN_Q_COMMENTS("comments");

const QString QA_FORMAT("%1 <b>%2</b>");
const QString XSTRING_Q_PREFIX("q");
const QString XSTRING_MAIN_A_PREFIX("main_a");
const QString XSTRING_FF_A_PREFIX("ff_a");
const QString XSTRING_Q_FF_RATING("q_ff_rating");
const QString XSTRING_Q_FF_WHY("q_ff_why");
const QString XSTRING_Q_COMMENTS("q_comments");
const QString MISSING("?");


void initializeAPEQCPFTPerinatal(TaskFactory& factory)
{
    static TaskRegistrar<APEQCPFTPerinatal> registered(factory);
}


APEQCPFTPerinatal::APEQCPFTPerinatal(CamcopsApp& app, DatabaseManager& db,
                                     const int load_pk) :
    Task(app, db, APEQCPFTPERINATAL_TABLENAME, true, false, false) // ... anon, clin, resp
{
    for (const QString& field : strseq(FN_QPREFIX, FIRST_MAIN_Q, LAST_MAIN_Q)) {
        addField(field, QVariant::Int);
    }
    addField(FN_Q_FF_RATING, QVariant::Int);
    addField(FN_Q_FF_WHY, QVariant::String);
    addField(FN_Q_COMMENTS, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString APEQCPFTPerinatal::shortname() const
{
    return "APEQ-CPFT-Perinatal";
}


QString APEQCPFTPerinatal::longname() const
{
    return tr("Assessment Patient Experience Questionnaire for "
              "CPFT Perinatal Services");
}


QString APEQCPFTPerinatal::description() const
{
    return tr("Patient feedback questionnaire on assessment by perinatal "
              "services at Cambridgeshire & Peterborough NHS Foundation Trust.");
}

// ============================================================================
// Instance info
// ============================================================================

bool APEQCPFTPerinatal::isComplete() const
{
    QStringList required_always{FN_Q_FF_RATING};
    for (const QString& field : strseq(FN_QPREFIX, FIRST_MAIN_Q, LAST_MAIN_Q)) {
        required_always.append(field);
    }
    return noValuesNullOrEmpty(required_always);
}


NameValueOptions APEQCPFTPerinatal::optionsMain() const
{
    NameValueOptions options;
    for (int a = 2; a >= 0; --a) {
        options.append(NameValuePair(
                           xstring(strnum(XSTRING_MAIN_A_PREFIX, a)), a));
    }
    return options;
}


NameValueOptions APEQCPFTPerinatal::optionsFFRating() const
{
    NameValueOptions options;
    for (int a = 5; a >= 0; --a) {
        options.append(NameValuePair(
                           xstring(strnum(XSTRING_FF_A_PREFIX, a)), a));
    }
    return options;
}


QStringList APEQCPFTPerinatal::summary() const
{
    const NameValueOptions options_ff_rating = optionsFFRating();
    return QStringList{
        QA_FORMAT.arg(
            xstring(XSTRING_Q_FF_RATING),
            options_ff_rating.nameFromValue(value(FN_Q_FF_RATING), MISSING)),
    };
}


QStringList APEQCPFTPerinatal::detail() const
{
    const NameValueOptions options_main = optionsMain();
    const NameValueOptions options_ff_rating = optionsFFRating();
    QStringList lines = completenessInfo();
    QString xstringname, fieldname;
    for (int q = FIRST_MAIN_Q; q <= LAST_MAIN_Q; ++q) {
        fieldname = xstringname = strnum(FN_QPREFIX, q);
        lines.append(QA_FORMAT.arg(
                         xstring(xstringname),
                         options_main.nameFromValue(value(fieldname), MISSING)));
    }
    lines.append(QA_FORMAT.arg(
                     xstring(XSTRING_Q_FF_RATING),
                     options_ff_rating.nameFromValue(value(FN_Q_FF_RATING),
                                                     MISSING)));
    lines.append(QA_FORMAT.arg(
                     xstring(XSTRING_Q_FF_WHY),
                     valueString(FN_Q_FF_WHY)));
    lines.append(QA_FORMAT.arg(
                     xstring(XSTRING_Q_COMMENTS),
                     valueString(FN_Q_COMMENTS)));
    return lines;
}


OpenableWidget* APEQCPFTPerinatal::editor(const bool read_only)
{
    const NameValueOptions options_main = optionsMain();
    const NameValueOptions options_ff_rating = optionsFFRating();

    auto makeInstruction = [this](const QString& xstringname) -> QuText* {
        return (new QuText(xstring(xstringname)))->setItalic()->setBig();
    };
    auto makeInfo = [this](const QString& xstringname) -> QuText* {
        return (new QuText(xstring(xstringname)))->setItalic()->setBig();
    };
    auto makeQuestion = [this](const QString& xstringname) -> QuText* {
        return (new QuText(xstring(xstringname)))->setBold();
    };
    auto makeMCQ = [this](const QString& fieldname,
                          const NameValueOptions& options) -> QuMcq* {
        QuMcq* mcq = new QuMcq(fieldRef(fieldname), options);
        mcq->setHorizontal(true);
        mcq->setAsTextButton(true);
        return mcq;
    };
    auto makeTextEdit = [this](const QString& fieldname) -> QuTextEdit* {
        QuTextEdit* t = new QuTextEdit(fieldRef(fieldname, false));
        return t;
    };
    QVector<QuElement*> elements{
        makeInstruction("instructions_1"),
        makeInstruction("instructions_2"),
    };
    for (int q = FIRST_MAIN_Q; q <= LAST_MAIN_Q; ++q) {
        elements.append(new QuSpacer());
        elements.append(makeQuestion(strnum(XSTRING_Q_PREFIX, q)));
        elements.append(makeMCQ(strnum(FN_QPREFIX, q), options_main));
    }
    elements.append(new QuSpacer());
    elements.append(makeQuestion(XSTRING_Q_FF_RATING));
    elements.append(makeMCQ(FN_Q_FF_RATING, options_ff_rating));
    elements.append(new QuSpacer());
    elements.append(makeQuestion(XSTRING_Q_FF_WHY));
    elements.append(makeTextEdit(FN_Q_FF_WHY));
    elements.append(new QuSpacer());
    elements.append(makeQuestion(XSTRING_Q_COMMENTS));
    elements.append(makeTextEdit(FN_Q_COMMENTS));
    elements.append(new QuSpacer());
    elements.append(makeInfo("thanks"));

    QuPage* page = new QuPage(elements);
    page->setTitle(longname());

    Questionnaire* questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}
