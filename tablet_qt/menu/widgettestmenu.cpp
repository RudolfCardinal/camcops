/*
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
#include <QPushButton>
#include "common/cssconst.h"
#include "common/uiconstants.h"
#include "diagnosis/icd10.h"
#include "lib/debugfunc.h"
#include "lib/uifunc.h"
#include "menulib/menuitem.h"
#include "questionnairelib/questionnaire.h"
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
#include "questionnairelib/qulineeditlonglong.h"
#include "questionnairelib/qulineeditulonglong.h"
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
#include "widgets/aspectratiopixmaplabel.h"
#include "widgets/basewidget.h"
#include "widgets/canvaswidget.h"
#include "widgets/clickablelabelnowrap.h"
#include "widgets/clickablelabelwordwrapwide.h"
#include "widgets/flowlayouthfw.h"
#include "widgets/horizontalline.h"
#include "widgets/imagebutton.h"
#include "widgets/labelwordwrapwide.h"
#include "widgets/verticalline.h"
#include "widgets/verticalscrollarea.h"


WidgetTestMenu::WidgetTestMenu(CamcopsApp& app)
    : MenuWindow(app, tr("Widget tests"), "")
{
    FieldRef::GetterFunction getter1 = std::bind(&WidgetTestMenu::dummyGetter1,
                                                 this);
    FieldRef::SetterFunction setter1 = std::bind(&WidgetTestMenu::dummySetter1,
                                                 this, std::placeholders::_1);
    FieldRef::GetterFunction getter2 = std::bind(&WidgetTestMenu::dummyGetter2,
                                                 this);
    FieldRef::SetterFunction setter2 = std::bind(&WidgetTestMenu::dummySetter2,
                                                 this, std::placeholders::_1);
    bool mandatory = true;
    m_fieldref_1 = FieldRefPtr(new FieldRef(getter1, setter1, mandatory));
    m_fieldref_2 = FieldRefPtr(new FieldRef(getter2, setter2, mandatory));

    m_options_1.addItem(NameValuePair("Option A1", 1));
    m_options_1.addItem(NameValuePair("Option A2", 2));
    m_options_1.addItem(NameValuePair("Option A3", 3));

    m_options_2.addItem(NameValuePair("Option B1", 1));
    m_options_2.addItem(NameValuePair("Option B2", 2));

    m_options_3.addItem(NameValuePair("Option C1", 1));
    m_options_3.addItem(NameValuePair("Option C2 " + UiConst::LOREM_IPSUM_1, 2));
    m_options_3.addItem(NameValuePair("Option C3", 3));

    QSizePolicy fixed_fixed(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QSizePolicy expand_expand(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QSizePolicy expand_fixed_hfw = UiFunc::expandingFixedHFWPolicy();
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
        MenuItem("AspectRatioPixmapLabel (should maintain aspect ratio and resize from 0 to its intrinsic size)",
                 std::bind(&WidgetTestMenu::testAspectRatioPixmapLabel, this)),
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
        MenuItem("CanvasWidget",
                 std::bind(&WidgetTestMenu::testCanvasWidget, this)),
        MenuItem("ClickableLabelNoWrap (short text) (not generally used: no word wrap)",
                 std::bind(&WidgetTestMenu::testClickableLabelNoWrap, this, false)),
        MenuItem("ClickableLabelNoWrap (long text) (not generally used: no word wrap)",
                 std::bind(&WidgetTestMenu::testClickableLabelNoWrap, this, true)),
        MenuItem("ClickableLabelWordWrapWide (short text)",
                 std::bind(&WidgetTestMenu::testClickableLabelWordWrapWide, this, false)),
        MenuItem("ClickableLabelWordWrapWide (long text)",
                 std::bind(&WidgetTestMenu::testClickableLabelWordWrapWide, this, true)),
        MenuItem("HorizontalLine",
                 std::bind(&WidgetTestMenu::testHorizontalLine, this)),
        MenuItem("ImageButton",
                 std::bind(&WidgetTestMenu::testImageButton, this)),
        MenuItem("LabelWordWrapWide (short text)",
                 std::bind(&WidgetTestMenu::testLabelWordWrapWide, this, false, true)),
        MenuItem("LabelWordWrapWide (long text) (within QVBoxLayout)",
                 std::bind(&WidgetTestMenu::testLabelWordWrapWide, this, true, false)),
        MenuItem("LabelWordWrapWide (long text) (within VBoxLayoutHfw)",
                 std::bind(&WidgetTestMenu::testLabelWordWrapWide, this, true, true)),
        MenuItem("VerticalLine",
                 std::bind(&WidgetTestMenu::testVerticalLine, this)),
        MenuItem("VerticalScrollArea",
                 std::bind(&WidgetTestMenu::testVerticalScrollArea, this)),

        // --------------------------------------------------------------------
        MenuItem("Layouts and the like").setLabelOnly(),
        // --------------------------------------------------------------------
        MenuItem("FlowLayout (containing fixed-size icons)",
                 std::bind(&WidgetTestMenu::testFlowLayout, this, 5)),
        MenuItem("BaseWidget (with short text)",
                 std::bind(&WidgetTestMenu::testBaseWidget, this, false)),
        MenuItem("BaseWidget (with long text)",
                 std::bind(&WidgetTestMenu::testBaseWidget, this, true)),

        MenuItem("Large-scale widgets").setLabelOnly(),
        MenuItem("MenuItem",
                 std::bind(&WidgetTestMenu::testMenuItem, this)),
        MenuItem("QuestionnaireHeader",
                 std::bind(&WidgetTestMenu::testQuestionnaireHeader, this)),
        MenuItem("Empty questionnaire",
                 std::bind(&WidgetTestMenu::testQuestionnaire, this)),
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
                 std::bind(&WidgetTestMenu::testQuBoolean, this, false, false)),
        MenuItem("QuBoolean (as_text_button=false, long text)",
                 std::bind(&WidgetTestMenu::testQuBoolean, this, false, true)),
        MenuItem("QuBoolean (as_text_button=true, short text)",
                 std::bind(&WidgetTestMenu::testQuBoolean, this, true, false)),
        MenuItem("QuBoolean (as_text_button=true, long text)",
                 std::bind(&WidgetTestMenu::testQuBoolean, this, true, true)),
        MenuItem("QuButton",
                 std::bind(&WidgetTestMenu::testQuButton, this)),
        MenuItem("QuCanvas",
                 std::bind(&WidgetTestMenu::testQuCanvas, this)),
        MenuItem("QuCountdown",
                 std::bind(&WidgetTestMenu::testQuCountdown, this)),
        MenuItem("QuDateTime",
                 std::bind(&WidgetTestMenu::testQuDateTime, this)),
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
        MenuItem("QuMCQGrid (expand=false)",
                 std::bind(&WidgetTestMenu::testQuMCQGrid, this, false)),
        MenuItem("QuMCQGrid (expand=true)",
                 std::bind(&WidgetTestMenu::testQuMCQGrid, this, true)),
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
        MenuItem("QuPhoto",
                 std::bind(&WidgetTestMenu::testQuPhoto, this)),
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
                 std::bind(&WidgetTestMenu::testQuText, this, false)),
        MenuItem("QuText (long text)",
                 std::bind(&WidgetTestMenu::testQuText, this, true)),
        MenuItem("QuTextEdit",
                 std::bind(&WidgetTestMenu::testQuTextEdit, this)),
        MenuItem("QuThermometer",
                 std::bind(&WidgetTestMenu::testQuThermometer, this)),
    };
}


QVariant WidgetTestMenu::dummyGetter1() const
{
    return m_dummy_value_1;
}


bool WidgetTestMenu::dummySetter1(const QVariant& value)
{
    bool changed = (value != m_dummy_value_1);
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
    bool changed = (value != m_dummy_value_2);
    if (changed) {
        m_dummy_value_2 = value;
    }
    return changed;
}


void WidgetTestMenu::dummyAction()
{
    UiFunc::alert("Action!");
}


void WidgetTestMenu::testQuestionnaireElement(QuElement* element)
{
    Questionnaire questionnaire(m_app);
    QWidget* widget = element->widget(&questionnaire);
    widget->setStyleSheet(
                m_app.getSubstitutedCss(UiConst::CSS_CAMCOPS_QUESTIONNAIRE));
    DebugFunc::debugWidget(widget, false, false);
}


// ============================================================================
// Qt widgets
// ============================================================================

void WidgetTestMenu::testQLabel(const QSizePolicy& policy,
                                bool long_text, bool word_wrap)
{
    QLabel* widget = new QLabel(long_text ? UiConst::LOREM_IPSUM_1 : "Hello");
    widget->setWordWrap(word_wrap);
    widget->setSizePolicy(policy);
    DebugFunc::debugWidget(widget);
}


void WidgetTestMenu::testQPushButton(const QSizePolicy& policy)
{
    QPushButton* widget = new QPushButton("Hello");
    widget->setSizePolicy(policy);
    // http://stackoverflow.com/questions/21367260/qt-making-a-qpushbutton-fill-layout-cell
    connect(widget, &QPushButton::clicked,
            this, &WidgetTestMenu::dummyAction);
    DebugFunc::debugWidget(widget);
}


// ============================================================================
// Low-level widgets
// ============================================================================

void WidgetTestMenu::testAspectRatioPixmapLabel()
{
    AspectRatioPixmapLabel* widget = new AspectRatioPixmapLabel();
    QPixmap pixmap = UiFunc::getPixmap(UiFunc::iconFilename(UiConst::ICON_CAMCOPS));
    widget->setPixmap(pixmap);
    DebugFunc::debugWidget(widget);
}


void WidgetTestMenu::testBooleanWidget(BooleanWidget::Appearance appearance,
                                       bool long_text)
{
    BooleanWidget* widget = new BooleanWidget();
    bool big = true;
    bool as_text_button = (appearance == BooleanWidget::Appearance::Text);
    widget->setAppearance(appearance);
    widget->setSize(big);
    widget->setValue(true, true);
    if (as_text_button) {
        widget->setText(long_text ? UiConst::LOREM_IPSUM_2 : "BooleanWidget");
    }
    DebugFunc::debugWidget(widget);
}


void WidgetTestMenu::testCanvasWidget()
{
    QSize size(200, 200);
    CanvasWidget* widget = new CanvasWidget(size);
    QImage img(size, QImage::Format_RGB32);
    widget->setImage(img);
    widget->clear(Qt::white);
    DebugFunc::debugWidget(widget);
}


void WidgetTestMenu::testClickableLabelNoWrap(bool long_text)
{
    QString text = long_text ? UiConst::LOREM_IPSUM_1 : "Text";
    ClickableLabelNoWrap* widget = new ClickableLabelNoWrap(text);
    connect(widget, &QAbstractButton::clicked,
            this, &WidgetTestMenu::dummyAction);
    DebugFunc::debugWidget(widget);
}


void WidgetTestMenu::testClickableLabelWordWrapWide(bool long_text)
{
    QString text = long_text ? UiConst::LOREM_IPSUM_1 : "Text";
    ClickableLabelWordWrapWide* widget = new ClickableLabelWordWrapWide(text);
    connect(widget, &QAbstractButton::clicked,
            this, &WidgetTestMenu::dummyAction);
    DebugFunc::debugWidget(widget);
}


void WidgetTestMenu::testHorizontalLine()
{
    const int width = 4;
    HorizontalLine* widget = new HorizontalLine(width);
    widget->setStyleSheet("background-color: black;");
    DebugFunc::debugWidget(widget);
}


void WidgetTestMenu::testImageButton()
{
    ImageButton* widget;
    widget = new ImageButton(UiConst::CBS_ADD);
    DebugFunc::debugWidget(widget);
}


void WidgetTestMenu::testLabelWordWrapWide(bool long_text, bool use_hfw_layout)
{
    QString text = long_text ? UiConst::LOREM_IPSUM_1 : "Text";
    LabelWordWrapWide* widget = new LabelWordWrapWide(text);
    bool set_background_by_name = false;
    bool set_background_by_stylesheet = true;
    bool show_widget_properties = true;
    bool show_widget_attributes = false;
    int spaces_per_level = 4;
    DebugFunc::debugWidget(widget, set_background_by_name,
                           set_background_by_stylesheet,
                           show_widget_properties,
                           show_widget_attributes,
                           spaces_per_level,
                           use_hfw_layout);
}


void WidgetTestMenu::testVerticalLine()
{
    const int width = 4;
    VerticalLine* widget = new VerticalLine(width);
    widget->setStyleSheet("background-color: black;");
    DebugFunc::debugWidget(widget);
}


void WidgetTestMenu::testVerticalScrollArea()
{
    QWidget* contentwidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout();
    contentwidget->setLayout(layout);

    layout->addWidget(new QLabel("hello"));
    layout->addWidget(new QLabel("there"));

    VerticalScrollArea* scrollwidget = new VerticalScrollArea();
    scrollwidget->setWidget(contentwidget);

    DebugFunc::debugWidget(scrollwidget);
}


// ============================================================================
// Layouts and the like
// ============================================================================

void WidgetTestMenu::testFlowLayout(int n_icons)
{
    QWidget* widget = new QWidget();
    FlowLayoutHfw* layout = new FlowLayoutHfw();
    widget->setLayout(layout);
    for (int i = 0; i < n_icons; ++i) {
        QLabel* icon = UiFunc::iconWidget(UiFunc::iconFilename(UiConst::CBS_ADD));
        layout->addWidget(icon);
    }
    DebugFunc::debugWidget(widget);
}


void WidgetTestMenu::testBaseWidget(bool long_text)
{
    FlowLayoutHfw* layout = new FlowLayoutHfw();
    layout->addWidget(new LabelWordWrapWide("Option Z1"));
    QString option2 = long_text ? "Option Z2 " + UiConst::LOREM_IPSUM_2
                                : "Option Z2";
    layout->addWidget(new LabelWordWrapWide(option2));
    layout->addWidget(new LabelWordWrapWide("Option Z3"));
    BaseWidget* widget = new BaseWidget();
    widget->setLayout(layout);
    DebugFunc::debugWidget(widget);
}


// ============================================================================
// Large-scale widgets
// ============================================================================

void WidgetTestMenu::testMenuItem()
{
    MenuItem item = MAKE_TASK_MENU_ITEM("ace3", m_app);
    QWidget* widget = item.rowWidget(m_app);
    DebugFunc::debugWidget(widget);
}


void WidgetTestMenu::testQuestionnaireHeader()
{
    QuestionnaireHeader* widget = new QuestionnaireHeader(
                nullptr, "Title text, quite long: " + UiConst::LOREM_IPSUM_3,
                false, true, false, CssConst::QUESTIONNAIRE_BACKGROUND_CONFIG);
    widget->setStyleSheet(m_app.getSubstitutedCss(UiConst::CSS_CAMCOPS_QUESTIONNAIRE));
    DebugFunc::debugWidget(widget);
}


void WidgetTestMenu::testQuestionnaire()
{
    QuPagePtr page(new QuPage());
    page->addElement(new QuText(UiConst::LOREM_IPSUM_1));
    page->setTitle("Reasonably long title with several words");
    Questionnaire* widget = new Questionnaire(m_app, {page});
    widget->build();
    DebugFunc::debugWidget(widget, false, false);
}


/*
void WidgetTestMenu::testAce3()
{
    TaskPtr task(new Ace3(m_app, m_app.db()));
    OpenableWidget* widget = task->editor();
    widget->build();
    DebugFunc::debugWidget(widget);
}
*/


