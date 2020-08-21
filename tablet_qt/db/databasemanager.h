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
#include <QMap>
#include <QMutex>
#include <QSharedPointer>
#include <QSqlDatabase>
#include <QVector>
#include <QWaitCondition>
#include "db/dbfunc.h"
#include "db/queryresult.h"
#include "db/sqlargs.h"
#include "db/sqlitepragmainfofield.h"
#include "db/threadedqueryrequest.h"
#include "db/whereconditions.h"
#include "db/whichdb.h"
class DatabaseWorkerThread;
class Field;
class QJsonArray;


// A database manager for SQLite/SQLCipher databases. It provides:
// - raw SQL access
// - query functions
// - background threading (delayed write) and synchronization

class DatabaseManager
{
    /*

    Called by the main (GUI) thread.
    Owns and operates a worker thread which handles ALL database comms for
    a single database, if multithreading mode is used.

    See database_performance.txt

    In terms of the number of threads: threads are cheap when they're
    waiting and we don't really want a single thread having to wait both
    for "open" requests and for query requests. The thread has to own its
    database connection fully. So: one thread per database, and one
    DatabaseManager per database. There should be no file-level crosstalk
    between the databases (it all goes via CamCOPS) so there is no
    requirement to synchronize mutexes across >1 database.

    In terms of the thread control mechanism:

    There are several ways to operate with QThread:

    - QThread (owned by creator thread) running a thread with a worker
      QObject (living in the new, QThread, thread) -- this model has the
      QThread running an event loop, and allows the worker QThread to
      receive signals;

    - subclassing QThread and overriding run(), for when you don't care
      about event loops or signals.

    The latter is tempting (and it's what we end up doing). However, we do
    have to deal with waiting for both an exit signal (e.g. an atomic bool),
    and database events to arrive. In a run() loop, we don't want to "spin"
    needlessly in what becomes a while(1) loop, like this:

        while (!exit_flag) {
            if (stuff_waiting) {
                process_stuff();
            }
            // otherwise loop around and around
        }

    but we can't do this:

        while (!exit_flag) {
            wait_for_stuff_to_arrive_eg_mutex_signal();  // WON'T NOTICE EXIT SIGNAL HERE
            process_stuff();
        }

    so the correct solution may be:

        event loop
        -> signal "quit" [well, just QThread::quit()]
        -> signal "stuff arrived" -> process it and return to event loop
        ... and can never be in both at the same time.

      ... probably more elegant and safer.
      How will we mimic a blocking call to the thread?
      Can still use QWaitCondition m_queries_are_complete.

      Note also another pattern:
        https://stackoverflow.com/questions/3556421/blocked-waiting-for-a-asynchronous-qt-signal

      Mind you, there is overhead to an event loop, and we are not going to use
      a dummy event loop to receive "blocking" notifications back on the main
      thread.

      And we can share a QWaitCondition between "incoming" and "abort":

        forever {
            // Fetch a request
            m_mutex_requests.lock();
            if (m_requests.isEmpty()) {
                m_requests_waiting_or_abort.wait(&m_mutex_requests);
            }
            if (m_requests.isEmpty()) {
                // m_requests_waiting_or_abort triggered with no requests;
                // this means "abort"!
            }
            QueryRequest request = m_requests.pop_front();
            bool now_empty = m_requests.isEmpty();
            m_mutex_requests.unlock();

            // Execute the request and push result if required
            execute(request);

            // If that (even transiently) cleared the request queue, let anyone
            // who was waiting for the results know
            if (now_empty) {
                m_queries_are_complete.wakeAll();
            }
        }

      or we could use a request that is a "quit" request...

      These things all save some (potentially large) copying.

    */

    friend class DatabaseWorkerThread;

public:
    // What to fetch from queries:

    // ========================================================================
    // Constructor and destructor:
    // ========================================================================
    DatabaseManager(const QString& filename,
                    const QString& connection_name,
                    const QString& database_type = whichdb::DBTYPE,
                    bool threaded = true);
    ~DatabaseManager();

    // ========================================================================
    // Settings
    // ========================================================================

    // Should we issue a VACUUM command when the database is closed?
    // (That's a good time to vacuum; users rarely care much how fast their
    // applications close.)
    void setVacuumOnClose(bool vacuum_on_close);

    // ========================================================================
    // Main methods:
    // ========================================================================

    // Execute an SQL command and ignore any reply.
    void execNoAnswer(const SqlArgs& sqlargs, bool suppress_errors = false);

    // Executes an SQL query and return the result.
    QueryResult query(const SqlArgs& sqlargs,
                      QueryResult::FetchMode fetch_mode = QueryResult::FetchMode::FetchAll,
                      bool store_column_names = false,
                      bool suppress_errors = false);

    // Executes an SQL command/query and returns whether it succeeded.
    bool exec(const SqlArgs& sqlargs, bool suppress_errors = false);

    // ========================================================================
    // Convenience query methods
    // ========================================================================

