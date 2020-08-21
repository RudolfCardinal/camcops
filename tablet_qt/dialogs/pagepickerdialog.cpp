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

// #define DEBUG_PRESS_A_TO_ADJUST_SIZE
// #define DEBUG_PRESS_D_TO_DUMP_LAYOUT

#include "pagepickerdialog.h"
#include <functional>  // for std::bind
#include <QDialogButtonBox>
#include <QEvent>
#include <QVBoxLayout>
#include "layouts/layouts.h"
#include "lib/sizehelpers.h"
#include "lib/uifunc.h"
#include "widgets/clickablelabelwordwrapwide.h"
#include "widgets/imagebutton.h"
#include "widgets/verticalscrollarea.h"

#if defined DEBUG_PRESS_A_TO_ADJUST_SIZE || defined DEBUG_PRESS_D_TO_DUMP_LAYOUT
    #include "qobjects/keypresswatcher.h"
#endif
#ifdef DEBUG_PRESS_D_TO_DUMP_LAYOUT
    #include "lib/layoutdumper.h"
    const layoutdumper::DumperConfig dumper_config;
#endif


PagePickerDialog::PagePickerDialog(QWidget* parent,
                                   const PagePickerItemList& pages,
                                   const QString& title) :
    QDialog(parent),
    m_pages(pages),
    m_title(title),
    m_new_page_number(nullptr),
    m_resized_to_contents(false)
{
}


int PagePickerDialog::choose(int* new_page_number)
{
    if (!new_page_number) {
        return QDialog::DialogCode::Rejected;
    }
    m_new_page_number = new_page_number;
    setWindowTitle(m_title);
    setMinimumSize(uifunc::minimumSizeForTitle(this));

    auto contentwidget = new QWidget();  // doesn't need to be BaseWidget; contains scroll area
    auto contentlayout = new VBoxLayout();
    contentwidget->setLayout(contentlayout);
    for (int i = 0; i < m_pages.size(); ++i) {
        const PagePickerItem& page = m_pages.at(i);
        auto itemlayout = new HBoxLayout();

        auto label = new ClickableLabelWordWrapWide(page.text());
        label->setSizePolicy(sizehelpers::expandingFixedHFWPolicy());
        itemlayout->addWidget(label);

        auto icon = new ImageButton(page.iconFilename());
        itemlayout->addWidget(icon);

        contentlayout->addLayout(itemlayout);

        // Safe object lifespan signal: can use std::bind
        connect(label, &ClickableLabelWordWrapWide::clicked,
                std::bind(&PagePickerDialog::itemClicked, this, i));
        connect(icon, &ImageButton::clicked,
                std::bind(&PagePickerDialog::itemClicked, this, i));
    }

    auto scroll = new VerticalScrollArea();
    scroll->setWidget(contentwidget);

    auto mainlayout = new QVBoxLayout();  // does not need to adjust height to contents; contains scroll area
    mainlayout->addWidget(scroll);
    setLayout(mainlayout);

    mainlayout->addStretch();

    // Offer a cancel button
    auto standard_buttons = new QDialogButtonBox(QDialogButtonBox::Cancel);
    connect(standard_buttons, &QDialogButtonBox::rejected,
            this, &PagePickerDialog::reject);
    mainlayout->addWidget(standard_buttons);

#if defined DEBUG_PRESS_A_TO_ADJUST_SIZE || defined DEBUG_PRESS_D_TO_DUMP_LAYOUT
    auto keywatcher = new KeyPressWatcher(this);
#endif
#ifdef DEBUG_PRESS_A_TO_ADJUST_SIZE
    keywatcher->addKeyEvent(
        Qt::Key_A,
        std::bind(&QWidget::adjustSize, this));
#endif
#ifdef DEBUG_PRESS_D_TO_DUMP_LAYOUT
    // keywatcher becomes child of this,
    // and layoutdumper is a namespace, so:
    // Safe object lifespan signal: can use std::bind
    keywatcher->addKeyEvent(
        Qt::Key_D,
        std::bind(&layoutdumper::dumpWidgetHierarchy, this, dumper_config));
#endif

    m_resized_to_contents = false;
    return exec();
}


void PagePickerDialog::itemClicked(const int item_index)
{
    if (!m_new_page_number) {
        return;
    }
    const PagePickerItem& page = m_pages.at(item_index);
    if (!page.selectable()) {
        uifunc::alert(
            tr("You can’t select this page yet because preceding pages "
               "(marked with a warning symbol) are incomplete."),
            tr("Complete preceding pages first.")
        );
        return;
    }
    *m_new_page_number = page.pageNumber();
    accept();
}


bool PagePickerDialog::event(QEvent* e)
{
    // See NvpChoiceDialog::event().

    const bool result = QDialog::event(e);
    const QEvent::Type type = e->type();
    if (type == QEvent::Type::WindowActivate) {
        if (!m_resized_to_contents) {  // do this once only:
            adjustSize();
            m_resized_to_contents = true;
        }
    }
    return result;
}
