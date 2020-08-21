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

// #define DEBUG_SPECIMEN_CREATION
// #define DEBUG_SAVES
// #define DEBUG_TRIGGERS_NEEDS_UPLOAD
#define SAVE_UPDATE_BACKGROUND  // .. this is the main point of multithreading
    // databases; to improve GUI response speed while still being able to
    // save at each touch to avoid data loss through user error.
#define ALLOW_SAVE_INSERT_BACKGROUND

#include "databaseobject.h"
#include <iostream>
#include <QDateTime>
#include <QMapIterator>
#include <QSqlField>
#include <QSqlQuery>
#include <QStringList>
#include "core/camcopsapp.h"
#include "db/databasemanager.h"
#include "db/dbfunc.h"
#include "db/blobfieldref.h"
#include "db/fieldref.h"
#include "db/queryresult.h"
#include "dbobjects/blob.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"

const QString NOT_NULL_ERROR("Error: attempting to save NULL to a NOT NULL "
                             "field:");


DatabaseObject::DatabaseObject(CamcopsApp& app,
                               DatabaseManager& db,
                               const QString& tablename,
                               const QString& pk_fieldname,
                               const bool has_modification_timestamp,
                               const bool has_creation_timestamp,
                               const bool has_move_off_tablet_field,
                               const bool triggers_need_upload) :
    m_app(app),
    m_db(db),
    m_tablename(tablename),
    m_pk_fieldname(pk_fieldname),
    m_has_modification_timestamp(has_modification_timestamp),
    m_has_move_off_tablet_field(has_move_off_tablet_field),
    m_triggers_need_upload(triggers_need_upload),
    m_exists_in_db(false)
{
    if (pk_fieldname.isEmpty()) {
        uifunc::stopApp(
            QString("DatabaseObject::DatabaseObject: Missing pk_fieldname; "
                    "table=%1").arg(m_tablename));
    }
    addField(pk_fieldname, QVariant::Int, true, true, true);
    if (has_move_off_tablet_field) {
        // Will be true for everything in data DB, but not system DB
        addField(dbconst::MOVE_OFF_TABLET_FIELDNAME, QVariant::Bool,
                 false, false, false);
    }
    if (has_modification_timestamp) {
        addField(dbconst::MODIFICATION_TIMESTAMP_FIELDNAME,
                 QVariant::DateTime);
    }
    if (has_creation_timestamp) {
        addField(dbconst::CREATION_TIMESTAMP_FIELDNAME,
                 QVariant::DateTime);
        QDateTime now = QDateTime::currentDateTime();
        m_record[dbconst::CREATION_TIMESTAMP_FIELDNAME].setValue(now);  // also: dirty
    }
}


// ============================================================================
// Adding fields
// ============================================================================

void DatabaseObject::addField(const QString& fieldname,
                              const QVariant::Type type,
                              const bool mandatory,
                              const bool unique,
                              const bool pk,
                              const QVariant& default_value)
{
    if (type == QVariant::ULongLong) {
        qWarning() << "SQLite3 does not properly support unsigned 64-bit "
                      "integers; please use signed if possible";
    }
    if (m_record.contains(fieldname)) {
        uifunc::stopApp("Attempt to insert duplicate fieldname: " + fieldname);
    }
    Field field(fieldname, type, mandatory, unique, pk, default_value);
    m_record.insert(fieldname, field);
    m_ordered_fieldnames.append(fieldname);
}


void DatabaseObject::addField(const QString& fieldname,
                              const QString& type_name,
                              const bool mandatory,
                              const bool unique,
                              const bool pk,
                              const QVariant& default_value)
{
    if (m_record.contains(fieldname)) {
        uifunc::stopApp("Attempt to insert duplicate fieldname: " + fieldname);
    }
    Field field(fieldname, type_name, mandatory, unique, pk, default_value);
    m_record.insert(fieldname, field);
    m_ordered_fieldnames.append(fieldname);
}


void DatabaseObject::addFields(const QStringList& fieldnames,
                               const QVariant::Type type,
                               const bool mandatory)
{
    for (const QString& fieldname : fieldnames) {
        addField(fieldname, type, mandatory);
    }
}


void DatabaseObject::addField(const Field& field)
{
    m_record.insert(field.name(), field);
    m_ordered_fieldnames.append(field.name());
}


