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
#include <QCoreApplication>  // for Q_DECLARE_TR_FUNCTIONS
#include <QDateTime>
#include <QString>
#include "common/aliases_camcops.h"
#include "db/databaseobject.h"

class CamcopsApp;
class OpenableWidget;
class QGraphicsScene;
class Version;

extern const QString PATIENT_FK_FIELDNAME;


class Task : public DatabaseObject
{
    Q_OBJECT
    friend class SingleTaskMenu;  // so it can call setupForEditingAndSave()
    friend class Patient;  // so it can call moveToPatient()
    friend class TaskChain;  // so it can call setupForEditingAndSave()

public:
    enum class TaskImplementationType {
        Full,
        UpgradableSkeleton,
        Skeleton,
    };

public:
    // Constructor
    // Args:
    //      app: the CamCOPS app
    //      db: the database that will hold this task
    //      tablename: the base table name
    //      is_anonymous: is this is an anonymous task (with no patient)?
    //      has_clinician: add standard fields for a clinician?
    //      has_respondent: add standard fields for a respondent (e.g. carer)?
    Task(CamcopsApp& app,
         DatabaseManager& db,
         const QString& tablename,
         bool is_anonymous,
         bool has_clinician,
         bool has_respondent);

    // Destructor
    virtual ~Task() {}

    // ------------------------------------------------------------------------
    // General info
    // ------------------------------------------------------------------------
    // Things that should ideally be class methods but we'll do by instance:

    // tablename(): already implemented by DatabaseObject

    // Short name of the task (e.g. "PHQ-9")
    virtual QString shortname() const = 0;

    // Long name of the task (e.g. "Patient Health Questionnaire-9")
    virtual QString longname() const = 0;

    // How is the task implemented -- does it come with all its content, or
    // is it a bare-bones skeleton (for tasks whose content we can't
    // reproduce), or is it an upgradeable skeleton (depending on institutional
    // permissions)?
    virtual TaskImplementationType implementationType() const { return TaskImplementationType::Full; }

    QString implementationTypeDescription() const;

    // Suffix for menu title (e.g. symbols for restricted/defunct tasks).
    QString menuTitleSuffix() const;

    // Title to be used on the menu. By default this is of the format
    // "longname (shortname)".
    virtual QString menutitle() const;

    // Description to be used on the menu.
    virtual QString description() const = 0;

    // Suffix for menu subtitle.
    QString menuSubtitleSuffix() const;

    // Menu subtitle with any necessary information suffix.
    QString menusubtitle() const;

    // Filename stem (e.g. "phq9") that will be used to form a URL to the
    // online documentation for this task. By default, it's tablename().
    virtual QString infoFilenameStem() const;

    // Task name to use when looking up an xstring() for this task. By default,
    // it's tablename().
    virtual QString xstringTaskname() const;

    // Returns a title for an instance of this task. If the task is anonymous
    // or with_pid is false, the default implementation includes the shortname
    // and the task's creation date. If patient information is available and
    // with_pid is true, it also includes some brief patient details.
    virtual QString instanceTitle(bool with_pid = true) const;

    // Is the task anonymous (no patient)?
    virtual bool isAnonymous() const;

    // Does the task have a clinician?
    virtual bool hasClinician() const;

    // Does the task have a respondent (e.g. a carer answering on behalf of or
    // in relation to the patient)?
    virtual bool hasRespondent() const;

    // Does this task prohibit clinical use?
    virtual bool prohibitsClinical() const { return false; }

    // Does this task prohibit commerical use?
    virtual bool prohibitsCommercial() const { return false; }

    // Does this task prohibit research use?
    virtual bool prohibitsEducational() const { return false; }

    // Does this task prohibit research use?
    virtual bool prohibitsResearch() const { return false; }

    // Is the task re-editable once it's been created?
    virtual bool isEditable() const { return true; }

