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

#include "clickablelabelnowrap.h"
#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QMouseEvent>
#include <QStyleOptionButton>
#include <QVBoxLayout>
#include "common/uiconst.h"
#include "lib/sizehelpers.h"


ClickableLabelNoWrap::ClickableLabelNoWrap(const QString& text,
                                           QWidget* parent) :
    ClickableLabelNoWrap(parent)  // delegating constructor
{
    m_label->setText(text);  // this is what the QLabel(text, parent) constructor does
}


ClickableLabelNoWrap::ClickableLabelNoWrap(QWidget* parent) :
    QPushButton(parent),
    m_label(new QLabel(this)),
    m_layout(new QVBoxLayout())
{
    m_label->setMouseTracking(false);
    m_label->setTextInteractionFlags(Qt::NoTextInteraction);
    m_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    // m_label->setObjectName(CssConst::DEBUG_GREEN);

    m_layout->setContentsMargins(uiconst::NO_MARGINS);

    m_layout->addWidget(m_label);

    setLayout(m_layout);
    // Default size policy is (QSizePolicy::Preferred, QSizePolicy::Preferred);
    // see qwidget.cpp
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
}


void ClickableLabelNoWrap::setTextFormat(const Qt::TextFormat format)
{
    m_label->setTextFormat(format);
}


void ClickableLabelNoWrap::setWordWrap(const bool on)
{
    m_label->setWordWrap(on);
    updateGeometry();
}


void ClickableLabelNoWrap::setAlignment(const Qt::Alignment alignment)
{
    m_label->setAlignment(alignment);
    m_layout->setAlignment(m_label, alignment);
}


void ClickableLabelNoWrap::setOpenExternalLinks(const bool open)
{
    m_label->setOpenExternalLinks(open);
}


void ClickableLabelNoWrap::setPixmap(const QPixmap& pixmap)
{
    m_label->setPixmap(pixmap);
    setFixedSize(pixmap.size());
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    updateGeometry();
}


QSize ClickableLabelNoWrap::sizeHint() const
{
    ensurePolished();
    QStyleOptionButton opt;
    initStyleOption(&opt);  // protected
    const QSize base_size = m_label->sizeHint();
    return base_size + sizehelpers::pushButtonExtraSizeRequired(this, &opt,
                                                                base_size);
}
