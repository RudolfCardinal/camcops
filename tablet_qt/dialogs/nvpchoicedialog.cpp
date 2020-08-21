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

#include "nvpchoicedialog.h"
#include <functional>
#include <QDialogButtonBox>
#include <QEvent>
#include <QLabel>
#include <QVariant>
#include <QWidget>
#include "common/uiconst.h"
#include "layouts/layouts.h"
#include "lib/sizehelpers.h"
#include "lib/uifunc.h"
#include "widgets/clickablelabelwordwrapwide.h"
#include "widgets/verticalscrollarea.h"

#if defined DEBUG_PRESS_A_TO_ADJUST_SIZE || defined DEBUG_PRESS_D_TO_DUMP_LAYOUT
    #include "qobjects/keypresswatcher.h"
#endif
#ifdef DEBUG_PRESS_D_TO_DUMP_LAYOUT
    #include "lib/layoutdumper.h"
    const layoutdumper::DumperConfig dumper_config;
#endif


NvpChoiceDialog::NvpChoiceDialog(QWidget* parent,
                                 const NameValueOptions& options,
                                 const QString& title) :
    QDialog(parent),
    m_options(options),
    m_title(title),
    m_show_existing_choice(false),
    m_p_new_value(nullptr),
    m_resized_to_contents(false)
{
}


void NvpChoiceDialog::showExistingChoice(const bool show_existing_choice,
                                         const QString& icon_filename,
                                         const QSize& icon_size)
{
    m_show_existing_choice = show_existing_choice;
    m_icon_filename = icon_filename;
    m_icon_size = icon_size;
}


int NvpChoiceDialog::choose(QVariant* new_value)
{
    if (!new_value) {
        return QDialog::DialogCode::Rejected;
    }
    m_p_new_value = new_value;
    setWindowTitle(m_title);
    const QVariant old_value = *m_p_new_value;

    m_resized_to_contents = false;

    auto contentwidget = new QWidget();  // doesn't need to be BaseWidget; contains scroll area
    auto contentlayout = new VBoxLayout();
    contentwidget->setLayout(contentlayout);
    for (int i = 0; i < m_options.size(); ++i) {
        const NameValuePair& nvp = m_options.at(i);
        auto label = new ClickableLabelWordWrapWide(nvp.name());
        label->setSizePolicy(sizehelpers::expandingFixedHFWPolicy());
        if (m_show_existing_choice) {
            const bool this_is_it = old_value == nvp.value();
            QLabel* icon = this_is_it
                    ? uifunc::iconWidget(m_icon_filename, nullptr, true, m_icon_size)
                    : uifunc::blankIcon(nullptr, m_icon_size);
            auto hlayout = new HBoxLayout();
            hlayout->addWidget(icon);
            hlayout->addWidget(label);
            contentlayout->addLayout(hlayout);
        } else {
            contentlayout->addWidget(label);
        }
        // Safe object lifespan signal: can use std::bind
        connect(label, &ClickableLabelWordWrapWide::clicked,
                std::bind(&NvpChoiceDialog::itemClicked, this, i));
    }

    auto scroll = new VerticalScrollArea();
    scroll->setWidget(contentwidget);

    auto mainlayout = new VBoxLayout();
    mainlayout->addWidget(scroll);
    setLayout(mainlayout);

    mainlayout->addStretch();

    // Offer a cancel button
    auto standard_buttons = new QDialogButtonBox(QDialogButtonBox::Cancel);
    connect(standard_buttons, &QDialogButtonBox::rejected,
            this, &NvpChoiceDialog::reject);
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

    // no help: // adjustSize();

    return exec();
}


void NvpChoiceDialog::itemClicked(const int index)
{
    if (!m_p_new_value) {
        return;
    }
    *m_p_new_value = m_options.value(index);
    accept();
}


bool NvpChoiceDialog::event(QEvent* e)
{
    const bool result = QDialog::event(e);
    const QEvent::Type type = e->type();
    // Manual adjustment works perfectly.
    // Show event not enough.
    // Calling adjustSize() twice made it jump between monitors, not resize.
    // Adding these didn't help:
    //      ShowParent
    //      Polish
    //      PolishRequest
    // However, doing it *once* following WindowActivate does help.
    // (Most useful thing here was to show all events!)

    // qDebug() << type;

    if (type == QEvent::Type::WindowActivate) {
        // qDebug() << Q_FUNC_INFO << "Calling adjustSize()";
        if (!m_resized_to_contents) {  // do this once only:
            adjustSize();
            m_resized_to_contents = true;
        }
    }
    return result;
}
