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
#include <QApplication>
#include <QPointer>
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QStack>
#include "common/aliases_camcops.h"
#include "common/dpi.h"
#include "common/textconst.h"
#include "common/uiconst.h"  // for FontSize
#include "crypto/secureqstring.h"
#include "db/fieldref.h"  // for FieldRefPtr
#include "dbobjects/patient.h"
#include "questionnairelib/namevalueoptions.h"
#include "tasklib/task.h"  // for TaskPtr
class IdPolicy;
class NetworkManager;
class OpenableWidget;
class QSqlDatabase;
class QMainWindow;
class QStackedWidget;
class QTextStream;
class QTranslator;
class SlowGuiGuard;
class Version;


// The main application object.
class CamcopsApp : public QApplication
{
    Q_OBJECT

    // ------------------------------------------------------------------------
    // Helper classes
    // ------------------------------------------------------------------------

public:

    // Describes the "lock state" of the whole CamCOPS app.
    enum class LockState {
        Unlocked,
        Locked,
        Privileged
    };

    // Stores information about opened windows, and information they are
    // associated with. Used to maintain a window stack, and restore state
    // nicely once the window is closed (e.g. restoring fullscreen state, or
    // ensure patient/task information is updated if the window referred to
    // one).
    struct OpenableInfo {
    public:
        OpenableInfo()
        {}

        //  widget:
        //      The window that is being opened.
        //  task:
        //      If it refers to a task, record that here.
        //  prev_window_state:
        //      The app's overall window state before opening this window.
        //  wants_fullscreen:
        //      Does the window want to be fullscreen?
        //  may_alter_task:
        //      Might it alter a task?
        //  patient:
        //      If it refers to a patient (e.g. a patient editing window),
        //      record that here.
        OpenableInfo(QPointer<OpenableWidget> widget, TaskPtr task,
                     Qt::WindowStates prev_window_state, bool wants_fullscreen,
                     bool may_alter_task, PatientPtr patient) :
            widget(widget),
            task(task),
            prev_window_state(prev_window_state),
            wants_fullscreen(wants_fullscreen),
            may_alter_task(may_alter_task),
            patient(patient)
        {}
    public:
        QPointer<OpenableWidget> widget;
        TaskPtr task;
        Qt::WindowStates prev_window_state;
        bool wants_fullscreen;
        bool may_alter_task;
        PatientPtr patient;
    };

    // ------------------------------------------------------------------------
    // Core
    // ------------------------------------------------------------------------
public:
    // Create the app (with command-line arguments).
    CamcopsApp(int& argc, char* argv[]);

    // Destructor.
    ~CamcopsApp();

    // Run the app.
    int run();

    // Encapsulate startup tasks that we can put in another thread so we can
    // show a "please wait" animation.
    void backgroundStartup();

    // Return the main CamCOPS database, containing task data.
    DatabaseManager& db();

    // Return the system CamCOPS database, containing configuration data.
    DatabaseManager& sysdb();

    // Return a task factory, for making tasks.
    TaskFactory* taskFactory();

    // ------------------------------------------------------------------------
    // Initialization
    // ------------------------------------------------------------------------
public:
    // Returns the full path to a (SQLite/SQLCipher) database that we'll use.
    QString dbFullPath(const QString& filename);

    // Change the language used.
    void setLanguage(const QString& language_code,
                     bool store_to_database = false);

    // Return the current language code
    QString getLanguage() const;

protected:
    // Directory used by CamCOPS to store its SQLite/SQLCipher directory.
    QString defaultDatabaseDir() const;

    // Process the (stored) command-line arguments.
    // Args:
    // - retcode: exit code if not happy to continue
    // Returns:
    // - happy to continue?
    bool processCommandLineArguments(int& retcode);

    // Announce startup information to the console/debugging stream.
    void announceStartup();

    // Register database drivers (e.g. SQLCipher) with Qt.
    void registerDatabaseDrivers();

    // Open our pair of databases, or create them if they don't exist.
    void openOrCreateDatabases();

    // Close our databases.
    void closeDatabases();

    // Give the database system the encryption password (and if they were not
    // encrypted, encrypt them).
    // Returns: was the user password set (changed)?
    bool connectDatabaseEncryption(QString& new_user_password,
                                   bool& user_cancelled_please_quit);

    // Function launched in a worker thread to descrypt databases. (The
    // decryption process can be slow if the database must be migrated from an
    // older version of SQLCipher, so we want the GUI thread to show a wait
    // indicator).
    void workerDecryptDatabases(const QString& passphrase, bool& success);

    // Closes any database encryption, encrypts on-disk databases with a
    // passphrase, then re-opens the databases.
    bool encryptExistingPlaintextDatabases(const QString& passphrase);

    // Seeds the random number generator.
    void seedRng();

    // Creates the stored variable table (in the "system" database)>
    void makeStoredVarTable();

