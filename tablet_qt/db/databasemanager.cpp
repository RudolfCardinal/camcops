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

#define USE_MULTITHREADED_DATABASES  // gives much better performance
#define ONE_SELECT_AT_A_TIME
    // ... enforces the principle that callers using SELECT should consume
    // their results before doing another SELECT (so: OK to leave this on).

// #define DEBUG_BACKGROUND_QUERY
// #define DEBUG_VERBOSE_PROCESS
// #define DEBUG_VERBOSE_RESULTS
// #define DEBUG_REPORT_TABLE_STRUCTURE_OK

#include "databasemanager.h"
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include "db/databaseworkerthread.h"
#include "db/dbfunc.h"
#include "db/field.h"
#include "db/fieldcreationplan.h"
#include "db/whereconditions.h"
#include "lib/containers.h"
#include "lib/convert.h"
#include "lib/uifunc.h"
using dbfunc::delimit;

// QSqlDatabase doesn't need to be passed by pointer; it copies itself
// safely. See qsqldatabase.cpp (and note also that pass-by-copy, rather than
// pointers or const references, is how QSqlQuery works in any case).


// ============================================================================
// Constructor and destructor
// ============================================================================

DatabaseManager::DatabaseManager(const QString& filename,
                                 const QString& connection_name,
                                 const QString& database_type,
                                 const bool threaded) :
    m_filename(filename),
    m_connection_name(connection_name),
    m_database_type(database_type),
    m_threaded(threaded),
    m_vacuum_on_close(true),
    m_thread(nullptr),
    m_opened_database(false)
{
    // GUI thread
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif
    openDatabaseOrDie();
}


DatabaseManager::~DatabaseManager()
{
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif
    closeDatabase();
}


// ============================================================================
// Settings
// ============================================================================

void DatabaseManager::setVacuumOnClose(const bool vacuum_on_close)
{
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif
    m_vacuum_on_close = vacuum_on_close;
}


// ============================================================================
// Opening/closing internals
// ============================================================================

void DatabaseManager::openDatabaseOrDie()
{
    // GUI thread
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif
    if (openDatabase()) {
        qInfo() << "Opened database:" << m_filename;
    } else {
        uifunc::stopApp(m_opening_failure_msg);
    }
}


bool DatabaseManager::openDatabase()
{
    // GUI thread
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif
    if (m_threaded) {
        if (!m_thread) {
            m_thread = QSharedPointer<DatabaseWorkerThread>(
                        new DatabaseWorkerThread(this));
            // We need a (semi-)random mutex to lock:
            m_mutex_requests.lock();
            m_thread->start();  // will call openDatabaseActual()
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name
             << "... waiting for m_open_db_complete";
#endif
            m_open_db_complete.wait(&m_mutex_requests);  // woken by: work()
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name
             << "... woken by m_open_db_complete";
#endif
            m_mutex_requests.unlock();
        }
        return m_opened_database;
    }
    return openDatabaseActual();
}


bool DatabaseManager::openDatabaseActual()
{
    // GUI OR WORKER THREAD
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif
    if (m_db.isOpen()) {
        m_opened_database = true;
        return true;
    }
    m_db = QSqlDatabase::addDatabase(m_database_type, m_connection_name);
    m_db.setDatabaseName(m_filename);
    m_opened_database = m_db.open();
    if (m_opened_database) {
        m_opening_failure_msg = "";
    } else {
        QSqlError error = m_db.lastError();
        m_opening_failure_msg = QString(
            "Connection to database failed. "
            "Database = %1; native error code = %2; error text = %3"
        ).arg(m_filename, error.nativeErrorCode(), error.text());
    }
    return m_opened_database;
}


void DatabaseManager::closeDatabase()
{
    // GUI thread
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif
    if (m_vacuum_on_close) {
        vacuum();
    }
    if (m_threaded) {
        if (m_thread) {
            ThreadedQueryRequest request(SqlArgs(),
                                         QueryResult::FetchMode::NoAnswer,
                                         false, false,
                                         true);  // special "die" request
            pushRequest(request);
            m_thread->wait();  // wait for it to finish (and close the database)
            m_thread = nullptr;  // deletes the thread
        }
    } else {
        closeDatabaseActual();
    }
}


void DatabaseManager::closeDatabaseActual()
{
    // GUI OR WORKER THREAD
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif
    if (m_db.isOpen()) {
        m_db.close();
        qInfo()<< "Qt will give a warning next (... \"all queries will cease "
                  "to work\") as we're about to call removeDatabase(); "
                  "this is OK";
        QSqlDatabase::removeDatabase(m_connection_name);
    }
    m_db = QSqlDatabase();

    // http://stackoverflow.com/questions/9519736/warning-remove-database
    // http://www.qtcentre.org/archive/index.php/t-40358.html
}


