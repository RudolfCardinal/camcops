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

#include "das28.h"
#include "common/textconst.h"
#include "common/uiconst.h"
#include "maths/mathfunc.h"
#include "lib/convert.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/qubutton.h"
#include "questionnairelib/qugridcell.h"
#include "questionnairelib/qugridcontainer.h"
#include "questionnairelib/qulineeditdouble.h"
#include "questionnairelib/qulineeditinteger.h"
#include "questionnairelib/quslider.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::anyNull;
using mathfunc::sumInt;
using stringfunc::strseq;

const QStringList SIDES = {"left", "right"};
const QStringList STATES = {"swollen", "tender"};

const int CRP_MIN = 0;
const int CRP_MAX = 300;
const int ESR_MIN = 1;
const int ESR_MAX = 300;

const int CRP_ESR_DP = 2;

const int GRID_ROW_SPAN = 1;
const int GRID_JOINT_COLUMN_SPAN = 3;
const int GRID_SIDE_COLUMN_SPAN = 2;
const int GRID_STATE_COLUMN_SPAN = 1;

const QString Das28::DAS28_TABLENAME("das28");
const QString FN_VAS("vas");
const QString FN_CRP("crp");
const QString FN_ESR("esr");


void initializeDas28(TaskFactory& factory)
{
    static TaskRegistrar<Das28> registered(factory);
}


