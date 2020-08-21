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

#include "quboolean.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>
#include "lib/convert.h"
#include "lib/uifunc.h"
#include "questionnairelib/mcqfunc.h"
#include "questionnairelib/questionnaire.h"
#include "widgets/aspectratiopixmap.h"
#include "widgets/basewidget.h"
#include "widgets/booleanwidget.h"
#include "widgets/clickablelabelwordwrapwide.h"
#include "widgets/labelwordwrapwide.h"


QuBoolean::QuBoolean(const QString& text, const QString& filename,
                     const QSize& size, FieldRefPtr fieldref) :
    m_text(text),
    m_image_filename(filename),
    m_image_size(size),
    m_adjust_image_for_dpi(true),
    m_fieldref(fieldref),
    m_content_clickable(true),
    m_indicator_on_left(true),  // due to LabelWordWrap, better as true
    m_big_indicator(true),  // finger-sized; standard...
    m_big_text(false),
    m_bold(false),
    m_italic(false),
    m_allow_unset(false),
    m_as_text_button(false),
    m_false_appears_blank(false),
    m_indicator(nullptr)
{
    Q_ASSERT(m_fieldref);
    connect(m_fieldref.data(), &FieldRef::valueChanged,
            this, &QuBoolean::fieldValueChanged);
    connect(m_fieldref.data(), &FieldRef::mandatoryChanged,
            this, &QuBoolean::fieldValueChanged);
}


QuBoolean::QuBoolean(const QString& text, FieldRefPtr fieldref) :
    QuBoolean(text, "", QSize(), fieldref)  // delegating constructor
{
}


QuBoolean::QuBoolean(const QString &filename, const QSize &size,
                     FieldRefPtr fieldref) :
    QuBoolean("", filename, size, fieldref)  // delegating constructor
{
}


QuBoolean* QuBoolean::setContentClickable(const bool clickable)
{
    m_content_clickable = clickable;
    return this;
}


QuBoolean* QuBoolean::setIndicatorOnLeft(const bool indicator_on_left)
{
    m_indicator_on_left = indicator_on_left;
    return this;
}


QuBoolean* QuBoolean::setBigIndicator(const bool big)
{
    m_big_indicator = big;
    return this;
}


QuBoolean* QuBoolean::setBigText(const bool big)
{
    m_big_text = big;
    return this;
}


QuBoolean* QuBoolean::setBold(const bool bold)
{
    m_bold = bold;
    return this;
}


QuBoolean* QuBoolean::setItalic(const bool italic)
{
    m_italic = italic;
    return this;
}


QuBoolean* QuBoolean::setAllowUnset(const bool allow_unset)
{
    m_allow_unset = allow_unset;
    return this;
}


QuBoolean* QuBoolean::setAsTextButton(const bool as_text_button)
{
    m_as_text_button = as_text_button;
    return this;
}


QuBoolean* QuBoolean::setAdjustImageForDpi(const bool adjust_image_for_dpi)
{
    m_adjust_image_for_dpi = adjust_image_for_dpi;
    return this;
}


QuBoolean* QuBoolean::setFalseAppearsBlank(const bool false_appears_blank)
{
    m_false_appears_blank = false_appears_blank;
    return this;
}


QPointer<QWidget> QuBoolean::makeWidget(Questionnaire* questionnaire)
{
    const bool read_only = questionnaire->readOnly();

    QPointer<QWidget> widget(new BaseWidget());
    widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    auto layout = new HBoxLayout();
    // ... allow the HFW layouts, so our owner can put us in a flow layout
    layout->setContentsMargins(uiconst::NO_MARGINS);
    widget->setLayout(layout);
    // To align things in a QHBoxLayout, align the widgets within the layout:
    //      layout->setAlignment(widget, alignment)
    // not the layout:
    //      layout->setAlignment(alignment)

    // Label
    QWidget* labelwidget = nullptr;
    if (!m_text.isEmpty() && !m_as_text_button) {
        // --------------------------------------------------------------------
        // Text label
        // --------------------------------------------------------------------
        if (!read_only && m_content_clickable) {
            auto label = new ClickableLabelWordWrapWide(m_text);
            connect(label, &ClickableLabelWordWrapWide::clicked,
                    this, &QuBoolean::clicked);
            labelwidget = label;
        } else {
            auto label = new LabelWordWrapWide(m_text);
            labelwidget = label;
        }
        const int fontsize = questionnaire->fontSizePt(
            m_big_text ? uiconst::FontSize::Big : uiconst::FontSize::Normal);
        QString css = uifunc::textCSS(fontsize, m_bold, m_italic);
        labelwidget->setStyleSheet(css);
        // needs_stretch stays false (or we'll prevent the text expanding)
    } else if (!m_image_filename.isEmpty()) {
        // --------------------------------------------------------------------
        // Image label (accompanying image)
        // --------------------------------------------------------------------
        QPixmap image = uifunc::getPixmap(m_image_filename, m_image_size);
        if (m_adjust_image_for_dpi) {
            image = image.scaled(convert::convertSizeByLogicalDpi(image.size()));
        }
        auto label = new AspectRatioPixmap();
        label->setPixmap(image);
        if (!read_only && m_content_clickable) {
            connect(label, &AspectRatioPixmap::clicked,
                    this, &QuBoolean::clicked);
        }
        labelwidget = label;
    }
    // otherwise... no label, just the indicator

    // ------------------------------------------------------------------------
    // Indicator
    // (typically a box with tick/cross/empty, but potentially a text button)
    // ------------------------------------------------------------------------
    m_indicator = new BooleanWidget();
    m_indicator->setSize(m_big_indicator);
    m_indicator->setBold(m_bold);
    m_indicator->setReadOnly(read_only);
    if (m_as_text_button) {
        m_indicator->setAppearance(BooleanWidget::Appearance::Text);
        m_indicator->setText(m_text);
    } else {
        if (m_false_appears_blank) {
            // Slightly unusual
            m_indicator->setAppearance(
                        BooleanWidget::Appearance::CheckRedFalseAppearsBlank);
        } else {
            // Normal
            m_indicator->setAppearance(BooleanWidget::Appearance::CheckRed);
        }
    }
    if (!read_only) {
        connect(m_indicator, &BooleanWidget::clicked,
                this, &QuBoolean::clicked);
    }

    // Whole thing
    const Qt::Alignment label_align = Qt::AlignVCenter;
    const Qt::Alignment indicator_align = Qt::AlignTop;
    if (labelwidget) {
        if (m_indicator_on_left) {
            layout->addWidget(m_indicator, 0, indicator_align);
            layout->addWidget(labelwidget, 0, label_align);
        } else {
            layout->addWidget(labelwidget, 0, label_align);
            layout->addWidget(m_indicator, 0, indicator_align);
        }
    } else {
        // Just the indicator
        layout->addWidget(m_indicator, 0, indicator_align);
    }
    layout->addStretch();

    setFromField();

    return widget;
}


void QuBoolean::setFromField()
{
    fieldValueChanged(m_fieldref.data());
}


void QuBoolean::clicked()
{
    mcqfunc::toggleBooleanField(m_fieldref.data(), m_allow_unset);
    emit elementValueChanged();
}


void QuBoolean::fieldValueChanged(const FieldRef* fieldref)
{
    if (!m_indicator) {
        // qDebug() << "... NO INDICATOR";
        return;
    }
    m_indicator->setValue(fieldref->value(), fieldref->mandatory());
}


FieldRefPtrList QuBoolean::fieldrefs() const
{
    return FieldRefPtrList{m_fieldref};
}
