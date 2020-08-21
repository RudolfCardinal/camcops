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

#define USE_NUMERIC_DATES

#include "qudatetime.h"
#include <QCalendarWidget>
#include <QDateTimeEdit>
#include <QFont>
#include <QHBoxLayout>
#include <QTextCharFormat>
#include "lib/uifunc.h"
#include "questionnairelib/questionnaire.h"
#include "widgets/imagebutton.h"
#include "widgets/spacer.h"

// http://doc.qt.io/qt-5/qdatetime.html#toString
#ifdef USE_NUMERIC_DATES
const QString DEFAULT_DATETIME_FORMAT("yyyy MM dd HH:mm");
const QString DEFAULT_DATE_FORMAT("yyyy MM dd");
const Qt::InputMethodHints DATETIME_IMH = Qt::ImhPreferNumbers;
const Qt::InputMethodHints DATE_IMH = Qt::ImhPreferNumbers;
#else
const QString DEFAULT_DATETIME_FORMAT("dd MMM yyyy HH:mm");
const QString DEFAULT_DATE_FORMAT("dd MMM yyyy");
const Qt::InputMethodHints DATETIME_IMH = Qt::ImhNone;
const Qt::InputMethodHints DATE_IMH = Qt::ImhNone;
#endif
const QString DEFAULT_TIME_FORMAT("HH:mm");
const Qt::InputMethodHints TIME_IMH = Qt::ImhPreferNumbers;
// Default pseudo-null date (what's display when nothing is selected):
// - 14 Sep 1752 is usual minimum (Gregorian calendar), but is a long way from now
// - 01 Jan 2000 is an option, but is too plausible
// - 01 Jan 1900 is a common choice (e.g. Epic, hence all those 117yo unknown
//   patients in 2017).
// const QDate PSEUDONULL_DATE(1752, 9, 14);
// const QDate PSEUDONULL_DATE(2000, 1, 1);
const QDate PSEUDONULL_DATE(1900, 1, 1);
const QTime PSEUDONULL_TIME(0, 0, 0, 0);
const QDateTime PSEUDONULL_DATETIME(PSEUDONULL_DATE, PSEUDONULL_TIME);


QuDateTime::QuDateTime(FieldRefPtr fieldref) :
    m_fieldref(fieldref),
    m_mode(Mode::DefaultDateTime),
    m_custom_input_method_hint(Qt::ImhNone),
    m_offer_now_button(false),
    m_offer_null_button(false),
    m_editor(nullptr),
    m_calendar_widget(nullptr),
    m_minimum_date(uiconst::QCALENDARWIDGET_MIN_DATE),
    m_maximum_date(uiconst::QCALENDARWIDGET_MAX_DATE)
{
    Q_ASSERT(m_fieldref);
    connect(m_fieldref.data(), &FieldRef::valueChanged,
            this, &QuDateTime::fieldValueChanged);
    connect(m_fieldref.data(), &FieldRef::mandatoryChanged,
            this, &QuDateTime::fieldValueChanged);
}


QuDateTime* QuDateTime::setMode(const QuDateTime::Mode mode)
{
    m_mode = mode;
    return this;
}


QuDateTime* QuDateTime::setCustomFormat(
        const QString& format,
        const Qt::InputMethodHints input_method_hint)
{
    m_custom_format = format;
    m_custom_input_method_hint = input_method_hint;
    return this;
}


QuDateTime* QuDateTime::setOfferNowButton(const bool offer_now_button)
{
    m_offer_now_button = offer_now_button;
    return this;
}


QuDateTime* QuDateTime::setOfferNullButton(const bool offer_null_button)
{
    m_offer_null_button = offer_null_button;
    return this;
}


void QuDateTime::setFromField()
{
    fieldValueChanged(m_fieldref.data(), nullptr);
}


QuDateTime* QuDateTime::setMinimumDate(const QDate& min_date)
{
    m_minimum_date = min_date;
    return this;
}


QuDateTime* QuDateTime::setMaximumDate(const QDate& max_date)
{
    m_maximum_date = max_date;
    return this;
}


