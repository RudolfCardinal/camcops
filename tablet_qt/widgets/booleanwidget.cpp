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

#include "booleanwidget.h"
#include <QDebug>
#include <QPainter>
#include <QStyle>
#include <QVariant>
#include "common/cssconst.h"
#include "common/preprocessor_aid.h"
#include "common/uiconst.h"
#include "lib/sizehelpers.h"
#include "lib/uifunc.h"
#include "widgets/clickablelabelwordwrapwide.h"
#include "widgets/imagebutton.h"

const QString CHECK_DISABLED("check_disabled.png");
const QString CHECK_UNSELECTED("check_unselected.png");
const QString CHECK_UNSELECTED_REQUIRED("check_unselected_required.png");
const QString CHECK_FALSE_BLACK("check_false_black.png");
const QString CHECK_FALSE_RED("check_false_red.png");
const QString CHECK_TRUE_BLACK("check_true_black.png");
const QString CHECK_TRUE_RED("check_true_red.png");

const QString RADIO_DISABLED("radio_disabled.png");
const QString RADIO_UNSELECTED("radio_unselected.png");
const QString RADIO_UNSELECTED_REQUIRED("radio_unselected_required.png");
const QString RADIO_SELECTED("radio_selected.png");


BooleanWidget::BooleanWidget(QWidget* parent) :
    QAbstractButton(parent),
    m_read_only(false),
    m_big(true),
    m_bold(false),
    m_appearance(Appearance::CheckRed),
    m_as_image(true),
    m_state(State::Null)
{
    m_imagebutton = new ImageButton();
    m_textbutton = new ClickableLabelWordWrapWide();
    m_layout = new VBoxLayout();
    m_layout->setContentsMargins(uiconst::NO_MARGINS);
    m_layout->addWidget(m_imagebutton);
    m_layout->addWidget(m_textbutton);
    setLayout(m_layout);

    connect(m_imagebutton, &ImageButton::clicked,
            this, &BooleanWidget::clicked);
    connect(m_textbutton, &ClickableLabelWordWrapWide::clicked,
            this, &BooleanWidget::clicked);

    updateWidget(true);
}


void BooleanWidget::setReadOnly(const bool read_only)
{
    if (read_only != m_read_only) {
        m_read_only = read_only;
        updateWidget(false);
    }
}


void BooleanWidget::setSize(const bool big)
{
    if (big != m_big) {
        m_big = big;
        updateWidget(true);
    }
}


void BooleanWidget::setBold(const bool bold)
{
    if (bold != m_bold) {
        m_bold = bold;
        updateWidget(true);
    }
}


void BooleanWidget::setAppearance(const BooleanWidget::Appearance appearance)
{
    if (appearance != m_appearance) {
        m_appearance = appearance;
        m_as_image = (appearance != Appearance::Text);
        updateWidget(true);
    }
}


void BooleanWidget::setValue(const QVariant& value, const bool mandatory,
                             const bool disabled)
{
    if (disabled) {
        setState(State::Disabled);
    } else if (value.isNull()) {
        setState(mandatory ? State::NullRequired : State::Null);
    } else if (value.toBool()) {
        setState(State::True);
    } else {
        setState(State::False);
    }
}


void BooleanWidget::setState(const BooleanWidget::State state)
{
    if (state != m_state) {
        m_state = state;
        updateWidget(false);
    }
}


