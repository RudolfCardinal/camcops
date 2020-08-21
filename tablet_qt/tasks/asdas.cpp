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

#include "asdas.h"
#include "common/textconst.h"
#include "common/uiconst.h"
#include "maths/mathfunc.h"
#include "lib/convert.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/namevaluepair.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/qugridcell.h"
#include "questionnairelib/qugridcontainer.h"
#include "questionnairelib/qulineeditdouble.h"
#include "questionnairelib/quslider.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::anyNull;
using stringfunc::strseq;

const int FIRST_Q = 1;
const int N_SCALE_QUESTIONS = 4;
const int N_QUESTIONS = 6;
const QString QPREFIX("q");
const QString Q_CRP("q5");
const QString Q_ESR("q6");

const double CRP_MAX = 2000;
const double ESR_MAX = 300;
const int CRP_ESR_DP = 2;

const QString Asdas::ASDAS_TABLENAME("asdas");


void initializeAsdas(TaskFactory& factory)
{
    static TaskRegistrar<Asdas> registered(factory);
}


Asdas::Asdas(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, ASDAS_TABLENAME, false, false, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addFields(strseq(QPREFIX, FIRST_Q, N_SCALE_QUESTIONS), QVariant::Int);

    addField(Q_CRP, QVariant::Double);
    addField(Q_ESR, QVariant::Double);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Asdas::shortname() const
{
    return "ASDAS";
}


QString Asdas::longname() const
{
    return tr("Ankylosing Spondylitis Disease Activity Score");
}


QString Asdas::description() const
{
    return tr("An ASAS-endorsed disease activity score (ASDAS) in patients "
              "with ankylosing spondylitis.");
}


QStringList Asdas::fieldNames() const
{
    return strseq(QPREFIX, FIRST_Q, N_QUESTIONS);
}


QStringList Asdas::scaleFieldNames() const
{
    return strseq(QPREFIX, FIRST_Q, N_SCALE_QUESTIONS);
}

// ============================================================================
// Instance info
// ============================================================================

bool Asdas::isComplete() const
{
    if (anyNull(values(scaleFieldNames()))) {
        return false;
    }

    if (value(Q_CRP).isNull() && value(Q_ESR).isNull()) {
        return false;
    }

    return true;
}


double Asdas::backPain() const
{
    return value("q1").toDouble();
}


double Asdas::morningStiffness() const
{
    return value("q2").toDouble();

}


double Asdas::patientGlobal() const
{
    return value("q3").toDouble();
}


double Asdas::peripheralPain() const
{
    return value("q4").toDouble();
}


QVariant Asdas::asdasCrp() const
{
    const QVariant crp = value(Q_CRP);
    if (crp.isNull()) {
        return QVariant();
    }

    const double adjusted_crp = std::max(crp.toDouble(), 2.0);

    return 0.12 * backPain() +
        0.06 * morningStiffness() +
        0.11 * patientGlobal() +
        0.07 * peripheralPain() +
        0.58 * std::log(adjusted_crp + 1);
}


QVariant Asdas::asdasEsr() const
{
    const QVariant esr = value(Q_ESR);
    if (esr.isNull()) {
        return QVariant();
    }

    return 0.08 * backPain() +
        0.07 * morningStiffness() +
        0.11 * patientGlobal() +
        0.09 * peripheralPain() +
        0.29 * std::sqrt(esr.toDouble());
}


QString Asdas::activityState(QVariant measurement) const
{
    if (measurement.isNull()) {
        return xstring("n_a");
    }

    if (measurement < 1.3) {
        return xstring("inactive");
    }

    if (measurement < 2.1) {
        return xstring("moderate");
    }

    if (measurement > 3.5) {
        return xstring("very_high");
    }

    return xstring("high");
}


QStringList Asdas::summary() const
{
    using stringfunc::bold;

    const QVariant crp = asdasCrp();
    const QVariant esr = asdasEsr();

    return QStringList{
        QString("%1: %2 (%3)").arg(xstring("asdas_crp"),
                                   convert::prettyValue(crp, CRP_ESR_DP),
                                   bold(activityState(crp))
        ),
        QString("%1: %2 (%3)").arg(xstring("asdas_esr"),
                                   convert::prettyValue(esr, CRP_ESR_DP),
                                   bold(activityState(esr))
        ),
    };
}


QStringList Asdas::detail() const
{
    QStringList lines = completenessInfo();
    const QString spacer = " ";
    const QString suffix = "";
    lines += fieldSummaries("q", suffix, spacer, QPREFIX, FIRST_Q, N_QUESTIONS);
    lines.append("");
    lines += summary();

    return lines;
}


OpenableWidget* Asdas::editor(const bool read_only)
{
    QuPagePtr page((new QuPage{})->setTitle(xstring("title_main")));

    auto slider_grid = new QuGridContainer();
    slider_grid->setExpandHorizontally(false);
    slider_grid->setFixedGrid(false);

    page->addElement(slider_grid);

    const int QUESTION_ROW_SPAN = 1;
    const int QUESTION_COLUMN_SPAN = 3;

    int row = 0;

    for (const QString& fieldname : scaleFieldNames()) {
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

        const auto question_text = new QuText(xstring(fieldname));
        question_text->setBold(true);
        slider_grid->addCell(QuGridCell(question_text, row, 0,
                                        QUESTION_ROW_SPAN, QUESTION_COLUMN_SPAN));
        row++;

        const auto min_label = new QuText(xstring(fieldname + "_min"));
        min_label->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        const auto max_label = new QuText(xstring(fieldname + "_max"));
        slider_grid->addCell(QuGridCell(min_label, row, 0));
        slider_grid->addCell(QuGridCell(slider, row, 1));
        slider_grid->addCell(QuGridCell(max_label, row, 2));

        row++;

        slider_grid->addCell(QuGridCell(new QuSpacer(QSize(uiconst::BIGSPACE,
                                                           uiconst::BIGSPACE)), row, 0));

        row++;

    }

    const auto crp_esr_inst = new QuText(xstring("crp_esr_instructions"));
    crp_esr_inst->setBold(true);

    page->addElement(crp_esr_inst);

    page->addElement(new QuText(xstring(Q_CRP)));
    const auto crp_field = new QuLineEditDouble(
        fieldRef(Q_CRP), 0, CRP_MAX, CRP_ESR_DP
    );
    page->addElement(crp_field);

    page->addElement(new QuText(xstring(Q_ESR)));
    const auto esr_field = new QuLineEditDouble(
        fieldRef(Q_ESR), 0, ESR_MAX, CRP_ESR_DP
    );
    page->addElement(esr_field);

    connect(fieldRef(Q_CRP).data(), &FieldRef::valueChanged,
            this, &Asdas::crpChanged);

    connect(fieldRef(Q_ESR).data(), &FieldRef::valueChanged,
            this, &Asdas::esrChanged);

    crpChanged();
    esrChanged();

    m_questionnaire = new Questionnaire(m_app, {page});
    m_questionnaire->setType(QuPage::PageType::Patient);
    m_questionnaire->setReadOnly(read_only);

    return m_questionnaire;
}


void Asdas::crpChanged()
{
    const bool esrMandatory = value(Q_CRP).isNull();

    fieldRef(Q_ESR)->setMandatory(esrMandatory);
}


void Asdas::esrChanged()
{
    const bool crpMandatory = value(Q_ESR).isNull();

    fieldRef(Q_CRP)->setMandatory(crpMandatory);
}
