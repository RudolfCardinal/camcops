/*
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
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.
*/

#include "widgettestmenu.h"
#include <QDebug>
#include <QPushButton>
#include <QRandomGenerator>
#include <QtGlobal>
#include <QVBoxLayout>
#include "common/cssconst.h"
#include "common/textconst.h"
#include "common/uiconst.h"
#include "dbobjects/blob.h"
#include "diagnosis/icd10.h"
#include "graphics/graphicsfunc.h"
#include "layouts/flowlayouthfw.h"
#include "lib/debugfunc.h"
#include "lib/layoutdumper.h"
#include "lib/sizehelpers.h"
#include "lib/uifunc.h"
#include "menulib/menuitem.h"
#include "qcustomplot/qcustomplot.h"
#include "questionnairelib/mcqfunc.h"
#include "questionnairelib/quaudioplayer.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/qubutton.h"
#include "questionnairelib/qucanvas.h"
#include "questionnairelib/qucountdown.h"
#include "questionnairelib/qudatetime.h"
#include "questionnairelib/qudiagnosticcode.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/questionnaireheader.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/quhorizontalline.h"
#include "questionnairelib/quimage.h"
#include "questionnairelib/qulineedit.h"
#include "questionnairelib/qulineeditdouble.h"
#include "questionnairelib/qulineeditinteger.h"
#include "questionnairelib/qulineeditint64.h"
#include "questionnairelib/qulineeditnhsnumber.h"
#include "questionnairelib/qulineedituint64.h"
#include "questionnairelib/qupage.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qumcqgrid.h"
#include "questionnairelib/qumcqgriddouble.h"
#include "questionnairelib/qumcqgridsingleboolean.h"
#include "questionnairelib/qumultipleresponse.h"
#include "questionnairelib/quphoto.h"
#include "questionnairelib/qupickerinline.h"
#include "questionnairelib/qupickerpopup.h"
#include "questionnairelib/quslider.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/quspinboxdouble.h"
#include "questionnairelib/quspinboxinteger.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "questionnairelib/quthermometer.h"
#include "tasks/ace3.h"
#include "widgets/adjustablepie.h"
#include "widgets/aspectratiopixmap.h"
#include "widgets/basewidget.h"
#include "widgets/canvaswidget.h"
#include "widgets/clickablelabelnowrap.h"
#include "widgets/clickablelabelwordwrapwide.h"
#include "widgets/fixedareahfwtestwidget.h"
#include "widgets/fixedaspectratiohfwtestwidget.h"
#include "widgets/fixednumblockshfwtestwidget.h"
#include "widgets/horizontalline.h"
#include "widgets/imagebutton.h"
#include "widgets/labelwordwrapwide.h"
#include "widgets/svgwidgetclickable.h"
#include "widgets/thermometer.h"
#include "widgets/verticalline.h"
#include "widgets/verticalscrollarea.h"


const QString SHORT_TEXT("hello world");


const QString& sampleText(const bool long_text)
{
    return long_text ? TextConst::LOREM_IPSUM_1 : SHORT_TEXT;
}


WidgetTestMenu::WidgetTestMenu(CamcopsApp& app) :
    MenuWindow(app, uifunc::iconFilename(uiconst::CBS_SPANNER))

{
    const bool mandatory = true;

    FieldRef::GetterFunction getter1 = std::bind(&WidgetTestMenu::dummyGetter1,
                                                 this);
    FieldRef::SetterFunction setter1 = std::bind(&WidgetTestMenu::dummySetter1,
                                                 this, std::placeholders::_1);
    FieldRef::GetterFunction getter2 = std::bind(&WidgetTestMenu::dummyGetter2,
                                                 this);
    FieldRef::SetterFunction setter2 = std::bind(&WidgetTestMenu::dummySetter2,
                                                 this, std::placeholders::_1);
    m_fieldref_1 = FieldRefPtr(new FieldRef(getter1, setter1, mandatory));
    m_fieldref_2 = FieldRefPtr(new FieldRef(getter2, setter2, mandatory));

    m_blob = QSharedPointer<Blob>(new Blob(app, app.db()));  // specimen BLOB
    m_fieldref_blob = BlobFieldRefPtr(new BlobFieldRef(m_blob, true, true));
    // ... disable_creation_warning = true

    m_options_1.append(NameValuePair("Option A1", 1));
    m_options_1.append(NameValuePair("Option A2", 2));
    m_options_1.append(NameValuePair("Option A3", 3));

    m_options_2.append(NameValuePair("Option B1", 1));
    m_options_2.append(NameValuePair("Option B2", 2));

    m_options_3.append(NameValuePair("Option C1", 1));
    m_options_3.append(NameValuePair("Option C2 " + TextConst::LOREM_IPSUM_1, 2));
    m_options_3.append(NameValuePair("Option C3", 3));
}


QString WidgetTestMenu::title() const
{
    return tr("Widget tests");
}


