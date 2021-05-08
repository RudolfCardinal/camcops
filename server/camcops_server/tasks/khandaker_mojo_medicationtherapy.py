#!/usr/bin/env python

"""
camcops_server/tasks/khandaker_mojo_medicationtherapy.py

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


from typing import List, Optional, Type, TYPE_CHECKING

from sqlalchemy.sql.sqltypes import Float, Integer, UnicodeText

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_db import (
    ancillary_relationship,
    GenericTabletRecordMixin,
    TaskDescendant,
)
from camcops_server.cc_modules.cc_html import answer, tr_qa
from camcops_server.cc_modules.cc_sqlalchemy import Base
from camcops_server.cc_modules.cc_sqla_coltypes import (
    CamcopsColumn,
)
from camcops_server.cc_modules.cc_task import (
    Task,
    TaskHasPatientMixin,
)

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_request import CamcopsRequest


class KhandakerMojoTableItem(GenericTabletRecordMixin, TaskDescendant, Base):
    __abstract__ = True

    def any_fields_none(self) -> bool:
        for f in self.mandatory_fields():
            if getattr(self, f) is None:
                return True
        return False

    @classmethod
    def mandatory_fields(cls) -> List[str]:
        raise NotImplementedError

    def get_response_option(self, req: "CamcopsRequest") -> Optional[str]:
        # Reads "self.response" from derived class.
        # noinspection PyUnresolvedReferences
        response = self.response  # type: Optional[int]
        if response is None:
            return None
        return self.task_ancestor().xstring(req, f"response_{response}")

    # -------------------------------------------------------------------------
    # TaskDescendant overrides
    # -------------------------------------------------------------------------

    @classmethod
    def task_ancestor_class(cls) -> Optional[Type["Task"]]:
        return KhandakerMojoMedicationTherapy

    def task_ancestor(self) -> Optional["KhandakerMojoMedicationTherapy"]:
        # Reads "self.medicationtable_id" from derived class.
        # noinspection PyUnresolvedReferences
        return KhandakerMojoMedicationTherapy.get_linked(
            self.medicationtable_id, self)


class KhandakerMojoMedicationItem(KhandakerMojoTableItem):
    __tablename__ = "khandaker_mojo_medication_item"

    medicationtable_id = CamcopsColumn(
        "medicationtable_id", Integer, nullable=False,
        comment="FK to medicationtable"
    )
    seqnum = CamcopsColumn(
        "seqnum", Integer, nullable=False,
        comment="Sequence number of this medication"
    )
    brand_name = CamcopsColumn(
        "brand_name", UnicodeText,
        comment="Brand name"
    )
    chemical_name = CamcopsColumn(
        "chemical_name", UnicodeText,
        comment="Chemical name for study team"
    )
    dose = CamcopsColumn(
        "dose", UnicodeText,
        comment="Dose"
    )
    frequency = CamcopsColumn(
        "frequency", UnicodeText,
        comment="Frequency"
    )
    duration_months = CamcopsColumn(
        "duration_months", Float,
        comment="Duration (months)"
    )
    indication = CamcopsColumn(
        "indication", UnicodeText,
        comment="Indication (what is the medication used for?)"
    )
    response = CamcopsColumn(
        "response", Integer,
        comment=("1 = treats all symptoms, "
                 "2 = most symptoms, "
                 "3 = some symptoms, "
                 "4 = no symptoms)")
    )

    @classmethod
    def mandatory_fields(cls) -> List[str]:
        return [
            "brand_name",
            "chemical_name",
            "dose",
            "frequency",
            "duration_months",
            "indication",
            "response",
        ]

    def get_html_table_row(self, req: "CamcopsRequest") -> str:
        return f"""
            <tr>
                <td>{answer(self.chemical_name)}</td>
                <td>{answer(self.brand_name)}</td>
                <td>{answer(self.dose)}</td>
                <td>{answer(self.frequency)}</td>
                <td>{answer(self.duration_months)}</td>
                <td>{answer(self.indication)}</td>
                <td>{answer(self.get_response_option(req))}</td>
            </tr>
        """


class KhandakerMojoTherapyItem(KhandakerMojoTableItem):
    __tablename__ = "khandaker_mojo_therapy_item"

    medicationtable_id = CamcopsColumn(
        "medicationtable_id", Integer, nullable=False,
        comment="FK to medicationtable"
    )
    seqnum = CamcopsColumn(
        "seqnum", Integer, nullable=False,
        comment="Sequence number of this therapy"
    )
    therapy = CamcopsColumn(
        "therapy", UnicodeText,
        comment="Therapy"
    )
    frequency = CamcopsColumn(
        "frequency", UnicodeText,
        comment="Frequency"
    )
    sessions_completed = CamcopsColumn(
        "sessions_completed", Integer,
        comment="Sessions completed"
    )
    sessions_planned = CamcopsColumn(
        "sessions_planned", Integer,
        comment="Sessions planned"
    )
    indication = CamcopsColumn(
        "indication", UnicodeText,
        comment="Indication (what is the medication used for?)"
    )
    response = CamcopsColumn(
        "response", Integer,
        comment=("1 = treats all symptoms, "
                 "2 = most symptoms, "
                 "3 = some symptoms, "
                 "4 = no symptoms)")
    )

    @classmethod
    def mandatory_fields(cls) -> List[str]:
        return [
            "therapy",
            "frequency",
            "sessions_completed",
            "sessions_planned",
            "indication",
            "response",
        ]

    def get_html_table_row(self, req: "CamcopsRequest") -> str:
        return f"""
            <tr>
                <td>{answer(self.therapy)}</td>
                <td>{answer(self.frequency)}</td>
                <td>{answer(self.sessions_completed)}</td>
                <td>{answer(self.sessions_planned)}</td>
                <td>{answer(self.indication)}</td>
                <td>{answer(self.get_response_option(req))}</td>
            </tr>
        """


class KhandakerMojoMedicationTherapy(TaskHasPatientMixin, Task):
    """
    Server implementation of the KhandakerMojoMedicationTherapy task
    """
    __tablename__ = "khandaker_mojo_medicationtherapy"
    shortname = "Khandaker_MOJO_MedicationTherapy"
    provides_trackers = False

    medication_items = ancillary_relationship(
        parent_class_name="KhandakerMojoMedicationTherapy",
        ancillary_class_name="KhandakerMojoMedicationItem",
        ancillary_fk_to_parent_attr_name="medicationtable_id",
        ancillary_order_by_attr_name="seqnum"
    )  # type: List[KhandakerMojoMedicationItem]

    therapy_items = ancillary_relationship(
        parent_class_name="KhandakerMojoMedicationTherapy",
        ancillary_class_name="KhandakerMojoTherapyItem",
        ancillary_fk_to_parent_attr_name="medicationtable_id",
        ancillary_order_by_attr_name="seqnum"
    )  # type: List[KhandakerMojoTherapyItem]

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Khandaker GM — MOJO — Medications and therapies")

    def is_complete(self) -> bool:
        # Whilst it's almost certain that anyone completing this task would be
        # on some kind of medication, we have no way of knowing when all
        # medication has been added to the table
        for item in self.medication_items:
            if item.any_fields_none():
                return False

        for item in self.therapy_items:
            if item.any_fields_none():
                return False

        return True

    def get_num_medication_items(self) -> int:
        return len(self.medication_items)

    def get_num_therapy_items(self) -> int:
        return len(self.therapy_items)

    def get_task_html(self, req: "CamcopsRequest") -> str:
        html = f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                    {tr_qa("Number of medications",
                            self.get_num_medication_items())}
                    {tr_qa("Number of therapies",
                            self.get_num_therapy_items())}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th>{self.xstring(req, "chemical_name")}</th>
                    <th>{self.xstring(req, "brand_name")}</th>
                    <th>{self.xstring(req, "dose")}</th>
                    <th>{self.xstring(req, "frequency")}</th>
                    <th>{self.xstring(req, "duration_months")}</th>
                    <th>{self.xstring(req, "indication")}</th>
                    <th>{self.xstring(req, "response")}</th>
                </tr>
        """
        for item in self.medication_items:
            html += item.get_html_table_row(req)

        html += f"""
            </table>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th>{self.xstring(req, "therapy")}</th>
                    <th>{self.xstring(req, "frequency")}</th>
                    <th>{self.xstring(req, "sessions_completed")}</th>
                    <th>{self.xstring(req, "sessions_planned")}</th>
                    <th>{self.xstring(req, "indication")}</th>
                    <th>{self.xstring(req, "response")}</th>
                </tr>
        """

        for item in self.therapy_items:
            html += item.get_html_table_row(req)

        html += """
            </table>
        """

        return html