    // Ensure all stored variables exist.
    void createStoredVars();

    // Consider and perform any system-wide database operations prior to
    // table creation.
    Version upgradeDatabaseBeforeTablesMade();

    // Perform any required task-specific upgrade steps.
    void upgradeDatabaseAfterTasksRegistered(const Version& old_version);

    // Create other tables, in both databases -- e.g. blobs, patient,
    // patient_idnum (in the data database); tables for extra strings, other
    // server info (in the system database).
    void makeOtherSystemTables();

    // Registers all tasks with the task factory.
    void registerTasks();

    // Creates all task tables (and any ancillary tables they need).
    void makeTaskTables();

    // GUI initialization 1/2: before storedvars available
    void initGuiOne();

    // GUI initialization 2/2: after storedvars available
    void initGuiTwoStylesheet();

    // Set global DPI constants
    void setDPI();

    // Open the CamCOPS main menu.
    void openMainWindow();

    // ------------------------------------------------------------------------
    // Opening/closing windows
    // ------------------------------------------------------------------------

public:
    // Launches a new window and keeps track of associated information that the
    // new window may refer to or alter.
    void openSubWindow(OpenableWidget* widget, TaskPtr task = TaskPtr(nullptr),
                       bool may_alter_task = false,
                       PatientPtr patient = PatientPtr(nullptr));

    // Creates and returns an object that will show a wait box whilst you do
    // something slow via the main (GUI) thread.
    SlowGuiGuard getSlowGuiGuard(const QString& text = tr("Opening..."),
                                 const QString& title = TextConst::pleaseWait(),
                                 int minimum_duration_ms = 100);

signals:
    // Signals that a task has been altered.
    void taskAlterationFinished(TaskPtr task);

    // "closeSubWindow() has finished."
    void subWindowFinishedClosing();

public slots:
    // "Close the topmost sub-window."
    void closeSubWindow();

    // "Enter fullscreen mode."
    void enterFullscreen();

    // "Leave fullscreen mode."
    void leaveFullscreen();

    // ------------------------------------------------------------------------
    // Security and related
    // ------------------------------------------------------------------------
public:
    // Is the app in privileged mode?
    bool privileged() const;

    // Is the app in locked mode?
    bool locked() const;

    // What is the app's lock state?
    LockState lockstate() const;

    // Unlock the app.
    void unlock();

    // Lock the app.
    void lock();

    // Put the app into privileged mode.
    void grantPrivilege();

    // Is the app storing the user's server password?
    bool storingServerPassword() const;

    // Stores the user's server password. The password passed to this function
    // is in plain text. It's encrypted before it's stored in the database.
    void setEncryptedServerPassword(const QString& password);

    // Retrieves the user's server password, if it was stored.
    SecureQString getPlaintextServerPassword() const;

    // Asks the user for a new app password, and changes it.
    void changeAppPassword();

    // Asks the user for a new privileged-mode password, and changes it.
    void changePrivPassword();

    // Returns the app's unique device ID, as a string.
    QString deviceId() const;

protected:
    // Resets the encryption key used for reversible password encryption
    // (obscuration).
    void resetEncryptionKeyIfRequired();

    // Asks the user for a password and checks it against a stored hash.
    bool checkPassword(const QString& hashed_password_varname,
                       const QString& text, const QString& title);

    // Sets a stored (hashed) password. The "password" argument is plaintext.
    void setHashedPassword(const QString& hashed_password_varname,
                           const QString& password);

    // Changes a password by asking the user for old/new passwords.
    // Returns: changed?
    bool changePassword(const QString& hashed_password_varname,
                        const QString& text,
                        QString* p_old_password = nullptr,
                        QString* p_new_password = nullptr);

    // Creates a new random device ID.
    void regenerateDeviceId();

    // Sets the app's lock state.
    void setLockState(LockState lockstate);

signals:
    // "Something has changed the app's lock state."
    void lockStateChanged(LockState lockstate);

    // ------------------------------------------------------------------------
    // Network
    // ------------------------------------------------------------------------
public:
    // Return the app's NetworkManager object.
    NetworkManager* networkManager() const;

    // Do we need to upload new data?
    bool needsUpload() const;

    // Tells the app it needs to upload new data.
    void setNeedsUpload(bool needs_upload);

protected:
    // Makes a new NetworkManager.
    void makeNetManager();

signals:
    // Signal that the "needs upload" state has changed.
    void needsUploadChanged(bool needs_upload);

    // ------------------------------------------------------------------------
    // Patient
    // ------------------------------------------------------------------------
public:
    // Is a patient selected?
    bool isPatientSelected() const;

    // Select a patient by ID.
    void setSelectedPatient(int patient_id, bool force_refresh = false);

    // Deselect a patient.
    void deselectPatient(bool force_refresh = false);