void WidgetTestMenu::makeItems()
{
    const bool qutext_bold = false;

    const QSizePolicy fixed_fixed(QSizePolicy::Fixed, QSizePolicy::Fixed);
    const QSizePolicy expand_expand(QSizePolicy::Expanding, QSizePolicy::Expanding);
    const QSizePolicy expand_fixed_hfw = sizehelpers::expandingFixedHFWPolicy();
    // UiFunc::horizExpandingPreferredHFWPolicy();

    m_items = {
        // --------------------------------------------------------------------
        MenuItem("Qt widgets").setLabelOnly(),
        // --------------------------------------------------------------------
        MenuItem("QLabel (size policy = Fixed, Fixed / short / no word wrap)",
                 std::bind(&WidgetTestMenu::testQLabel, this,
                           fixed_fixed, false, false)),
        MenuItem("QLabel (size policy = Fixed, Fixed / long / no word wrap)",
                 std::bind(&WidgetTestMenu::testQLabel, this,
                           fixed_fixed, true, false)),
        MenuItem("QLabel (size policy = Fixed, Fixed / long / word wrap)",
                 std::bind(&WidgetTestMenu::testQLabel, this,
                           fixed_fixed, true, true)),
        MenuItem("QLabel (size policy = Expanding, Expanding / short / no word wrap)",
                 std::bind(&WidgetTestMenu::testQLabel, this,
                           expand_expand, false, false)),
        MenuItem("QLabel (size policy = Expanding, Expanding / long / no word wrap)",
                 std::bind(&WidgetTestMenu::testQLabel, this,
                           expand_expand, true, false)),
        MenuItem("QLabel (size policy = Expanding, Expanding / long / word wrap)",
                 std::bind(&WidgetTestMenu::testQLabel, this,
                           expand_expand, true, true)),
        MenuItem("QLabel (size policy = Expanding, Fixed, heightForWidth / short / no word wrap)",
                 std::bind(&WidgetTestMenu::testQLabel, this,
                           expand_fixed_hfw, false, false)),
        MenuItem("QLabel (size policy = Expanding, Fixed, heightForWidth / long / no word wrap)",
                 std::bind(&WidgetTestMenu::testQLabel, this,
                           expand_fixed_hfw, true, false)),
        MenuItem("QLabel (size policy = Expanding, Fixed, heightForWidth / long / word wrap)",
                 std::bind(&WidgetTestMenu::testQLabel, this,
                           expand_fixed_hfw, true, true)),
        MenuItem("QPushButton (size policy = Fixed, Fixed)",
                 std::bind(&WidgetTestMenu::testQPushButton, this, fixed_fixed)),
        MenuItem("QPushButton (size policy = Expanding, Expanding)",
                 std::bind(&WidgetTestMenu::testQPushButton, this, expand_expand)),

        // --------------------------------------------------------------------
        MenuItem("Low-level widgets").setLabelOnly(),
        // --------------------------------------------------------------------
        MenuItem("AdjustablePie (1)",
                 std::bind(&WidgetTestMenu::testAdjustablePie, this, 1, true)),
        MenuItem("AdjustablePie (2)",
                 std::bind(&WidgetTestMenu::testAdjustablePie, this, 2, true)),
        MenuItem("AdjustablePie (3, don't rotate labels)",
                 std::bind(&WidgetTestMenu::testAdjustablePie, this, 3, false)),
        MenuItem("AdjustablePie (3, rotate labels)",
                 std::bind(&WidgetTestMenu::testAdjustablePie, this, 3, true)),
        MenuItem("AspectRatioPixmap (should maintain aspect ratio and resize from 0 to its intrinsic size)",
                 std::bind(&WidgetTestMenu::testAspectRatioPixmap, this)),
        MenuItem("BooleanWidget (appearance=CheckBlack)",
                 std::bind(&WidgetTestMenu::testBooleanWidget, this,
                           BooleanWidget::Appearance::CheckBlack, false)),
        MenuItem("BooleanWidget (appearance=CheckRed)",
                 std::bind(&WidgetTestMenu::testBooleanWidget, this,
                           BooleanWidget::Appearance::CheckRed, false)),
        MenuItem("BooleanWidget (appearance=Radio)",
                 std::bind(&WidgetTestMenu::testBooleanWidget, this,
                           BooleanWidget::Appearance::Radio, false)),
        MenuItem("BooleanWidget (appearance=Text, short text)",
                 std::bind(&WidgetTestMenu::testBooleanWidget, this,
                           BooleanWidget::Appearance::Text, false)),
        MenuItem("BooleanWidget (appearance=Text, long text)",
                 std::bind(&WidgetTestMenu::testBooleanWidget, this,
                           BooleanWidget::Appearance::Text, true)),
        MenuItem("CanvasWidget (allow_shrink=false)",
                 std::bind(&WidgetTestMenu::testCanvasWidget, this, false)),
        MenuItem("CanvasWidget (allow_shrink=true)",
                 std::bind(&WidgetTestMenu::testCanvasWidget, this, true)),
        MenuItem("ClickableLabelNoWrap (short text) (not generally used: no word wrap)",
                 std::bind(&WidgetTestMenu::testClickableLabelNoWrap, this, false)),
        MenuItem("ClickableLabelNoWrap (long text) (not generally used: no word wrap)",
                 std::bind(&WidgetTestMenu::testClickableLabelNoWrap, this, true)),
        MenuItem("ClickableLabelWordWrapWide (short text)",
                 std::bind(&WidgetTestMenu::testClickableLabelWordWrapWide, this, false)),
        MenuItem("ClickableLabelWordWrapWide (long text)",
                 std::bind(&WidgetTestMenu::testClickableLabelWordWrapWide, this, true)),
        MenuItem("FixedAreaHfwTestWidget",
                 std::bind(&WidgetTestMenu::testFixedAreaHfwTestWidget, this)),
        MenuItem("FixedAspectRatioHfwTestWidget",
                 std::bind(&WidgetTestMenu::testFixedAspectRatioHfwTestWidget, this)),
        MenuItem("FixedNumBlocksHfwTestWidget",
                 std::bind(&WidgetTestMenu::testFixedNumBlocksHfwTestWidget, this)),
        MenuItem("HorizontalLine",
                 std::bind(&WidgetTestMenu::testHorizontalLine, this)),
        MenuItem("ImageButton",
                 std::bind(&WidgetTestMenu::testImageButton, this)),
        MenuItem("LabelWordWrapWide (short text)",
                 std::bind(&WidgetTestMenu::testLabelWordWrapWide, this, false, true, false)),
        MenuItem("LabelWordWrapWide (long text) (within QVBoxLayout)",
                 std::bind(&WidgetTestMenu::testLabelWordWrapWide, this, true, false, false)),
        MenuItem("LabelWordWrapWide (long text) (within VBoxLayoutHfw)",
                 std::bind(&WidgetTestMenu::testLabelWordWrapWide, this, true, true, false)),
        MenuItem("LabelWordWrapWide (long text) (within VBoxLayoutHfw + icons)",
                 std::bind(&WidgetTestMenu::testLabelWordWrapWide, this, true, true, true)),
        MenuItem("SvgWidgetClickable",
                 std::bind(&WidgetTestMenu::testSvgWidgetClickable, this)),
        MenuItem("Thermometer",
                 std::bind(&WidgetTestMenu::testThermometer, this)),
        MenuItem("VerticalLine",
                 std::bind(&WidgetTestMenu::testVerticalLine, this)),

        // --------------------------------------------------------------------
        MenuItem("Layouts and the like").setLabelOnly(),
        // --------------------------------------------------------------------
        MenuItem("FlowLayout (containing fixed-size icons, left-align)",
                 std::bind(&WidgetTestMenu::testFlowLayout, this, 5, false, Qt::AlignLeft)),
        MenuItem("FlowLayout (containing fixed-size icons, centre-align)",
                 std::bind(&WidgetTestMenu::testFlowLayout, this, 5, false, Qt::AlignCenter)),
        MenuItem("FlowLayout (containing fixed-size icons, right-align)",
                 std::bind(&WidgetTestMenu::testFlowLayout, this, 5, false, Qt::AlignRight)),
        MenuItem("FlowLayout (containing word-wrapped text)",
                 std::bind(&WidgetTestMenu::testFlowLayout, this, 5, true, Qt::AlignLeft)),
        MenuItem("FlowLayout (containing FixedNumBlocksHfwTestWidget)",
                 std::bind(&WidgetTestMenu::testFlowLayoutFixedNumBlocksHfwTestWidget, this, 4)),
        MenuItem("FlowLayout (containing mixture)",
                 std::bind(&WidgetTestMenu::testFlowLayoutMixture, this)),
        MenuItem("BaseWidget (with short text)",
                 std::bind(&WidgetTestMenu::testBaseWidget, this, false)),
        MenuItem("BaseWidget (with long text)",
                 std::bind(&WidgetTestMenu::testBaseWidget, this, true)),
        MenuItem("VBoxLayout (either QVBoxLayout or VBoxLayoutHfw), "
                 "with 2 x LabelWordWrapWide (short text)",
                 std::bind(&WidgetTestMenu::testVBoxLayout, this, false)),
        MenuItem("VBoxLayout (either QVBoxLayout or VBoxLayoutHfw), "
                 "with 2 x LabelWordWrapWide (long text)",
                 std::bind(&WidgetTestMenu::testVBoxLayout, this, true)),
        MenuItem("HBoxLayoutHfw, icon-stretch-icon",
                 std::bind(&WidgetTestMenu::testHBoxLayoutHfwStretch, this)),
        MenuItem("GridLayoutHfw (example 1: fixed-size icons and word-wrapping text)",
                 std::bind(&WidgetTestMenu::testGridLayoutHfw, this, 1)),
        MenuItem("GridLayoutHfw (example 2: 4 x short text, an example with "
                 "height-for-width items only)",
                 std::bind(&WidgetTestMenu::testGridLayoutHfw, this, 2)),
        MenuItem("GridLayoutHfw (example 3: approximating QuMcqGrid)",
                 std::bind(&WidgetTestMenu::testGridLayoutHfw, this, 3)),
        MenuItem("GridLayoutHfw (example 4: 3 x ImageButton, an example with "
                 "no height-for-width items)",
                 std::bind(&WidgetTestMenu::testGridLayoutHfw, this, 4)),
        MenuItem("GridLayoutHfw (example 5: fixed-size icons and a "
                "FixedNumBlocksHfwTestWidget)",
                 std::bind(&WidgetTestMenu::testGridLayoutHfw, this, 5)),
        MenuItem("VerticalScrollArea (QVBoxLayout, fixed-size icons)",
                 std::bind(&WidgetTestMenu::testVerticalScrollAreaSimple, this)),
        MenuItem("VerticalScrollArea (VBoxLayout, short text)",
                 std::bind(&WidgetTestMenu::testVerticalScrollAreaComplex, this, false)),
        MenuItem("VerticalScrollArea (VBoxLayout, long text)",
                 std::bind(&WidgetTestMenu::testVerticalScrollAreaComplex, this, true)),
        MenuItem("VerticalScrollArea (FixedAreaHfwTestWidget)",
                 std::bind(&WidgetTestMenu::testVerticalScrollAreaFixedAreaHfwWidget, this)),
        MenuItem("VerticalScrollArea (AspectRatioPixmap)",
                 std::bind(&WidgetTestMenu::testVerticalScrollAreaAspectRatioPixmap, this)),
        MenuItem("VerticalScrollArea (GridLayout)",
                 std::bind(&WidgetTestMenu::testVerticalScrollGridLayout, this)),

        MenuItem("Large-scale widgets").setLabelOnly(),
        MenuItem("MenuItem",
                 std::bind(&WidgetTestMenu::testMenuItem, this)),
        MenuItem("QuestionnaireHeader",
                 std::bind(&WidgetTestMenu::testQuestionnaireHeader, this)),
        MenuItem("Empty questionnaire (short title)",
                 std::bind(&WidgetTestMenu::testQuestionnaire, this, false, false)),
        MenuItem("Empty questionnaire (long title)",
                 std::bind(&WidgetTestMenu::testQuestionnaire, this, true, false)),
        MenuItem("Empty questionnaire (long title + as OpenableWidget)",
                 std::bind(&WidgetTestMenu::testQuestionnaire, this, true, true)),
        /*
        MenuItem("Dummy ACE-III [will CRASH as no patient; layout testing only]"),
                 std::bind(&WidgetTestMenu::testAce3, this)),
        */

        // --------------------------------------------------------------------
        MenuItem("Questionnaire element widgets").setLabelOnly(),
        // --------------------------------------------------------------------
        MenuItem("QuAudioPlayer",
                 std::bind(&WidgetTestMenu::testQuAudioPlayer, this)),
        MenuItem("QuBoolean (as_text_button=false, short text)",
                 std::bind(&WidgetTestMenu::testQuBoolean, this, false, false, false)),
        MenuItem("QuBoolean (as_text_button=false, short text, false_appears_blank)",
                 std::bind(&WidgetTestMenu::testQuBoolean, this, false, false, true)),
        MenuItem("QuBoolean (as_text_button=false, long text)",
                 std::bind(&WidgetTestMenu::testQuBoolean, this, false, true, false)),
        MenuItem("QuBoolean (as_text_button=true, short text)",
                 std::bind(&WidgetTestMenu::testQuBoolean, this, true, false, false)),
        MenuItem("QuBoolean (as_text_button=true, long text)",
                 std::bind(&WidgetTestMenu::testQuBoolean, this, true, true, false)),
        MenuItem("QuButton",
                 std::bind(&WidgetTestMenu::testQuButton, this)),
        MenuItem("QuCanvas",
                 std::bind(&WidgetTestMenu::testQuCanvas, this)),
        MenuItem("QuCountdown",
                 std::bind(&WidgetTestMenu::testQuCountdown, this)),
        MenuItem("QuDateTime",
                 std::bind(&WidgetTestMenu::testQuDateTime, this)),
        MenuItem("QuDateTime (limited to 20th century)",
                 std::bind(&WidgetTestMenu::testQuDateTimeLimited, this)),
        MenuItem("QuDiagnosticCode (NB iffy display if you select one!)",
                 std::bind(&WidgetTestMenu::testQuDiagnosticCode, this)),
        MenuItem("QuHeading (short text)",
                 std::bind(&WidgetTestMenu::testQuHeading, this, false)),
        MenuItem("QuHeading (long text)",
                 std::bind(&WidgetTestMenu::testQuHeading, this, true)),
        MenuItem("QuHorizontalLine",
                 std::bind(&WidgetTestMenu::testQuHorizontalLine, this)),
        MenuItem("QuImage",
                 std::bind(&WidgetTestMenu::testQuImage, this)),
        MenuItem("QuLineEdit",
                 std::bind(&WidgetTestMenu::testQuLineEdit, this)),
        MenuItem("QuLineEditDouble",
                 std::bind(&WidgetTestMenu::testQuLineEditDouble, this)),
        MenuItem("QuLineEditInteger",
                 std::bind(&WidgetTestMenu::testQuLineEditInteger, this)),
        MenuItem("QuLineEditLongLong",
                 std::bind(&WidgetTestMenu::testQuLineEditLongLong, this)),
        MenuItem("QuLineEditNHSNumber",
                 std::bind(&WidgetTestMenu::testQuLineEditNHSNumber, this)),
        MenuItem("QuLineEditULongLong",
                 std::bind(&WidgetTestMenu::testQuLineEditULongLong, this)),
        MenuItem("QuMCQ (horizontal=false, short text)",
                 std::bind(&WidgetTestMenu::testQuMCQ, this, false, false, false)),
        MenuItem("QuMCQ (horizontal=false, long text)",
                 std::bind(&WidgetTestMenu::testQuMCQ, this, false, true, false)),
        MenuItem("QuMCQ (horizontal=true, short text)",
                 std::bind(&WidgetTestMenu::testQuMCQ, this, true, false, false)),
        MenuItem("QuMCQ (horizontal=true, long text)",
                 std::bind(&WidgetTestMenu::testQuMCQ, this, true, true, false)),
        MenuItem("QuMCQ (horizontal=true, short text, as text button)",
                 std::bind(&WidgetTestMenu::testQuMCQ, this, true, false, true)),
        MenuItem("QuMCQGrid (expand=false, example=1)",
                 std::bind(&WidgetTestMenu::testQuMCQGrid, this, false, 1)),
        MenuItem("QuMCQGrid (expand=true, example=1)",
                 std::bind(&WidgetTestMenu::testQuMCQGrid, this, true, 1)),
        MenuItem("QuMCQGrid (expand=true, example=2)",
                 std::bind(&WidgetTestMenu::testQuMCQGrid, this, true, 2)),
        MenuItem("QuMCQGrid (expand=true, example=3)",
                 std::bind(&WidgetTestMenu::testQuMCQGrid, this, true, 3)),
        MenuItem("QuMCQGridDouble (expand=false)",
                 std::bind(&WidgetTestMenu::testQuMCQGridDouble, this, false)),
        MenuItem("QuMCQGridDouble (expand=true)",
                 std::bind(&WidgetTestMenu::testQuMCQGridDouble, this, true)),
        MenuItem("QuMCQGridSingleBoolean (expand=false)",
                 std::bind(&WidgetTestMenu::testQuMCQGridSingleBoolean, this, false)),
        MenuItem("QuMCQGridSingleBoolean (expand=true)",
                 std::bind(&WidgetTestMenu::testQuMCQGridSingleBoolean, this, true)),
        MenuItem("QuMultipleResponse (horizontal=false, short text)",
                 std::bind(&WidgetTestMenu::testQuMultipleResponse, this, false, false)),
        MenuItem("QuMultipleResponse (horizontal=false, long text)",
                 std::bind(&WidgetTestMenu::testQuMultipleResponse, this, false, true)),
        MenuItem("QuMultipleResponse (horizontal=true, short text)",
                 std::bind(&WidgetTestMenu::testQuMultipleResponse, this, true, false)),
        MenuItem("QuMultipleResponse (horizontal=true, long text)",
                 std::bind(&WidgetTestMenu::testQuMultipleResponse, this, true, true)),
        // Not yet fixed:
        // - widget pops up in modal window
        // - camera then gets opened in window belonging to main window
        //   ... but its UI input is blocked, so we get nowhere
        //MenuItem("QuPhoto",
        //         std::bind(&WidgetTestMenu::testQuPhoto, this)),
        MenuItem("QuPickerInline",
                 std::bind(&WidgetTestMenu::testQuPickerInline, this)),
        MenuItem("QuPickerPopup",
                 std::bind(&WidgetTestMenu::testQuPickerPopup, this)),
        MenuItem("QuSlider (horizontal=false)",
                 std::bind(&WidgetTestMenu::testQuSlider, this, false)),
        MenuItem("QuSlider (horizontal=true)",
                 std::bind(&WidgetTestMenu::testQuSlider, this, true)),
        MenuItem("QuSpacer",
                 std::bind(&WidgetTestMenu::testQuSpacer, this)),
        MenuItem("QuSpinBoxDouble",
                 std::bind(&WidgetTestMenu::testQuSpinBoxDouble, this)),
        MenuItem("QuSpinBoxInteger",
                 std::bind(&WidgetTestMenu::testQuSpinBoxInteger, this)),
        MenuItem("QuText (short text)",
                 std::bind(&WidgetTestMenu::testQuText, this, false, qutext_bold)),
        MenuItem("QuText (long text)",
                 std::bind(&WidgetTestMenu::testQuText, this, true, qutext_bold)),
        MenuItem("QuTextEdit",
                 std::bind(&WidgetTestMenu::testQuTextEdit, this)),
        MenuItem("QuThermometer",
                 std::bind(&WidgetTestMenu::testQuThermometer, this)),

        // --------------------------------------------------------------------
        MenuItem("Graphs").setLabelOnly(),
        // --------------------------------------------------------------------
        MenuItem("Test QCustomPlot #1: y = x<sup>2</sup>",
                 std::bind(&WidgetTestMenu::testQCustomPlot1, this)),
        MenuItem("Test QCustomPlot #2: date axis",
                 std::bind(&WidgetTestMenu::testQCustomPlot2, this)),

    };
}


