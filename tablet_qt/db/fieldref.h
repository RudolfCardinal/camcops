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
#include <functional>
#include <QDate>
#include <QDateTime>
#include <QImage>
#include <QObject>
#include <QPixmap>
#include <QSharedPointer>
#include "field.h"
#include "databaseobject.h"

class Blob;
class CamcopsApp;

// Represents a reference to a Field object, or to something similar.
//
// The FieldRef (usually via a FieldRefPtr) is the main way that Questionnaire
// objects interact with Field objects within a DatabaseObject.
//
// Whereas a Field represents data and associated fieldname (etc.), a FieldRef
// adds signals, deals with some complex field types (e.g. BLOBs) behind the
// scenes, and so on.
//
// FieldRef objects can also provide an interface to non-Field things, like
// simple C++ functions, or the CamcopsApp's StoredVar objects. This means that
// by using the FieldRef as the common currency for editors like Questionnaire,
// those editors can edit a variety of things in a common way.

class FieldRef : public QObject
{
    /*

If a FieldRef didn't do signals, one would think:
- Copy these things by value. They're small.
- Don't use references; the owning function is likely to have finished
  and made the reference become invalid.
- Don't bother with pointers; they have pointers within them anyway.
- The only prerequisite is that the things they point to outlast the
  lifetime of this object.

However, it'd be very helpful if they could do signals.
- In which case, they should be a QObject.
- In which case, you can't copy them.
- In which case, they should be managed by pointer.
- In which case, they should be managed by a QSharedPointer.

The FieldRef manages various kinds of indirection; see FieldRefMethod below.

    */

    Q_OBJECT
public:

    // ========================================================================
    // Helper classes
    // ========================================================================

    // How is the FieldRef going to operate?
    enum class FieldRefMethod {
        Invalid,
            // Dummy value indicating "not configured".
        Field,
            // Direct connection to a Field object.
        DatabaseObject,
            // Connection to a Field object belonging to a DatabaseObject.
        DatabaseObjectBlobField,
            // Connection to (a) a field in the DatabaseObject that stores the
            // PK of a BLOB record, and (b) a record in the BLOB table that
            // stores the actual blob, and references back to the
            // table/PK/field of the DatabaseObject in question.
        IsolatedBlobFieldForTesting,
            // As the name suggests.
        Functions,
            // Getter/setter functions, to allow the use e.g. of Questionnaires
            // (which use FieldRefs) together with arbitrary C++ objects, e.g.
            // for setting StoredVar objects.
        StoredVar,
            // Connection to a named StoredVar of the master app object.
        CachedStoredVar,
            // Connection to a named CachedStoredVar of the master app object.
    };

    // ========================================================================
    // Shorthand
    // ========================================================================
    // A function that looks like "QVariant getSomething()":
    using GetterFunction = std::function<QVariant()>;

    // A function that looks like "bool setSomething(const QVariant& value)";
    // its return value is "changed?"
    using SetterFunction = std::function<bool(const QVariant&)>;

protected:
    // Protected constructor
    FieldRef(FieldRefMethod method,
             const bool mandatory,
             Field* p_field,
             DatabaseObject* p_dbobject,
             const QString& fieldname,
             const bool autosave,
             QSharedPointer<Blob> blob,
             const GetterFunction& getterfunc,
             const SetterFunction& setterfunc,
             CamcopsApp* p_app,
             const QString& storedvar_name);

public:
    // ========================================================================
    // Constructors
    // ========================================================================

    // Default constructor
    FieldRef();

    // Construct from a Field pointer.
    // Args:
    //  mandatory: do we require data to be present in the underlying field?
    FieldRef(Field* p_field, bool mandatory);

    // Construct from a named field within a DatabaseObject.
    // Args:
    //  mandatory: do we require data to be present in the underlying field?
    //  autosave: should the database object write to disk ASAP?
    //  blob: is this a BLOB field?
    FieldRef(DatabaseObject* p_dbobject, const QString& fieldname,
             bool mandatory, bool autosave = true, bool blob = false,
             CamcopsApp* p_app = nullptr);

    // Construct from a Blob pointer.
    // Args:
    //  mandatory: do we require data to be present in the underlying field?
    FieldRef(QSharedPointer<Blob> blob, bool mandatory,
             bool disable_creation_warning = false);  // for widget testing only; specimen BLOB

    // Construct from a pair of functions to get/set data.
    // Args:
    //  mandatory: do we require data to be present in the underlying field?
    FieldRef(const GetterFunction& getterfunc,
             const SetterFunction& setterfunc,
             bool mandatory);

    // Construct from a named StoredVar within a CamcopsApp.
    // Args:
    //  mandatory: do we require data to be present in the underlying field?
    //  cached: operate on the editing cache copy?
    FieldRef(CamcopsApp* app, const QString& storedvar_name,
             bool mandatory, bool cached);  // StoredVar

    // ========================================================================
    // Validity check
    // ========================================================================

    // Do we have the necessary data for our chosen method?
    bool valid() const;

    // ========================================================================
    // Setting the value
    // ========================================================================

