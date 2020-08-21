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
#include <QDate>
#include <QDateTime>
#include <QMap>
#include <QString>
#include "common/aliases_camcops.h"
#include "common/aliases_qt.h"
#include "common/dbconst.h"
#include "db/field.h"
#include "db/sqlargs.h"
#include "db/whereconditions.h"

class CamcopsApp;
class NameValueOptions;
class QueryResult;


const QString DBOBJECT_DEFAULT_SEPARATOR = " = ";
const QString DBOBJECT_DEFAULT_SUFFIX = "";


// A database object representing a single row of a table.
//
// It supports a single integer PK field, and some other default facilities
// (like a modification timestamp field).
//
// Extend it to add more fields.
//
class DatabaseObject : public QObject
{
    Q_OBJECT  // so our derived classes can be too
public:
    // ========================================================================
    // Constructor/destructor
    // ========================================================================

    // Subclass constructors should follow a specific format, in which they:
    //
    // - create all fields, via addField() or addFields()
    // - if they're the final subclass, they should call load()
    //
    // Args:
    //  app:
    //      the CamCOPS application
    //  db:
    //      the database manager for the database this object will live in
    //  tablename:
    //      this object's database table name
    //  pk_fieldname:
    //      the name of a field (column) containing an integer primary key (PK)
    //  has_modification_timestamp:
    //      add the "when_last_modified" field and track modification time?
    //  has_creation_timestamp:
    //      add the "when_created" field and track creation time?
    //  has_move_off_tablet_field:
    //      add the "_move_off_tablet" field (used by upload code)?
    //  triggers_need_upload:
    //      if true, writing new data to this object tells the app that it
    //      needs to upload
    DatabaseObject(CamcopsApp& app,
                   DatabaseManager& db,
                   const QString& tablename,
                   const QString& pk_fieldname = dbconst::PK_FIELDNAME,
                   bool has_modification_timestamp = true,
                   bool has_creation_timestamp = false,
                   bool has_move_off_tablet_field = true,
                   bool triggers_need_upload = true);
    virtual ~DatabaseObject() = default;

    // ========================================================================
    // Adding fields
    // ========================================================================

    // Adds a field.
    // Args:
    //  mandatory:
    //      NOT NULL?
    //  unique:
    //      apply a UNIQUE constraint?
    //  pk:
    //      apply a PRIMARY KEY constraint?
    //  default_value:
    //      value for fields that haven't been written to (or read from the
    //      database)
    void addField(const QString& fieldname,
                  QVariant::Type type,
                  bool mandatory = false,
                  bool unique = false,
                  bool pk = false,
                  const QVariant& default_value = QVariant());

    // Alternative version of addField().
    void addField(const QString& fieldname,
                  const QString& type_name,
                  bool mandatory = false,
                  bool unique = false,
                  bool pk = false,
                  const QVariant& default_value = QVariant());

    // Alternative version of addField().
    void addField(const Field& field);

    // Bulk field addition.
    void addFields(const QStringList& fieldnames, QVariant::Type type,
                   bool mandatory = false);

    // Do we have the specified field?
    bool hasField(const QString& fieldname) const;

    // Return all fieldnames.
    QStringList fieldnames() const;

    // ========================================================================
    // Field access
    // ========================================================================

    // ------------------------------------------------------------------------
    // Set or modify a field
    // ------------------------------------------------------------------------

    // Sets a field's value.
    bool setValue(const QString& fieldname, const QVariant& value,
                  bool touch_record = true);  // returns: changed?

    // Sets a field's value.
    bool setValue(const QString& fieldname, const QVector<int>& value,
                  bool touch_record = true);  // returns: changed?

    // Sets a field's value.
    bool setValue(const QString& fieldname, const QStringList& value,
                  bool touch_record = true);  // returns: changed?

    // Adds an increment to an integer field's value.
    void addToValueInt(const QString& fieldname, int increment);

    // ------------------------------------------------------------------------
    // Read a field
    // ------------------------------------------------------------------------

    // Returns a field's value.
    QVariant value(const QString& fieldname) const;

    // Returns a field's value, pretty-formatted. For display only.
    QString prettyValue(const QString& fieldname, int dp = -1) const;

