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

#include "whiskertestmenu.h"
#include "common/uiconst.h"
#include "common/varconst.h"
#include "dialogs/logbox.h"
#include "lib/datetime.h"
#include "lib/uifunc.h"
#include "menulib/menuitem.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/questionnairefunc.h"
#include "questionnairelib/quhorizontalline.h"
#include "questionnairelib/qulineedit.h"
#include "questionnairelib/qulineeditinteger.h"
#include "questionnairelib/qupage.h"
#include "whisker/whiskerconstants.h"
#include "whisker/whiskerinboundmessage.h"
#include "whisker/whiskermanager.h"


// ----------------------------------------------------------------------------
// Constants for Whisker test task
// ----------------------------------------------------------------------------
// Line aliases
const QString digital_input("digital_input");
const QString digital_output("digital_output");
// Display, document
const QString main_display("main");
const QString second_display("virtualdisplay");
const QString doc("doc");
// Some demo objects
const QString text_obj = "objtext";
const QString bmp_obj_1("objbitmap1");
const QString bmp_obj_2("objbitmap2");
const QString line_obj("objline");
const QString arc_obj("objarc");
const QString bezier_obj("objbez");
const QString chord_obj("objchord");
const QString ellipse_obj("names_are_unimportant");
const QString pie_obj("objpie");
const QString polygon_obj_1("objpoly1");
const QString polygon_obj_2("objpoly2");
const QString rectangle_obj("objrect");
const QString roundrect_obj("objrr");
const QString camcogquadpattern_obj("camcogquadpattern");
const QString video_obj_1("vid1");
const QString video_obj_2("vid2");
const QString video_obj_both("vidboth");
// Test events.
const QString background_event("background");
const QString event_bmp_1("Bitmap_1");
const QString event_bmp_2("Bitmap_2");
const QString event_ellipse("Ellipse");
const QString event_roundrect("RoundRect");
const QString event_rectangle("Rectangle");
// const QString event_text("Text");
const QString event_polygon_1("Polygon_1");
const QString event_polygon_2("Polygon_2");
const QString event_chord("Chord");
const QString event_pie("Pie");
const QString event_camcogquadpattern("camcogquadpattern");
const QString suffix_event_clicked(" clicked");
const QString suffix_event_unclicked(" mouseup");
const QString suffix_event_touched(" touched");
const QString suffix_event_released(" released");
// const QString suffix_event_double_clicked(" double-clicked");
const QString suffix_event_mouse_moved(" mouse-moved");
const QString suffix_event_touch_moved(" touch-moved");
const QString event_video_1_touched("vid1touched");
const QString event_video_2_touched("vid2touched");
const QString suffix_event_video_play("play");
const QString suffix_event_video_pause("pause");
const QString suffix_event_video_stop("stop");
const QString suffix_event_video_back("back");
const QString suffix_event_video_fwd("fwd");
// Timer/line events
const QString event_single_tick("0.5Hz_tick_single");
const QString event_infinite_tick("0.2Hz_tick_infinite");
const QString event_counted_tick("1Hz_tick_5count");
const QString event_input_on("digital_input_on");
const QString event_input_off("digital_input_off");
// Timings
const unsigned int n_counted_ticks = 5;
const unsigned int single_tick_period_ms = 500;
const unsigned int infinite_tick_period_ms = 5000;
const unsigned int counted_tick_period_ms = 1000;
const int FIVE_SEC = 5000;
const unsigned int n_flashes = 10;
const unsigned int flash_on_ms = 300;
const unsigned int flash_off_ms = 700;
// Other
const QString DEFAULT_MEDIA_DIR(R"(C:\Program Files (x86)\WhiskerControl\Server Test Media)");


WhiskerTestMenu::WhiskerTestMenu(CamcopsApp& app) :
    MenuWindow(app, uifunc::iconFilename(uiconst::ICON_WHISKER)),
    m_whisker(nullptr),
    m_logbox(nullptr),
    m_host("localhost"),
    m_main_port(whiskerconstants::WHISKER_DEFAULT_PORT),
    m_display_num(0),
    m_use_video(true),
    m_use_two_videos(true),
    m_media_directory(DEFAULT_MEDIA_DIR),
    m_bmp_filename_1("Coffee.bmp"),
    m_bmp_filename_2("santa_fe.bmp"),
    m_video_filename_1("mediaexample.wmv"),
    m_video_filename_2("mediaexample.wmv"),
    m_input_line_num(0),
    m_output_line_num(8)
{
}


QString WhiskerTestMenu::title() const
{
    return tr("Test interface to Whisker");
}


