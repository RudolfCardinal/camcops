## -*- coding: utf-8 -*-
<%doc>

camcops_server/templates/css/css_base.mako

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

CSS notes:

- Sequences of 4: top, right, bottom, left

- margin is outside, padding is inside

- ``#identifier``
- ``.class``

- http://www.w3schools.com/cssref/css_selectors.asp
- http://stackoverflow.com/questions/4013604
- http://stackoverflow.com/questions/6023419

</%doc>

<%page expression_filter="n"/>
## ... everything here is trusted.

<%
    # Passing parameters through templates:
    # https://groups.google.com/forum/#!topic/mako-discuss/U5jNLDqgppQ

    va = self.get_css_varargs()  # calls child
%>

/* Display PNG fallback image... */
svg img.svg {
    display: none;
}
img.pngfallback {
    display: inline;
}
/* ... unless our browser supports SVG */
html.svg svg img.svg {
    display: inline;
}
html.svg img.pngfallback {
    display: none;
}

/* Overall defaults */

body {
    font-family: Arial, Helvetica, sans-serif;
    font-size: ${va.MAINFONTSIZE};
    margin: ${va.ZERO} ${va.ZERO} ${va.ZERO} ${va.ZERO};  /* margin here affects form layout too */
    padding: ${va.BODYPADDING};
}
code {
    font-size: 0.8em;
    font-family: Consolas, Monaco, 'Lucida Console', 'Liberation Mono',
        'DejaVu Sans Mono', 'Bitstream Vera Sans Mono', 'Courier New';
    background-color: #eeeeee;
    padding: 1px 5px 1px 5px;
}
div {
    margin: ${va.ELEMENTGAP} ${va.ZERO} ${va.ELEMENTGAP} ${va.ZERO};
    padding: ${va.NORMALPAD};
}
em {
    color: rgb(0, 0, 255);  /* blue */
    font-style: normal;
}
h1 {
    font-size: ${va.GIANTFONTSIZE};
    line-height: ${va.GIANTLINEHEIGHT};
    font-weight: bold;
    margin: ${va.TWICE_ELEMENTGAP} ${va.ZERO} ${va.ZERO} ${va.ZERO};
}
h2 {
    font-size: ${va.LARGEFONTSIZE};
    line-height: ${va.LARGELINEHEIGHT};
    font-weight: bold;
    margin: ${va.ELEMENTGAP} ${va.ZERO} ${va.ZERO} ${va.ZERO};
}
h3 {
    font-size: ${va.NORMALFONTSIZE};
    line-height: ${va.MAINLINEHEIGHT};
    font-weight: bold;
    margin: ${va.ELEMENTGAP} ${va.ZERO} ${va.ZERO} ${va.ZERO};
}
h4 {
    font-size: ${va.NORMALFONTSIZE};
    line-height: ${va.MAINLINEHEIGHT};
    font-weight: normal;
    font-style: italic;
    margin: ${va.ELEMENTGAP} ${va.ZERO} ${va.ZERO} ${va.ZERO};
}
img {
    max-width: 100%;
    max-height: 100%;
}
ol, ul {
    margin: ${va.ELEMENTGAP};
}
p {
    margin: ${va.ELEMENTGAP} ${va.ZERO} ${va.ELEMENTGAP} ${va.ZERO};  /* see esp. p within div, such as task footnotes for web version */
}
sup, sub {
    font-size: 0.7em; /* 1 em is the size of the parent font */
    vertical-align: baseline;
    position: relative;
    top: -0.5em;
}
sub {
    top: 0.5em;
}
table {
    width: 100%; /* particularly for PDFs */
    vertical-align: top;
    border-collapse: collapse;
    border: ${va.THINLINE} solid black;
    padding: ${va.ZERO};
    margin: ${va.ELEMENTGAP} ${va.ZERO} ${va.ELEMENTGAP} ${va.ZERO};
}
tr, th, td {
    vertical-align: top;
    text-align: left;
    margin: ${va.ZERO};
    padding: ${va.TABLEPAD};
    border: ${va.THINLINE} solid black;
    line-height: ${va.TABLELINEHEIGHT};
}

blockquote > p {
    background: #eee;  /* light grey */
    padding: 15px;
}

blockquote > p::before {
    content: '\201C';  /* left double quote */
}

blockquote > p::after {
    content: '\201D';  /* right double quote */
}

/* Specific classes */

