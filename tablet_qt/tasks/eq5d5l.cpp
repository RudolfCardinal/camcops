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

#include "eq5d5l.h"

#include "lib/uifunc.h"
#include "lib/stringfunc.h"
#include "lib/version.h"
#include "maths/mathfunc.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/quflowcontainer.h"
#include "questionnairelib/qugridcell.h"
#include "questionnairelib/qugridcontainer.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/quhorizontalcontainer.h"
#include "questionnairelib/qulineeditinteger.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/quthermometer.h"
#include "questionnairelib/quverticalcontainer.h"
#include "tasklib/taskfactory.h"

const QString Eq5d5l::EQ5D5L_TABLENAME("eq5d5l");

const QString QPREFIX("q");
const QString OPT_PREFIX("o");

const QString VAS_QUESTION("health_vas");

const int FIRST_Q = 1;
const int LAST_Q = 5;

using mathfunc::noneNull;
using stringfunc::strnum;
using stringfunc::strseq;


void initializeEq5d5l(TaskFactory& factory)
{
    static TaskRegistrar<Eq5d5l> registered(factory);
}


Eq5d5l::Eq5d5l(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, EQ5D5L_TABLENAME, false, false, false),
    m_in_tickbox_change(false)
{
    addFields(strseq(QPREFIX, FIRST_Q, LAST_Q), QVariant::Int);
    addField(VAS_QUESTION, QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Eq5d5l::shortname() const
{
    return "EQ-5D-5L";
}


QString Eq5d5l::longname() const
{
    return tr("EuroQol 5-Dimension, 5-Level Health Scale");
}


QString Eq5d5l::description() const
{
    return tr("Self-rated health scale; 5 questions plus a visual analogue scale.");
}


Version Eq5d5l::minimumServerVersion() const
{
    return Version(2, 2, 8);
}


// ============================================================================
// Instance info
// ============================================================================

QStringList Eq5d5l::summary() const
{
    return QStringList{
        tr("Health state code: %1").arg(getHealthStateCode()),
        tr("Visual analogue health: %1").arg(prettyValue(VAS_QUESTION)),
    };
}


QStringList Eq5d5l::detail() const
{
    QStringList lines = completenessInfo() + summary();
    lines.append("");

    for (int qnum = FIRST_Q; qnum <= LAST_Q; ++qnum) {
        const QString& fieldname = strnum(QPREFIX, qnum);
        const QString istr = QString::number(qnum);
        const QString qcat = xstring(QString("q%1_h").arg(qnum));
        const QString line = stringfunc::standardResult(
            QString("Q%1 (%2)").arg(istr, qcat),
            prettyValue(fieldname)
        );
        lines.append(line);
    }

    lines += completenessInfo();
    return lines;
}


QString Eq5d5l::getHealthStateCode() const
{
    QString descriptive = "";
    for (const QString& field : strseq(QPREFIX, FIRST_Q, LAST_Q)) {
        const QVariant v = value(field);
        descriptive += v.isNull() ? "9" : QString::number(v.toInt());
    }

    return descriptive;
}


bool Eq5d5l::isComplete() const
{
    return noneNull(values(strseq(QPREFIX, FIRST_Q, LAST_Q))) &&
            !value(VAS_QUESTION).isNull();
}


OpenableWidget* Eq5d5l::editor(const bool read_only)
{
    QVector<QuPagePtr> pages;
    QVector<QuElement*> elements;
    const QString sname = shortname();

    for (auto field : strseq(QPREFIX, FIRST_Q, LAST_Q)) {
        const QString heading = QString("%1_h").arg(field);
        const QString qoptprefix = QString("%1_%2").arg(field, OPT_PREFIX);

        NameValueOptions options{
            {xstring(qoptprefix + "1"), 1},
            {xstring(qoptprefix + "2"), 2},
            {xstring(qoptprefix + "3"), 3},
            {xstring(qoptprefix + "4"), 4},
            {xstring(qoptprefix + "5"), 5},
        };

        elements.append({
            (new QuText(xstring("t1_instruction")))->setBold(),
            new QuMcq(fieldRef(field), options)
        });

        const QString xheading = xstring(heading);
        pages.append(QuPagePtr(
            (new QuPage(elements))
                ->setTitle(QString("%1: %2").arg(sname, xheading))
                ->setIndexTitle(xheading)
        ));

        elements.clear();
    }
    QVector<QuThermometerItem> items;

    const QString resource_prefix("eq5d5lslider/");
    QString resource;

    items.append(QuThermometerItem(
        uifunc::resourceFilename(resource_prefix + "base_sel.png"),
        uifunc::resourceFilename(resource_prefix + "base_unsel.png"),
        "0", 0
    ));

    QString itemtext;
    for (int i = 1; i < 100; ++i) {

        if (i % 5 == 0) {
            // larger ticks with numbers every 5
            resource = resource_prefix + "mid%1.png";
            itemtext = QString::number(i);
        } else {
            resource = resource_prefix  + "tick%1.png";
            itemtext = "";
        }

        QuThermometerItem item(
            uifunc::resourceFilename(resource.arg("_sel")),  // active
            uifunc::resourceFilename(resource.arg("_unsel")),  // inactive
            itemtext,  // text
            i  // value
        );

        items.append(item);
    }

    items.append(QuThermometerItem(
        uifunc::resourceFilename(resource_prefix + "top_sel.png"),
        uifunc::resourceFilename(resource_prefix + "top_unsel.png"),
        "100", 100
    ));

    QVector<QuElementPtr> instructions;
    for (int i = 1; i <= 5; ++i) {
        instructions.append(
           QuElementPtr(
            (new QuText(xstring(strnum("t2_i", i))))
                ->setBig()
           )
        );
        instructions.append(QuElementPtr(new QuSpacer));
    }

    FieldRefPtr fr_vas = fieldRef(VAS_QUESTION);

    instructions.append(QuElementPtr(new QuSpacer));
    instructions.append(QuElementPtr(
        new QuHorizontalContainer{
            (new QuText(tr("YOUR HEALTH TODAY =")))->setBig(),
            new QuLineEditInteger(fr_vas, 0, 100)
        }
    ));

    const QString xtherm = xstring("t2_h");

    auto therm = new QuThermometer(fr_vas, items);
    // ... will be owned by the grid when inserted;
    therm->setRescale(true, 0.4, true);

    const bool allow_scroll = false;
    const bool zoomable = false;

    // Just having a non-scrolling page doesn't prevent the thermometer from
    // getting too big vertically and disappearing... until Martin's fix
    // capping Thermometer::heightForWidth(). Then it works very well.
    //
    // Using "zoomable" works fine, but can make the text a bit small (since
    // the zoom is applied to both the text and the thermometer).
    // An attempt via QuZoomContainer hasn't been very successful so far.

    pages.append(QuPagePtr(
        (new QuPage{
            (new QuGridContainer{
                QuGridCell(
                    new QuVerticalContainer{instructions},
                    0, 0, 1, 1, Qt::AlignLeft | Qt::AlignTop),
                QuGridCell(therm,
                           0, 1, 1, 1, Qt::AlignHCenter | Qt::AlignTop)
            })
                // For equal column widths:
             ->setFixedGrid(true)
             ->setExpandHorizontally(true)
        })->setTitle(QString("%1: %2").arg(sname, xtherm))
          ->setIndexTitle(xtherm)
          ->allowScroll(allow_scroll, zoomable)
    ));

    auto questionnaire = new Questionnaire(m_app, pages);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}