// ============================================================================
// Public API
// ============================================================================

void DatabaseManager::execNoAnswer(const SqlArgs& sqlargs,
                                   const bool suppress_errors)
{
    // GUI thread
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif
    if (m_threaded) {
        ThreadedQueryRequest request(sqlargs, QueryResult::FetchMode::NoAnswer,
                                     false, suppress_errors);
        pushRequest(request);
    } else {
        QSqlQuery query(m_db);
        dbfunc::execQuery(query, sqlargs, suppress_errors);
    }
}


QueryResult DatabaseManager::query(const SqlArgs& sqlargs,
                                   QueryResult::FetchMode fetch_mode,
                                   const bool store_column_names,
                                   const bool suppress_errors)
{
    // GUI thread
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif
    Q_ASSERT(fetch_mode != QueryResult::FetchMode::NoAnswer);
    // ... don't use the query() interface if you want no answer; use execNoAnswer()

    if (m_threaded) {
        // 1. Queue the query
        ThreadedQueryRequest request(sqlargs, fetch_mode, store_column_names,
                                     suppress_errors);
        pushRequest(request);

        // 2. Wait for all queries to finish
        waitForQueriesToComplete();

        // 3. Read the result
        return popResult();
    }

    QSqlQuery query(m_db);
    bool success = dbfunc::execQuery(query, sqlargs, suppress_errors);
    QueryResult result(query, success, fetch_mode, store_column_names);
    return result;
}


bool DatabaseManager::exec(const SqlArgs& sqlargs, const bool suppress_errors)
{
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif
    const QueryResult result = query(sqlargs, QueryResult::FetchMode::NoFetch,
                                     false, suppress_errors);
    return result.succeeded();
}


// ============================================================================
// GUI thread internals
// ============================================================================

void DatabaseManager::pushRequest(const ThreadedQueryRequest& request)
{
    // GUI thread
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name
             << "... pushing request:" << request;
#endif

    m_mutex_requests.lock();
    m_requests.push_back(request);
    m_mutex_requests.unlock();

    m_requests_waiting.wakeAll();  // wakes: work()
}


QueryResult DatabaseManager::popResult()
{
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif
    m_mutex_results.lock();
    QueryResult result = m_results.front();
#ifdef DEBUG_VERBOSE_RESULTS
    qDebug().nospace() << "Result:\n" << result;
#endif
    m_results.pop_front();
#ifdef ONE_SELECT_AT_A_TIME
    Q_ASSERT(m_results.isEmpty());
#endif
    m_mutex_results.unlock();

    return result;
}


void DatabaseManager::waitForQueriesToComplete()
{
    // GUI thread
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif

    m_mutex_requests.lock();
    if (!m_requests.isEmpty()) {  // must hold mutex to read this
#ifdef DEBUG_VERBOSE_PROCESS
        qDebug() << Q_FUNC_INFO << m_connection_name
                 << "... requests exist; waiting for m_queries_are_complete";
#endif
        m_queries_are_complete.wait(&m_mutex_requests);  // woken by: work()
#ifdef DEBUG_VERBOSE_PROCESS
        qDebug() << Q_FUNC_INFO << m_connection_name
                 << "... woken by m_queries_are_complete";
#endif
        // ... this mutex is UNLOCKED as we go to sleep, and LOCKED
        //     as we wake: http://doc.qt.io/qt-5/qwaitcondition.html#wait
    }
#ifdef DEBUG_VERBOSE_PROCESS
    else {
        qDebug() << m_connection_name
                 << "... no pending query requests; proceed";
    }
#endif
    m_mutex_requests.unlock();
}


// ============================================================================
// Worker thread internals
// ============================================================================

void DatabaseManager::work()
{
    // Main worker thread function.
    // When we leave this function, the thread will terminate.
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif

    m_opened_database = openDatabaseActual();
    m_open_db_complete.wakeAll();  // wakes: openDatabase()

    forever {
        // Fetch a request
        m_mutex_requests.lock();
        if (m_requests.isEmpty()) {
            m_requests_waiting.wait(&m_mutex_requests);  // woken by: pushRequest()
        }
        ThreadedQueryRequest request = m_requests.front();
        // DO NOT CALL pop_front() YET - might be interpreted by
        // waitForQueriesToComplete() at just the wrong moment as "no queries
        // waiting"
        m_mutex_requests.unlock();

#ifdef DEBUG_VERBOSE_PROCESS
        qDebug() << Q_FUNC_INFO << m_connection_name
                 << "... processing request:" << request;
#endif

        if (request.thread_abort_request_not_query) {
            // Dummy query that means "die".
            closeDatabaseActual();
            return;
        }

        // Execute the request and push result if required
        execute(request);

        // Now we can remove the request:
        m_mutex_requests.lock();
        m_requests.pop_front();
        const bool now_empty = m_requests.isEmpty();
        m_mutex_requests.unlock();

        // If that (even transiently) cleared the request queue, let anyone
        // who was waiting for the results know
        if (now_empty) {
            m_queries_are_complete.wakeAll();  // wakes: waitForQueriesToComplete()
        }
    }
}