    // Is the field's value NULL?
    bool valueIsNull(const QString& fieldname) const;

    // Is the (boolean) field's value false or NULL?
    bool valueIsFalseNotNull(const QString& fieldname) const;

    // Is the (string) field's value "" or NULL?
    bool valueIsNullOrEmpty(const QString& fieldname) const;

    // Returns a field's value as a bool.
    bool valueBool(const QString& fieldname) const;

    // Returns a field's value as an int.
    int valueInt(const QString& fieldname) const;

    // Returns a field's value as a qint64 (qlonglong).
    qint64 valueInt64(const QString& fieldname) const;

    // Returns a field's value as a quint64 (qulonglong).
    quint64 valueUInt64(const QString& fieldname) const;

    // Returns a field's value as a double.
    double valueDouble(const QString& fieldname) const;

    // Returns a field's value as a QDateTime.
    QDateTime valueDateTime(const QString& fieldname) const;

    // Returns a field's value as a QDate.
    QDate valueDate(const QString& fieldname) const;

    // Returns a field's value as a QByteArray.
    QByteArray valueByteArray(const QString& fieldname) const;

    // Returns a field's value as a QString.
    QString valueString(const QString& fieldname) const;

    // Returns a field's value as a QStringList (for CSV-encoded fields).
    QStringList valueStringList(const QString& fieldname) const;

    // Returns a field's value as a QChar (for single-character string fields).
    QChar valueQChar(const QString& fieldname) const;

    // Returns a field's value as a char.
    char valueLatin1Char(const QString& fieldname) const;

    // Returns a field's value as a QVector<int> (for CSV-encoded fields).
    QVector<int> valueVectorInt(const QString& fieldname) const;

    // Returns a FieldRef (q.v.) pointer for the specified field.
    FieldRefPtr fieldRef(const QString& fieldname,
                         bool mandatory = true,
                         bool autosave = true,
                         bool blob = false);

    // Returns a BlobFieldRef (q.v.) pointer for the specified field.
    BlobFieldRefPtr blobFieldRef(const QString& fieldname, bool mandatory);

protected:
    // Low-level field access (for internal use only)
    // Returns the Field object for a given fieldname.
    Field& getField(const QString& fieldname);

    // ------------------------------------------------------------------------
    // Read multiple fields
    // ------------------------------------------------------------------------

public:
    // Returns all values for the specified fieldnames.
    QVector<QVariant> values(const QStringList& fieldnames) const;

    // Are all the values true?
    bool allValuesTrue(const QStringList& fieldnames) const;

    // Are any of the values true?
    bool anyValuesTrue(const QStringList& fieldnames) const;

    // Are all of the values false or null?
    bool allValuesFalseOrNull(const QStringList& fieldnames) const;

    // Are all of the values false (not true or null)?
    bool allValuesFalse(const QStringList& fieldnames) const;

    // Are any of the values false (not true or null)?
    bool anyValuesFalse(const QStringList& fieldnames) const;

    // Are any of the values null?
    bool anyValuesNull(const QStringList& fieldnames) const;

    // Are none of the values null?
    bool noValuesNull(const QStringList& fieldnames) const;

    // Are any of the values null or empty strings?
    bool anyValuesNullOrEmpty(const QStringList& fieldnames) const;

    // Are none of the values null or empty strings?
    bool noValuesNullOrEmpty(const QStringList& fieldnames) const;

    // ========================================================================
    // PK access
    // ========================================================================

public:
    // Returns the PK value, as a QVariant.
    QVariant pkvalue() const;

    // Returns the PK value, as an int, or dbconst::NONEXISTENT_PK.
    int pkvalueInt() const;

    // ========================================================================
    // Whole-object summary
    // ========================================================================

public:
    // Returns a summary for this field of the style "name = <b>value</b>".
    // Uses prettyValue() to provide the value.
    QString fieldSummary(
            const QString& fieldname,
            const QString& altname = "",
            const QString& separator = DBOBJECT_DEFAULT_SEPARATOR,
            const QString& suffix = DBOBJECT_DEFAULT_SUFFIX) const;

