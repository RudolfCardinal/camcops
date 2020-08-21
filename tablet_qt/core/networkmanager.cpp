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

// #define DEBUG_NETWORK_REQUESTS
// #define DEBUG_NETWORK_REPLIES_RAW
// #define DEBUG_NETWORK_REPLIES_DICT
// #define DEBUG_ACTIVITY
// #define DEBUG_JSON
#define USE_BACKGROUND_DATABASE

#include "networkmanager.h"
#include <functional>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QSqlQuery>
#include <QtGlobal>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QSslConfiguration>
#include <QUrl>
#include <QUrlQuery>
#include "common/preprocessor_aid.h"
#include "common/varconst.h"
#include "core/camcopsapp.h"
#include "db/dbnestabletransaction.h"
#include "dbobjects/idnumdescription.h"
#include "dbobjects/patientidnum.h"
#include "dialogs/passwordentrydialog.h"
#include "dialogs/logbox.h"
#include "dbobjects/blob.h"
#include "lib/containers.h"
#include "lib/convert.h"
#include "lib/datetime.h"
#include "lib/idpolicy.h"
#include "lib/uifunc.h"
#include "tasklib/task.h"
#include "tasklib/taskfactory.h"
#include "version/camcopsversion.h"

using dbfunc::delimit;

// Keys used by server or client (S server, C client, B bidirectional)
// SEE ALSO patient.cpp, for the JSON ones.
const QString KEY_CAMCOPS_VERSION("camcops_version");  // C->S
const QString KEY_DATABASE_TITLE("databaseTitle");  // S->C
const QString KEY_DATEVALUES("datevalues");  // C->S
const QString KEY_DBDATA("dbdata");  // C->S, new in v2.3.0
const QString KEY_DEVICE("device");  // C->S
const QString KEY_DEVICE_FRIENDLY_NAME("devicefriendlyname");  // C->S
const QString KEY_ERROR("error");  // S->C
const QString KEY_FIELDS("fields");    // B; fieldnames
const QString KEY_FINALIZING("finalizing");  // C->S, in JSON, v2.3.0
const QString KEY_ID_POLICY_UPLOAD("idPolicyUpload");  // S->C
const QString KEY_ID_POLICY_FINALIZE("idPolicyFinalize");  // S->C
const QString KEY_MOVE_OFF_TABLET_VALUES("move_off_tablet_values");  // C->S, v2.3.0
const QString KEY_NFIELDS("nfields");  // B
const QString KEY_NRECORDS("nrecords");  // B
const QString KEY_OPERATION("operation");  // C->S
const QString KEY_PASSWORD("password");  // C->S
const QString KEY_PATIENT_INFO("patient_info");  // C->S, new in v2.3.0
const QString KEY_PKNAME("pkname");  // C->S
const QString KEY_PKNAMEINFO("pknameinfo");  // C->S
const QString KEY_PKVALUES("pkvalues");  // C->S
const QString KEY_RESULT("result");  // S->C
const QString KEY_SERVER_CAMCOPS_VERSION("serverCamcopsVersion");  // S->C
const QString KEY_SESSION_ID("session_id");  // B
const QString KEY_SESSION_TOKEN("session_token");  // B
const QString KEY_SUCCESS("success");  // S->C
const QString KEY_TABLE("table");  // C->S
const QString KEY_TABLES("tables");  // C->S
const QString KEY_USER("user");  // C->S
const QString KEY_VALUES("values");  // C->S
const QString KEYPREFIX_ID_DESCRIPTION("idDescription");  // S->C
const QString KEYSPEC_ID_DESCRIPTION(KEYPREFIX_ID_DESCRIPTION + "%1");  // S->C
const QString KEYPREFIX_ID_SHORT_DESCRIPTION("idShortDescription");  // S->C
const QString KEYSPEC_ID_SHORT_DESCRIPTION(KEYPREFIX_ID_SHORT_DESCRIPTION + "%1");  // S->C
const QString KEYPREFIX_ID_VALIDATION_METHOD("idValidationMethod");  // S->C, new in v2.2.8
const QString KEYSPEC_ID_VALIDATION_METHOD(KEYPREFIX_ID_VALIDATION_METHOD + "%1");  // S->C, new in v2.2.8
const QString KEYSPEC_RECORD("record%1");  // B

// Operations for server:
const QString OP_CHECK_DEVICE_REGISTERED("check_device_registered");
const QString OP_CHECK_UPLOAD_USER_DEVICE("check_upload_user_and_device");
const QString OP_DELETE_WHERE_KEY_NOT("delete_where_key_not");
const QString OP_END_UPLOAD("end_upload");
const QString OP_GET_EXTRA_STRINGS("get_extra_strings");
const QString OP_GET_ID_INFO("get_id_info");
const QString OP_GET_ALLOWED_TABLES("get_allowed_tables");  // v2.2.0
const QString OP_REGISTER("register");
const QString OP_START_PRESERVATION("start_preservation");
const QString OP_START_UPLOAD("start_upload");
const QString OP_UPLOAD_ENTIRE_DATABASE("upload_entire_database");  // v2.3.0
const QString OP_UPLOAD_TABLE("upload_table");
const QString OP_UPLOAD_RECORD("upload_record");
const QString OP_UPLOAD_EMPTY_TABLES("upload_empty_tables");
const QString OP_VALIDATE_PATIENTS("validate_patients");  // v2.3.0
const QString OP_WHICH_KEYS_TO_SEND("which_keys_to_send");

const Version MIN_SERVER_VERSION_FOR_VALIDATE_PATIENTS("2.3.0");
const Version MIN_SERVER_VERSION_FOR_ONE_STEP_UPLOAD("2.3.0");

const QString ENCODE_TRUE("1");
const QString ENCODE_FALSE("0");


// ============================================================================
// NetworkManager
// ============================================================================

// - MAIN COMMUNICATION METHOD:
//   serverPost(dict, &callbackfunction);

// CALLBACK LIFETIME SAFETY in this class:
// - There is only one NetworkManager in the whole app, owned by the
//   CamcopsApp.
// - The QNetworkAccessManager lives as long as the NetworkManager.
// - Therefore, any callbacks to this class are lifetime-safe and can use
//   std::bind.
// - HOWEVER, callbacks to something transient may not be (e.g. another object
//   sets up a callback to itself but with std::bind rather than to a QObject;
//   network function is called; object is deleted; network replies; boom).
//   So BEWARE there.
// - Since we have a single set of principal network access functions relating
//   to upload/server interaction, the simplest thing is to build them all into
//   this class, and then we don't have to worry about lifetime problems.


NetworkManager::NetworkManager(CamcopsApp& app,
                               DatabaseManager& db,
                               TaskFactoryPtr p_task_factory,
                               QWidget* parent) :
    m_app(app),
    m_db(db),
    m_p_task_factory(p_task_factory),
    m_parent(parent),
    m_offer_cancel(true),
    m_silent(parent == nullptr),
    m_logbox(nullptr),
    m_mgr(new QNetworkAccessManager(this)),  // will be autodeleted by QObject
    m_upload_method(UploadMethod::Copy),
    m_upload_next_stage(NextUploadStage::Invalid),
    m_upload_current_record_index(0),
    m_recordwise_prune_req_sent(false),
    m_recordwise_pks_pruned(false),
    m_upload_n_records(0)
{
}


NetworkManager::~NetworkManager()
{
    deleteLogBox();
}


// ============================================================================
// User interface
// ============================================================================

void NetworkManager::ensureLogBox() const
{
    if (!m_logbox) {
#ifdef DEBUG_ACTIVITY
        qDebug() << Q_FUNC_INFO << "creating logbox";
#endif
        m_logbox = new LogBox(m_parent, m_title, m_offer_cancel);
        m_logbox->setStyleSheet(
                    m_app.getSubstitutedCss(uiconst::CSS_CAMCOPS_MAIN));
        connect(m_logbox.data(), &LogBox::accepted,
                this, &NetworkManager::logboxFinished,
                Qt::UniqueConnection);
        connect(m_logbox.data(), &LogBox::rejected,
                this, &NetworkManager::logboxCancelled,
                Qt::UniqueConnection);
        m_logbox->open();
    }
}


void NetworkManager::deleteLogBox()
{
    if (!m_logbox) {
        return;
    }
    m_logbox->deleteLater();
    m_logbox = nullptr;
}


void NetworkManager::setSilent(const bool silent)
{
    m_silent = silent;
}


void NetworkManager::setTitle(const QString& title)
{
    m_title = title;
    if (m_logbox) {
        m_logbox->setWindowTitle(title);
    }
}


void NetworkManager::statusMessage(const QString& msg) const
{
    qInfo().noquote() << "Network:" << msg;
    if (m_silent) {
#ifdef DEBUG_ACTIVITY
        qDebug() << Q_FUNC_INFO << "silent";
#endif
        return;
    }
    ensureLogBox();
    m_logbox->statusMessage(
                QString("%1: %2").arg(datetime::nowTimestamp(), msg));
}


void NetworkManager::htmlStatusMessage(const QString& html) const
{
    if (m_silent) {
#ifdef DEBUG_ACTIVITY
        qDebug() << Q_FUNC_INFO << "silent";
#endif
        return;
    }
    ensureLogBox();
    m_logbox->statusMessage(html, true);
}


void NetworkManager::logboxCancelled()
{
    // User has hit cancel
#ifdef DEBUG_ACTIVITY
    qDebug() << Q_FUNC_INFO;
#endif
    cleanup();
    deleteLogBox();
    emit cancelled();
}


