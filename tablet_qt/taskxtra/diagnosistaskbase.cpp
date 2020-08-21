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

#include "diagnosistaskbase.h"
#include <algorithm>
#include <QDate>
#include "diagnosisitembase.h"
#include "common/textconst.h"
#include "diagnosis/diagnosticcodeset.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/qubutton.h"
#include "questionnairelib/qudatetime.h"
#include "questionnairelib/qudiagnosticcode.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quflowcontainer.h"
#include "questionnairelib/qugridcontainer.h"
#include "questionnairelib/quhorizontalcontainer.h"
#include "questionnairelib/quhorizontalline.h"
#include "questionnairelib/qulineedit.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/quverticalcontainer.h"
using stringfunc::bold;

const QString DiagnosisTaskBase::RELATES_TO_DATE("relates_to_date");  // new in v2.0.0


DiagnosisTaskBase::DiagnosisTaskBase(
        CamcopsApp& app, DatabaseManager& db,
        const QString& tablename, const int load_pk) :
    Task(app, db, tablename, false, true, false),  // ... anon, clin, resp
    m_questionnaire(nullptr),
    m_codeset(nullptr)
{
    addField(RELATES_TO_DATE, QVariant::Date);  // new in v2.0.0

    load(load_pk);
}


// ============================================================================
// Instance info
// ============================================================================

bool DiagnosisTaskBase::isComplete() const
{
    if (valueIsNull(RELATES_TO_DATE)) {
        return false;
    }
    if (m_items.empty()) {
        return false;
    }
    for (const DiagnosisItemBasePtr& item : m_items) {
        if (item->isEmpty()) {
            return false;
        }
    }
    return true;
}


QStringList DiagnosisTaskBase::summary() const
{
    QStringList lines;
    lines.append(tr("Relates to: ") + bold(prettyValue(RELATES_TO_DATE)) + ".");
    for (const DiagnosisItemBasePtr& item : m_items) {
        lines.append(QString("%1: <b>%2 – %3</b>.").arg(
                         QString::number(item->seqnum()),
                         item->code(),
                         item->description()));
    }
    return lines;
}


QStringList DiagnosisTaskBase::detail() const
{
    return completenessInfo() + summary();
}


OpenableWidget* DiagnosisTaskBase::editor(const bool read_only)
{
    m_codeset = makeCodeset();

    m_core_elements = QVector<QuElementPtr>{
        getClinicianQuestionnaireBlockElementPtr(),
        QuElementPtr((new QuHorizontalContainer{
            new QuText(tr("Date diagnoses relate to:")),
            (new QuDateTime(fieldRef(RELATES_TO_DATE)))
                        ->setMode(QuDateTime::Mode::DefaultDate)
                        ->setOfferNowButton(true),
        })->setWidgetAlignment(Qt::AlignTop)),
        QuElementPtr(new QuButton(
            TextConst::add(),
            std::bind(&DiagnosisTaskBase::addItem, this)
        )),
    };

    QuPage* page = (new QuPage())
                   ->setTitle(longname())
                   ->setType(QuPage::PageType::Clinician);
    rebuildPage(page);

    m_questionnaire = new Questionnaire(m_app, {QuPagePtr(page)});
    m_questionnaire->setType(QuPage::PageType::Clinician);
    m_questionnaire->setReadOnly(read_only);
    return m_questionnaire;
}


// ============================================================================
// Ancillary management
// ============================================================================

QVector<DatabaseObjectPtr> DiagnosisTaskBase::getAllAncillary() const
{
    QVector<DatabaseObjectPtr> ancillaries;
    for (const DiagnosisItemBasePtr& item : m_items) {
        ancillaries.append(item);
    }
    return ancillaries;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

void DiagnosisTaskBase::addItem()
{
    bool one_is_empty = false;
    for (const DiagnosisItemBasePtr& item : m_items) {
        if (item->valueIsNullOrEmpty(DiagnosisItemBase::CODE)) {
            one_is_empty = true;
            break;
        }
    }
    if (one_is_empty) {
        uifunc::alert(tr("A diagnosis already needs setting; won’t add "
                         "another"));
        return;
    }
    DiagnosisItemBasePtr item = makeItem();
    item->setSeqnum(m_items.size() + 1);  // bugfix 2018-12-01; now always 1-based seqnum
    item->save();
    m_items.append(item);
    refreshQuestionnaire();
}


void DiagnosisTaskBase::deleteItem(const int index)
{
    if (index < 0 || index >= m_items.size()) {
        return;
    }
    DiagnosisItemBasePtr item = m_items.at(index);
    item->deleteFromDatabase();
    m_items.removeAt(index);
    renumberItems();
    refreshQuestionnaire();
}


void DiagnosisTaskBase::moveUp(const int index)
{
    if (index < 1 || index >= m_items.size()) {
        return;
    }
    std::swap(m_items[index - 1], m_items[index]);
    renumberItems();
    refreshQuestionnaire();
}


void DiagnosisTaskBase::moveDown(const int index)
{
    if (index < 0 || index >= m_items.size() - 1) {
        return;
    }
    std::swap(m_items[index], m_items[index + 1]);
    renumberItems();
    refreshQuestionnaire();
}


QVariant DiagnosisTaskBase::getCode(const int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return QVariant();
    }
    DiagnosisItemBasePtr item = m_items.at(index);
    return item->value(DiagnosisItemBase::CODE);
}


