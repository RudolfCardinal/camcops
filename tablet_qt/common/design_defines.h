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

// ============================================================================
// Stash duplicate copy of ID descriptions in patient table?
// ============================================================================
// Q. Keep a copy of ID long/short descriptions in the Patient table, written
//    by the client (from its storedvar copy of the server's descriptions) at
//    the moment of upload, and then cached by the server in the patient table?
//    - In favour of "yes": this is a defence against the server administrator
//      mucking around with the server IDs/descriptions retrospectively.
//    - In favour of "no": it's only a defence against that, and it is probably
//      needless and potentially confusing de-normalization.
//    - If the administrator wants to change the descriptions in a trivial way,
//      it's then possible (without all the server consistency checks
//      complaining). If the administrator wants to make a major change, one
//      would have to ask: why? - but then the administrator could unregister
//      all devices, so a manual registration is enforced (making the user
//      check), and even if they don't, the upload process checks for ID
//      description consistency and will abort if it's not consistent; see
//      NetworkManager::areDescriptionsOK().
// A. Design change 2017-07-08: NO. Don't #define this.
// As of 2018-11-05, it no longer has any effect.

// #define DUPLICATE_ID_DESCRIPTIONS_INTO_PATIENT_TABLE

// ============================================================================
// Limit to 8 ID numbers, in patient table, or use separate table?
// ============================================================================
// Design change 2017-07-24: Do NOT #define this.
// As of 2018-11-05, it no longer has any effect.

// #define LIMIT_TO_8_IDNUMS_AND_USE_PATIENT_TABLE


// ============================================================================
// Have clients send "analytics" information to CamCOPS central?
// ============================================================================
// A. Design change 2017-07-08: no. The server can do this; we shouldn't burden
//    clients with pointless extra network activity, even if users agree.
//    Also, subsequent more general privacy point: no, whether from the client
//    or the server.
// As of 2018-11-05, it no longer has any effect.

// #define ALLOW_SEND_ANALYTICS
