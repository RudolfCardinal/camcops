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

#include "varconst.h"

namespace varconst {

// Language
const QString LANGUAGE("language");

// Version storage
const QString CAMCOPS_TABLET_VERSION_AS_STRING("camcopsVersion");

// Questionnaire
const QString QUESTIONNAIRE_SIZE_PERCENT("questionnaireTextSizePercent");
const QString OVERRIDE_LOGICAL_DPI("overrideLogicalDpi");
const QString OVERRIDE_LOGICAL_DPI_X("overrideLogicalDpiX");
const QString OVERRIDE_LOGICAL_DPI_Y("overrideLogicalDpiY");
const QString OVERRIDE_PHYSICAL_DPI("overridePhysicalDpi");
const QString OVERRIDE_PHYSICAL_DPI_X("overridePhysicalDpiX");
const QString OVERRIDE_PHYSICAL_DPI_Y("overridePhysicalDpiY");

// Server
const QString SERVER_ADDRESS("serverAddress");
const QString SERVER_PORT("serverPort");
const QString SERVER_PATH("serverPath");
const QString SERVER_TIMEOUT_MS("serverTimeoutMs");
const QString VALIDATE_SSL_CERTIFICATES("validateSSLCertificates");
const QString SSL_PROTOCOL("sslProtocol");
const QString DEBUG_USE_HTTPS_TO_SERVER("debugUseHttpsToServer");
const QString STORE_SERVER_PASSWORD("storeServerPassword");
const QString UPLOAD_METHOD("uploadMethod");
const QString MAX_DBSIZE_FOR_ONESTEP_UPLOAD("maxDbSizeForOneStepUpload");

const int UPLOAD_METHOD_MULTISTEP = 0;
const int UPLOAD_METHOD_ONESTEP = 1;
const int UPLOAD_METHOD_BYSIZE = 2;
const int DEFAULT_UPLOAD_METHOD = UPLOAD_METHOD_BYSIZE;
const qint64 DEFAULT_MAX_DBSIZE_FOR_ONESTEP_UPLOAD = 2000000;  // ~2 Mb

// Uploading flag
const QString NEEDS_UPLOAD("needsUpload");

// Terms and conditions
const QString AGREED_TERMS_AT("agreedTermsOfUseAt");

// Intellectual property
const QString IP_USE_CLINICAL("useClinical");
const QString IP_USE_COMMERCIAL("useCommercial");
const QString IP_USE_EDUCATIONAL("useEducational");
const QString IP_USE_RESEARCH("useResearch");

// Patients and policies
const QString ID_POLICY_UPLOAD("idPolicyUpload");
const QString ID_POLICY_FINALIZE("idPolicyFinalize");

// Other information from server
const QString SERVER_DATABASE_TITLE("databaseTitle");
const QString SERVER_CAMCOPS_VERSION("serverCamcopsVersion");
const QString LAST_SERVER_REGISTRATION("lastServerRegistration");
const QString LAST_SUCCESSFUL_UPLOAD("lastSuccessfulUpload");

// User
const QString DEVICE_FRIENDLY_NAME("deviceFriendlyName");
const QString SERVER_USERNAME("serverUser");
const QString SERVER_USERPASSWORD_OBSCURED("serverPasswordObscured");
const QString DEFAULT_CLINICIAN_SPECIALTY("defaultClinicianSpecialty");
const QString DEFAULT_CLINICIAN_NAME("defaultClinicianName");
const QString DEFAULT_CLINICIAN_PROFESSIONAL_REGISTRATION("defaultClinicianProfessionalRegistration");
const QString DEFAULT_CLINICIAN_POST("defaultClinicianPost");
const QString DEFAULT_CLINICIAN_SERVICE("defaultClinicianService");
const QString DEFAULT_CLINICIAN_CONTACT_DETAILS("defaultClinicianContactDetails");
const QString OFFER_UPLOAD_AFTER_EDIT("offerUploadAfterEdit");

// Cryptography
const QString OBSCURING_KEY("obscuringKey");
const QString OBSCURING_IV("obscuringIV");
const QString USER_PASSWORD_HASH("userPassword");
const QString PRIV_PASSWORD_HASH("privPassword");

// Device ID
const QString DEVICE_ID("deviceId");

}  // namespace varconst