    // Force the patient list to be refreshed
    void forceRefreshPatientList();

    // Tell the app that a patient's details may have changed.
    void patientHasBeenEdited(int patient_id);

    // Returns the selected patient (or nullptr).
    Patient* selectedPatient() const;

    // Returns the selected patient's ID (or dbconst::NONEXISTENT_PK).
    int selectedPatientId() const;

    // Returns all patients.
    PatientPtrList getAllPatients(bool sorted = true);

protected:
    void reloadPatient(int patient_id);

signals:
    // The patient selection has changed (new patient selected or deselected).
    void selectedPatientChanged(const Patient* patient);

    // The details (e.g. name/DOB/...) of the selected patient have changed.
    void selectedPatientDetailsChanged(const Patient* patient);

    // Emitted when the patient list needs to be refreshed.
    void refreshPatientList();

    // ------------------------------------------------------------------------
    // CSS convenience; fonts etc.
    // ------------------------------------------------------------------------
public:
    // From a .css file, perform substitutions (e.g. for our current font
    // sizes) and return the final CSS.
    QString getSubstitutedCss(const QString& filename) const;

    // Return the font size in points for a given font size (in terms of
    // uiconst::FontSize, e.g. small/medium/big) and scaling factor (in
    // percent).
    int fontSizePt(uiconst::FontSize fontsize, double factor_pct = -1) const;

    // Return the app's detected DPI settings.
    // These are the Qt settings, ignoring any override settings.
    Dpi qtLogicalDotsPerInch() const;
    Dpi qtPhysicalDotsPerInch() const;

signals:
    // Emitted when the user has changed the font size settings.
    void fontSizeChanged();

    // ------------------------------------------------------------------------
    // Server info: version, ID numbers, policies
    // ------------------------------------------------------------------------
public:
    // The CamCOPS server's version.
    Version serverVersion() const;

    // The server's upload policy.
    IdPolicy uploadPolicy() const;

    // The server's finalize (preserve) policy.
    IdPolicy finalizePolicy() const;

    // Do we have an ID description for a given ID number type?
    bool idDescriptionExists(int which_idnum);

    // Returns the ID description for a given ID number type.
    QString idDescription(int which_idnum);

    // Returns the ID short description for a given ID number type.
    QString idShortDescription(int which_idnum);

    // Wipe the app's copies of all ID number descriptions.
    void deleteAllIdDescriptions();

    // Store an ID numbers description and other details.
    bool setIdDescription(int which_idnum, const QString& desc,
                          const QString& shortdesc,
                          const QString& validation_method);

    // Return all ID number descriptions.
    QVector<IdNumDescriptionPtr> getAllIdDescriptions();

    // Which ID number types are available?
    QVector<int> whichIdNumsAvailable();

    // Return the ID description information for the specified ID number type.
    IdNumDescriptionConstPtr getIdInfo(int which_idnum);

protected:
    void clearIdDescriptionCache();
    mutable QMap<int, IdNumDescriptionConstPtr> m_iddescription_cache;

    // ------------------------------------------------------------------------
    // Extra strings (downloaded from server)
    // ------------------------------------------------------------------------
public:
    // Return an xstring (extra string) for the given task and string name.
    QString xstring(const QString& taskname, const QString& stringname,
                    const QString& default_str = "");

    // Does the app know about any extra strings for the specified task name?
    bool hasExtraStrings(const QString& taskname);

    // Clear the in-memory string cache.
    void clearExtraStringCache();

    // Delete all downloaded extra strings from the database.
    void deleteAllExtraStrings();

    // Set (store to database) all extra strings from the download information.
    void setAllExtraStrings(const RecordList& recordlist);

    // Return an appstring (an extra string for the app, not a specific task).
    QString appstring(const QString& stringname,
                      const QString& default_str = "");

protected:
    // Load an "extra string" from the database.
    // This is also (partly) where translations get implemented.
    QString xstringDirect(const QString& taskname, const QString& stringname,
                          const QString& default_str = "");
    mutable QMap<QPair<QString, QString>, QString> m_extrastring_cache;

    // ------------------------------------------------------------------------
    // Allowed tables on the server
    // ------------------------------------------------------------------------
public:
    // Tell the app (via a download record) which tables the server will
    // permit to be uploaded.
    void setAllowedServerTables(const RecordList& recordlist);

    // May this app upload a specific table?
    // (This depends on whether the table exists on the server and if the
    // server/client versions permit information exchange for this table.)
    bool mayUploadTable(const QString& tablename,
                        const Version& server_version,
                        bool& server_has_table,
                        Version& min_client_version,
                        Version& min_server_version);

protected:
    // Return all tables from the "data" database that aren't main or ancillary
    // task tables -- that is: blobs, patient, patient_idnum.
    QStringList nonTaskTables() const;

