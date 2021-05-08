#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_xml.py

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

**XML helper functions/classes.**

"""

import base64
import datetime
import logging
from typing import Any, List, Optional, TYPE_CHECKING, Union
import xml.sax.saxutils

from cardinal_pythonlib.logs import BraceStyleAdapter
from cardinal_pythonlib.reprfunc import auto_repr
from cardinal_pythonlib.sqlalchemy.orm_inspect import gen_columns
import pendulum  # avoid name confusion with Date
from pendulum import DateTime as Pendulum
from semantic_version.base import Version
from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.type_api import TypeEngine

from camcops_server.cc_modules.cc_simpleobjects import XmlSimpleValue
from camcops_server.cc_modules.cc_sqla_coltypes import gen_camcops_blob_columns

if TYPE_CHECKING:
    from camcops_server.cc_modules.cc_request import CamcopsRequest  # noqa: E501,F401
    from camcops_server.cc_modules.cc_summaryelement import SummaryElement  # noqa: E501,F401

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Constants
# =============================================================================

XML_NAME_SNOMED_CODES = "snomed_ct_codes"

XML_NAMESPACES = [
    ' xmlns:xsi="https://www.w3.org/2001/XMLSchema-instance"'
    # ' xmlns:dt="https://www.w3.org/2001/XMLSchema-datatypes"'
]
XML_IGNORE_NAMESPACES = [
    'xmlns:mc="https://schemas.openxmlformats.org/markup-compatibility/2006"',
    'xmlns:ignore="https://camcops.readthedocs.org/ignore"',
    # ... actual URL unimportant
    'mc:Ignorable="ignore"'
]
# http://www.w3.org/TR/xmlschema-1/
# http://www.w3.org/TR/2004/REC-xmlschema-2-20041028/datatypes.html


class XmlDataTypes(object):
    """
    Constants representing standard XML data types.
    """
    BASE64BINARY = "base64Binary"
    BOOLEAN = "boolean"
    DATE = "date"
    DATETIME = "dateTime"
    DOUBLE = "double"
    INTEGER = "integer"
    STRING = "string"
    TIME = "time"


# =============================================================================
# XML element
# =============================================================================

class XmlElement(object):
    """
    Represents XML data in a tree.
    """
    def __init__(self, name: str, value: Any = None, datatype: str = None,
                 comment: str = None, literal: str = None) -> None:
        """
        Args:
            name: name of this XML element
            value: value of this element: may be a raw value or a list of
                :class:`camcops_server.cc_modules.cc_xml.XmlElement` objects
                (default: ``None``)
            datatype: data type of this element (default: ``None``)
            comment: description of this element (default: ``None``)
            literal: literal XML; overrides all other options
        """
        # Special: boolean requires lower case "true"/"false" (or 0/1)
        if datatype == XmlDataTypes.BOOLEAN and value is not None:
            value = str(value).lower()
        self.name = name
        self.value = value
        self.datatype = datatype
        self.comment = comment
        self.literal = literal

    def __repr__(self) -> str:
        """
        Shows just this element.
        """
        return auto_repr(self, with_addr=True)


class XmlLiteral(XmlElement):
    """
    Represents literal XML.
    """
    def __init__(self, literal: str) -> None:
        super().__init__(name="", literal=literal)


# =============================================================================
# Some literals
# =============================================================================

XML_COMMENT_ANCILLARY = XmlLiteral("<!-- Ancillary records -->")
XML_COMMENT_ANONYMOUS = XmlLiteral("<!-- Anonymous task; no patient info -->")
XML_COMMENT_BLOBS = XmlLiteral("<!-- Associated BLOBs -->")
XML_COMMENT_CALCULATED = XmlLiteral("<!-- Calculated fields -->")
XML_COMMENT_PATIENT = XmlLiteral("<!-- Associated patient details -->")
XML_COMMENT_SNOMED_CT = XmlLiteral("<!-- SNOMED-CT codes -->")
XML_COMMENT_SPECIAL_NOTES = XmlLiteral("<!-- Any special notes added -->")
XML_COMMENT_STORED = XmlLiteral("<!-- Stored fields -->")


# =============================================================================
# XML processing
# =============================================================================
# The xml.etree.ElementTree and lxml libraries can both do this sort of thing.
# However, they do look quite fiddly and we only want to create something
# simple. Therefore, let's roll our own:

def make_xml_branches_from_columns(
        obj,
        skip_fields: List[str] = None) -> List[XmlElement]:
    """
    Returns a list of XML branches, each an
    :class:`camcops_server.cc_modules.cc_xml.XmlElement`, from an SQLAlchemy
    ORM object, using the list of SQLAlchemy Column objects that
    define/describe its fields.

    Args:
        obj: the SQLAlchemy ORM object
        skip_fields: database column names to skip
    """
    skip_fields = skip_fields or []  # type: List[str]
    branches = []  # type: List[XmlElement]
    for attrname, column in gen_columns(obj):
        # log.debug("make_xml_branches_from_columns: {!r}", attrname)
        colname = column.name
        if colname in skip_fields:
            continue
        branches.append(XmlElement(
            name=colname,
            value=getattr(obj, attrname),
            datatype=get_xml_datatype_from_sqla_column(column),
            comment=column.comment
        ))
    return branches


def make_xml_branches_from_summaries(
        summaries: List["SummaryElement"],
        skip_fields: List[str] = None,
        sort_by_name: bool = True) -> List[XmlElement]:
    """
    Returns a list of XML branches, each an
    :class:`camcops_server.cc_modules.cc_xml.XmlElement`, from a list of
    summary data provided by a task.

    Args:
        summaries: list of :class:`SummaryElement` objects
        skip_fields: summary element names to skip
        sort_by_name: sort branches by element name?
    """
    skip_fields = skip_fields or []
    branches = []
    for s in summaries:
        name = s.name
        if name in skip_fields:
            continue
        branches.append(XmlElement(
            name=name,
            value=s.value,
            datatype=get_xml_datatype_from_sqla_column_type(s.coltype),
            comment=s.comment
        ))
    if sort_by_name:
        branches.sort(key=lambda el: el.name)
    return branches


def make_xml_branches_from_blobs(
        req: "CamcopsRequest",
        obj,
        skip_fields: List[str] = None) -> List[XmlElement]:
    """
    Return XML branches from those attributes of an SQLAlchemy ORM object
    (e.g. task) that represent BLOBs.

    Args:
        req: the :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`
        obj: the SQLAlchemy ORM object
        skip_fields: database column names to skip

    Returns:
        a list of :class:`camcops_server.cc_modules.cc_xml.XmlElement` objects

    """
    skip_fields = skip_fields or []  # type: List[str]
    branches = []  # type: List[XmlElement]
    for id_attrname, column in gen_camcops_blob_columns(obj):
        colname = column.name
        if colname in skip_fields:
            continue
        relationship_attr = column.blob_relationship_attr_name
        blob = getattr(obj, relationship_attr)
        branches.append(XmlElement(
            name=relationship_attr,
            value=None if blob is None else blob.get_xml_element(req),
            comment=column.comment,
        ))
    return branches


def xml_header(eol: str = '\n') -> str:
    """
    XML declaration header.
    """
    return f'<?xml version="1.0" encoding="UTF-8"?>{eol}'


def get_xml_datatype_from_sqla_column_type(coltype: TypeEngine) -> str:
    """
    Returns the XML schema datatype from an SQLAlchemy column type,
    such as ``Integer``. Compare :func:`get_xml_datatype_from_sqla_column`.
    """
    # http://www.xml.dvint.com/docs/SchemaDataTypesQR-2.pdf
    # http://www.w3.org/TR/2004/REC-xmlschema-2-20041028/datatypes.html
    pt = coltype.python_type
    # pt is a *type*, not an *instance* of that type, so we use issubclass:
    # Watch the order. Move from more specific to less specific.
    # For example, issubclass(bool, int) == True, so do bool first.
    if issubclass(pt, datetime.datetime) or issubclass(pt, Pendulum):
        return XmlDataTypes.DATETIME
    if issubclass(pt, datetime.date) or issubclass(pt, pendulum.Date):
        return XmlDataTypes.DATE
    if issubclass(pt, datetime.time) or issubclass(pt, pendulum.Time):
        return XmlDataTypes.TIME
    if issubclass(pt, bool):
        return XmlDataTypes.BOOLEAN
    if issubclass(pt, int):
        return XmlDataTypes.INTEGER
    if issubclass(pt, float):
        return XmlDataTypes.DOUBLE
    if issubclass(pt, str) or issubclass(pt, Version):
        return XmlDataTypes.STRING
    # BLOBs are handled separately.
    raise NotImplementedError(
        f"Don't know XML type for SQLAlchemy type {coltype!r} with Python "
        f"type {pt!r}")


def get_xml_datatype_from_sqla_column(column: Column) -> Optional[str]:
    """
    Returns the XML schema datatype from an SQLAlchemy Column, such as
    ``Integer()``. Compare :func:`get_xml_datatype_from_sqla_column_type`.
    """
    coltype = column.type  # type: TypeEngine
    return get_xml_datatype_from_sqla_column_type(coltype)


def get_xml_blob_element(name: str,
                         blobdata: Optional[bytes],
                         comment: str = None) -> XmlElement:
    """
    Returns an XmlElement representing a base-64-encoded BLOB.

    Args:
        name: XML element name
        blobdata: the raw binary, or ``None``
        comment: XML comment
    """
    if blobdata:
        # blobdata is raw binary
        b64bytes = base64.b64encode(blobdata)
        b64str = b64bytes.decode("ascii")
        value = b64str
    else:
        value = None
    return XmlElement(
        name=name,
        value=value,
        datatype=XmlDataTypes.BASE64BINARY,
        comment=comment
    )
    # http://www.w3.org/TR/2001/REC-xmlschema-2-20010502/#base64Binary


def xml_escape_value(value: str) -> str:
    """
    Escape a value for XML.
    """
    # http://stackoverflow.com/questions/1091945/
    # https://wiki.python.org/moin/EscapingXml
    return xml.sax.saxutils.escape(value)


def xml_quote_attribute(attr: str) -> str:
    """
    Escapes and quotes an attribute for XML.

    More stringent than value escaping.
    """
    return xml.sax.saxutils.quoteattr(attr)


def get_xml_tree(element: Union[XmlElement, XmlSimpleValue,
                                List[Union[XmlElement, XmlSimpleValue]]],
                 level: int = 0,
                 indent_spaces: int = 4,
                 eol: str = '\n',
                 include_comments: bool = False) -> str:
    # noinspection HttpUrlsUsage
    """
    Returns an :class:`camcops_server.cc_modules.cc_xml.XmlElement` as text.

    Args:
        element: root :class:`camcops_server.cc_modules.cc_xml.XmlElement`
        level: starting level/depth (used for recursion)
        indent_spaces: number of spaces to indent formatted XML
        eol: end-of-line string
        include_comments: include comments describing each field?

    We will represent NULL values with ``xsi:nil``, but this requires a
    namespace:

    - https://stackoverflow.com/questions/774192
    - http://books.xmlschemata.org/relaxng/relax-CHP-11-SECT-1.html

    Comments:

    - http://blog.galasoft.ch/posts/2010/02/quick-tip-commenting-out-properties-in-xaml/
    - https://stackoverflow.com/questions/2073140/

    Regarding newlines:

    - We do nothing special, i.e. newlines are provided in raw format.
    - However, some browsers may fail to display them correctly (i.e. they look
      like they're missing) -- e.g. Firefox, Chrome -- see
      https://stackoverflow.com/questions/2004386. Just try saving and
      inspecting the results with a text editor, or use the browser's "View
      Source" function (which, for Chrome, shows both newlines and line numbers
      too).

    """  # noqa
    xmltext = ""
    prefix = ' ' * level * indent_spaces

    if isinstance(element, XmlElement):

        if element.literal:
            # A user-inserted piece of XML. Insert, but indent.
            xmltext += prefix + element.literal + eol

        else:

            # Attributes
            namespaces = []
            if level == 0:  # root
                # Apply namespace to root element (will inherit):
                namespaces.extend(XML_NAMESPACES)
                if include_comments:
                    namespaces.extend(XML_IGNORE_NAMESPACES)
            namespace = " ".join(namespaces)
            if element.datatype:
                dt = f' xsi:type="{element.datatype}"'
            else:
                # log.warning("XmlElement has no datatype: {!r}", element)
                dt = ""
            cmt = ""
            if include_comments and element.comment:
                cmt = f' ignore:comment={xml_quote_attribute(element.comment)}'
            attributes = f"{namespace}{dt}{cmt}"

            # Assemble
            if element.value is None:
                # NULL handling
                xmltext += (
                    f'{prefix}<{element.name}{attributes} '
                    f'xsi:nil="true"/>{eol}'
                )
            else:
                complex_value = isinstance(element.value, XmlElement) \
                    or isinstance(element.value, list)
                value_to_recurse = element.value if complex_value else \
                    XmlSimpleValue(element.value)
                # ... XmlSimpleValue is a marker that subsequently
                # distinguishes things that were part of an XmlElement from
                # user-inserted raw XML.
                nl = eol if complex_value else ""
                pr2 = prefix if complex_value else ""
                v = get_xml_tree(
                    value_to_recurse,
                    level=level + 1,
                    indent_spaces=indent_spaces,
                    eol=eol,
                    include_comments=include_comments
                )
                xmltext += (
                    f'{prefix}<{element.name}{attributes}>{nl}'
                    f'{v}{pr2}</{element.name}>{eol}'
                )

    elif isinstance(element, list):
        for subelement in element:
            xmltext += get_xml_tree(subelement, level,
                                    indent_spaces=indent_spaces,
                                    eol=eol,
                                    include_comments=include_comments)
        # recursive

    elif isinstance(element, XmlSimpleValue):
        # The lowest-level thing a value. No extra indent.
        xmltext += xml_escape_value(str(element.value))

    else:
        raise ValueError(f"Bad value to get_xml_tree: {element!r}")

    return xmltext


def get_xml_document(root: XmlElement,
                     indent_spaces: int = 4,
                     eol: str = '\n',
                     include_comments: bool = False) -> str:
    """
    Returns an entire XML document as text, given the root
    :class:`camcops_server.cc_modules.cc_xml.XmlElement`.

    Args:
        root: root :class:`camcops_server.cc_modules.cc_xml.XmlElement`
        indent_spaces: number of spaces to indent formatted XML
        eol: end-of-line string
        include_comments: include comments describing each field?
    """
    if not isinstance(root, XmlElement):
        raise AssertionError("get_xml_document: root not an XmlElement; "
                             "XML requires a single root")
    return xml_header(eol) + get_xml_tree(
        root,
        indent_spaces=indent_spaces,
        eol=eol,
        include_comments=include_comments
    )
