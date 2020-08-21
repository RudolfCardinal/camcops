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

#include "qumultipleresponse.h"
#include "common/cssconst.h"
#include "layouts/layouts.h"
#include "layouts/flowlayouthfw.h"
#include "lib/uifunc.h"
#include "maths/ccrandom.h"
#include "questionnairelib/questionnaire.h"
#include "widgets/basewidget.h"
#include "widgets/booleanwidget.h"
#include "widgets/clickablelabelwordwrapwide.h"
#include "widgets/labelwordwrapwide.h"


QuMultipleResponse::QuMultipleResponse(
        const QVector<QuestionWithOneField>& items) :
    m_items(items),
    m_minimum_answers(0),
    m_maximum_answers(-1),
    m_randomize(false),
    m_show_instruction(true),
    m_horizontal(false),
    m_as_text_button(false),
    m_bold(false),
    m_instruction_label(nullptr)
{
    // Connect fieldrefs at widget build time, for simplicity.
}


QuMultipleResponse::QuMultipleResponse(
        std::initializer_list<QuestionWithOneField> items) :
    QuMultipleResponse(QVector<QuestionWithOneField>(items))  // delegating constructor
{
}


QuMultipleResponse::QuMultipleResponse() :
    QuMultipleResponse(QVector<QuestionWithOneField>())  // delegating constructor
{
}


QuMultipleResponse* QuMultipleResponse::addItem(
        const QuestionWithOneField& item)
{
    m_items.append(item);
    return this;
}


QuMultipleResponse* QuMultipleResponse::setMinimumAnswers(int minimum_answers)
{
    if (minimum_answers < 0) {
        minimum_answers = 0;
    }
    const bool changed = minimum_answers != m_minimum_answers;
    if (changed) {
        m_minimum_answers = minimum_answers;
        minOrMaxChanged();
    }
    return this;
}


QuMultipleResponse* QuMultipleResponse::setMaximumAnswers(int maximum_answers)
{
    if (maximum_answers == 0) {  // dumb value, use -1 for don't care
        maximum_answers = -1;
    }
    const bool changed = maximum_answers != m_maximum_answers;
    if (changed) {
        m_maximum_answers = maximum_answers;
        minOrMaxChanged();
    }
    return this;
}


void QuMultipleResponse::minOrMaxChanged()
{
    if (!m_widgets.empty()) {
        // we're live
        if (m_show_instruction && m_instruction_label &&
                m_instruction.isEmpty()) {
            m_instruction_label->setText(defaultInstruction());
        }
        fieldValueChanged();  // may change mandatory colour
        emit elementValueChanged();  // may change page "next" status etc.
    }
}


QuMultipleResponse* QuMultipleResponse::setRandomize(const bool randomize)
{
    m_randomize = randomize;
    return this;
}


QuMultipleResponse* QuMultipleResponse::setShowInstruction(
        bool show_instruction)
{
    m_show_instruction = show_instruction;
    return this;
}


QuMultipleResponse* QuMultipleResponse::setInstruction(
        const QString& instruction)
{
    m_instruction = instruction;
    return this;
}


QuMultipleResponse* QuMultipleResponse::setHorizontal(const bool horizontal)
{
    m_horizontal = horizontal;
    return this;
}


QuMultipleResponse* QuMultipleResponse::setAsTextButton(
        const bool as_text_button)
{
    m_as_text_button = as_text_button;
    return this;
}


QuMultipleResponse* QuMultipleResponse::setBold(const bool bold)
{
    m_bold = bold;
    return this;
}


