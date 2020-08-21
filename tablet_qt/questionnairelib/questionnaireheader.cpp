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

#include "questionnaireheader.h"
#include <QAbstractButton>
#include <QDebug>
#include <QPushButton>
#include "common/cssconst.h"
#include "common/uiconst.h"
#include "layouts/layouts.h"
#include "lib/sizehelpers.h"
#include "lib/uifunc.h"
#include "widgets/horizontalline.h"
#include "widgets/imagebutton.h"
#include "widgets/labelwordwrapwide.h"


QuestionnaireHeader::QuestionnaireHeader(QWidget* parent,
                                         const QString& title,
                                         const bool read_only,
                                         const bool offer_page_jump,
                                         const bool within_chain,
                                         const QString& css_name,
                                         const bool debug_allowed) :
    BaseWidget(parent),
    m_title(title),
    m_button_debug(nullptr),
    m_button_jump(nullptr),
    m_button_previous(nullptr),
    m_button_next(nullptr),
    m_button_finish(nullptr),
    m_icon_no_next(nullptr)
{
    if (!css_name.isEmpty()) {
        setObjectName(css_name);  // was not working! But works for e.g. cancel button below
        // Problem appears to be that WA_StyledBackground is not set.
        setAttribute(Qt::WidgetAttribute::WA_StyledBackground, true);

        // Following which discovery...
        // http://stackoverflow.com/questions/24653405/why-is-qwidget-with-border-and-background-image-styling-behaving-different-to-ql
        // ... suggests using QFrame
        // http://stackoverflow.com/questions/12655538/how-to-set-qwidget-background-color
        // ... suggests using setAutoFillBackground(true)
        // http://doc.qt.io/qt-5.7/qwidget.html#autoFillBackground-prop
        // ... advises caution with setAutoFillBackground() and stylesheets
    }
#ifndef QUESTIONNAIRE_HEADER_USE_HFW_BASE
    setSizePolicy(sizehelpers::expandingFixedHFWPolicy());  // if deriving from QWidget
#endif

    auto mainlayout = new VBoxLayout();
    setLayout(mainlayout);

    // ------------------------------------------------------------------------
    // Main row
    // ------------------------------------------------------------------------
    auto toprowlayout = new HBoxLayout();
    mainlayout->addLayout(toprowlayout);

    const Qt::Alignment button_align = Qt::AlignHCenter | Qt::AlignTop;
    const Qt::Alignment text_align = Qt::AlignHCenter | Qt::AlignVCenter;

    // Cancel button
    auto cancel = new ImageButton(uiconst::CBS_CANCEL);
    toprowlayout->addWidget(cancel, 0, button_align);
    connect(cancel, &QAbstractButton::clicked,
            this, &QuestionnaireHeader::cancelClicked);

    // Read-only icon
    if (read_only) {
        QLabel* read_only_icon = uifunc::iconWidget(
            uifunc::iconFilename(uiconst::ICON_READ_ONLY));
        toprowlayout->addWidget(read_only_icon, 0, button_align);
    }

    // Spacing
    toprowlayout->addStretch();

    // Title
    auto title_label = new LabelWordWrapWide(title);
    title_label->setAlignment(text_align);
    toprowlayout->addWidget(title_label, 0, text_align);
    // default alignment fills whole cell, but that looks better

    // Spacing
    toprowlayout->addStretch();

    // Right-hand icons/buttons
    if (debug_allowed) {
        m_button_debug = new QPushButton("Dump layout");
        connect(m_button_debug, &QAbstractButton::clicked,
                this, &QuestionnaireHeader::debugLayout);
        toprowlayout->addWidget(m_button_debug, 0, text_align);
    }

    m_button_previous = new ImageButton(uiconst::CBS_BACK);
    toprowlayout->addWidget(m_button_previous, 0, button_align);

    if (offer_page_jump) {
        m_button_jump = new ImageButton(uiconst::CBS_CHOOSE_PAGE);
        connect(m_button_jump, &QAbstractButton::clicked,
                this, &QuestionnaireHeader::jumpClicked);
        toprowlayout->addWidget(m_button_jump, 0, button_align);
    }

    m_button_next = new ImageButton(uiconst::CBS_NEXT);
    toprowlayout->addWidget(m_button_next, 0, button_align);

    if (within_chain) {
        m_button_finish = new ImageButton(uiconst::CBS_FAST_FORWARD);
    } else {
        m_button_finish = new ImageButton(uiconst::CBS_FINISH);
    }
    toprowlayout->addWidget(m_button_finish, 0, button_align);

    m_icon_no_next = uifunc::iconWidget(
        uifunc::iconFilename(uiconst::ICON_WARNING));
    toprowlayout->addWidget(m_icon_no_next, 0, button_align);

    setButtons(false, false, false);
    connect(m_button_previous, &QAbstractButton::clicked,
            this, &QuestionnaireHeader::previousClicked);
    connect(m_button_next, &QAbstractButton::clicked,
            this, &QuestionnaireHeader::nextClicked);
    connect(m_button_finish, &QAbstractButton::clicked,
            this, &QuestionnaireHeader::finishClicked);

    // ------------------------------------------------------------------------
    // Horizontal line
    // ------------------------------------------------------------------------
    auto horizline = new HorizontalLine(uiconst::HEADER_HLINE_WIDTH);
    horizline->setObjectName(cssconst::QUESTIONNAIRE_HORIZONTAL_LINE);
    mainlayout->addWidget(horizline);

}


void QuestionnaireHeader::setButtons(const bool previous,
                                     const bool next,
                                     const bool finish)
{
    m_button_previous->setVisible(previous);
    m_button_next->setVisible(next);
    m_button_finish->setVisible(finish);
    m_icon_no_next->setVisible(!next && !finish);
}


void QuestionnaireHeader::setFinishButtonIcon(const QString& base_filename)
{
    m_button_finish->setImages(base_filename);
}
