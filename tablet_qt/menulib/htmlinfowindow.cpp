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

#include "htmlinfowindow.h"
#include <QLabel>
#include <QTextBrowser>
#include <QVBoxLayout>
#include "core/camcopsapp.h"
#include "common/cssconst.h"
#include "lib/filefunc.h"
#include "lib/uifunc.h"
#include "menulib/menuheader.h"
#include "widgets/labelwordwrapwide.h"

// http://doc.qt.io/qt-5/qtextbrowser.html


HtmlInfoWindow::HtmlInfoWindow(CamcopsApp& app, const QString& title,
                               const QString& filename, const QString& icon,
                               const bool fullscreen) :
    m_app(app)
{
    setStyleSheet(m_app.getSubstitutedCss(uiconst::CSS_CAMCOPS_MENU));
    setObjectName(cssconst::MENU_WINDOW_OUTER_OBJECT);

    // Layouts
    auto mainlayout = new QVBoxLayout();

    auto dummy_layout = new QVBoxLayout();
    dummy_layout->setContentsMargins(uiconst::NO_MARGINS);
    setLayout(dummy_layout);
    auto dummy_widget = new QWidget();
    dummy_widget->setObjectName(cssconst::MENU_WINDOW_BACKGROUND);
    dummy_layout->addWidget(dummy_widget);
    dummy_widget->setLayout(mainlayout);

    // Header
    auto header = new MenuHeader(this, m_app, false, title, icon);
    mainlayout->addWidget(header);
    connect(header, &MenuHeader::backClicked,
            this, &HtmlInfoWindow::finished);

    // HTML
    if (filefunc::fileExists(filename)) {
        const QString html = filefunc::textfileContents(filename);
        auto browser = new QTextBrowser();
        browser->setHtml(html);
        browser->setOpenExternalLinks(true);
        mainlayout->addWidget(browser);
        // It manages scrolling itself.
        // But not touch scrolling: touching us
        uifunc::applyScrollGestures(browser->viewport());
    } else {
        QLabel* label = new LabelWordWrapWide(tr("No such file") + ": " +
                                              filename);
        label->setObjectName(cssconst::WARNING);
        mainlayout->addWidget(label);
        mainlayout->addStretch();
    }

    // Fullscreen?
    setWantsFullscreen(fullscreen);
}