QPointer<QWidget> QuMultipleResponse::makeWidget(Questionnaire* questionnaire)
{
    // Clear old stuff
    m_widgets.clear();

    // Randomize?
    if (m_randomize) {
        ccrandom::shuffle(m_items);
    }

    const bool read_only = questionnaire->readOnly();

    QPointer<QWidget> mainwidget = new BaseWidget();
    QLayout* mainlayout;
    if (m_horizontal) {
        mainlayout = new FlowLayoutHfw();
    } else {
        mainlayout = new VBoxLayout();
    }
    mainlayout->setContentsMargins(uiconst::NO_MARGINS);
    mainwidget->setLayout(mainlayout);

    for (int i = 0; i < m_items.size(); ++i) {
        const QuestionWithOneField& item = m_items.at(i);

        // Widget
        QPointer<BooleanWidget> w = new BooleanWidget();
        w->setReadOnly(read_only);
        w->setAppearance(m_as_text_button
                         ? BooleanWidget::Appearance::Text
                         : BooleanWidget::Appearance::CheckRed);
        if (m_as_text_button) {
            w->setText(item.text());
            w->setBold(m_bold);
        }
        if (!read_only) {
            // Safe object lifespan signal: can use std::bind
            connect(w, &BooleanWidget::clicked,
                    std::bind(&QuMultipleResponse::clicked, this, i));
        }
        m_widgets.append(w);

        // Layout, +/- label
        if (m_as_text_button) {
            mainlayout->addWidget(w);
            mainlayout->setAlignment(w, Qt::AlignTop);
        } else {
            // cf. QuMCQ
            auto itemwidget = new QWidget();
            ClickableLabelWordWrapWide* namelabel = new ClickableLabelWordWrapWide(item.text());
            namelabel->setEnabled(!read_only);
            const int fontsize = questionnaire->fontSizePt(uiconst::FontSize::Normal);
            const bool italic = false;
            const QString css = uifunc::textCSS(fontsize, m_bold, italic);
            namelabel->setStyleSheet(css);
            if (!read_only) {
                // Safe object lifespan signal: can use std::bind
                connect(namelabel, &ClickableLabelWordWrapWide::clicked,
                        std::bind(&QuMultipleResponse::clicked, this, i));
            }
            auto itemlayout = new HBoxLayout();
            itemlayout->setContentsMargins(uiconst::NO_MARGINS);
            itemwidget->setLayout(itemlayout);
            itemlayout->addWidget(w, 0, Qt::AlignTop);
            itemlayout->addWidget(namelabel, 0, Qt::AlignVCenter);  // different
            itemlayout->addStretch();

            mainlayout->addWidget(itemwidget);
            mainlayout->setAlignment(itemwidget, Qt::AlignTop);
        }

        // Field-to-this connections
        // You can't use connect() with a std::bind and simultaneously
        // specify a connection type such as Qt::UniqueConnection (which
        // only works with QObject, I think).
        // However, you can use a QSignalMapper. Then you can wipe it before
        // connecting, removing the need for Qt::UniqueConnection.
        // Finally, you need to disambiguate the slot with e.g.
        //      void (QSignalMapper::*map_signal)() = &QSignalMapper::map;
        // ... But in the end, all widgets may need to be updated when a single
        // value changes (based on the number required), so all this was
        // pointless and we can use a single signal with no parameters.

        FieldRef* fr = item.fieldref().data();
        connect(fr, &FieldRef::valueChanged,
                this, &QuMultipleResponse::fieldValueChanged,
                Qt::UniqueConnection);
        connect(fr, &FieldRef::mandatoryChanged,
                this, &QuMultipleResponse::fieldValueChanged,
                Qt::UniqueConnection);
    }

    QPointer<QWidget> final_widget;
    if (m_show_instruction) {
        // Higher-level widget containing {instructions, actual MCQ}
        auto layout_w_instr = new VBoxLayout();
        layout_w_instr->setContentsMargins(uiconst::NO_MARGINS);
        QString instruction = m_instruction.isEmpty() ? defaultInstruction()
                                                      : m_instruction;
        m_instruction_label = new LabelWordWrapWide(instruction);
        m_instruction_label->setObjectName(cssconst::MCQ_INSTRUCTION);
        layout_w_instr->addWidget(m_instruction_label);
        layout_w_instr->addWidget(mainwidget);
        QPointer<QWidget> widget_w_instr = new QWidget();
        widget_w_instr->setLayout(layout_w_instr);
        final_widget = widget_w_instr;
    } else {
        final_widget = mainwidget;
    }

    setFromFields();

    return final_widget;
}