bool DatabaseObject::hasField(const QString& fieldname) const
{
    return m_ordered_fieldnames.contains(fieldname);
}


QStringList DatabaseObject::fieldnames() const
{
    return m_ordered_fieldnames;
}


// ============================================================================
// Field access
// ============================================================================

// ----------------------------------------------------------------------------
// Set or modify a field
// ----------------------------------------------------------------------------

bool DatabaseObject::setValue(const QString& fieldname, const QVariant& value,
                              const bool touch_record)
{
    // In general, extra "default" initialization done in a constructor should
    // probably set touch_record = false, as otherwise creating a prototype
    // task makes the app think it needs to upload something.
    requireField(fieldname);
    const bool dirty = m_record[fieldname].setValue(value);
    if (dirty && touch_record) {
        touch();
        if (m_triggers_need_upload) {
#ifdef DEBUG_TRIGGERS_NEEDS_UPLOAD
            qDebug() << "Triggering setNeedsUpload() from field" << fieldname
                     << "value" << value;
#endif
            m_app.setNeedsUpload(true);
        }
    }
    if (dirty) {
        emit dataChanged();
    }
    return dirty;
}


bool DatabaseObject::setValue(const QString& fieldname,
                              const QVector<int>& value,
                              const bool touch_record)
{
    return setValue(fieldname, QVariant::fromValue(value), touch_record);
}


bool DatabaseObject::setValue(const QString& fieldname,
                              const QStringList& value,
                              const bool touch_record)
{
    return setValue(fieldname, QVariant::fromValue(value), touch_record);
}


void DatabaseObject::addToValueInt(const QString& fieldname,
                                   const int increment)
{
    setValue(fieldname, valueInt(fieldname) + increment);
}


// ----------------------------------------------------------------------------
// Read a field
// ----------------------------------------------------------------------------

QVariant DatabaseObject::value(const QString& fieldname) const
{
    requireField(fieldname);
    return m_record[fieldname].value();
}


QString DatabaseObject::prettyValue(const QString &fieldname,
                                    const int dp) const
{
    requireField(fieldname);
    return m_record[fieldname].prettyValue(dp);
}


bool DatabaseObject::valueIsNull(const QString &fieldname) const
{
    const QVariant v = value(fieldname);
    return v.isNull();
}


bool DatabaseObject::valueIsFalseNotNull(const QString &fieldname) const
{
    const QVariant v = value(fieldname);
    return !v.isNull() && !v.toBool();
}


bool DatabaseObject::valueIsNullOrEmpty(const QString &fieldname) const
{
    const QVariant v = value(fieldname);
    return v.isNull() || v.toString() == "";
}


bool DatabaseObject::valueBool(const QString& fieldname) const
{
    const QVariant v = value(fieldname);
    return v.toBool();
}


int DatabaseObject::valueInt(const QString& fieldname) const
{
    const QVariant v = value(fieldname);
    return v.toInt();
}


qint64 DatabaseObject::valueInt64(const QString& fieldname) const
{
    const QVariant v = value(fieldname);
    return v.toLongLong();
}


quint64 DatabaseObject::valueUInt64(const QString& fieldname) const
{
    const QVariant v = value(fieldname);
    return v.toULongLong();
}


double DatabaseObject::valueDouble(const QString& fieldname) const
{
    const QVariant v = value(fieldname);
    return v.toDouble();
}


QDateTime DatabaseObject::valueDateTime(const QString& fieldname) const
{
    const QVariant v = value(fieldname);
    return v.toDateTime();
}


QDate DatabaseObject::valueDate(const QString& fieldname) const
{
    const QVariant v = value(fieldname);
    return v.toDate();
}


QByteArray DatabaseObject::valueByteArray(const QString& fieldname) const
{
    const QVariant v = value(fieldname);
    return v.toByteArray();
}


QString DatabaseObject::valueString(const QString& fieldname) const
{
    const QVariant v = value(fieldname);
    return v.toString();
}


QStringList DatabaseObject::valueStringList(const QString& fieldname) const
{
    const QVariant v = value(fieldname);
    return v.toStringList();
}


QChar DatabaseObject::valueQChar(const QString& fieldname) const
{
    const QVariant v = value(fieldname);
    return v.toChar();
}


