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

#include "srs.h"
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
#include "questionnairelib/quflowcontainer.h"
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

const QString Srs::SRS_TABLENAME("srs");

const int SESSION_MIN = 1;
const int SESSION_MAX = 1000;

const double VAS_MIN_FLOAT = 0;
const double VAS_MAX_FLOAT = 10;
const double VAS_ABSOLUTE_CM = 10;
const int VAS_MIN_INT = 0;
const int VAS_MAX_INT = 1000;

const double VAS_MAX_TOTAL = VAS_MAX_FLOAT * 4;

const QString FN_SESSION("q_session");
const QString FN_DATE("q_date");
const QString FN_RELATIONSHIP("q_relationship");
const QString FN_GOALS("q_goals");
const QString FN_APPROACH("q_approach");
const QString FN_OVERALL("q_overall");


void initializeSrs(TaskFactory& factory)
{
    static TaskRegistrar<Srs> registered(factory);
}


Srs::Srs(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, SRS_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addField(FN_SESSION, QVariant::Int);
    addField(FN_DATE, QVariant::Date);
    addField(FN_RELATIONSHIP, QVariant::Double);
    addField(FN_GOALS, QVariant::Double);
    addField(FN_APPROACH, QVariant::Double);
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

QString Srs::shortname() const
{
    return "SRS";
}


QString Srs::longname() const
{
    return tr("Session Rating Scale");
}


QString Srs::description() const
{
    return tr("Fixed-length visual analogue scales for providing "
              "psychotherapy session feedback.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Srs::isComplete() const
{
    const QStringList required_always{
        FN_SESSION,
        FN_DATE,
        FN_RELATIONSHIP,
        FN_GOALS,
        FN_APPROACH,
        FN_OVERALL,
    };
    return !anyNullOrEmpty(values(required_always));
}


QStringList Srs::summary() const
{
    return QStringList{
        QString("%1<b>%2</b>.").arg(xstring("session_number_q"),
                                    value(FN_SESSION).toString()),
        QString("%1: <b>%2</b>.").arg(xstring("date_q"),
                                      value(FN_DATE).toString()),
        totalScorePhrase(totalScore(), static_cast<int>(VAS_MAX_TOTAL))
    };
}


QStringList Srs::detail() const
{
    QStringList lines;
    lines.append(summary());
    lines.append("<b>Scores</b>");
    const QString vas_sep = ": ";
    lines.append(xstring("q1_title") + vas_sep + value(FN_RELATIONSHIP).toString());
    lines.append(xstring("q2_title") + vas_sep + value(FN_GOALS).toString());
    lines.append(xstring("q3_title") + vas_sep + value(FN_APPROACH).toString());
    lines.append(xstring("q4_title") + vas_sep + value(FN_OVERALL).toString());
    return lines;
}


OpenableWidget* Srs::editor(const bool read_only)
{
    const Qt::Alignment valign = Qt::AlignVCenter;  // normal
    // const Qt::Alignment valign = Qt::AlignTop;  // for debugging
    const Qt::Alignment centre = Qt::AlignHCenter | valign;
    const Qt::Alignment left = Qt::AlignLeft | valign;
    const Qt::Alignment right = Qt::AlignRight | valign;

    auto grid = new QuGridContainer();
    int row = 0;

    auto makeVAS = [this, &centre](const QString& fieldname) -> QuSlider* {
        auto slider = new QuSlider(fieldRef(fieldname),
                                   VAS_MIN_INT, VAS_MAX_INT, 1);
        slider->setConvertForRealField(true, VAS_MIN_FLOAT, VAS_MAX_FLOAT);
        slider->setAbsoluteLengthCm(VAS_ABSOLUTE_CM);
        slider->setSymmetric(true);
        slider->setNullApparentValueCentre();
        slider->setTickInterval(VAS_MAX_INT - VAS_MIN_INT);
        slider->setTickPosition(QSlider::TickPosition::TicksAbove);
        slider->setWidgetAlignment(centre);
        return slider;
    };
    auto addHeading = [this, &grid, &row, &centre](
            const QString& xstringname) -> void {
        grid->addCell(QuGridCell(
            (new QuText(xstring(xstringname)))->setTextAndWidgetAlignment(centre),
            row++, 0, 1, 3, centre, false));
    };
    auto addVas = [this, &grid, &row, &centre, &left, &right](
            const QString& leftstring,
            QuSlider* vas,
            const QString& rightstring) -> void {
        grid->addCell(QuGridCell(
            (new QuText(xstring(leftstring)))->setTextAndWidgetAlignment(right),
            row, 0, 1, 1, centre, false));
        grid->addCell(QuGridCell(vas, row, 1));
        grid->addCell(QuGridCell(
            (new QuText(xstring(rightstring)))->setTextAndWidgetAlignment(left),
            row, 2, 1, 1, centre, false));
        ++row;
    };
    auto addSpacer = [&grid, &row]() -> void {
        grid->addCell(QuGridCell(new QuSpacer(), row++, 1));
    };

    auto vas_relationship = makeVAS(FN_RELATIONSHIP);
    auto vas_goals = makeVAS(FN_GOALS);
    auto vas_approach = makeVAS(FN_APPROACH);
    auto vas_overall = makeVAS(FN_OVERALL);

    grid->setColumnStretch(0, 1);  // text; expand equally to column 2
    grid->setColumnStretch(1, 0);  // VAS; don't expand beyond what's necessary
    grid->setColumnStretch(2, 1);  // text; expand equally to column 0

    addHeading("q1_title");
    addVas("q1_left", vas_relationship, "q1_right");
    addSpacer();
    addHeading("q2_title");
    addVas("q2_left", vas_goals, "q2_right");
    addSpacer();
    addHeading("q3_title");
    addVas("q3_left", vas_approach, "q3_right");
    addSpacer();
    addHeading("q4_title");
    addVas("q4_left", vas_overall, "q4_right");

    // qDebug() << "grid:" << *grid;

    QuPagePtr page(new QuPage{
        (new QuGridContainer{
                QuGridCell(new QuText(xstring("session_number_q")), 0, 0),
                QuGridCell(new QuLineEditInteger(fieldRef(FN_SESSION),
                           SESSION_MIN, SESSION_MAX), 0, 1)
        })->setExpandHorizontally(false),
        (new QuGridContainer{
            QuGridCell(new QuText(xstring("date_q")), 0, 0),
            QuGridCell((new QuDateTime(fieldRef(FN_DATE)))
                           ->setMode(QuDateTime::DefaultDate)
                           ->setOfferNowButton(true), 0, 1)
        })->setExpandHorizontally(false),
        new QuHorizontalLine(),
        // --------------------------------------------------------------------
        // Padding
        // --------------------------------------------------------------------
        new QuSpacer(),
        (new QuText(xstring("instructions_to_subject")))
                       ->setItalic()
                       ->setTextAndWidgetAlignment(centre),
        new QuSpacer(),
        new QuHorizontalLine(),
        new QuSpacer(),
        // --------------------------------------------------------------------
        // Visual-analogue sliders
        // --------------------------------------------------------------------
        grid,
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

    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

double Srs::totalScore() const
{
    const QStringList vas_scales{
        FN_RELATIONSHIP,
        FN_GOALS,
        FN_APPROACH,
        FN_OVERALL,
    };

    return sumDouble(values(vas_scales));
}
