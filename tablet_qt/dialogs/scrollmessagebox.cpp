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

// http://www.qtforum.org/article/18183/messagebox-with-qscrollbar.html
// ... modified a bit

#define USE_CUSTOM_HFW
// #define ENFORCE_MINIMUM
// #define DEBUG_RESIZE_FOR_TITLE

#include "scrollmessagebox.h"
#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QFontMetrics>
#include <QGridLayout>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QSize>
#include <QStyle>
#include "common/preprocessor_aid.h"
#include "common/textconst.h"
#include "layouts/gridlayouthfw.h"
#include "lib/uifunc.h"
#include "widgets/verticalscrollarea.h"

#ifdef ENFORCE_MINIMUM
const QSize MIN_SIZE(600, 600);
// const QSize MAX_SIZE(1024, 1500);
#endif


// ============================================================================
// Constructor
// ============================================================================

ScrollMessageBox::ScrollMessageBox(const QMessageBox::Icon& icon,
                                   const QString& title,
                                   const QString& text,
                                   QWidget* parent) :
    QDialog(parent,
            Qt::Dialog | Qt::WindowTitleHint |
            Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint),
    m_clicked_button(nullptr)
{
    // Note: the default scroll area border is removed by main.css
    setWindowTitle(title);

    m_text_label = new QLabel(text);
    m_text_label->setWordWrap(true);
    m_text_label->setTextInteractionFlags(Qt::NoTextInteraction);
    // m_text_label->setTextInteractionFlags(Qt::TextInteractionFlags(
    //         style()->styleHint(
    //                 QStyle::SH_MessageBox_TextInteractionFlags, 0, this)));
    m_text_label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    m_text_label->setOpenExternalLinks(true);

#ifdef USE_CUSTOM_HFW
    auto scroll = new VerticalScrollArea(this);
#else
    auto scroll = new QScrollArea(this);
#endif
    scroll->setWidget(m_text_label);
    scroll->setWidgetResizable(true);
#ifdef ENFORCE_MINIMUM
    setMinimumSize(MIN_SIZE);
#else
    setMinimumSize(uifunc::minimumSizeForTitle(this));
    scroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // ... will shrink for small contents
#endif
    uifunc::applyScrollGestures(scroll->viewport());

    m_icon_label = new QLabel();
    setIcon(icon);

    m_button_box = new QDialogButtonBox();
    m_button_box->setCenterButtons(style()->styleHint(
                    QStyle::SH_MessageBox_CenterButtons, nullptr, this));
    QObject::connect(m_button_box, &QDialogButtonBox::clicked,
                     this, &ScrollMessageBox::handleButtonClicked);

#ifdef USE_CUSTOM_HFW
    auto grid = new GridLayoutHfw();
#else
    auto grid = new QGridLayout();
#endif

    /*
        ICON    { LABEL LABEL LABEL }
        ICON    { LABEL LABEL LABEL } in scroller
                { LABEL LABEL LABEL }

        BUTTONS BUTTONS BUTTONS BUTTONS
    */

    // addWidget(widget, row, col, row_span, col_span, alignment)
    grid->addWidget(m_icon_label, 0, 0, 1, 1, Qt::AlignTop);
    grid->addWidget(scroll,       0, 1, 1, 1);
    grid->addWidget(m_button_box, 1, 0, 1, 2);
#ifndef USE_CUSTOM_HFW
    grid->setSizeConstraint(QLayout::SetNoConstraint);
    // If you do this with a GridLayoutHfw, it's amusing, but not sensible;
    // you can drag the buttons *over* the label, for example.
#endif
    setLayout(grid);

    setModal(true);
}


// ============================================================================
// Public interface
// ============================================================================

void ScrollMessageBox::addButton(QAbstractButton* button,
                                 const QDialogButtonBox::ButtonRole role)
{
    m_button_box->addButton(button, role);
    // The button box TAKES OWNERSHIP:
    // http://doc.qt.io/qt-4.8/qdialogbuttonbox.html#addButton
    update();
}


void ScrollMessageBox::addButton(QAbstractButton* button,
                                 const QMessageBox::ButtonRole role)
{
    addButton(button, forceEnumMD(role));
}


QPushButton* ScrollMessageBox::addButton(
        const QString& text,
        const QDialogButtonBox::ButtonRole role)
{
    auto pushbutton = new QPushButton(text);
    addButton(pushbutton, role);
    return pushbutton;
}


QPushButton* ScrollMessageBox::addButton(const QString& text,
                                         const QMessageBox::ButtonRole role)
{
    return addButton(text, forceEnumMD(role));
}