    // Is the task less than fully functional, e.g. requiring strings that have
    // not been downloaded (or are not available) from a CamCOPS server?
    virtual bool isCrippled() const;

    // Is this an experimental task? (Affects labelling.)
    virtual bool isExperimental() const { return false; }

    // Is this a defunct task? (Affects labelling.)
    virtual bool isDefunct() const { return false; }

    // Are there any extra strings (xstrings) for the task, downloaded from the
    // server?
    virtual bool hasExtraStrings() const;

    // Is it permissible to create a new instance of the task? (If not, why
    // not?)
    virtual bool isTaskPermissible(QString& why_not_permissible) const;

    // What is the minimum CamCOPS server version that will accept this task?
    virtual Version minimumServerVersion() const;

    // Is this task uploadable? Reasons that it may not be include:
    // - the server doesn't have the task's table
    // - the client says the server is too old (in general, or for this task)
    // - the server says the client is too old
    virtual bool isTaskUploadable(QString& why_not_uploadable) const;

    // ------------------------------------------------------------------------
    // Tables and other classmethods
    // ------------------------------------------------------------------------

    // Return a list of names of ancillary tables used by this task. (For
    // example, the PhotoSequence task has an ancillary table to contain its
    // photos. One sequence, lots of photos.)
    virtual QStringList ancillaryTables() const { return QStringList(); }

    // Each ancillary table (if there are any) has a foreign key (FK) to the
    // base table. What's the FK column name?
    virtual QString ancillaryTableFKToTaskFieldname() const { return ""; }

    // Return all tables used by this task (base + ancillary).
    QStringList allTables() const;

    // Make all tables (base table and any ancillary tables).
    virtual void makeTables();

    // Make all ancillary tables.
    virtual void makeAncillaryTables() {}

    // How many instances of this task type (optionally meeting a set of WHERE
    // criteria) exist in the database?
    int count(const WhereConditions& where = WhereConditions()) const;

    // How many instances of this task type exist in the database for the
    // specified patient (by the CamCOPS client's patient PK)?
    int countForPatient(int patient_id) const;

    // Perform any special steps required by this task as we upgrade the client
    // database.
    virtual void upgradeDatabase(const Version& old_version,
                                 const Version& new_version);

    // ------------------------------------------------------------------------
    // Database object functions
    // ------------------------------------------------------------------------

    // Load data from the database into the fields for this instance.
    // No need to override, but do need to CALL load() FROM CONSTRUCTOR:
    virtual bool load(int pk = dbconst::NONEXISTENT_PK);

    // Save data from this task instance to the database, if any data needs
    // saving.
    // - Performs some sanity checks, then calls DatabaseObject::save().
    virtual bool save();

    // ------------------------------------------------------------------------
    // Specific info
    // ------------------------------------------------------------------------

    // Is the task complete?
    virtual bool isComplete() const = 0;

    // Is the task complete? Cached version (automatically reloaded when task
    // data changes).
    bool isCompleteCached() const;

    // Returns summary information about the task. (Shown in the task menus
    // and in the summary view.)
    virtual QStringList summary() const;

    // Returns more detailed information about the task.
    virtual QStringList detail() const;

    // Returns an editor widget (e.g. a questionnaire or a graphics widget) for
    // editing this task (or viewing it, if read_only is true).
    virtual OpenableWidget* editor(bool read_only = false);

protected:
    void onDataChanged();

    // ------------------------------------------------------------------------
    // Assistance functions
    // ------------------------------------------------------------------------
public:
    // When was this task created?
    QDateTime whenCreated() const;

    // If the task is incomplete, returns string(s) to indicate this
    // (otherwise, returns an empty list).
    QStringList completenessInfo() const;

    // Returns an xstring for this task. This is a named string, downloaded for
    // this task from the server.
    QString xstring(const QString& stringname,
                    const QString& default_str = "") const;

    // Returns an appstring. This is a named string, downloaded from the server
    // for the CamCOPS client in general.
    QString appstring(const QString& stringname,
                      const QString& default_str = "") const;

