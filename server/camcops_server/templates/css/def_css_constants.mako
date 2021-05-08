## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/css/def_css_constants.mako

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

Mako template containing Python to return one of several sets of constants
determining font sizes etc. (for PDF versus web HTML).

</%doc>

<%!

class CssArgsBase(object):
    SMALLFONTSIZE = "0.85em"
    TINYFONTSIZE = "0.7em"
    NORMALFONTSIZE = "1.0em"
    LARGEFONTSIZE = "1.2em"
    GIANTFONTSIZE = "1.4em"
    BANNERFONTSIZE = "1.6em"

    # Rules: line height is 1.1-1.2 * font size
    # ... but an em is related to the calculated font-size of the element,
    #   http://www.impressivewebs.com/understanding-em-units-css/
    # so it can always be 1.2:
    MAINLINEHEIGHT = "1.1em"
    SMALLLINEHEIGHT = "1.1em"
    TINYLINEHEIGHT = "1.0em"  # except this one
    LARGELINEHEIGHT = "1.1em"
    GIANTLINEHEIGHT = "1.1em"
    BANNERLINEHIGHT = "1.1em"
    TABLELINEHEIGHT = "1.1em"

    VSPACE_NORMAL = "0.5em"
    VSPACE_LARGE = "0.8em"

    SIGNATUREHEIGHT = "3em"


class CssVarArgsBase(CssArgsBase):
    MAINFONTSIZE = 'medium' 
    SMALLGAP = '2px' 
    ELEMENTGAP = '5px'
    TWICE_ELEMENTGAP = '10px'
    NORMALPAD = '2px' 
    TABLEPAD = '2px' 
    INDENT_NORMAL = '20px' 
    INDENT_LARGE = '75px' 
    THINLINE = '1px' 
    ZERO = '0px' 
    MAINMARGIN = '10px'
    BODYPADDING = '5px' 
    BANNER_PADDING = '25px' 
    
    PDF_LOGO_HEIGHT = '20mm'  # irrelevant for HTML; PDF only

    paged_media = False  # irrelevant for HTML; PDF only
    ORIENTATION = 'portrait'  # irrelevant for HTML; PDF only


class CssVarArgsWeb(CssVarArgsBase):
    pass


class CssVarArgsPdf(CssVarArgsBase):
    MAINFONTSIZE = '10pt'
    SMALLGAP = '0.2mm'
    ELEMENTGAP = '1mm'
    TWICE_ELEMENTGAP = '2mm'
    NORMALPAD = '0.5mm'
    TABLEPAD = '0.5mm'
    INDENT_NORMAL = '5mm'
    INDENT_LARGE = '10mm'
    THINLINE = '0.2mm'
    ZERO = '0mm'
    MAINMARGIN = '2cm'
    BODYPADDING = '0mm'
    BANNER_PADDING = '0.5cm'


class CssVarArgsPdfPortrait(CssVarArgsPdf):
    """"
    For PDF generation using paged media in portrait mode. NOT CURRENTLY USED.
    """
    paged_media = True
    ORIENTATION = 'portrait'


class CssVarArgsPdfLandscape(CssVarArgsPdf):
    """"
    For PDF generation using paged media in landscape mode. NOT CURRENTLY USED.
    """
    paged_media = True
    ORIENTATION = 'landscape'


class CssVarArgsPdfNoPagedMedia(CssVarArgsPdf):
    """"
    For PDF generation WITHOUT paged media.
    As for wkhtmltopdf, but see also below.
    """
    paged_media = False

class CssVarArgsWkhtmltopdf(CssVarArgsPdf):
    SMALLFONTSIZE = "0.85em"
    SMALLLINEHEIGHT = "1.1em"

%>

<%def name="_get_css_varargs(name)"><%
    if name == "web":
        return CssVarArgsWeb
    elif name == "pdf_no_paged_media":
        return CssVarArgsPdfNoPagedMedia
    elif name == "pdf_portrait":
        return CssVarArgsPdfPortrait
    elif name == "pdf_landscape":
        return CssVarArgsPdfLandscape
    elif name == "wkhtmltopdf_header_footer":
        return CssVarArgsWkhtmltopdf
    raise ValueError("Bug: bad argument to _get_css_varargs")
%></%def>