void WhiskerTestMenu::makeItems()
{
    m_items = {
        MenuItem(
            tr("Configure demo Whisker task"),
            MenuItem::OpenableWidgetMaker(
                std::bind(&WhiskerTestMenu::configureWhisker, this,
                          std::placeholders::_1)
            )
        ),
        MenuItem(tr("Connect to Whisker server"),
                 std::bind(&WhiskerTestMenu::connectWhisker, this)),
        MenuItem(tr("Disconnect from Whisker server"),
                 std::bind(&WhiskerTestMenu::disconnectWhisker, this)),
        MenuItem(tr("Test network latency to Whisker server"),
                 std::bind(&WhiskerTestMenu::testWhiskerNetworkLatency, this)),
        MenuItem(
            tr("Run demo Whisker task"),
            std::bind(&WhiskerTestMenu::runDemoWhiskerTask, this)
        ),
    };
}


void WhiskerTestMenu::ensureWhiskerManager()
{
    if (!m_whisker) {
        m_whisker = new WhiskerManager(this);
    }
}


void WhiskerTestMenu::connectWhisker()
{
    ensureWhiskerManager();
    m_whisker->connectToServer(m_host.toString(),
                               static_cast<quint16>(m_main_port.toUInt()));
}


void WhiskerTestMenu::disconnectWhisker()
{
    ensureWhiskerManager();
    m_whisker->disconnectFromServer();
}


void WhiskerTestMenu::ensureWhiskerConnected()
{
    ensureWhiskerManager();
    if (!m_whisker->isConnected()) {
        connectWhisker();
    }
}


void WhiskerTestMenu::testWhiskerNetworkLatency()
{
    ensureWhiskerManager();
    if (!m_whisker->isConnected()) {
        m_whisker->alertNotConnected();
        return;
    }
    int latency_ms = m_whisker->getNetworkLatencyMs();
    uifunc::alert(
                tr("Network latency: %1 ms").arg(latency_ms),
                whiskerconstants::WHISKER_ALERT_TITLE);
}


