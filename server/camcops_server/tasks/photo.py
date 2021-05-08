#!/usr/bin/env python

"""
camcops_server/tasks/photo.py

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

"""

from typing import List, Optional, Type

import cardinal_pythonlib.rnc_web as ws
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Integer, UnicodeText

from camcops_server.cc_modules.cc_blob import (
    Blob,
    blob_relationship,
    get_blob_img_html,
)
from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_ctvinfo import CTV_INCOMPLETE, CtvInfo
from camcops_server.cc_modules.cc_db import (
    ancillary_relationship,
    GenericTabletRecordMixin,
    TaskDescendant,
)
from camcops_server.cc_modules.cc_html import answer, tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_snomed import SnomedExpression, SnomedLookup
from camcops_server.cc_modules.cc_sqla_coltypes import CamcopsColumn
from camcops_server.cc_modules.cc_sqlalchemy import Base
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasClinicianMixin,
    TaskHasPatientMixin,
)


# =============================================================================
# Photo
# =============================================================================

class Photo(TaskHasClinicianMixin, TaskHasPatientMixin, Task):
    """
    Server implementation of the Photo task.
    """
    __tablename__ = "photo"
    shortname = "Photo"

    description = Column(
        "description", UnicodeText,
        comment="Description of the photograph"
    )
    photo_blobid = CamcopsColumn(
        "photo_blobid", Integer,
        is_blob_id_field=True, blob_relationship_attr_name="photo",
        comment="ID of the BLOB (foreign key to blobs.id, given "
                "matching device and current/frozen record status)"
    )
    # IGNORED. REMOVE WHEN ALL PRE-2.0.0 TABLETS GONE:
    rotation = Column(  # DEFUNCT as of v2.0.0
        "rotation", Integer,
        comment="Rotation (clockwise, in degrees) to be applied for viewing"
    )

    photo = blob_relationship("Photo", "photo_blobid")  # type: Optional[Blob]

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Photograph")

    def is_complete(self) -> bool:
        return self.photo_blobid is not None

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        if not self.is_complete():
            return CTV_INCOMPLETE
        if not self.description:
            return []
        return [CtvInfo(content=self.description)]

    def get_task_html(self, req: CamcopsRequest) -> str:
        # noinspection PyTypeChecker
        return """
            <table class="{CssClass.TASKDETAIL}">
                <tr class="{CssClass.SUBHEADING}"><td>Description</td></tr>
                <tr><td>{description}</td></tr>
                <tr class="{CssClass.SUBHEADING}"><td>Photo</td></tr>
                <tr><td>{photo}</td></tr>
            </table>
        """.format(
            CssClass=CssClass,
            description=answer(
                ws.webify(self.description), default="(No description)",
                default_for_blank_strings=True
            ),
            # ... xhtml2pdf crashes if the contents are empty...
            photo=get_blob_img_html(self.photo)
        )

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        if not self.is_complete():
            return []
        return [
            SnomedExpression(req.snomed(SnomedLookup.PHOTOGRAPH_PROCEDURE)),
            SnomedExpression(req.snomed(SnomedLookup.PHOTOGRAPH_PHYSICAL_OBJECT)),  # noqa
        ]


# =============================================================================
# PhotoSequence
# =============================================================================

class PhotoSequenceSinglePhoto(GenericTabletRecordMixin, TaskDescendant, Base):
    __tablename__ = "photosequence_photos"

    photosequence_id = Column(
        "photosequence_id", Integer, nullable=False,
        comment="Tablet FK to photosequence"
    )
    seqnum = Column(
        "seqnum", Integer, nullable=False,
        comment="Sequence number of this photo "
                "(consistently 1-based as of 2018-12-01)"
    )
    description = Column(
        "description", UnicodeText,
        comment="Description of the photograph"
    )
    photo_blobid = CamcopsColumn(
        "photo_blobid", Integer,
        is_blob_id_field=True, blob_relationship_attr_name="photo",
        comment="ID of the BLOB (foreign key to blobs.id, given "
                "matching device and current/frozen record status)"
    )
    # IGNORED. REMOVE WHEN ALL PRE-2.0.0 TABLETS GONE:
    rotation = Column(  # DEFUNCT as of v2.0.0
        "rotation", Integer,
        comment="(DEFUNCT COLUMN) "
                "Rotation (clockwise, in degrees) to be applied for viewing"
    )

    photo = blob_relationship("PhotoSequenceSinglePhoto", "photo_blobid")

    def get_html_table_rows(self) -> str:
        # noinspection PyTypeChecker
        return """
            <tr class="{CssClass.SUBHEADING}">
                <td>Photo {num}: <b>{description}</b></td>
            </tr>
            <tr><td>{photo}</td></tr>
        """.format(
            CssClass=CssClass,
            num=self.seqnum,
            description=ws.webify(self.description),
            photo=get_blob_img_html(self.photo)
        )

    # -------------------------------------------------------------------------
    # TaskDescendant overrides
    # -------------------------------------------------------------------------

    @classmethod
    def task_ancestor_class(cls) -> Optional[Type["Task"]]:
        return PhotoSequence

    def task_ancestor(self) -> Optional["PhotoSequence"]:
        return PhotoSequence.get_linked(self.photosequence_id, self)


class PhotoSequence(TaskHasClinicianMixin, TaskHasPatientMixin, Task):
    """
    Server implementation of the PhotoSequence task.
    """
    __tablename__ = "photosequence"
    shortname = "PhotoSequence"

    sequence_description = Column(
        "sequence_description", UnicodeText,
        comment="Description of the sequence of photographs"
    )

    photos = ancillary_relationship(
        parent_class_name="PhotoSequence",
        ancillary_class_name="PhotoSequenceSinglePhoto",
        ancillary_fk_to_parent_attr_name="photosequence_id",
        ancillary_order_by_attr_name="seqnum"
    )  # type: List[PhotoSequenceSinglePhoto]

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Photograph sequence")

    def get_clinical_text(self, req: CamcopsRequest) -> List[CtvInfo]:
        infolist = [CtvInfo(content=self.sequence_description)]
        for p in self.photos:
            infolist.append(CtvInfo(content=p.description))
        return infolist

    def get_num_photos(self) -> int:
        return len(self.photos)

    def is_complete(self) -> bool:
        # If you're wondering why this is being called unexpectedly: it may be
        # because this task is being displayed in the task list, at which point
        # we colour it by its complete-or-not status.
        return bool(self.sequence_description) and self.get_num_photos() > 0

    def get_task_html(self, req: CamcopsRequest) -> str:
        html = f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr_qa("Number of photos", self.get_num_photos())}
                    {tr_qa("Description", self.sequence_description)}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
        """
        for p in self.photos:
            html += p.get_html_table_rows()
        html += """
            </table>
        """
        return html

    def get_snomed_codes(self, req: CamcopsRequest) -> List[SnomedExpression]:
        if not self.is_complete():
            return []
        return [
            SnomedExpression(req.snomed(SnomedLookup.PHOTOGRAPH_PROCEDURE)),
            SnomedExpression(req.snomed(SnomedLookup.PHOTOGRAPH_PHYSICAL_OBJECT)),  # noqa
        ]
