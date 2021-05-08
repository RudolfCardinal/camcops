#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_specialnote.py

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

**Special notes that are attached, on the server, to tasks or patients.**

"""

from typing import List, Optional

import cardinal_pythonlib.rnc_web as ws
from sqlalchemy.orm import relationship, Session as SqlASession
from sqlalchemy.sql.expression import update
from sqlalchemy.sql.schema import Column, ForeignKey
from sqlalchemy.sql.sqltypes import Boolean, Integer, UnicodeText

from camcops_server.cc_modules.cc_constants import ERA_NOW
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    PendulumDateTimeAsIsoTextColType,
    EraColType,
    TableNameColType,
)
from camcops_server.cc_modules.cc_sqlalchemy import Base
from camcops_server.cc_modules.cc_user import User
from camcops_server.cc_modules.cc_xml import (
    make_xml_branches_from_columns,
    XmlElement,
)


# =============================================================================
# SpecialNote class
# =============================================================================

SPECIALNOTE_FWD_REF = "SpecialNote"


class SpecialNote(Base):
    """
    Represents a special note, attached server-side to a task or patient.

    "Task" means all records representing versions of a single task instance,
    identified by the combination of {id, device, era}.
    """
    __tablename__ = "_special_notes"

    # PK:
    note_id = Column(
        "note_id", Integer,
        primary_key=True, autoincrement=True,
        comment="Arbitrary primary key"
    )
    # Composite FK:
    basetable = Column(
        "basetable", TableNameColType,
        index=True,
        comment="Base table of task concerned (part of FK)"
    )
    task_id = Column(
        "task_id", Integer,
        index=True,
        comment="Client-side ID of the task, or patient, concerned "
                "(part of FK)"
    )
    device_id = Column(
        "device_id", Integer,
        index=True,
        comment="Source tablet device (part of FK)"
    )
    era = Column(
        "era", EraColType,
        index=True,
        comment="Era (part of FK)"
    )
    # Details of note
    note_at = Column(
        "note_at", PendulumDateTimeAsIsoTextColType,
        comment="Date/time of note entry (ISO 8601)"
    )
    user_id = Column(
        "user_id", Integer,
        ForeignKey("_security_users.id"),
        comment="User that entered this note"
    )
    user = relationship("User")
    note = Column(
        "note", UnicodeText,
        comment="Special note, added manually"
    )
    hidden = Column(
        "hidden", Boolean, nullable=False, default=False,
        comment="Manually hidden (effectively: deleted)"
    )

    def get_note_as_string(self) -> str:
        """
        Return a string-formatted version of the note.
        """
        return (
            f"[{self.note_at or '?'}, "
            f"{self.get_username() or '?'}]\n"
            f"{self.note or ''}"
        )

    def get_note_as_html(self) -> str:
        """
        Return an HTML-formatted version of the note.
        """
        return (
            f"[{self.note_at or '?'}, {self.get_username() or '?'}]<br>"
            f"<b>{ws.webify(self.note) or ''}</b>"
        )

    def get_username(self) -> Optional[str]:
        if self.user is None:
            return None
        return self.user.username

    def get_xml_root(self, skip_fields: List[str] = None) -> XmlElement:
        """
        Get root of XML tree, as an
        :class:`camcops_server.cc_modules.cc_xml.XmlElement`.
        """
        branches = make_xml_branches_from_columns(
            self, skip_fields=skip_fields)
        return XmlElement(name=self.__tablename__, value=branches)

    @classmethod
    def forcibly_preserve_special_notes_for_device(cls, req: CamcopsRequest,
                                                   device_id: int) -> None:
        """
        Force-preserve all special notes for a given device.

        WRITES TO DATABASE.

        For update methods, see also:
        https://docs.sqlalchemy.org/en/latest/orm/persistence_techniques.html
        """
        dbsession = req.dbsession
        new_era = req.now_era_format

        # METHOD 1: use the ORM, object by object
        #
        # noinspection PyProtectedMember
        # notes = dbsession.query(cls)\
        #     .filter(cls._device_id == device_id)\
        #     .filter(cls._era == ERA_NOW)\
        #     .all()
        # for note in notes:
        #     note._era = new_era

        # METHOD 2: use the Core, in bulk
        # You can use update(table)... or table.update()...;
        # http://docs.sqlalchemy.org/en/latest/core/dml.html#sqlalchemy.sql.expression.update  # noqa

        # noinspection PyUnresolvedReferences
        dbsession.execute(
            update(cls.__table__)
            .where(cls.device_id == device_id)
            .where(cls.era == ERA_NOW)
            .values(era=new_era)
        )

    @classmethod
    def get_specialnote_by_id(cls, dbsession: SqlASession,
                              note_id: int) -> Optional["SpecialNote"]:
        """
        Returns a special note, given its ID.
        """
        return (
            dbsession.query(cls)
            .filter(cls.note_id == note_id)
            .first()
        )

    def get_group_id_of_target(self) -> Optional[int]:
        """
        Returns the group ID for the object (task or patient) that this
        special note is about.
        """
        from camcops_server.cc_modules.cc_patient import Patient
        from camcops_server.cc_modules.cc_taskfactory import task_factory_clientkeys_no_security_checks  # noqa
        dbsession = SqlASession.object_session(self)
        group_id = None
        if self.basetable == Patient.__tablename__:
            # Patient
            patient = Patient.get_patient_by_id_device_era(
                dbsession=dbsession,
                client_id=self.task_id,
                device_id=self.device_id,
                era=self.era
            )
            if patient:
                group_id = patient.group_id
        else:
            # Task
            task = task_factory_clientkeys_no_security_checks(
                dbsession=dbsession,
                basetable=self.basetable,
                client_id=self.task_id,
                device_id=self.device_id,
                era=self.era
            )
            if task:
                group_id = task.group_id
        return group_id

    def user_may_delete_specialnote(self, user: "User") -> bool:
        """
        May the specified user delete this note?
        """
        if user.superuser:
            # Superuser can delete anything
            return True
        if self.user_id == user.id:
            # Created by the current user, therefore deletable by them.
            return True
        group_id = self.get_group_id_of_target()
        if group_id is None:
            return False
        # Can the current user administer the group that the task/patient
        # belongs to? If so, they may delete the special note.
        return user.may_administer_group(group_id)
