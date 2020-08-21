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

#include "field.h"
#include "common/preprocessor_aid.h"
#include "lib/convert.h"
#include "lib/datetime.h"
#include "lib/uifunc.h"
#include "lib/version.h"

const QString SQLITE_TYPE_BLOB("BLOB");
const QString SQLITE_TYPE_INTEGER("INTEGER");
const QString SQLITE_TYPE_REAL("REAL");
const QString SQLITE_TYPE_TEXT("TEXT");


Field::Field(const QString& name,
             const QVariant::Type type,
             const bool mandatory,
             const bool unique,
             const bool pk,
             const QVariant& default_value) :
    m_name(name),
    m_type(type),
    m_pk(pk),
    m_unique(unique || pk),
    m_mandatory(mandatory || pk),
    m_set(false),
    m_dirty(true)
{
    if (default_value.isNull()) {
        m_default_value = QVariant(type);  // NULL
    } else {
        m_default_value = default_value;
    }
    m_value = m_default_value;
}


Field::Field(const QString& name,
             const QString& type_name,
             const bool mandatory,
             const bool unique,
             const bool pk,
             const QVariant& default_value) :
    m_name(name),
    m_type(QVariant::UserType),
    m_type_name(type_name),
    m_pk(pk),
    m_unique(unique || pk),
    m_mandatory(mandatory || pk),
    m_set(false),
    m_dirty(true)
{
    m_default_value = default_value;
    m_value = m_default_value;
}


Field::Field() :  // needed by QMap
    Field("", QVariant::Int)  // delegating constructor (C++11)
{
}


Field& Field::setPk(const bool pk)
{
    m_pk = pk;
    return *this;
}


Field& Field::setUnique(const bool unique)
{
    m_unique = unique;
    return *this;
}


Field& Field::setMandatory(const bool mandatory)
{
    m_mandatory = mandatory;
    return *this;
}


Field& Field::setDefaultValue(const QVariant& value)
{
    m_default_value = value;
    m_default_value.convert(m_type);
    if (!m_set) {
        m_value = m_default_value;
    }
    return *this;
}


QString Field::name() const
{
    return m_name;
}


QVariant::Type Field::type() const
{
    return m_type;
}


bool Field::isPk() const
{
    return m_pk;
}


bool Field::isUnique() const
{
    return m_unique;
}


bool Field::isMandatory() const
{
    return m_mandatory;
}


bool Field::notNull() const
{
    // SQLite allows NULL values in primary keys, but this is a legacy of
    // bugs in early SQLite versions.
    // http://www.sqlite.org/lang_createtable.html
    return m_mandatory || m_pk;
}


QString Field::sqlColumnDef() const
{
    QString type = sqlColumnType();
    if (m_pk) {
        type += " PRIMARY KEY";
    }
    // AUTOINCREMENT usually not required: https://www.sqlite.org/autoinc.html
    if (m_unique && !m_pk) {
        type += " UNIQUE";
    }
    if (notNull()) {
        type += " NOT NULL";
    }
    return type;
}


QVariant Field::value() const
{
    return m_value;
}


QString Field::prettyValue(const int dp) const
{
    return convert::prettyValue(m_value, dp, m_type);
}


bool Field::setValue(const QVariant& value)
{
    if (!m_set || value != m_value) {
        m_dirty = true;
    }
    m_value = value;
    if (!m_value.isNull() && m_type != QVariant::UserType) {
        // Don't try to convert NULL values; needless warning.
        // Don't try to convert user type; it'll go wrong.
        const bool converted = m_value.convert(m_type);
        if (!converted) {
            if (m_type == QVariant::Char) {
                // Deal with special oddities, e.g. failure to convert
                // a QVariant of type QString to one of type QChar.
                m_value = convert::toQCharVariant(value);
            } else {
                qWarning() << Q_FUNC_INFO << "Failed to convert" << value
                           << "to type" << m_type;
            }
        }
    }
    m_set = true;
    return m_dirty;
}


bool Field::nullify()
{
    if (!m_set || !isNull()) {
        m_dirty = true;
    }
    m_value = QVariant(m_type);
    m_set = true;
    return m_dirty;
}


bool Field::isNull() const
{
    return m_value.isNull();
}


bool Field::isDirty() const
{
    return m_dirty;
}


void Field::setDirty()
{
    m_dirty = true;
}


void Field::clearDirty()
{
    m_dirty = false;
}


QDebug operator<<(QDebug debug, const Field& f)
{
    if (f.m_value.isNull()) {
        debug.nospace() << "NULL (" << QVariant::typeToName(f.m_type) << ")";
    } else {
        debug.nospace() << f.m_value;
    }
    if (f.m_dirty) {
        debug.nospace() << " (*)";
    }
    return debug;
}