    // What's the minimum server version we'll accept to upload the specified
    // table?
    Version minServerVersionForTable(const QString& tablename);

    // Clear the "allowed server tables" information.
    void deleteAllowedServerTables();

    // ------------------------------------------------------------------------
    // Stored variables: generic
    // ------------------------------------------------------------------------
    // These have a hidden cache system to reduce database access, in that
    // m_storedvars stores values and doesn't ask the database again:
public:
    // Return a stored variable.
    QVariant var(const QString& name) const;

    // Return a stored variable as a string.
    QString varString(const QString& name) const;

    // Return a stored variable as a bool.
    bool varBool(const QString& name) const;

    // Return a stored variable as an int.
    int varInt(const QString& name) const;

    // Return a stored variable as a qint64 (qlonglong).
    qint64 varLongLong(const QString& name) const;

    // Return a stored variable as a double.
    double varDouble(const QString& name) const;

    // Sets a stored variable.
    bool setVar(const QString& name, const QVariant& value,
                bool save_to_db = true);

    // Does a stored variable exist?
    bool hasVar(const QString& name) const;

    // Return a FieldRefPtr to a stored variable.
    FieldRefPtr storedVarFieldRef(const QString& name, bool mandatory = true,
                                  bool cached = true);

    // And so we can operate on an "externally visible" cached version, for
    // editing settings (with an option to save or discard), we have a second
    // cache:

    // Clear the storedvar editing cache.
    void clearCachedVars();

    // Gets a storedvar from the editing cache.
    QVariant getCachedVar(const QString& name) const;

    // Sets a storedvar in the editing cache.
    bool setCachedVar(const QString& name, const QVariant& value);

    // Has a storedvar changed in the cache?
    bool cachedVarChanged(const QString& name) const;

public slots:
    // Save the changes from the storedvar editing cache to the database.
    void saveCachedVars();

protected:
    void createVar(const QString& name, QVariant::Type type,
                   const QVariant& default_value = QVariant());

    // ------------------------------------------------------------------------
    // Terms and conditions
    // ------------------------------------------------------------------------
public:
    // When did the user agree the terms and conditions?
    QDateTime agreedTermsAt() const;

protected:
    // Has the user agreed the terms and conditions?
    bool hasAgreedTerms() const;

    // Offer terms and conditions to the user. Stop the app if they refuse.
    void offerTerms();

    // ------------------------------------------------------------------------
    // Uploading
    // ------------------------------------------------------------------------
public:
    // Upload to the server.
    void upload();

    // ------------------------------------------------------------------------
    // App strings, or derived
    // ------------------------------------------------------------------------
public:
    // Returns name/value options for the standard UK NHS marital status codes.
    NameValueOptions nhsPersonMaritalStatusCodeOptions();

    // Returns name/value options for the standard UK NHS ethnicity codes.
    NameValueOptions nhsEthnicCategoryCodeOptions();

    static const QString DEFAULT_LANGUAGE;

    // ------------------------------------------------------------------------
    // Internal data
    // ------------------------------------------------------------------------
protected:

    // Translators; see https://doc.qt.io/qt-5/internationalization.html
    QSharedPointer<QTranslator> m_qt_translator;  // translates Qt strings
    QSharedPointer<QTranslator> m_app_translator;  // translates CamCOPS strings

    // Database directory.
    QString m_database_path;

    // "Data" database (for task data).
    DatabaseManagerPtr m_datadb;

    // "System" database (for app config and server info).
    DatabaseManagerPtr m_sysdb;

    // Task factory.
    TaskFactoryPtr m_p_task_factory;

    // The app's lock state.
    LockState m_lockstate;

    // The main menu (first, bottom-most) window.
    QPointer<QMainWindow> m_p_main_window;

    // The stack of visible windows.
    QPointer<QStackedWidget> m_p_window_stack;

    // The stack of hidden windows.
    QSharedPointer<QStackedWidget> m_p_hidden_stack;  // we own it entirely, so QSharedPointer

    // Before we went fullscreen, were we maximized?
    bool m_maximized_before_fullscreen;

    // The currently selected patient
    PatientPtr m_patient;

    // Information about windows we've opened in the stack.
    QStack<OpenableInfo> m_info_stack;

    // Are stored variables available for use?
    bool m_storedvars_available;

    // Current language code
    QString m_current_language;

    // "Stored variables" (app config settings), by name.
    QMap<QString, StoredVarPtr> m_storedvars;

    // Our network manager object.
    QSharedPointer<NetworkManager> m_netmgr;

    // Editing cache for stored variables.
    mutable QMap<QString, QVariant> m_cachedvars;

    // Our DPI settings.
    // - physical: actual device DPI
    // - logical: the user may be able to control this via their desktop
    //   settings
    Dpi m_qt_logical_dpi;
    Dpi m_qt_physical_dpi;
};