void ScrollMessageBox::setDefaultButton(QPushButton* button)
{
    if (!m_button_box->buttons().contains(button)) {
        return;
    }
    // The button box's buttons() is a QList<QAbstractButton*>.
    button->setDefault(true);
    button->setFocus();
}


QAbstractButton* ScrollMessageBox::clickedButton() const
{
    return m_clicked_button;
}


// ============================================================================
// Internals
// ============================================================================

void ScrollMessageBox::setIcon(const QMessageBox::Icon icon)
{
    const QPixmap px = standardIcon(icon);
    m_icon_label->setPixmap(px);
    m_icon_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    update();
}


QPixmap ScrollMessageBox::standardIcon(const QMessageBox::Icon icon)
{
    QStyle* style = this->style();
    const int icon_size = style->pixelMetric(QStyle::PM_MessageBoxIconSize, nullptr, this);
    QIcon tmp_icon;
    switch (icon) {
    case QMessageBox::Information:
        tmp_icon = style->standardIcon(QStyle::SP_MessageBoxInformation, nullptr, this);
        break;
    case QMessageBox::Warning:
        tmp_icon = style->standardIcon(QStyle::SP_MessageBoxWarning, nullptr, this);
        break;
    case QMessageBox::Critical:
        tmp_icon = style->standardIcon(QStyle::SP_MessageBoxCritical, nullptr, this);
        break;
    case QMessageBox::Question:
        tmp_icon = style->standardIcon(QStyle::SP_MessageBoxQuestion, nullptr, this);
        break;
    case QMessageBox::NoIcon:
#ifdef COMPILER_WANTS_DEFAULT_IN_EXHAUSTIVE_SWITCH
    default:
#endif
        break;
    }
    if (!tmp_icon.isNull()) {
        return tmp_icon.pixmap(icon_size, icon_size);
    }
    return QPixmap();
}


void ScrollMessageBox::handleButtonClicked(QAbstractButton* button)
{
    m_clicked_button = button;
    const int ret = m_button_box->standardButton(button);
    done(ret);
}


QDialogButtonBox::ButtonRole ScrollMessageBox::forceEnumMD(
        const QMessageBox::ButtonRole role)
{
    // They are numerically identical:
    // - http://doc.qt.io/qt-4.8/qdialogbuttonbox.html#ButtonRole-enum
    // - http://doc.qt.io/qt-4.8/qmessagebox.html#ButtonRole-enum
    return static_cast<QDialogButtonBox::ButtonRole>(role);
}


QMessageBox::ButtonRole ScrollMessageBox::forceEnumDM(
        const QDialogButtonBox::ButtonRole role)
{
    return static_cast<QMessageBox::ButtonRole>(role);
}


// ============================================================================
// Static helper functions
// ============================================================================

QDialogButtonBox::StandardButton ScrollMessageBox::critical(
        QWidget* parent,
        const QString& title,
        const QString& text)
{
    ScrollMessageBox box(QMessageBox::Critical, title, text, parent);
    box.addButton(TextConst::ok(), QDialogButtonBox::YesRole);
    return static_cast<QDialogButtonBox::StandardButton>(box.exec());
}


QDialogButtonBox::StandardButton ScrollMessageBox::information(
        QWidget* parent,
        const QString& title,
        const QString& text)
{
    ScrollMessageBox box(QMessageBox::Information, title, text, parent);
    box.addButton(TextConst::ok(), QDialogButtonBox::YesRole);
    return static_cast<QDialogButtonBox::StandardButton>(box.exec());
}


QDialogButtonBox::StandardButton ScrollMessageBox::question(
        QWidget* parent,
        const QString& title,
        const QString& text)
{
    ScrollMessageBox box(QMessageBox::Question, title, text, parent);
    box.addButton(TextConst::ok(), QDialogButtonBox::YesRole);
    return static_cast<QDialogButtonBox::StandardButton>(box.exec());
}


QDialogButtonBox::StandardButton ScrollMessageBox::warning(
        QWidget* parent,
        const QString& title,
        const QString& text)
{
    ScrollMessageBox box(QMessageBox::Warning, title, text, parent);
    box.addButton(TextConst::ok(), QDialogButtonBox::YesRole);
    return static_cast<QDialogButtonBox::StandardButton>(box.exec());
}


QDialogButtonBox::StandardButton ScrollMessageBox::plain(
        QWidget* parent,
        const QString& title,
        const QString& text)
{
    ScrollMessageBox box(QMessageBox::NoIcon, title, text, parent);
    box.addButton(TextConst::ok(), QDialogButtonBox::YesRole);
    return static_cast<QDialogButtonBox::StandardButton>(box.exec());
}
