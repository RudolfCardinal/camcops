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

// #define DEBUG_LANGUAGE_LOOKUP

#include "extrastring.h"
#include "core/camcopsapp.h"
#include "db/databasemanager.h"
#include "db/dbfunc.h"

const QString EXTRASTRINGS_TABLENAME("extrastrings");
const QString ExtraString::TASK_FIELD("task");
const QString ExtraString::NAME_FIELD("name");
const QString ExtraString::LANGUAGE_FIELD("language");
const QString ExtraString::VALUE_FIELD("value");


// Specimen constructor:
ExtraString::ExtraString(CamcopsApp& app, DatabaseManager& db) :
    DatabaseObject(app, db, EXTRASTRINGS_TABLENAME, dbconst::PK_FIELDNAME,
                   true, false, false, false)
{
    // Define fields
    addField(TASK_FIELD, QVariant::String, true, false, false);
    addField(NAME_FIELD, QVariant::String, true, false, false);
    addField(LANGUAGE_FIELD, QVariant::String, false, false, false);
    addField(VALUE_FIELD, QVariant::String, false, false, false);
}


// String loading constructor:
ExtraString::ExtraString(CamcopsApp& app,
                         DatabaseManager& db,
                         const QString& task,
                         const QString& name,
                         const QString& language_code) :
    ExtraString(app, db)  // delegating constructor
{
    if (task.isEmpty() || name.isEmpty()) {
        // Specimen only
        return;
    }
    // Not a specimen; load, or set defaults and save

    // Qt, and (following Qt) the CamCOPS client, use underscores, e.g.
    // "en_GB". The normal practice for language tags is to use a hyphen, e.g.
    // "en-GB", as per:
    // - https://en.wikipedia.org/wiki/IETF_language_tag
    // - https://tools.ietf.org/html/rfc5646
    // However, the normal practice for locales is to use an underscore, as per
    // Python's "locale.getlocale()" and
    // https://en.wikipedia.org/wiki/Locale_(computer_software)
    // The CamCOPS server, and thus our downloaded strings, use the underscore.

#ifdef DEBUG_LANGUAGE_LOOKUP
    const QString debugprefix = QString("Lookup string %1.%2[%3]:")
            .arg(task, name, language_code);
#endif

    // 1. Exact language/country match.
    //    "language" is e.g. "en_GB".
    WhereConditions where_exact_lang;
    where_exact_lang.add(TASK_FIELD, task);
    where_exact_lang.add(NAME_FIELD, name);
    where_exact_lang.add(LANGUAGE_FIELD, language_code);
#ifdef DEBUG_LANGUAGE_LOOKUP
    qDebug().noquote() << debugprefix << where_exact_lang;
#endif
    if (load(where_exact_lang)) {
        return;
    }

    // 2. Match to language if not country
    const QString close_lang = language_code.left(2) + "%";
    //    "close_lang" is e.g. "en%".
    WhereConditions where_close_lang;
    where_close_lang.add(TASK_FIELD, task);
    where_close_lang.add(NAME_FIELD, name);
    where_close_lang.add(LANGUAGE_FIELD, "LIKE", close_lang);
#ifdef DEBUG_LANGUAGE_LOOKUP
    qDebug().noquote() << debugprefix << where_close_lang;
#endif
    if (load(where_close_lang)) {
        return;
    }

    // 3. Default language or blank.
    QString sql = QString("%1 = ? AND %2 = ? AND (%3 = ? OR %3 = '' OR %3 IS NULL)")
            .arg(dbfunc::delimit(TASK_FIELD),
                 dbfunc::delimit(NAME_FIELD),
                 dbfunc::delimit(LANGUAGE_FIELD));
    ArgList args{task, name, language_code};
    WhereConditions where_default_lang;
    where_default_lang.set(SqlArgs(sql, args));
#ifdef DEBUG_LANGUAGE_LOOKUP
    qDebug().noquote() << debugprefix << where_default_lang;
#endif
    load(where_default_lang);
}


// String saving constructor:
ExtraString::ExtraString(CamcopsApp& app,
                         DatabaseManager& db,
                         const QString& task,
                         const QString& name,
                         const QString& language_code,
                         const QString& value) :
    ExtraString(app, db)  // delegating constructor
{
    if (task.isEmpty() || name.isEmpty()) {
        qWarning() << Q_FUNC_INFO << "Using the save-blindly constructor "
                                     "without a name or task!";
        return;
    }
    setValue(TASK_FIELD, task);
    setValue(NAME_FIELD, name);
    setValue(LANGUAGE_FIELD, language_code);
    setValue(VALUE_FIELD, value);
    save();
}


QString ExtraString::task() const
{
    return valueString(TASK_FIELD);
}


QString ExtraString::name() const
{
    return valueString(NAME_FIELD);
}


QString ExtraString::languageCode() const
{
    return valueString(LANGUAGE_FIELD);
}


QString ExtraString::value() const
{
    return valueString(VALUE_FIELD);
}


bool ExtraString::anyExist(const QString& task) const
{
    WhereConditions where;
    where.add(TASK_FIELD, task);
    return m_db.count(EXTRASTRINGS_TABLENAME, where) > 0;
}


void ExtraString::deleteAllExtraStrings()
{
    m_db.deleteFrom(EXTRASTRINGS_TABLENAME);
}


void ExtraString::makeIndexes()
{
    m_db.createIndex("_idx_extrastrings_task_name",
                     EXTRASTRINGS_TABLENAME,
                     {ExtraString::TASK_FIELD,
                      ExtraString::NAME_FIELD});
}


QMap<QString, int> ExtraString::getStringCountByLanguage() const
{
    using dbfunc::delimit;
    const QString sql_languages(
        QString("SELECT DISTINCT(%1) FROM %2")
            .arg(delimit(LANGUAGE_FIELD),
                 delimit(EXTRASTRINGS_TABLENAME))
    );
    const QueryResult result_languages = m_db.query(sql_languages);
    QStringList languages = result_languages.firstColumnAsStringList();
    languages.sort();
    QMap<QString, int> count_by_language;
    for (const QString& language : languages) {
        const SqlArgs query_lang(
            QString("SELECT COUNT(*) FROM %1 WHERE %2 = ?")
                    .arg(delimit(EXTRASTRINGS_TABLENAME),
                         delimit(LANGUAGE_FIELD)),
            {language}
        );
        const int c = m_db.fetchInt(query_lang);
        count_by_language[language] = c;
    }
    return count_by_language;
}
