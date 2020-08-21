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

#include "qolbasic.h"
#include "lib/convert.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quslider.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::meanOrNull;
using mathfunc::noneNull;
using stringfunc::standardResult;


const QString QolBasic::QOLBASIC_TABLENAME("qolbasic");
const QString TTO("tto");
const QString RS("rs");

const int DP_TTO = 2;
const int DP_RS = 2;
const int DP_MEAN = 3;


void initializeQolBasic(TaskFactory& factory)
{
    static TaskRegistrar<QolBasic> registered(factory);
}


QolBasic::QolBasic(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, QOLBASIC_TABLENAME, false, false, false)  // ... anon, clin, resp
{
    addField(TTO, QVariant::Double);
    addField(RS, QVariant::Double);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString QolBasic::shortname() const
{
    return "QoL-Basic";
}


QString QolBasic::longname() const
{
    return tr("Quality of Life: basic assessment");
}


QString QolBasic::description() const
{
    return tr("Time trade-off and response scale measures of quality of life.");
}


QString QolBasic::infoFilenameStem() const
{
    return "qol";
}


// ============================================================================
// Instance info
// ============================================================================

bool QolBasic::isComplete() const
{
    return noneNull(values({TTO, RS}));
}


QStringList QolBasic::summary() const
{
    return QStringList{
        standardResult(xstring("tto_q_s"),
                       convert::prettyValue(qolTto(), DP_TTO)),
        standardResult(xstring("rs_q_s"),
                       convert::prettyValue(qolRs(), DP_RS)),
        standardResult(xstring("mean_qol"),
                       convert::prettyValue(meanQol(), DP_MEAN)),
    };
}


QStringList QolBasic::detail() const
{
    return completenessInfo() + summary();
}


OpenableWidget* QolBasic::editor(const bool read_only)
{
    // The TTO slider goes from 0-10 in steps of 0.1
    // QuSlider uses integers internally but can scale.
    // So we say:
    // (a) use 0-100 in steps of 1 internally
    // (b) scale for real output of 0-10 with 1dp
    QuSlider* tto_slider = new QuSlider(fieldRef(TTO), 0, 100, 1);
    tto_slider->setConvertForRealField(true, 0, 10.0, 1);
    tto_slider->setBigStep(10);
    tto_slider->setTickInterval(10);
    tto_slider->setNullApparentValue(0);  // debatable: 0? 50?
    tto_slider->setHorizontal(true);
    tto_slider->setShowValue(true);
    tto_slider->setTickLabels({
        {0, "0"},
        {10, "1"},
        {20, "2"},
        {30, "3"},
        {40, "4"},
        {50, "5"},
        {60, "6"},
        {70, "7"},
        {80, "8"},
        {90, "9"},
        {100, "10"},
    });
    tto_slider->setEdgeInExtremeLabels(false);
    tto_slider->setTickPosition(QSlider::TicksBothSides);
    tto_slider->setTickLabelPosition(QSlider::TicksBelow);

    // The RS slider goes from 0-100 in steps of 1.
    QuSlider* rs_slider = new QuSlider(fieldRef(RS), 0, 100, 1);
    rs_slider->setBigStep(10);
    rs_slider->setTickInterval(10);
    rs_slider->setNullApparentValue(0);  // debatable: 0? 50?
    rs_slider->setHorizontal(true);
    rs_slider->setShowValue(true);
    rs_slider->setTickLabels({
        {0, xstring("rs_0")},
        {100, xstring("rs_100")},
    });
    rs_slider->setTickInterval(100);  // just 0 and 100
    rs_slider->setEdgeInExtremeLabels(true);
    rs_slider->setTickPosition(QSlider::TicksBothSides);
    rs_slider->setTickLabelPosition(QSlider::TicksBelow);

    QuPagePtr page1((new QuPage{
        new QuText(xstring("tto_q")),
        tto_slider,
    })->setTitle(xstring("tto_title")));

    QuPagePtr page2((new QuPage{
         new QuText(xstring("rs_q")),
         rs_slider,
    })->setTitle(xstring("rs_title")));

    auto questionnaire = new Questionnaire(m_app, {page1, page2});
    questionnaire->setType(QuPage::PageType::Patient);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

QVariant QolBasic::qolTto() const
{
    const QVariant tto = value(TTO);
    return tto.isNull() ? tto : tto.toDouble() / 10.0;
}


QVariant QolBasic::qolRs() const
{
    const QVariant rs = value(RS);
    return rs.isNull() ? rs : rs.toDouble() / 100.0;
}


QVariant QolBasic::meanQol() const
{
    const QVariant tto = qolTto();
    const QVariant rs = qolRs();
    return meanOrNull({tto, rs}, true);
}
