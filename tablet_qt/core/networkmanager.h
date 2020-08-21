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

// #define DEBUG_OFFER_HTTP_TO_SERVER  // should NOT be defined in production (which is HTTPS only)

#include <QNetworkRequest>
#include <QMap>
#include <QPointer>
#include <QObject>
#include <QSqlDatabase>
#include <QSslError>
#include <QString>
#include <QUrl>
#include "common/aliases_camcops.h"
#include "common/design_defines.h"

class CamcopsApp;
class LogBox;
class QNetworkAccessManager;
class QNetworkReply;


// Controls network operations, optionally providing a progress display.
class NetworkManager : public QObject
{
    Q_OBJECT

    // ------------------------------------------------------------------------
    // Shorthand
    // ------------------------------------------------------------------------

    using ReplyFuncPtr = void (NetworkManager::*)(QNetworkReply*);
    // ... a pointer to a member function of NetworkManager that takes a
    // QNetworkReply* parameter and returns void

    // ------------------------------------------------------------------------
    // Helper classes
    // ------------------------------------------------------------------------

public:
    // How should we upload?
    enum class UploadMethod {
        Copy,
        MoveKeepingPatients,
        Move
    };

    // ------------------------------------------------------------------------
    // Core
    // ------------------------------------------------------------------------
public:
    NetworkManager(CamcopsApp& app, DatabaseManager& db,
                   TaskFactoryPtr p_task_factory, QWidget* parent);
    ~NetworkManager();

    // ------------------------------------------------------------------------
    // User interface
    // ------------------------------------------------------------------------
public:
    // Operate in silent mode (without status information)?
    void setSilent(bool silent);

    // Sets the window title.
    void setTitle(const QString& title);

    // Shows a plain-text status message.
    void statusMessage(const QString& msg) const;

    // Shows an HTML status message.
    void htmlStatusMessage(const QString& html) const;

protected:
    // Ensure we have a logbox.
    void ensureLogBox() const;

    // Delete our logbox.
    void deleteLogBox();

protected slots:
    // "The user pressed cancel on the logbox dialogue."
    void logboxCancelled();

    // "The user pressed OK/Finish on the logbox dialogue."
    void logboxFinished();

    // ------------------------------------------------------------------------
    // Basic connection management
    // ------------------------------------------------------------------------
protected:

    // Ensure we know the user's upload password (ask if not).
    bool ensurePasswordKnown();

    // Disconnect signals/slots from our Qt manager object.
    void disconnectManager();

    // Create a generic network request.
    QNetworkRequest createRequest(
            const QUrl& url,
            bool offer_cancel,
            bool ssl,
            bool ignore_ssl_errors,
            QSsl::SslProtocol ssl_protocol = QSsl::AnyProtocol);

    // Returns the URL for the CamCOPS server, as a QUrl.
    QUrl serverUrl(bool& success) const;

    // Returns the URL for the CamCOPS server, as a string.
    QString serverUrlDisplayString() const;

    // Create a request to our server.
    QNetworkRequest createServerRequest(bool& success);

    // Send a message to the server via an HTTP POST, and set up a callback
    // for the results.
    void serverPost(Dict dict, ReplyFuncPtr reply_func,
                    bool include_user = true);

    // Process the server's reply into our internal data structures,
    // principally m_reply_dict.
    bool processServerReply(QNetworkReply* reply);

    // Formats a human-readable version of "size", e.g. "3 Kb" or similar.
    QString sizeBytes(qint64 size) const;

    // Returns a list of downloaded records from our internal m_reply_dict.
    RecordList getRecordList() const;

    // Does the reply have the correct format from the CamCOPS API?
    bool replyFormatCorrect() const;

    // Did the reply say it was successful?
    bool replyReportsSuccess() const;

    // Wipe internal transmission/reply information.
    void cleanup();

protected slots:
    // We come here when there's an SSL error and we want to ignore it.
    void sslIgnoringErrorHandler(QNetworkReply* reply,
                                 const QList<QSslError>& errlist);

public slots:
    // "User pressed cancel."
    void cancel();

    // "Network operation failed somehow."
    void fail();

    // "Network operation succeeded."
    void succeed();

    // We're finished, whether successfully or not.
    void finish(bool success = true);

    // ------------------------------------------------------------------------
    // Testing
    // ------------------------------------------------------------------------
public:
    // Tests HTTP GET.
    void testHttpGet(const QString& url, bool offer_cancel = true);

    // Tests HTTPS GET.
    void testHttpsGet(const QString& url, bool offer_cancel = true,
                      bool ignore_ssl_errors = false);

protected:
    // Callback for the tests.
    void testReplyFinished(QNetworkReply* reply);

    // ------------------------------------------------------------------------
    // Server registration
    // ------------------------------------------------------------------------
public:
    // Register with the CamCOPS server.
    void registerWithServer();  // "register" is a C++ keyword

    // Fetch all information without registration (i.e. fetch ID descriptions,
    // table details, extra strings...).
    void fetchAllServerInfo();

