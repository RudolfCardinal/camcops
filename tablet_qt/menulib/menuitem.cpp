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

// #define DEBUG_VERBOSE
// #define DEBUG_SHOW_PATIENT_INFO_IN_DESCRIPTION

#include "menuitem.h"
#include <QDebug>
#include <QLabel>
#include <QPushButton>
#include <QScopedPointer>
#include <QSize>
#include <QUrl>
#include "core/camcopsapp.h"
#include "common/cssconst.h"
#include "common/uiconst.h"
#include "dbobjects/patient.h"
#include "layouts/layouts.h"
#include "lib/convert.h"
#include "lib/datetime.h"  // for SHORT_DATETIME_FORMAT
#include "lib/idpolicy.h"
#include "lib/uifunc.h"
#include "menu/choosepatientmenu.h"
#include "menu/singletaskmenu.h"
#include "menulib/htmlinfowindow.h"
#include "menulib/menuwindow.h"
#include "tasklib/taskchain.h"
#include "tasklib/taskfactory.h"
#include "widgets/basewidget.h"
#include "widgets/labelwordwrapwide.h"
#include "widgets/openablewidget.h"

const int STRETCH_3COL_WTASKNAME_TASKNAME = 2;
const int STRETCH_3COL_WTASKNAME_TIMESTAMP = 2;
const int STRETCH_3COL_WTASKNAME_SUMMARY = 7;

const int STRETCH_3COL_WPATIENT_PATIENT = 3;
const int STRETCH_3COL_WPATIENT_TIMESTAMP = 2;
const int STRETCH_3COL_WPATIENT_SUMMARY = 7;

const int STRETCH_2COL_TIMESTAMP = 2;
const int STRETCH_2COL_SUMMARY = 8;


MenuItem::MenuItem() :
    m_title("?")
{
    setDefaults();
}


MenuItem::MenuItem(const QString& title) :
    m_title(title)
{
    // this constructor used for placeholders for not-implemented stuff
    setDefaults();
    m_implemented = false;
}


MenuItem::MenuItem(const QString& title, const MenuItem::ActionFunction& func,
                   const QString& icon, const QString& subtitle) :
    m_title(title)
{
    setDefaults();
    m_func = func;
    m_icon = icon;
    m_subtitle = subtitle;
}


MenuItem::MenuItem(const QString& title,
                   const MenuItem::OpenableWidgetMaker& func,
                   const QString& icon, const QString& subtitle) :
    m_title(title)
{
    setDefaults();
    m_openable_widget_maker = func;
    m_icon = icon;
    m_subtitle = subtitle;
}


MenuItem::MenuItem(MenuProxyPtr p_menuproxy, CamcopsApp& app)
    // m_title: below
{
    setDefaults();
    m_p_menuproxy = p_menuproxy;

    QScopedPointer<MenuWindow> mw(m_p_menuproxy->create(app));
    m_title = mw->title();
    m_subtitle = mw->subtitle();
    m_icon = mw->icon();
}


MenuItem::MenuItem(const ChoosePatientMenuItem& choose_patient,
                   CamcopsApp& app)
{
    // We do some more of the work for the client, since "choose patient"
    // appears on lots of menus.
    Q_UNUSED(choose_patient)  // it's just a marker object
    // You can't call another constructor directly, so...

    setDefaults();
    m_p_menuproxy = MenuProxyPtr(new MenuProxy<ChoosePatientMenu>);

    QScopedPointer<MenuWindow> mw(m_p_menuproxy->create(app));
    m_title = mw->title();
    m_subtitle = mw->subtitle();
    m_icon = mw->icon();
}


MenuItem::MenuItem(const TaskMenuItem& taskmenuitem, CamcopsApp& app)
    // m_title: below
{
    setDefaults();
    m_task_tablename = taskmenuitem.tablename;

    TaskPtr task = app.taskFactory()->create(m_task_tablename);
    if (task == nullptr) {
        m_title = tr("UNKNOWN TASK") + ": " + taskmenuitem.tablename;
        m_implemented = false;
        return;
    }
    m_title = task->menutitle();
    m_subtitle = task->menusubtitle();
    if (task->isAnonymous()) {
        m_icon = uifunc::iconFilename(uiconst::ICON_ANONYMOUS);
    }
}


MenuItem::MenuItem(const TaskChainMenuItem& chain)
{
    setDefaults();
    m_p_taskchain = chain.chain;
    m_title = m_p_taskchain->title();
    m_subtitle = m_p_taskchain->subtitle();
    m_icon = uifunc::iconFilename(uiconst::ICON_CHAIN);
}