    // Like fieldSummary(); for boolean fields; value is Yes/No.
    QString fieldSummaryYesNo(
            const QString& fieldname,
            const QString& altname,
            const QString& separator = DBOBJECT_DEFAULT_SEPARATOR,
            const QString& suffix = DBOBJECT_DEFAULT_SUFFIX) const;

    // Like fieldSummary(); for boolean fields; value is Yes/No/NULL.
    QString fieldSummaryYesNoNull(
            const QString& fieldname,
            const QString& altname,
            const QString& separator = DBOBJECT_DEFAULT_SEPARATOR,
            const QString& suffix = DBOBJECT_DEFAULT_SUFFIX) const;

    // Like fieldSummary(); for boolean fields; value is Yes/No/Unknown.
    QString fieldSummaryYesNoUnknown(
            const QString& fieldname,
            const QString& altname,
            const QString& separator = DBOBJECT_DEFAULT_SEPARATOR,
            const QString& suffix = DBOBJECT_DEFAULT_SUFFIX) const;

    // Like fieldSummary(); for boolean fields; value is True/False/Unknown.
    QString fieldSummaryTrueFalseUnknown(
            const QString& fieldname,
            const QString& altname,
            const QString& separator = DBOBJECT_DEFAULT_SEPARATOR,
            const QString& suffix = DBOBJECT_DEFAULT_SUFFIX) const;

    // Like fieldSummary(); value is encoded via a NameValueOptions objects
    QString fieldSummaryNameValueOptions(
            const QString& fieldname,
            const NameValueOptions& options,
            const QString& altname,
            const QString& separator = DBOBJECT_DEFAULT_SEPARATOR,
            const QString& suffix = DBOBJECT_DEFAULT_SUFFIX) const;

    // Returns a list of default field summaries, one per field.
    QStringList recordSummaryLines(
            const QString& separator = DBOBJECT_DEFAULT_SEPARATOR,
            const QString& suffix = DBOBJECT_DEFAULT_SUFFIX) const;

    // Returns recordSummaryLines() joined by "<br>".
    QString recordSummaryString(
            const QString& separator = DBOBJECT_DEFAULT_SEPARATOR,
            const QString& suffix = DBOBJECT_DEFAULT_SUFFIX) const;

    // Returns a list of default field summaries, one per field.
    QString recordSummaryCSVString(
            const QString& equals_separator = DBOBJECT_DEFAULT_SEPARATOR,
            const QString& comma_separator = ", ") const;

    // ========================================================================
    // Loading, saving
    // ========================================================================

    // Loads the record from the database by PK; returns "found?"
    // - Also calls loadAllAncillary(), if that has been overridden, to load
    //   associated ancillary objects.
    // - If not found, calls nullify().
    virtual bool load(int pk);

    // Loads the record from the database by single-column WHERE clause;
    // see load() above.
    virtual bool load(const QString& fieldname, const QVariant& where_value);

    // Loads the record from the database by WhereConditions;
    // see load() above.
    virtual bool load(const WhereConditions& where);

    // Creates the SQL/arguments to fetch records from this table.
    virtual SqlArgs fetchQuerySql(const WhereConditions& where = WhereConditions(),
                                  const OrderBy& order_by = OrderBy());

    // Sets our internal field values from a particular row of the results of
    // an SQL query.
    // If order_matches_fetchquery is true, assumes the result's fields are
    // in the same order as ours -- which will be true if the query came from
    // our fetchQuerySql().
    virtual void setFromQuery(const QueryResult& query_result,
                              int row,
                              bool order_matches_fetchquery = false);

    // Saves the record to the database, if required. Returns: success?
    virtual bool save();

    // Saves a record without storing its PK back (if it was a new record.)
    // NOT FOR ROUTINE USE. Special shortcut for background INSERT.
    void saveWithoutKeepingPk();

    // Set all fields to null values.
    void nullify();

    // Is the PK field currently NULL?
    bool isPkNull() const;

    // Mark the record as "last modified now".
    void touch(bool only_if_unset = false);

    // Mark all fields as dirty.
    void setAllDirty();

    // Does the record exist in the database?
    bool existsInDb() const;