QVariant WidgetTestMenu::dummyGetter1() const
{
    return m_dummy_value_1;
}


bool WidgetTestMenu::dummySetter1(const QVariant& value)
{
    const bool changed = (value != m_dummy_value_1);
    if (changed) {
        m_dummy_value_1 = value;
    }
    return changed;
}


QVariant WidgetTestMenu::dummyGetter2() const
{
    return m_dummy_value_2;
}


bool WidgetTestMenu::dummySetter2(const QVariant& value)
{
    const bool changed = (value != m_dummy_value_2);
    if (changed) {
        m_dummy_value_2 = value;
    }
    return changed;
}


void WidgetTestMenu::dummyAction()
{
    uifunc::alert("Action!");
}


void WidgetTestMenu::testQuestionnaireElement(QuElement* element)
{
    Questionnaire questionnaire(m_app);
    QWidget* widget = element->widget(&questionnaire);
    if (!widget) {
        uifunc::alert("Element failed to create a widget!");
        return;
    }
    layoutdumper::DumperConfig config;
    QString stylesheet(m_app.getSubstitutedCss(uiconst::CSS_CAMCOPS_QUESTIONNAIRE));
    debugfunc::debugWidget(widget, false, false, config, true, &stylesheet);
}


// ============================================================================
// Qt widgets
// ============================================================================