void NetworkManager::logboxFinished()
{
    // User has acknowledged finish
#ifdef DEBUG_ACTIVITY
    qDebug() << Q_FUNC_INFO;
#endif
    cleanup();
    deleteLogBox();
    emit finished();
}


// ============================================================================
// Basic connection management
// ============================================================================

void NetworkManager::disconnectManager()
{
    m_mgr->disconnect();
}


QNetworkRequest NetworkManager::createRequest(const QUrl& url,
                                              const bool offer_cancel,
                                              const bool ssl,
                                              const bool ignore_ssl_errors,
                                              QSsl::SslProtocol ssl_protocol)
{
    // Clear any previous callbacks
    disconnectManager();

    m_offer_cancel = offer_cancel;

    QNetworkRequest request;

#ifdef DEBUG_NETWORK_REQUESTS
    qDebug().nospace().noquote()
            << Q_FUNC_INFO
            << ": offer_cancel=" << offer_cancel
            << ", ssl=" << ssl
            << ", ignore_ssl_errors=" << ignore_ssl_errors
            << ", ssl_protocol=" << convert::describeSslProtocol(ssl_protocol);
#endif

    if (ssl) {
        QSslConfiguration config = QSslConfiguration::defaultConfiguration();
        config.setProtocol(ssl_protocol);
        // NB the OpenSSL version must also support the protocol (e.g. TLSv2);
        // ... see also https://bugreports.qt.io/browse/QTBUG-31230
        // ... but TLSv2 working fine with manually compiled OpenSSL
        request.setSslConfiguration(config);
        if (ignore_ssl_errors) {
            QObject::connect(
                        m_mgr, &QNetworkAccessManager::sslErrors,
                        std::bind(&NetworkManager::sslIgnoringErrorHandler,
                                  this, std::placeholders::_1,
                                  std::placeholders::_2));

        }
    }

    // URL
    request.setUrl(url);

    return request;
}


QUrl NetworkManager::serverUrl(bool& success) const
{
    QUrl url;
#ifdef DEBUG_OFFER_HTTP_TO_SERVER
    url.setScheme(m_app.varBool(varconst::DEBUG_USE_HTTPS_TO_SERVER) ? "https"
                                                                     : "http");
#else
    url.setScheme("https");
#endif
    url.setHost(m_app.varString(varconst::SERVER_ADDRESS));
    url.setPort(m_app.varInt(varconst::SERVER_PORT));
    QString path = m_app.varString(varconst::SERVER_PATH);
    if (!path.startsWith('/')) {
        path = "/" + path;
    }
    url.setPath(path);
    success = !url.host().isEmpty();
    return url;
}


QString NetworkManager::serverUrlDisplayString() const
{
    bool success = false;  // we don't care about the result
    QUrl url = serverUrl(success);
    const QString str = url.toDisplayString();
    return str;
}


QNetworkRequest NetworkManager::createServerRequest(bool& success)
{
    QSsl::SslProtocol ssl_protocol = convert::sslProtocolFromDescription(
                m_app.varString(varconst::SSL_PROTOCOL));
    return createRequest(
                serverUrl(success),
                true,  // always offer cancel
                true,  // always use SSL
                !m_app.varBool(varconst::VALIDATE_SSL_CERTIFICATES),  // ignore SSL errors?
                ssl_protocol);
}


void NetworkManager::serverPost(Dict dict, ReplyFuncPtr reply_func,
                                const bool include_user)
{
    // Request (URL, SSL, etc.).
    bool success = true;
    QNetworkRequest request = createServerRequest(success);
    if (!success) {
        statusMessage(tr("Server host details not specified; see Settings"));
        fail();
        return;
    }

    // Complete the dictionary
    // dict[KEY_CAMCOPS_VERSION] = camcopsversion::CAMCOPS_VERSION.toFloatString();  // outdated
    dict[KEY_CAMCOPS_VERSION] = camcopsversion::CAMCOPS_CLIENT_VERSION.toString();  // server copes as of v2.0.0
    dict[KEY_DEVICE] = m_app.deviceId();
    if (include_user) {
        QString user = m_app.varString(varconst::SERVER_USERNAME);
        if (user.isEmpty()) {
            statusMessage(tr("User information required but you have not yet "
                             "specified it; see Settings"));
            fail();
            return;
        }
        dict[KEY_USER] = user;

        if (!ensurePasswordKnown()) {
            statusMessage(tr("Password not specified"));
            fail();
            return;
        }
        dict[KEY_PASSWORD] = m_tmp_password;
    }
    if (!m_tmp_session_id.isEmpty() && !m_tmp_session_token.isEmpty()) {
        dict[KEY_SESSION_ID] = m_tmp_session_id;
        dict[KEY_SESSION_TOKEN] = m_tmp_session_token;
    }

    // Clean up the reply storage objects
    m_reply_data.clear();
    m_reply_dict.clear();

    // Connect up the reply signals
    QObject::connect(m_mgr, &QNetworkAccessManager::finished,
                     this, reply_func);

    // Send the request
    const QUrlQuery postdata = convert::getPostDataAsUrlQuery(dict);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      "application/x-www-form-urlencoded");
    const QByteArray final_data = postdata.toString(QUrl::FullyEncoded).toUtf8();
    // See discussion of encoding in Convert::getPostDataAsUrlQuery
#ifdef DEBUG_NETWORK_REQUESTS
    qDebug() << "Request to server: " << final_data;
#endif
    statusMessage(tr("... sending ") + sizeBytes(final_data.length()));
    m_mgr->post(request, final_data);
}


bool NetworkManager::processServerReply(QNetworkReply* reply)
{
    if (!reply) {
        statusMessage("Bug: null pointer to processServerReply");
        fail();
        return false;
    }
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        statusMessage(tr("Network failure: ") + reply->errorString());
        fail();
        return false;
    }
    m_reply_data = reply->readAll();  // can probably do this only once
    statusMessage(tr("... received ") + sizeBytes(m_reply_data.length()));
#ifdef DEBUG_NETWORK_REPLIES_RAW
    qDebug() << "Network reply (raw): " << m_reply_data;
#endif
    m_reply_dict = convert::getReplyDict(m_reply_data);
#ifdef DEBUG_NETWORK_REPLIES_DICT
    qInfo() << "Network reply (dictionary): " << m_reply_dict;
#endif
    if (!replyFormatCorrect()) {
        statusMessage(tr(
            "Reply is not from CamCOPS API. Are your server settings "
            "misconfigured? Reply is below."));
        htmlStatusMessage(convert::getReplyString(m_reply_data));
        fail();
        return false;
    }
    m_tmp_session_id = m_reply_dict[KEY_SESSION_ID];
    m_tmp_session_token = m_reply_dict[KEY_SESSION_TOKEN];
    if (replyReportsSuccess()) {
        return true;
    }
    // If the server's reporting success=0, it should provide an
    // error too:
    statusMessage(tr("Server reported an error: ") +
                  m_reply_dict[KEY_ERROR]);
    fail();
    return false;
}


QString NetworkManager::sizeBytes(const qint64 size) const
{
    return convert::prettySize(size, true, false, true, "bytes");
}


bool NetworkManager::replyFormatCorrect() const
{
    // Characteristics of a reply that has come from the CamCOPS API, not
    // (for example) a "page not found" error from Apache:
    return m_reply_dict.contains(KEY_SUCCESS) &&
           m_reply_dict.contains(KEY_SESSION_ID) &&
           m_reply_dict.contains(KEY_SESSION_TOKEN);
}


bool NetworkManager::replyReportsSuccess() const
{
    return m_reply_dict[KEY_SUCCESS].toInt();
}


RecordList NetworkManager::getRecordList() const
{
    RecordList recordlist;

    if (!m_reply_dict.contains(KEY_NRECORDS) ||
            !m_reply_dict.contains(KEY_NFIELDS) ||
            !m_reply_dict.contains(KEY_FIELDS)) {
        statusMessage(tr("ERROR: missing field or record information"));
        return RecordList();
    }

    const int nrecords = m_reply_dict[KEY_NRECORDS].toInt();
    if (nrecords <= 0) {
        statusMessage(tr("ERROR: No records"));
        return RecordList();
    }

    int nfields = m_reply_dict[KEY_NFIELDS].toInt();
    const QString fields = m_reply_dict[KEY_FIELDS];
    const QStringList fieldnames = fields.split(',');
    if (nfields != fieldnames.length()) {
        statusMessage(
            tr("WARNING: nfields (%1) doesn't match number of actual "
               "fields (%2); field list is: %3")
                    .arg(nfields)
                    .arg(fieldnames.length())
                    .arg(fields));
        nfields = fieldnames.length();
    }
    if (nfields <= 0) {
        statusMessage(tr("ERROR: No fields"));
        return RecordList();
    }
    for (int r = 0; r < nrecords; ++r) {
        QMap<QString, QVariant> record;
        const QString recordname = KEYSPEC_RECORD.arg(r);
        if (!m_reply_dict.contains(recordname)) {
            statusMessage(tr("ERROR: missing record: ") + recordname);
            return RecordList();
        }
        const QString valuelist = m_reply_dict[recordname];
        const QVector<QVariant> values = convert::csvSqlLiteralsToValues(valuelist);
        if (values.length() != nfields) {
            statusMessage(tr("ERROR: #values not equal to #fields"));
            return RecordList();
        }
        for (int f = 0; f < nfields; ++f) {
            record[fieldnames[f]] = values[f];
        }
        recordlist.push_back(record);
    }
    return recordlist;
}