    // ========================================================================
    // Batch operations
    // ========================================================================

    // Returns all PKs for this table.
    QVector<int> getAllPKs() const;

    // ========================================================================
    // Deletingc
    // ========================================================================

    // Deletes this record from the database.
    // - Also deletes any associated BLOBs.
    // - Also deletes any associated ancillary objects.
    virtual void deleteFromDatabase();

    // ========================================================================
    // Debugging:
    // ========================================================================

    // Ensure we have a field with the specified name, or stop the whole app.
    void requireField(const QString& fieldname) const;

    // Returns a string including tablename/PK name/PK value.
    QString debugDescription() const;

    // ========================================================================
    // Special field access
    // ========================================================================

    // Has this record been marked as for moving off the client, to the server?
    bool shouldMoveOffTablet() const;

    // Set the "move off the client, to the server" field.
    void setMoveOffTablet(bool move_off);

    // Toggles the "move off tablet" field.
    void toggleMoveOffTablet();

    // ========================================================================
    // DDL
    // ========================================================================

    // Returns SQL to create the table.
    QString sqlCreateTable() const;

    // Returns the table name.
    QString tablename() const;

    // Returns the PK field name.
    QString pkname() const;

    // Creates the table in the associated database.
    // - Also creates ancillary tables.
    void makeTable();

    // Returns the associated database.
    // (For making BLOBs share the same database.)
    DatabaseManager& database() const;

    // ========================================================================
    // Signals
    // ========================================================================
signals:
    // "Some data has changed."
    void dataChanged();

    // ========================================================================
    // Internals: ancillary management
    // ========================================================================
protected:

    // Load all ancillary objects from the database.
    // Calls loadAllAncillary(pk).
    void loadAllAncillary();

    // Override this to implement the loading of ancillary objects.
    virtual void loadAllAncillary(int pk);

    // Returns all ancillary objects. Override this if you use some.
    virtual QVector<DatabaseObjectPtr> getAllAncillary() const;

    // Returns all ancillary SPECIMEN objects. Override this if you use some.
    virtual QVector<DatabaseObjectPtr> getAncillarySpecimens() const;

    // ========================================================================
    // Internals: saving, etc.
    // ========================================================================
protected:
    // Performs an "INSERT OR REPLACE INTO" to save this object.
    bool saveInsert(bool read_pk_from_database = true);

    // Performs an "UPDATE" to save this object.
    bool saveUpdate();

    // Clear the "dirty" flag for all fields.
    void clearAllDirty();

    // Are any fields dirty?
    bool anyDirty() const;

    // Returns all field names, in order of m_record.
    // [SHOULD MATCH fieldsOrdered(); is this pointless?]
    QStringList fieldnamesMapOrder() const;

    // Returns all fieldnames, in order of their addition.
    QVector<Field> fieldsOrdered() const;

protected:
    // Our application object.
    CamcopsApp& m_app;

    // Our database manager.
    DatabaseManager& m_db;

    // The table name.
    QString m_tablename;  // also used as key for extra strings

    // Name of the PK field.
    QString m_pk_fieldname;

    // Do we have the modification timestamp field?
    bool m_has_modification_timestamp;

    // Do we have the "move off tablet" field?
    bool m_has_move_off_tablet_field;

    // Should writing new data to this object make us tell the app that it
    // needs to upload?
    bool m_triggers_need_upload;

    using MapType = QMap<QString, Field>;
    using MapIteratorType = QMapIterator<QString, Field>;
    using MutableMapIteratorType = QMutableMapIterator<QString, Field>;

    // Map fieldname to Field object.
    MapType m_record;  // Note: no intrinsic ordering to fields from this. So:

    // List of fieldnames (same order as m_record!).
    QStringList m_ordered_fieldnames;

    // Record of FieldRefPtr objects already offered via our fieldRef()
    // function. We maintain this record so that if we get asked for a
    // FieldRefPtr to the same field twice, we return the same FieldRefPtr.
    QMap<QString, FieldRefPtr> m_fieldrefs;

    // Does our record exist in the database?
    bool m_exists_in_db;

public:
    friend QDebug operator<<(QDebug debug, const DatabaseObject& d);
};