FieldRefPtrList QuDateTime::fieldrefs() const
{
    return FieldRefPtrList{m_fieldref};
}


QPointer<QWidget> QuDateTime::makeWidget(Questionnaire* questionnaire)
{
    const bool read_only = questionnaire->readOnly();

    QPointer<QWidget> widget = new QWidget();
    widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    auto layout = new QHBoxLayout();
    layout->setContentsMargins(uiconst::NO_MARGINS);
    widget->setLayout(layout);

    QString format;
    bool use_calendar = true;
    Qt::InputMethodHints input_method_hint = Qt::ImhNone;
    switch (m_mode) {
    case DefaultDateTime:
        format = DEFAULT_DATETIME_FORMAT;
        input_method_hint = DATETIME_IMH;
        break;
    case DefaultDate:
        format = DEFAULT_DATE_FORMAT;
        input_method_hint = DATE_IMH;
        break;
    case DefaultTime:
        format = DEFAULT_TIME_FORMAT;
        input_method_hint = TIME_IMH;
        use_calendar = false;
        break;
    case CustomDateTime:
    case CustomDate:
        format = m_custom_format;
        input_method_hint = m_custom_input_method_hint;
        break;
    case CustomTime:
        format = m_custom_format;
        input_method_hint = m_custom_input_method_hint;
        use_calendar = false;
        break;
    }

    m_editor = new QDateTimeEdit();

    if (use_calendar) {
        // Bug? Needs to be set here rather than on QCalendarWidget
        // https://bugreports.qt.io/browse/QTBUG-77775
        m_editor->setMinimumDate(m_minimum_date);
        m_editor->setMaximumDate(m_maximum_date);
    }
    m_editor->setDisplayFormat(format);
    m_editor->setInputMethodHints(input_method_hint);
    // ... or, on Android, you get a numbers-only keyboard even with a format
    // like "12 Jan 1970".
    // - That's because QDateTimeEditPrivate::init() calls
    //   q->setInputMethodHints(Qt::ImhPreferNumbers);
    // - Note also that Qt::ImhDate and Qt::ImhTime give you numbers plus
    //   punctuation, for ":"; see qqnxabstractvirtualkeyboard.cpp

    m_editor->setCalendarPopup(use_calendar);
    // ... need to call setCalendarPopup(true) BEFORE setCalendarWidget(); see
    // - https://bugreports.qt.io/browse/QTBUG-12300
    // - https://doc.qt.io/qt-5/qdatetimeedit.html#setCalendarWidget

    /*
    TO THINK ABOUT: QuDateTime time picker
    - Qt only supplies a date (calendar) popup.
      You can explore its features using the "calendarwidget" demo.
    - It is possible to write ones to do times as well.
    - http://doc.qt.io/qt-5/qdatetimeedit.html#using-a-pop-up-calendar-widget
    - http://doc.qt.io/qt-5/qtwidgets-widgets-calendarwidget-example.html
    - https://forum.qt.io/topic/71670/qdatetimeedit-with-date-and-time-picker/6
    - Looking at the various bits of source:

        void QDateTimeEditPrivate::initCalendarPopup(QCalendarWidget* cw)
        {
            Q_Q(QDateTimeEdit);
            if (!monthCalendar) {
                monthCalendar = new QCalendarPopup(q, cw);
                monthCalendar->setObjectName(QLatin1String("qt_datetimedit_calendar"));
                QObject::connect(monthCalendar, SIGNAL(newDateSelected(QDate)), q, SLOT(setDate(QDate)));
                QObject::connect(monthCalendar, SIGNAL(hidingCalendar(QDate)), q, SLOT(setDate(QDate)));
                QObject::connect(monthCalendar, SIGNAL(activated(QDate)), q, SLOT(setDate(QDate)));
                QObject::connect(monthCalendar, SIGNAL(activated(QDate)), monthCalendar, SLOT(close()));
                QObject::connect(monthCalendar, SIGNAL(resetButton()), q, SLOT(_q_resetButton()));
            } else if (cw) {
                monthCalendar->setCalendarWidget(cw);
            }
            syncCalendarWidget();
        }

    - Alternatively, for dates: alter the stylesheet to make the
      QCalendarWidget big enough to use on tablets.

    */

    if (use_calendar) {
        // Editor does NOT take ownership, so we should:
        // http://doc.qt.io/qt-5/qdatetimeedit.html#setCalendarWidget
        delete m_calendar_widget;
        m_calendar_widget = QPointer<QCalendarWidget>(new QCalendarWidget());

        m_calendar_widget->setSelectionMode(QCalendarWidget::SingleSelection);
        m_calendar_widget->setNavigationBarVisible(true);
        m_calendar_widget->setFirstDayOfWeek(Qt::Monday);
        m_calendar_widget->setGridVisible(true);

        QTextCharFormat header_text_format;
        header_text_format.setFontWeight(uiconst::QCALENDARWIDGET_HEADER_FONTWEIGHT);
        m_calendar_widget->setHeaderTextFormat(header_text_format);

        m_calendar_widget->setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames);
        m_calendar_widget->setVerticalHeaderFormat(QCalendarWidget::ISOWeekNumbers);

        QTextCharFormat day_format;
        day_format.setForeground(uiconst::QCALENDARWIDGET_TEXT_WEEKDAY);
        m_calendar_widget->setWeekdayTextFormat(Qt::Monday, day_format);
        m_calendar_widget->setWeekdayTextFormat(Qt::Tuesday, day_format);
        m_calendar_widget->setWeekdayTextFormat(Qt::Wednesday, day_format);
        m_calendar_widget->setWeekdayTextFormat(Qt::Thursday, day_format);
        m_calendar_widget->setWeekdayTextFormat(Qt::Friday, day_format);
        day_format.setForeground(uiconst::QCALENDARWIDGET_TEXT_WEEKEND);
        m_calendar_widget->setWeekdayTextFormat(Qt::Saturday, day_format);
        m_calendar_widget->setWeekdayTextFormat(Qt::Sunday, day_format);

        // QFont font;
        // font.setBold(true);  // works!
        // font.setPixelSize(60);  // Does NOT work!
        // m_calendar_widget->setFont(font);

        // Hack: change calendar title colors
        // See https://www.qtcentre.org/threads/30478-How-To-Change-Style-Sheet-for-QCalendarWidget
        QWidget* calendar_navbar = m_calendar_widget->findChild<QWidget*>("qt_calendar_navigationbar");
        if (calendar_navbar) {
            // Does get called.
            // qDebug() << Q_FUNC_INFO << "Setting colour of calendar_navbar";
            QPalette pal = calendar_navbar->palette();
            pal.setColor(calendar_navbar->backgroundRole(), uiconst::QCALENDARWIDGET_NAVBAR_BACKGROUND);
            // ... DOES NOT WORK +++
            pal.setColor(calendar_navbar->foregroundRole(), uiconst::QCALENDARWIDGET_NAVBAR_FOREGROUND);
            // ... DOES NOT WORK +++
            calendar_navbar->setPalette(pal);
        }

        // Cell formatting: see QCalendarModel::formatForCell() in qcalendarwidget.cpp

        m_editor->setCalendarWidget(m_calendar_widget);

        /*
        2017-07-17: CRASH on Android; segfault. Stack trace:
        1   QObject::~QObject()                                                                                                                                                  0xe12e0b46
        2   QObject::~QObject()                                                                                                                                                  0xe12e0bcc
        3   QtSharedPointer::CustomDeleter<QCalendarWidget, QtSharedPointer::NormalDeleter>::execute()                                                                           0xdf115fc0
        4   QtSharedPointer::ExternalRefCountWithCustomDeleter<QCalendarWidget, QtSharedPointer::NormalDeleter>::deleter(QtSharedPointer::ExternalRefCountData *)                0xdf115b98
        5   QtSharedPointer::ExternalRefCountData::destroy()                                                                                                                     0xdee92ab4
        6   QSharedPointer<QCalendarWidget>::deref(QtSharedPointer::ExternalRefCountData *)                                                                                      0xdf0fca74
        7   QSharedPointer<QCalendarWidget>::deref()                                                                                                                             0xdf0fc89c
        8   QSharedPointer<QCalendarWidget>::~QSharedPointer()                                                                                                                   0xdf0fc75c
        9   QSharedPointer<QCalendarWidget>::operator=(QSharedPointer<QCalendarWidget>&&)                                                                                        0xdf1154fc
        10  QuDateTime::makeWidget(Questionnaire *)                                                                                                                              0xdf114584
        11  QuElement::widget(Questionnaire *)                                                                                                                                   0xdf1188a0
        12  QuPage::widget(Questionnaire *) const                                                                                                                                0xdf146e24
        13  Questionnaire::build()                                                                                                                                               0xdf11c67c
        14  Questionnaire::goToPage(int, bool)                                                                                                                                   0xdf11de00
        15  Questionnaire::processNextClicked()                                                                                                                                  0xdf11d810
        16  Questionnaire::nextClicked()                                                                                                                                         0xdf11d700
        17  QtPrivate::FunctorCall<QtPrivate::IndexesList<>, QtPrivate::List<>, void, void (Questionnaire:: *)()>::call(void (Questionnaire:: *)(), Questionnaire *, void * *)   0xdf122008
        18  void QtPrivate::FunctionPointer<void (Questionnaire:: *)()>::call<QtPrivate::List<>, void>(void (Questionnaire:: *)(), Questionnaire *, void * *)                    0xdf121edc
        19  QtPrivate::QSlotObject<void (Questionnaire:: *)(), QtPrivate::List<>, void>::impl(int, QtPrivate::QSlotObjectBase *, QObject *, void * *, bool *)                    0xdf121868
        20  QMetaObject::activate(QObject *, int, int, void * *)                                                                                                                 0xe12dd404
        ... <More>

        Now that strongly suggests to me that the QDateTime *had* taken
        ownership, and had deleted the object, so when the QSharedPointer gets
        reassigned, we attempt to double-delete and it crashes.

        Is that the case? Sequence is
            void QDateTimeEdit::setCalendarWidget(QCalendarWidget *calendarWidget)
            void QDateTimeEditPrivate::initCalendarPopup(QCalendarWidget *cw)
            ... then either:
                QCalendarPopup::QCalendarPopup(QWidget * parent, QCalendarWidget *cw)
                void QCalendarPopup::setCalendarWidget(QCalendarWidget *cw)
            ... or just
                void QCalendarPopup::setCalendarWidget(QCalendarWidget *cw)
            and that does
                delete calendar.data();
                calendar = QWeakPointer<QCalendarWidget>(cw);
                widgetLayout->addWidget(cw);
            It also looks like someone tried to fix this:
                https://git.qt.io/consulting-usa/qtbase-xcb-rendering/commit/7d28f7772cd8f5aad63359ed0b9c57c12923dc85
            NO, IT DOESN'T DO THAT, THAT'S OLD CODE. THE FIX IS IN QT 5.9.1. IT DOES:
                delete calendar.data();  // RNC: should be fine if already nullptr
                calendar = QPointer<QCalendarWidget>(cw);  // RNC: ... and a QPointer is set to 0 when the referenced object is destroyed
                widgetLayout->addWidget(cw);

            SO, PROBABLY THE SOLUTION IS TO USE A QPOINTER, AND A MANUAL
            DELETE CALL, NOT A QSHAREDPOINTER. If you delete the raw pointer,
            the QPointer will go to nullptr. See:
                - https://stackoverflow.com/questions/22304118/what-is-the-difference-between-qpointer-qsharedpointer-and-qweakpointer-classes
        */

    }

    m_editor->setEnabled(!read_only);
    m_editor->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    // Fixed horizontal keeps the drop-down button close to the text.
    // Expanding vertical makes the drop-down button and spin buttons a
    // reasonable size (not too small).
    m_editor->setMinimumHeight(uiconst::g_min_spinbox_height);
    // Also, the QDateTimeEdit *is* a QAbstractSpinBox, so:
    m_editor->setButtonSymbols(uiconst::SPINBOX_SYMBOLS);
    if (!read_only) {
        connect(m_editor.data(), &QDateTimeEdit::dateTimeChanged,
                this, &QuDateTime::widgetValueChanged);
    }
    layout->addWidget(m_editor);

    if (m_offer_now_button) {
        auto now_button = new ImageButton(uiconst::CBS_TIME_NOW);
        now_button->setEnabled(!read_only);
        if (!read_only) {
            connect(now_button, &QAbstractButton::clicked,
                    this, &QuDateTime::setToNow);
        }
        layout->addWidget(now_button);
    }

    if (m_offer_null_button) {
        auto null_button = new ImageButton(uiconst::CBS_DELETE);
        null_button->setEnabled(!read_only);
        if (!read_only) {
            connect(null_button, &QAbstractButton::clicked,
                    this, &QuDateTime::setToNull);
        }
        layout->addWidget(null_button);
    }

    layout->addStretch();

    setFromField();
    return widget;
}


