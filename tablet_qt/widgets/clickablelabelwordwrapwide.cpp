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

// #define DEBUG_CALCULATIONS

#include "clickablelabelwordwrapwide.h"
#include <QDebug>
#include <QMargins>
#include <QStyleOptionButton>
#include "lib/sizehelpers.h"
#include "lib/uifunc.h"
#include "widgets/labelwordwrapwide.h"


ClickableLabelWordWrapWide::ClickableLabelWordWrapWide(const QString& text,
                                                       const bool stretch,
                                                       QWidget* parent) :
    ClickableLabelWordWrapWide(stretch, parent)  // delegating constructor
{
    m_label->setText(text);  // this is what the QLabel(text, parent) constructor does
}


ClickableLabelWordWrapWide::ClickableLabelWordWrapWide(const bool stretch,
                                                       QWidget* parent) :
    QPushButton(parent),
    m_label(new LabelWordWrapWide(this)),
    m_layout(new VBoxLayout())
{
    m_label->setMouseTracking(false);
    m_label->setTextInteractionFlags(Qt::NoTextInteraction);
    // ... makes sure that all clicks come to us (not e.g. trigger URL)

    // m_label->setObjectName(CssConst::DEBUG_YELLOW);

    // m_layout->setContentsMargins(UiConst::NO_MARGINS);
    // no, use CSS instead // layout->setMargin(0);

    m_layout->addWidget(m_label);
    if (stretch) {
        m_layout->addStretch();
    }

    setLayout(m_layout);
    setSizePolicy(stretch ? sizehelpers::expandingFixedHFWPolicy()
                          : sizehelpers::maximumFixedHFWPolicy());
    // http://doc.qt.io/qt-5/layout.html

    adjustSize();
}


void ClickableLabelWordWrapWide::setTextFormat(const Qt::TextFormat format)
{
    Q_ASSERT(m_label);
    m_label->setTextFormat(format);
    adjustSize();
}


void ClickableLabelWordWrapWide::setWordWrap(const bool on)
{
    Q_ASSERT(m_label);
    m_label->setWordWrap(on);
    adjustSize();
}


void ClickableLabelWordWrapWide::setAlignment(const Qt::Alignment alignment)
{
    Q_ASSERT(m_label);
    m_label->setAlignment(alignment);
    m_layout->setAlignment(m_label, alignment);
}


void ClickableLabelWordWrapWide::setOpenExternalLinks(const bool open)
{
    Q_ASSERT(m_label);
    m_label->setOpenExternalLinks(open);
}


// http://permalink.gmane.org/gmane.comp.lib.qt.general/40030

QSize ClickableLabelWordWrapWide::translateSize(const QSize& size) const
{
    QStyleOptionButton opt;
    initStyleOption(&opt);  // protected
    return size + sizehelpers::pushButtonExtraSizeRequired(this, &opt, size);
}


QSize ClickableLabelWordWrapWide::sizeHint() const
{
    Q_ASSERT(m_label);
    ensurePolished();
    QSize result = translateSize(m_label->sizeHint());
#ifdef DEBUG_CALCULATIONS
    qDebug() << Q_FUNC_INFO << "->" << result;
#endif
    return result;
}


QSize ClickableLabelWordWrapWide::minimumSizeHint() const
{
    Q_ASSERT(m_label);
    QSize result = translateSize(m_label->minimumSizeHint());
#ifdef DEBUG_CALCULATIONS
    qDebug() << Q_FUNC_INFO << "->" << result;
#endif
    return result;
}


#ifdef GUI_USE_RESIZE_FOR_HEIGHT
void ClickableLabelWordWrapWide::resizeEvent(QResizeEvent* event)
{
#ifdef DEBUG_CALCULATIONS
    qDebug() << Q_FUNC_INFO;
#endif
    QPushButton::resizeEvent(event);
    sizehelpers::resizeEventForHFWParentWidget(this);
}
#endif


void ClickableLabelWordWrapWide::setText(const QString& text)
{
    Q_ASSERT(m_label);
#ifdef DEBUG_CALCULATIONS
    qDebug() << Q_FUNC_INFO << text;
#endif
    m_label->setText(text);
    adjustSize();  // QWidget::adjustSize(): adjust this widget to fit contents
}
