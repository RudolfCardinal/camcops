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
#include "db/databaseobject.h"
class CamcopsApp;


// Represents an extra string downloaded from the server.

class ExtraString : public DatabaseObject
{
public:
    // Specimen constructor:
    ExtraString(CamcopsApp& app, DatabaseManager& db);

    // String loading constructor:
    ExtraString(CamcopsApp& app, DatabaseManager& db,
                const QString& task,
                const QString& name,
                const QString& language_code);

    // String saving constructor:
    ExtraString(CamcopsApp& app, DatabaseManager& db,
                const QString& task,
                const QString& name,
                const QString& language_code,
                const QString& value);

    // Destructor
    virtual ~ExtraString() = default;

    // Returns the string's task.
    QString task() const;

    // Returns the string's name.
    QString name() const;

    // Returns the string's language.
    QString languageCode() const;

    // Returns the string's value.
    QString value() const;

    // Do any extra strings exist for the specified task?
    // (Resembles a Python classmethod; sort-of static function.)
    bool anyExist(const QString& task) const;

    // Delete all extra strings from the database.
    // (Resembles a Python classmethod; sort-of static function.)
    void deleteAllExtraStrings();

    // Make table indexes.
    // (Resembles a Python classmethod; sort-of static function.)
    void makeIndexes();

    // "Classmethod": count number of strings by language
    QMap<QString, int> getStringCountByLanguage() const;

public:
    static const QString TASK_FIELD;
    static const QString NAME_FIELD;
    static const QString LANGUAGE_FIELD;
    static const QString VALUE_FIELD;
};