OpenableWidget* WhiskerTestMenu::configureWhisker(CamcopsApp& app)
{
    auto makeTitle = [](const QString& part1, const QString& part2) -> QString {
        return QString("<b>%1</b> (%2):").arg(part1, part2);
    };
    auto makeHint = [](const QString& part1, const QString& part2) -> QString {
        return QString("%1 (%2)").arg(part1, part2);
    };

    app.clearCachedVars();  // ... in case any are left over

    FieldRef::GetterFunction get_host = std::bind(
                &WhiskerTestMenu::getValue, this, &m_host);
    FieldRef::SetterFunction set_host = std::bind(
                &WhiskerTestMenu::setValue, this, &m_host, std::placeholders::_1);
    FieldRefPtr host_fr(new FieldRef(get_host, set_host, true));
    const QString host_t(tr("Whisker host"));
    const QString host_h(tr("host name or IP address; default: localhost"));

    FieldRef::GetterFunction get_port = std::bind(
                &WhiskerTestMenu::getValue, this, &m_main_port);
    FieldRef::SetterFunction set_port = std::bind(
                &WhiskerTestMenu::setValue, this, &m_main_port, std::placeholders::_1);
    FieldRefPtr port_fr(new FieldRef(get_port, set_port, true));
    const QString port_t(tr("Whisker main port"));
    const QString port_h(tr("default 3233"));

    FieldRef::GetterFunction get_display_num = std::bind(
                &WhiskerTestMenu::getValue, this, &m_display_num);
    FieldRef::SetterFunction set_display_num = std::bind(
                &WhiskerTestMenu::setValue, this, &m_display_num, std::placeholders::_1);
    FieldRefPtr display_num_fr(new FieldRef(get_display_num, set_display_num, true));
    const QString display_num_t(tr("Whisker display number"));
    const QString display_num_h(tr("e.g. 0"));

    FieldRef::GetterFunction get_use_video = std::bind(
                &WhiskerTestMenu::getValue, this, &m_use_video);
    FieldRef::SetterFunction set_use_video = std::bind(
                &WhiskerTestMenu::setValue, this, &m_use_video, std::placeholders::_1);
    FieldRefPtr use_video_fr(new FieldRef(get_use_video, set_use_video, true));
    const QString use_video_t(tr("Use video"));

    FieldRef::GetterFunction get_use_two_videos = std::bind(
                &WhiskerTestMenu::getValue, this, &m_use_two_videos);
    FieldRef::SetterFunction set_use_two_videos = std::bind(
                &WhiskerTestMenu::setValue, this, &m_use_two_videos, std::placeholders::_1);
    FieldRefPtr use_two_videos_fr(new FieldRef(get_use_two_videos, set_use_two_videos, true));
    const QString use_two_videos_t(tr("Use two videos"));

    FieldRef::GetterFunction get_media_directory = std::bind(
                &WhiskerTestMenu::getValue, this, &m_media_directory);
    FieldRef::SetterFunction set_media_directory = std::bind(
                &WhiskerTestMenu::setValue, this, &m_media_directory, std::placeholders::_1);
    FieldRefPtr media_directory_fr(new FieldRef(get_media_directory, set_media_directory, true));
    const QString media_directory_t(tr("Media directory"));
    const QString media_directory_h(tr("(e.g. ") + DEFAULT_MEDIA_DIR);

    FieldRef::GetterFunction get_bmp_filename_1 = std::bind(
                &WhiskerTestMenu::getValue, this, &m_bmp_filename_1);
    FieldRef::SetterFunction set_bmp_filename_1 = std::bind(
                &WhiskerTestMenu::setValue, this, &m_bmp_filename_1, std::placeholders::_1);
    FieldRefPtr bmp_filename_1_fr(new FieldRef(get_bmp_filename_1, set_bmp_filename_1, true));
    const QString bmp_filename_1_t(tr("Bitmap (.BMP) filename 1"));

    FieldRef::GetterFunction get_bmp_filename_2 = std::bind(
                &WhiskerTestMenu::getValue, this, &m_bmp_filename_2);
    FieldRef::SetterFunction set_bmp_filename_2 = std::bind(
                &WhiskerTestMenu::setValue, this, &m_bmp_filename_2, std::placeholders::_1);
    FieldRefPtr bmp_filename_2_fr(new FieldRef(get_bmp_filename_2, set_bmp_filename_2, true));
    const QString bmp_filename_2_t(tr("Bitmap (.BMP) filename 2"));

    FieldRef::GetterFunction get_video_filename_1 = std::bind(
                &WhiskerTestMenu::getValue, this, &m_video_filename_1);
    FieldRef::SetterFunction set_video_filename_1 = std::bind(
                &WhiskerTestMenu::setValue, this, &m_video_filename_1, std::placeholders::_1);
    FieldRefPtr video_filename_1_fr(new FieldRef(get_video_filename_1, set_video_filename_1, true));
    const QString video_filename_1_t(tr("Video (.WMV) filename 1"));

    FieldRef::GetterFunction get_video_filename_2 = std::bind(
                &WhiskerTestMenu::getValue, this, &m_video_filename_2);
    FieldRef::SetterFunction set_video_filename_2 = std::bind(
                &WhiskerTestMenu::setValue, this, &m_video_filename_2, std::placeholders::_1);
    FieldRefPtr video_filename_2_fr(new FieldRef(get_video_filename_2, set_video_filename_2, true));
    const QString video_filename_2_t(tr("Video (.WMV) filename 2"));

    FieldRef::GetterFunction get_input_line_num = std::bind(
                &WhiskerTestMenu::getValue, this, &m_input_line_num);
    FieldRef::SetterFunction set_input_line_num = std::bind(
                &WhiskerTestMenu::setValue, this, &m_input_line_num, std::placeholders::_1);
    FieldRefPtr input_line_num_fr(new FieldRef(get_input_line_num, set_input_line_num, true));
    const QString input_line_num_t(tr("Digital input line number"));
    const QString input_line_num_h(tr("e.g. 0"));

    FieldRef::GetterFunction get_output_line_num = std::bind(
                &WhiskerTestMenu::getValue, this, &m_output_line_num);
    FieldRef::SetterFunction set_output_line_num = std::bind(
                &WhiskerTestMenu::setValue, this, &m_output_line_num, std::placeholders::_1);
    FieldRefPtr output_line_num_fr(new FieldRef(get_output_line_num, set_output_line_num, true));
    const QString output_line_num_t(tr("Digital output line number"));
    const QString output_line_num_h(tr("e.g. 8"));

    const int max_display_num = 1000;  // silly
    const int max_line_num = 1000;  // probably silly

    QuPagePtr page(new QuPage{
        questionnairefunc::defaultGridRawPointer({
            {
                makeTitle(host_t, host_h),
                (new QuLineEdit(host_fr))->setHint(makeHint(host_t, host_h))
            },
            {
                makeTitle(port_t, port_h),
                new QuLineEditInteger(port_fr,
                    uiconst::IP_PORT_MIN, uiconst::IP_PORT_MAX)
            },
        }, 1, 1),
        new QuHorizontalLine(),
        questionnairefunc::defaultGridRawPointer({
            {
                makeTitle(display_num_t, display_num_h),
                new QuLineEditInteger(display_num_fr, 0, max_display_num)
            },
            {
                "",
                (new QuBoolean(use_video_t, use_video_fr))->setAsTextButton(),
            },
            {
                "",
                (new QuBoolean(use_two_videos_t, use_two_videos_fr))->setAsTextButton(),
            },
            {
                media_directory_t,
                (new QuLineEdit(media_directory_fr))->setHint(media_directory_h)
            },
            {
                bmp_filename_1_t,
                new QuLineEdit(bmp_filename_1_fr)
            },
            {
                bmp_filename_2_t,
                new QuLineEdit(bmp_filename_2_fr)
            },
            {
                video_filename_1_t,
                new QuLineEdit(video_filename_1_fr)
            },
            {
                video_filename_2_t,
                new QuLineEdit(video_filename_2_fr)
            },
            {
                makeTitle(input_line_num_t, input_line_num_h),
                new QuLineEditInteger(input_line_num_fr, 0, max_line_num)
            },
            {
                makeTitle(output_line_num_t, output_line_num_h),
                new QuLineEditInteger(output_line_num_fr, 0, max_line_num)
            },
        }, 1, 1),
    });
    page->setTitle(tr("Configure Whisker demo task"));
    page->setType(QuPage::PageType::Config);

    auto questionnaire = new Questionnaire(m_app, {page});
    connect(questionnaire, &Questionnaire::completed,
            &app, &CamcopsApp::saveCachedVars);
    connect(questionnaire, &Questionnaire::cancelled,
            &app, &CamcopsApp::clearCachedVars);
    return questionnaire;
}


