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

#include "satisfactioncommon.h"
#include "common/appstrings.h"
#include "core/camcopsapp.h"
#include "common/textconst.h"
#include "common/varconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
using stringfunc::standardResult;
using stringfunc::strnum;

const QString SERVICE("service");
const QString RATING("rating");
const QString GOOD("good");
const QString BAD("bad");


SatisfactionCommon::SatisfactionCommon(CamcopsApp& app,
                                       DatabaseManager& db,
                                       const QString& tablename,
                                       const bool anonymous,
                                       const int load_pk) :
    Task(app, db, tablename, anonymous, false, false)  // ... anon, clin, resp
{
    addField(SERVICE, QVariant::String);
    addField(RATING, QVariant::Int);
    addField(GOOD, QVariant::String);
    addField(BAD, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Instance info
// ============================================================================

bool SatisfactionCommon::isComplete() const
{
    return !valueIsNull(RATING);
}


QStringList SatisfactionCommon::summary() const
{
    return QStringList{standardResult(TextConst::rating(), getRatingText())};
}


QStringList SatisfactionCommon::detail() const
{
    QStringList lines = completenessInfo() + summary();
    lines.append(fieldSummary(SERVICE,
                              appstring(appstrings::SATIS_SERVICE_BEING_RATED)));
    lines.append(fieldSummary(GOOD, appstring(appstrings::SATIS_GOOD_S)));
    lines.append(fieldSummary(BAD, appstring(appstrings::SATIS_BAD_S)));
    return lines;
}


void SatisfactionCommon::setDefaultsAtFirstUse()
{
    setValue(SERVICE, m_app.varString(varconst::DEFAULT_CLINICIAN_SERVICE));
}


OpenableWidget* SatisfactionCommon::satisfactionEditor(const QString& rating_q,
                                                       const bool read_only)
{
    NameValueOptions options;
    for (int i = 4; i >= 0; --i) {
        options.append(NameValuePair(
            appstring(strnum(appstrings::SATIS_RATING_A_PREFIX, i)),
            i));
    }

    QuPagePtr page((new QuPage{
        (new QuText(rating_q + " " + valueString(SERVICE) + "?"))->setBold(),
        new QuMcq(fieldRef(RATING), options),
        new QuText(appstring(appstrings::SATIS_GOOD_Q)),
        new QuTextEdit(fieldRef(GOOD, false)),
        new QuText(appstring(appstrings::SATIS_BAD_Q)),
        new QuTextEdit(fieldRef(BAD, false)),
    })->setTitle(longname()));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

QString SatisfactionCommon::getRatingText() const
{
    const QVariant rating = value(RATING);
    if (rating.isNull() || rating.toInt() < 0 || rating.toInt() > 4) {
        return "";
    }
    return appstring(strnum(appstrings::SATIS_RATING_A_PREFIX,
                            rating.toInt()));
}