    // Fetch ID number type description/information from the server.
    void fetchIdDescriptions();

    // Fetch extra strings from the server.
    void fetchExtraStrings();
protected:
    // Multi-step operations for the above:
    void registerSub1(QNetworkReply* reply);
    void registerSub2(QNetworkReply* reply);
    void registerSub3(QNetworkReply* reply);
    void fetchIdDescriptionsSub1(QNetworkReply* reply);
    void fetchExtraStringsSub1(QNetworkReply* reply);
    void fetchAllServerInfoSub1(QNetworkReply* reply);

    // Store ID/policy information from the server.
    void storeServerIdentificationInfo();

    // Store "which tables are allowed" information from the server.
    void storeAllowedTables();

    // Store extra strings from the server.
    void storeExtraStrings();

    // ------------------------------------------------------------------------
    // Upload
    // ------------------------------------------------------------------------
public:
    // Upload to the server.
    void upload(UploadMethod method);

protected:
    // Upload core:
    void uploadNext(QNetworkReply* reply);
    // Specific upload comms:
    void checkDeviceRegistered();
    void checkUploadUser();
    void uploadFetchServerIdInfo();
    void uploadValidatePatients();
    void uploadFetchAllowedTables();
    void startUpload();
    void startPreservation();
    void sendEmptyTables(const QStringList& tablenames);
    void sendTableWhole(const QString& tablename);
    void sendTableRecordwise(const QString& tablename);
    void requestRecordwisePkPrune();
    void sendNextRecord();
    void endUpload();
    // Internal upload functions
    bool isPatientInfoComplete();
    bool applyPatientMoveOffTabletFlagsToTasks();
    bool catalogueTablesForUpload();
    bool isServerVersionOK() const;
    bool arePoliciesOK() const;
    bool areDescriptionsOK() const;
    QVector<int> whichIdnumsUsedOnTablet() const;
    bool pruneRecordwisePks();
    void wipeTables();
    void queryFail(const QString& sql);
    void queryFailClearingMoveOffFlag(const QString& tablename);
    bool clearMoveOffTabletFlag(const QString& tablename);
    bool pruneDeadBlobs();
    bool serverSupportsValidatePatients() const;
    bool serverSupportsOneStepUpload() const;
    bool shouldUseOneStepUpload() const;
    void uploadOneStep();
    QString getPkInfoAsJson();

    // ------------------------------------------------------------------------
    // Signals
    // ------------------------------------------------------------------------
signals:
    // "Operation was cancelled."
    void cancelled();

    // "Operation has finished, successfully or not; user has acknowledged."
    void finished();

    // ------------------------------------------------------------------------
    // Translatable text
    // ------------------------------------------------------------------------
protected:
    static QString txtPleaseRefetchServerInfo();

    // ------------------------------------------------------------------------
    // Data
    // ------------------------------------------------------------------------
protected:
    // Our app.
    CamcopsApp& m_app;

    // The data database.
    DatabaseManager& m_db;

    // Our app's task factory.
    TaskFactoryPtr m_p_task_factory;

    // Parent widget.
    QWidget* m_parent;

    // Window title.
    QString m_title;

    // Offer a cancel button?
    bool m_offer_cancel;

    // Suppress all status messages?
    bool m_silent;

    // Our logbox (triggered when a status message is displayed)
    mutable QPointer<LogBox> m_logbox;

    // Our Qt network manager
    QNetworkAccessManager* m_mgr;

    // Temporary storage of information going to the server:
    QString m_tmp_password;
    QString m_tmp_session_id;
    QString m_tmp_session_token;

    // Incoming information.
    // We store these here to save passing around large objects, and for
    // convenience:
    QByteArray m_reply_data;
    Dict m_reply_dict;  // the main repository of information received

    // How will we upload?
    UploadMethod m_upload_method;

    // Sequencing of the upload steps
    enum class NextUploadStage {
        Invalid,
        CheckUser,
        FetchServerIdInfo,
        ValidatePatients,  // v2.3.0
        FetchAllowedTables,
        CheckPoliciesThenStartUpload,
        StartPreservation,
        Uploading,
        Finished,
    };

    // Internal calculations for uploading.
    NextUploadStage m_upload_next_stage;
    QVector<int> m_upload_patient_ids_to_move_off;
    QStringList m_upload_empty_tables;
    QStringList m_upload_tables_to_send_whole;
    QStringList m_upload_tables_to_send_recordwise;
    QString m_upload_recordwise_table_in_progress;
    QStringList m_upload_recordwise_fieldnames;
    int m_upload_current_record_index;
    bool m_recordwise_prune_req_sent;
    bool m_recordwise_pks_pruned;
    QVector<int> m_upload_recordwise_pks_to_send;
    int m_upload_n_records;  // cached as m_upload_recordwise_pks_to_send shrinks during upload
    QStringList m_upload_tables_to_wipe;
    QString m_upload_patient_info_json;
};