bool NetworkManager::ensurePasswordKnown()
{
    if (!m_tmp_password.isEmpty()) {
        // We already have it, from whatever source
        return true;
    }
    if (m_app.storingServerPassword()) {
        m_tmp_password = m_app.getPlaintextServerPassword();
        if (!m_tmp_password.isEmpty()) {
            return true;
        }
    }
    // If we get here, either we're not storing the password or it hasn't been
    // entered.
    const QString text = tr("Enter password for user <b>%1</b> on server %2")
            .arg(m_app.varString(varconst::SERVER_USERNAME),
                 serverUrlDisplayString());
    const QString title = tr("Enter server password");
    QWidget* parent = m_logbox ? m_logbox : m_parent;
    PasswordEntryDialog dlg(text, title, parent);
    const int reply = dlg.exec();
    if (reply != QDialog::Accepted) {
        return false;
    }
    // fetch/write back password
    m_tmp_password = dlg.password();
    return true;
}


void NetworkManager::cleanup()
{
    disconnectManager();
    m_tmp_password = "";
    m_tmp_session_id = "";
    m_tmp_session_token = "";
    m_reply_data.clear();
    m_reply_dict.clear();

    m_upload_next_stage = NextUploadStage::Invalid;
    m_upload_patient_ids_to_move_off.clear();
    m_upload_empty_tables.clear();
    m_upload_tables_to_send_whole.clear();
    m_upload_tables_to_send_recordwise.clear();
    m_upload_recordwise_table_in_progress = "";
    m_upload_recordwise_fieldnames.clear();
    m_upload_current_record_index = -1;
    m_upload_recordwise_pks_to_send.clear();
    m_upload_n_records = 0;
    m_upload_tables_to_wipe.clear();
    m_upload_patient_info_json = "";
}


void NetworkManager::sslIgnoringErrorHandler(QNetworkReply* reply,
                                             const QList<QSslError> & errlist)
{
    // Error handle that ignores SSL certificate errors and continues
    statusMessage(tr("+++ Ignoring %1 SSL error(s):").arg(errlist.length()));
    for (const QSslError& err : errlist) {
        statusMessage("    " + err.errorString());
    }
    reply->ignoreSslErrors();
}


void NetworkManager::cancel()
{
#ifdef DEBUG_ACTIVITY
    qDebug() << Q_FUNC_INFO;
#endif
    cleanup();
    if (m_logbox) {
        m_logbox->reject();  // its rejected() signal calls our logboxCancelled()
    } else {
        emit cancelled();
    }
}


void NetworkManager::fail()
{
    finish(false);
}


void NetworkManager::succeed()
{
    finish(true);
}


void NetworkManager::finish(const bool success)
{
#ifdef DEBUG_ACTIVITY
    qDebug() << Q_FUNC_INFO;
#endif
    cleanup();
    if (m_logbox) {
        m_logbox->finish(success);  // its signals call our logboxCancelled() or logboxFinished()
    } else {
        if (success) {
            emit finished();
        } else {
            emit cancelled();
        }
    }
}



// ============================================================================
// Testing
// ============================================================================

void NetworkManager::testHttpGet(const QString& url, const bool offer_cancel)
{
    QNetworkRequest request = createRequest(QUrl(url), offer_cancel,
                                            false, false);
    statusMessage(tr("Testing HTTP GET connection to: ") + url);
    // Safe object lifespan signal: can use std::bind
    QObject::connect(m_mgr, &QNetworkAccessManager::finished,
                     std::bind(&NetworkManager::testReplyFinished,
                               this, std::placeholders::_1));
    // GET
    m_mgr->get(request);
    statusMessage(tr("... sent request to: ") + url);
}


void NetworkManager::testHttpsGet(const QString& url, const bool offer_cancel,
                                  const bool ignore_ssl_errors)
{
    QNetworkRequest request = createRequest(QUrl(url), offer_cancel,
                                            true, ignore_ssl_errors,
                                            QSsl::AnyProtocol);
    statusMessage(tr("Testing HTTPS GET connection to: ") + url);
    // Safe object lifespan signal: can use std::bind
    QObject::connect(m_mgr, &QNetworkAccessManager::finished,
                     std::bind(&NetworkManager::testReplyFinished, this,
                               std::placeholders::_1));
    // Note: the reply callback arrives on the main (GUI) thread.
    // GET
    m_mgr->get(request);
    statusMessage(tr("... sent request to: ") + url);
}


void NetworkManager::testReplyFinished(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        statusMessage(tr("Result:"));
        statusMessage(reply->readAll());
    } else {
        statusMessage(tr("Network error: ") + reply->errorString());
    }
    reply->deleteLater();  // http://doc.qt.io/qt-5/qnetworkaccessmanager.html#details
    finish();
}


// ============================================================================
// Server registration
// ============================================================================

void NetworkManager::registerWithServer()
{
    statusMessage(tr("Registering with ") + serverUrlDisplayString());
    Dict dict;
    dict[KEY_OPERATION] = OP_REGISTER;
    dict[KEY_DEVICE_FRIENDLY_NAME] = m_app.varString(varconst::DEVICE_FRIENDLY_NAME);
    serverPost(dict, &NetworkManager::registerSub1);
}


void NetworkManager::registerSub1(QNetworkReply* reply)
{
    if (!processServerReply(reply)) {
        return;
    }
    statusMessage(tr("... registered and received identification information"));
    storeServerIdentificationInfo();

    statusMessage(tr("Requesting allowed tables"));
    Dict dict;
    dict[KEY_OPERATION] = OP_GET_ALLOWED_TABLES;
    serverPost(dict, &NetworkManager::registerSub2);
}


void NetworkManager::registerSub2(QNetworkReply* reply)
{
    if (!processServerReply(reply)) {
        return;
    }
    statusMessage(tr("... received allowed tables"));
    storeAllowedTables();

    statusMessage(tr("Requesting extra strings"));
    Dict dict;
    dict[KEY_OPERATION] = OP_GET_EXTRA_STRINGS;
    serverPost(dict, &NetworkManager::registerSub3);
}


void NetworkManager::registerSub3(QNetworkReply* reply)
{
    if (!processServerReply(reply)) {
        return;
    }
    statusMessage(tr("... received extra strings"));
    storeExtraStrings();
    statusMessage(tr("Completed successfully."));
    succeed();
}


void NetworkManager::fetchIdDescriptions()
{
    statusMessage(tr("Getting ID info from ") + serverUrlDisplayString());
    Dict dict;
    dict[KEY_OPERATION] = OP_GET_ID_INFO;
    serverPost(dict, &NetworkManager::fetchIdDescriptionsSub1);
}


void NetworkManager::fetchIdDescriptionsSub1(QNetworkReply* reply)
{
    if (!processServerReply(reply)) {
        return;
    }
    statusMessage(tr("... registered and received identification information"));
    storeServerIdentificationInfo();
    succeed();
}


void NetworkManager::fetchExtraStrings()
{
    statusMessage(tr("Getting extra strings from ") + serverUrlDisplayString());
    Dict dict;
    dict[KEY_OPERATION] = OP_GET_EXTRA_STRINGS;
    serverPost(dict, &NetworkManager::fetchExtraStringsSub1);
}


void NetworkManager::fetchExtraStringsSub1(QNetworkReply* reply)
{
    if (!processServerReply(reply)) {
        return;
    }
    statusMessage(tr("... received extra strings"));
    storeExtraStrings();
    succeed();
}


void NetworkManager::fetchAllServerInfo()
{
    statusMessage(tr("Fetching server info from ") + serverUrlDisplayString());
    statusMessage(tr("Requesting ID info"));
    Dict dict;
    dict[KEY_OPERATION] = OP_GET_ID_INFO;
    serverPost(dict, &NetworkManager::fetchAllServerInfoSub1);
}


void NetworkManager::fetchAllServerInfoSub1(QNetworkReply* reply)
{
    if (!processServerReply(reply)) {
        return;
    }
    statusMessage(tr("... received identification information"));
    storeServerIdentificationInfo();

    statusMessage(tr("Requesting allowed tables"));
    Dict dict;
    dict[KEY_OPERATION] = OP_GET_ALLOWED_TABLES;
    // Now we move across to the "registration" chain of functions:
    serverPost(dict, &NetworkManager::registerSub2);
}


void NetworkManager::storeServerIdentificationInfo()
{
    m_app.setVar(varconst::SERVER_DATABASE_TITLE, m_reply_dict[KEY_DATABASE_TITLE]);
    m_app.setVar(varconst::SERVER_CAMCOPS_VERSION, m_reply_dict[KEY_SERVER_CAMCOPS_VERSION]);
    m_app.setVar(varconst::ID_POLICY_UPLOAD, m_reply_dict[KEY_ID_POLICY_UPLOAD]);
    m_app.setVar(varconst::ID_POLICY_FINALIZE, m_reply_dict[KEY_ID_POLICY_FINALIZE]);

    m_app.deleteAllIdDescriptions();
    for (const QString& keydesc : m_reply_dict.keys()) {
        if (keydesc.startsWith(KEYPREFIX_ID_DESCRIPTION)) {
            const QString number = keydesc.right(keydesc.length() -
                                                 KEYPREFIX_ID_DESCRIPTION.length());
            bool ok = false;
            const int which_idnum = number.toInt(&ok);
            if (ok) {
                const QString desc = m_reply_dict[keydesc];
                const QString key_shortdesc = KEYSPEC_ID_SHORT_DESCRIPTION.arg(which_idnum);
                const QString shortdesc = m_reply_dict[key_shortdesc];
                const QString key_validation = KEYSPEC_ID_VALIDATION_METHOD.arg(which_idnum);
                const QString validation_method = m_reply_dict[key_validation];
                m_app.setIdDescription(which_idnum, desc, shortdesc, validation_method);
            } else {
                qWarning() << "Bad ID description key:" << keydesc;
            }
        }
    }

    m_app.setVar(varconst::LAST_SERVER_REGISTRATION, datetime::now());
    m_app.setVar(varconst::LAST_SUCCESSFUL_UPLOAD, QVariant());
    // ... because we might have registered with a different server, we set
    // this to NULL, so it doesn't give the impression that we have uploaded
    // our data to the new server.

    // Deselect patient, or its description text may be out of date
    m_app.deselectPatient();
}


