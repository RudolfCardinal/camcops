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

#include "esspri.h"
#include "common/textconst.h"
#include "common/uiconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/qugridcell.h"
#include "questionnairelib/qugridcontainer.h"
#include "questionnairelib/quslider.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::anyNull;
using mathfunc::meanOrNull;
using mathfunc::scorePhraseVariant;
using stringfunc::strnum;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_QUESTIONS = 3;
const int MAX_SCORE = 10;
const QString QPREFIX("q");


const QString Esspri::ESSPRI_TABLENAME("esspri");

void initializeEsspri(TaskFactory& factory)
{
    static TaskRegistrar<Esspri> registered(factory);
}


Esspri::Esspri(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, ESSPRI_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addFields(strseq(QPREFIX, FIRST_Q, N_QUESTIONS), QVariant::Int);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Esspri::shortname() const
{
    return "ESSPRI";
}


QString Esspri::longname() const
{
    return tr("EULAR Sjögren’s Syndrome Patient Reported Index");
}


QString Esspri::description() const
{
    return tr("A patient-reported index designed to assess the severity of "
              "symptoms in primary Sjögren’s syndrome.");
}


QStringList Esspri::fieldNames() const
{
    return strseq(QPREFIX, FIRST_Q, N_QUESTIONS);
}

// ============================================================================
// Instance info
// ============================================================================


bool Esspri::isComplete() const
{
    if (anyNull(values(fieldNames()))) {
        return false;
    }

    return true;
}


QVariant Esspri::overallScore() const
{
    const bool ignore_null = false;

    return meanOrNull(values(fieldNames()), ignore_null);
}


QStringList Esspri::summary() const
{
    return QStringList{
        scorePhraseVariant(xstring("overall_score"), overallScore(), MAX_SCORE),
    };
}


QStringList Esspri::detail() const
{
    QStringList lines = completenessInfo();
    const QString spacer = " ";
    const QString suffix = "";
    lines += fieldSummaries("q", suffix, spacer, QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();

    return lines;
}


OpenableWidget* Esspri::editor(const bool read_only)
{
    QuPagePtr page((new QuPage{})->setTitle(xstring("title_main")));

    auto slider_grid = new QuGridContainer();
    slider_grid->setExpandHorizontally(false);
    slider_grid->setFixedGrid(false);

    page->addElement(slider_grid);

    const int QUESTION_ROW_SPAN = 1;
    const int QUESTION_COLUMN_SPAN = 3;

    int row = 0;

    for (const QString& fieldname : fieldNames()) {
        QuSlider* slider = new QuSlider(fieldRef(fieldname), 0, 10, 1);
        slider->setUseDefaultTickLabels(true);
        slider->setHorizontal(true);
        slider->setBigStep(1);

        const bool can_shrink = true;
        slider->setAbsoluteLengthCm(10, can_shrink);

        slider->setTickInterval(1);
        slider->setTickLabelPosition(QSlider::TicksAbove);

        slider->setShowValue(false);
        slider->setSymmetric(true);

        slider_grid->addCell(QuGridCell(new QuText(xstring(fieldname)), row, 0,
                                        QUESTION_ROW_SPAN, QUESTION_COLUMN_SPAN));
        row++;

        slider_grid->addCell(QuGridCell(new QuText(xstring(fieldname + "_min")), row, 0));
        slider_grid->addCell(QuGridCell(slider, row, 1));
        slider_grid->addCell(QuGridCell(new QuText(xstring(fieldname + "_max")), row, 2));

        row++;

        slider_grid->addCell(QuGridCell(new QuSpacer(QSize(uiconst::BIGSPACE,
                                                           uiconst::BIGSPACE)), row, 0));

        row++;

    }

    m_questionnaire = new Questionnaire(m_app, {page});
    m_questionnaire->setType(QuPage::PageType::Patient);
    m_questionnaire->setReadOnly(read_only);

    return m_questionnaire;
}