.badidpolicy_mild {
    background-color: rgb(255, 255, 153);  /* canary */
}
.badidpolicy_severe {
    background-color: rgb(255, 255, 0);  /* yellow */
}
.invalid_id_number_foreground {
    color: rgb(128, 0, 128);  /* purple */
    font-weight: bold;
}
.invalid_id_number_background {
    background-color: rgb(218, 112, 214);  /* orchid */
}
.banner {
    text-align: center;
    font-size: ${va.BANNERFONTSIZE};
    line-height: ${va.BANNERLINEHIGHT};
    padding: ${va.BANNER_PADDING};
    margin: ${va.ZERO};
}
.banner_referral_general_adult {
    background-color: rgb(255, 165, 0);
}
.banner_referral_old_age {
    background-color: rgb(0, 255, 127);
}
.banner_referral_substance_misuse {
    background-color: rgb(0, 191, 255);
}
.clinician {
    background-color: rgb(200, 255, 255);
}
table.clinician, table.clinician th, table.clinician td {
    border: ${va.THINLINE} solid black;
}
.copyright {
    font-style: italic;
    font-size: ${va.TINYFONTSIZE};
    line-height: ${va.TINYLINEHEIGHT};
    background-color: rgb(227, 227, 227);
}
.ctv_datelimit_start {
    /* line below */
    text-align: right;
    border-style: none none solid none;
    border-width: ${va.THINLINE};
    border-color: black;
}
.ctv_datelimit_end {
    /* line above */
    text-align: right;
    border-style: solid none none none;
    border-width: ${va.THINLINE};
    border-color: black;
}
.ctv_taskheading {
    background-color: rgb(200, 200, 255);
    font-weight: bold;
}
.ctv_fieldheading {
    background-color: rgb(200, 200, 200);
    font-weight: bold;
    font-style: italic;
    margin: ${va.ELEMENTGAP} ${va.ZERO} ${va.SMALLGAP} ${va.INDENT_NORMAL};
}
.ctv_fieldsubheading {
    background-color: rgb(200, 200, 200);
    font-style: italic;
    margin: ${va.ELEMENTGAP} ${va.ZERO} ${va.SMALLGAP} ${va.INDENT_NORMAL};
}
.ctv_fielddescription {
    font-style: italic;
    margin: ${va.ELEMENTGAP} ${va.ZERO} ${va.SMALLGAP} ${va.INDENT_NORMAL};
}
.ctv_fieldcontent {
    /* font-weight: bold; */
    margin: ${va.SMALLGAP} ${va.ZERO} ${va.ELEMENTGAP} ${va.INDENT_NORMAL};
}
.ctv_warnings {
    margin: ${va.ELEMENTGAP} ${va.ZERO} ${va.SMALLGAP} ${va.INDENT_NORMAL};
}
.error {
    color: rgb(255, 0, 0);
}
.explanation {
    background-color: rgb(200, 255, 200);
}
.filters {
    font-style: italic;
}
input[type="date"].form-control,
input[type="time"].form-control {
    line-height: normal; /* Bootstrap default looks wrong */
}
table.extradetail {
    border: ${va.THINLINE} solid black;
    background-color: rgb(210, 210, 210);
}
table.extradetail th {
    border: ${va.THINLINE} solid black;
    font-style: italic;
    font-weight: bold;
    font-size: ${va.TINYFONTSIZE};
}
table.extradetail td {
    border: ${va.THINLINE} solid black;
    font-size: ${va.TINYFONTSIZE};
}
.extradetail2 {
    background-color: rgb(240, 240, 240);
}
td.figure {
    padding: ${va.ZERO};
    background-color: rgb(255, 255, 255);
}
div.filter {
    /* for task filters */
    margin-left: ${va.INDENT_LARGE};
    padding: ${va.ZERO};
}
form.filter {
    /* for task filters */
    display: inline;
    margin: ${va.ZERO};
}
.flash_messages {
    padding:0;
    margin:0;
}
.footnotes {
    /* font-style: italic; */
    font-size: ${va.SMALLFONTSIZE};
    line-height: ${va.SMALLLINEHEIGHT};
}
.formtitle {
    font-size: ${va.LARGEFONTSIZE};
    color: rgb(34, 139, 34);
}
table.general, table.general th, table.general td {
    border: ${va.THINLINE} solid black;
}
table.general th.col1, table.general td.col1 {
    width: 22%;
}
table.general th.col2, table.general td.col2 {
    width: 78%;
}
.green {
    color: rgb(34, 139, 34);
}
p.hangingindent {
    padding-left: ${va.INDENT_NORMAL};
    text-indent: -${va.INDENT_NORMAL};
}
.heading {
    background-color: rgb(0, 0, 0);
    color: rgb(255, 255, 255);
    font-style: italic;
}
.highlight {
    background-color: rgb(255, 250, 205);
}
.important {
    color: rgb(64, 0, 192);
    font-weight: bold;
}
.ip_use_label {
    font-weight: bold;
}
.specialnote {
    background-color: rgb(255, 255, 153);
}
.live_on_tablet {
    background-color: rgb(216, 208, 245);
}
.incomplete {
    background-color: rgb(255, 165, 0);
}
.superuser {
    background-color: rgb(255, 192, 203);
}
p.indent {
    margin-left: ${va.INDENT_NORMAL};
}
div.indented {
    margin-left: ${va.INDENT_LARGE};
}
.navigation {
    background-color: rgb(200, 255, 200);
}
.noborder {
    border: none;
    /* NB also: hidden overrides none with border-collapse */
}
.noborderphoto {
    padding: ${va.ZERO};
    border: none;
}
.office {
    background-color: rgb(227, 227, 227);
    font-style: italic;
    font-size: ${va.TINYFONTSIZE};
    line-height: ${va.TINYLINEHEIGHT};
}
.patient {
    background-color: rgb(255, 200, 200);
}
.pdf_logo_header {
    width: 100%;
    border: none;
}
.pdf_logo_header table, .pdf_logo_header tr {
    width: 100%;
    border: none;
}
.pdf_logo_header .image_td {
    width: 45%;
    border: none;
}
.pdf_logo_header .centregap_td {
    width: 10%;
    border: none;
}
.pdf_logo_header .logo_left {
    float: left;
    max-width: 100%;
    max-height: ${va.PDF_LOGO_HEIGHT};
    height: auto;
    width: auto;
}
.pdf_logo_header .logo_right {
    float: right;
    max-width: 100%;
    max-height: ${va.PDF_LOGO_HEIGHT};
    height: auto;
    width: auto;
}
.photo {
    padding: ${va.ZERO};
}
.respondent {
    background-color: rgb(189, 183, 107);
}
table.respondent, table.respondent th, table.respondent td {
    border: ${va.THINLINE} solid black;
}
.signature_label {
    border: none;
    text-align: center;
}
.signature {
    line-height: ${va.SIGNATUREHEIGHT};
    border: ${va.THINLINE} solid black;
}
.smallprint {
    font-style: italic;
    font-size: ${va.SMALLFONTSIZE};
}
.subheading {
    background-color: rgb(200, 200, 200);
    font-style: italic;
}
.subsubheading {
    font-style: italic;
}
.summary {
    background-color: rgb(200, 200, 255);
}
table.summary, .summary th, .summary td {
    border: ${va.THINLINE} solid black;
}
table.taskconfig, .taskconfig th, .taskconfig td {
    border: ${va.THINLINE} solid black;
    background-color: rgb(230, 230, 230);
}
table.taskconfig th {
    font-style: italic; font-weight: normal;
}
table.taskdetail, .taskdetail th, .taskdetail td {
    border: ${va.THINLINE} solid black;
}
table.taskdetail th {
    font-weight: normal; font-style: italic;
}
table.taskdetail td {
    font-weight: normal;
}
.taskheader {
    background-color: rgb(200, 200, 200);
}
.trackerheader {
    font-size: ${va.TINYFONTSIZE};
    line-height: ${va.TINYLINEHEIGHT};
    background-color: rgb(218, 112, 240);
}
.tracker_all_consistent {
    font-style: italic;
    font-size: ${va.TINYFONTSIZE};
    line-height: ${va.TINYLINEHEIGHT};
    background-color: rgb(227, 227, 227);
}
.warning {
    background-color: rgb(255, 100, 100);
}