char DatabaseObject::valueLatin1Char(const QString& fieldname) const
{
    const QVariant v = value(fieldname);
    const QChar c = v.toChar();  // 16-bit char
    return c.toLatin1();  // 8-bit char
}


QVector<int> DatabaseObject::valueVectorInt(const QString& fieldname) const
{
    const QVariant v = value(fieldname);
    return v.value<QVector<int>>();
}


FieldRefPtr DatabaseObject::fieldRef(const QString& fieldname,
                                     const bool mandatory,
                                     const bool autosave,
                                     const bool blob)
{
    // If we ask for two fieldrefs to the same field, they need to be linked
    // (in terms of signals), and therefore the same underlying FieldRef
    // object. So we maintain a map.
    // If an existing FieldRef has been created for this field, that field
    // reference is re-used, regardless of the (subsequent) autosave setting.
    requireField(fieldname);
    if (!m_fieldrefs.contains(fieldname)) {
        CamcopsApp* p_app = &m_app;
        m_fieldrefs[fieldname] = FieldRefPtr(
            new FieldRef(this, fieldname, mandatory, autosave, blob, p_app));
    }
    return m_fieldrefs[fieldname];
}


BlobFieldRefPtr DatabaseObject::blobFieldRef(const QString& fieldname,
                                             const bool mandatory)
{
    requireField(fieldname);
    if (!m_fieldrefs.contains(fieldname)) {
        CamcopsApp* p_app = &m_app;
        m_fieldrefs[fieldname] = FieldRefPtr(
            new BlobFieldRef(this, fieldname, mandatory, p_app));
    }
    FieldRefPtr base_fr = m_fieldrefs[fieldname];
    BlobFieldRefPtr blob_fr = qSharedPointerDynamicCast<BlobFieldRef>(base_fr);
    Q_ASSERT(blob_fr.data());
    return blob_fr;
}


Field& DatabaseObject::getField(const QString& fieldname)
{
    // Dangerous in that it returns a reference.
    requireField(fieldname);
    return m_record[fieldname];
}


// ----------------------------------------------------------------------------
// Read multiple fields
// ----------------------------------------------------------------------------

QVector<QVariant> DatabaseObject::values(const QStringList& fieldnames) const
{
    QVector<QVariant> values;
    for (const QString& fieldname : fieldnames) {
        values.append(value(fieldname));
    }
    return values;
}


bool DatabaseObject::allValuesTrue(const QStringList& fieldnames) const
{
    for (const QString& fieldname : fieldnames) {
        if (!valueBool(fieldname)) {
            return false;
        }
    }
    return true;
}


bool DatabaseObject::anyValuesTrue(const QStringList& fieldnames) const
{
    for (const QString& fieldname : fieldnames) {
        if (valueBool(fieldname)) {
            return true;
        }
    }
    return false;
}


bool DatabaseObject::allValuesFalseOrNull(const QStringList& fieldnames) const
{
    return !anyValuesTrue(fieldnames);
}


bool DatabaseObject::allValuesFalse(const QStringList& fieldnames) const
{
    for (const QString& fieldname : fieldnames) {
        if (!valueIsFalseNotNull(fieldname)) {
            return false;
        }
    }
    return true;
}


bool DatabaseObject::anyValuesFalse(const QStringList& fieldnames) const
{
    for (const QString& fieldname : fieldnames) {
        if (valueIsFalseNotNull(fieldname)) {
            return true;
        }
    }
    return false;
}


bool DatabaseObject::anyValuesNull(const QStringList& fieldnames) const
{
    for (const QString& fieldname : fieldnames) {
        if (valueIsNull(fieldname)) {
            return true;
        }
    }
    return false;
}


bool DatabaseObject::noValuesNull(const QStringList& fieldnames) const
{
    for (const QString& fieldname : fieldnames) {
        if (valueIsNull(fieldname)) {
            return false;
        }
    }
    return true;
}


bool DatabaseObject::anyValuesNullOrEmpty(const QStringList& fieldnames) const
{
    for (const QString& fieldname : fieldnames) {
        if (valueIsNullOrEmpty(fieldname)) {
            return true;
        }
    }
    return false;
}


bool DatabaseObject::noValuesNullOrEmpty(const QStringList& fieldnames) const
{
    for (const QString& fieldname : fieldnames) {
        if (valueIsNullOrEmpty(fieldname)) {
            return false;
        }
    }
    return true;
}