QString Field::sqlColumnType() const
{
    // SQLite types: https://www.sqlite.org/datatype3.html
    //      SQLite uses up to 8 bytes (depending on actual value) and
    //      integers are signed, so the maximum INTEGER
    //      is 2^63 - 1 = 9,223,372,036,854,775,807
    // C++ types:
    //      int -- typically 32-bit; not guaranteed on all C++ platforms,
    //             though 32-bit on all Qt platforms, I think
    // Qt types: http://doc.qt.io/qt-5/qtglobal.html
    //      - qint8, qint16, qint32, qint64...
    //      - standard int is int32
    //        32-bit signed: up to +2,147,483,647 = 2147483647
    //      - qlonglong is the same as qint64
    //        64-bit signed: up to +9,223,372,036,854,775,807 = 9223372036854775807
    //      - qulonglong is the same as quint64
    //        64-bit unsigned: 0 to +18,446,744,073,709,551,615 = 18446744073709551615
    // C++ type name: QVariant::typeToName(m_type);
    switch (m_type) {

    case QVariant::Bool:
    case QVariant::Int:  // normally 32-bit
    case QVariant::LongLong:  // 64-bit
    case QVariant::UInt:  // normally 32-bit
    case QVariant::ULongLong:  // 64-bit
        return SQLITE_TYPE_INTEGER;

    case QVariant::Double:
        return SQLITE_TYPE_REAL;

    case QVariant::Char:
    case QVariant::Date:
    case QVariant::DateTime:
    case QVariant::String:
    case QVariant::StringList:
    case QVariant::Time:
    case QVariant::Uuid:
        return SQLITE_TYPE_TEXT;

    case QVariant::ByteArray:
        return SQLITE_TYPE_BLOB;

    case QVariant::UserType:
        if (m_type_name == convert::TYPENAME_QVECTOR_INT) {
            return SQLITE_TYPE_TEXT;
        }
        if (m_type_name == convert::TYPENAME_VERSION) {
            return SQLITE_TYPE_TEXT;
        }
        break;
    default:
        break;
    }
    uifunc::stopApp(
        QString("Field::sqlColumnType: Unknown field type: %1").arg(m_type));
#ifdef COMPILER_WANTS_RETURN_AFTER_NORETURN
    return "";
#endif
}


void Field::setFromDatabaseValue(const QVariant& db_value)
{
    // SQLite -> C++
    switch (m_type) {
    case QVariant::Char:
        // If you just do "m_value = db_value", it will become an invalid
        // value when the convert() call is made below, so will appear as NULL.
        m_value = convert::toQCharVariant(db_value);
        break;
    case QVariant::Date:
        m_value = QVariant(datetime::isoToDate(db_value.toString()));
        break;
    case QVariant::DateTime:
        m_value = QVariant(datetime::isoToDateTime(db_value.toString()));
        break;
    case QVariant::StringList:
        m_value = QVariant(convert::csvStringToQStringList(db_value.toString()));
        break;
    case QVariant::UserType:
        if (m_type_name == convert::TYPENAME_QVECTOR_INT) {
            m_value.setValue(convert::csvStringToIntVector(
                                 db_value.toString()));
        } else if (m_type_name == convert::TYPENAME_VERSION) {
            m_value.setValue(Version::fromString(db_value.toString()));
        } else {
            m_value = db_value;
        }
        break;
    default:
        m_value = db_value;
        break;
    }
    if (m_type != QVariant::UserType) {
        m_value.convert(m_type);
    }
    m_dirty = false;
}


QVariant Field::databaseValue() const
{
    // C++ -> SQLite
    if (m_value.isNull()) {
        return m_value;  // NULL
    }
    switch (m_type) {
    case QVariant::Char:
        return m_value.toString();
    case QVariant::Date:
        return QVariant(datetime::dateToIso(m_value.toDate()));
    case QVariant::DateTime:
        return QVariant(datetime::datetimeToIsoMs(m_value.toDateTime()));
    case QVariant::StringList:
        return convert::qStringListToCsvString(m_value.toStringList());
    case QVariant::UserType:
        if (m_type_name == convert::TYPENAME_QVECTOR_INT) {
            return convert::numericVectorToCsvString(
                        convert::qVariantToIntVector(m_value));
        }
        if (m_type_name == convert::TYPENAME_VERSION) {
            return Version::fromVariant(m_value).toString();
        }
        break;
    case QVariant::Uuid:
        return m_value.toString();
        // see http://doc.qt.io/qt-5/quuid.html#toString; e.g.
        // "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}" where 'x' is a hex digit
    default:
        break;
    }
    return m_value;
}
