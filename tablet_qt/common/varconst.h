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
#include <QString>
#include "common/design_defines.h"

namespace varconst {

// ----------------------------------------------------------------------------
// Names of server variables, and a few associated constants.
// ----------------------------------------------------------------------------

// Language
extern const QString LANGUAGE;

// Version storage
extern const QString CAMCOPS_TABLET_VERSION_AS_STRING;

// Questionnaire
extern const QString QUESTIONNAIRE_SIZE_PERCENT;
extern const QString OVERRIDE_LOGICAL_DPI;
extern const QString OVERRIDE_LOGICAL_DPI_X;
extern const QString OVERRIDE_LOGICAL_DPI_Y;
extern const QString OVERRIDE_PHYSICAL_DPI;
extern const QString OVERRIDE_PHYSICAL_DPI_X;
extern const QString OVERRIDE_PHYSICAL_DPI_Y;

// Server
extern const QString SERVER_ADDRESS;
extern const QString SERVER_PORT;
extern const QString SERVER_PATH;
extern const QString SERVER_TIMEOUT_MS;
extern const QString VALIDATE_SSL_CERTIFICATES;
extern const QString SSL_PROTOCOL;
extern const QString DEBUG_USE_HTTPS_TO_SERVER;
extern const QString STORE_SERVER_PASSWORD;
extern const QString UPLOAD_METHOD;
extern const QString MAX_DBSIZE_FOR_ONESTEP_UPLOAD;

extern const int UPLOAD_METHOD_MULTISTEP;  // the original way
extern const int UPLOAD_METHOD_ONESTEP;  // available from v2.3.0
extern const int UPLOAD_METHOD_BYSIZE;  // available from v2.3.0
extern const int DEFAULT_UPLOAD_METHOD;
extern const qint64 DEFAULT_MAX_DBSIZE_FOR_ONESTEP_UPLOAD;

// Uploading flag
extern const QString NEEDS_UPLOAD;

// Terms and conditions
extern const QString AGREED_TERMS_AT;

// Intellectual property
extern const QString IP_USE_CLINICAL;
extern const QString IP_USE_COMMERCIAL;
extern const QString IP_USE_EDUCATIONAL;
extern const QString IP_USE_RESEARCH;

// Patients and policies
extern const QString ID_POLICY_UPLOAD;
extern const QString ID_POLICY_FINALIZE;

// Note that ID descriptions/short descriptions have multiple server
// variables as set by
//      DbConst::IDDESC_FIELD_FORMAT
//      DbConst::IDSHORTDESC_FIELD_FORMAT

// Other information from server
extern const QString SERVER_DATABASE_TITLE;
extern const QString SERVER_CAMCOPS_VERSION;
extern const QString LAST_SERVER_REGISTRATION;
extern const QString LAST_SUCCESSFUL_UPLOAD;

// User
// ... server interaction
extern const QString DEVICE_FRIENDLY_NAME;
extern const QString SERVER_USERNAME;
extern const QString SERVER_USERPASSWORD_OBSCURED;
extern const QString OFFER_UPLOAD_AFTER_EDIT;
// ... default clinician details
extern const QString DEFAULT_CLINICIAN_SPECIALTY;
extern const QString DEFAULT_CLINICIAN_NAME;
extern const QString DEFAULT_CLINICIAN_PROFESSIONAL_REGISTRATION;
extern const QString DEFAULT_CLINICIAN_POST;
extern const QString DEFAULT_CLINICIAN_SERVICE;
extern const QString DEFAULT_CLINICIAN_CONTACT_DETAILS;

// Cryptography
extern const QString OBSCURING_KEY;  // for server p/w, which we must retrieve
extern const QString OBSCURING_IV;
extern const QString USER_PASSWORD_HASH;
extern const QString PRIV_PASSWORD_HASH;

// Device ID
extern const QString DEVICE_ID;

}  // namespace varconst
