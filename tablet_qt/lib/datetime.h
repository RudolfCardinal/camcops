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

#pragma once
#include <QDateTime>
class QVariant;

namespace datetime {

extern const QString LONG_DATE_FORMAT;  // e.g. Thursday 31 Dec 2000
extern const QString TIMESTAMP_FORMAT;  // e.g. 2000-12-31 23:59:59.999
extern const QString SHORT_DATETIME_FORMAT;  // e.g. 2000-12-31 23:59
extern const QString ISO_DATE_FORMAT;  // e.g. 2000-12-31
extern const QString SHORT_DATE_FORMAT;  // e.g. 2000-12-31
extern const QString TEXT_DATE_FORMAT;  // e.g. 31 Dec 2000
extern const QString TEXT_DATETIME_FORMAT;  // e.g. // Thu 31 Dec 2000, 23:59
extern const QString UNKNOWN;  // "?"

// Converts a date to ISO_DATE_FORMAT
QString dateToIso(const QDate& d);

// Converts a date/time to an ISO-8601 format preserving millisecond accuracy
// and timezone. If "use_z_timezone" is true, use "Z" for UTC.
// Example: "2016-06-02T10:04:03.588+01:00"
QString datetimeToIsoMs(const QDateTime& dt, bool use_z_timezone = false);

// Converts a date/time to an ISO-8601 format, as per datetimeToIsoMs(),
// but also coerces it into the UTC equivalent.
QString datetimeToIsoMsUtc(const QDateTime& dt, bool use_z_timezone = false);

// Converts an ISO-format string, e.g. "2017-07-14", to a date.
QDate isoToDate(const QString& iso);

// Converts an ISO-format string into a date/time.
QDateTime isoToDateTime(const QString& iso);

// Returns the date/time now.
QDateTime now();

// Returns today's date.
QDate nowDate();

// Returns the date/time now, in TIMESTAMP_FORMAT.
QString nowTimestamp();

// Formats a date/time in TIMESTAMP_FORMAT.
QString timestampDateTime(const QDateTime& dt);

// Formats a date/time QVariant in TIMESTAMP_FORMAT, or UNKNOWN if null.
QString timestampDateTime(const QVariant& dt);

// Formats a date/time in SHORT_DATETIME_FORMAT.
QString shortDateTime(const QDateTime& dt);

// Formats a date/time QVariant in SHORT_DATETIME_FORMAT, or UNKNOWN if null.
QString shortDateTime(const QVariant& dt);

// Formats a date/time in TEXT_DATETIME_FORMAT.
QString textDateTime(const QDateTime& dt);

// Formats a date/time QVariant in TEXT_DATETIME_FORMAT, or UNKNOWN if null.
QString textDateTime(const QVariant& dt);

// Formats a date in SHORT_DATE_FORMAT.
QString shortDate(const QDate& d);

// Formats a date QVariant in SHORT_DATE_FORMAT, or UNKNOWN if null.
QString shortDate(const QVariant& d);

// Formats a date in TEXT_DATE_FORMAT.
QString textDate(const QDate& d);

// Formats a date QVariant in TEXT_DATE_FORMAT, or UNKNOWN if null.
QString textDate(const QVariant& d);

// Calculate "birthday" age (conventional age) that someone will be on "to"
// if they were born on "from".
int ageYearsFrom(const QDate& from, const QDate& to);

// Calculates "birthday" (conventional) age today if they were born on "dob",
// or "default_years" if "dob" is null.
int ageYears(const QVariant& dob, int default_years = -1);

// Calculates the number of seconds from "from" to "to".
double doubleSecondsFrom(const QDateTime& from, const QDateTime& to);

// Time conversions
double msToSec(double ms);
int secToIntMs(double sec);
double secToMin(double sec);

}  // namespace datetime