void DatabaseManager::execute(const ThreadedQueryRequest& request)
{
    // Worker thread
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif

    // 1. Prepare query
    QSqlQuery query(m_db);

    // 2. Execute query
#ifdef DEBUG_BACKGROUND_QUERY
    qDebug() << m_connection_name
             << "Executing background query:" << request;
#endif
    const bool success = dbfunc::execQuery(query,
                                           request.sqlargs,
                                           request.suppress_errors);

    // 3. Deal with results.
    //    NOTE that even if the query fails, we must push a (blank) result,
    //    to meet the guarantee of SELECT -> result every time.
    if (request.fetch_mode != QueryResult::FetchMode::NoAnswer) {
        QueryResult result(query,
                           success,
                           request.fetch_mode,
                           request.store_column_names);
        pushResult(result);
    }
}


void DatabaseManager::pushResult(const QueryResult& result)
{
    // Worker thread
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif
    m_mutex_results.lock();
#ifdef ONE_SELECT_AT_A_TIME
    Q_ASSERT(m_results.isEmpty());
#endif
    m_results.push_back(result);
    m_mutex_results.unlock();
}


// ============================================================================
// Convenience methods (all GUI thread)
// ============================================================================

void DatabaseManager::execNoAnswer(const QString& sql, const ArgList& args)
{
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif
    const SqlArgs sqlargs(sql, args);
    execNoAnswer(sqlargs);
}


bool DatabaseManager::exec(const QString& sql, const ArgList& args)
{
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif
    const SqlArgs sqlargs(sql, args);
    return exec(sqlargs);
}


QueryResult DatabaseManager::query(const QString& sql,
                                   const ArgList& args,
                                   const QueryResult::FetchMode fetch_mode,
                                   const bool store_column_names,
                                   const bool suppress_errors)
{
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif
    const SqlArgs sqlargs(sql, args);
    return query(sqlargs, fetch_mode, store_column_names, suppress_errors);
}


QueryResult DatabaseManager::query(const QString& sql,
                                   const QueryResult::FetchMode fetch_mode,
                                   const bool store_column_names,
                                   const bool suppress_errors)
{
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif
    const SqlArgs sqlargs(sql);
    return query(sqlargs, fetch_mode, store_column_names, suppress_errors);
}


// ============================================================================
// DANGEROUS INTERNALS
// ============================================================================

QSqlDriver* DatabaseManager::driver() const
{
#ifdef DEBUG_VERBOSE_PROCESS
    qDebug() << Q_FUNC_INFO << m_connection_name;
#endif
    return m_db.driver();
}


// ============================================================================
// SQL (all GUI thread)
// ============================================================================

// ----------------------------------------------------------------------------
// Select
// ----------------------------------------------------------------------------

QVariant DatabaseManager::fetchFirstValue(const SqlArgs& sqlargs)
{
    return query(sqlargs, QueryResult::FetchMode::FetchFirst).firstValue();
}


QVariant DatabaseManager::fetchFirstValue(const QString& sql)
{
    return fetchFirstValue(SqlArgs(sql));
}


int DatabaseManager::fetchInt(const SqlArgs& sqlargs,
                              const int failure_default)
{
    QueryResult result = query(sqlargs, QueryResult::FetchMode::FetchFirst);
    if (!result.succeeded()) {
        return failure_default;
    }
    return result.firstValue().toInt();
}


int DatabaseManager::count(const QString& tablename,
                           const WhereConditions& where)
{
    SqlArgs sqlargs("SELECT COUNT(*) FROM " + delimit(tablename));
    where.appendWhereClauseTo(sqlargs);
    return fetchInt(sqlargs, 0);
}


QVector<int> DatabaseManager::getSingleFieldAsIntList(
        const QString& tablename,
        const QString& fieldname,
        const WhereConditions& where)
{
    SqlArgs sqlargs(QString("SELECT %1 FROM %2").arg(delimit(fieldname),
                                                     delimit(tablename)));
    where.appendWhereClauseTo(sqlargs);
    const QueryResult result = query(sqlargs);
    return result.firstColumnAsIntList();
}