// It will show a NULL as yellow, but as soon as you edit the field,
// it un-NULLs it irreversibly. (You could use e.g. 14 Sep 1752 00:00 as a
// pseudo-NULL that you can enter, but that doesn't work when you want to
// enter midnight deliberately, and starting with 1752 just looks odd.)

void QuDateTime::widgetValueChanged(const QDateTime& datetime)
{
    setField(datetime, false);
}


void QuDateTime::setField(const QDateTime& datetime,
                          const bool reset_this_widget)
{
    QVariant newvalue = datetime;
    switch (m_mode) {
    case DefaultDateTime:
    case CustomDateTime:
        newvalue.convert(QVariant::DateTime);
        break;
    case DefaultDate:
    case CustomDate:
        newvalue.convert(QVariant::Date);
        break;
    case DefaultTime:
    case CustomTime:
        newvalue.convert(QVariant::Time);
        break;
    }
    const bool changed = m_fieldref->setValue(newvalue,
                                              reset_this_widget ? nullptr : this);
    if (changed) {
        emit elementValueChanged();
    }
}


void QuDateTime::setToNow()
{
    setField(QDateTime::currentDateTime(), true);
}


void QuDateTime::setToNull()
{
    setField(QDateTime(), true);
}


bool QuDateTime::hasDateComponent() const
{
    switch (m_mode) {
    case DefaultDateTime:
    case CustomDateTime:
    case DefaultDate:
    case CustomDate:
        return true;
    default:
        return false;
    }
}