void QuMultipleResponse::clicked(const int index)
{
    if (!validIndex(index)) {
        qWarning() << Q_FUNC_INFO << "- out of range";
        return;
    }
    const bool at_max = nTrueAnswers() >= maximumAnswers();
    bool changed = false;
    const QuestionWithOneField item = m_items.at(index);
    FieldRefPtr fieldref = item.fieldref();
    const QVariant value = fieldref->value();
    QVariant newvalue;
    if (value.isNull()) {  // NULL -> true
        if (!at_max) {
            newvalue = true;
            changed = true;
        }
    } else if (value.toBool()) {  // true -> false
        newvalue = false;
        changed = true;
    } else {  // false -> true
        if (!at_max) {
            newvalue = true;
            changed = true;
        }
    }
    if (!changed) {
        return;
    }
    fieldref->setValue(newvalue);  // Will trigger valueChanged
    emit elementValueChanged();
}


void QuMultipleResponse::setFromFields()
{
    fieldValueChanged();
}


void QuMultipleResponse::fieldValueChanged()
{
    const bool need_more = nTrueAnswers() < minimumAnswers();
    for (int i = 0; i < m_items.size(); ++i) {
        FieldRefPtr fieldref = m_items.at(i).fieldref();
        const QVariant value = fieldref->value();
        QPointer<BooleanWidget> w = m_widgets.at(i);
        if (!w) {
            qCritical() << Q_FUNC_INFO << "- defunct pointer!";
            continue;
        }
        if (!value.isNull() && value.toBool()) {
            // true
            w->setState(BooleanWidget::State::True);
        } else {
            // null or false (both look like blanks)
            w->setState(need_more ? BooleanWidget::State::NullRequired
                                  : BooleanWidget::State::Null);
            // We ignore mandatory properties on the fieldref, since we have a
            // minimum/maximum specified for them collectively.
            // Then we override missingInput() so that the QuPage uses our
            // information, not the fieldref information.
        }
    }
}


FieldRefPtrList QuMultipleResponse::fieldrefs() const
{
    FieldRefPtrList fieldrefs;
    for (const QuestionWithOneField& item : m_items) {
        fieldrefs.append(item.fieldref());
    }
    return fieldrefs;
}


int QuMultipleResponse::minimumAnswers() const
{
    return m_minimum_answers;
}


int QuMultipleResponse::maximumAnswers() const
{
    if (m_maximum_answers < 0) {  // the "don't care" value is -1
        return m_items.size();
    }
    return qMin(m_items.size(), m_maximum_answers);
}


QString QuMultipleResponse::defaultInstruction() const
{
    const int minimum = minimumAnswers();
    const int maximum = maximumAnswers();
    if (minimum == maximum) {
        return QString("Choose %1:").arg(minimum);
    }
    if (m_minimum_answers <= 0) {
        return QString("Choose up to %1:").arg(maximum);
    }
    if (m_maximum_answers < 0) {
        return QString("Choose %1 or more:").arg(minimum);
    }
    return QString("Choose %1–%2:").arg(minimum).arg(maximum);
}


bool QuMultipleResponse::validIndex(const int index)
{
    return index >= 0 && index < m_items.size();
}


int QuMultipleResponse::nTrueAnswers() const
{
    int n = 0;
    for (const QuestionWithOneField& item : m_items) {
        const QVariant value = item.fieldref()->value();
        if (!value.isNull() && value.toBool()) {
            n += 1;
        }
    }
    return n;
}


bool QuMultipleResponse::missingInput() const
{
    return nTrueAnswers() < minimumAnswers();
}