QVector<int> DatabaseManager::getPKs(const QString& tablename,
                                     const QString& pkname,
                                     const WhereConditions& where)
{
    return getSingleFieldAsIntList(tablename, pkname, where);
}


bool DatabaseManager::existsByPk(const QString& tablename,
                                 const QString& pkname,
                                 const int pkvalue)
{
    const SqlArgs sqlargs(
        QString("SELECT EXISTS(SELECT * FROM %1 WHERE %2 = ?)")
                .arg(delimit(tablename),
                     delimit(pkname)),
        ArgList{pkvalue}
    );
    // EXISTS always returns 0 or 1
    // https://www.sqlite.org/lang_expr.html
    return fetchInt(sqlargs) == 1;
}


// ----------------------------------------------------------------------------
// Transactions
// ----------------------------------------------------------------------------

void DatabaseManager::beginTransaction()
{
    execNoAnswer("BEGIN TRANSACTION");
}


void DatabaseManager::commit()
{
    // If we ever need to do proper transactions, use an RAII object that
    // executes BEGIN TRANSATION on creation and either COMMIT or ROLLBACK
    // on deletion, and/or handles nesting via SAVEPOINT/RELEASE.
    execNoAnswer("COMMIT");
}


void DatabaseManager::rollback()
{
    execNoAnswer("ROLLBACK");
}


// ----------------------------------------------------------------------------
// Modifications
// ----------------------------------------------------------------------------

bool DatabaseManager::deleteFrom(const QString& tablename,
                                 const WhereConditions& where)
{
    SqlArgs sqlargs(QString("DELETE FROM %1").arg(delimit(tablename)));
    where.appendWhereClauseTo(sqlargs);
    return exec(sqlargs);
}


// ----------------------------------------------------------------------------
// Reading schema/structure
// ----------------------------------------------------------------------------

QStringList DatabaseManager::getAllTables()
{
    // System tables begin with sqlite_
    // - https://www.sqlite.org/fileformat.html
    // An underscore is a wildcard for LIKE
    // - https://www.sqlite.org/lang_expr.html
    const QString sql = "SELECT name "
                        "FROM sqlite_master "
                        "WHERE sql NOT NULL "
                        "AND type='table' "
                        "AND name NOT LIKE 'sqlite\\_%' ESCAPE '\\' "
                        "ORDER BY name";
    const QueryResult result = query(sql);
    return result.firstColumnAsStringList();
}


bool DatabaseManager::tableExists(const QString& tablename)
{
    const SqlArgs sqlargs(
        "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name=?",
        {tablename}
    );
    return fetchInt(sqlargs) > 0;
}


QVector<SqlitePragmaInfoField> DatabaseManager::getPragmaInfo(
        const QString& tablename)
{
    const QString sql = QString("PRAGMA table_info(%1)").arg(delimit(tablename));
    const QueryResult result = query(sql);
    if (!result.succeeded()) {
        uifunc::stopApp("getPragmaInfo: PRAGMA table_info failed for "
                        "table " + tablename);
    }
    QVector<SqlitePragmaInfoField> infolist;
    const int nrows = result.nRows();
    for (int row = 0; row < nrows; ++row) {
        SqlitePragmaInfoField fieldinfo;
        fieldinfo.cid = result.at(row, 0).toInt();  // column ID
        fieldinfo.name = result.at(row, 1).toString();
        fieldinfo.type = result.at(row, 2).toString();
        fieldinfo.notnull = result.at(row, 3).toBool();
        fieldinfo.dflt_value = result.at(row, 4);
        fieldinfo.pk = result.at(row, 5).toBool();
        infolist.append(fieldinfo);
    }
    return infolist;
}


QStringList DatabaseManager::getFieldNames(const QString& tablename)
{
    const QVector<SqlitePragmaInfoField> infolist = getPragmaInfo(tablename);
    return dbfunc::fieldNamesFromPragmaInfo(infolist);
}


QString DatabaseManager::dbTableDefinitionSql(const QString& tablename)
{
    const QString sql = "SELECT sql FROM sqlite_master WHERE tbl_name=?";
    const ArgList args({tablename});
    return fetchFirstValue(SqlArgs(sql, args)).toString();
}


qint64 DatabaseManager::approximateDatabaseSize()
{
    // NB includes dead pages; VACUUM first for a better answer
    const QString sql = "SELECT page_count * page_size AS size "
                        "FROM pragma_page_count(), pragma_page_size()";
    return fetchFirstValue(sql).toLongLong();
}