// ============================================================================
// PK access
// ============================================================================

QVariant DatabaseObject::pkvalue() const
{
    return value(pkname());
}


int DatabaseObject::pkvalueInt() const
{
    const QVariant pk = pkvalue();
    return pk.isNull() ? dbconst::NONEXISTENT_PK : pk.toInt();
}


// ============================================================================
// Whole-object summary
// ============================================================================

QString DatabaseObject::fieldSummary(const QString& fieldname,
                                     const QString& altname,
                                     const QString& separator,
                                     const QString& suffix) const
{
    const QString name = altname.isEmpty() ? fieldname : altname;
    return stringfunc::standardResult(name, prettyValue(fieldname),
                                      separator, suffix);
}


QString DatabaseObject::fieldSummaryYesNo(const QString& fieldname,
                                          const QString& altname,
                                          const QString& separator,
                                          const QString& suffix) const
{
    const QString name = altname.isEmpty() ? fieldname : altname;
    return stringfunc::standardResult(name,
                                      uifunc::yesNo(valueBool(fieldname)),
                                      separator, suffix);
}


QString DatabaseObject::fieldSummaryYesNoNull(const QString& fieldname,
                                              const QString& altname,
                                              const QString& separator,
                                              const QString& suffix) const
{
    const QString name = altname.isEmpty() ? fieldname : altname;
    return stringfunc::standardResult(name,
                                      uifunc::yesNoNull(value(fieldname)),
                                      separator, suffix);
}


QString DatabaseObject::fieldSummaryYesNoUnknown(const QString& fieldname,
                                                 const QString& altname,
                                                 const QString& separator,
                                                 const QString& suffix) const
{
    const QString name = altname.isEmpty() ? fieldname : altname;
    return stringfunc::standardResult(name,
                                      uifunc::yesNoUnknown(value(fieldname)),
                                      separator, suffix);
}


QString DatabaseObject::fieldSummaryTrueFalseUnknown(
        const QString& fieldname,
        const QString& altname,
        const QString& separator,
        const QString& suffix) const
{
    const QString name = altname.isEmpty() ? fieldname : altname;
    return stringfunc::standardResult(name,
                                      uifunc::trueFalseUnknown(value(fieldname)),
                                      separator, suffix);
}


QString DatabaseObject::fieldSummaryNameValueOptions(
        const QString& fieldname,
        const NameValueOptions& options,
        const QString& altname,
        const QString& separator,
        const QString& suffix) const
{
    const QString name = altname.isEmpty() ? fieldname : altname;
    const QVariant v = value(fieldname);
    const QString pretty_value = options.nameFromValue(v);
    return stringfunc::standardResult(name, pretty_value,
                                      separator, suffix);
}


QStringList DatabaseObject::recordSummaryLines(const QString& separator,
                                               const QString& suffix) const
{
    QStringList list;
    for (const QString& fieldname : m_ordered_fieldnames) {
        const Field& field = m_record[fieldname];
        list.append(stringfunc::standardResult(field.name(), field.prettyValue(),
                                               separator, suffix));
    }
    return list;
}


QString DatabaseObject::recordSummaryString(const QString& separator,
                                            const QString& suffix) const
{
    return recordSummaryLines(separator, suffix).join("<br>");
}


QString DatabaseObject::recordSummaryCSVString(
        const QString& equals_separator,
        const QString& comma_separator) const
{
    return recordSummaryLines(equals_separator, "").join(comma_separator);
}


// ============================================================================
// Loading, saving
// ============================================================================

bool DatabaseObject::load(const int pk)
{
    if (pk == dbconst::NONEXISTENT_PK) {
#ifdef DEBUG_SPECIMEN_CREATION
        qDebug() << "Ignoring DatabaseObject::load() call for explicitly "
                    "invalid PK";
#endif
        return false;
    }
    WhereConditions where;
    where.add(pkname(), QVariant(pk));
    return load(where);
}


bool DatabaseObject::load(const QString& fieldname,
                          const QVariant& where_value)
{
    if (!m_record.contains(fieldname)) {
        qCritical() << Q_FUNC_INFO
                    << "Attempt to load with nonexistent fieldname:"
                    << fieldname;
        nullify();
        return false;
    }
    WhereConditions where;
    where.add(fieldname, where_value);
    return load(where);
}