/* The next three: need both L/R to float and clear:both for IE */
.web_logo_header {
    display: block;
    overflow: hidden;
    width: 100%;
    border: none;
    clear: both;
}
/* ... overflow:hidden so the div expands to its floating contents */
.web_logo_header .logo_left {
    width: 45%;
    float: left;
    text-decoration: none;
    border: ${va.ZERO};
}
.web_logo_header .logo_right {
    width: 45%;
    float: right;
    text-decoration: none;
    border: ${va.ZERO};
}

/* For tables that will make it to a PDF, fix Weasyprint column widths.
   But not for all (e.g. webview task list) tables. */
table.clinician, table.extradetail, table.general,
        table.pdf_logo_header, table.summary,
        table.taskconfig, table.taskdetail,
        table.fixed {
    table-layout: fixed;
}

%if va.paged_media:

    /* PDF extras */
    #headerContent {
        font-size: ${va.SMALLFONTSIZE};
        line-height: ${va.SMALLLINEHEIGHT};
    }
    #footerContent {
        font-size: ${va.SMALLFONTSIZE};
        line-height: ${va.SMALLLINEHEIGHT};
    }

    /* PDF paging via CSS Paged Media */
    @page {
        size: A4 ${va.ORIENTATION};
        margin-left: ${va.MAINMARGIN};
        margin-right: ${va.MAINMARGIN};
        margin-top: ${va.MAINMARGIN};
        margin-bottom: ${va.MAINMARGIN};
        @frame header {
            /* -pdf-frame-border: 1; */ /* for debugging */
            -pdf-frame-content: headerContent;
            top: 1cm;
            margin-left: ${va.MAINMARGIN};
            margin-right: ${va.MAINMARGIN};
        }
        @frame footer {
            /* -pdf-frame-border: 1; */ /* for debugging */
            -pdf-frame-content: footerContent;
            bottom: 0.5cm; /* distance up from page's bottom margin? */
            height: 1cm; /* height of the footer */
            margin-left: ${va.MAINMARGIN};
            margin-right: ${va.MAINMARGIN};
        }
    }

    ## WEASYPRINT: NOT WORKING PROPERLY YET: WEASYPRINT DOESN'T YET SUPPORT RUNNING ELEMENTS
    ## http://librelist.com/browser//weasyprint/2013/7/4/header-and-footer-for-each-page/#abe45ec357d593df44ffca48253817ef
    ## http://weasyprint.org/docs/changelog/

%endif
