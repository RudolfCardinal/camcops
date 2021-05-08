#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_filename.py

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

**Functions for handling filenames, and some associated constants.**

"""

import logging
import os
from typing import List, TYPE_CHECKING

from cardinal_pythonlib.datetimefunc import (
    format_datetime,
    get_now_localtz_pendulum,
)
from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.stringfunc import mangle_unicode_to_ascii
from pendulum import Date, DateTime as Pendulum

from camcops_server.cc_modules.cc_constants import DateFormat
from camcops_server.cc_modules.cc_exception import STR_FORMAT_EXCEPTIONS

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_patientidnum import PatientIdNum  # noqa: E501,F401
    from camcops_server.cc_modules.cc_request import CamcopsRequest  # noqa: E501,F401

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Ancillary functions for export filenames
# =============================================================================

class PatientSpecElementForFilename(object):
    """
    Parts of the patient information that can be used to autogenerate
    the "patient" part of a filename specification.
    """
    SURNAME = "surname"
    FORENAME = "forename"
    DOB = "dob"
    SEX = "sex"
    ALLIDNUMS = "allidnums"
    IDSHORTDESC_PREFIX = "idshortdesc"  # special
    IDNUM_PREFIX = "idnum"  # special


class FilenameSpecElement(object):
    """
    Types of informatino that can be used to autogenerate a filename.
    """
    PATIENT = "patient"
    CREATED = "created"
    NOW = "now"
    TASKTYPE = "tasktype"
    SERVERPK = "serverpk"
    FILETYPE = "filetype"
    ANONYMOUS = "anonymous"
    # ... plus all those from PatientSpecElementForFilename


def patient_spec_for_filename_is_valid(patient_spec: str,
                                       valid_which_idnums: List[int]) -> bool:
    """
    Returns ``True`` if the ``patient_spec`` appears valid; otherwise
    ``False``.
    """
    pse = PatientSpecElementForFilename
    testdict = {
        pse.SURNAME: "surname",
        pse.FORENAME: "forename",
        pse.DOB: "dob",
        pse.SEX: "sex",
        pse.ALLIDNUMS: "allidnums",
    }
    for n in valid_which_idnums:
        nstr = str(n)
        testdict[pse.IDSHORTDESC_PREFIX + nstr] = pse.IDSHORTDESC_PREFIX + nstr
        testdict[pse.IDNUM_PREFIX + nstr] = pse.IDNUM_PREFIX + nstr
    try:
        # Legal substitutions only?
        patient_spec.format(**testdict)
        return True
    except STR_FORMAT_EXCEPTIONS:  # duff patient_spec; details unimportant
        return False


def filename_spec_is_valid(filename_spec: str,
                           valid_which_idnums: List[int]) -> bool:
    """
    Returns ``True`` if the ``filename_spec`` appears valid; otherwise
    ``False``.
    """
    pse = PatientSpecElementForFilename
    fse = FilenameSpecElement
    testdict = {
        # As above:
        pse.SURNAME: "surname",
        pse.FORENAME: "forename",
        pse.DOB: "dob",
        pse.SEX: "sex",
        pse.ALLIDNUMS: "allidnums",
        # Plus:
        fse.PATIENT: "patient",
        fse.CREATED: "created",
        fse.NOW: "now",
        fse.TASKTYPE: "tasktype",
        fse.SERVERPK: "serverpk",
        fse.FILETYPE: "filetype",
        fse.ANONYMOUS: "anonymous",
    }
    for n in valid_which_idnums:
        nstr = str(n)
        testdict[pse.IDSHORTDESC_PREFIX + nstr] = pse.IDSHORTDESC_PREFIX + nstr
        testdict[pse.IDNUM_PREFIX + nstr] = pse.IDNUM_PREFIX + nstr
    try:
        # Legal substitutions only?
        filename_spec.format(**testdict)
        return True
    except STR_FORMAT_EXCEPTIONS:  # duff filename_spec; details unimportant
        return False


def get_export_filename(req: "CamcopsRequest",
                        patient_spec_if_anonymous: str,
                        patient_spec: str,
                        filename_spec: str,
                        filetype: str,
                        is_anonymous: bool = False,
                        surname: str = None,
                        forename: str = None,
                        dob: Date = None,
                        sex: str = None,
                        idnum_objects: List['PatientIdNum'] = None,
                        creation_datetime: Pendulum = None,
                        basetable: str = None,
                        serverpk: int = None,
                        skip_conversion_to_safe_filename: bool = False) -> str:
    """
    Get filename, for file exports/transfers.
    Also used for e-mail headers and bodies.

    Args:
        req: :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        patient_spec_if_anonymous:
            patient specification to be used for anonymous tasks
        patient_spec:
            patient specification to be used for patient-identifiable tasks
        filename_spec:
            specification to use to create the filename (may include
            patient information from the patient specification)
        filetype:
            task output format and therefore file type (e.g. HTML, PDF, XML)
        is_anonymous: is it an anonymous task?
        surname: patient's surname
        forename: patient's forename
        dob: patient's date of birth
        sex: patient's sex
        idnum_objects: list of :class:`PatientIdNum` objects for the patient
        creation_datetime: date/time the task was created
        basetable: name of the task's base table
        serverpk: server PK of the task
        skip_conversion_to_safe_filename: don't bother converting the result
            to a safe filename (because it'll be used for something else, like
            an e-mail subject)

    Returns:
        the generated filename

    """
    idnum_objects = idnum_objects or []  # type: List['PatientIdNum']
    pse = PatientSpecElementForFilename
    fse = FilenameSpecElement
    d = {
        pse.SURNAME: surname or "",
        pse.FORENAME: forename or "",
        pse.DOB: (
            format_datetime(dob, DateFormat.FILENAME_DATE_ONLY, "")
            if dob else ""
        ),
        pse.SEX: sex or "",
    }
    all_id_components = []
    for idobj in idnum_objects:
        if idobj.which_idnum is not None:
            nstr = str(idobj.which_idnum)
            has_num = idobj.idnum_value is not None
            d[pse.IDNUM_PREFIX + nstr] = str(idobj.idnum_value) if has_num else ""  # noqa
            d[pse.IDSHORTDESC_PREFIX + nstr] = idobj.short_description(req) or ""  # noqa
            if has_num and idobj.short_description(req):
                all_id_components.append(idobj.get_filename_component(req))
    d[pse.ALLIDNUMS] = "_".join(all_id_components)
    if is_anonymous:
        patient = patient_spec_if_anonymous
    else:
        try:
            patient = str(patient_spec).format(**d)
        except STR_FORMAT_EXCEPTIONS:
            log.warning("Bad patient_spec: {!r}; dictionary was {!r}",
                        patient_spec, d)
            patient = "invalid_patient_spec"
    d.update({
        fse.PATIENT: patient,
        fse.CREATED: format_datetime(creation_datetime,
                                     DateFormat.FILENAME, ""),
        fse.NOW: format_datetime(get_now_localtz_pendulum(),
                                 DateFormat.FILENAME),
        fse.TASKTYPE: str(basetable or ""),
        fse.SERVERPK: str(serverpk or ""),
        fse.FILETYPE: filetype.lower(),
        fse.ANONYMOUS: patient_spec_if_anonymous if is_anonymous else "",
    })
    try:
        formatted = str(filename_spec).format(**d)
    except STR_FORMAT_EXCEPTIONS:
        log.warning("Bad filename_spec: {!r}", filename_spec)
        formatted = "invalid_filename_spec"
    if skip_conversion_to_safe_filename:
        return formatted
    return convert_string_for_filename(formatted, allow_paths=True)


def convert_string_for_filename(s: str, allow_paths: bool = False) -> str:
    """
    Remove characters that don't play nicely in filenames across multiple
    operating systems.
    """
    # http://stackoverflow.com/questions/7406102
    # ... modified
    s = mangle_unicode_to_ascii(s)
    s = s.replace(" ", "_")
    keepcharacters = ['.', '_', '-']
    if allow_paths:
        keepcharacters.extend([os.sep])  # '/' under UNIX; '\' under Windows
    s = "".join(c for c in s if c.isalnum() or c in keepcharacters)
    return s


def change_filename_ext(filename: str, new_extension_with_dot: str) -> str:
    """
    Replaces the extension, i.e. the part of the filename after its last '.'.
    """
    (root, ext) = os.path.splitext(filename)
    # ... converts "blah.blah.txt" to ("blah.blah", ".txt")
    return root + new_extension_with_dot