MenuItem::MenuItem(const QString& title, const HtmlMenuItem& htmlmenuitem,
                   const QString& icon, const QString& subtitle) :
    m_title(title)
{
    setDefaults();
    m_html_item = htmlmenuitem;  // extra
    m_icon = icon;
    m_subtitle = subtitle;
}


MenuItem::MenuItem(const QString& title, const UrlMenuItem& urlmenuitem,
                   const QString& icon, const QString& subtitle) :
    m_title(title)
{
    setDefaults();
    m_url_item = urlmenuitem;  // extra
    m_icon = icon;
    m_subtitle = subtitle;
}


MenuItem::MenuItem(TaskPtr p_task, const bool task_shows_taskname,
                   const bool task_shows_patient) :
    m_title("?")
{
    setDefaults();
    m_p_task = p_task;
    m_task_shows_taskname = task_shows_taskname;
    m_task_shows_patient = task_shows_patient;
}


MenuItem::MenuItem(PatientPtr p_patient)
{
    setDefaults();
    m_p_patient = p_patient;
#ifdef DEBUG_VERBOSE
    qDebug() << Q_FUNC_INFO << this;
#endif
}


void MenuItem::setDefaults()
{
    // Not the most efficient, but saves lots of duplication

    // not m_title
    m_subtitle = "";
    m_icon = "";

    m_arrow_on_right = false;
    m_chain = false;
    m_copyright_details_pending = false;
    m_implemented = true;
    m_label_only = false;
    m_needs_privilege = false;
    m_not_if_locked = false;
    m_task_shows_taskname = false;
    m_task_shows_patient = false;
    m_unsupported = false;

    m_func = nullptr;
    m_openable_widget_maker = nullptr;
    m_p_menuproxy.clear();
    m_task_tablename = "";
    m_p_task.clear();
    m_p_taskchain.clear();
    m_p_patient.clear();
}


QString MenuItem::title() const
{
    if (m_p_task) {
        return m_p_task->instanceTitle();
    }
    return m_title;
}


QString MenuItem::subtitle() const
{
    return m_subtitle;
}


TaskPtr MenuItem::task() const
{
    return m_p_task;
}


PatientPtr MenuItem::patient() const
{
#ifdef DEBUG_VERBOSE
    qDebug() << Q_FUNC_INFO << this;
#endif
    return m_p_patient;
}