bool DatabaseObject::load(const WhereConditions& where)
{
    const SqlArgs sqlargs = fetchQuerySql(where);
    const QueryResult result = m_db.query(sqlargs, QueryResult::FetchMode::FetchFirst);
    const bool found = result.nRows() > 0;
    if (found) {
        setFromQuery(result, 0, true);
        // ... uses the first result found
        // ... sets m_exists_in_db
        loadAllAncillary();
    } else {
        nullify();  // clears m_exists_in_db
    }
    return found;
}


SqlArgs DatabaseObject::fetchQuerySql(const WhereConditions& where,
                                      const OrderBy& order_by)
{
    const QStringList fields = fieldnamesMapOrder();
    QStringList delimited_fieldnames;
    for (int i = 0; i < fields.size(); ++i) {
        delimited_fieldnames.append(dbfunc::delimit(fields.at(i)));
    }
    const QString sql = (
        "SELECT " + delimited_fieldnames.join(", ") + " FROM " +
        dbfunc::delimit(tablename())
    );
    const ArgList args;
    SqlArgs sqlargs(sql, args);
    where.appendWhereClauseTo(sqlargs);
    dbfunc::addOrderByClause(order_by, sqlargs);
    return sqlargs;
}


void DatabaseObject::setFromQuery(const QueryResult& query_result,
                                  const int row,
                                  const bool order_matches_fetchquery)
{
    MutableMapIteratorType it(m_record);
    // Note: QMap iteration is ordered; http://doc.qt.io/qt-5/qmap.html
    if (order_matches_fetchquery) {  // faster
        int field_index = -1;
        while (it.hasNext()) {
            it.next();
            ++field_index;
            it.value().setFromDatabaseValue(query_result.at(row, field_index));
        }
    } else {
        while (it.hasNext()) {
            it.next();
            QString fieldname = it.key();
            // Empirically, these fieldnames are fine: no delimiting quotes,
            // despite use of delimiters in the SELECT SQL.
            // qDebug().noquote() << "fieldname:" << fieldname;
            it.value().setFromDatabaseValue(query_result.at(row, fieldname));
        }
    }
    m_exists_in_db = true;

    // And also:
    loadAllAncillary();

    emit dataChanged();
}


bool DatabaseObject::save()
{
    touch(true);  // set timestamp only if timestamp not set
    if (!anyDirty()) {
        return true;  // nothing to do, so let's not bother the database
    }
    bool success;
    if (m_exists_in_db) {
        success = saveUpdate();
    } else {
        success = saveInsert(isPkNull());
    }
    clearAllDirty();
    m_exists_in_db = success;
    return success;
}


void DatabaseObject::saveWithoutKeepingPk()
{
    // As for save(), but we give this function a separate name because it
    // is dangerous. It saves new objects without storing their PK back.
    // Only used for rapid background saves of things like ExtraString.
    touch(true);
    if (!anyDirty()) {
        return;
    }
    if (m_exists_in_db) {
        saveUpdate();
    } else {
        saveInsert(false);
    }
    m_exists_in_db = true;
    clearAllDirty();
}


void DatabaseObject::nullify()
{
    MapIteratorType i(m_record);
    while (i.hasNext()) {
        i.next();
        Field field = i.value();
        field.nullify();
    }
    m_exists_in_db = false;
    emit dataChanged();
}


bool DatabaseObject::isPkNull() const
{
    const QVariant v = pkvalue();
    return v.isNull();
}


void DatabaseObject::touch(const bool only_if_unset)
{
    if (!m_has_modification_timestamp) {
        return;
    }
    if (only_if_unset &&
            !m_record[dbconst::MODIFICATION_TIMESTAMP_FIELDNAME].isNull()) {
        return;
    }
    // Don't set the timestamp value with setValue()! Infinite loop.
    const QDateTime now = QDateTime::currentDateTime();
    m_record[dbconst::MODIFICATION_TIMESTAMP_FIELDNAME].setValue(now);  // also: dirty
    emit dataChanged();
}


void DatabaseObject::setAllDirty()
{
    MutableMapIteratorType i(m_record);
    while (i.hasNext()) {
        i.next();
        i.value().setDirty();
    }
    emit dataChanged();
}