    // Assistance function for summary() or detail().
    // - Returns a list of strings of the format
    //   "<name><spacer><b>value</b><suffix>" for specified fields.
    // - The field name (from which <value> is taken) ranges from
    //   <fieldprefix><first> to <fieldprefix><last>.
    // - The name ranges from <xstringprefix><first><xstringsuffix> to
    //   <xstringprefix><last><xstringsuffix>.
    QStringList fieldSummaries(const QString& xstringprefix,
                               const QString& xstringsuffix,
                               const QString& spacer,
                               const QString& fieldprefix,
                               int first,
                               int last,
                               const QString& suffix = "") const;

    // As for fieldSummaries(), but the value is shown as "Yes"/"No", for
    // Boolean fields.
    QStringList fieldSummariesYesNo(const QString& xstringprefix,
                                    const QString& xstringsuffix,
                                    const QString& spacer,
                                    const QString& fieldprefix,
                                    int first,
                                    int last,
                                    const QString& suffix = "") const;

    // Returns a string list of the clinician's details (specialty, name,
    // etc.).
    QStringList clinicianDetails(const QString& separator = ": ") const;

    // Returns a string list of the respondent's details (name, relationship).
    QStringList respondentDetails() const;

    // ------------------------------------------------------------------------
    // Editing
    // ------------------------------------------------------------------------

    // How long has the user spent editing this task?
    double editingTimeSeconds() const;

protected:

    // Set up all defaults (including setting the patient ID, for non-anonymous
    // tasks) and save to database. Use when you've created a task and want
    // to edit it.
    void setupForEditingAndSave(const int patient_id = dbconst::NONEXISTENT_PK);

    // Set the clinician fields to the app's default clinician information.
    // Called when the task is first created from the menus.
    // Only relevant for tasks with a clinician.
    void setDefaultClinicianVariablesAtFirstUse();

    // Override if you need to do additional configuration for a new task.
    // Called when the task is first created from the menus.
    virtual void setDefaultsAtFirstUse() {}

    // Helper function for graphical/animated tasks to create their editor.
    // Makes an OpenableWidget containing a ScreenLikeGraphicsView to display
    // the specified QGraphicsScene.
    // - background_colour: the background colour of the ScreenLikeGraphicsView
    // - fullscreen: open this window in fullscreen mode?
    // - esc_can_abort: passed to OpenableWidget::setEscapeKeyCanAbort().
    OpenableWidget* makeGraphicsWidget(
            QGraphicsScene* scene, const QColor& background_colour,
            bool fullscreen = true, bool esc_can_abort = true);

    // Helper function for graphical/animated tasks to create their editor.
    // Calls makeGraphicsWidget() [q.v.], then hooks the widget's abort signal
    // to Task::onEditFinishedAbort(), and starts the editing clock.
    OpenableWidget* makeGraphicsWidgetForImmediateEditing(
            QGraphicsScene* scene, const QColor& background_colour,
            bool fullscreen = true, bool esc_can_abort = true);

    // Returns a questionnaire element representing clinician details
    // (specialty, name, etc.). Only applicable to tasks with a clinician.
    QuElement* getClinicianQuestionnaireBlockRawPointer();
    QuElementPtr getClinicianQuestionnaireBlockElementPtr();

    // Returns a questionnaire page representing clinician details.
    //  Only applicable to tasks with a clinician.
    QuPagePtr getClinicianDetailsPage();

    // Do we have enough information about the clinician (meaning their name)?
    //  Only applicable to tasks with a clinician.
    bool isClinicianComplete() const;

    // Do we have enough information about the respondent (meaning their name
    // and relationship)? Only applicable to tasks with a respondent.
    bool isRespondentComplete() const;

    // Returns the respondent's relationship to the patient (from our standard
    // field.). Only applicable to tasks with a respondent.
    QVariant respondentRelationship() const;