void NetworkManager::storeAllowedTables()
{
    RecordList recordlist = getRecordList();
    m_app.setAllowedServerTables(recordlist);
    statusMessage(tr("Saved %1 allowed tables").arg(recordlist.length()));
}


void NetworkManager::storeExtraStrings()
{
    RecordList recordlist = getRecordList();
    if (!recordlist.isEmpty()) {
        m_app.setAllExtraStrings(recordlist);
        statusMessage(tr("Saved %1 extra strings").arg(recordlist.length()));
    }
}


// ============================================================================
// Upload
// ============================================================================

// ----------------------------------------------------------------------------
// Upload: CORE
// ----------------------------------------------------------------------------

void NetworkManager::upload(const UploadMethod method)
{
    statusMessage(tr("Preparing to upload to: ") + serverUrlDisplayString());
    // ... in part so uploadNext() status message looks OK

    // The GUI doesn't get a chance to respond until after this function
    // has completed.
    // SlowGuiGuard guard();  // not helpful

    m_app.processEvents();  // these, scattered around, are very helpful.

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // 1. Internal database checks/flag-setting
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    cleanup();
    m_upload_method = method;

    // Offline things first:
    if (!isPatientInfoComplete()) {  // also sets m_patient_info_json
        fail();
        return;
    }
    m_app.processEvents();

    statusMessage(tr("Removing any defunct binary large objects"));
    if (!pruneDeadBlobs()) {
        fail();
        return;
    }
    statusMessage(tr("... done"));
    m_app.processEvents();

    statusMessage("Setting move-off flags for tasks, where applicable");
    if (!applyPatientMoveOffTabletFlagsToTasks()) {
        fail();
        return;
    }
    statusMessage(tr("... done"));
    m_app.processEvents();

    m_app.processEvents();

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // 2. Begin comms with the server by checking device is registered.
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    checkDeviceRegistered();
    m_upload_next_stage = NextUploadStage::CheckUser;
    // ... will end up at uploadNext().
}


void NetworkManager::uploadNext(QNetworkReply* reply)
{
    // This function imposes an order on the upload sequence, which makes
    // everything else work.

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Whatever happens next, check the server was happy with our last request.
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    // The option for reply to be nullptr is so we can do a no-op.
    if (reply && !processServerReply(reply)) {
        return;
    }
    if (m_upload_next_stage == NextUploadStage::Invalid) {
        // stage might be Invalid if user hit cancel while messages still
        // inbound
        return;
    }
    statusMessage(tr("... OK"));

    switch (m_upload_next_stage) {

    case NextUploadStage::CheckUser:
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // FROM: check device registration. (Checked implicitly.)
        // TO: check user OK.
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        checkUploadUser();
        m_upload_next_stage = NextUploadStage::FetchServerIdInfo;
        break;

    case NextUploadStage::FetchServerIdInfo:
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // FROM: check user OK. (Checked implicitly.)
        // TO: fetch server ID info (server version, database title,
        //      which ID numbers, ID policies)
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        uploadFetchServerIdInfo();
        m_upload_next_stage = NextUploadStage::ValidatePatients;
        break;

    case NextUploadStage::ValidatePatients:
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // FROM: fetch server ID info
        // TO: ask server to validate patients
        //     ... or if the server doesn't support that, move on another step
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        if (!isServerVersionOK() || !arePoliciesOK() || !areDescriptionsOK()) {
            fail();
            return;
        }
        if (serverSupportsValidatePatients()) {
            uploadValidatePatients();  // v2.3.0
            m_upload_next_stage = NextUploadStage::FetchAllowedTables;
            break;
        } else {
            // Otherwise:

            // [[fallthrough]];
            // ... C++17 syntax!
            // - https://en.cppreference.com/w/cpp/language/attributes
            // - https://en.cppreference.com/w/cpp/language/attributes/fallthrough

            // [[clang::fallthrough]];
            // ... compiler-specific

            Q_FALLTHROUGH();
            // ... well done, Qt
            // - http://doc.qt.io/qt-5/qtglobal.html#Q_FALLTHROUGH
        }

    case NextUploadStage::FetchAllowedTables:
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // FROM: ask server to validate patients
        // TO: fetch allowed tables/minimum client versions
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        uploadFetchAllowedTables();
        m_upload_next_stage = NextUploadStage::CheckPoliciesThenStartUpload;
        break;

    case NextUploadStage::CheckPoliciesThenStartUpload:
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // FROM: fetch allowed tables/minimum client versions
        // TO: start upload or preservation
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        statusMessage("... received allowed tables");
        storeAllowedTables();
        if (!catalogueTablesForUpload()) {  // checks per-table version requirements
            fail();
            return;
        }
        if (shouldUseOneStepUpload()) {
            uploadOneStep();
            m_upload_next_stage = NextUploadStage::Finished;
        } else {
            startUpload();
            if (m_upload_method == UploadMethod::Copy) {
                // If we copy, we proceed to uploading
                m_upload_next_stage = NextUploadStage::Uploading;
            } else {
                // If we're moving, we preserve records.
                m_upload_next_stage = NextUploadStage::StartPreservation;
            }
        }
        break;

    case NextUploadStage::StartPreservation:
        startPreservation();
        m_upload_next_stage = NextUploadStage::Uploading;
        break;

    case NextUploadStage::Uploading:
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // FROM: start upload or preservation
        // TO: upload, tablewise then recordwise (CYCLES ROUND here until done)
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        if (!m_upload_empty_tables.isEmpty()) {

            sendEmptyTables(m_upload_empty_tables);
            m_upload_empty_tables.clear();

        } else if (!m_upload_tables_to_send_whole.isEmpty()) {

            QString table = m_upload_tables_to_send_whole.front();
            m_upload_tables_to_send_whole.pop_front();
            sendTableWhole(table);

        } else if (!m_upload_recordwise_pks_to_send.isEmpty()) {

            if (!m_recordwise_prune_req_sent) {
                requestRecordwisePkPrune();
            } else {
                if (!m_recordwise_pks_pruned) {
                    if (!pruneRecordwisePks()) {
                        fail();
                        return;
                    }
                    if (m_upload_recordwise_pks_to_send.isEmpty()) {
                        // Quasi-recursive way of saying "do whatever you would
                        // have done otherwise", since the server had said "I'm
                        // not interested in any records from that table".
                        statusMessage(tr("... server doesn't want anything "
                                         "from this table"));
                        uploadNext(nullptr);
                        return;
                    }
                }
                sendNextRecord();
            }

        } else if (!m_upload_tables_to_send_recordwise.isEmpty()) {

            QString table = m_upload_tables_to_send_recordwise.front();
            m_upload_tables_to_send_recordwise.pop_front();
            sendTableRecordwise(table);

        } else {

            endUpload();
            m_upload_next_stage = NextUploadStage::Finished;

        }
        break;

    case NextUploadStage::Finished:
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        // FROM: upload, or uploadOneStep()
        // All done successfully!
        // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        wipeTables();
        statusMessage(tr("Finished"));
        m_app.setVar(varconst::LAST_SUCCESSFUL_UPLOAD, datetime::now());
        m_app.setNeedsUpload(false);
        m_app.deselectPatient(true);  // even for "copy" method; see changelog
        m_app.forceRefreshPatientList();
        succeed();
        break;

    default:
        uifunc::stopApp("Bug: unknown m_upload_next_stage");
    }
}


// ----------------------------------------------------------------------------
// Upload: COMMS
// ----------------------------------------------------------------------------

void NetworkManager::checkDeviceRegistered()
{
    statusMessage(tr("Checking device is registered with server"));
    Dict dict;
    dict[KEY_OPERATION] = OP_CHECK_DEVICE_REGISTERED;
    serverPost(dict, &NetworkManager::uploadNext);
}


void NetworkManager::checkUploadUser()
{
    statusMessage(tr("Checking user/device permitted to upload"));
    Dict dict;
    dict[KEY_OPERATION] = OP_CHECK_UPLOAD_USER_DEVICE;
    serverPost(dict, &NetworkManager::uploadNext);
}


void NetworkManager::uploadFetchServerIdInfo()
{
    statusMessage(tr("Fetching server's version/ID policies/ID descriptions"));
    Dict dict;
    dict[KEY_OPERATION] = OP_GET_ID_INFO;
    serverPost(dict, &NetworkManager::uploadNext);
}


bool NetworkManager::serverSupportsValidatePatients() const
{
    return m_app.serverVersion() >= MIN_SERVER_VERSION_FOR_VALIDATE_PATIENTS;
}


void NetworkManager::uploadValidatePatients()
{
    // Added in v2.3.0
    statusMessage(tr("Validating patients for upload"));
    Dict dict;
    dict[KEY_OPERATION] = OP_VALIDATE_PATIENTS;
    dict[KEY_PATIENT_INFO] = m_upload_patient_info_json;
    serverPost(dict, &NetworkManager::uploadNext);
}