void WidgetTestMenu::testQLabel(const QSizePolicy& policy,
                                const bool long_text, const bool word_wrap)
{
    auto widget = new QLabel(sampleText(long_text));
    widget->setWordWrap(word_wrap);
    widget->setSizePolicy(policy);
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testQPushButton(const QSizePolicy& policy)
{
    auto widget = new QPushButton("Hello");
    widget->setSizePolicy(policy);
    // http://stackoverflow.com/questions/21367260/qt-making-a-qpushbutton-fill-layout-cell
    connect(widget, &QPushButton::clicked,
            this, &WidgetTestMenu::dummyAction);
    debugfunc::debugWidget(widget);
}


// ============================================================================
// Low-level widgets
// ============================================================================

void WidgetTestMenu::testAdjustablePie(const int n, const bool rotate_labels)
{
    auto pie = new AdjustablePie(n);
    const qreal prop = 1.0 / n;
    const QVector<qreal> proportions(n, prop);
    pie->setProportions(proportions);
    pie->setLabelRotation(rotate_labels);
    if (n == 1) {
        pie->setCentreLabel("Whole pie!");
    }
    for (int i = 0; i < n; ++i) {
        pie->setLabel(i, QString("Sector %1").arg(i));
    }
    debugfunc::debugWidget(pie);
}


void WidgetTestMenu::testAspectRatioPixmap()
{
    auto widget = new AspectRatioPixmap();
    const QPixmap pixmap = uifunc::getPixmap(uifunc::iconFilename(uiconst::ICON_CAMCOPS));
    widget->setPixmap(pixmap);
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testBooleanWidget(
        const BooleanWidget::Appearance appearance,
        const bool long_text)
{
    auto widget = new BooleanWidget();
    const bool big = true;
    const bool as_text_button = (appearance == BooleanWidget::Appearance::Text);
    widget->setAppearance(appearance);
    widget->setSize(big);
    widget->setValue(true, true);
    if (as_text_button) {
        widget->setText(long_text ? TextConst::LOREM_IPSUM_2 : "BooleanWidget");
    }
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testCanvasWidget(const bool allow_shrink)
{
    const QSize size(400, 400);
    auto widget = new CanvasWidget(size);
    const QImage img(size, QImage::Format_RGB32);
    widget->setImage(img);
    widget->setAllowShrink(allow_shrink);
    widget->clear(Qt::white);
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testClickableLabelNoWrap(const bool long_text)
{
    auto widget = new ClickableLabelNoWrap(sampleText(long_text));
    connect(widget, &QAbstractButton::clicked,
            this, &WidgetTestMenu::dummyAction);
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testClickableLabelWordWrapWide(const bool long_text)
{
    auto widget = new ClickableLabelWordWrapWide(sampleText(long_text));
    connect(widget, &QAbstractButton::clicked,
            this, &WidgetTestMenu::dummyAction);
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testFixedAreaHfwTestWidget()
{
    auto widget = new FixedAreaHfwTestWidget();
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testFixedAspectRatioHfwTestWidget()
{
    auto widget = new FixedAspectRatioHfwTestWidget();
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testFixedNumBlocksHfwTestWidget()
{
    auto widget = new FixedNumBlocksHfwTestWidget();
    const bool use_hfw_layout = true;
    debugfunc::debugWidget(widget,
                           false, true, layoutdumper::DumperConfig(),
                           use_hfw_layout);
}


void WidgetTestMenu::testHorizontalLine()
{
    const int width = 4;
    auto widget = new HorizontalLine(width);
    widget->setStyleSheet("background-color: black;");
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testImageButton()
{
    auto widget = new ImageButton(uiconst::CBS_ADD);
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testLabelWordWrapWide(
        const bool long_text, const bool use_hfw_layout,
        const bool with_icons)
{
    auto label = new LabelWordWrapWide(sampleText(long_text));
    QWidget* widget;
    if (with_icons) {
        widget = new QWidget();
        auto layout = new HBoxLayout(widget);
        layout->addWidget(new ImageButton(uiconst::CBS_ADD));
        layout->addWidget(label);
        layout->addWidget(new ImageButton(uiconst::CBS_ADD));
    } else {
        widget = label;
    }
    const bool set_background_by_name = false;
    const bool set_background_by_stylesheet = true;
    layoutdumper::DumperConfig config;
    debugfunc::debugWidget(widget, set_background_by_name,
                           set_background_by_stylesheet,
                           config,
                           use_hfw_layout);
}


void WidgetTestMenu::testSvgWidgetClickable()
{
    auto widget = new SvgWidgetClickable();
    widget->setSvgFromString(graphicsfunc::TEST_SVG);
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testThermometer()
{
    const QStringList left_strings{
        "row zero", "row one", "row two", "row three", "row four", "row five",
        "row six", "row seven", "row eight", "row nine", "row ten"
    };
    const QStringList right_strings{
        "10", "9", "8", "7", "6", "5", "4", "3", "2", "1", "0"
    };
    QVector<QPixmap> active_images;
    QVector<QPixmap> inactive_images;
    for (int i = 10; i >= 0; --i) {
        const QString active_filename = uifunc::resourceFilename(
                    QString("distressthermometer/dt_sel_%1.png").arg(i));
        const QString inactive_filename = uifunc::resourceFilename(
                    QString("distressthermometer/dt_unsel_%1.png").arg(i));
        active_images.append(uifunc::getPixmap(active_filename));
        inactive_images.append(uifunc::getPixmap(inactive_filename));
    }
    auto widget = new Thermometer(
        active_images, inactive_images,
        &left_strings, &right_strings,
        1,  // left_string_scale
        2,  // image_scale
        1,  // right_string_scale
        true,  // allow_deselection
        false,  // read_only
        true,  // rescale
        0.25,  // rescale_factor
        4,  // text_gap_px
        nullptr  // parent
    );
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testVerticalLine()
{
    const int width = 4;
    auto widget = new VerticalLine(width);
    widget->setStyleSheet("background-color: black;");
    debugfunc::debugWidget(widget);
}


// ============================================================================
// Layouts and the like
// ============================================================================

void WidgetTestMenu::testFlowLayout(const int n_icons, const bool text,
                                    const Qt::Alignment halign)
{
    auto widget = new QWidget();
    widget->setSizePolicy(sizehelpers::preferredPreferredHFWPolicy());
    auto layout = new FlowLayoutHfw();
    layout->setHorizontalAlignmentOfContents(halign);
    widget->setLayout(layout);
    for (int i = 0; i < n_icons; ++i) {
        if (text) {
            layout->addWidget(new LabelWordWrapWide("A few words"));
        } else {
            QLabel* icon = uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD));
            layout->addWidget(icon);
        }
    }
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testFlowLayoutFixedNumBlocksHfwTestWidget(const int n)
{
    auto widget = new QWidget();
    widget->setSizePolicy(sizehelpers::preferredPreferredHFWPolicy());
    auto layout = new FlowLayoutHfw();
    widget->setLayout(layout);
    for (int i = 0; i < n; ++i) {
        layout->addWidget(new FixedNumBlocksHfwTestWidget());
    }
    const bool use_hfw_layout = true;  // just for experimentation
    debugfunc::debugWidget(widget, false, false, layoutdumper::DumperConfig(),
                           use_hfw_layout);
}


void WidgetTestMenu::testFlowLayoutMixture()
{
    auto widget = new QWidget();
    widget->setSizePolicy(sizehelpers::preferredPreferredHFWPolicy());
    auto layout = new FlowLayoutHfw();
    widget->setLayout(layout);
    for (int i = 0; i < 4; ++i) {
        layout->addWidget(new FixedAspectRatioHfwTestWidget());
        layout->addWidget(new FixedNumBlocksHfwTestWidget());
        layout->addWidget(new FixedAreaHfwTestWidget());
    }
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testBaseWidget(const bool long_text)
{
    auto layout = new FlowLayoutHfw();
    layout->addWidget(new LabelWordWrapWide("Option Z1"));
    QString option2 = long_text ? "Option Z2 " + TextConst::LOREM_IPSUM_2
                                : "Option Z2";
    layout->addWidget(new LabelWordWrapWide(option2));
    layout->addWidget(new LabelWordWrapWide("Option Z3"));
    auto widget = new BaseWidget();
    widget->setLayout(layout);
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testVBoxLayout(const bool long_text)
{
    auto widget = new QWidget();
    auto layout = new VBoxLayout();
    widget->setLayout(layout);
    layout->addWidget(new LabelWordWrapWide(sampleText(long_text)));
    layout->addWidget(new LabelWordWrapWide(sampleText(long_text)));
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testHBoxLayoutHfwStretch()
{
    auto widget = new QWidget();
    widget->setSizePolicy(sizehelpers::expandingExpandingHFWPolicy());
    auto layout = new HBoxLayoutHfw();
    widget->setLayout(layout);
    layout->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)));
    layout->addStretch();
    layout->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)));
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testGridLayoutHfw(const int example)
{
    auto widget = new QWidget();
    auto grid = new GridLayoutHfw();
    widget->setLayout(grid);
    switch (example) {
    case 1:
    default:
        // row 0
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 0, 0);
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 0, 1);
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 0, 2);
        // row 1
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 1, 0);
        grid->addWidget(new LabelWordWrapWide(TextConst::LOREM_IPSUM_1), 1, 1);
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 1, 2);
        // row 2
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 2, 0);
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 2, 1);
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 2, 2);
        break;
    case 2:
        // row 0
        grid->addWidget(new LabelWordWrapWide(SHORT_TEXT), 0, 0);
        grid->addWidget(new LabelWordWrapWide(SHORT_TEXT), 0, 1);
        // row 1
        grid->addWidget(new LabelWordWrapWide(SHORT_TEXT), 1, 0);
        grid->addWidget(new LabelWordWrapWide(SHORT_TEXT), 1, 1);
        break;
    case 3:
        // spanning (first, as background)
        mcqfunc::addOptionBackground(grid, 0, 0, 4);
        mcqfunc::addVerticalLine(grid, 1, 3);
        // row 0
        grid->addWidget(new LabelWordWrapWide(SHORT_TEXT), 0, 2);
        grid->addWidget(new LabelWordWrapWide(SHORT_TEXT), 0, 3);
        grid->addWidget(new LabelWordWrapWide(SHORT_TEXT), 0, 4);
        // row 1
        grid->addWidget(new LabelWordWrapWide(SHORT_TEXT), 1, 0);
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 1, 2);
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 1, 3);
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 1, 4);
        // row 2
        grid->addWidget(new LabelWordWrapWide(SHORT_TEXT), 2, 0);
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 2, 2);
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 2, 3);
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 2, 4);
        break;
    case 4:
        // row 0
        grid->addWidget(new ImageButton(uiconst::CBS_ADD), 0, 0);
        // row 1
        grid->addWidget(new ImageButton(uiconst::CBS_ADD), 1, 0);
        // row 2
        grid->addWidget(new ImageButton(uiconst::CBS_ADD), 2, 0);
        break;
    case 5:
        // row 0
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 0, 0);
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 0, 1);
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 0, 2);
        // row 1
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 1, 0);
        grid->addWidget(new FixedNumBlocksHfwTestWidget(), 1, 1);
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 1, 2);
        // row 2
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 2, 0);
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 2, 1);
        grid->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)), 2, 2);
        break;
    }
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testVerticalScrollAreaSimple()
{
    // QVBoxLayout and three simple fixed-size icons
    auto contentwidget = new QWidget();
    auto layout = new QVBoxLayout();  // simpler than VBoxLayoutHfw
    contentwidget->setLayout(layout);

    layout->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)));
    layout->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)));
    layout->addWidget(uifunc::iconWidget(uifunc::iconFilename(uiconst::CBS_ADD)));

    auto scrollwidget = new VerticalScrollArea();
    scrollwidget->setWidget(contentwidget);
    debugfunc::debugWidget(scrollwidget);
}