    // Version of execNoAnswer() with separate sql/args parameters.
    void execNoAnswer(const QString& sql, const ArgList& args = ArgList());

    // Version of exec() with separate sql/args parameters.
    bool exec(const QString& sql, const ArgList& args = ArgList());

    // Version of query() with separate sql/args parameters.
    QueryResult query(const QString& sql,
                      const ArgList& args = ArgList(),
                      QueryResult::FetchMode fetch_mode = QueryResult::FetchMode::FetchAll,
                      bool store_column_names = false,
                      bool suppress_errors = false);

    // Version of query() with sql parameter only.
    QueryResult query(const QString& sql,
                      QueryResult::FetchMode fetch_mode,
                      bool store_column_names = false,
                      bool suppress_errors = false);

    // ========================================================================
    // SQL
    // ========================================================================

    // ------------------------------------------------------------------------
    // Select
    // ------------------------------------------------------------------------

    // Executes an SQL query and returns the first column of the first row.
    QVariant fetchFirstValue(const SqlArgs& sqlargs);

    // Executes an SQL query and returns the first column of the first row.
    QVariant fetchFirstValue(const QString& sql);

    // Executes an SQL query and returns the first column of the first row
    // as an integer.
    int fetchInt(const SqlArgs& sqlargs, int failure_default = -1);

    // Executes an SQL COUNT() query and returns the count.
    int count(const QString& tablename,
              const WhereConditions& where = WhereConditions());

    // Executes "SELECT <fieldname> FROM <tablename> WHERE <where>" and returns
    // the values as integers.
    QVector<int> getSingleFieldAsIntList(const QString& tablename,
                                         const QString& fieldname,
                                         const WhereConditions& where);

    // Returns all integer PKs from the specified table/PK column.
    QVector<int> getPKs(const QString& tablename,
                        const QString& pkname,
                        const WhereConditions& where = WhereConditions());

    // Does a record with the specified primary key (PK) exist?
    bool existsByPk(const QString& tablename,
                    const QString& pkname, int pkvalue);

    // ------------------------------------------------------------------------
    // Transactions
    // ------------------------------------------------------------------------

    // Executes "BEGIN TRANSACTION".
    void beginTransaction();

    // Executes "COMMIT".
    void commit();

    // Executes "ROLLBACK".
    void rollback();

    // ------------------------------------------------------------------------
    // Modifications
    // ------------------------------------------------------------------------

    // Deletes from a table according to the WHERE conditions.
    bool deleteFrom(const QString& tablename,
                    const WhereConditions& where = WhereConditions());

    // ------------------------------------------------------------------------
    // Reading schema/structure
    // ------------------------------------------------------------------------

    // Returns the names of all tables in the database.
    QStringList getAllTables();

    // Does a table exist in the database?
    bool tableExists(const QString& tablename);

    // Returns the SQLite "PRAGMA table_info" information for a table.
    QVector<SqlitePragmaInfoField> getPragmaInfo(const QString& tablename);

    // Returns the field (column) names for a table.
    QStringList getFieldNames(const QString& tablename);

    // Returns SQL to recreate a table.
    QString dbTableDefinitionSql(const QString& tablename);

    // Estimates the database size on disk, in bytes.
    qint64 approximateDatabaseSize();

    // ------------------------------------------------------------------------
    // Altering schema/structure
    // ------------------------------------------------------------------------

    // Creates an index on the specified fields.
    bool createIndex(const QString& indexname,
                     const QString& tablename,
                     QStringList fieldnames);

    // Renames columns.
    // (This is a complex operation involving a temporary table.)
    void renameColumns(const QString& tablename,
                       const QVector<QPair<QString, QString>>& from_to,
                       const QString& tempsuffix = dbfunc::TABLE_TEMP_SUFFIX);

    // Renames a table.
    void renameTable(const QString& from, const QString& to);

    // Changes the column types of specified columns.
    // (This is a complex operation involving a temporary table.)
    void changeColumnTypes(const QString& tablename,
                           const QVector<QPair<QString, QString>>& field_newtype,
                           const QString& tempsuffix = dbfunc::TABLE_TEMP_SUFFIX);

    // Creates a table.
    void createTable(const QString& tablename,
                     const QVector<Field>& fieldlist,
                     const QString& tempsuffix = dbfunc::TABLE_TEMP_SUFFIX);

    // Drops (deletes) a table.
    void dropTable(const QString& tablename);

    // Drops (deletes) multiple tables.
    void dropTables(const QStringList& tables);

    // Drops tables other than those specified.
    void dropTablesNotIn(const QStringList& good_tables);

    // Get tables that are present in the database but were not explicitly
    // created (this session) via createTable().
    QStringList tablesNotExplicitlyCreatedByUs();

    // Drops tables that were not explicitly created (this session) via
    // createTable().
    void dropTablesNotExplicitlyCreatedByUs();

    // ------------------------------------------------------------------------
    // Performance tweaks
    // ------------------------------------------------------------------------