void NetworkManager::uploadFetchAllowedTables()
{
    statusMessage(tr("Fetching server's allowed tables/client versions"));
    Dict dict;
    dict[KEY_OPERATION] = OP_GET_ALLOWED_TABLES;
    serverPost(dict, &NetworkManager::uploadNext);
}


void NetworkManager::startUpload()
{
    statusMessage(tr("Starting upload"));
    Dict dict;
    dict[KEY_OPERATION] = OP_START_UPLOAD;
    serverPost(dict, &NetworkManager::uploadNext);
}


void NetworkManager::startPreservation()
{
    statusMessage(tr("Starting preservation"));
    Dict dict;
    dict[KEY_OPERATION] = OP_START_PRESERVATION;
    serverPost(dict, &NetworkManager::uploadNext);
}


void NetworkManager::sendEmptyTables(const QStringList& tablenames)
{
    statusMessage(tr("Uploading empty tables: ") + tablenames.join(", "));
    Dict dict;
    dict[KEY_OPERATION] = OP_UPLOAD_EMPTY_TABLES;
    dict[KEY_TABLES] = tablenames.join(",");
    serverPost(dict, &NetworkManager::uploadNext);
}


void NetworkManager::sendTableWhole(const QString& tablename)
{
    statusMessage(tr("Uploading table: ") + tablename);
    Dict dict;
    dict[KEY_OPERATION] = OP_UPLOAD_TABLE;
    dict[KEY_TABLE] = tablename;
    const QStringList fieldnames = m_db.getFieldNames(tablename);
    dict[KEY_PKNAME] = dbconst::PK_FIELDNAME;  // version 2.0.4
    // There was a BUG here before v2.0.4:
    // - the old Titanium code gave fieldnames starting with the PK
    // - the SQLite reporting order isn't necessarily like that
    // - for the upload_table command, the receiving code relied on the PK
    //   being first
    // - So as of tablet v2.0.4, the client explicitly reports PK name (and
    //   makes no guarantee about field order) and as of server v2.1.0, the
    //   server takes the PK name if the tablet is >=2.0.4, or "id" otherwise
    //   (because the client PK name always was "id"!). This allows old tablets
    //   to work (for which: could use fieldnames[0] or "id") and early buggy
    //   C++ clients to work (for which: "id" is the only valid option).
    dict[KEY_FIELDS] = fieldnames.join(",");
    const QString sql = dbfunc::selectColumns(fieldnames, tablename);
    const QueryResult result = m_db.query(sql);
    if (!result.succeeded()) {
        queryFail(sql);
        return;
    }
    const int nrows = result.nRows();
    for (int record = 0; record < nrows; ++record) {
        dict[KEYSPEC_RECORD.arg(record)] = result.csvRow(record);
    }
    dict[KEY_NRECORDS] = QString::number(nrows);
    serverPost(dict, &NetworkManager::uploadNext);
}


void NetworkManager::sendTableRecordwise(const QString& tablename)
{
    statusMessage(tr("Preparing to send table (recordwise): ") + tablename);

    m_upload_recordwise_table_in_progress = tablename;
    m_upload_recordwise_fieldnames = m_db.getFieldNames(tablename);
    m_recordwise_prune_req_sent = false;
    m_recordwise_pks_pruned = false;
    m_upload_recordwise_pks_to_send = m_db.getPKs(tablename,
                                                  dbconst::PK_FIELDNAME);
    m_upload_n_records = m_upload_recordwise_pks_to_send.size();
    m_upload_current_record_index = 0;

    // First, DELETE WHERE pk NOT...
    const QString pkvalues = convert::numericVectorToCsvString(m_upload_recordwise_pks_to_send);
    Dict dict;
    dict[KEY_OPERATION] = OP_DELETE_WHERE_KEY_NOT;
    dict[KEY_TABLE] = tablename;
    dict[KEY_PKNAME] = dbconst::PK_FIELDNAME;
    dict[KEY_PKVALUES] = pkvalues;
    statusMessage(tr("Sending message: ") + OP_DELETE_WHERE_KEY_NOT);
    serverPost(dict, &NetworkManager::uploadNext);
}


void NetworkManager::requestRecordwisePkPrune()
{
    const QString sql = QString("SELECT %1, %2, %3 FROM %4")
            .arg(delimit(dbconst::PK_FIELDNAME),  // result column 0
                 delimit(dbconst::MODIFICATION_TIMESTAMP_FIELDNAME),  // result column 1
                 delimit(dbconst::MOVE_OFF_TABLET_FIELDNAME),  // result column 2
                 delimit(m_upload_recordwise_table_in_progress));
    const QueryResult result = m_db.query(sql);
    const QStringList pkvalues = result.columnAsStringList(0);
    const QStringList datevalues = result.columnAsStringList(1);
    const QStringList move_off_tablet_values = result.columnAsStringList(2);
    Dict dict;
    dict[KEY_OPERATION] = OP_WHICH_KEYS_TO_SEND;
    dict[KEY_TABLE] = m_upload_recordwise_table_in_progress;
    dict[KEY_PKNAME] = dbconst::PK_FIELDNAME;
    dict[KEY_PKVALUES] = pkvalues.join(",");
    dict[KEY_DATEVALUES] = datevalues.join(",");
    dict[KEY_MOVE_OFF_TABLET_VALUES] = move_off_tablet_values.join(",");  // v2.3.0
    m_recordwise_prune_req_sent = true;
    statusMessage(tr("Sending message: ") + OP_WHICH_KEYS_TO_SEND);
    serverPost(dict, &NetworkManager::uploadNext);
}


void NetworkManager::sendNextRecord()
{
    ++m_upload_current_record_index;
    statusMessage(tr("Uploading table %1, record %2/%3")
                  .arg(m_upload_recordwise_table_in_progress)
                  .arg(m_upload_current_record_index)
                  .arg(m_upload_n_records));
    // Don't use m_upload_recordwise_pks_to_send.size() as the count, as that
    // changes during upload.
    const int pk = m_upload_recordwise_pks_to_send.front();
    m_upload_recordwise_pks_to_send.pop_front();

    SqlArgs sqlargs(dbfunc::selectColumns(
                        m_upload_recordwise_fieldnames,
                        m_upload_recordwise_table_in_progress));
    WhereConditions where;
    where.add(dbconst::PK_FIELDNAME, pk);
    where.appendWhereClauseTo(sqlargs);
    const QueryResult result = m_db.query(sqlargs, QueryResult::FetchMode::FetchFirst);
    if (!result.succeeded() || result.nRows() < 1) {
        queryFail(sqlargs.sql);
        return;
    }
    const QString values = result.csvRow(0);

    Dict dict;
    dict[KEY_OPERATION] = OP_UPLOAD_RECORD;
    dict[KEY_TABLE] = m_upload_recordwise_table_in_progress;
    dict[KEY_FIELDS] = m_upload_recordwise_fieldnames.join(",");
    dict[KEY_PKNAME] = dbconst::PK_FIELDNAME;
    dict[KEY_VALUES] = values;
    serverPost(dict, &NetworkManager::uploadNext);
}


void NetworkManager::endUpload()
{
    statusMessage(tr("Finishing upload"));
    Dict dict;
    dict[KEY_OPERATION] = OP_END_UPLOAD;
    serverPost(dict, &NetworkManager::uploadNext);
}


// ----------------------------------------------------------------------------
// Upload: INTERNAL FUNCTIONS
// ----------------------------------------------------------------------------

bool NetworkManager::isPatientInfoComplete()
{
    statusMessage(tr("Checking patient information sufficiently complete"));

    Patient specimen_patient(m_app, m_db);
    const SqlArgs sqlargs = specimen_patient.fetchQuerySql();
    const QueryResult result = m_db.query(sqlargs);
    if (!result.succeeded()) {
        queryFail(sqlargs.sql);
        return false;
    }

    const bool finalizing = m_upload_method != UploadMethod::Copy;
    int nfailures_upload = 0;
    int nfailures_finalize = 0;
    int nfailures_clash = 0;
    int nfailures_move_off = 0;
    QJsonArray patients_json_array;
    const int nrows = result.nRows();
    for (int row = 0; row < nrows; ++row) {
        Patient patient(m_app, m_db);
        patient.setFromQuery(result, row, true);
        const bool finalizing_this_pt = patient.shouldMoveOffTablet();
        if (!patient.compliesWithUpload()) {
            ++nfailures_upload;
        }
        const bool complies_with_finalize = patient.compliesWithFinalize();
        if (!complies_with_finalize) {
            ++nfailures_finalize;
        }
        if (patient.anyIdClash()) {
            // not the most efficient; COUNT DISTINCT...
            // However, this gives us the number of patients clashing.
            ++nfailures_clash;
        }
        if (m_upload_method != UploadMethod::Move && finalizing_this_pt) {
            // To move a patient off, it must comply with the finalize policy.
            if (!complies_with_finalize) {
                ++nfailures_move_off;
            } else {
                m_upload_patient_ids_to_move_off.append(patient.pkvalue().toInt());
            }
        }

        // Set JSON too. See below.
        QJsonObject ptjson = patient.jsonDescription();
        ptjson[KEY_FINALIZING] = finalizing || finalizing_this_pt;
        patients_json_array.append(ptjson);
    }

    if (nfailures_clash > 0) {
        statusMessage(tr("Failure: %1 patient(s) having clashing ID numbers")
                      .arg(nfailures_clash));
        return false;
    }
    if (nfailures_move_off > 0) {
        statusMessage(tr(
                "You are trying to move off %1 patient(s) using the "
                "explicit per-patient move-off flag, but they do not "
                "comply with the server's finalize ID policy [%2]")
                      .arg(nfailures_move_off)
                      .arg(m_app.finalizePolicy().pretty()));
        return false;
    }
    if (m_upload_method == UploadMethod::Copy && nfailures_upload > 0) {
        // Copying; we're allowed not to meet the finalizing requirements,
        // but we must meet the uploading requirements
        statusMessage(tr("Failure: %1 patient(s) do not meet the "
                         "server's upload ID policy of: %2")
                      .arg(nfailures_upload)
                      .arg(m_app.uploadPolicy().pretty()));
        return false;
    }
    if (finalizing && nfailures_upload + nfailures_finalize > 0) {
        // Finalizing; must meet all requirements
        statusMessage(tr(
            "Failure: %1 patient(s) do not meet the server's upload ID policy "
            "[%2]; %3 patient(s) do not meet its finalize ID policy [%4]")
                      .arg(nfailures_upload)
                      .arg(m_app.uploadPolicy().pretty())
                      .arg(nfailures_finalize)
                      .arg(m_app.finalizePolicy().pretty()));
        return false;
    }

    // We also set the patient info JSON here, so we only iterate through
    // patients once.
    //
    // Compare camcops_server.cc_modules.client_api.validate_patients() on the
    // server.
    //
    // Top-level JSON can be an object or an array.
    // - https://stackoverflow.com/questions/3833299/can-an-array-be-top-level-json-text
    // - http://www.ietf.org/rfc/rfc4627.txt?number=4627
    const QJsonDocument jsondoc(patients_json_array);
    m_upload_patient_info_json  = jsondoc.toJson(QJsonDocument::Compact);
    //                                    ^^^^^^ ... a QByteArray in UTF-8
    // - https://stackoverflow.com/questions/28181627/how-to-convert-a-qjsonobject-to-qstring
#ifdef DEBUG_JSON
    qDebug().noquote() << "Patient info JSON:" << m_upload_patient_info_json;
#endif

    return true;
}