QWidget* MenuItem::rowWidget(CamcopsApp& app) const
{
    Qt::Alignment text_align = Qt::AlignLeft | Qt::AlignVCenter;
    QSizePolicy sp_icon(QSizePolicy::Fixed, QSizePolicy::Fixed);

    auto row = new BaseWidget();
    auto rowlayout = new HBoxLayout();
    row->setLayout(rowlayout);

    if (m_p_task) {
        // --------------------------------------------------------------------
        // Task instance
        // --------------------------------------------------------------------
        // Stretch: http://stackoverflow.com/questions/14561516/qt-qhboxlayout-percentage-size
        //
        // ICON | ICON | +----------------------------------------------------+
        // ICON | ICON | | A                   B                 C            |
        // ICON | ICON | |                                                    |
        // ICON | ICON | +----------------------------------------------------+
        //
        // ... where BOXES is either a two- or a three-column layout
        // Layout within that is a bit tricky. We want to specify the stretches
        // so columns are in a fixed proportion. However, labels don't
        // themselves expand beyond what's necessary, and the fixed-stretch
        // method multiplies everything by a fixed amount, so we encapsulate
        // all the text elements in their own QHBoxLayout with its own
        // addStretch(), then stretch-multiply up those (which requires
        // encapsulating *those* in another QWidget...).

        const bool complete = m_p_task->isCompleteCached();
        int timestamp_stretch = STRETCH_2COL_TIMESTAMP;
        int summary_stretch = STRETCH_2COL_SUMMARY;

        // Notification of "incomplete" status
        QLabel* incomplete_icon = complete
                ? uifunc::blankIcon()
                : uifunc::iconWidget(uifunc::iconFilename(uiconst::ICON_WARNING));
        incomplete_icon->setSizePolicy(sp_icon);
        rowlayout->addWidget(incomplete_icon);

        // Move-off item, if selected (only applicable to anonymous tasks)
        if (m_p_task->isAnonymous()) {
            QLabel* icon = m_p_task->shouldMoveOffTablet()
                    ? uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_FINISHFLAG))
                    : uifunc::blankIcon();
            QSizePolicy sp_icon(QSizePolicy::Fixed, QSizePolicy::Fixed);
            icon->setSizePolicy(sp_icon);
            rowlayout->addWidget(icon);
        }

        // Taskname OR patient
        if (m_task_shows_taskname || m_task_shows_patient) {
            auto col1_widget = new BaseWidget();
            auto col1_hbox = new QHBoxLayout();
            col1_widget->setLayout(col1_hbox);

            int firstcol_stretch = STRETCH_3COL_WTASKNAME_TASKNAME;
            QString contents;
            if (m_task_shows_taskname) {
                contents = m_p_task->shortname();
                timestamp_stretch = STRETCH_3COL_WTASKNAME_TIMESTAMP;
                summary_stretch = STRETCH_3COL_WTASKNAME_SUMMARY;
            } else {
                if (m_p_task->isAnonymous()) {
                    contents = tr("<Anonymous task>");
                } else {
                    Patient* pt = m_p_task->patient();
                    if (pt) {
                        contents = pt->surnameUpperForename();
                    }
                }
                firstcol_stretch = STRETCH_3COL_WPATIENT_PATIENT;
                timestamp_stretch = STRETCH_3COL_WPATIENT_TIMESTAMP;
                summary_stretch = STRETCH_3COL_WPATIENT_SUMMARY;
            }
            QLabel* taskname = new LabelWordWrapWide(contents);
            taskname->setAlignment(text_align);
            taskname->setObjectName(complete
                                    ? cssconst::TASK_ITEM_TASKNAME_COMPLETE
                                    : cssconst::TASK_ITEM_TASKNAME_INCOMPLETE);

            col1_hbox->addWidget(taskname);
            col1_hbox->addStretch();
            QSizePolicy sp_taskname(QSizePolicy::Preferred,
                                    QSizePolicy::Preferred);
            sp_taskname.setHorizontalStretch(firstcol_stretch);
            col1_widget->setSizePolicy(sp_taskname);
            rowlayout->addWidget(col1_widget);
        }

        // Timestamp
        auto col2_widget = new BaseWidget();
        auto col2_hbox = new QHBoxLayout();
        col2_widget->setLayout(col2_hbox);

        QLabel* timestamp = new LabelWordWrapWide(
            m_p_task->whenCreated().toString(datetime::SHORT_DATETIME_FORMAT));
        timestamp->setAlignment(text_align);
        timestamp->setObjectName(complete
                                 ? cssconst::TASK_ITEM_TIMESTAMP_COMPLETE
                                 : cssconst::TASK_ITEM_TIMESTAMP_INCOMPLETE);

        col2_hbox->addWidget(timestamp);
        col2_hbox->addStretch();
        QSizePolicy sp_timestamp(QSizePolicy::Preferred,
                                 QSizePolicy::Preferred);
        sp_timestamp.setHorizontalStretch(timestamp_stretch);
        col2_widget->setSizePolicy(sp_timestamp);
        rowlayout->addWidget(col2_widget);

        // Summary
        auto col3_widget = new BaseWidget();
        auto col3_hbox = new QHBoxLayout();
        col3_widget->setLayout(col3_hbox);

        QLabel* summary = new LabelWordWrapWide(
                (m_p_task->completenessInfo() + m_p_task->summary()).join(" "));
        summary->setAlignment(text_align);
        summary->setObjectName(complete
                               ? cssconst::TASK_ITEM_SUMMARY_COMPLETE
                               : cssconst::TASK_ITEM_SUMMARY_INCOMPLETE);

        col3_hbox->addWidget(summary);
        col3_hbox->addStretch();
        QSizePolicy sp_summary(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sp_summary.setHorizontalStretch(summary_stretch);
        col3_widget->setSizePolicy(sp_summary);
        rowlayout->addWidget(col3_widget);

    } else if (m_p_patient) {
        // --------------------------------------------------------------------
        // Patient (for patient-choosing menu)
        // --------------------------------------------------------------------
        //
        // ICON | ICON | - SURNAME, Forename
        // ICON | ICON | - Sex, age, DOB
        // ICON | ICON | - ID numbers

        // Title/subtitle style
        auto textlayout = new VBoxLayout();

        const QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Preferred);

        auto title = new LabelWordWrapWide(m_p_patient->surnameUpperForename());
        auto subtitle1 = new LabelWordWrapWide(m_p_patient->sexAgeDob());
        auto subtitle2 = new LabelWordWrapWide(m_p_patient->shortIdnumSummary());

        title->setAlignment(text_align);
        subtitle1->setAlignment(text_align);
        subtitle2->setAlignment(text_align);
        title->setObjectName(cssconst::MENU_ITEM_TITLE);
        subtitle1->setObjectName(cssconst::MENU_ITEM_SUBTITLE);
        subtitle2->setObjectName(cssconst::MENU_ITEM_SUBTITLE);
        title->setSizePolicy(sp);
        subtitle1->setSizePolicy(sp);
        subtitle2->setSizePolicy(sp);
        textlayout->addWidget(title);
        textlayout->addWidget(subtitle1);
        textlayout->addWidget(subtitle2);

        // Patient: stop/warning/finishflag/blank
        // ... "stop" => can't upload => finishflag irrelevant
        // ... "warning" => can't finalize => finishflag irrelevant
        // However, it's confusing if you can press the "finishflag" button
        // and nothing appears to happen (because your patient's in the "stop"
        // or "warning" state). So... allow a slightly mis-aligned but more
        // informative state if a patient has >1 relevant icon.
        int n_icons = 0;
        if (!m_p_patient->compliesWith(app.uploadPolicy()) ||
                m_p_patient->anyIdClash()) {
            rowlayout->addWidget(uifunc::iconWidget(
                    uifunc::iconFilename(uiconst::ICON_STOP)));
            ++n_icons;
        } else if (!m_p_patient->compliesWith(app.finalizePolicy())) {
            rowlayout->addWidget(uifunc::iconWidget(
                    uifunc::iconFilename(uiconst::ICON_WARNING)));
            ++n_icons;
        }
        if (m_p_patient->shouldMoveOffTablet()) {
            rowlayout->addWidget(uifunc::iconWidget(
                    uifunc::iconFilename(uiconst::CBS_FINISHFLAG)));
            ++n_icons;
        }
        if (n_icons == 0) {
            // For alignment, have at least one:
            rowlayout->addWidget(uifunc::blankIcon());
        }

        rowlayout->addLayout(textlayout);
        rowlayout->addStretch();

    } else {
        // --------------------------------------------------------------------
        // Conventional menu item
        // --------------------------------------------------------------------
        //
        // ICON | - Title                                           | childicon
        // ICON | - Subtitle                                        | childicon
        // ICON |                                                   | childicon

        // Icon
        if (!m_label_only) {  // Labels go full-left
            if (!m_icon.isEmpty()) {
                QLabel* icon = uifunc::iconWidget(m_icon);
                rowlayout->addWidget(icon);
            } else if (m_chain) {
                QLabel* icon = uifunc::iconWidget(
                    uifunc::iconFilename(uiconst::ICON_CHAIN));
                rowlayout->addWidget(icon);
            } else {
                rowlayout->addWidget(uifunc::blankIcon());
            }
        }

        // Title/subtitle
        auto textlayout = new VBoxLayout();

        auto title = new LabelWordWrapWide(m_title);
        title->setAlignment(text_align);
        title->setObjectName(cssconst::MENU_ITEM_TITLE);
        textlayout->addWidget(title);
        if (!m_subtitle.isEmpty()) {
            auto subtitle = new LabelWordWrapWide(m_subtitle);
            subtitle->setAlignment(text_align);
            subtitle->setObjectName(cssconst::MENU_ITEM_SUBTITLE);
            textlayout->addWidget(subtitle);
        }
        rowlayout->addLayout(textlayout);
        rowlayout->addStretch();

        // Arrow on right
        if (m_arrow_on_right) {
            QLabel* iconLabel = uifunc::iconWidget(
                uifunc::iconFilename(uiconst::ICON_HASCHILD),
                nullptr, false);
            rowlayout->addWidget(iconLabel);
        }

        // Background colour, via stylesheets
        if (m_label_only) {
            row->setObjectName(cssconst::LABEL_ONLY);
        } else if (!m_implemented) {
            row->setObjectName(cssconst::NOT_IMPLEMENTED);
        } else if (m_unsupported) {
            row->setObjectName(cssconst::UNSUPPORTED);
        } else if (m_not_if_locked && app.locked()) {
            row->setObjectName(cssconst::LOCKED);
        } else if (m_needs_privilege && !app.privileged()) {
            row->setObjectName(cssconst::NEEDS_PRIVILEGE);
        }
        // ... but not for locked/needs privilege, as otherwise we'd need
        // to refresh the whole menu? Well, we could try it.
        // On Linux desktop, it's extremely fast.
    }

    // Size policy
    // ... NO; now done automatically by HeightForWidthLayoutContainer
    // QSizePolicy size_policy(QSizePolicy::MinimumExpanding,  // horizontal
    //                         QSizePolicy::Fixed);  // vertical
    // row->setSizePolicy(size_policy);

    return row;
}