Das28::Das28(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, DAS28_TABLENAME, false, true, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addFields(getJointFieldNames(), QVariant::Bool);

    addField(FN_VAS, QVariant::Int);
    addField(FN_CRP, QVariant::Double);
    addField(FN_ESR, QVariant::Double);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Das28::shortname() const
{
    return "DAS28";
}


QString Das28::longname() const
{
    return tr("Disease Activity Score-28");
}


QString Das28::description() const
{
    return tr("A measure of disease activity in rheumatoid arthritis "
              "(joint examination, inflammatory marker, visual analogue scale)");
}


QStringList Das28::getJointFieldNames() const
{
    QStringList field_names;

    for (const QString& joint : getJointNames()) {
        for (const QString& side : SIDES) {
            for (const QString& state : STATES) {
                field_names.append(QString("%1_%2_%3").arg(side, joint, state));
            }
        }
    }

    return field_names;
}


QStringList Das28::getSwollenFieldNames() const
{
    QStringList field_names;

    for (const QString& joint : getJointNames()) {
        for (const QString& side : SIDES) {
            field_names.append(QString("%1_%2_swollen").arg(side, joint));
        }
    }

    return field_names;
}


QStringList Das28::getTenderFieldNames() const
{
    QStringList field_names;

    for (const QString& joint : getJointNames()) {
        for (const QString& side : SIDES) {
            field_names.append(QString("%1_%2_tender").arg(side, joint));
        }
    }

    return field_names;
}


QStringList Das28::getJointNames() const
{
    auto names = QStringList({"shoulder", "elbow", "wrist"});

    for (int i = 1; i <= 5; i++) {
        names.append(QString("mcp_%1").arg(i));
    }

    for (int i = 1; i <= 5; i++) {
        names.append(QString("pip_%1").arg(i));
    }

    names.append("knee");

    return names;
}


QStringList Das28::fieldNames() const
{
    return getJointFieldNames() + QStringList({FN_VAS, FN_CRP, FN_ESR});
}


// ============================================================================
// Instance info
// ============================================================================

bool Das28::isComplete() const
{
    if (anyNull(values(getJointFieldNames() + QStringList(FN_VAS)))) {
        return false;
    }

    if (value(FN_CRP).isNull() && value(FN_ESR).isNull()) {
        return false;
    }

    return true;
}


QVariant Das28::das28Crp() const
{
    const QVariant crp = value(FN_CRP);
    const QVariant vas = value(FN_VAS);

    if (crp.isNull() || vas.isNull()) {
        return QVariant();
    }

    return 0.56 * std::sqrt(tenderJointCount()) +
        0.28 * std::sqrt(swollenJointCount()) +
        0.36 * std::log(crp.toDouble() + 1) +
        0.014 * vas.toInt() +
        0.96;
}


QVariant Das28::das28Esr() const
{
    const QVariant esr = value(FN_ESR);
    const QVariant vas = value(FN_VAS);

    if (esr.isNull() || vas.isNull()) {
        return QVariant();
    }

    return 0.56 * std::sqrt(tenderJointCount()) +
        0.28 * std::sqrt(swollenJointCount()) +
        0.70 * std::log(esr.toDouble()) +
        0.014 * vas.toInt();
}


int Das28::swollenJointCount() const
{
    return sumInt(values(getSwollenFieldNames()));
}


int Das28::tenderJointCount() const
{
    return sumInt(values(getTenderFieldNames()));
}


QString Das28::activityStateCrp(const QVariant& measurement) const
{
    // as recommended by https://rmdopen.bmj.com/content/3/1/e000382

    if (measurement.isNull()) {
        return xstring("n_a");
    }

    if (measurement < 2.4) {
        return xstring("remission");
    }

    if (measurement < 2.9) {
        return xstring("low");
    }

    if (measurement > 4.6) {
        return xstring("high");
    }

    return xstring("moderate");
}


QString Das28::activityStateEsr(const QVariant& measurement) const
{
    // https://onlinelibrary.wiley.com/doi/full/10.1002/acr.21649
    // (has same cutoffs for CRP)

    if (measurement.isNull()) {
        return xstring("n_a");
    }

    if (measurement < 2.6) {
        return xstring("remission");
    }

    if (measurement < 3.2) {
        return xstring("low");
    }

    if (measurement > 5.1) {
        return xstring("high");
    }

    return xstring("moderate");
}


QStringList Das28::summary() const
{
    using stringfunc::bold;

    const QVariant das28_crp = das28Crp();
    const QVariant das28_esr = das28Esr();

    return QStringList{
        QString("%1: %2 (%3)").arg(xstring("das28_crp"),
                                   convert::prettyValue(das28_crp, CRP_ESR_DP),
                                   bold(activityStateCrp(das28_crp))
        ),
        QString("%1: %2 (%3)").arg(xstring("das28_esr"),
                                   convert::prettyValue(das28_esr, CRP_ESR_DP),
                                   bold(activityStateEsr(das28_esr))
        ),
    };
}


QStringList Das28::detail() const
{
    QStringList lines = completenessInfo();

    auto html = QString("<table>");
    html.append("<tr>");
    html.append("<th></th>");

    auto states_html = QString("<tr><th></th>");
    for (const QString& side : SIDES) {
        html.append(QString("<th colspan='2'>%1</th>").arg(xstring(side)));

        for (const QString& state : STATES) {
            states_html.append(QString("<th style='padding:0 10px;'>%1</th>").arg(xstring(state)));
        }
    }
    html.append("</tr>");
    states_html.append("</tr>");

    html.append(states_html);

    for (const QString& joint : getJointNames()) {
        html.append("<tr>");
        html.append(QString("<th style='text-align:right;'>%1</th>").arg(xstring(joint)));

        for (const QString& side : SIDES) {
            for (const QString& state : STATES) {
                const auto fieldname = QString("%1_%2_%3").arg(
                    side, joint, state);

                html.append("<td style='text-align:center;'>");

                QString cell_contents = "?";
                const QVariant cell_value = value(fieldname);

                if (!cell_value.isNull()) {
                    cell_contents = cell_value.toBool() ? "✓" : "×";
                }
                html.append(cell_contents);
                html.append("</td>");
            }
        }
        html.append("</td>");
        html.append("</tr>");
    }
    html.append("</table>");

    lines.append(html);

    lines.append(fieldSummary(FN_VAS, xstring("vas"), " "));
    lines.append(fieldSummary(FN_CRP, xstring("crp"), " "));
    lines.append(fieldSummary(FN_ESR, xstring("esr"), " "));

    lines.append("");
    lines += summary();

    return lines;
}


OpenableWidget* Das28::editor(const bool read_only)
{
    QuPagePtr page((new QuPage())->setTitle(xstring("title_main")));

    page->addElement(new QuText(xstring("observer")));
    page->addElement(getClinicianQuestionnaireBlockRawPointer());

    auto all_ok_button = new QuButton(
        xstring("mark_all_unmarked_ok"),
        std::bind(&Das28::markAllUnmarkedJointsOk, this)
    );
    page->addElement(all_ok_button);
    page->addElement(
        new QuSpacer(QSize(uiconst::BIGSPACE, uiconst::BIGSPACE))
    );
    page->addElement(getJointGrid());

    page->addElement(new QuText(xstring("vas_instructions")));

    QuSlider* vas_slider = new QuSlider(fieldRef(FN_VAS), 0, 100, 1);
    vas_slider->setHorizontal(true);
    vas_slider->setBigStep(1);

    const bool can_shrink = false;
    vas_slider->setAbsoluteLengthCm(10, can_shrink);

    vas_slider->setTickInterval(1);
    vas_slider->setTickLabels({
            {0, xstring("vas_min")},
            {100, xstring("vas_max")}
        }
    );
    vas_slider->setTickLabelPosition(QSlider::TicksAbove);

    vas_slider->setShowValue(false);
    vas_slider->setSymmetric(true);
    page->addElement(vas_slider);
    const auto crp_esr_inst = new QuText(xstring("crp_esr_instructions"));
    crp_esr_inst->setBold(true);

    page->addElement(crp_esr_inst);

    page->addElement(new QuText(xstring("crp")));
    const auto crp_field = new QuLineEditDouble(
        fieldRef(FN_CRP), CRP_MIN, CRP_MAX, CRP_ESR_DP);
    page->addElement(crp_field);

    page->addElement(new QuText(xstring("esr")));
    const auto esr_field = new QuLineEditDouble(
        fieldRef(FN_ESR), ESR_MIN, ESR_MAX, CRP_ESR_DP);
    page->addElement(esr_field);

    connect(fieldRef(FN_CRP).data(), &FieldRef::valueChanged,
            this, &Das28::crpChanged);

    connect(fieldRef(FN_ESR).data(), &FieldRef::valueChanged,
            this, &Das28::esrChanged);

    crpChanged();
    esrChanged();

    m_questionnaire = new Questionnaire(m_app, {page});
    m_questionnaire->setType(QuPage::PageType::ClinicianWithPatient);
    m_questionnaire->setReadOnly(read_only);

    return m_questionnaire;
}

void Das28::markAllUnmarkedJointsOk()
{
    for (const FieldRefPtr& field : m_joint_fieldrefs) {
        if (field->value().isNull()) {
            field->setValue(false);
        }
    }
}


QuGridContainer* Das28::getJointGrid()
{
    auto grid = new QuGridContainer();
    grid->setExpandHorizontally(false);
    grid->setFixedGrid(false);

    int row = 0;

    const QStringList first_joints = {"shoulder", "mcp_1", "pip_1", "knee"};

    m_joint_fieldrefs.clear();

    for (const QString& joint : getJointNames()) {
        if (first_joints.contains(joint)) {
            if (row != 0) {
                grid->addCell(QuGridCell(new QuSpacer(QSize(uiconst::BIGSPACE,
                                                            uiconst::BIGSPACE)), row, 0));
                row++;
            }

            addJointGridHeading(grid, row);
        }

        int column = 0;

        grid->addCell(QuGridCell(new QuText(xstring(joint)), row, column,
                                 GRID_ROW_SPAN, GRID_JOINT_COLUMN_SPAN));
        column += GRID_JOINT_COLUMN_SPAN;

        for (const QString& side : SIDES) {
            for (const QString & state : STATES) {
                const auto fieldname = QString("%1_%2_%3").arg(
                    side, joint, state);
                FieldRefPtr field = fieldRef(fieldname);
                QuBoolean* element = new QuBoolean("", field);
                m_joint_fieldrefs.append(field);

                grid->addCell(QuGridCell(element, row, column));
                column++;
            }
        }

        row++;
    }

    return grid;
}


void Das28::addJointGridHeading(QuGridContainer* grid, int& row)
{
    int column = 0;

    grid->addCell(QuGridCell(new QuText(""), row, column,
                             GRID_ROW_SPAN, GRID_JOINT_COLUMN_SPAN));
    column += GRID_JOINT_COLUMN_SPAN;

    const auto left_label = new QuText(xstring("left"));
    left_label->setBold(true);

    const auto right_label = new QuText(xstring("right"));
    right_label->setBold(true);
    grid->addCell(QuGridCell(left_label, row, column,
                             GRID_ROW_SPAN, GRID_SIDE_COLUMN_SPAN));
    column += GRID_SIDE_COLUMN_SPAN;

    grid->addCell(QuGridCell(right_label, row, column,
                             GRID_ROW_SPAN, GRID_SIDE_COLUMN_SPAN));

    column = 0;
    row++;

    grid->addCell(QuGridCell(new QuText(""), row, column,
                             GRID_ROW_SPAN, GRID_JOINT_COLUMN_SPAN));
    column += GRID_JOINT_COLUMN_SPAN;

    for (int i = 0; i < SIDES.length(); i++) {
        for (const QString & state: STATES) {
            grid->addCell(QuGridCell(new QuText(xstring(state)), row, column,
                                     GRID_ROW_SPAN, GRID_STATE_COLUMN_SPAN));
            column += GRID_STATE_COLUMN_SPAN;
        }
    }

    row++;
}

void Das28::crpChanged()
{
    const bool esr_mandatory = value(FN_CRP).isNull();

    fieldRef(FN_ESR)->setMandatory(esr_mandatory);
}


void Das28::esrChanged()
{
    const bool crp_mandatory = value(FN_ESR).isNull();

    fieldRef(FN_CRP)->setMandatory(crp_mandatory);
}