    // Returns a questionnaire element representing respondent details.
    // Only applicable to tasks with a respondent.
    QuElement* getRespondentQuestionnaireBlockRawPointer(bool second_person);
    QuElementPtr getRespondentQuestionnaireBlockElementPtr(bool second_person);

    // Returns a questionnaire page representing respondent details.
    // Only applicable to tasks with a respondent.
    QuPagePtr getRespondentDetailsPage(bool second_person);

    // Returns a questionnaire element representing clinician and respondent
    // details. Only applicable to tasks with a clinician and a respondent.
    QuPagePtr getClinicianAndRespondentDetailsPage(bool second_person);

    // Create a standard set of NameValueOptions from the task's xstrings,
    // in ascending or descending order.
    NameValueOptions makeOptionsFromXstrings(const QString& xstring_prefix,
                                             int first, int last,
                                             const QString& xstring_suffix = "");

public slots:
    // "The user has started to edit this task."
    void onEditStarted();

    // "The user has finished editing this task, successfully or not."
    // Updates the "time spent editing" clock and may set the "first exit was
    // finish/abort" flags.
    void onEditFinished(bool aborted = false);

    // "The user has finished editing this task, successfully."
    // Calls editFinished(false).
    void onEditFinishedProperly();

    // "The user has finished editing this task, unsuccessfully."
    // Calls editFinished(true).
    void onEditFinishedAbort();

signals:
    // Task has been aborted (and all its internal cleanup is done).
    void editingAborted();

    // Task has finished cleanly (and all its internal cleanup is done).
    void editingFinished();

    // ------------------------------------------------------------------------
    // Patient functions (for non-anonymous tasks)
    // ------------------------------------------------------------------------
public:

    // Returns the task's patient, or nullptr.
    Patient* patient() const;

    // Returns the patient's name (e.g. "Bob Jones"), or "".
    QString getPatientName() const;

    // Is the patient present and female?
    bool isFemale() const;

    // Is the patient present and male?
    bool isMale() const;

protected:
    // Sets the task's patient. (Used when tasks are being added.)
    void setPatient(int patient_id);

    // Moves this task to another patient. (Used for patient merges.)
    void moveToPatient(int patient_id);

    // ------------------------------------------------------------------------
    // Instance data
    // ------------------------------------------------------------------------
protected:
    mutable QSharedPointer<Patient> m_patient;  // our patient
    bool m_editing;  // are we editing?
    QDateTime m_editing_started;  // when did the current edit start?
    mutable bool m_is_complete_is_cached;
    mutable bool m_is_complete_cached_value;

    // ------------------------------------------------------------------------
    // Class data
    // ------------------------------------------------------------------------
protected:
    bool m_is_anonymous;  // is the task anonymous?
    bool m_has_clinician;  // does the task have a clinician?
    bool m_has_respondent;  // does the task have a respondent?

    // ------------------------------------------------------------------------
    // Translatable text
    // ------------------------------------------------------------------------
public:
    // String for "task is incomplete", for summary views.
    static QString incompleteMarker();

    // ------------------------------------------------------------------------
    // Static data
    // ------------------------------------------------------------------------
public:
    // Standard fieldnames
    static const QString PATIENT_FK_FIELDNAME;
    static const QString FIRSTEXIT_IS_FINISH_FIELDNAME;
    static const QString FIRSTEXIT_IS_ABORT_FIELDNAME;
    static const QString WHEN_FIRSTEXIT_FIELDNAME;
    static const QString EDITING_TIME_S_FIELDNAME;

    static const QString CLINICIAN_SPECIALTY;
    static const QString CLINICIAN_NAME;
    static const QString CLINICIAN_PROFESSIONAL_REGISTRATION;
    static const QString CLINICIAN_POST;
    static const QString CLINICIAN_SERVICE;
    static const QString CLINICIAN_CONTACT_DETAILS;

    static const QString RESPONDENT_NAME;
    static const QString RESPONDENT_RELATIONSHIP;
};