void MenuItem::act(CamcopsApp& app) const
{
    // ========================================================================
    // Reasons to refuse
    // ========================================================================
    if (m_label_only) {
        // qDebug() << "Label-only row touched; ignored";
        return;
    }
    if (m_p_task) {
        // Handled via verb buttons instead.
        return;
    }
    if (m_p_taskchain) {
        if (!uifunc::confirm(TextConst::startChainQuestion(),
                             TextConst::startChainTitle(),
                             TextConst::yes(), TextConst::no())) {
            return;
        }
        m_p_taskchain->start();
        return;
    }
    if (!m_implemented) {
        uifunc::alert(tr("Not implemented yet!"));
        return;
    }
    if (m_unsupported) {
        uifunc::alert(tr("Not supported on this platform!"));
        return;
    }
    if (m_needs_privilege && !app.privileged()) {
        uifunc::alert(tr("You must set Privileged Mode first"));
        return;
    }
    if (m_not_if_locked && app.locked()) {
        uifunc::alertNotWhenLocked();
        return;
    }
    // ========================================================================
    // Ways to act
    // ========================================================================
    if (m_p_menuproxy) {
        MenuWindow* pWindow = m_p_menuproxy->create(app);
        app.openSubWindow(pWindow);
        return;
    }
    if (m_func) {
        m_func();
        return;
    }
    if (m_openable_widget_maker) {
        OpenableWidget* widget = m_openable_widget_maker(app);
        app.openSubWindow(widget);
        return;
    }
    if (!m_task_tablename.isEmpty()) {
        auto pWindow = new SingleTaskMenu(m_task_tablename, app);
        app.openSubWindow(pWindow);
        return;
    }
    if (!m_html_item.filename.isEmpty()) {
        auto pWindow = new HtmlInfoWindow(
            app, m_html_item.title, m_html_item.filename,
            m_html_item.icon, m_html_item.fullscreen);
        app.openSubWindow(pWindow);
        return;
    }
    if (!m_url_item.url.isEmpty()) {
        uifunc::visitUrl(m_url_item.url);
        return;
    }
    qWarning() << "Menu item selected but no action specified:"
               << m_title;
}