void WidgetTestMenu::testVerticalScrollAreaComplex(const bool long_text)
{
    // VBoxLayout (i.e. likely VBoxLayoutHfw) and two word-wrapping labels
    auto contentwidget = new BaseWidget();
    auto layout = new VBoxLayout();
    contentwidget->setLayout(layout);

    layout->addWidget(new LabelWordWrapWide(sampleText(long_text)));
    layout->addWidget(new LabelWordWrapWide(sampleText(long_text)));

    auto scrollwidget = new VerticalScrollArea();
    scrollwidget->setWidget(contentwidget);
    debugfunc::debugWidget(scrollwidget);
}


void WidgetTestMenu::testVerticalScrollAreaFixedAreaHfwWidget()
{
    auto contentwidget = new FixedAreaHfwTestWidget();

    auto scrollwidget = new VerticalScrollArea();
    scrollwidget->setWidget(contentwidget);
    debugfunc::debugWidget(scrollwidget);
}


void WidgetTestMenu::testVerticalScrollAreaAspectRatioPixmap()
{
    auto contentwidget = new AspectRatioPixmap();
    QPixmap pixmap = uifunc::getPixmap(uifunc::iconFilename(uiconst::ICON_CAMCOPS));
    contentwidget->setPixmap(pixmap);

    auto scrollwidget = new VerticalScrollArea();
    scrollwidget->setWidget(contentwidget);
    debugfunc::debugWidget(scrollwidget);
}