// ----------------------------------------------------------------------------
// Altering schema/structure
// ----------------------------------------------------------------------------

bool DatabaseManager::createIndex(const QString& indexname,
                                  const QString& tablename,
                                  QStringList fieldnames)
{
    if (!tableExists(tablename)) {
        qWarning() << "WARNING: ignoring createIndex for non-existent table:"
                   << tablename;
        return false;
    }
    for (int i = 0; i < fieldnames.size(); ++i) {
        fieldnames[i] = delimit(fieldnames.at(i));
    }
    const QString sql = QString("CREATE INDEX IF NOT EXISTS %1 ON %2 (%3)").arg(
        delimit(indexname), delimit(tablename), fieldnames.join(", "));
    return exec(sql);
}


void DatabaseManager::renameColumns(
        const QString& tablename,
        const QVector<QPair<QString, QString>>& from_to,
        const QString& tempsuffix)
{
    if (!tableExists(tablename)) {
        qWarning() << "WARNING: ignoring renameColumns for non-existent table:"
                   << tablename;
        return;
    }
    QString creation_sql = dbTableDefinitionSql(tablename);
    QStringList old_fieldnames = getFieldNames(tablename);
    QStringList new_fieldnames = old_fieldnames;
    const QString dummytable = tablename + tempsuffix;
    if (tableExists(dummytable)) {
        uifunc::stopApp("renameColumns: temporary table exists: " +
                        dummytable);
    }
    int n_changes = 0;
    for (const QPair<QString, QString>& pair : from_to) {  // For each rename...
        const QString& from = pair.first;
        const QString& to = pair.second;
        if (from == to) {
            continue;
        }
        // Check the source is valid
        if (!old_fieldnames.contains(from)) {
            uifunc::stopApp("renameColumns: 'from' field doesn't "
                            "exist: " + tablename + "." + from);
        }
        // Check the destination doesn't exist already
        if (new_fieldnames.contains(to)) {
            uifunc::stopApp(
                "renameColumns: destination field already exists (or "
                "attempt to rename two columns to the same name): " +
                tablename + "." + to);
        }
        // Rename the fieldname in the new_fieldnames list, and in the SQL
        new_fieldnames[new_fieldnames.indexOf(from)] = to;
        creation_sql.replace(delimit(from), delimit(to));
        ++n_changes;
    }
    if (n_changes == 0) {
        qDebug() << m_connection_name
                 << "renameColumns: nothing to do:" << tablename;
        return;
    }
    qDebug() << Q_FUNC_INFO << m_connection_name;
    qDebug() << "- table:" << tablename;
    qDebug() << "- from_to:" << from_to;
    qDebug() << "- old_fieldnames:" << old_fieldnames;
    qDebug() << "- new_fieldnames:" << new_fieldnames;
    // Delimit everything
    const QString delimited_tablename = delimit(tablename);
    const QString delimited_dummytable = delimit(dummytable);
    for (int i = 0; i < old_fieldnames.size(); ++i) {
        old_fieldnames[i] = delimit(old_fieldnames.at(i));
        new_fieldnames[i] = delimit(new_fieldnames.at(i));
    }
    beginTransaction();
    execNoAnswer(QString("ALTER TABLE %1 RENAME TO %2").arg(
                     delimited_tablename, delimited_dummytable));
    // Make a new, clean table:
    execNoAnswer(creation_sql);
    // Copy the data across:
    execNoAnswer(QString("INSERT INTO %1 (%2) SELECT %3 FROM %4").arg(
             delimited_tablename,
             new_fieldnames.join(","),
             old_fieldnames.join(","),
             delimited_dummytable));
    // Drop the temporary table:
    dropTable(dummytable);
    commit();
}


void DatabaseManager::renameTable(const QString& from, const QString& to)
{
    if (!tableExists(from)) {
        qWarning() << Q_FUNC_INFO
                   << "WARNING: ignoring renameTable for non-existent table:"
                   << from;
        return;
    }
    if (tableExists(to)) {
        uifunc::stopApp("renameTable: destination table already exists: " +
                        to);
    }
    // http://stackoverflow.com/questions/426495
    execNoAnswer(QString("ALTER TABLE %1 RENAME TO %2").arg(from, to));
    // don't COMMIT (error: "cannot commit - no transaction is active")
}