void WhiskerTestMenu::runDemoWhiskerTask()
{
    status(tr("Starting demo Whisker task"));  // ensures modal logbox
    ensureWhiskerManager();
    if (m_whisker->isConnected()) {
        status(tr("Whisker server already connected."));
        demoWhiskerTaskMain();
    } else {
        status(tr("Connecting to Whisker server..."));
        connect(m_whisker.data(), &WhiskerManager::onFullyConnected,
                this, &WhiskerTestMenu::demoWhiskerTaskMain,
                Qt::UniqueConnection);
        connectWhisker();
    }
}


void WhiskerTestMenu::demoWhiskerTaskMain()
{
    using namespace whiskerapi;
    using namespace whiskerconstants;

    status(tr("Setting up Whisker manager"));
    WhiskerManager* w = m_whisker.data();  // for briefer notation
    connect(w, &WhiskerManager::eventReceived,
            this, &WhiskerTestMenu::eventReceived,
            Qt::UniqueConnection);
    connect(w, &WhiskerManager::keyEventReceived,
            this, &WhiskerTestMenu::keyEventReceived,
            Qt::UniqueConnection);
    connect(w, &WhiskerManager::clientMessageReceived,
            this, &WhiskerTestMenu::clientMessageReceived,
            Qt::UniqueConnection);
    connect(w, &WhiskerManager::warningReceived,
            this, &WhiskerTestMenu::otherMessageReceived,
            Qt::UniqueConnection);
    connect(w, &WhiskerManager::syntaxErrorReceived,
            this, &WhiskerTestMenu::otherMessageReceived,
            Qt::UniqueConnection);
    connect(w, &WhiskerManager::errorReceived,
            this, &WhiskerTestMenu::otherMessageReceived,
            Qt::UniqueConnection);
    // ... all will autodisconnect when "this" is deleted, as the menu closes

    // We follow DemoConsoleClientTask.cpp from the Whisker SDK:

    // ------------------------------------------------------------------------
    // Additional constants
    // ------------------------------------------------------------------------
    const bool ignore_reply = true;
    const QColor black(0, 0, 0);
    const QColor red(255, 0, 0);
    const QColor green(0, 255, 0);
    const QColor blue(0, 0, 255);
    const QColor yellow(255, 255, 0);
    const QColor palergreen(0, 200, 0);
    const QColor darkred(100, 0, 0);
    const QColor darkcyan(0, 100, 100);
    const QColor darkyellow(100, 100, 0);
    const QColor vdarkgrey(50, 50, 50);

    // ------------------------------------------------------------------------
    // Variables
    // ------------------------------------------------------------------------
    const unsigned int display_num = m_display_num.toUInt();
    const bool use_video = m_use_video.toBool();
    const bool use_two_videos = m_use_two_videos.toBool();
    const QString media_directory = m_media_directory.toString();
    const QString bmp_filename_1 = m_bmp_filename_1.toString();
    const QString bmp_filename_2 = m_bmp_filename_2.toString();
    const QString video_filename_1 = m_video_filename_1.toString();
    const QString video_filename_2 = m_video_filename_2.toString();
    const unsigned int input_line_num = m_input_line_num.toUInt();
    const unsigned int output_line_num = m_output_line_num.toUInt();

    // ------------------------------------------------------------------------
    // Setup
    // ------------------------------------------------------------------------

    status(tr("Claiming devices and setting up display documents"));
    w->lineClaim(input_line_num, false, digital_input, ResetState::Leave);
    w->lineClaim(output_line_num, true, digital_output, ResetState::Leave);
    w->displayClaim(display_num, main_display);
    w->displayScaleDocuments(main_display, true, ignore_reply);
    if (!use_video) {
        DisplayCreationOptions dco;
        w->displayCreateDevice(second_display, dco);
        w->displayScaleDocuments(second_display, true, ignore_reply);
    }
    w->displayCreateDocument(doc, ignore_reply);
    w->displaySetDocumentSize(doc, QSize(1600, 1200), ignore_reply);
    w->displaySetBackgroundColour(doc, darkred, ignore_reply);

    // ------------------------------------------------------------------------
    // Simple objects
    // ------------------------------------------------------------------------

    status(tr("Creating simple display objects"));
    Pen pen(1, yellow, PenStyle::Solid);
    Brush brush(blue, darkcyan, true,
                BrushStyle::Solid, BrushHatchStyle::BDiagonal);

    w->displayAddObject(
                doc, line_obj,
                Line(QPoint(50, 50), QPoint(700, 700), pen),
                ignore_reply);
    w->displayAddObject(
                doc, arc_obj,
                Arc(QRect(100, 100, 300, 300),
                    QPoint(150, 100), QPoint(350, 100), pen),
                ignore_reply);
    w->displayAddObject(
                doc, bezier_obj,
                Bezier(QPoint(100, 100), QPoint(100, 400),
                       QPoint(400, 100), QPoint(400, 400), pen),
                ignore_reply);

    pen.width = 2;

    w->displayAddObject(
                doc, chord_obj,
                Chord(QRect(300, 300, 200, 200),
                      QPoint(300, 350), QPoint(500, 350), pen, brush),
                ignore_reply);

    brush.colour = palergreen;

    w->displayAddObject(
                doc, ellipse_obj,
                Ellipse(QRect(650, 100, 100, 300), pen, brush),
                ignore_reply);
    w->displayAddObject(
                doc, pie_obj,
                Pie(QRect(600, 300, 200, 200),
                    QPoint(800, 300), QPoint(800, 500), pen, brush),
                ignore_reply);

    brush.style = BrushStyle::Hatched;
    brush.opaque = false;

    w->displayAddObject(
                doc, rectangle_obj,
                Rectangle(QRect(150, 450, 100, 100), pen, brush),
                ignore_reply);
    w->displayAddObject(
                doc, roundrect_obj,
                RoundRect(QRect(900, 450, 300, 100),
                          QSize(150, 150), pen, brush),
                ignore_reply);

    brush.hatch_style = BrushHatchStyle::FDiagonal;
    brush.bg_colour = darkyellow;

    w->displayAddObject(
                doc, polygon_obj_1,
                Polygon(
                    // triangle
                    QVector<QPoint>{{400, 500}, {600, 450}, {600, 550}},
                    pen, brush, false),
                ignore_reply);
    w->displayAddObject(
                doc, polygon_obj_2,
                Polygon(
                    // pentagram
                    QVector<QPoint>{{150, 425},
                                    {75, 650},
                                    {250, 500},
                                    {50, 500},
                                    {225, 650}},
                    pen, brush, false),
                ignore_reply);

    Text text(QPoint(50, 50), tr("CamCOPS Whisker demo"), 0, "Times New Roman");
    text.italic = true;
    w->displayAddObject(doc, text_obj, text, ignore_reply);

    w->setMediaDirectory(media_directory, ignore_reply);
    w->displayAddObject(
                doc, bmp_obj_1,
                Bitmap(QPoint(100, 100), bmp_filename_1),
                ignore_reply);
    w->displayAddObject(
                doc, bmp_obj_2,
                Bitmap(QPoint(200, 200), bmp_filename_2),
                ignore_reply);

    w->displayAddObject(
                doc, camcogquadpattern_obj,
                CamcogQuadPattern(
                    QPoint(350, 100),
                    QSize(10, 8),
                    QVector<uint8_t>{1, 2, 3, 4, 5, 6, 7, 8},
                    QVector<uint8_t>{9, 10, 11, 12, 13, 14, 15, 16},
                    QVector<uint8_t>{255, 254, 253, 252, 251, 250, 249, 248},
                    QVector<uint8_t>{247, 246, 245, 244, 243, 242, 241, 240},
                    red,
                    green,
                    blue,
                    yellow,
                    vdarkgrey
                ),
                ignore_reply);

    auto setDemoEvents = [&](const QString& obj,
                             const QString& eventstem) -> void {
        w->displaySetEvent(doc, obj, DocEventType::MouseDown,
                           eventstem + suffix_event_clicked, ignore_reply);
        w->displaySetEvent(doc, obj, DocEventType::MouseUp,
                           eventstem + suffix_event_unclicked, ignore_reply);
        w->displaySetEvent(doc, obj, DocEventType::MouseMove,
                           eventstem + suffix_event_mouse_moved, ignore_reply);
        w->displaySetEvent(doc, obj, DocEventType::TouchDown,
                           eventstem + suffix_event_touched, ignore_reply);
        w->displaySetEvent(doc, obj, DocEventType::TouchUp,
                           eventstem + suffix_event_released, ignore_reply);
        w->displaySetEvent(doc, obj, DocEventType::TouchMove,
                           eventstem + suffix_event_touch_moved, ignore_reply);
    };

    setDemoEvents(bmp_obj_1, event_bmp_1);
    setDemoEvents(bmp_obj_2, event_bmp_2);
    setDemoEvents(ellipse_obj, event_ellipse);
    setDemoEvents(roundrect_obj, event_roundrect);
    setDemoEvents(rectangle_obj, event_rectangle);
    // setDemoEvents(strTextObj, event_text);
    setDemoEvents(polygon_obj_1, event_polygon_1);
    setDemoEvents(polygon_obj_2, event_polygon_2);
    setDemoEvents(chord_obj, event_chord);
    setDemoEvents(pie_obj, event_pie);
    setDemoEvents(camcogquadpattern_obj, event_camcogquadpattern);

    w->displaySetBackgroundEvent(doc, DocEventType::MouseDown,
                                 background_event + suffix_event_clicked,
                                 ignore_reply);
    w->displaySetBackgroundEvent(doc, DocEventType::TouchDown,
                                 background_event + suffix_event_touched,
                                 ignore_reply);

    // ------------------------------------------------------------------------
    // Video
    // ------------------------------------------------------------------------

    status(tr("Creating video objects"));
    auto setVideoDemoEvents = [&](const QString& obj,
                                  const QString& touchevent) -> void {
        w->displaySetEvent(doc, obj, DocEventType::MouseDown,
                           touchevent, ignore_reply);
        w->displaySetEvent(doc, obj, DocEventType::TouchDown,
                           touchevent, ignore_reply);
    };
    auto createVideoCluster = [&](const QString& prefix, int top) -> void {
        const QString strPlayObj(prefix + "play");
        const QString strPauseObj(prefix + "pause");
        const QString strStopObj(prefix + "stop");
        const QString strBackObj(prefix + "back");
        const QString strFwdObj(prefix + "fwd");

        brush.style = BrushStyle::Solid;
        brush.colour = blue;
        w->displayAddObject(
                    doc, strPlayObj,
                    Polygon(QVector<QPoint>{QPoint(800, top),
                                            QPoint(850, top + 25),
                                            QPoint(800, top + 50)},
                            pen, brush),
                    ignore_reply);

        brush.colour = black;
        w->displayAddObject(
                    doc, strPauseObj,
                    Rectangle(QRect(900, top, 50, 50), pen, brush),
                    ignore_reply);
        brush.colour = green;
        w->displayAddObject(
                    doc, strPauseObj,
                    Rectangle(QRect(900, top, 15, 50), pen, brush),
                    ignore_reply);
        w->displayAddObject(
                    doc, strPauseObj,
                    Rectangle(QRect(935, top, 15, 50), pen, brush),
                    ignore_reply);

        brush.colour = red;
        w->displayAddObject(
                    doc, strStopObj,
                    Rectangle(QRect(1000, top, 50, 50), pen, brush),
                    ignore_reply);

        brush.colour = black;
        w->displayAddObject(
                    doc, strBackObj,
                    Rectangle(QRect(1100, top, 50, 50), pen, brush),
                    ignore_reply);

        brush.colour = black;
        w->displayAddObject(
                    doc, strBackObj,
                    Polygon(QVector<QPoint>{QPoint(1125, top),
                                            QPoint(1100, top + 25),
                                            QPoint(1125, top + 50)},
                            pen, brush),
                    ignore_reply);
        w->displayAddObject(
                    doc, strBackObj,
                    Polygon(QVector<QPoint>{QPoint(1150, top),
                                            QPoint(1125, top + 25),
                                            QPoint(1150, top + 50)},
                            pen, brush),
                    ignore_reply);

        brush.colour = black;
        w->displayAddObject(
                    doc, strFwdObj,
                    Rectangle(QRect(1200, top, 50, 50), pen, brush),
                    ignore_reply);
        brush.colour = yellow;
        w->displayAddObject(
                    doc, strFwdObj,
                    Polygon(QVector<QPoint>{QPoint(1200, top),
                                            QPoint(1225, top + 25),
                                            QPoint(1200, top + 50)},
                            pen, brush),
                    ignore_reply);
        w->displayAddObject(
                    doc, strFwdObj,
                    Polygon(QVector<QPoint>{QPoint(1225, top),
                                            QPoint(1250, top + 25),
                                            QPoint(1225, top + 50)},
                            pen, brush),
                    ignore_reply);

        setVideoDemoEvents(strPlayObj, prefix + suffix_event_video_play);
        setVideoDemoEvents(strPauseObj, prefix + suffix_event_video_pause);
        setVideoDemoEvents(strStopObj, prefix + suffix_event_video_stop);
        setVideoDemoEvents(strFwdObj, prefix + suffix_event_video_fwd);
        setVideoDemoEvents(strBackObj, prefix + suffix_event_video_back);
    };

    if (use_video) {
        const QString strAudioDevice("audiodevice");

        w->audioClaim(1, strAudioDevice);
        // w->displaySetAudioDevice(0, 1);
        w->displaySetAudioDevice(main_display, strAudioDevice, ignore_reply);
        const int videotop1 = 50, videotop2 = 600;
        bool bLoop = true;

        w->displayAddObject(
                    doc, video_obj_1,
                    Video(QPoint(50, videotop1), video_filename_1, bLoop),
                    ignore_reply);
        if (use_two_videos) {
            w->displayAddObject(
                        doc, video_obj_2,
                        Video(QPoint(50, videotop2), video_filename_2, bLoop),
                        ignore_reply);
        }
        qDebug() << "~~~ Starting video 2 at 10s";
        w->videoSeekAbsolute(doc, video_obj_2, 10000, ignore_reply);
        w->videoTimestamps(true, ignore_reply);

        createVideoCluster(video_obj_1, 50);
        setVideoDemoEvents(video_obj_1, event_video_1_touched);
        if (use_two_videos) {
            createVideoCluster(video_obj_both, 300);
            createVideoCluster(video_obj_2, 600);
            setVideoDemoEvents(video_obj_2, event_video_2_touched);
        }
    }

    // ------------------------------------------------------------------------
    // OK; go.
    // ------------------------------------------------------------------------

    w->displayShowDocument(main_display, doc, ignore_reply);
    if (!use_video) {
        w->displayShowDocument(second_display, doc, ignore_reply);
    }

    w->displayKeyboardEvents(doc, KeyEventType::Both, false);  // don't ignore this reply

    w->lineSetEvent(digital_input, event_input_on, LineEventType::On, ignore_reply);
    w->lineSetEvent(digital_input, event_input_off, LineEventType::Off, ignore_reply);

    w->flashLinePulses(digital_output, n_flashes, flash_on_ms, flash_off_ms);

    w->timerSetEvent(event_single_tick, single_tick_period_ms,
                     0, ignore_reply);
    w->timerSetEvent(event_infinite_tick, infinite_tick_period_ms,
                     VAL_TIMER_INFINITE_RELOADS, ignore_reply);
    const int counted_tick_reloads = n_counted_ticks - 1;
    w->timerSetEvent(event_counted_tick, counted_tick_period_ms,
                     counted_tick_reloads, ignore_reply);

    status(tr(
        "All objects created. Try responding to display objects, providing "
        "keyboard input, toggling digital input lines via Whisker."));
}


