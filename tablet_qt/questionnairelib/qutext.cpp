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

#include "qutext.h"
#include <QDebug>
#include "lib/uifunc.h"
#include "questionnairelib/questionnaire.h"
#include "widgets/labelwordwrapwide.h"


QuText::QuText(const QString& text, FieldRefPtr fieldref) :
    m_text(text),
    m_fieldref(fieldref),
    m_fontsize( uiconst::FontSize::Normal),
    m_bold(false),
    m_italic(false),
    m_warning(false),
    m_text_format(Qt::AutoText),
    m_open_links(false),
    m_text_alignment(Qt::AlignLeft | Qt::AlignVCenter),
    m_label(nullptr),
    m_forced_fontsize_pt(-1)
{
    if (fieldref) {
        connect(m_fieldref.data(), &FieldRef::valueChanged,
                this, &QuText::fieldValueChanged);
    }
}


QuText::QuText(const QString& text) :
    QuText(text, nullptr)  // delegating constructor
{
}


QuText::QuText(FieldRefPtr fieldref) :
    QuText("", fieldref)  // delegating constructor
{
    Q_ASSERT(m_fieldref);
}


QuText* QuText::setFontSize(uiconst::FontSize fontsize)
{
    m_fontsize = fontsize;
    return this;
}


QuText* QuText::setBig(const bool big)
{
    m_fontsize = big ? uiconst::FontSize::Big : uiconst::FontSize::Normal;
    return this;
}


QuText* QuText::setBold(const bool bold)
{
    m_bold = bold;
    return this;
}


QuText* QuText::setItalic(const bool italic)
{
    m_italic = italic;
    return this;
}


QuText* QuText::setWarning(const bool warning)
{
    m_warning = warning;
    return this;
}


QuText* QuText::setFormat(const Qt::TextFormat format)
{
    m_text_format = format;
    return this;
}


QuText* QuText::setOpenLinks(const bool open_links)
{
    m_open_links = open_links;
    return this;
}


QuText* QuText::setTextAlignment(const Qt::Alignment alignment)
{
    m_text_alignment = alignment;
    return this;
}


QuText* QuText::setTextAndWidgetAlignment(const Qt::Alignment alignment)
{
    setTextAlignment(alignment);
    setWidgetAlignment(alignment);
    return this;
}


QPointer<QWidget> QuText::makeWidget(Questionnaire* questionnaire)
{
    QString text;
    if (m_fieldref && m_fieldref->valid()) {
        text = m_fieldref->valueString();
    } else {
        text = m_text;
    }
    m_label = new LabelWordWrapWide(text);
    const int fontsize = questionnaire->fontSizePt(m_fontsize);
    setWidgetFontSize(m_forced_fontsize_pt > 0 ? m_forced_fontsize_pt
                                               : fontsize);
    m_label->setTextFormat(m_text_format);
    m_label->setOpenExternalLinks(m_open_links);
    m_label->setAlignment(m_text_alignment);
    // ... this is QLabel::setAlignment; see
    //     http://doc.qt.io/qt-5/qlabel.html#alignment-prop
    return QPointer<QWidget>(m_label);
}


void QuText::fieldValueChanged(const FieldRef* fieldref)
{
    if (!m_label) {
        qDebug() << Q_FUNC_INFO << "... NO LABEL";
        return;
    }
    m_label->setText(fieldref->valueString());
}


void QuText::forceFontSize(const int fontsize_pt, const bool repolish)
{
    m_forced_fontsize_pt = fontsize_pt;
    setWidgetFontSize(m_forced_fontsize_pt, repolish);
}


void QuText::setText(const QString &text, const bool repolish)
{
    m_text = text;
    if (!m_label) {
        return;
    }
    m_label->setText(text);
    if (repolish) {
        repolishWidget();
    }
}


void QuText::setWidgetFontSize(const int fontsize_pt, const bool repolish)
{
    if (!m_label) {
        return;
    }
    const QString colour = m_warning ? uiconst::WARNING_COLOUR_CSS : "";
    const QString css = uifunc::textCSS(fontsize_pt, m_bold, m_italic, colour);
    m_label->setStyleSheet(css);
    if (repolish) {
        repolishWidget();
    }
}


void QuText::repolishWidget()
{
    if (m_label) {
        uifunc::repolish(m_label);
        m_label->updateGeometry();
    }
}
