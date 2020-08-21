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

// By Joe Kearney, Rudolf Cardinal.

#include "ors.h"
#include "common/textconst.h"
#include "maths/mathfunc.h"
#include "lib/datetime.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/questionnairefunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/qudatetime.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qugridcontainer.h"
#include "questionnairelib/qugridcell.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/quhorizontalcontainer.h"
#include "questionnairelib/quhorizontalline.h"
#include "questionnairelib/qulineedit.h"
#include "questionnairelib/qulineeditinteger.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/quslider.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "questionnairelib/quverticalcontainer.h"
#include "tasklib/taskfactory.h"

using mathfunc::anyNullOrEmpty;
using mathfunc::sumDouble;
using mathfunc::totalScorePhrase;

const QString Ors::ORS_TABLENAME("ors");

const int SESSION_MIN = 1;
const int SESSION_MAX = 1000;

const int COMPLETED_BY_SELF = 0;
const int COMPLETED_BY_OTHER = 1;

const double VAS_MIN_FLOAT = 0;
const double VAS_MAX_FLOAT = 10;
const double VAS_ABSOLUTE_CM = 10;
const int VAS_MIN_INT = 0;
const int VAS_MAX_INT = 1000;

const double VAS_MAX_TOTAL = VAS_MAX_FLOAT * 4;

const QString FN_SESSION("q_session");
const QString FN_DATE("q_date");
const QString FN_WHOSE_GOAL("q_who");
const QString FN_WHOSE_GOAL_OTHER("q_who_other");
const QString FN_INDIVIDUAL("q_individual");
const QString FN_INTERPERSONAL("q_interpersonal");
const QString FN_SOCIAL("q_social");
const QString FN_OVERALL("q_overall");

const QString TAG_OTHER("other");


void initializeOrs(TaskFactory& factory)
{
    static TaskRegistrar<Ors> registered(factory);
}


Ors::Ors(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, ORS_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addField(FN_SESSION, QVariant::Int);
    addField(FN_DATE, QVariant::Date);
    addField(FN_WHOSE_GOAL, QVariant::Int);
    addField(FN_WHOSE_GOAL_OTHER, QVariant::String);
    addField(FN_INDIVIDUAL, QVariant::Double);
    addField(FN_INTERPERSONAL, QVariant::Double);
    addField(FN_SOCIAL, QVariant::Double);
    addField(FN_OVERALL, QVariant::Double);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.

    // Extra initialization:
    if (load_pk == dbconst::NONEXISTENT_PK) {
        setValue(FN_DATE, datetime::nowDate(), false);
    }
}


// ============================================================================
// Class info
// ============================================================================

QString Ors::shortname() const
{
    return "ORS";
}


QString Ors::longname() const
{
    return tr("Outcome Rating Scale");
}