void WhiskerTestMenu::eventReceived(const WhiskerInboundMessage& msg)
{
    ensureWhiskerManager();
    WhiskerManager* w = m_whisker.data();
    const QString event = msg.event();

    stream() << "Received event: " << event;

    // Video control:

    auto reportVideoTimings = [&]() -> void {
        unsigned int time = w->videoGetTimeMs(doc, video_obj_1);
        unsigned int duration = w->videoGetDurationMs(doc, video_obj_1);
        stream()
                << "video1 time: " << time
                << "; video1 duration: " << duration;
        time = w->videoGetTimeMs(doc, video_obj_2);
        duration = w->videoGetDurationMs(doc, video_obj_2);
        stream()
                << "video2 time: " << time
                << "; video2 duration: " << duration;
    };

    if (event == event_video_1_touched) {
        stream() << "~~~ Seeking video 1 forward 5s, playing video 1, pausing video 2";
        w->videoSeekRelative(doc, video_obj_1, FIVE_SEC);
        w->videoPlay(doc, video_obj_1);
        w->videoPause(doc, video_obj_2);
        reportVideoTimings();
    } else if (event == event_video_2_touched) {
        stream() << "~~~ Pausing video 1, seeking video 1 back 5s, playing video 2";
        w->videoSeekRelative(doc, video_obj_1, -FIVE_SEC);
        w->videoPlay(doc, video_obj_2);
        w->videoPause(doc, video_obj_1);
        reportVideoTimings();
    } else if (event == video_obj_1 + suffix_event_video_play) {
        stream() << "~~~ Playing video 1";
        w->videoPlay(doc, video_obj_1);
    } else if (event == video_obj_2 + suffix_event_video_play) {
        stream() << "~~~ Playing video 2";
        w->videoPlay(doc, video_obj_2);
    } else if (event == video_obj_both + suffix_event_video_play) {
        stream() << "~~~ Playing video 1+2";
        w->videoPlay(doc, video_obj_1);
        w->videoPlay(doc, video_obj_2);
    } else if (event == video_obj_1 + suffix_event_video_pause) {
        stream() << "~~~ Pausing video 1";
        w->videoPause(doc, video_obj_1);
    } else if (event == video_obj_2 + suffix_event_video_pause) {
        stream() << "~~~ Pausing video 2";
        w->videoPause(doc, video_obj_2);
    } else if (event == video_obj_both + suffix_event_video_pause) {
        stream() << "~~~ Pausing video 1+2";
        w->videoPause(doc, video_obj_1);
        w->videoPause(doc, video_obj_2);
    } else if (event == video_obj_1 + suffix_event_video_stop) {
        stream() << "~~~ Stopping video 1";
        w->videoStop(doc, video_obj_1);
    } else if (event == video_obj_2 + suffix_event_video_stop) {
        stream() << "~~~ Stopping video 2";
        w->videoStop(doc, video_obj_2);
    } else if (event == video_obj_both + suffix_event_video_stop) {
        stream() << "~~~ Stopping video 1+2";
        w->videoStop(doc, video_obj_1);
        w->videoStop(doc, video_obj_2);
    } else if (event == video_obj_1 + suffix_event_video_fwd) {
        stream() << "~~~ Fwding video 1";
        w->videoSeekRelative(doc, video_obj_1, FIVE_SEC);
    } else if (event == video_obj_2 + suffix_event_video_fwd) {
        stream() << "~~~ Fwding video 2";
        w->videoSeekRelative(doc, video_obj_2, FIVE_SEC);
    } else if (event == video_obj_both + suffix_event_video_fwd) {
        stream() << "~~~ Fwding video 1+2";
        w->videoSeekRelative(doc, video_obj_1, FIVE_SEC);
        w->videoSeekRelative(doc, video_obj_2, FIVE_SEC);
    } else if (event == video_obj_1 + suffix_event_video_back) {
        stream() << "~~~ Backing video 1";
        w->videoSeekRelative(doc, video_obj_1, -FIVE_SEC);
    } else if (event == video_obj_2 + suffix_event_video_back) {
        stream() << "~~~ Backing video 2";
        w->videoSeekRelative(doc, video_obj_2, -FIVE_SEC);
    } else if (event == video_obj_both + suffix_event_video_back) {
        stream() << "~~~ Backing video 1+2";
        w->videoSeekRelative(doc, video_obj_1, -FIVE_SEC);
        w->videoSeekRelative(doc, video_obj_2, -FIVE_SEC);
    }
}