void DatabaseManager::changeColumnTypes(
        const QString& tablename,
        const QVector<QPair<QString, QString>>& changes,
        const QString& tempsuffix)
{
    // changes: pairs <fieldname, newtype>
    if (!tableExists(tablename)) {
        qWarning() << "WARNING: ignoring changeColumnTypes for non-existent "
                      "table:" << tablename;
        return;
    }
    const QString dummytable = tablename + tempsuffix;
    if (tableExists(dummytable)) {
        uifunc::stopApp("changeColumnTypes: temporary table exists: " +
                        dummytable);
    }
    QVector<SqlitePragmaInfoField> infolist = getPragmaInfo(tablename);
    qDebug() << m_connection_name << "changeColumnTypes";
    qDebug() << "- pragma info:" << infolist;
    qDebug() << "- changes:" << changes;
    int n_changes = 0;
    for (int i = 0; i < changes.size(); ++i) {
        const QString changefield = changes.at(i).first;
        for (int j = 0; i < infolist.size(); ++j) {
            SqlitePragmaInfoField& info = infolist[j];
            if (changefield.compare(info.name, Qt::CaseInsensitive) == 0) {
                QString newtype = changes.at(i).second;
                info.type = newtype;
                ++n_changes;
            }
        }
    }
    if (n_changes == 0) {
        qDebug() << "... nothing to do";
        return;
    }
    const QString creation_sql = dbfunc::makeCreationSqlFromPragmaInfo(
                tablename, infolist);
    const QString fieldnames = dbfunc::fieldNamesFromPragmaInfo(
                infolist, true).join(",");
    const QString delimited_tablename = delimit(tablename);
    const QString delimited_dummytable = delimit(dummytable);
    beginTransaction();
    execNoAnswer(QString("ALTER TABLE %1 RENAME TO %2").arg(
                     delimited_tablename, delimited_dummytable));
    execNoAnswer(creation_sql);  // make a new clean table
    execNoAnswer(QString("INSERT INTO %1 (%2) SELECT %3 FROM %4").arg(
         delimited_tablename,
         fieldnames,
         fieldnames,
         delimited_dummytable));
    dropTable(dummytable);
    commit();
}