bool NetworkManager::applyPatientMoveOffTabletFlagsToTasks()
{
    // If we were uploading, we need to undo our move-off flags (in case the
    // user changes their mind about a patient)
    // We could use a system of "set before upload, clear afterwards".
    // However, failing to clear (for some reason) is a risk.
    // Therefore, we set and clear flags here, for all tables.
    // That is, we make sure these flags are all correct immediately before
    // an upload (which is when we care).

    if (m_upload_method != UploadMethod::Copy) {
        // if we're not using UploadMethod::Copy, everything is going to be
        // moved anyway, by virtue of startPreservation()
        statusMessage(tr("... not applicable; all tasks will be moved"));
        return true;
    }

    DbNestableTransaction trans(m_db);

    // ========================================================================
    // Step 1: clear all move-off flags, except in the source tables (being:
    // patient tables and anonymous task primary tables).
    // ========================================================================
    for (const TaskPtr& specimen : m_p_task_factory->allSpecimens()) {
        if (specimen->isAnonymous()) {
            // anonymous task: clear the ancillary tables
            for (const QString& tablename : specimen->ancillaryTables()) {
                if (!clearMoveOffTabletFlag(tablename)) {
                    queryFailClearingMoveOffFlag(tablename);
                    return false;
                }
            }
        } else {
            // task with patient: clear all tables
            for (const QString& tablename : specimen->allTables()) {
                if (!clearMoveOffTabletFlag(tablename)) {
                    queryFailClearingMoveOffFlag(tablename);
                    return false;
                }
            }
        }
    }
    // Clear all flags for BLOBs
    if (!clearMoveOffTabletFlag(Blob::TABLENAME)) {
        queryFailClearingMoveOffFlag(Blob::TABLENAME);
        return false;
    }

    // ========================================================================
    // Step 2: Apply flags from patients to their idnums/tasks/ancillary tables.
    // ========================================================================
    // m_upload_patient_ids_to_move_off has been precalculated for efficiency

    const int n_patients = m_upload_patient_ids_to_move_off.length();
    if (n_patients > 0) {
        QString pt_paramholders = dbfunc::sqlParamHolders(n_patients);
        ArgList pt_args = dbfunc::argListFromIntList(m_upload_patient_ids_to_move_off);
        // Maximum length of an SQL statement: lots
        // https://www.sqlite.org/limits.html
        QString sql;

        // Patient ID number table
        sql = QString("UPDATE %1 SET %2 = 1 WHERE %3 IN (%4)")
                      .arg(delimit(PatientIdNum::PATIENT_IDNUM_TABLENAME),
                           delimit(dbconst::MOVE_OFF_TABLET_FIELDNAME),
                           delimit(PatientIdNum::FK_PATIENT),
                           pt_paramholders);
#ifdef USE_BACKGROUND_DATABASE
            m_db.execNoAnswer(sql, pt_args);
#else
            if (!m_db.exec(sql, pt_args)) {
                queryFail(sql);
                return false;
            }
#endif

        // Task tables
        for (const TaskPtr& specimen : m_p_task_factory->allSpecimens()) {
            if (specimen->isAnonymous()) {
                continue;
            }
            const QString main_tablename = specimen->tablename();
            // (a) main table, with FK to patient
            sql = QString("UPDATE %1 SET %2 = 1 WHERE %3 IN (%4)")
                    .arg(delimit(main_tablename),
                         delimit(dbconst::MOVE_OFF_TABLET_FIELDNAME),
                         delimit(Task::PATIENT_FK_FIELDNAME),
                         pt_paramholders);
#ifdef USE_BACKGROUND_DATABASE
            m_db.execNoAnswer(sql, pt_args);
#else
            if (!m_db.exec(sql, pt_args)) {
                queryFail(sql);
                return false;
            }
#endif
            // (b) ancillary tables
            const QStringList ancillary_tables = specimen->ancillaryTables();
            if (ancillary_tables.isEmpty()) {
                // no ancillary tables
                continue;
            }
            WhereConditions where;
            where.add(dbconst::MOVE_OFF_TABLET_FIELDNAME, 1);
            const QVector<int> task_pks = m_db.getSingleFieldAsIntList(
                        main_tablename, dbconst::PK_FIELDNAME, where);
            if (task_pks.isEmpty()) {
                // no tasks to be moved off
                continue;
            }
            const QString fk_task_fieldname = specimen->ancillaryTableFKToTaskFieldname();
            if (fk_task_fieldname.isEmpty()) {
                uifunc::stopApp(QString(
                    "Task %1 has ancillary tables but "
                    "ancillaryTableFKToTaskFieldname() returns empty")
                                .arg(main_tablename));
            }
            const QString task_paramholders = dbfunc::sqlParamHolders(task_pks.length());
            const ArgList task_args = dbfunc::argListFromIntList(task_pks);
            for (const QString& ancillary_table : ancillary_tables) {
                sql = QString("UPDATE %1 SET %2 = 1 WHERE %3 IN (%4)")
                        .arg(delimit(ancillary_table),
                             delimit(dbconst::MOVE_OFF_TABLET_FIELDNAME),
                             delimit(fk_task_fieldname),
                             task_paramholders);
#ifdef USE_BACKGROUND_DATABASE
                m_db.execNoAnswer(sql, task_args);
#else
                if (!m_db.exec(sql, task_args)) {
                    queryFail(sql);
                    return false;
                }
#endif
            }
        }
    }

    // ========================================================================
    // Step 3: Apply flags from anonymous tasks to their ancillary tables.
    // ========================================================================

    for (const TaskPtr& specimen : m_p_task_factory->allSpecimens()) {
        if (!specimen->isAnonymous()) {
            continue;
        }
        const QString main_tablename = specimen->tablename();
        const QStringList ancillary_tables = specimen->ancillaryTables();
        if (ancillary_tables.isEmpty()) {
            continue;
        }
        // Get PKs of all anonymous tasks being moved off
        WhereConditions where;
        where.add(dbconst::MOVE_OFF_TABLET_FIELDNAME, 1);
        const QVector<int> task_pks = m_db.getSingleFieldAsIntList(
                    main_tablename, dbconst::PK_FIELDNAME, where);
        if (task_pks.isEmpty()) {
            // no tasks to be moved off
            continue;
        }
        const QString fk_task_fieldname = specimen->ancillaryTableFKToTaskFieldname();
        if (fk_task_fieldname.isEmpty()) {
            uifunc::stopApp(QString(
                "Task %1 has ancillary tables but "
                "ancillaryTableFKToTaskFieldname() returns empty")
                            .arg(main_tablename));
        }
        const QString task_paramholders = dbfunc::sqlParamHolders(task_pks.length());
        const ArgList task_args = dbfunc::argListFromIntList(task_pks);
        for (const QString& ancillary_table : ancillary_tables) {
            QString sql = QString("UPDATE %1 SET %2 = 1 WHERE %3 IN (%4)")
                    .arg(delimit(ancillary_table),
                         delimit(dbconst::MOVE_OFF_TABLET_FIELDNAME),
                         delimit(fk_task_fieldname),
                         task_paramholders);
#ifdef USE_BACKGROUND_DATABASE
            m_db.execNoAnswer(sql, task_args);
#else
            if (!m_db.exec(sql, task_args)) {
                queryFail(sql);
                return false;
            }
#endif
        }
    }

    // ========================================================================
    // Step 4. BLOB table.
    // ========================================================================
    // Options here are:
    // - iterate through every task (and ancillary table), loading them from
    //   SQL to C++, and asking each what BLOB IDs they possess;
    // - store patient_id (or NULL) with each BLOB;
    // - iterate through each BLOB, looking for the move-off flag on the
    //   associated task/ancillary record.
    // The most efficient and simple is likely to be (3).

    // (a) For every BLOB...
    const QString sql = dbfunc::selectColumns(
                QStringList{dbconst::PK_FIELDNAME,
                            Blob::SRC_TABLE_FIELDNAME,
                            Blob::SRC_PK_FIELDNAME},
                Blob::TABLENAME);
    const QueryResult result = m_db.query(sql);
    if (!result.succeeded()) {
        queryFail(sql);
        return false;
    }
    const int nrows = result.nRows();
    for (int row = 0; row < nrows; ++row) {
        // (b) find the table/PK of the linked task (or other table)
        const int blob_pk = result.at(row, 0).toInt();
        const QString src_table = result.at(row, 1).toString();
        const int src_pk = result.at(row, 2).toInt();

        // (c) find the move-off flag for that linked task
        SqlArgs sub1_sqlargs(
                    dbfunc::selectColumns(
                        QStringList{dbconst::MOVE_OFF_TABLET_FIELDNAME},
                        src_table));
        WhereConditions sub1_where;
        sub1_where.add(dbconst::PK_FIELDNAME, src_pk);
        sub1_where.appendWhereClauseTo(sub1_sqlargs);
        const int move_off_int = m_db.fetchInt(sub1_sqlargs, -1);
        if (move_off_int == -1) {
            // No records matching
            qWarning().nospace()
                    << "BLOB refers to "
                    << src_table
                    << "."
                    << dbconst::PK_FIELDNAME
                    << " = "
                    << src_pk
                    << " but record doesn't exist!";
            continue;
        }
        if (move_off_int == 0) {
            // Record exists; task not marked for move-off
            continue;
        }

        // (d) set the BLOB's move-off flag
        const UpdateValues update_values{{dbconst::MOVE_OFF_TABLET_FIELDNAME, true}};
        SqlArgs sub2_sqlargs = dbfunc::updateColumns(update_values, Blob::TABLENAME);
        WhereConditions sub2_where;
        sub2_where.add(dbconst::PK_FIELDNAME, blob_pk);
        sub2_where.appendWhereClauseTo(sub2_sqlargs);
#ifdef USE_BACKGROUND_DATABASE
        m_db.execNoAnswer(sub2_sqlargs);
#else
        if (!m_db.exec(sub2_sqlargs)) {
            queryFail(sub2_sqlargs.sql);
            return false;
        }
#endif
    }
    return true;
}