void BooleanWidget::updateWidget(const bool full_refresh)
{
    QString img;
    switch (m_appearance) {
    case Appearance::CheckBlack:
        switch (m_state) {
        case State::Disabled:
            img = CHECK_DISABLED;
            break;
        case State::Null:
            img = CHECK_UNSELECTED;
            break;
        case State::NullRequired:
            img = CHECK_UNSELECTED_REQUIRED;
            break;
        case State::False:
            img = CHECK_FALSE_BLACK;
            break;
        case State::True:
            img = CHECK_TRUE_BLACK;
            break;
        }
        break;

    case Appearance::CheckBlackFalseAppearsBlank:
        switch (m_state) {
        case State::Disabled:
            img = CHECK_DISABLED;
            break;
        case State::Null:
            img = CHECK_UNSELECTED;
            break;
        case State::NullRequired:
            img = CHECK_UNSELECTED_REQUIRED;
            break;
        case State::False:
            img = CHECK_UNSELECTED;  // difference from CheckBlack
            break;
        case State::True:
            img = CHECK_TRUE_BLACK;
            break;
        }
        break;

    case Appearance::CheckRed:
#ifdef COMPILER_WANTS_DEFAULT_IN_EXHAUSTIVE_SWITCH
    default:
#endif
        switch (m_state) {
        case State::Disabled:
            img = CHECK_DISABLED;
            break;
        case State::Null:
            img = CHECK_UNSELECTED;
            break;
        case State::NullRequired:
            img = CHECK_UNSELECTED_REQUIRED;
            break;
        case State::False:
            img = CHECK_FALSE_RED;
            break;
        case State::True:
            img = CHECK_TRUE_RED;
            break;
        }
        break;

    case Appearance::CheckRedFalseAppearsBlank:
        switch (m_state) {
        case State::Disabled:
            img = CHECK_DISABLED;
            break;
        case State::Null:
            img = CHECK_UNSELECTED;
            break;
        case State::NullRequired:
            img = CHECK_UNSELECTED_REQUIRED;
            break;
        case State::False:
            img = CHECK_UNSELECTED;  // difference from CheckRed
            break;
        case State::True:
            img = CHECK_TRUE_RED;
            break;
        }
        break;

    case Appearance::Radio:
        switch (m_state) {
        case State::Disabled:
            img = RADIO_DISABLED;
            break;
        case State::Null:
            img = RADIO_UNSELECTED;
            break;
        case State::NullRequired:
            img = RADIO_UNSELECTED_REQUIRED;
            break;
        case State::False:
            // not so meaningful
            img = RADIO_UNSELECTED;
            break;
        case State::True:
            img = RADIO_SELECTED;
            break;
        }
        break;

    case Appearance::Text:
        // http://wiki.qt.io/DynamicPropertiesAndStylesheets
        {
            QString css = uifunc::textCSS(-1, m_bold, false);
            m_textbutton->setStyleSheet(css);

            switch (m_state) {
            case State::Disabled:
                m_textbutton->setProperty(cssconst::PROPERTY_STATE,
                                          cssconst::VALUE_DISABLED);
                break;
            case State::Null:
                m_textbutton->setProperty(cssconst::PROPERTY_STATE,
                                          cssconst::VALUE_NULL);
                break;
            case State::NullRequired:
                m_textbutton->setProperty(cssconst::PROPERTY_STATE,
                                          cssconst::VALUE_NULL_REQUIRED);
                break;
            case State::False:
                m_textbutton->setProperty(cssconst::PROPERTY_STATE,
                                          cssconst::VALUE_FALSE);
                break;
            case State::True:
                m_textbutton->setProperty(cssconst::PROPERTY_STATE,
                                          cssconst::VALUE_TRUE);
                break;
            }
            m_textbutton->setProperty(cssconst::PROPERTY_READ_ONLY,
                                      uifunc::cssBoolean(m_read_only));
        }
        break;
    }
    if (m_as_image) {
        if (full_refresh) {
            m_imagebutton->setVisible(true);
            m_textbutton->setVisible(false);
            m_imagebutton->setImageSize(m_big ? uiconst::g_iconsize
                                              : uiconst::g_small_iconsize);
            setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        }
        m_imagebutton->setImages(img, true, false, false, false, m_read_only);
        // ... don't alter unpressed images
        // ... FOR NOW, put pressed marker on top (as PNGs are not transparent
        //     inside the check boxes etc.)
    } else {  // Text
        if (full_refresh) {
            m_imagebutton->setVisible(false);
            m_textbutton->setVisible(true);
            setSizePolicy(sizehelpers::maximumFixedHFWPolicy());
        }
        uifunc::repolish(m_textbutton);
    }
    if (full_refresh) {
        updateGeometry();
    } else {
        update();
    }
}


void BooleanWidget::setText(const QString& text)
{
    // qDebug() << Q_FUNC_INFO << text;
    m_textbutton->setText(text);
    if (m_appearance == Appearance::Text) {
        updateGeometry();  // text change often implies size change
    }
}


void BooleanWidget::paintEvent(QPaintEvent* e)
{
    Q_UNUSED(e)
    /*
    // To draw child widgets explicitly, use render (since paintEvent is
    // protected).
    // http://stackoverflow.com/questions/18042969
    QPainter painter(this);
    if (m_as_image) {
        m_imagebutton->render(&painter);
    } else {
        m_textbutton->render(&painter);
    }
    // However, our child widgets draw themselves anyway.
    // We just have to implement this function somehow as QAbstractButton is
    // an abstract base class.
    */
}