void DatabaseManager::createTable(const QString& tablename,
                                  const QVector<Field>& fieldlist,
                                  const QString& tempsuffix)
{
    // Record the created table name. If we ever use
    // dropTablesNotExplicitlyCreatedByUs(), it is vital that ALL table
    // creation calls come through this function.
    m_created_tables.append(tablename);

    const QString creation_sql = dbfunc::sqlCreateTable(tablename, fieldlist);
    if (!tableExists(tablename)) {
        // Create table from scratch.
        qInfo() << "Creating table" << tablename;
        execNoAnswer(creation_sql);
        return;
    }

    // Otherwise, it's a bit more complex...

    // 1. Create a list of plans. Start with the fields we want, which we
    //    will add (unless later it turns out they exist already).
    QVector<FieldCreationPlan> planlist;
    QStringList goodfieldlist;
    for (const Field& field : fieldlist) {
        FieldCreationPlan p;
        p.name = field.name();
        p.intended_field = &field;
        p.add = true;
        planlist.append(p);
        goodfieldlist.append(delimit(p.name));
    }

    // 2. Fetch a list of existing fields.
    // - If any are in our "desired" list, and we didn't know they were in
    //   the database, don't add them (but maybe change them if we want them
    //   to have a different type).
    // - If they're not in our "desired" list, then they're superfluous, so
    //   aim to drop them.
    const QVector<SqlitePragmaInfoField> infolist = getPragmaInfo(tablename);
    for (const SqlitePragmaInfoField& info : infolist) {
        bool existing_is_superfluous = true;
        for (FieldCreationPlan& plan : planlist) {
            const Field* intended_field = plan.intended_field;
            if (!intended_field) {
                // This shouldn't happen!
                continue;
            }
            if (!plan.exists_in_db && intended_field->name() == info.name) {
                plan.exists_in_db = true;
                plan.add = false;
                plan.change = (
                    info.type != intended_field->sqlColumnType() ||
                    info.notnull != intended_field->notNull() ||
                    info.pk != intended_field->isPk()
                );
                plan.existing_type = info.type;
                plan.existing_not_null = info.notnull;
                existing_is_superfluous = false;
            }
        }
        if (existing_is_superfluous) {
            FieldCreationPlan plan;
            plan.name = info.name;
            plan.exists_in_db = true;
            plan.existing_type = info.type;
            plan.drop = true;
            planlist.append(plan);
        }
    }

    // 3. For any fields that require adding: add them.
    //    For any that require dropping or altering, make a note for the
    //    complex step.
    bool drop_or_change_mods_required = false;
    for (const FieldCreationPlan& plan : planlist) {
        if (plan.add && plan.intended_field) {
            if (plan.intended_field->isPk()) {
                uifunc::stopApp(QString(
                    "createTable: Cannot add a PRIMARY KEY column "
                    "(%s.%s)").arg(tablename, plan.name));
            }
            execNoAnswer(QString("ALTER TABLE %1 ADD COLUMN %2 %3").arg(
                tablename,
                delimit(plan.name),
                plan.intended_field->sqlColumnDef()));
        }
        if (plan.drop || plan.change) {
            drop_or_change_mods_required = true;
        }
    }

#ifdef DEBUG_VERBOSE_TABLE_CHANGE_PLANS
    qDebug() << Q_FUNC_INFO << m_connection_name
             << "tablename:" << tablename
             << "goodfieldlist:" << goodfieldlist
             << "infolist:" << infolist
             << "modifications_required:" << drop_or_change_mods_required
             << "plan:" << planlist;
#endif

    if (!drop_or_change_mods_required) {
#ifdef DEBUG_REPORT_TABLE_STRUCTURE_OK
        qDebug() << "Table" << tablename
                 << "OK; no drop/change alteration required";
#endif
        return;
    }

    // 4. Implement drop/change modifications (via a temporary table).
    qDebug().nospace() << "Amendment plan for " << tablename
                       << ": " << planlist;
    // Deleting columns: http://www.sqlite.org/faq.html#q11
    // ... also http://stackoverflow.com/questions/8442147/
    // Basically, requires (a) copy data to temporary table; (b) drop original;
    // (c) create new; (d) copy back.
    // Or, another method: (a) rename table; (b) create new; (c) copy data
    // across; (d) drop temporary.
    // We deal with fields of incorrect type similarly (in this case, any
    // conversion occurs as we SELECT back the values into the new, proper
    // fields). Not sure it really is important, though:
    // http://sqlite.org/datatype3.html
    const QString dummytable = tablename + tempsuffix;
    if (tableExists(dummytable)) {
        uifunc::stopApp("createTable: temporary table exists: " + dummytable);
    }
    const QString delimited_tablename = delimit(tablename);
    const QString delimited_dummytable = delimit(dummytable);
    const QString goodfieldstring = goodfieldlist.join(",");
    qInfo() << "Modifying structure of table:" << tablename;
    beginTransaction();
    execNoAnswer(QString("ALTER TABLE %1 RENAME TO %2").arg(
                     delimited_tablename, delimited_dummytable));
    execNoAnswer(creation_sql);  // make a new clean table
    execNoAnswer(QString("INSERT INTO %1 (%2) SELECT %3 FROM %4").arg(
         delimited_tablename,
         goodfieldstring,
         goodfieldstring,
         delimited_dummytable));
    dropTable(dummytable);
    commit();
}


void DatabaseManager::dropTable(const QString& tablename)
{
    qInfo() << "Dropping table:" << tablename;
    execNoAnswer(QString("DROP TABLE %1").arg(delimit(tablename)));
}


void DatabaseManager::dropTables(const QStringList& tables)
{
    for (const QString& tablename : tables) {
        dropTable(tablename);
    }
}


void DatabaseManager::dropTablesNotIn(const QStringList& good_tables)
{
    const QStringList existing = getAllTables();
    const QStringList superfluous = containers::setSubtract(existing, good_tables);
    dropTables(superfluous);
}


QStringList DatabaseManager::tablesNotExplicitlyCreatedByUs()
{
    // See createTable(), which writes m_created_tables
    const QStringList existing = getAllTables();
    return containers::setSubtract(existing, m_created_tables);
}


void DatabaseManager::dropTablesNotExplicitlyCreatedByUs()
{
    dropTables(tablesNotExplicitlyCreatedByUs());
}


// ----------------------------------------------------------------------------
// Performance tweaks
// ----------------------------------------------------------------------------

void DatabaseManager::vacuum()
{
    qInfo() << "Vacuuming database" << m_filename;
    execNoAnswer("VACUUM");
}


// ----------------------------------------------------------------------------
// Encryption queries, via SQLCipher
// ----------------------------------------------------------------------------

bool DatabaseManager::canReadDatabase()
{
    const QueryResult result = query("SELECT COUNT(*) FROM sqlite_master",
                                     QueryResult::FetchMode::NoFetch,
                                     false,
                                     true);  // suppress errors
    return result.succeeded();
    // We suppress errors if this fails. It will fail if the database
    // is encrypted and we've not supplied the right key.
}