    // Set the underlying data value.
    // ... originator is optional and used as a performance hint (see QSlider)
    bool setValue(const QVariant& value, const QObject* originator = nullptr);

    // Trigger a valueChanged() signal.
    // (For rare manual use.)
    void emitValueChanged(const QObject* originator = nullptr);

    // ========================================================================
    // Retrieving the value
    // ========================================================================

    // Returns the underlying data value.
    QVariant value() const;

    // Is the value NULL?
    bool isNull() const;

    // Returns the underlying data value, as a bool.
    bool valueBool() const;

    // Returns the underlying data value, as an int.
    int valueInt() const;

    // Returns the underlying data value, as a qint64 (qlonglong).
    qint64 valueInt64() const;

    // Returns the underlying data value, as a double.
    double valueDouble() const;

    // Returns the underlying data value, as a QDateTime.
    QDateTime valueDateTime() const;

    // Returns the underlying data value, as a QDate.
    QDate valueDate() const;

    // Returns the underlying data value, as a QTime.
    QTime valueTime() const;

    // Returns the underlying data value, as a string.
    QString valueString() const;

    // Returns the underlying data value, as a string list.
    QStringList valueStringList() const;

    // Returns the underlying data value, as bytes.
    QByteArray valueByteArray() const;

    // Returns the underlying data value, as a vector of int.
    QVector<int> valueVectorInt() const;

    // ========================================================================
    // BLOB-related functions, overridden by BlobFieldRef for higher performance
    // ========================================================================

    // Is this a BLOB field?
    bool isBlob() const;

    // Returns the BLOB as a QImage.
    virtual QImage image(bool* p_loaded = nullptr) const;

    // Returns the BLOB as a QPixmap.
    virtual QPixmap pixmap(bool* p_loaded = nullptr) const;

    // Rotates the BLOB.
    // (Low-performance version; overridden by BlobFieldRef.)
    virtual void rotateImage(int angle_degrees_clockwise,
                             const QObject* originator = nullptr);

    // Sets the BLOB image.
    // (Low-performance version; overridden by BlobFieldRef.)
    virtual bool setImage(const QImage& image,
                          const QObject* originator = nullptr);

    // Sets the BLOB image.
    // (Low-performance version; overridden by BlobFieldRef.)
    virtual bool setRawImage(const QByteArray& data,
                             const QString& extension_without_dot,
                             const QString& mimetype,
                             const QObject* originator = nullptr);

    // ========================================================================
    // Completeness of input
    // ========================================================================

    // Is data mandatory?
    bool mandatory() const;

    // Sets the mandatory status.
    // ... originator is optional and used as a performance hint (see QSlider)
    void setMandatory(bool mandatory, const QObject* originator = nullptr);

    // Is the field complete (not NULL or empty)?
    bool complete() const;

    // Is there missing input, i.e. (mandatory() && !complete())?
    bool missingInput() const;

    // ========================================================================
    // Hints
    // ========================================================================

    // Sets a hint that can be used to distinguish different FieldRef objects.
    // (Example: see cape42.cpp.)
    void setHint(const QVariant& hint);

    // Returns the hint.
    QVariant getHint() const;

    // ========================================================================
    // Debugging
    // ========================================================================

    // Returns a description of the method (e.g. field, getter/setter, etc.).
    QString getFieldRefMethodDescription() const;

    // Returns a description of the target (e.g. a field's name).
    QString getTargetDescription() const;

    // Debugging description.
    friend QDebug operator<<(QDebug debug, const FieldRef& f);

protected:
    // Signal that the value has changed; perhaps trigger an autosave.
    bool signalSetValue(bool changed, const QObject* originator);

    // For FieldRefMethod::DatabaseObjectBlobField only.
    // Sets the database object's field value (FK) to the PK of the associated
    // BLOB object.
    void setFkToBlob();

signals:
    // "The underlying value has changed."
    void valueChanged(const FieldRef* fieldref,
                      const QObject* originator) const;

    // "The mandatory status has changed."
    void mandatoryChanged(const FieldRef* fieldref,
                          const QObject* originator) const;
    // You should NOT cause a valueChanged() signal to be emitted whilst in a
    // mandatoryChanged() signal, but it's fine to emit mandatoryChanged()
    // signals (typically on other fields) whilst processing valueChanged()
    // signals.

protected:

    // The data access method we're using.
    FieldRefMethod m_method;

    // Is data mandatory?
    bool m_mandatory;

    // Info for FieldRefMethod::Field
    Field* m_p_field;

    // Info for FieldRefMethod::DatabaseObject
    DatabaseObject* m_p_dbobject;
    QString m_fieldname;
    bool m_autosave;

    // Extra info for FieldRefMethod::DatabaseObjectBlobField
    QSharedPointer<Blob> m_blob;

    // Info for FieldRefMethod::Functions
    GetterFunction m_getterfunc;
    SetterFunction m_setterfunc;

    // Info for FieldRefMethod::StoredVar, ::CachedStoredVar
    CamcopsApp* m_app;
    QString m_storedvar_name;

    // Our hint
    QVariant m_hint;
};