// ============================================================================
// Questionnaire element widgets
// ============================================================================

void WidgetTestMenu::testQuAudioPlayer()
{
    QuAudioPlayer element(UiConst::DEMO_SOUND_URL);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuBoolean(bool as_text_button, bool long_text)
{
    QString text = long_text ? UiConst::LOREM_IPSUM_1 : "QuBoolean";
    QuBoolean element(text, m_fieldref_1);
    element.setAsTextButton(as_text_button);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuButton()
{
    QuButton element("QuButton", std::bind(&WidgetTestMenu::dummyAction, this));
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuCanvas()
{
    QuCanvas element(m_fieldref_1);
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


void WidgetTestMenu::testQuDiagnosticCode()
{
    QSharedPointer<Icd10> icd10 = QSharedPointer<Icd10>(new Icd10(m_app));
    QuDiagnosticCode element(icd10, m_fieldref_1, m_fieldref_2);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuHeading(bool long_text)
{
    QuHeading element(long_text ? UiConst::LOREM_IPSUM_1 : "Heading");
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuHorizontalLine()
{
    QuHorizontalLine element;
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuImage()
{
    QuImage element(UiFunc::iconFilename(UiConst::ICON_CAMCOPS));
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
    QuLineEditLongLong element(m_fieldref_1);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuLineEditULongLong()
{
    QuLineEditULongLong element(m_fieldref_1);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuMCQ(bool horizontal, bool long_text,
                               bool as_text_button)
{
    QuMCQ element(m_fieldref_1, long_text ? m_options_3 : m_options_1);
    element.setHorizontal(horizontal);
    element.setAsTextButton(as_text_button);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuMCQGrid(bool expand)
{
    QList<QuestionWithOneField> question_field_pairs{
        QuestionWithOneField(m_fieldref_1, "Question 1"),
        QuestionWithOneField(m_fieldref_2, "Question 2 " + UiConst::LOREM_IPSUM_1),
    };
    QuMCQGrid element(question_field_pairs, m_options_1);
    element.setExpand(expand);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuMCQGridDouble(bool expand)
{
    QList<QuestionWithTwoFields> question_field_pairs{
        QuestionWithTwoFields("Question 1", m_fieldref_1, m_fieldref_2),
        QuestionWithTwoFields("Question 2 " + UiConst::LOREM_IPSUM_1,
                              m_fieldref_1, m_fieldref_2),
    };
    QuMCQGridDouble element(question_field_pairs, m_options_1, m_options_2);
    element.setExpand(expand);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuMCQGridSingleBoolean(bool expand)
{
    QList<QuestionWithTwoFields> question_field_pairs{
        QuestionWithTwoFields("Question 1", m_fieldref_1, m_fieldref_2),
        QuestionWithTwoFields("Question 2 " + UiConst::LOREM_IPSUM_1,
                              m_fieldref_1, m_fieldref_2),
    };
    QuMCQGridSingleBoolean element(question_field_pairs,
                                   m_options_1, "boolean");
    element.setExpand(expand);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuMultipleResponse(bool horizontal, bool long_text)
{
    QList<QuestionWithOneField> question_field_pairs{
        QuestionWithOneField(m_fieldref_1, "Question 1"),
        QuestionWithOneField(m_fieldref_2, long_text ? UiConst::LOREM_IPSUM_1
                                                     : "Question 2"),
    };
    QuMultipleResponse element(question_field_pairs);
    element.setHorizontal(horizontal);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuPhoto()
{
    QuPhoto element(m_fieldref_1);
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


void WidgetTestMenu::testQuSlider(bool horizontal)
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


void WidgetTestMenu::testQuText(bool long_text)
{
    QuText element(long_text ? UiConst::LOREM_IPSUM_1 : "text");
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuTextEdit()
{
    QuTextEdit element(m_fieldref_1);
    testQuestionnaireElement(&element);
}


void WidgetTestMenu::testQuThermometer()
{
    QList<QuThermometerItem> thermometer_items;
    for (int i = 0; i <= 10; ++i) {
        QString text = QString::number(i);
        QuThermometerItem item(
            UiFunc::resourceFilename(
                        QString("distressthermometer/dt_sel_%1.png").arg(i)),
            UiFunc::resourceFilename(
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
