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

#include "fft.h"
#include "core/camcopsapp.h"
#include "common/textconst.h"
#include "common/varconst.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using stringfunc::strnum;
using stringfunc::standardResult;

const QString Fft::FFT_TABLENAME("fft");

const QString SERVICE("service");  // the service being rated
const QString RATING("rating");


void initializeFft(TaskFactory& factory)
{
    static TaskRegistrar<Fft> registered(factory);
}


Fft::Fft(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, FFT_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addField(SERVICE, QVariant::String);
    addField(RATING, QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.

    // Extra initialization:
    if (load_pk == dbconst::NONEXISTENT_PK) {
        setValue(SERVICE,
                 m_app.varString(varconst::DEFAULT_CLINICIAN_SERVICE),
                 false);
    }
}


// ============================================================================
// Class info
// ============================================================================

QString Fft::shortname() const
{
    return "FFT";
}


QString Fft::longname() const
{
    return tr("Friends and Family Test");
}


QString Fft::description() const
{
    return tr("Single-question patient rating of a clinical service.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Fft::isComplete() const
{
    return !valueIsNull(RATING);
}


QStringList Fft::summary() const
{
    return QStringList{
        fieldSummary(SERVICE, TextConst::service(), ": ", "."),
        standardResult(TextConst::rating(), ratingText()),
    };
}


QStringList Fft::detail() const
{
    return completenessInfo() + summary();
}


OpenableWidget* Fft::editor(const bool read_only)
{
    NameValueOptions options;
    for (int i = 1; i <= 6; ++i) {
        options.append(NameValuePair(xstring(strnum("a", i)), i));
    }

    QuPagePtr page((new QuPage{
        (new QuText(valueString(SERVICE)))->setBold(),
        (new QuText(xstring("q")))->setBold(),
        new QuMcq(fieldRef(RATING), options),
    })->setTitle(longname()));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

QString Fft::ratingText() const
{
    const QVariant rating_var = value(RATING);
    if (rating_var.isNull()) {
        return "";
    }
    const int rating = rating_var.toInt();
    if (rating < 1 || rating > 6) {
        return "";
    }
    return xstring(strnum("a", rating));
}