QString Ors::description() const
{
    return tr("Fixed-length visual analogue scales measuring well-being.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Ors::isComplete() const
{
    const QStringList required_always{
        FN_SESSION,
        FN_DATE,
        FN_WHOSE_GOAL,
        FN_INDIVIDUAL,
        FN_INTERPERSONAL,
        FN_SOCIAL,
        FN_OVERALL,
    };

    if (anyNullOrEmpty(values(required_always))) {
        return false;
    }

    if (value(FN_WHOSE_GOAL).toInt() == COMPLETED_BY_OTHER &&
         valueIsNullOrEmpty(FN_WHOSE_GOAL_OTHER)) {
        return false;
    }

    return true;
}


QStringList Ors::summary() const
{
    return QStringList{
        QString("%1<b>%2</b>.").arg(xstring("session_number_q"),
                                    value(FN_SESSION).toString()),
        QString("%1: <b>%2</b>.").arg(xstring("date_q"),
                                      value(FN_DATE).toString()),
        totalScorePhrase(totalScore(), static_cast<int>(VAS_MAX_TOTAL))
    };
}


QStringList Ors::detail() const
{
    QStringList lines;
    lines.append(summary());
    lines.append("<b>Scores</b>");
    const QString vas_sep = ": ";
    lines.append(xstring("q1_title") + vas_sep + value(FN_INDIVIDUAL).toString());
    lines.append(xstring("q2_title") + vas_sep + value(FN_INTERPERSONAL).toString());
    lines.append(xstring("q3_title") + vas_sep + value(FN_SOCIAL).toString());
    lines.append(xstring("q4_title") + vas_sep + value(FN_OVERALL).toString());
    return lines;
}


OpenableWidget* Ors::editor(const bool read_only)
{
    const Qt::Alignment centre = Qt::AlignHCenter | Qt::AlignVCenter;

    m_completed_by = NameValueOptions{
       { xstring("who_a1"), COMPLETED_BY_SELF },
       { xstring("who_a2"), COMPLETED_BY_OTHER },
     };

    auto who_q = new QuMcq(fieldRef(FN_WHOSE_GOAL), m_completed_by);
    who_q->setHorizontal(true)->setAsTextButton(true);

    auto makeTitle = [this, &centre](const QString& xstringname) -> QuText* {
        return (new QuText(xstring(xstringname)))
                ->setTextAndWidgetAlignment(centre);
    };
    auto makeVAS = [this](const QString& fieldname) -> QuSlider* {
        auto slider = new QuSlider(fieldRef(fieldname),
                                   VAS_MIN_INT, VAS_MAX_INT, 1);
        slider->setConvertForRealField(true, VAS_MIN_FLOAT, VAS_MAX_FLOAT);
        slider->setAbsoluteLengthCm(VAS_ABSOLUTE_CM);
        slider->setSymmetric(true);
        slider->setNullApparentValueCentre();
        slider->setTickInterval(VAS_MAX_INT - VAS_MIN_INT);
        slider->setTickPosition(QSlider::TickPosition::TicksAbove);
        return slider;
    };

    QuPagePtr page(new QuPage{
        (new QuGridContainer{
                QuGridCell(new QuText(xstring("session_number_q")), 0, 0),
                QuGridCell(new QuLineEditInteger(fieldRef(FN_SESSION), SESSION_MIN, SESSION_MAX), 0, 1)
        })->setExpandHorizontally(false),
        (new QuGridContainer{
            QuGridCell(new QuText(xstring("date_q")), 0, 0),
            QuGridCell((new QuDateTime(fieldRef(FN_DATE)))
                           ->setMode(QuDateTime::DefaultDate)
                           ->setOfferNowButton(true), 0, 1)
        })->setExpandHorizontally(false),
        (new QuGridContainer{
            QuGridCell(new QuText(xstring("who_q")), 0, 0),
            QuGridCell(who_q, 0, 1)
        })->setExpandHorizontally(false),
        (new QuText(xstring("who_other_q")))->addTag(TAG_OTHER),
        (new QuTextEdit(fieldRef(FN_WHOSE_GOAL_OTHER), false))->addTag(TAG_OTHER),
        new QuHorizontalLine(),
        // --------------------------------------------------------------------
        // Padding
        // --------------------------------------------------------------------
        new QuSpacer(),
        new QuSpacer(),
        new QuSpacer(),
        new QuText(xstring("instructions_to_subject")),
        new QuSpacer(),
        // --------------------------------------------------------------------
        // Visual-analogue sliders
        // --------------------------------------------------------------------
        (new QuVerticalContainer{
            makeTitle("q1_title"),
            makeTitle("q1_subtitle"),
            makeVAS(FN_INDIVIDUAL),
            new QuSpacer(),
            makeTitle("q2_title"),
            makeTitle("q2_subtitle"),
            makeVAS(FN_INTERPERSONAL),
            new QuSpacer(),
            makeTitle("q3_title"),
            makeTitle("q3_subtitle"),
            makeVAS(FN_SOCIAL),
            new QuSpacer(),
            makeTitle("q4_title"),
            makeTitle("q4_subtitle"),
            makeVAS(FN_OVERALL),
         })->setContainedWidgetAlignments(centre),
        // --------------------------------------------------------------------
        // Padding
        // --------------------------------------------------------------------
        new QuSpacer(),
        new QuSpacer(),
        new QuHorizontalLine(),
        new QuSpacer(),
        // --------------------------------------------------------------------
        // Footer
        // --------------------------------------------------------------------
        (new QuVerticalContainer{
            (new QuText(xstring("copyright")))->setTextAlignment(centre),
            (new QuText(xstring("licensing")))->setTextAlignment(centre)
        })->setContainedWidgetAlignments(centre)

    });

    page->setTitle(longname());
    m_questionnaire = new Questionnaire(m_app, {page});
    m_questionnaire->setReadOnly(read_only);

    connect(fieldRef(FN_WHOSE_GOAL).data(), &FieldRef::valueChanged,
            this, &Ors::updateMandatory);

    updateMandatory();

    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

void Ors::updateMandatory()
{
   const bool required = valueInt(FN_WHOSE_GOAL) == COMPLETED_BY_OTHER;
    fieldRef(FN_WHOSE_GOAL_OTHER)->setMandatory(required);
    if (!m_questionnaire) {
        return;
    }
    m_questionnaire->setVisibleByTag(TAG_OTHER, required);
}


double Ors::totalScore() const
{
    const QStringList vas_scales{
        FN_INDIVIDUAL,
        FN_INTERPERSONAL,
        FN_SOCIAL,
        FN_OVERALL,
    };

    return sumDouble(values(vas_scales));
}