bool NetworkManager::catalogueTablesForUpload()
{
    statusMessage(tr("Cataloguing tables for upload"));
    const QStringList recordwise_tables{Blob::TABLENAME};
    const QStringList patient_tables{Patient::TABLENAME,
                                     PatientIdNum::PATIENT_IDNUM_TABLENAME};
    const QStringList all_tables = m_db.getAllTables();
    const Version server_version = m_app.serverVersion();
    bool may_upload;
    bool server_has_table;  // table present on server
    Version min_client_version;  // server's requirement for min client version
    Version min_server_version;  // client's requirement for min server version
    for (const QString& table : all_tables) {
        const int n_records = m_db.count(table);
        may_upload = m_app.mayUploadTable(
                    table, server_version,
                    server_has_table, min_client_version, min_server_version);
        if (!may_upload) {
            if (server_has_table) {
                // This table requires a newer client than we are, OR we
                // require a newer server than it is.
                // If the table is empty, proceed. Otherwise, fail.
                if (server_version < min_server_version) {
                    if (n_records != 0) {
                        statusMessage(tr(
                            "ERROR: Table '%1' contains data; it is present "
                            "on the server but the client requires server "
                            "version >=%2; the server is version %3"
                        ).arg(table, min_server_version.toString(),
                              server_version.toString()));
                        return false;
                    }
                    statusMessage(tr(
                        "WARNING: Table '%1' is present on the server but "
                        "the client requires server version >=%2; the "
                        "server is version %3; proceeding ONLY BECAUSE "
                        "THIS TABLE IS EMPTY."
                    ).arg(table, min_server_version.toString(),
                          server_version.toString()));
                } else {
                    if (n_records != 0) {
                        statusMessage(tr(
                            "ERROR: Table '%1' contains data; it is present "
                            "on the server but the server requires client "
                            "version >=%2; you are using version %3"
                        ).arg(table, min_client_version.toString(),
                              camcopsversion::CAMCOPS_CLIENT_VERSION.toString()));
                        return false;
                    }
                    statusMessage(tr(
                        "WARNING: Table '%1' is present on the server but "
                        "the server requires client version >=%2; you are "
                        "using version %3; proceeding ONLY BECAUSE THIS "
                        "TABLE IS EMPTY."
                    ).arg(table, min_client_version.toString(),
                          camcopsversion::CAMCOPS_CLIENT_VERSION.toString()));
                }
            } else {
                // The table isn't on the server.
                if (n_records != 0) {
                    statusMessage(tr(
                        "ERROR: Table '%1' contains data but is absent on the "
                        "server. You probably need a newer server version. "
                        "(Once you have upgraded the server, re-register with "
                        "it.)").arg(table));
                    return false;
                }
                statusMessage(tr(
                    "WARNING: Table '%1' is absent on the server. You "
                    "probably need a newer server version. (Once you have "
                    "upgraded the server, re-register with it.) "
                    "Proceeding ONLY BECAUSE THIS TABLE IS EMPTY."
                ).arg(table));
            }
        }
        // How to upload?
        if (n_records == 0) {
            if (may_upload) {
                m_upload_empty_tables.append(table);
            }
        } else if (recordwise_tables.contains(table)) {
            m_upload_tables_to_send_recordwise.append(table);
        } else {
            m_upload_tables_to_send_whole.append(table);
        }

        // Whether to clear afterwards?
        // (Note that if we get here and may_upload is false, it must be the
        // case that the table is empty, in which case it doesn't matter
        // whether we clear it or not.)
        switch (m_upload_method) {
        case UploadMethod::Copy:
#ifdef COMPILER_WANTS_DEFAULT_IN_EXHAUSTIVE_SWITCH
        default:
#endif
            break;
        case UploadMethod::MoveKeepingPatients:
            if (!patient_tables.contains(table)) {
                m_upload_tables_to_wipe.append(table);
            }
            break;
        case UploadMethod::Move:
            m_upload_tables_to_wipe.append(table);
            break;
        }
    }
    return true;
}


bool NetworkManager::isServerVersionOK() const
{
    statusMessage(tr("Checking server CamCOPS version"));
    const QString server_version_str = m_reply_dict[KEY_SERVER_CAMCOPS_VERSION];
    const Version server_version(server_version_str);
    const Version stored_server_version = m_app.serverVersion();

    if (server_version < camcopsversion::MINIMUM_SERVER_VERSION) {
        statusMessage(tr("Server CamCOPS version (%1) is too old; must be >= %2")
                      .arg(server_version_str,
                           camcopsversion::MINIMUM_SERVER_VERSION.toString()));
        return false;
    }
    if (server_version != stored_server_version) {
        statusMessage(tr("Server version (%1) doesn't match stored version (%2).")
                      .arg(server_version.toString(),
                           stored_server_version.toString()) +
                      txtPleaseRefetchServerInfo());
        return false;
    }
    statusMessage(tr("... OK"));
    return true;
}


bool NetworkManager::arePoliciesOK() const
{
    statusMessage(tr("Checking ID policies match server"));
    const QString local_upload = m_app.uploadPolicy().pretty();
    const QString local_finalize = m_app.finalizePolicy().pretty();
    const QString server_upload = IdPolicy(m_reply_dict[KEY_ID_POLICY_UPLOAD]).pretty();
    const QString server_finalize = IdPolicy(m_reply_dict[KEY_ID_POLICY_FINALIZE]).pretty();
    bool ok = true;
    if (local_upload != server_upload) {
        statusMessage(tr("Local upload policy [%1] doesn't match server's [%2].")
                      .arg(local_upload, server_upload) +
                      txtPleaseRefetchServerInfo());
        ok = false;
    }
    if (local_finalize != server_finalize) {
        statusMessage(tr("Local finalize policy [%1] doesn't match server's [%2].")
                      .arg(local_finalize, server_finalize) +
                      txtPleaseRefetchServerInfo());
        ok = false;
    }
    if (ok) {
        statusMessage(tr("... OK"));
    }
    return ok;
}


