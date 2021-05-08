#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_anon.py

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

**Anonymisation functions.**

Largely superseded by CRATE (https://doi.org/10.1186%2Fs12911-017-0437-1).

"""

from collections import OrderedDict
import csv
import sys
from typing import (
    Dict, List, Generator, TextIO, Tuple, TYPE_CHECKING, Union,
)

from cardinal_pythonlib.sqlalchemy.orm_inspect import coltype_as_typeengine
from cardinal_pythonlib.sqlalchemy.schema import (
    convert_sqla_type_for_dialect,
    does_sqlatype_require_index_len,
    is_sqlatype_date,
    is_sqlatype_text_of_length_at_least,
    RE_COLTYPE_WITH_ONE_PARAM,
)
from cardinal_pythonlib.sqlalchemy.session import SQLITE_MEMORY_URL
# from sqlalchemy.dialects.mssql.base import MSDialect
from sqlalchemy.dialects.mysql.base import MySQLDialect
from sqlalchemy.engine import create_engine
from sqlalchemy.engine.interfaces import Dialect
from sqlalchemy.orm import Session as SqlASession, sessionmaker
from sqlalchemy.sql.schema import Column

from camcops_server.cc_modules.cc_constants import TABLET_ID_FIELD
from camcops_server.cc_modules.cc_db import FN_PK
from camcops_server.cc_modules.cc_dump import DumpController
from camcops_server.cc_modules.cc_patient import Patient
from camcops_server.cc_modules.cc_patientidnum import (
    extra_id_colname,
    EXTRA_IDNUM_FIELD_PREFIX,
)
from camcops_server.cc_modules.cc_simpleobjects import TaskExportOptions
from camcops_server.cc_modules.cc_sqla_coltypes import CamcopsColumn

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_exportrecipientinfo import ExportRecipientInfo  # noqa
    from camcops_server.cc_modules.cc_request import CamcopsRequest

# =============================================================================
# Constants
# =============================================================================

MIN_STRING_LENGTH_TO_CONSIDER_SCRUBBING = 256


# =============================================================================
# Write data dictionaries for anonymisation tools
# =============================================================================

def _gen_columns_for_anon_staging_db(req: "CamcopsRequest",
                                     recipient: "ExportRecipientInfo") \
        -> Generator[Union[Column, CamcopsColumn], None, None]:
    """
    Generates all columns for an anonymisation staging database.
    """
    url = SQLITE_MEMORY_URL
    engine = create_engine(url, echo=False)
    session = sessionmaker(bind=engine)()  # type: SqlASession
    export_options = TaskExportOptions(
        include_blobs=recipient.db_include_blobs,
        db_patient_id_per_row=recipient.db_patient_id_per_row,
        db_make_all_tables_even_empty=True,
        db_include_summaries=recipient.db_add_summaries,
    )

    dc = DumpController(dst_engine=engine,
                        dst_session=session,
                        export_options=export_options,
                        req=req)
    for col in dc.gen_all_dest_columns():
        yield col


# -----------------------------------------------------------------------------
# CRIS
# -----------------------------------------------------------------------------

def _get_type_size_as_text_from_sqltype(sqltype: str) -> Tuple[str, str]:
    """
    Splits SQL size definitions like ``VARCHAR(10)`` into tuples like
    ``('VARCHAR', '10')`` If it doesn't fit that format, return
    ``(sqltype, '')``.
    """
    m = RE_COLTYPE_WITH_ONE_PARAM.match(sqltype)
    if m is not None:
        finaltype = m.group('type').upper()
        size = m.group('size').strip().upper()
    else:
        size = ""
        finaltype = sqltype
    return finaltype, size


# noinspection PyUnusedLocal
def _get_cris_dd_row(column: Union[Column, CamcopsColumn, None],
                     recipient: "ExportRecipientInfo",
                     dest_dialect: Dialect = None) -> Dict:
    """
    Args:
        column:
            A column specification (or ``None`` to create a dummy dictionary).
        dest_dialect:
            The SQL dialect of the destination database. If ``None``, then
            MySQL is used as the default.

    Returns:
        An :class:`OrderedDict` with information for a CRIS data dictionary
        row.
    """
    dest_dialect = dest_dialect or MySQLDialect()  # MSDialect() for SQL Server
    valid_values = None
    if column is None:
        # Dummy row
        colname = None
        tablename = None
        taskname = None
        comment = None
        feft = None
        security_status = None
        finaltype = None
        tlfa = None
        size = None
    else:
        colname = column.name
        tablename = column.table.name
        taskname = tablename
        comment = column.comment
        coltype = coltype_as_typeengine(column.type)
        is_free_text = is_sqlatype_text_of_length_at_least(
            coltype, min_length=MIN_STRING_LENGTH_TO_CONSIDER_SCRUBBING)
        exempt_from_anonymisation = False
        identifies_patient = False

        if isinstance(column, CamcopsColumn):
            exempt_from_anonymisation = column.exempt_from_anonymisation
            identifies_patient = column.identifies_patient
            if column.permitted_value_checker:
                valid_values = column.permitted_value_checker.permitted_values_csv()  # noqa

        needs_scrubbing = is_free_text and not exempt_from_anonymisation

        # Tag list - fields anon
        tlfa = "Y" if needs_scrubbing else ""

        # Destination SQL type
        desttype = convert_sqla_type_for_dialect(
            coltype=coltype,
            dialect=dest_dialect,
            strip_collation=True,
            expand_for_scrubbing=needs_scrubbing,
        )
        destsqltype = desttype.compile(dialect=dest_dialect)
        finaltype, size = _get_type_size_as_text_from_sqltype(destsqltype)

        # Security status
        system_id = (
            colname == TABLET_ID_FIELD or
            colname.endswith("_id")
        )
        patient_idnum_field = colname.startswith(EXTRA_IDNUM_FIELD_PREFIX)
        internal_field = colname.startswith("_")
        if identifies_patient and (tablename == Patient.__tablename__ and
                                   colname == Patient.dob.name):
            security_status = 3  # truncate (e.g. DOB, postcode)
        elif identifies_patient and tablename == Patient.__tablename__:
            security_status = 2  # use to scrub
        elif system_id or internal_field or identifies_patient:
            security_status = 1  # drop (e.g. for pointless internal keys)
        else:
            security_status = 4  # bring through

        # Front end field type
        if system_id or patient_idnum_field:
            feft = 34  # patient ID; other internal keys
        elif is_sqlatype_date(coltype):
            feft = 4  # dates
        elif is_free_text:
            feft = 3  # giant free text, I think
        elif valid_values is not None:
            feft = 2  # picklist
        else:
            feft = 1  # text, numbers

    return OrderedDict([
        ("Tab", "CamCOPS"),
        ("Form name", taskname),
        ("CRIS tree label", colname),
        ("Source system table name", tablename),
        ("SQL column name", colname),
        ("Front end field type", feft),
        ("Valid values", valid_values),
        ("Result column name", colname),
        ("Family doc tab name", ""),
        ("Family doc form name", ""),
        ("Security status", security_status),
        ("Exclude", ""),
        ("End SQL Type", finaltype),
        ("Header field (Y/N)", ""),
        ("Header field name", ""),
        ("Header field active (Y/N)", ""),
        ("View name", ""),
        ("Exclude from family doc", ""),
        ("Tag list - fields anon", tlfa),
        ("Anon type", ""),  # formerly "Additional info"
        ("Form start date", ""),
        ("Form end date", ""),
        ("Source", ""),
        ("Size", size),
        ("Header logic", ""),
        ("Patient/contact", ""),
        ("Comments", comment),
    ])


def write_cris_data_dictionary(req: "CamcopsRequest",
                               recipient: "ExportRecipientInfo",
                               file: TextIO = sys.stdout) -> None:
    """
    Generates a draft CRIS data dictionary.

    CRIS is an anonymisation tool. See

    - Stewart R, Soremekun M, Perera G, Broadbent M, Callard F, Denis M, Hotopf
      M, Thornicroft G, Lovestone S (2009).
      The South London and Maudsley NHS Foundation Trust Biomedical Research
      Centre (SLAM BRC) case register: development and descriptive data.
      *BMC Psychiatry* 9: 51.
      https://www.ncbi.nlm.nih.gov/pubmed/19674459

    - Fernandes AC, Cloete D, Broadbent MT, Hayes RD, Chang CK, Jackson RG,
      Roberts A, Tsang J, Soncul M, Liebscher J, Stewart R, Callard F (2013).
      Development and evaluation of a de-identification procedure for a case
      register sourced from mental health electronic records.
      *BMC Med Inform Decis Mak.* 13: 71.
      https://www.ncbi.nlm.nih.gov/pubmed/23842533

    Args:
        req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        recipient: a :class:`camcops_server.cc_modules.cc_exportrecipientinfo.ExportRecipientInfo`
        file: output file
    """  # noqa
    dummy = _get_cris_dd_row(column=None, recipient=recipient)
    wr = csv.DictWriter(file, fieldnames=list(dummy.keys()))
    wr.writeheader()
    for col in _gen_columns_for_anon_staging_db(req, recipient):
        d = _get_cris_dd_row(column=col, recipient=recipient)
        wr.writerow(d)


# -----------------------------------------------------------------------------
# CRATE
# -----------------------------------------------------------------------------

def _get_crate_dd_row(column: Union[Column, CamcopsColumn, None],
                      recipient: "ExportRecipientInfo",
                      dest_dialect: Dialect = None,
                      src_db: str = "camcops",
                      default_indexlen: int = 100) -> Dict:
    """
    Args:
        column:
            A column specification (or ``None`` to create a dummy dictionary).
        recipient: 
            a :class:`camcops_server.cc_modules.cc_exportrecipientinfo.ExportRecipientInfo`
        dest_dialect:
            The SQL dialect of the destination database. If ``None``, then
            MySQL is used as the default.
        src_db:
            Value to be used for the "src_db" field.
        default_indexlen:
            Default index length for fields that require one.

    Returns:
        An :class:`OrderedDict` with information for a CRATE data dictionary
        row.
    """  # noqa
    dest_dialect = dest_dialect or MySQLDialect()
    exempt_from_anonymisation = False
    identifies_patient = False
    identifies_respondent = False
    force_include = False
    if column is None:
        # Dummy row
        colname = None
        tablename = None
        comment = None
        coltype = None
        needs_scrubbing = False
        desttype = None
        destsqltype = None
    else:
        colname = column.name
        tablename = column.table.name
        comment = column.comment
        coltype = coltype_as_typeengine(column.type)
        is_free_text = is_sqlatype_text_of_length_at_least(
            coltype, min_length=MIN_STRING_LENGTH_TO_CONSIDER_SCRUBBING)

        if isinstance(column, CamcopsColumn):
            exempt_from_anonymisation = column.exempt_from_anonymisation
            identifies_patient = column.identifies_patient
            force_include = column.include_in_anon_staging_db

        needs_scrubbing = is_free_text and not exempt_from_anonymisation
        desttype = convert_sqla_type_for_dialect(
            coltype=coltype,
            dialect=dest_dialect,
            strip_collation=True,
            expand_for_scrubbing=needs_scrubbing,
        )
        destsqltype = desttype.compile(dialect=dest_dialect)

    # src_flags
    src_flags = []  # type: List[str]
    primary_key = colname == FN_PK
    if primary_key:
        src_flags.extend(["K", "C"])
    primary_pid = (
        recipient.db_patient_id_per_row and  # otherwise just in PatientIdNum
        recipient.primary_idnum and
        colname == extra_id_colname(recipient.primary_idnum)
    )
    if primary_pid:
        src_flags.append("P")
    defines_primary_pids = False  # no single unique table for this...
    if defines_primary_pids:
        src_flags.append("*")
    master_pid = False  # not supported for now
    if master_pid:
        src_flags.append("M")

    # scrub_src
    if identifies_patient and tablename == Patient.__tablename__:
        scrub_src = "patient"
    elif identifies_respondent:
        scrub_src = "thirdparty"
    else:
        scrub_src = None

    # scrub_method
    scrub_method = None  # default is fine

    # Include in output?
    include = (
        force_include or
        primary_key or
        primary_pid or
        master_pid or
        not (identifies_patient or identifies_respondent)
    )

    # alter_method
    if needs_scrubbing:
        alter_method = "scrub"
    elif tablename == Patient.__tablename__ and colname == Patient.dob.name:
        alter_method = "truncate_date"
    else:
        alter_method = None

    # Indexing
    crate_index = None
    crate_indexlen = None
    if column is not None and column.index:
        crate_index = "U" if column.unique else "I"
        if does_sqlatype_require_index_len(desttype):
            crate_indexlen = default_indexlen

    return OrderedDict([
        ("src_db", src_db),
        ("src_table", tablename),
        ("src_field", colname),
        ("src_datatype", str(coltype)),
        ("src_flags", "".join(src_flags) if src_flags else None),
        ("scrub_src", scrub_src),
        ("scrub_method", scrub_method),
        ("decision", "include" if include else "OMIT"),
        ("inclusion_values", None),
        ("exclusion_values", None),
        ("alter_method", alter_method),
        ("dest_table", tablename),
        ("dest_field", colname),
        ("dest_datatype", destsqltype),
        ("index", crate_index),
        ("indexlen", crate_indexlen),
        ("comment", comment),
    ])


def write_crate_data_dictionary(req: "CamcopsRequest",
                                recipient: "ExportRecipientInfo",
                                file: TextIO = sys.stdout) -> None:
    """
    Generates a draft CRATE data dictionary.
    
    CRATE is an anonymisation tool. See:

    - Cardinal RN (2017).
      Clinical records anonymisation and text extraction (CRATE): an
      open-source software system.
      *BMC Medical Informatics and Decision Making* 17: 50.
      https://www.pubmed.gov/28441940;
      https://doi.org/10.1186/s12911-017-0437-1.

    - https://crateanon.readthedocs.io/

    Args:
        req: a :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        recipient: a :class:`camcops_server.cc_modules.cc_exportrecipientinfo.ExportRecipientInfo`
        file: output file
    """  # noqa
    dummy = _get_crate_dd_row(column=None, recipient=recipient)
    wr = csv.DictWriter(file, fieldnames=list(dummy.keys()))
    wr.writeheader()
    for col in _gen_columns_for_anon_staging_db(req, recipient):
        d = _get_crate_dd_row(column=col, recipient=recipient)
        wr.writerow(d)