void WhiskerTestMenu::keyEventReceived(const WhiskerInboundMessage& msg)
{
    const QString description = msg.keyEventDown()
            ? "down"
            : (msg.keyEventUp() ? "up" : "?");
    stream() << "Key event: keycode " << msg.keyEventCode()
             << ", " << description
             << " (from document " << msg.keyEventDoc() << ")";
}


void WhiskerTestMenu::clientMessageReceived(const WhiskerInboundMessage& msg)
{
    stream() << "Client message from client "
             << msg.clientMessageSourceClientNum()
             << ": " << msg.clientMessage();
}


void WhiskerTestMenu::otherMessageReceived(const WhiskerInboundMessage& msg)
{
    stream() << msg.message();
}


void WhiskerTestMenu::taskCancelled()
{
    deleteLogBox();
    if (m_whisker) {
        m_whisker->disconnectServerAndSignals(this);
    }
}


QVariant WhiskerTestMenu::getValue(const QVariant* member) const
{
    return *member;
}


bool WhiskerTestMenu::setValue(QVariant* member, const QVariant& value)
{
    const bool changed = value != *member;
    if (changed) {
        *member = value;
    }
    return changed;
}


void WhiskerTestMenu::ensureLogBox()
{
    if (!m_logbox) {
        m_logbox = new LogBox(this, tr("Whisker test task"), true);
        m_logbox->setStyleSheet(
                    m_app.getSubstitutedCss(uiconst::CSS_CAMCOPS_MAIN));
        m_logbox->useWaitCursor(false);
        connect(m_logbox.data(), &LogBox::rejected,
                this, &WhiskerTestMenu::taskCancelled,
                Qt::UniqueConnection);
        m_logbox->open();
    }
}