bool DatabaseObject::existsInDb() const
{
    return m_exists_in_db;
}


// ============================================================================
// Batch operations
// ============================================================================

QVector<int> DatabaseObject::getAllPKs() const
{
    return m_db.getPKs(m_tablename, m_pk_fieldname);
}


// ============================================================================
// Deleting
// ============================================================================

void DatabaseObject::deleteFromDatabase()
{
    const QVariant pk = pkvalue();
    if (pk.isNull()) {
        qWarning() << "Attempting to delete a DatabaseObject with a "
                      "NULL PK; ignored";
        return;
    }

    // ------------------------------------------------------------------------
    // Delete any associated BLOBs
    // ------------------------------------------------------------------------
    // There is no automatic way of knowing if we possess a BLOB, since a
    // BLOB field is simply an integer FK to the BLOB table.
    // However, we can reliably do it the other way round, and, moreover,
    // delete all associated BLOBs in one DELETE command:
    WhereConditions where_blob;
    where_blob.add(Blob::SRC_TABLE_FIELDNAME, tablename());
    where_blob.add(Blob::SRC_PK_FIELDNAME, pk);
    if (!m_db.deleteFrom(Blob::TABLENAME, where_blob)) {
        qWarning() << "Failed to delete BLOB(s) where:" << where_blob;
    }

    // ------------------------------------------------------------------------
    // Delete associated ancillary objects
    // ------------------------------------------------------------------------
    // - How best to do this?
    //
    // - We could register table/fkname pairs. That allows us to delete
    //   ancillary objects without instantiating a C++ object. And it allows
    //   that deletion in SQL, so we can delete multiple ancillaries in one go,
    //   rather than via C++ per-object deletion. However, it doesn't let us
    //   create a hierarchy (an ancillary of an ancillary, in an arbitrary way;
    //   the best would be registration with a "top-level" C++ object that
    //   deletes all its ancillaries (but then the FK system would be more
    //   complex).
    //   To do automatic BLOB deletion... would need to
    //   (1) SELECT all the ancillary PKs based on FK to primary object PK
    //   (2) delete from the ancillary table
    //   ... again, this either supports only a single level of inheritance,
    //   or one would have to store the top-level PK within all levels (mind
    //   you, you'd probably have to do that anyway).
    //
    // - We could have a chain of C++ objects that register themselves with
    //   their parents. As long as they are all loaded, they could then
    //   delete themselves and their children (ad infinitum). This would
    //   handle BLOBs neatly. Might make menu loading a bit less efficient.
    //   They'd have to autoload when their owner does. Mind you, that's often
    //   what we want to do anyway...
    //
    // - We could look at SQLite's ON DELETE CASCADE.
    //   However, that would require us to rework the BLOB system (e.g. one
    //   BLOB table per task/ancillary), which could get less elegant.
    //   Plus, there are other ways to mess up FKs in SQLite.
    //
    // DECISION:
    // - The C++ object way.
    //
    // OTHER CONSIDERATION:
    // - If objects load their ancillaries as they are themselves loaded, can
    //   SQLite cope? The potential is for two simultaneous queries:
    //      - load tasks T1, T2, T3
    //      - during load of T1, T1 wishes to load A1a, A1b, A1c
    //          - will the ancillary query fail?
    //          - If not, will the top-level query proceed to T2?
    for (const DatabaseObjectPtr& ancillary : getAllAncillary()) {
        ancillary->deleteFromDatabase();
    }

    // ------------------------------------------------------------------------
    // Delete ourself
    // ------------------------------------------------------------------------
    WhereConditions where_self;
    where_self.add(pkname(), pk);
    const bool success = m_db.deleteFrom(m_tablename, where_self);
    if (success) {
        nullify();
        m_app.setNeedsUpload(true);
    } else {
        qWarning() << "Failed to delete object with PK" << pk
                   << "from table" << m_tablename;
    }
}


// ============================================================================
// Debugging
// ============================================================================

void DatabaseObject::requireField(const QString &fieldname) const
{
    if (!m_record.contains(fieldname)) {
        uifunc::stopApp("DatabaseObject::requireField: Database object with "
                        "tablename '" + m_tablename + "' does not contain "
                        "field: " + fieldname);
    }
}