    // Executes "VACUUM".
    void vacuum();

    // ------------------------------------------------------------------------
    // Encryption queries, via SQLCipher
    // ------------------------------------------------------------------------

    // Can we read the database? If not, we've probably given the wrong
    // password.
    bool canReadDatabase();

    // Performs all steps necessary to read an encrypted database.
    // - passphrase: the passphrase for "PRAGMA key"
    // - migrate: if true, run "PRAGMA cipher_migrate"
    // - compatibility_sqlcipher_major_version: if >0 and migrate is false,
    //   run PRAGMA cipher_compatibility
    bool decrypt(const QString& passphrase,
                 bool migrate = false,
                 int compatibility_sqlcipher_major_version = -1);

    // Executes "PRAGMA key" to access an encrypted database.
    bool pragmaKey(const QString& passphase);

    // Executes "PRAGMA cipher_compatibility" to access an older SQLCipher
    // database.
    bool pragmaCipherCompatibility(int sqlcipher_major_version);

    // Executes "PRAGMA cipher_migrate" to migrate from an older SQLCipher
    // version
    bool pragmaCipherMigrate();

    // Executes "PRAGMA rekey" to change a database's password.
    bool pragmaRekey(const QString& passphase);

    // Is the database empty?
    bool databaseIsEmpty();

    // Exports the entire database to another, encrypted, database.
    bool encryptToAnother(const QString& filename, const QString& passphrase);

    // ------------------------------------------------------------------------
    // JSON output
    // ------------------------------------------------------------------------

    // Returns the entire database in a JSON representation.
    QString getDatabaseAsJson();

    // Returns a table (with all its data) in a JSON representation.
    QJsonArray getTableAsJson(const QString& tablename);

    // ========================================================================
    // Internals
    // ========================================================================
protected:

    // ------------------------------------------------------------------------
    // Opening/closing internals
    // ------------------------------------------------------------------------

    // Opens the database, or stops the whole app.
    void openDatabaseOrDie();

    // Opens the database (directly or via a worker thread); returns success.
    bool openDatabase();

    // Low-level function to open a database directly; returns success.
    bool openDatabaseActual();

    // Closes the database (directly or via a worker thread).
    void closeDatabase();

    // Low-level function to close a database directly.
    void closeDatabaseActual();

    // ------------------------------------------------------------------------
    // GUI thread internals
    // ------------------------------------------------------------------------

    // Pushes a request onto the request queue.
    void pushRequest(const ThreadedQueryRequest& request);

    // Waits for all pending queries to complete.
    void waitForQueriesToComplete();

    // Returns the next reply from the result queue.
    QueryResult popResult();

    // ------------------------------------------------------------------------
    // Worker thread internals
    // ------------------------------------------------------------------------

    // Worker thread main function. Reads requests, processes them (which
    // may cause results to join the result queue).
    void work();

    // Executes an SQL command/query; may cause a result to join the result
    // queue.
    void execute(const ThreadedQueryRequest& request);

    // Adds a result to the result queue.
    void pushResult(const QueryResult& result);

    // ------------------------------------------------------------------------
    // Debugging and DANGEROUS internals
    // ------------------------------------------------------------------------

    // Returns the low-level driver object.
    QSqlDriver* driver() const;  // UNCERTAIN IF THIS IS OK to return the driver on the GUI thread, even if it lives in another

protected:
    // Database filename
    QString m_filename;  // written only in constructor; thread-safe access

    // Internal name of the database connection
    QString m_connection_name;  // written only in constructor; thread-safe access

    // Database type, e.g. "QSQLITE" or "SQLCIPHER".
    QString m_database_type;  // written only in constructor; thread-safe access

    // Are we using a multithreaded approach? (Faster.)
    bool m_threaded;  // written only in constructor; thread-safe access

    // Issue a VACUUM when we close the database?
    bool m_vacuum_on_close;

    // Our worker thread
    QSharedPointer<DatabaseWorkerThread> m_thread;

    // Have we opened the database?
    bool m_opened_database;  // used in a thread-safe way

    // Failure message if opening failed.
    QString m_opening_failure_msg;

    // Underlying Qt database object.
    QSqlDatabase m_db;  // connection owned by worker thread if m_threaded, else GUI thread

    // Mutex to lock the request queue.
    QMutex m_mutex_requests;
    // SQL request (execution) queue.
    QVector<ThreadedQueryRequest> m_requests;

    // Mutex to lock the result queue.
    QMutex m_mutex_results;
    // Query result (reply) queue.
    QVector<QueryResult> m_results;

    // Wait condition: "the database is open"
    QWaitCondition m_open_db_complete;
    // Wait condition: "requests are waiting"
    QWaitCondition m_requests_waiting;
    // Wait condition: "all queries are complete"
    QWaitCondition m_queries_are_complete;

    // Names of tables that we have created via createTable().
    QStringList m_created_tables;  // trivial internal helper; accessed only by GUI thread

    friend class HelpMenu;  // for driver() access
};