bool QuDateTime::hasTimeComponent() const
{
    switch (m_mode) {
    case DefaultDateTime:
    case CustomDateTime:
    case DefaultTime:
    case CustomTime:
        return true;
    default:
        return false;
    }
}


void QuDateTime::fieldValueChanged(const FieldRef* fieldref,
                                   const QObject* originator)
{
    if (!m_editor) {
        return;
    }
    // Missing?
    uifunc::setPropertyMissing(m_editor, fieldref->missingInput());
    if (originator != this) {
        if (hasDateComponent()) {
            QDateTime display_value = fieldref->valueDateTime();
            if (!display_value.isValid()) {
                display_value = PSEUDONULL_DATETIME;
                // because QDateTimeEdit::setDateTime() will ignore invalid values
            }
            const QSignalBlocker blocker(m_editor);
            m_editor->setDateTime(display_value);
        } else {
            // We can't use fieldref->valueDateTime(); that returns an invalid
            // QDateTime. (Bugfix 2019-08-22.)
            QTime display_value = fieldref->valueTime();
            if (!display_value.isValid()) {
                display_value = PSEUDONULL_TIME;
                // because QDateTimeEdit::setTime() will ignore invalid values
            }
            const QSignalBlocker blocker(m_editor);
            m_editor->setTime(display_value);
        }
    }
    // NULL will be shown as the pseudonull value.
    // The yellow marker will disappear when that value is edited.
}