bool DatabaseManager::decrypt(const QString &passphrase,
                              const bool migrate,
                              const int compatibility_sqlcipher_major_version)
{
    bool success = pragmaKey(passphrase);
    if (migrate) {
        // You might think that there's no point doing cipher_migrate if we can
        // read the database, and calls to canReadDatabase() are quick, so we
        // should check that first. However, this sequence fails:
        //
        //      SELECT COUNT(*) FROM sqlite_master;  -- OK; "Error: file is not a database"
        //      PRAGMA key = 'passphrase';  -- OK
        //      SELECT COUNT(*) FROM sqlite_master;  -- causes a problem; "Error: file is not a database"
        //      PRAGMA cipher_migrate;  -- "1"
        //      .tables  -- "Error: file is not a database"
        //
        // whereas this works:
        //
        //      SELECT COUNT(*) FROM sqlite_master;  -- "Error: file is not a database"
        //      PRAGMA key = 'passphrase';
        //      PRAGMA cipher_migrate;  -- "0"
        //      .tables  -- works fine
        //
        // and this also works:
        //
        //      SELECT COUNT(*) FROM sqlite_master;
        //      PRAGMA key = 'passphrase';
        //      SELECT COUNT(*) FROM sqlite_master;  -- causes a problem; "Error: file is not a database"
        //      PRAGMA key = 'passphrase';  -- resets the problem
        //      PRAGMA cipher_migrate;  -- "0"
        //      .tables  -- works fine
        //
        // So we must proceed to "PRAGMA cipher_migrate" directly every time,
        // like this:
        //
        //   success = success && pragmaCipherMigrate();
        //
        // or re-call "PRAGMA key". Since cipher_migrate takes about 0.25s to
        // do nothing, which is significant (esp. for two databases), let's do
        // that:

        if (!canReadDatabase()) {
            success = success && pragmaKey(passphrase) && pragmaCipherMigrate();
        }

        // This way is obviously quicker (empirically) once cipher_migrate has
        // become unnecessary.

    } else if (compatibility_sqlcipher_major_version > 0) {
        success = pragmaCipherCompatibility(compatibility_sqlcipher_major_version);
    }
    return success;
}


bool DatabaseManager::pragmaKey(const QString& passphase)
{
    // "PRAGMA key" is specific to SQLCipher
    const QString sql = QString("PRAGMA key=%1")
            .arg(convert::toSqlLiteral(passphase));
    return exec(sql);
}


bool DatabaseManager::pragmaCipherCompatibility(
        const int sqlcipher_major_version)
{
    // "PRAGMA cipher_compatibility = <level>" is specific to SQLCipher
    if (sqlcipher_major_version < 1) {
        return false;
    }
    const QString sql = QString("PRAGMA cipher_compatibility = %1")
            .arg(sqlcipher_major_version);
    return exec(sql);
}


bool DatabaseManager::pragmaCipherMigrate()
{
    // "PRAGMA cipher_migrate" is specific to SQLCipher
    const QString sql("PRAGMA cipher_migrate");
    return exec(sql);
}


bool DatabaseManager::pragmaRekey(const QString& passphase)
{
    // "PRAGMA rekey" is specific to SQLCipher
    const QString sql = QString("PRAGMA rekey=%1")
            .arg(convert::toSqlLiteral(passphase));
    return exec(sql);
}


bool DatabaseManager::databaseIsEmpty()
{
    return count("sqlite_master") == 0;
}


bool DatabaseManager::encryptToAnother(const QString& filename,
                                       const QString& passphrase)
{
    // ATTACH DATABASE can create and encrypt from scratch, so the file
    // specified by "filename" doesn't have to exist.
    return exec(QString("ATTACH DATABASE %1 AS encrypted KEY %2")
                .arg(convert::toSqlLiteral(filename),
                     convert::toSqlLiteral(passphrase))) &&
            exec("SELECT sqlcipher_export('encrypted')") &&
            exec("DETACH DATABASE encrypted");
}


// ----------------------------------------------------------------------------
// JSON output
// ----------------------------------------------------------------------------

QString DatabaseManager::getDatabaseAsJson()
{
    QJsonObject root;
    for (const QString& tablename : getAllTables()) {
        root[tablename] = getTableAsJson(tablename);
    }
    const QJsonDocument jsondoc(root);
    return jsondoc.toJson(QJsonDocument::Compact);
}


QJsonArray DatabaseManager::getTableAsJson(const QString& tablename)
{
    SqlArgs sqlargs(QString("SELECT * FROM %1").arg(delimit(tablename)));
    const QueryResult result = query(sqlargs, QueryResult::FetchMode::FetchAll,
                                     true);
    return result.jsonRows();
}