QString DatabaseObject::debugDescription() const
{
    return QString("DatabaseObject(tablename=%1,%2=%3)").arg(
                tablename(),
                pkname(),
                QString::number(pkvalueInt()));
}


// ============================================================================
// Special field access
// ============================================================================

bool DatabaseObject::shouldMoveOffTablet() const
{
    if (!m_has_move_off_tablet_field) {
        qWarning() << Q_FUNC_INFO << "m_has_move_off_tablet_field is false";
        return false;
    }
    return valueBool(dbconst::MOVE_OFF_TABLET_FIELDNAME);
}


void DatabaseObject::setMoveOffTablet(const bool move_off)
{
    if (!m_has_move_off_tablet_field) {
        qWarning() << Q_FUNC_INFO << "m_has_move_off_tablet_field is false";
        return;
    }
    setValue(dbconst::MOVE_OFF_TABLET_FIELDNAME, move_off, false);
    save();

    for (const DatabaseObjectPtr& ancillary : getAllAncillary()) {
        ancillary->setMoveOffTablet(move_off);
    }
}


void DatabaseObject::toggleMoveOffTablet()
{
    setMoveOffTablet(!shouldMoveOffTablet());
}


// ============================================================================
// DDL
// ============================================================================

QString DatabaseObject::sqlCreateTable() const
{
    return dbfunc::sqlCreateTable(m_tablename, fieldsOrdered());
}


QString DatabaseObject::tablename() const
{
    return m_tablename;
}


QString DatabaseObject::pkname() const
{
    return m_pk_fieldname;
}


void DatabaseObject::makeTable()
{
    m_db.createTable(m_tablename, fieldsOrdered());
    for (const DatabaseObjectPtr& specimen : getAncillarySpecimens()) {
        specimen->makeTable();
    }
}


DatabaseManager& DatabaseObject::database() const
{
    return m_db;
}


// ========================================================================
// Internals: ancillary management
// ========================================================================

void DatabaseObject::loadAllAncillary()
{
    const int pk = pkvalueInt();
    loadAllAncillary(pk);
}


void DatabaseObject::loadAllAncillary(const int pk)
{
    Q_UNUSED(pk)
}


QVector<DatabaseObjectPtr> DatabaseObject::getAllAncillary() const
{
    return QVector<DatabaseObjectPtr>();
}


QVector<DatabaseObjectPtr> DatabaseObject::getAncillarySpecimens() const
{
    return QVector<DatabaseObjectPtr>();
}


// ========================================================================
// Additional protected
// ========================================================================

bool DatabaseObject::saveInsert(const bool read_pk_from_database)
{
    ArgList args;
    QStringList fieldnames;
    QStringList placeholders;
    MapIteratorType i(m_record);
    while (i.hasNext()) {
        i.next();
        const QString fieldname = i.key();
        const Field field = i.value();
        if (field.isPk() && field.value().isNull()) {
            // If we are performing an INSERT command with a null PK, we expect
            // the database to create the PK via autonumbering, so we omit it
            // from the fields being written by value.
            continue;
        }
        fieldnames.append(dbfunc::delimit(fieldname));
        args.append(field.databaseValue());  // not field.value()
        placeholders.append("?");
        if (field.isMandatory() && field.isNull()) {
            qWarning() << NOT_NULL_ERROR << fieldname;
        }
    }
    const QString sql = (
        "INSERT OR REPLACE INTO " + dbfunc::delimit(m_tablename) +
        " (" +
        fieldnames.join(", ") +
        ") VALUES (" +
        placeholders.join(", ") +
        ")"
    );
#ifdef ALLOW_SAVE_INSERT_BACKGROUND
    // ------------------------------------------------------------------------
    // ALLOW_SAVE_INSERT_BACKGROUND is defined
    // ------------------------------------------------------------------------
    if (read_pk_from_database) {
        // INSERT and write the autogenerated PK value to the C++ object.
        // (Therefore, we have to wait for the result.)
        const QueryResult result = m_db.query(sql, args,
                                              QueryResult::FetchMode::NoFetch);
        if (!result.succeeded()) {
            qCritical() << Q_FUNC_INFO << "Failed to INSERT record into table"
                        << m_tablename;
            return false;
        }
        const QVariant new_pk = result.lastInsertId();
        setValue(pkname(), new_pk);
#ifdef DEBUG_SAVES
        qDebug().nospace() << "Save/insert: " << qUtf8Printable(m_tablename)
                           << ", " << pkname() << "=" << new_pk;
#endif
    } else {
        // INSERT and forget about the new autogenerated PK value.
        // This is the proper "background" insert.
        m_db.execNoAnswer(sql, args);
#ifdef DEBUG_SAVES
        qDebug() << "Background save/insert for table:"
                 << qUtf8Printable(m_tablename);
#endif
    }
#else
    // ------------------------------------------------------------------------
    // ALLOW_SAVE_INSERT_BACKGROUND is not defined
    // ------------------------------------------------------------------------
    Q_UNUSED(read_pk_from_database)
    QueryResult result = m_db.query(sql, args,
                                    QueryResult::FetchMode::NoFetch);
    if (!result.succeeded()) {
        qCritical() << Q_FUNC_INFO << "Failed to INSERT record into table"
                    << m_tablename;
        return false;
    }
    QVariant new_pk = result.lastInsertId();
    setValue(pkname(), new_pk);
#ifdef DEBUG_SAVES
    qDebug().nospace() << "Save/insert: " << qUtf8Printable(m_tablename)
                       << ", " << pkname() << "=" << new_pk;
#endif
#endif
    return true;
}