bool MenuItem::isImplemented() const
{
    return m_implemented;
}


MenuItem& MenuItem::setImplemented(const bool implemented)
{
    m_implemented = implemented;
    return *this;
}


MenuItem& MenuItem::setLabelOnly(const bool label_only)
{
    m_label_only = label_only;
    return *this;
}


MenuItem& MenuItem::setNeedsPrivilege(const bool needs_privilege)
{
    m_needs_privilege = needs_privilege;
    if (needs_privilege) {
        m_not_if_locked = true;  // just for safety!
    }
    return *this;
}


MenuItem& MenuItem::setNotIfLocked(const bool not_if_locked)
{
    m_not_if_locked = not_if_locked;
    return *this;
}


MenuItem& MenuItem::setUnsupported(const bool unsupported)
{
    m_unsupported = unsupported;
    return *this;
}


QString MenuItem::info() const
{
    // Prettier than operator<<
    QStringList parts{"Menu item:"};
    if (!m_title.isEmpty()) {
        parts.append(QString("title=%1")
                     .arg(convert::stringToCppLiteral(m_title)));
    }
    if (m_p_task) {
        parts.append(QString("task=%1")
                     .arg(convert::stringToCppLiteral(m_p_task->shortname())));
    }
    if (m_p_patient) {
#ifdef DEBUG_SHOW_PATIENT_INFO_IN_DESCRIPTION
        QString patient_info = QString("%1, %2 (%3, DOB %4); %5")
                        .arg(m_p_patient->surname().toUpper(),
                             m_p_patient->forename(),
                             QString("%1 y").arg(m_p_patient->ageYears()),
                             m_p_patient->dobText(),
                             m_p_patient->shortIdnumSummary());
        parts.append(QString("patient=%1")
                     .arg(convert::stringToCppLiteral(patient_info)));
#else
        parts.append("patient=<hidden>");
#endif
    }
    return parts.join(" ");
}


bool MenuItem::matchesSearch(const QString& search_text_lower) const
{
    return title().toLower().contains(search_text_lower) ||
            subtitle().toLower().contains(search_text_lower);
}


QDebug operator<<(QDebug debug, const MenuItem& m)
{
    debug.nospace() << "MenuItem @ " << convert::prettyPointer(&m)
                    << " (m_title=" << m.m_title
                    << ", m_p_task=" << m.m_p_task
                    << ", m_p_patient=" << m.m_p_patient
                    << ")";
    return debug;
}


QDebug operator<<(QDebug debug, const MenuItem* m)
{
    debug << *m;
    return debug;
}