bool DiagnosisTaskBase::setCode(const int index, const QVariant& value)
{
    if (index < 0 || index >= m_items.size()) {
        return false;
    }
    DiagnosisItemBasePtr item = m_items.at(index);
    const bool changed = item->setValue(DiagnosisItemBase::CODE, value);
    if (changed) {
        item->save();
    }
    return changed;
}


QVariant DiagnosisTaskBase::getDescription(const int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return QVariant();
    }
    DiagnosisItemBasePtr item = m_items.at(index);
    return item->value(DiagnosisItemBase::DESCRIPTION);
}


bool DiagnosisTaskBase::setDescription(const int index, const QVariant& value)
{
    if (index < 0 || index >= m_items.size()) {
        return false;
    }
    DiagnosisItemBasePtr item = m_items.at(index);
    const bool changed = item->setValue(DiagnosisItemBase::DESCRIPTION, value);
    if (changed) {
        item->save();
    }
    return changed;
}


QVariant DiagnosisTaskBase::getComment(const int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return QVariant();
    }
    DiagnosisItemBasePtr item = m_items.at(index);
    return item->value(DiagnosisItemBase::COMMENT);
}


bool DiagnosisTaskBase::setComment(const int index, const QVariant& value)
{
    if (index < 0 || index >= m_items.size()) {
        return false;
    }
    DiagnosisItemBasePtr item = m_items.at(index);
    const bool changed = item->setValue(DiagnosisItemBase::COMMENT, value);
    if (changed) {
        item->save();
    }
    return changed;
}


void DiagnosisTaskBase::refreshQuestionnaire()
{
    if (!m_questionnaire) {
        return;
    }
    QuPage* page = m_questionnaire->currentPagePtr();
    rebuildPage(page);
    m_questionnaire->refreshCurrentPage();
}


void DiagnosisTaskBase::rebuildPage(QuPage* page)
{
    const Qt::Alignment widget_align = Qt::AlignTop;
    QVector<QuElement*> elements;
    const int n = m_items.size();
    for (int i = 0; i < n; ++i) {
        const bool first = i == 0;
        const bool last = i == n - 1;
        elements.append(new QuHorizontalLine());
        elements.append((new QuText(TextConst::diagnosis() + " " +
                                    QString::number(i + 1)))->setBold());

        FieldRef::GetterFunction get_code = std::bind(
                    &DiagnosisTaskBase::getCode, this, i);
        FieldRef::GetterFunction get_desc = std::bind(
                    &DiagnosisTaskBase::getDescription, this, i);
        FieldRef::GetterFunction get_comment = std::bind(
                    &DiagnosisTaskBase::getComment, this, i);
        FieldRef::SetterFunction set_code = std::bind(
                    &DiagnosisTaskBase::setCode, this, i, std::placeholders::_1);
        FieldRef::SetterFunction set_desc = std::bind(
                    &DiagnosisTaskBase::setDescription, this, i, std::placeholders::_1);
        FieldRef::SetterFunction set_comment = std::bind(
                    &DiagnosisTaskBase::setComment, this, i, std::placeholders::_1);
        FieldRefPtr fr_code = FieldRefPtr(new FieldRef(get_code, set_code, true));
        FieldRefPtr fr_desc = FieldRefPtr(new FieldRef(get_desc, set_desc, true));
        FieldRefPtr fr_comment = FieldRefPtr(new FieldRef(get_comment, set_comment, false));

        auto buttons = new QuFlowContainer({
            new QuButton(
                TextConst::delete_(),
                std::bind(&DiagnosisTaskBase::deleteItem, this, i)
            ),
            (new QuButton(
                TextConst::moveUp(),
                std::bind(&DiagnosisTaskBase::moveUp, this, i)
            ))->setActive(!first),
            (new QuButton(
                TextConst::moveDown(),
                std::bind(&DiagnosisTaskBase::moveDown, this, i)
            ))->setActive(!last),
        });
        buttons->setContainedWidgetAlignments(widget_align);

        auto dx = new QuDiagnosticCode(m_codeset, fr_code, fr_desc);
        auto comment_label = new QuText(TextConst::comment() + ":");
        auto comment_edit = new QuLineEdit(fr_comment);

        auto maingrid = new QuGridContainer();
        const int row_span = 1;
        const int col_span = 1;
        const int button_width = 2;
        const int other_width = 4;
        maingrid->addCell(QuGridCell(buttons, 0, 0,
                                     row_span, col_span, widget_align));
        maingrid->addCell(QuGridCell(dx, 0, 1,
                                     row_span, col_span, widget_align));
        maingrid->addCell(QuGridCell(comment_label, 1, 1,
                                     row_span, col_span, widget_align));
        maingrid->addCell(QuGridCell(comment_edit, 2, 1,
                                     row_span, col_span, widget_align));
        maingrid->setColumnStretch(0, button_width);
        maingrid->setColumnStretch(1, other_width);
        maingrid->setFixedGrid(false);
        maingrid->setExpandHorizontally(true);
        elements.append(maingrid);
    }

    page->clearElements();
    page->addElements(m_core_elements);
    page->addElements(elements);
}


void DiagnosisTaskBase::renumberItems()
{
    // Fine to reset the number to something that doesn't change; the save()
    // call will do nothing.
    const int n = m_items.size();
    for (int i = 0; i < n; ++i) {
        DiagnosisItemBasePtr item = m_items.at(i);
        item->setSeqnum(i + 1);  // 1-based seqnum
        item->save();
    }
}
