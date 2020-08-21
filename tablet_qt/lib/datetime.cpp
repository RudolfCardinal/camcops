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

#include "datetime.h"
#include <QTimeZone>
#include <QVariant>

namespace datetime {


const QString LONG_DATE_FORMAT("dddd d MMMM yyyy");  // Thursday 31 Dec 2000
const QString TIMESTAMP_FORMAT("yyyy-MM-dd HH:mm:ss.zzz");  // 2000-12-31 23:59:59.999
const QString SHORT_DATETIME_FORMAT("yyyy-MM-dd HH:mm");  // 2000-12-31 23:59
const QString ISO_DATE_FORMAT("yyyy-MM-dd");  // 2000-12-31
const QString SHORT_DATE_FORMAT(ISO_DATE_FORMAT);
const QString TEXT_DATE_FORMAT("dd MMM yyyy");  // 31 Dec 2000
const QString TEXT_DATETIME_FORMAT("ddd dd MMM yyyy, HH:mm");  // Thu 31 Dec 2000, 23:59
const QString UNKNOWN("?");


QString dateToIso(const QDate& d)
{
    return d.toString(ISO_DATE_FORMAT);
}


// http://stackoverflow.com/questions/21976264/qt-isodate-formatted-date-time-including-timezone

QString datetimeToIsoMs(const QDateTime& dt, const bool use_z_timezone)
{
    // An ISO-8601 format preserving millisecond accuracy and timezone.
    // Equivalent in moment.js: thing.format("YYYY-MM-DDTHH:mm:ss.SSSZ")
    // Example: "2016-06-02T10:04:03.588+01:00"
    // Here we also allow 'Z' for UTC.
    // -- no, we don't; for example, MySQL's CONVERT_TZ does not accept 'Z'.
    //    => default use_z_timezone to false

    // In Qt, BEWARE:
    //      dt;  // QDateTime(2016-06-02 10:28:06.708 BST Qt::TimeSpec(LocalTime))
    //      dt.toString(Qt::ISODate);  // "2016-06-02T10:28:06" -- DROPS timezone
    if (!dt.isValid()) {
        return "";
    }
    const QString localtime = dt.toString("yyyy-MM-ddTHH:mm:ss.zzz");
    int offset_from_utc_s = dt.offsetFromUtc();
    // FOR TESTING: offsetFromUtcSec = -(3600 * 2.5);
    QString tzinfo;
    if (use_z_timezone && offset_from_utc_s == 0) {
        tzinfo = "Z";
    } else {
        const QString sign = offset_from_utc_s < 0 ? "-" : "+";
        offset_from_utc_s = abs(offset_from_utc_s);
        const int hours = offset_from_utc_s / 3600;
        const int minutes = (offset_from_utc_s % 3600) / 60;
        tzinfo += QString("%1%2:%3").arg(sign)
            .arg(hours, 2, 10, QChar('0'))
            .arg(minutes, 2, 10, QChar('0'));
        // http://stackoverflow.com/questions/2618414/convert-an-int-to-a-qstring-with-zero-padding-leading-zeroes
    }
    return localtime + tzinfo;
}


QString datetimeToIsoMsUtc(const QDateTime& dt, const bool use_z_timezone)
{
    const QDateTime utc_dt = dt.toTimeSpec(Qt::UTC);
    return datetimeToIsoMs(utc_dt, use_z_timezone);
}


QDate isoToDate(const QString& iso)
{
    // e.g. "2017-07-14"
    return QDate::fromString(iso, Qt::ISODate);
    // http://doc.qt.io/qt-5/qt.html#DateFormat-enum
    // Qt::ISODate:
    // ISO 8601 extended format: either YYYY-MM-DD for dates or
    // YYYY-MM-DDTHH:mm:ss, YYYY-MM-DDTHH:mm:ssTZD (e.g.,
    // 1997-07-16T19:20:30+01:00) for combined dates and times.
}


QDateTime isoToDateTime(const QString& iso)
{
    // e.g. "2017-07-14T"
    return QDateTime::fromString(iso, Qt::ISODateWithMs);
    // http://doc.qt.io/qt-5/qt.html#DateFormat-enum
    // Qt::ISODateWithMs:
    // ISO 8601 extended format, including milliseconds if applicable.

    // HOWEVER, source is more accurate than documentation... note that in
    // QDateTime::fromString (qdatetime.cpp), treated identically to ISODate!
}


QDateTime now()
{
    return QDateTime::currentDateTime();
}


QDate nowDate()
{
    return QDate::currentDate();
}


QString nowTimestamp()
{
    return timestampDateTime(now());
}


QString timestampDateTime(const QDateTime& dt)
{
    return dt.toString(TIMESTAMP_FORMAT);
}


QString timestampDateTime(const QVariant &dt)
{
    return dt.isNull() ? UNKNOWN : timestampDateTime(dt.toDateTime());
}


QString shortDateTime(const QDateTime& dt)
{
    return dt.toString(SHORT_DATETIME_FORMAT);
}


QString shortDateTime(const QVariant& dt)
{
    return dt.isNull() ? UNKNOWN : shortDateTime(dt.toDateTime());
}


QString textDateTime(const QDateTime& dt)
{
    return dt.toString(TEXT_DATETIME_FORMAT);
}


QString textDateTime(const QVariant& dt)
{
    return dt.isNull() ? UNKNOWN : textDateTime(dt.toDateTime());
}


QString shortDate(const QDate& d)
{
    return d.toString(SHORT_DATE_FORMAT);
}


QString shortDate(const QVariant &d)
{
    return d.isNull() ? UNKNOWN : shortDate(d.toDate());
}


QString textDate(const QDate& d)
{
    return d.toString(TEXT_DATE_FORMAT);
}


QString textDate(const QVariant& d)
{
    return d.isNull() ? UNKNOWN : textDate(d.toDate());
}


int ageYearsFrom(const QDate& from, const QDate& to)
{
    // Unhelpful:
    // https://forum.qt.io/topic/27906/difference-in-days-months-and-years-between-two-dates/9
    if (from > to) {
        return -ageYearsFrom(to, from);
    }
    // Now, "birthday age" calculation.
    // Examples:                                yeardiff    delta
    // * 1 Jan 2000 ->  1 Jan 2000 = age 0      0           0
    // * 1 Jan 2000 -> 31 Dec 2000 = age 0      0           0
    // * 1 Jun 2000 -> 31 Apr 2001 = age 0      1           -1
    // * 2 Jun 2000 ->  1 Jun 2001 = age 0      1           -1
    // * 2 Jun 2000 ->  2 Jun 2001 = age 1      1           0
    int years = to.year() - from.year();
    if (to.month() < from.month() ||
            (to.month() == from.month() && to.day() < from.day())) {
        years -= 1;
    }
    return years;
}


int ageYears(const QVariant& dob, const int default_years)
{
    if (dob.isNull()) {
        return default_years;
    }
    return ageYearsFrom(dob.toDate(), nowDate());
}


double doubleSecondsFrom(const QDateTime& from, const QDateTime& to)
{
    return msToSec(from.msecsTo(to));
}


double msToSec(const double ms)
{
    return ms / 1000.0;
}


double secToMin(const double sec)
{
    return sec / 60.0;
}


int secToIntMs(const double sec)
{
    return qRound(sec * 1000);
}


} // namespace datetime