bool DatabaseObject::saveUpdate()
{
#ifdef DEBUG_SAVES
    qDebug().nospace() << "Save/update: " << qUtf8Printable(m_tablename)
                       << ", " << pkname() << "=" << pkvalue();
#endif
    ArgList args;
    QStringList fieldnames;
    MapIteratorType i(m_record);
    while (i.hasNext()) {
        i.next();
        const QString fieldname = i.key();
        const Field field = i.value();
        if (field.isDirty()) {
            fieldnames.append(dbfunc::delimit(fieldname) + "=?");
            args.append(field.databaseValue());  // not field.value()
            if (field.isMandatory() && field.isNull()) {
                qWarning() << NOT_NULL_ERROR << fieldname;
            }
        }
    }
    if (fieldnames.isEmpty()) {
#ifdef DEBUG_SAVES
        qDebug() << "... no dirty fields; nothing to do";
#endif
        return true;
    }
    const QString sql = (
        "UPDATE " + dbfunc::delimit(m_tablename) + " SET " +
        fieldnames.join(", ") +
        " WHERE " + dbfunc::delimit(pkname()) + "=?"
    );
    args.append(pkvalue());
#ifdef SAVE_UPDATE_BACKGROUND
    m_db.execNoAnswer(sql, args);
    return true;
#else
    bool success = m_db.exec(sql, args);
    if (!success) {
        qCritical() << Q_FUNC_INFO << "Failed to UPDATE record into table"
                    << m_tablename;
        return success;
    }
    return success;
#endif
}


void DatabaseObject::clearAllDirty()
{
    MutableMapIteratorType i(m_record);
    while (i.hasNext()) {
        i.next();
        i.value().clearDirty();
    }
}


bool DatabaseObject::anyDirty() const
{
    MapIteratorType i(m_record);
    while (i.hasNext()) {
        i.next();
        if (i.value().isDirty()) {
            return true;
        }
    }
    return false;
}

QStringList DatabaseObject::fieldnamesMapOrder() const
{
    QStringList fieldnames;
    MapIteratorType i(m_record);
    while (i.hasNext()) {
        i.next();
        fieldnames.append(i.key());
    }
    return fieldnames;
}

QVector<Field> DatabaseObject::fieldsOrdered() const
{
    QVector<Field> ordered_fields;
    for (const QString& fieldname : m_ordered_fieldnames) {
        ordered_fields.append(m_record[fieldname]);
    }
    return ordered_fields;
}


// ========================================================================
// For friends
// ========================================================================

QDebug operator<<(QDebug debug, const DatabaseObject& d)
{
    debug << d.m_tablename << " record: " << d.m_record << "\n";
    // use m_record.count() to get the number of fields
    // use m_record.field(i) to get the field at position i
    // use m_record.value(i) to get the value at position i
    // use m_record.value(fieldname) similarly
    // debug << "... Creation: " << qUtf8Printable(d.sqlCreateTable());
    return debug;
}
