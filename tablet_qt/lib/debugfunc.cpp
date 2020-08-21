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

// #define DEBUG_EVEN_GIANT_VARIANTS

#include "debugfunc.h"
#include <QDebug>
#include <QDialog>
#include <QVariant>
#include <QVBoxLayout>
#include "common/cssconst.h"
#include "common/uiconst.h"
#include "layouts/vboxlayouthfw.h"
#include "qobjects/keypresswatcher.h"
#include "qobjects/showwatcher.h"

namespace debugfunc {


void debugConcisely(QDebug debug, const QVariant& value)
{
#ifdef DEBUG_EVEN_GIANT_VARIANTS
    debug << value;
#else
    switch (value.type()) {

    // Big things; don't show their actual value to the console
    case QVariant::ByteArray:
        debug << "<ByteArray>";
        break;

    // Normal things
    default:
        debug << value;
        break;
    }
#endif
}


void debugConcisely(QDebug debug, const QVector<QVariant>& values)
{
    QDebug d = debug.nospace();
    d << "(";
    const int n = values.length();
    for (int i = 0; i < n; ++i) {
        if (i > 0) {
            d << ", ";
        }
        debugConcisely(d, values.at(i));
    }
    d << ")";
}


void dumpQObject(QObject* obj)
{
    qDebug("----------------------------------------------------");
    qDebug("Widget name : %s", qPrintable(obj->objectName()));
    qDebug("Widget class: %s", obj->metaObject()->className());
    qDebug("\nObject info [if Qt itself built in debug mode]:");
    obj->dumpObjectInfo();
    qDebug("\nObject tree [if Qt itself built in debug mode]:");
    obj->dumpObjectTree();
    qDebug("----------------------------------------------------");
}


void debugWidget(QWidget* widget,
                 const bool set_background_by_name,
                 const bool set_background_by_stylesheet,
                 const layoutdumper::DumperConfig& config,
                 const bool use_hfw_layout,
                 const QString* dialog_stylesheet)
{
    QDialog dlg;
    QSizePolicy dlg_sp(QSizePolicy::Preferred, QSizePolicy::Preferred);
    if (use_hfw_layout) {
        dlg_sp.setHeightForWidth(true);
    }
    dlg.setSizePolicy(dlg_sp);
    dlg.setWindowTitle("Press D/dump layout, A/adjustSize");
    if (dialog_stylesheet) {
        dlg.setStyleSheet(*dialog_stylesheet);
    }
    VBoxLayoutHfw* hfwlayout = use_hfw_layout ? new VBoxLayoutHfw() : nullptr;
    QVBoxLayout* vboxlayout = use_hfw_layout ? nullptr : new QVBoxLayout();
    QLayout* layout = use_hfw_layout ? static_cast<QLayout*>(hfwlayout)
                                     : static_cast<QLayout*>(vboxlayout);
    layout->setContentsMargins(uiconst::NO_MARGINS);
    if (widget) {
        if (set_background_by_name) {
            widget->setObjectName(cssconst::DEBUG_GREEN);
        }
        if (set_background_by_stylesheet) {
            widget->setStyleSheet("background: green;");
        }
        // const Qt::Alignment align = Qt::AlignTop;
        const Qt::Alignment align = 0;
        // We can't do what follows via the QLayout* pointer, which is why we
        // have to maintain these two:
        if (use_hfw_layout) {
            hfwlayout->addWidget(widget, 0, align);
        } else {
            vboxlayout->addWidget(widget, 0, align);
        }
        auto showwatcher = new ShowWatcher(&dlg, true);
        Q_UNUSED(showwatcher)
        auto keywatcher = new KeyPressWatcher(&dlg);
        // keywatcher becomes child of dlg,
        // and LayoutDumper is a namespace, so:
        // Safe object lifespan signal: can use std::bind
        keywatcher->addKeyEvent(
            Qt::Key_D,
            std::bind(&layoutdumper::dumpWidgetHierarchy, &dlg, config));
        keywatcher->addKeyEvent(
            Qt::Key_A,
            std::bind(&QWidget::adjustSize, widget));
    } else {
        qDebug() << Q_FUNC_INFO << "null widget";
    }
    dlg.setLayout(layout);
    dlg.exec();
}


}  // namespace debugfunc
