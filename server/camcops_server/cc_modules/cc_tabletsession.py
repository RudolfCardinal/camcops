#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_tabletsession.py

===============================================================================

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
    along with CamCOPS. If not, see <https://www.gnu.org/licenses/>.

===============================================================================

**Session information for client devices (tablets etc.).**

"""

import logging
from typing import Optional, Set, TYPE_CHECKING

from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.reprfunc import simple_repr
from pyramid.exceptions import HTTPBadRequest

from camcops_server.cc_modules.cc_client_api_core import (
    fail_user_error,
    TabletParam,
)
from camcops_server.cc_modules.cc_constants import (
    DEVICE_NAME_FOR_SERVER,
    USER_NAME_FOR_SYSTEM,
)
from camcops_server.cc_modules.cc_device import Device
from camcops_server.cc_modules.cc_pyramid import RequestMethod
from camcops_server.cc_modules.cc_validators import (
    validate_anything,
    validate_device_name,
    validate_username,
)
from camcops_server.cc_modules.cc_version import (
    FIRST_CPP_TABLET_VER,
    FIRST_TABLET_VER_WITH_SEPARATE_IDNUM_TABLE,
    FIRST_TABLET_VER_WITHOUT_IDDESC_IN_PT_TABLE,
    FIRST_TABLET_VER_WITH_EXPLICIT_PKNAME_IN_UPLOAD_TABLE,
    make_version,
    MINIMUM_TABLET_VERSION,
)

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_request import CamcopsRequest

log = BraceStyleAdapter(logging.getLogger(__name__))

INVALID_USERNAME_PASSWORD = "Invalid username/password (or user not authorized)"
NO_UPLOAD_GROUP_SET = "No upload group set for user "


class TabletSession(object):
    """
    Represents session information for client devices. They use HTTPS POST
    calls and do not bother with cookies.
    """
    def __init__(self, req: "CamcopsRequest") -> None:
        # Check the basics
        if req.method != RequestMethod.POST:
            raise HTTPBadRequest("Must use POST method")
            # ... this is for humans to view, so it has a pretty error

        # Read key things
        self.req = req
        self.operation = req.get_str_param(TabletParam.OPERATION)
        try:
            self.device_name = req.get_str_param(
                TabletParam.DEVICE, validator=validate_device_name)
            self.username = req.get_str_param(
                TabletParam.USER, validator=validate_username)
        except ValueError as e:
            fail_user_error(str(e))
        self.password = req.get_str_param(
            TabletParam.PASSWORD, validator=validate_anything)
        self.session_id = req.get_int_param(TabletParam.SESSION_ID)
        self.session_token = req.get_str_param(
            TabletParam.SESSION_TOKEN, validator=validate_anything)
        self.tablet_version_str = req.get_str_param(
            TabletParam.CAMCOPS_VERSION, validator=validate_anything)
        try:
            self.tablet_version_ver = make_version(self.tablet_version_str)
        except ValueError:
            fail_user_error(
                f"CamCOPS tablet version nonsensical: "
                f"{self.tablet_version_str!r}")

        # Basic security check: no pretending to be the server
        if self.device_name == DEVICE_NAME_FOR_SERVER:
            fail_user_error(
                f"Tablets cannot use the device name "
                f"{DEVICE_NAME_FOR_SERVER!r}")
        if self.username == USER_NAME_FOR_SYSTEM:
            fail_user_error(
                f"Tablets cannot use the username {USER_NAME_FOR_SYSTEM!r}")

        self._device_obj = None  # type: Optional[Device]

        # Ensure table version is OK
        if self.tablet_version_ver < MINIMUM_TABLET_VERSION:
            fail_user_error(
                f"Tablet CamCOPS version too old: is "
                f"{self.tablet_version_str}, need {MINIMUM_TABLET_VERSION}")
        # Other version things are done via properties

        # Upload efficiency
        self._dirty_table_names = set()  # type: Set[str]

        # Report
        log.info("Incoming client API connection from IP={i}, port={p}, "
                 "device_name={dn!r}, "
                 # "device_id={di}, "
                 "camcops_version={v}, "
                 "username={u}, operation={o}",
                 i=req.remote_addr,
                 p=req.remote_port,
                 dn=self.device_name,
                 # di=self.device_id,
                 v=self.tablet_version_str,
                 u=self.username,
                 o=self.operation)

    def __repr__(self) -> str:
        return simple_repr(
            self,
            ["session_id", "session_token", "device_name", "username",
             "operation"],
            with_addr=True
        )

    # -------------------------------------------------------------------------
    # Database objects, accessed on demand
    # -------------------------------------------------------------------------

    @property
    def device(self) -> Optional[Device]:
        """
        Returns the :class:`camcops_server.cc_modules.cc_device.Device`
        associated with this request/session, or ``None``.
        """
        if self._device_obj is None:
            dbsession = self.req.dbsession
            self._device_obj = Device.get_device_by_name(dbsession,
                                                         self.device_name)
        return self._device_obj

    # -------------------------------------------------------------------------
    # Permissions and similar
    # -------------------------------------------------------------------------

    @property
    def device_id(self) -> Optional[int]:
        """
        Returns the integer device ID, if known.
        """
        device = self.device
        if not device:
            return None
        return device.id

    @property
    def user_id(self) -> Optional[int]:
        """
        Returns the integer user ID, if known.
        """
        user = self.req.user
        if not user:
            return None
        return user.id

    def is_device_registered(self) -> bool:
        """
        Is the device registered with our server?
        """
        return self.device is not None

    def reload_device(self):
        """
        Re-fetch the device information from the database.
        (Or, at least, do so when it's next required.)
        """
        self._device_obj = None

    def ensure_device_registered(self) -> None:
        """
        Ensure the device is registered. Raises :exc:`UserErrorException`
        on failure.
        """
        if not self.is_device_registered():
            fail_user_error("Unregistered device")

    def ensure_valid_device_and_user_for_uploading(self) -> None:
        """
        Ensure the device/username/password combination is valid for uploading.
        Raises :exc:`UserErrorException` on failure.
        """
        user = self.req.user
        if not user:
            fail_user_error(INVALID_USERNAME_PASSWORD)
        if user.upload_group_id is None:
            fail_user_error(NO_UPLOAD_GROUP_SET + user.username)
        if not user.may_upload:
            fail_user_error("User not authorized to upload to selected group")
        # Username/password combination found and is valid. Now check device.
        self.ensure_device_registered()

    def ensure_valid_user_for_device_registration(self) -> None:
        """
        Ensure the username/password combination is valid for device
        registration. Raises :exc:`UserErrorException` on failure.
        """
        user = self.req.user
        if not user:
            fail_user_error(INVALID_USERNAME_PASSWORD)
        if user.upload_group_id is None:
            fail_user_error(NO_UPLOAD_GROUP_SET + user.username)
        if not user.may_register_devices:
            fail_user_error("User not authorized to register devices for "
                            "selected group")

    def set_session_id_token(self, session_id: int,
                             session_token: str) -> None:
        """
        Sets the session ID and token.
        Typical situation:

        - :class:`camcops_server.cc_modules.cc_tabletsession.TabletSession`
          created; may or may not have an ID/token as part of the POST request
        - :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
          translates that into a server-side session
        - If one wasn't found and needs to be created, we write back
          the values here.
        """
        self.session_id = session_id
        self.session_token = session_token

    # -------------------------------------------------------------------------
    # Version information (via property as not always needed)
    # -------------------------------------------------------------------------

    @property
    def cope_with_deleted_patient_descriptors(self) -> bool:
        """
        Must we cope with an old client that had ID descriptors
        in the Patient table?
        """
        return (self.tablet_version_ver <
                FIRST_TABLET_VER_WITHOUT_IDDESC_IN_PT_TABLE)

    @property
    def cope_with_old_idnums(self) -> bool:
        """
        Must we cope with an old client that had ID numbers embedded in the
        Patient table?
        """
        return (self.tablet_version_ver <
                FIRST_TABLET_VER_WITH_SEPARATE_IDNUM_TABLE)

    @property
    def explicit_pkname_for_upload_table(self) -> bool:
        """
        Is the client a nice new one that explicitly names the
        primary key when uploading tables?
        """
        return (self.tablet_version_ver >=
                FIRST_TABLET_VER_WITH_EXPLICIT_PKNAME_IN_UPLOAD_TABLE)

    @property
    def pkname_in_upload_table_neither_first_nor_explicit(self):
        """
        Is the client a particularly tricky old version that is a C++ client
        (generally a good thing, but meaning that the primary key might not be
        the first field in uploaded tables) but had a bug such that it did not
        explicitly name its PK either?

        See discussion of bug in ``NetworkManager::sendTableWhole`` (C++).
        For these versions, the only safe thing is to take ``"id"`` as the
        name of the (client-side) primary key.
        """
        return (self.tablet_version_ver >= FIRST_CPP_TABLET_VER and
                not self.explicit_pkname_for_upload_table)
