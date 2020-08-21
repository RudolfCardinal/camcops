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

#include "camcopsversion.h"

namespace camcopsversion {  // http://semver.org/

// ----------------------------------------------------------------------------
// CamCOPS client version/date
// ----------------------------------------------------------------------------
// Increase this when the client is changed:

const Version CAMCOPS_CLIENT_VERSION(2, 3, 7);
const QDate CAMCOPS_CLIENT_CHANGEDATE(2020, 3, 3);

// +++ NOW ALSO UPDATE: +++
//
//      documentation/source/changelog.rst
//      documentation/source/developer/releasing.rst (Google Play Store versions)
//      tablet_qt/android/AndroidManifest.xml (version code + version name)
//      tablet_qt/camcops_windows_innosetup.iss (CamcopsClientVersion)


// ----------------------------------------------------------------------------
// Minimum server version that the client will upload to
// ----------------------------------------------------------------------------
// It is NOT normal to have to increase this if you add a new task.
// You should be overriding minimumServerVersion() for the new task instead.
// Only change this if aspects of the core server tables or API change in a
// breaking way.
// (Why? Suppose you are running an old server, and your users download a newer
// client; they should still be able to operate as long as they're not trying
// to create/upload tasks that you don't yet support.)
// See also other upload-related version constants in networkmanager.cpp.

const Version MINIMUM_SERVER_VERSION(2, 2, 0);

}  // namespace camcopsversion