void WidgetTestMenu::testVerticalScrollGridLayout()
{
    auto contentwidget = new BaseWidget();
    auto layout = new GridLayoutHfw();
    contentwidget->setLayout(layout);

    const bool long_text = true;
    QPixmap pixmap = uifunc::getPixmap(uifunc::iconFilename(uiconst::ICON_CAMCOPS));

    layout->addWidget(new LabelWordWrapWide(sampleText(long_text)), 0, 1);
    layout->addWidget(new LabelWordWrapWide(sampleText(long_text)), 0, 2);
    layout->addWidget(new LabelWordWrapWide(sampleText(long_text)), 1, 0);
    layout->addWidget(new AspectRatioPixmap(&pixmap), 1, 1);
    layout->addWidget(new AspectRatioPixmap(&pixmap), 1, 2);
    layout->addWidget(new LabelWordWrapWide(sampleText(long_text)), 2, 0);
    layout->addWidget(new AspectRatioPixmap(&pixmap), 2, 1);
    layout->addWidget(new AspectRatioPixmap(&pixmap), 2, 2);

    auto scrollwidget = new VerticalScrollArea();
    scrollwidget->setWidget(contentwidget);
    debugfunc::debugWidget(scrollwidget);
}


// ============================================================================
// Large-scale widgets
// ============================================================================

