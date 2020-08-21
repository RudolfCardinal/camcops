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
#include <QDate>
#include <QJsonObject>
#include <QMap>
#include <QPointer>
class CamcopsApp;
class IdPolicy;
class OpenableWidget;
class Questionnaire;

extern const QString FORENAME_FIELD;
extern const QString SURNAME_FIELD;
extern const QString DOB_FIELD;
extern const QString SEX_FIELD;
extern const QString ADDRESS_FIELD;
extern const QString GP_FIELD;
extern const QString OTHER_DETAILS_POLICYNAME;

extern const QString IDNUM_FIELD_PREFIX;
extern const QString IDNUM_FIELD_FORMAT;
extern const QString ANY_IDNUM_POLICYNAME;
extern const QString OTHER_IDNUM_POLICYNAME;


// Represents a patient.

class Patient : public DatabaseObject
{
    Q_OBJECT
    using AttributesType = QMap<QString, bool>;
public:
    // ------------------------------------------------------------------------
    // Creation
    // ------------------------------------------------------------------------
    Patient(CamcopsApp& app,
            DatabaseManager& db,
            int load_pk = dbconst::NONEXISTENT_PK);

    // ------------------------------------------------------------------------
    // Ancillary management
    // ------------------------------------------------------------------------

    virtual void loadAllAncillary(int pk) override;
    virtual QVector<DatabaseObjectPtr> getAncillarySpecimens() const override;
    virtual QVector<DatabaseObjectPtr> getAllAncillary() const override;

    // ------------------------------------------------------------------------
    // Information about patients
    // ------------------------------------------------------------------------

    // Returns the patient's database PK.
    int id() const;

    // Returns the forename.
    QString forename() const;

    // Returns the surname.
    QString surname() const;

    // Returns the sex; see CommonOptions::SEX_*.
    QString sex() const;

    // Is the patient female?
    bool isFemale() const;

    // Is the patient male?
    bool isMale() const;

    // Returns the date of birth, as a QDate.
    QDate dob() const;

    // Returns the date of birth as text, e.g. "31 Dec 2000".
    QString dobText() const;

    // Returns the conventional age in years (i.e. rounded down).
    int ageYears() const;

    // Do we know the forename?
    bool hasForename() const;

    // Do we know the surname?
    bool hasSurname() const;

    // Do we know the sex?
    bool hasSex() const;

    // Do we know the DOB?
    bool hasDob() const;

    // Do we know the address?
    bool hasAddress() const;

    // Do we know the GP?
    bool hasGP() const;

    // Is there information in the "other" field?
    bool hasOtherDetails() const;

    // Do we have an ID number of the specified type?
    bool hasIdnum(int which_idnum) const;

    // Which ID number types are present and have an ID number?
    QVector<int> whichIdnumsPresent() const;

    // Which ID number types have entries present?
    // (They do not necessarily have data.)
    QVector<int> whichIdnumsHaveEntries() const;

    // Returns an ID number of the specified ID type as a QVariant.
    QVariant idnumVariant(int which_idnum) const;

    // Returns an ID number of the specified ID type as a qint64 (qlonglong).
    // = 64-bit signed integer: up to 2^63 - 1 = 9,223,372,036,854,775,807
    qint64 idnumInteger(int which_idnum) const;

    // Returns the attributes possessed by the patient (e.g. "sex and
    // forename and id number type 3"), to provide information for checking
    // against an ID policy mentioning the specified ID number types.
    AttributesType policyAttributes(
            const QVector<int>& policy_mentioned_idnums) const;

    // Returns a JSON representation of the patient.
    QJsonObject jsonDescription() const;

    // Does the patient comply with the specified ID policy?
    bool compliesWith(const IdPolicy& policy) const;

    // Does the patient comply with the tablet software's standard minimum
    // ID policy?
    bool compliesWithTablet() const;

    // Does the patient comply with the server's upload policy?
    bool compliesWithUpload() const;

    // Does the patient comply with the server's finalize (preserve) policy?
    bool compliesWithFinalize() const;

    // Returns a short ID number summary, like "RiO 12345, NHS 9876543210".
    QString shortIdnumSummary() const;

    // Do other patients in our database clash on the specified ID number type?
    bool othersClashOnIdnum(int which_idnum) const;

    // Do other patients in our database clash on any ID number types?
    bool anyIdClash() const;

    // How many tasks are in the database for this patient?
    int numTasks() const;

    // Delete the patient (and associated tasks) from the database.
    virtual void deleteFromDatabase() override;

    // Does this patient match another, allowing them to be merged?
    bool matchesForMerge(const Patient* other) const;

    // Helper functions for various viewers:

    // e.g. "<b>JONES, Bob</b><br>M, 19y, DOB 1 Jan 2000<br>RiO 12345, NHS 9876543210"
    QString descriptionForMerge() const;

    // e.g. "Bob Jones"
    QString forenameSurname() const;

    // e.g. "JONES, Bob"
    QString surnameUpperForename() const;

    // e.g. "M, 19y, DOB 1 Jan 2000"
    QString sexAgeDob() const;

    // e.g. "19y, M, DOB 1 Jan 2000"
    QString ageSexDob() const;

    // e.g. "JONES, Bob (M, 19y, DOB 1 Jan 2000)\nRiO 12345, NHS 9876543210"
    QString twoLineDetailString() const;

    // e.g. "<b>JONES, Bob</b> (M, 19y, DOB 1 Jan 2000); RiO 12345, NHS 9876543210"
    QString oneLineHtmlDetailString() const;

    // ------------------------------------------------------------------------
    // Editing and other manipulations
    // ------------------------------------------------------------------------

    // Returns an editor to edit this patient.
    OpenableWidget* editor(bool read_only);

    // Merge with another patient. Move tasks from "other" to "this".
    void mergeInDetailsAndTakeTasksFrom(const Patient* other);

public:
    static const QString TABLENAME;

protected:
    // Build the editor page.
    void buildPage(bool read_only);

    // Ask the user for input and add an ID number entry.
    void addIdNum();

    // Delete an ID number of the specified type.
    void deleteIdNum(int which_idnum);

    // Sort ID numbers by their type, and if we are editing the patient,
    // refresh the questionnaire to reflect the current ID numbers.
    void sortIdNums();

    // Updates the "missing" alerts etc.
    void updateQuestionnaireIndicators(const FieldRef* fieldref = nullptr,
                                       const QObject* originator = nullptr);

protected:
    // ID number objects
    QVector<PatientIdNumPtr> m_idnums;

    // Records our editing questionnaire's page.
    QuPagePtr m_page;

    // Records our editing questionnaire.
    QPointer<Questionnaire> m_questionnaire;
};
