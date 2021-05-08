#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_device.py

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

**Representation of the client devices.**

"""

from typing import Optional, TYPE_CHECKING

from cardinal_pythonlib.classes import classproperty
from pendulum import DateTime as Pendulum
from sqlalchemy.orm import Query, relationship, Session as SqlASession
from sqlalchemy.sql.schema import Column, ForeignKey
from sqlalchemy.sql.sqltypes import Boolean, DateTime, Integer, Text

from camcops_server.cc_modules.cc_constants import DEVICE_NAME_FOR_SERVER
from camcops_server.cc_modules.cc_report import Report
from camcops_server.cc_modules.cc_user import User
from camcops_server.cc_modules.cc_sqla_coltypes import (
    DeviceNameColType,
    SemanticVersionColType,
)
from camcops_server.cc_modules.cc_sqlalchemy import Base
from camcops_server.cc_modules.cc_version import CAMCOPS_SERVER_VERSION

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_request import CamcopsRequest


# =============================================================================
# Device class
# =============================================================================

class Device(Base):
    """
    Represents a tablet (client) device.
    """
    __tablename__ = "_security_devices"
    id = Column(
        "id", Integer,
        primary_key=True, autoincrement=True,
        comment="ID of the source tablet device"
    )
    name = Column(
        "name", DeviceNameColType,
        unique=True, index=True,
        comment="Short cryptic unique name of the source tablet device"
    )
    registered_by_user_id = Column(
        "registered_by_user_id", Integer, ForeignKey("_security_users.id"),
        comment="ID of user that registered the device"
    )
    registered_by_user = relationship("User",
                                      foreign_keys=[registered_by_user_id])
    when_registered_utc = Column(
        "when_registered_utc", DateTime,
        comment="Date/time when the device was registered (UTC)"
    )
    friendly_name = Column(
        "friendly_name", Text,
        comment="Friendly name of the device"
    )
    camcops_version = Column(
        "camcops_version", SemanticVersionColType,
        comment="CamCOPS version number on the tablet device"
    )
    last_upload_batch_utc = Column(
        "last_upload_batch_utc", DateTime,
        comment="Date/time when the device's last upload batch started (UTC)"
    )
    ongoing_upload_batch_utc = Column(
        "ongoing_upload_batch_utc", DateTime,
        comment="Date/time when the device's ongoing upload batch "
                "started (UTC)"
    )
    uploading_user_id = Column(
        "uploading_user_id", Integer, ForeignKey("_security_users.id",
                                                 use_alter=True),
        comment="ID of user in the process of uploading right now"
    )
    uploading_user = relationship("User", foreign_keys=[uploading_user_id])
    currently_preserving = Column(
        "currently_preserving", Boolean, default=False,
        comment="Preservation currently in progress"
    )

    @classmethod
    def get_device_by_name(cls, dbsession: SqlASession,
                           device_name: str) -> Optional['Device']:
        """
        Returns a device by its name.
        """
        if not device_name:
            return None
        device = (
            dbsession.query(cls)
            .filter(cls.name == device_name)
            .first()
        )  # type: Optional[Device]
        return device

    @classmethod
    def get_device_by_id(cls, dbsession: SqlASession,
                         device_id: int) -> Optional['Device']:
        """
        Returns a device by its integer ID.
        """
        if device_id is None:
            return None
        device = (
            dbsession.query(cls)
            .filter(cls.id == device_id)
            .first()
        )  # type: Optional[Device]
        return device

    @classmethod
    def get_server_device(cls, dbsession: SqlASession) -> "Device":
        """
        Return the special device meaning "the server", creating it if it
        doesn't already exist.
        """
        device = cls.get_device_by_name(dbsession, DEVICE_NAME_FOR_SERVER)
        if device is None:
            device = Device()
            device.name = DEVICE_NAME_FOR_SERVER
            device.friendly_name = "CamCOPS server"
            device.registered_by_user = User.get_system_user(dbsession)
            device.when_registered_utc = Pendulum.utcnow()
            device.camcops_version = CAMCOPS_SERVER_VERSION
            dbsession.add(device)
            dbsession.flush()  # So that we can use the PK elsewhere
        return device

    def get_friendly_name(self) -> str:
        """
        Get the device's friendly name (or failing that, its name).
        """
        if self.friendly_name is None:
            return self.name
        return self.friendly_name

    def get_friendly_name_and_id(self) -> str:
        """
        Get a formatted representation of the device (name, ID,
        friendly name).
        """
        if self.friendly_name is None:
            return self.name
        return f"{self.name} (device# {self.id}, {self.friendly_name})"

    def get_id(self) -> int:
        """
        Get the device's integer ID.
        """
        return self.id

    def is_valid(self) -> bool:
        """
        Having instantiated an instance with ``Device(device_id)``, this
        function reports whether it is a valid device, i.e. is it in the
        database?
        """
        return self.id is not None


# =============================================================================
# Reports
# =============================================================================

class DeviceReport(Report):
    """
    Report to show registered devices.
    This is a superuser-only report, so we do not override superuser_only.
    """
    # noinspection PyMethodParameters
    @classproperty
    def report_id(cls) -> str:
        return "devices"

    @classmethod
    def title(cls, req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("(Server) Devices registered with the server")

    def get_query(self, req: "CamcopsRequest") -> Query:
        dbsession = req.dbsession
        query = (
            dbsession.query(Device.id,
                            Device.name,
                            Device.registered_by_user_id,
                            Device.when_registered_utc,
                            Device.friendly_name,
                            Device.camcops_version,
                            Device.last_upload_batch_utc)
            .order_by(Device.id)
        )
        return query