void WidgetTestMenu::testMenuItem()
{
    const MenuItem item = MAKE_TASK_MENU_ITEM("ace3", m_app);
    QWidget* widget = item.rowWidget(m_app);
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testQuestionnaireHeader()
{
    auto widget = new QuestionnaireHeader(
                nullptr, TextConst::LOREM_IPSUM_1,
                false, true, false, cssconst::QUESTIONNAIRE_BACKGROUND_CONFIG);
    widget->setStyleSheet(m_app.getSubstitutedCss(uiconst::CSS_CAMCOPS_QUESTIONNAIRE));
    debugfunc::debugWidget(widget);
}


void WidgetTestMenu::testQuestionnaire(const bool long_title,
                                       const bool as_openable_widget)
{
    QuPagePtr page(new QuPage());
    page->addElement(new QuText(TextConst::LOREM_IPSUM_1));
    page->setTitle(long_title ? TextConst::LOREM_IPSUM_1
                              : "Reasonably long title with several words");
    auto widget = new Questionnaire(m_app, {page});
    if (as_openable_widget) {
        m_app.openSubWindow(widget);
    } else {
        widget->build();
        debugfunc::debugWidget(widget, false, false);
    }
}


/*
void WidgetTestMenu::testAce3()
{
    TaskPtr task(new Ace3(m_app, m_app.db()));
    OpenableWidget* widget = task->editor();
    if (!widget) {
        uifunc::alert("ACE-3: no editor provided!");
        return;
    }
    widget->build();
    DebugFunc::debugWidget(widget);
}
*/


// ============================================================================
// Questionnaire element widgets
// ============================================================================

void WidgetTestMenu::testQuAudioPlayer()
{
    QuAudioPlayer element(uiconst::DEMO_SOUND_URL_1);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuBoolean(const bool as_text_button,
                                   const bool long_text,
                                   const bool false_appears_blank)
{
    QuBoolean element(sampleText(long_text), m_fieldref_1);
    element.setAsTextButton(as_text_button);
    element.setFalseAppearsBlank(false_appears_blank);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuButton()
{
    QuButton element("QuButton", std::bind(&WidgetTestMenu::dummyAction, this));
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuCanvas()
{
    QuCanvas element(m_fieldref_blob);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuCountdown()
{
    const int time_s = 10;
    QuCountdown element(time_s);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuDateTime()
{
    QuDateTime element(m_fieldref_1);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuDateTimeLimited()
{
    QuDateTime element(m_fieldref_1);
    element.setMinimumDate(QDate(1900, 1, 1));
    element.setMaximumDate(QDate(1999, 12, 31));
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuDiagnosticCode()
{
    QSharedPointer<Icd10> icd10 = QSharedPointer<Icd10>(new Icd10(m_app));
    QuDiagnosticCode element(icd10, m_fieldref_1, m_fieldref_2);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuHeading(const bool long_text)
{
    QuHeading element(sampleText(long_text));
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuHorizontalLine()
{
    QuHorizontalLine element;
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuImage()
{
    QuImage element(uifunc::iconFilename(uiconst::ICON_CAMCOPS));
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuLineEdit()
{
    QuLineEdit element(m_fieldref_1);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuLineEditDouble()
{
    QuLineEditDouble element(m_fieldref_1);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuLineEditInteger()
{
    QuLineEditInteger element(m_fieldref_1);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuLineEditLongLong()
{
    QuLineEditInt64 element(m_fieldref_1);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuLineEditNHSNumber()
{
    QuLineEditNHSNumber element(m_fieldref_1);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuLineEditULongLong()
{
    QuLineEditUInt64 element(m_fieldref_1);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuMCQ(const bool horizontal, const bool long_text,
                               const bool as_text_button)
{
    QuMcq element(m_fieldref_1, long_text ? m_options_3 : m_options_1);
    element.setHorizontal(horizontal);
    element.setAsTextButton(as_text_button);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuMCQGrid(const bool expand, const int example)
{
    const QString q2 = example == 1 ? "Question 2" : TextConst::LOREM_IPSUM_1;
    QVector<QuestionWithOneField> question_field_pairs{
        QuestionWithOneField(m_fieldref_1, "Question 1"),
        QuestionWithOneField(m_fieldref_2, q2),
    };
    QuMcqGrid element(question_field_pairs, m_options_1);
    element.setExpand(expand);
    switch (example) {
    case 1:
    case 2:
    default:
        break;
    case 3:
        element.setTitle("MCQ 2 title; " + TextConst::LOREM_IPSUM_2);
        break;
    }
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuMCQGridDouble(const bool expand)
{
    QVector<QuestionWithTwoFields> question_field_pairs{
        QuestionWithTwoFields("Question 1", m_fieldref_1, m_fieldref_2),
        QuestionWithTwoFields("Question 2 " + TextConst::LOREM_IPSUM_1,
                              m_fieldref_1, m_fieldref_2),
    };
    QuMcqGridDouble element(question_field_pairs, m_options_1, m_options_2);
    element.setExpand(expand);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuMCQGridSingleBoolean(const bool expand)
{
    QVector<QuestionWithTwoFields> question_field_pairs{
        QuestionWithTwoFields("Question 1", m_fieldref_1, m_fieldref_2),
        QuestionWithTwoFields("Question 2 " + TextConst::LOREM_IPSUM_1,
                              m_fieldref_1, m_fieldref_2),
    };
    QuMcqGridSingleBoolean element(question_field_pairs,
                                   m_options_1, "boolean");
    element.setExpand(expand);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuMultipleResponse(const bool horizontal,
                                            const bool long_text)
{
    QVector<QuestionWithOneField> question_field_pairs{
        QuestionWithOneField(m_fieldref_1, "Question 1"),
        QuestionWithOneField(m_fieldref_2, long_text ? TextConst::LOREM_IPSUM_1
                                                     : "Question 2"),
    };
    QuMultipleResponse element(question_field_pairs);
    element.setHorizontal(horizontal);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuPhoto()
{
    QuPhoto element(m_fieldref_blob);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuPickerInline()
{
    QuPickerInline element(m_fieldref_1, m_options_3);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuPickerPopup()
{
    QuPickerPopup element(m_fieldref_1, m_options_3);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuSlider(const bool horizontal)
{
    QuSlider element(m_fieldref_1, 0, 10, 1);
    element.setHorizontal(horizontal);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuSpacer()
{
    QuSpacer element;
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuSpinBoxDouble()
{
    QuSpinBoxDouble element(m_fieldref_1, 0.0, 10.0, 2);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuSpinBoxInteger()
{
    QuSpinBoxInteger element(m_fieldref_1, 0, 10);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuText(const bool long_text, const bool bold)
{
    QuText element(sampleText(long_text));
    if (bold) {
        element.setBold(true);
    }
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuTextEdit()
{
    QuTextEdit element(m_fieldref_1);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuThermometer()
{
    QVector<QuThermometerItem> thermometer_items;
    for (int i = 0; i <= 10; ++i) {
        QString text = QString::number(i);
        QuThermometerItem item(
            uifunc::resourceFilename(
                        QString("distressthermometer/dt_sel_%1.png").arg(i)),
            uifunc::resourceFilename(
                        QString("distressthermometer/dt_unsel_%1.png").arg(i)),
            text,
            i
        );
        thermometer_items.append(item);
    }
    QuThermometer element(m_fieldref_1, thermometer_items);
    element.setRescale(true, 0.4);
    testQuestionnaireElement(&element);
}


// ============================================================================
// Graphs
// ============================================================================

QCustomPlot* WidgetTestMenu::makeQCustomPlotOrWarn()
{
    auto p = new QCustomPlot();
    if (!p) {
        qWarning() << "Unable to create QCustomPlot";
    }
    return p;
}


void WidgetTestMenu::showPlot(QCustomPlot* p, const QSize& minsize)
{
    auto dlg = new QDialog(this);  // memory management now by Qt
    auto layout = new QVBoxLayout();
    dlg->setLayout(layout);
    p->setMinimumSize(minsize);
    layout->addWidget(p);
    dlg->setModal(true);
    dlg->show();
}


void WidgetTestMenu::testQCustomPlot1()
{
    // From https://www.qcustomplot.com/index.php/tutorials/basicplotting
    auto plot = makeQCustomPlotOrWarn();
    if (!plot) {
        return;
    }

    // generate some data:
    const int n = 101;
    QVector<double> x(n), y(n);
    for (int i = 0; i < n; ++i) {
      x[i] = i / 50.0 - 1;  // x goes from -1 to 1
      y[i] = x[i] * x[i];  // let's plot a quadratic function
    }
    // create graph and assign data to it:
    plot->addGraph();
    plot->graph(0)->setData(x, y);
    // give the axes some labels:
    plot->xAxis->setLabel("x");
    plot->yAxis->setLabel("y");
    // set axes ranges, so we see all data:
    plot->xAxis->setRange(-1, 1);
    plot->yAxis->setRange(0, 1);
    plot->replot();

    // Show dialogue
    showPlot(plot);  // takes ownership
}


void WidgetTestMenu::testQCustomPlot2()
{
    // From https://www.qcustomplot.com/index.php/tutorials/basicplotting,
    // modified a bit:
    // - random number generation
    // - seconds since epoch

    auto plot = makeQCustomPlotOrWarn();
    if (!plot) {
        return;
    }
    QRandomGenerator rng(8);  // seed at creation

    // set locale to english, so we get english month names:
    plot->setLocale(QLocale(QLocale::English, QLocale::UnitedKingdom));
    // seconds of current time, we'll use it as starting point in time for data:
    double now = QDateTime::currentDateTime().toSecsSinceEpoch();
    // create multiple graphs:
    const int ngraphs = 5;
    const int n = 250;
    for (int gi = 0; gi < ngraphs; ++gi) {
        plot->addGraph();
        QColor color(20 + 200 / 4.0 * gi, 70 * (1.6 - gi / 4.0), 150, 150);
        plot->graph()->setLineStyle(QCPGraph::lsLine);
        plot->graph()->setPen(QPen(color.lighter(200)));
        plot->graph()->setBrush(QBrush(color));
        // generate random walk data:
        QVector<QCPGraphData> timedata(n);
        for (int i = 0; i < n; ++i) {
            timedata[i].key = now + 24 * 3600 * i;  // units are seconds
            const double randval = rng.generateDouble() - 0.5;  // range [-0.5, +0.5)
            if (i == 0) {
                timedata[i].value = (i / 50.0 + 1) * randval;
            } else {
                timedata[i].value =
                        qFabs(timedata[i - 1].value) * (1 + 0.02 / 4.0 * (4 - gi)) +
                        (i / 50.0 + 1) * randval;
            }
        }
        plot->graph()->data()->set(timedata);
    }
    // configure bottom axis to show date instead of number:
    QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
    dateTicker->setDateTimeFormat("d MMMM\nyyyy");
    plot->xAxis->setTicker(dateTicker);
    // configure left axis text labels:
    QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
    textTicker->addTick(10, "a bit\nlow");
    textTicker->addTick(50, "quite\nhigh");
    plot->yAxis->setTicker(textTicker);
    // set a more compact font size for bottom and left axis tick labels:
    plot->xAxis->setTickLabelFont(QFont(QFont().family(), 8));
    plot->yAxis->setTickLabelFont(QFont(QFont().family(), 8));
    // set axis labels:
    plot->xAxis->setLabel("Date");
    plot->yAxis->setLabel("Random wobbly lines value");
    // make top and right axes visible but without ticks and labels:
    plot->xAxis2->setVisible(true);
    plot->yAxis2->setVisible(true);
    plot->xAxis2->setTicks(false);
    plot->yAxis2->setTicks(false);
    plot->xAxis2->setTickLabels(false);
    plot->yAxis2->setTickLabels(false);
    // set axis ranges to show all data:
    plot->xAxis->setRange(now, now + 24 * 3600 * n - 1);
    plot->yAxis->setRange(0, 60);
    // show legend with slightly transparent background brush:
    plot->legend->setVisible(true);
    plot->legend->setBrush(QColor(255, 255, 255, 150));

    // Show dialogue
    showPlot(plot);  // takes ownership
}