bool NetworkManager::areDescriptionsOK() const
{
    statusMessage(tr("Checking ID descriptions match server"));
    bool idnums_all_on_server = true;
    bool descriptions_match = true;
    QVector<int> which_idnums_on_server;
    QVector<IdNumDescriptionPtr> iddescriptions = m_app.getAllIdDescriptions();
    for (const IdNumDescriptionPtr& iddesc : iddescriptions) {
        const int n = iddesc->whichIdNum();
        const QString key_desc = KEYSPEC_ID_DESCRIPTION.arg(n);
        const QString key_shortdesc = KEYSPEC_ID_SHORT_DESCRIPTION.arg(n);
        const QString key_validation = KEYSPEC_ID_VALIDATION_METHOD.arg(n);
        if (m_reply_dict.contains(key_desc) &&
                m_reply_dict.contains(key_shortdesc)) {
            const QString local_desc = iddesc->description();
            const QString local_shortdesc = iddesc->shortDescription();
            const QString server_desc = m_reply_dict[key_desc];
            const QString server_shortdesc = m_reply_dict[key_shortdesc];
            descriptions_match = descriptions_match &&
                    local_desc == server_desc &&
                    local_shortdesc == server_shortdesc;
            which_idnums_on_server.append(n);
            // Old servers may not provide the ID number validator info.
            // But new ones will (v2.2.8+), in which case we'll check.
            if (m_reply_dict.contains(key_validation)) {
                const QString local_validation = iddesc->validationMethod();
                const QString server_validation = m_reply_dict[key_validation];
                descriptions_match = descriptions_match &&
                        local_validation == server_validation;
            }
        } else {
            idnums_all_on_server = false;
        }
    }
    QVector<int> which_idnums_on_tablet = whichIdnumsUsedOnTablet();
    QVector<int> extra_idnums_on_tablet = containers::setSubtract(
                which_idnums_on_tablet, which_idnums_on_server);
    const bool extra_idnums = !extra_idnums_on_tablet.isEmpty();

    const bool ok = descriptions_match && idnums_all_on_server && !extra_idnums;
    if (ok) {
        statusMessage(tr("... OK"));
    } else if (!idnums_all_on_server) {
        statusMessage(tr("Some ID numbers defined on the tablet are absent on "
                         "the server!") + txtPleaseRefetchServerInfo());
    } else if (!descriptions_match) {
        statusMessage(tr("Descriptions do not match!") + txtPleaseRefetchServerInfo());
    } else if (extra_idnums) {
        statusMessage(tr(
                "ID numbers %1 are used on the tablet but not defined "
                "on the server! Please edit patient records to remove "
                "them.").arg(convert::numericVectorToCsvString(
                                 extra_idnums_on_tablet)));
    } else {
        statusMessage("Logic bug: something not OK but don't know why");
    }
    return ok;
}


QVector<int> NetworkManager::whichIdnumsUsedOnTablet() const
{
    const QString sql = QString("SELECT DISTINCT %1 FROM %2 ORDER BY %1")
            .arg(delimit(PatientIdNum::FN_WHICH_IDNUM),
                 delimit(PatientIdNum::PATIENT_IDNUM_TABLENAME));
    const QueryResult result = m_db.query(sql);
    return result.firstColumnAsIntList();
}


bool NetworkManager::pruneRecordwisePks()
{
    if (!m_reply_dict.contains(KEY_RESULT)) {
        statusMessage(tr("Server's reply was missing the key: ") + KEY_RESULT);
        return false;
    }
    const QString reply = m_reply_dict[KEY_RESULT];
    statusMessage(tr("Server requests only PKs: ") + reply);
    m_upload_recordwise_pks_to_send = convert::csvStringToIntVector(reply);
    m_upload_n_records = m_upload_recordwise_pks_to_send.size();
    m_recordwise_pks_pruned = true;
    return true;
}


void NetworkManager::wipeTables()
{
    DbNestableTransaction trans(m_db);

    // Plain wipes, of entire tables
    for (const QString& wipe_table : m_upload_tables_to_wipe) {
        // Note: m_upload_tables_to_wipe will contain the patient table if
        // we're moving everything; see catalogueTablesForUpload()
        statusMessage(tr("Wiping table: ") + wipe_table);
        if (!m_db.deleteFrom(wipe_table)) {
            statusMessage(tr("... failed to delete!"));
            trans.fail();
            fail();
        }
    }

    // Selective wipes: tasks, patients, ancillary tables...
    // - We wipe: (a) records in tasks whose patient record was marked for
    //   moving (and whose _move_off_tablet field was propagated through to the
    //   task, as above); (b) any anonymous tasks specifically marked for
    //   moving; (c) any ancillary tasks of the above.
    // - The simplest way is to go through ALL tables (task + ancillary +
    //   patient + patient ID...) and delete records for which
    //   "_move_off_tablet" is set (skipping any tables we've already wiped
    //   completely, for speed).
    if (m_upload_method != UploadMethod::Move) {
        // ... if we were doing a Move, *everything* has gone
        statusMessage(tr("Wiping any specifically requested patients and/or anonymous tasks"));
        WhereConditions where_move_off;
        where_move_off.add(dbconst::MOVE_OFF_TABLET_FIELDNAME, 1);

        const QStringList all_tables = m_db.getAllTables();
        for (const QString& tablename : all_tables) {
            if (m_upload_tables_to_wipe.contains(tablename)) {
                continue;  // Already totally wiped
            }
            m_db.deleteFrom(tablename, where_move_off);
        }
    }
}


void NetworkManager::queryFail(const QString &sql)
{
    statusMessage(tr("Query failed: ") + sql);
    fail();
}


void NetworkManager::queryFailClearingMoveOffFlag(const QString& tablename)
{
    queryFail(tr("... trying to clear move-off-tablet flag for table: ") +
              tablename);
}


bool NetworkManager::clearMoveOffTabletFlag(const QString& tablename)
{
    // 1. Clear all
    const QString sql = QString("UPDATE %1 SET %2 = 0")
            .arg(delimit(tablename),
                 delimit(dbconst::MOVE_OFF_TABLET_FIELDNAME));
#ifdef USE_BACKGROUND_DATABASE
    m_db.execNoAnswer(sql);
    return true;
#else
    return m_db.exec(sql);
#endif
}


bool NetworkManager::pruneDeadBlobs()
{
    using dbfunc::delimit;

    const QStringList all_tables = m_db.getAllTables();
    QVector<int> bad_blob_pks;

    // For all BLOBs...
    QString sql = dbfunc::selectColumns(
                QStringList{dbconst::PK_FIELDNAME,
                            Blob::SRC_TABLE_FIELDNAME,
                            Blob::SRC_PK_FIELDNAME},
                Blob::TABLENAME);
    const QueryResult result = m_db.query(sql);
    if (!result.succeeded()) {
        queryFail(sql);
        return false;
    }
    const int nrows = result.nRows();
    for (int row = 0; row < nrows; ++row) {
        const int blob_pk = result.at(row, 0).toInt();
        const QString src_table = result.at(row, 1).toString();
        const int src_pk = result.at(row, 2).toInt();
        if (src_pk == dbconst::NONEXISTENT_PK) {
            continue;
        }
        // Does our BLOB refer to something non-existent?
        if (!all_tables.contains(src_table) ||
                !m_db.existsByPk(src_table, dbconst::PK_FIELDNAME, src_pk)) {
            bad_blob_pks.append(blob_pk);
        }
    }

    const int n_bad_blobs = bad_blob_pks.length();
    statusMessage(tr("... %1 defunct BLOBs").arg(n_bad_blobs));
    if (n_bad_blobs == 0) {
        return true;
    }

    qWarning() << "Deleting defunct BLOBs with PKs:" << bad_blob_pks;
    const QString paramholders = dbfunc::sqlParamHolders(n_bad_blobs);
    sql = QString("DELETE FROM %1 WHERE %2 IN (%3)")
            .arg(delimit(Blob::TABLENAME),
                 delimit(dbconst::PK_FIELDNAME),
                 paramholders);
    ArgList args = dbfunc::argListFromIntList(bad_blob_pks);
#ifdef USE_BACKGROUND_DATABASE
    m_db.execNoAnswer(sql, args);
#else
    if (!m_db.exec(sql, args)) {
        queryFail(sql);
        return false;
    }
#endif
    return true;
}


// ============================================================================
// One-step upload
// ============================================================================

bool NetworkManager::serverSupportsOneStepUpload() const
{
    return m_app.serverVersion() >= MIN_SERVER_VERSION_FOR_ONE_STEP_UPLOAD;
}


bool NetworkManager::shouldUseOneStepUpload() const
{
    if (!serverSupportsOneStepUpload()) {
        return false;
    }
    const int method = m_app.varInt(varconst::UPLOAD_METHOD);
    // Can't use switch; const int is not const enough.
    // Can't use enums; have to store in an int field.
    if (method == varconst::UPLOAD_METHOD_ONESTEP) {
        return true;
    } else if (method == varconst::UPLOAD_METHOD_BYSIZE) {
        return m_db.approximateDatabaseSize() <= m_app.varLongLong(
                    varconst::MAX_DBSIZE_FOR_ONESTEP_UPLOAD);
    } else {
        // e.g. varconst::UPLOAD_METHOD_MULTISTEP or bad value
        return false;
    }
}


void NetworkManager::uploadOneStep()
{
    statusMessage(tr("Starting one-step upload"));
    const bool preserving = m_upload_method != UploadMethod::Copy;
    Dict dict;
    dict[KEY_OPERATION] = OP_UPLOAD_ENTIRE_DATABASE;
    dict[KEY_FINALIZING] = preserving ? ENCODE_TRUE : ENCODE_FALSE;
    dict[KEY_PKNAMEINFO] = getPkInfoAsJson();
    dict[KEY_DBDATA] = m_db.getDatabaseAsJson();
#ifdef DEBUG_JSON
    qDebug().noquote() << Q_FUNC_INFO << dict[KEY_DBDATA];
#endif
    serverPost(dict, &NetworkManager::uploadNext);
}


QString NetworkManager::getPkInfoAsJson()
{
    QJsonObject root;
    for (const QString& tablename : m_db.getAllTables()) {
        root[tablename] = dbconst::PK_FIELDNAME;  // they're all the same...
    }
    const QJsonDocument jsondoc(root);
    return jsondoc.toJson(QJsonDocument::Compact);
}


// ============================================================================
// Translatable text
// ============================================================================

QString NetworkManager::txtPleaseRefetchServerInfo()
{
    // return " " + tr("Please re-register with the server.");
    return " " + tr("Please re-fetch server information.");
}