void WhiskerTestMenu::deleteLogBox()
{
    if (!m_logbox) {
        return;
    }
    m_logbox->deleteLater();
    m_logbox = nullptr;
}


void WhiskerTestMenu::status(const QString& msg)
{
    ensureLogBox();
    m_logbox->statusMessage(
                QString("%1: %2").arg(datetime::nowTimestamp(), msg));
}


WhiskerTestMenu::StatusStream::StatusStream(WhiskerTestMenu& parent) :
    QTextStream(&m_str),
    m_parent(parent)
{
}


WhiskerTestMenu::StatusStream::~StatusStream()
{
    // If inhering from std::stringstream:
    //      QString s = QString::fromStdString(str());
    //      m_parent.status(s);
    //
    // If inheriting from QTextStream:

    flush();
    m_parent.status(m_str);
}


WhiskerTestMenu::StatusStream WhiskerTestMenu::stream()
{
    // Tricky, as std::stringstream has a deleted copy constructor.
    // (So does QTextStream.)
    // You can't do this:
    //      return StatusStream(*this);
    // ... or you get:
    //      error: use of deleted function ‘WhiskerTestMenu::StatusStream::StatusStream(const WhiskerTestMenu::StatusStream&)’
    //
    // https://stackoverflow.com/questions/3442520/how-copy-from-one-stringstream-object-to-another-in-c
    // https://stackoverflow.com/questions/7935639/can-we-return-objects-having-a-deleted-private-copy-move-constructor-by-value-fr

    return {*this};
}
