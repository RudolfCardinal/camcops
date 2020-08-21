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

// #define DEBUG_ICON_LOAD
// #define DEBUG_SCROLL_GESTURES
// #define DEBUG_MIN_SIZE_FOR_TITLE

#include "uifunc.h"
#include <QAbstractItemView>
#include <QApplication>
#include <QBrush>
#include <QDebug>
#include <QDesktopServices>
#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QObject>
#include <QPainter>
#include <QPen>
#include <QPixmapCache>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QScroller>
#include <QStyle>
#include <QThread>
#include <QToolButton>
#include <QUrl>
#include "common/colourdefs.h"
#include "common/cssconst.h"
#include "common/platform.h"
#include "common/textconst.h"
#include "common/uiconst.h"
#include "dialogs/logmessagebox.h"
#include "dialogs/passwordchangedialog.h"
#include "dialogs/passwordentrydialog.h"
#include "dialogs/scrollmessagebox.h"
#include "lib/css.h"
#include "lib/convert.h"
#include "lib/layoutdumper.h"
#include "lib/stringfunc.h"
#include "qobjects/debugeventwatcher.h"


namespace uifunc {

// ============================================================================
// QPixmap loader
// ============================================================================

QPixmap getPixmap(const QString& filename, const QSize& size,
                  const bool cache)
{
    QPixmap pm;
    bool success = true;
    if (cache) {
        if (!QPixmapCache::find(filename, &pm)) {
#ifdef DEBUG_ICON_LOAD
            qDebug() << "Loading icon:" << filename;
#endif
            success = pm.load(filename);
            QPixmapCache::insert(filename, pm);
        }
    } else {
        success = pm.load(filename);
    }
    if (success) {
        if (size.isValid()) {
            // Rescale
            pm = pm.scaled(size, Qt::IgnoreAspectRatio);
        }
    } else {
        qCritical() << Q_FUNC_INFO << "Unable to load icon:" << filename;
    }
    return pm;
}


// ============================================================================
// Icons
// ============================================================================

QLabel* iconWidget(const QString& filename, QWidget* parent,
                   const bool scale, const QSize& size)
{
#ifdef DEBUG_ICON_LOAD
    qDebug() << "iconWidget:" << filename;
#endif
    auto iconlabel = new QLabel(parent);
    setLabelToIcon(iconlabel, filename, scale, size);
    return iconlabel;
}


void setLabelToIcon(QLabel* iconlabel, const QString& filename, bool scale,
                    const QSize& size)
{
    if (!iconlabel) {
        return;
    }
    if (filename.isEmpty()) {
        iconlabel->setFixedSize(QSize());
        iconlabel->setText("");
    } else {
        QSize target_size;  // invalid size
        if (scale) {
            target_size = size;
        }
        QPixmap iconimage = getPixmap(filename, target_size);
        iconlabel->setFixedSize(iconimage.size());
        iconlabel->setPixmap(iconimage);
    }
}



QPixmap addCircleBackground(const QPixmap& image, const QColor& colour,
                            const bool behind, const qreal pixmap_opacity)
{
    const QSize size(image.size());
    QPixmap pm(size);
    pm.fill(QCOLOR_TRANSPARENT);
    QPainter painter(&pm);
    const QBrush brush(colour);
    painter.setBrush(brush);
    const QPen pen(QCOLOR_TRANSPARENT);
    painter.setPen(pen);
    if (behind) {
        // Background to indicate "being touched"
        painter.drawEllipse(0, 0, size.width(), size.height());
        // Icon
        painter.setOpacity(pixmap_opacity);
        painter.drawPixmap(0, 0, image);
    } else {
        // The other way around
        painter.setOpacity(pixmap_opacity);
        painter.drawPixmap(0, 0, image);
        painter.drawEllipse(0, 0, size.width(), size.height());
    }
    return pm;
}


QPixmap addPressedBackground(const QPixmap& image, const bool behind)
{
    return addCircleBackground(image, uiconst::BUTTON_PRESSED_COLOUR, behind);
}


QPixmap addUnpressedBackground(const QPixmap& image, const bool behind)
{
    return addCircleBackground(image, uiconst::BUTTON_UNPRESSED_COLOUR, behind);
}


QPixmap makeDisabledIcon(const QPixmap& image)
{
    return addCircleBackground(image, uiconst::BUTTON_DISABLED_COLOUR,
                               true, uiconst::DISABLED_ICON_OPACITY);
}


QLabel* blankIcon(QWidget* parent, const QSize& size)
{
    QPixmap iconimage(size);
    iconimage.fill(QCOLOR_TRANSPARENT);
    auto iconlabel = new QLabel(parent);
    iconlabel->setFixedSize(size);
    iconlabel->setPixmap(iconimage);
    return iconlabel;
}


QString resourceFilename(const QString& resourcepath)
{
    return QString(":/resources/%1").arg(resourcepath);
}


QUrl resourceUrl(const QString& resourcepath)
{
    return QUrl(QString("qrc:///resources/%1").arg(resourcepath));
}


QString iconFilename(const QString& basefile)
{
    return resourceFilename(QString("camcops/images/%1").arg(basefile));
}


// ============================================================================
// Buttons
// ============================================================================

QString iconButtonStylesheet(const QString& normal_filename,
                             const QString& pressed_filename)
{
    QString stylesheet = "QToolButton {"
                         "border-image: url('" + normal_filename + "');"
                         "}";
    if (!pressed_filename.isEmpty()) {
        // http://doc.qt.io/qt-5/stylesheet-syntax.html
        stylesheet += "QToolButton:pressed {"
                      "border-image: url('" + pressed_filename + "');"
                      "}";
    }
    // Related:
    // http://stackoverflow.com/questions/18388098/qt-pushbutton-hover-pressed-icons
    // http://stackoverflow.com/questions/12391125/qpushbutton-css-pressed
    // http://stackoverflow.com/questions/20207224/styling-a-qpushbutton-with-two-images
    return stylesheet;
}


QAbstractButton* iconButton(const QString& normal_filename,
                            const QString& pressed_filename,
                            QWidget* parent)
{
    auto button = new QToolButton(parent);
    button->setIconSize(uiconst::g_iconsize);
    // Impossible to do this without stylesheets!
    // But you can do stylesheets in code...
    button->setStyleSheet(iconButtonStylesheet(normal_filename,
                                               pressed_filename));
    return button;
}

/*
QString UiFunc::iconPngFilename(const QString& stem)
{
    return iconFilename(stem + ".png");
}


QString UiFunc::iconTouchedPngFilename(const QString& stem)
{
    return iconFilename(stem + "_T.png");
}
*/


// ============================================================================
// Widget manipulations
// ============================================================================

void setBackgroundColour(QWidget* widget, const QColor& colour)
{
    // https://wiki.qt.io/How_to_Change_the_Background_Color_of_QWidget

    // Palette method not working. (Conflict with stylesheet?)
    // http://doc.qt.io/qt-5/qwidget.html#autoFillBackground-prop
    //
    // QPalette palette(widget->palette());
    // palette.setColor(QPalette::Background, Qt::red);
    // widget->setPalette(palette);
    // widget->setAutoFillBackground(true);

    // Stylesheet method working.
    widget->setStyleSheet("background-color:" + css::colourCss(colour) + ";");

    // See also:
    // https://stackoverflow.com/questions/25466030/make-qwidget-transparent
    // http://doc.qt.io/qt-5/qwidget.html#transparency-and-double-buffering

}


void setBackgroundAndPressedColour(QWidget* widget,
                                   const QColor& background,
                                   const QColor& pressed)
{
    // untested
    widget->setStyleSheet(QString(
        "QWidget {"
        "  background-color: %1;"
        "}"
        "QWidget:pressed {"
        "  background-color: %1;"
        "}").arg(css::colourCss(background), css::colourCss(pressed)));
}


void removeAllChildWidgets(QObject* object)
{
    // http://stackoverflow.com/questions/22643853/qt-clear-all-widgets-from-inside-a-qwidgets-layout
    // ... modified a little
    qDebug() << "removeAllChildWidgets";
    for (QWidget* w : object->findChildren<QWidget*>()) {
        // qDebug() << "1";
        if (!(w->windowFlags() & Qt::Window)) {
            // qDebug() << "2";
            delete w;
        }
    }

    // BUT layouts do not become parents of their widgets:
    // http://stackoverflow.com/questions/4065378/qt-get-children-from-layout
}


const Qt::Alignment HALIGN_MASK = (Qt::AlignLeft | Qt::AlignRight |
                                   Qt::AlignHCenter | Qt::AlignJustify);
const Qt::Alignment VALIGN_MASK = (Qt::AlignTop | Qt::AlignBottom |
                                   Qt::AlignVCenter | Qt::AlignBaseline);


Qt::Alignment combineAlignment(const Qt::Alignment halign,
                               const Qt::Alignment valign)
{
    return (halign & HALIGN_MASK) | (valign & VALIGN_MASK);
}


void repolish(QWidget* widget)
{
    // http://wiki.qt.io/DynamicPropertiesAndStylesheets
    // http://stackoverflow.com/questions/18187376/stylesheet-performance-hits-with-qt

    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}


void setProperty(QWidget* widget, const QString& property,
                 const QVariant& value, const bool repolish_afterwards)
{
    if (!widget) {
        qWarning() << Q_FUNC_INFO << "- ignored for null widget";
        return;
    }
    const QByteArray propdata = property.toLatin1();
    const char* propname = propdata.constData();
    widget->setProperty(propname, value);
    if (repolish_afterwards) {
        repolish(widget);
    }
}


QString cssBoolean(const bool value)
{
    return value ? cssconst::VALUE_TRUE : cssconst::VALUE_FALSE;
}


void setPropertyItalic(QWidget* widget, const bool italic, const bool repolish)
{
    setProperty(widget, cssconst::PROPERTY_ITALIC, cssBoolean(italic),
                repolish);
}


void setPropertyMissing(QWidget* widget,
                        const bool missing, const bool repolish)
{
    setProperty(widget, cssconst::PROPERTY_MISSING, cssBoolean(missing),
                repolish);
}


void clearLayout(QLayout* layout, bool delete_widgets)
{
    // DANGER: do not use "delete"; use "deleteLater()".
    // If you use delete, if this is used from signals from within this layout;
    // you can get a segfault from e.g. QAbstractItemView::mouseReleaseEvent.

    if (!layout) {
        qWarning() << "Null pointer passed to clearLayout";
        return;
    }
    // http://stackoverflow.com/questions/4857188/clearing-a-layout-in-qt
    // https://stackoverflow.com/questions/4272196/qt-remove-all-widgets-from-layout

    // For all the layout items in our layout...
    while (QLayoutItem* item = layout->takeAt(0)) {
        // We now own "item", so may (and will) delete it.

        // If the layout item has a layout, clear it out
        if (QLayout* child_layout = item->layout()) {
            clearLayout(child_layout, delete_widgets);
            child_layout->deleteLater();
        }

        // If the layout item has a widget, and we're deleting widgets, delete
        // this widget
        if (delete_widgets) {
            if (QWidget* child_widget = item->widget()) {
                child_widget->deleteLater();
            }
        }

        // Delete the layout item (which we own).
        delete item;
    }
    layout->invalidate();
}


void scrollToEnd(QPlainTextEdit* editor)
{
    QScrollBar* vsb = editor->verticalScrollBar();
    if (vsb) {
        vsb->setValue(vsb->maximum());
    }
    QScrollBar* hsb = editor->horizontalScrollBar();
    if (hsb) {
        hsb->setValue(0);
    }
}


void scrollToStart(QPlainTextEdit* editor)
{
    QScrollBar* vsb = editor->verticalScrollBar();
    if (vsb) {
        vsb->setValue(0);
    }
    QScrollBar* hsb = editor->horizontalScrollBar();
    if (hsb) {
        hsb->setValue(0);
    }
}


/*
bool isScrollAtEnd(QPlainTextEdit* editor)
{
    QScrollBar* vsb = editor->verticalScrollBar();
    return vsb && vsb->value() == vsb->maximum();
}
*/


// ============================================================================
// Killing the app
// ============================================================================

bool amInGuiThread()
{
    // https://stackoverflow.com/questions/977653
    return QThread::currentThread() == QCoreApplication::instance()->thread();
}


void stopApp(const QString& error, const QString& title)
{
    // MODAL DIALOGUE, FOLLOWED BY HARD KILL,
    // so callers don't need to worry about what happens afterwards.
    if (amInGuiThread()) {
        ScrollMessageBox box(QMessageBox::Critical, title, error);
        box.addButton(QObject::tr("Abort"), QDialogButtonBox::AcceptRole);
        box.exec();
    }
    const QString msg = "ABORTING: " + error;
    qFatal("%s", qPrintable(msg));
    // If the first argument is not a string literal:
    // "format not a string literal and no format arguments"
    // https://bugreports.qt.io/browse/QTBUG-8967

    // qFatal() will kill the app
    // http://doc.qt.io/qt-4.8/qtglobal.html#qFatal

    // exit(EXIT_FAILURE);
}


// ============================================================================
// Alerts
// ============================================================================

void alert(const QString& text, const QString& title)
{
    // Tasks may elect to show long text here
    ScrollMessageBox::plain(nullptr, title, text);
}


void alert(const QStringList& lines, const QString& title)
{
    alert(stringfunc::joinHtmlLines(lines), title);
}


void alertLogMessageBox(const QString& text, const QString& title,
                        const bool as_html)
{
    LogMessageBox box(nullptr, title, text, as_html);
    box.exec();
}


void alertLogMessageBox(const QStringList& lines, const QString& title,
                        const bool as_html)
{
    const QString text = lines.join(as_html ? "<br>" : "\n");
    alertLogMessageBox(text, title, as_html);
}


void alertNotWhenLocked()
{
    alert(QObject::tr("Can’t perform this action when CamCOPS is locked"),
          QObject::tr("Unlock first"));
}


// ============================================================================
// Confirmation
// ============================================================================

bool confirm(const QString& text, const QString& title,
             QString yes, QString no, QWidget* parent)
{
    if (yes.isEmpty()) {
        yes = TextConst::yes();
    }
    if (no.isEmpty()) {
        no = TextConst::no();
    }
    ScrollMessageBox box(QMessageBox::Question, title, text, parent);
    QAbstractButton* yes_button = box.addButton(yes, QDialogButtonBox::YesRole);
    box.addButton(no, QDialogButtonBox::NoRole);
    box.exec();
    return box.clickedButton() == yes_button;
}


// ============================================================================
// Password checks/changes
// ============================================================================

bool getPassword(const QString& text, const QString& title,
                 QString& password, QWidget* parent)
{
    PasswordEntryDialog dlg(text, title, parent);
    const int reply = dlg.exec();
    if (reply != QDialog::Accepted) {
        return false;
    }
    // fetch/write back password
    password = dlg.password();
    return true;
}


bool getOldNewPasswords(const QString& text, const QString& title,
                        const bool require_old_password,
                        QString& old_password, QString& new_password,
                        QWidget* parent)
{
    PasswordChangeDialog dlg(text, title, require_old_password, parent);
    const int reply = dlg.exec();
    if (reply != QMessageBox::Accepted) {
        return false;
    }
    // Fetch/write back passwords
    old_password = dlg.oldPassword();
    new_password = dlg.newPassword();
    return true;
}


// ============================================================================
// CSS
// ============================================================================

QString textCSS(const int fontsize_pt,
                const bool bold,
                const bool italic,
                const QString& colour)
{
    QString css;
    if (fontsize_pt > 0) {
        css += QString("font-size: %1pt;").arg(fontsize_pt);
    }
    // Only pt and px supported
    // http://doc.qt.io/qt-5.7/stylesheet-reference.html
    if (bold) {
        css += "font-weight: bold;";
    }
    if (italic) {
        css += "font-style: italic";
    }
    if (!colour.isEmpty()) {
        css += QString("color: %1").arg(colour);
    }
    return css;
}


// ============================================================================
// Opening URLS
// ============================================================================

void visitUrl(const QString& url)
{
    qInfo().noquote() << "Launching URL:" << url;
    bool success = QDesktopServices::openUrl(QUrl(url));
    if (!success) {
        alert(QObject::tr("Failed to open browser"));
    }
}


// ============================================================================
// Strings
// ============================================================================

QString escapeString(const QString& string)
{
    // See also http://doc.qt.io/qt-5/qregexp.html#escape
    // Obsolete: Qt::escape()

    // Convert to a C++ literal.
    // There's probably a much more efficient way...
    const QByteArray arr = string.toLatin1();
    const int len = arr.length();
    QString result;
    result.reserve(static_cast<int>(len * 1.1));  // as per QString::toHtmlEscaped
    result.append('"');  // opening quote
    for (int i = 0; i < len; ++i) {
        char c = arr.at(i);
        if (c < ' ') {
            result.append('\\');
            result.append(c - 1 + 'a');
        } else {
            result.append(c);
        }
    }
    result.append('"');  // closing quote
    result.squeeze();  // as per QString::toHtmlEscaped
    return result;
}


QString yesNo(const bool yes)
{
    return yes ? TextConst::yes() : TextConst::no();
}


QString yesNoNull(const QVariant& value)
{
    return value.isNull() ? convert::NULL_STR : yesNo(value.toBool());
}


QString yesNoUnknown(const QVariant& value)
{
    return value.isNull() ? TextConst::unknown() : yesNo(value.toBool());
}


QString trueFalse(const bool yes)
{
    return yes ? TextConst::txtTrue() : TextConst::txtFalse();
}


QString trueFalseNull(const QVariant& value)
{
    return value.isNull() ? convert::NULL_STR : trueFalse(value.toBool());
}


QString trueFalseUnknown(const QVariant& value)
{
    return value.isNull() ? TextConst::unknown() : trueFalse(value.toBool());
}



// ============================================================================
// Scrolling
// ============================================================================

#ifdef DEBUG_SCROLL_GESTURES
static void debugScrollerStateChanged(QScroller::State new_state)
{
    qDebug() << new_state;
}
#endif


void applyScrollGestures(QWidget* widget)
{
    // This method works well, except that if the widget is also handling
    // mouse clicks (etc.), it can get slightly confusing.

    if (!widget) {
        stopApp("Null pointer to applyScrollGestures");
    }

    // 1. Grab the relevant gesture. Only one gesture can be grabbed.
    //    - Tried: TouchGesture on tablets, LeftMouseButtonGesture on desktops
    //      ... fine for e.g. ScrollMessageBox, but QListView goes funny on
    //      Android (in that scroll gestures also leak through as clicks).
    //    - Tried: LeftMouseButtonGesture throughout.
    //      Fine for QListView on Android, but scrolling then doesn't work
    //      for ScrollMessageBox.
    //    - Others have noticed this too:
    //      https://forum.qt.io/topic/37930/solved-qt-5-2-android-qscroller-on-qtablewidget-clicks
    //    - So:
    const bool widget_is_itemview = (
                dynamic_cast<QAbstractItemView*>(widget) ||
                dynamic_cast<QAbstractItemView*>(widget->parent()));
    // It makes little difference which of these two we choose:
    // const bool use_touch = platform::PLATFORM_ANDROID && !widget_is_itemview;
    const bool use_touch = false;

    QScroller::ScrollerGestureType gesture_type = use_touch
            ? QScroller::TouchGesture
            : QScroller::LeftMouseButtonGesture;
    /*
    if (platform::PLATFORM_ANDROID) {
        // Some widgets automatically take a two-finger swipe, a PanGesture.
        widget->ungrabGesture(Qt::PanGesture);
        // ... but ungrabbing this just stops them responding; doesn't seem to
        //     help us to respond to something else
    }
    */
    QScroller::grabGesture(widget, gesture_type);  // will ungrab any other

    // Still a problem: scroller not responding for ScrollMessageBox on Android.
    // Yet everything else is working.
    // VerticalScrollArea versus QScrollArea? No.

#ifdef DEBUG_SCROLL_GESTURES
    qDebug().nospace()
            << Q_FUNC_INFO
            << ": widget_is_itemview == " << widget_is_itemview
            << ", use_touch == " << use_touch
            << ", widget->isWidgetType() == " << widget->isWidgetType()
            << ", widget->testAttribute(Qt::WA_AcceptTouchEvents) == "
            << widget->testAttribute(Qt::WA_AcceptTouchEvents);
    new DebugEventWatcher(widget, DebugEventWatcher::All);  // owned by widget henceforth
#endif

    /* UNNECESSARY: done by QScroller::grabGesture if TouchGesture is used:
    if (use_touch) {
        // Unsure if this is necessary:
        widget->setAttribute(Qt::WA_AcceptTouchEvents);
    }
    */

    // 2. Disable overshoot.
    QScroller* scroller = QScroller::scroller(widget);
    if (scroller) {
        // http://stackoverflow.com/questions/24677152
        QScrollerProperties prop = scroller->scrollerProperties();
        QVariant overshoot_policy = QVariant::fromValue<QScrollerProperties::OvershootPolicy>(
                    QScrollerProperties::OvershootAlwaysOff);
        prop.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy,
                             overshoot_policy);
        prop.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy,
                             overshoot_policy);
        scroller->setScrollerProperties(prop);
#ifdef DEBUG_SCROLL_GESTURES
        QObject::connect(scroller, &QScroller::stateChanged,
                         std::bind(&debugScrollerStateChanged, std::placeholders::_1));
#endif
    } else {
        qWarning() << Q_FUNC_INFO << "Couldn't make scroller!";
    }

    // Slightly nasty hacks:
    if (widget_is_itemview) {
        makeItemViewScrollSmoothly(widget);
        // ... and since we often apply scroll gestures to the viewport() of
        //     a list view, try its parent too:
        makeItemViewScrollSmoothly(widget->parent());
    }

    // Other discussions about this:
    // - https://forum.qt.io/topic/30546/kinetic-scrolling-on-qscrollarea-on-android-device/5
    // - http://falsinsoft.blogspot.co.uk/2015/09/qt-snippet-use-qscroller-with.html
    // - http://nootka-app.blogspot.co.uk/2015/11/story-of-porting-complex-qt-application_18.html
}


void makeItemViewScrollSmoothly(QObject* object)
{
    // Nasty hacks:
    auto itemview = dynamic_cast<QAbstractItemView*>(object);
    if (itemview) {
#ifdef DEBUG_SCROLL_GESTURES
        qDebug() << Q_FUNC_INFO
                 << "Calling setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel)"
                    " and setVerticalScrollMode(QAbstractItemView::ScrollPerPixel)";
#endif
        itemview->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        itemview->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    }
}


// ============================================================================
// Sizing
// ============================================================================

QSize minimumSizeForTitle(const QDialog* dialog,
                          const bool include_app_name)
{
    if (!dialog) {
        return QSize();
    }
    // +---------------------------------------------+
    // | ICON  TITLETEXT - APPTITLE    WINDOWBUTTONS |
    // |                                             |
    // | contents                                    |
    // +---------------------------------------------+

    // https://doc.qt.io/qt-5/qwidget.html#windowTitle-prop
    const QString window_title = dialog->windowTitle();
    const QString app_name = QApplication::applicationDisplayName();
    QString full_title = window_title;
    if (include_app_name && !platform::PLATFORM_TABLET) {
        // Qt for Android doesn't append this suffix.
        // It does for Linux and Windows.
        const QString title_suffix = QString(" — %1").arg(app_name);
        full_title += title_suffix;
    }
    const QFont title_font = QApplication::font("QWorkspaceTitleBar");
    const QFontMetrics fm(title_font);
    const int title_w = fm.boundingRect(full_title).width();  // "_w" means width

    // dialog->ensurePolished();
    // const QSize frame_size = dialog->frameSize();
    // const QSize content_size = dialog->size();
    // ... problem was that both are QSize(640, 480) upon creation
    // ... even if ensurePolished() is called first
    // const QSize frame_extra = frame_size - content_size;

    // How to count the number of icons shown on a window? ***
    // - Android: 0
    // - Linux: presumably may vary with window manager, but 4 is typical under
    //   XFCE (1 icon on left, 3 [rollup/maximize/close] on right), but need a
    //   bit more for spacing; 6 works better (at 24 pixels width per icon)
    // - Windows: also 4 (icon left, minimize/maximize/close on right)
    const int n_icons = platform::PLATFORM_TABLET ? 0 : 6;

    // How to read the size (esp. width) of a window icon? ***
    // const int icon_w = frame_extra.height();
    // ... on the basis that window icons are square!
    // ... but the problem is that frame size may as yet be zero
    const int icon_w = 24;

    const int final_w = title_w + n_icons * icon_w;
    const QSize dialog_min_size = dialog->minimumSize();
    QSize size(dialog_min_size);
    size.setWidth(qMax(size.width(), final_w));
#ifdef DEBUG_MIN_SIZE_FOR_TITLE
    qDebug().nospace()
            << Q_FUNC_INFO
            << "window_title = " << window_title
            << ", app_name = " << app_name
            << ", full_title = " << full_title
            << ", title_font = " << title_font
            << ", title_w = " << title_w
            // << ", frame_size = " << frame_size
            // << ", content_size = " << content_size
            // << ", frame_extra = " << frame_extra
            << ", n_icons = " << n_icons
            << ", icon_w = " << icon_w
            << ", dialog_min_size = " << dialog_min_size
            << ", size = " << size;
#endif
    return size;
}


} // namespace uifunc
