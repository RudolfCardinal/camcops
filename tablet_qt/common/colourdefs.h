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

#pragma once

/*

===============================================================================
RATIONALE: static/module-level initialization of QColor objects
===============================================================================

For the QColor objects below:

(1) Do NOT do this:

        // uiconst.cpp
        const QColor RED(255, 0, 0);

        // elsewhere
        const QColor EDGE_COLOUR = uiconst::RED;
        const QColor EDGE_COLOUR(uiconst::RED);

    ... because the object in the other compilation unit may not be initialized
    when you need it.
    http://stackoverflow.com/questions/211237/static-variables-initialisation-order

(2) This is probably OK:

         const QColor& EDGE_COLOUR = uiconst::RED;

(3) But you might as well do this:

         const QColor EDGE_COLOUR("red");

    based on the standard names:

         http://doc.qt.io/qt-5/qcolor.html#setNamedColor
         https://www.w3.org/TR/SVG/types.html#ColorKeywords

(4) Ah, not even that; the QColor lookup table (in qcolor.cpp) may not be
    initialized yet. Use numbers! Hence this file.

    Using #define means it's impossible to fail to have these initialized in
    time, and this gives us a name-to-number mapping.

    Defining to QColor(r, g, b) means we can use it wherever a QColor is
    required, even if it's slightly less efficient to do e.g.

        const THE_COLOUR(QCOLOR_PURPLE);

(5) Finally, note that it *should* work but this is a compiler bug. See below.


More detail on (4)/(5), with working thoughts

    The name-based QColor constructor:
        const QColor TEST_COLOUR("purple");
    works fine under Linux but gives an invalid colour (shown as black)
    under Windows. Is QT_NO_COLORNAMES defined somewhere? Otherwise it's
    odd. (Source is in qt5\qtbase\src\gui\painting\qcolor.cpp.)
    These are meant to be as at
        https://www.w3.org/TR/SVG/types.html#ColorKeywords

    Ah. Within a function, that works fine.
    Is it a static initialization order problem?
    With reference to https://isocpp.org/wiki/faq/ctors#static-init-order :
    Presumably the two relevant bits are

        ided3d.cpp:
            const QColor TEST_COLOUR("purple");

            -> qcolor.h
                inline QColor(const char *aname) : QColor(QLatin1String(aname)) {}
            -> qcolor.h
                inline QColor::QColor(QLatin1String name) { setNamedColor(aname); }
            -> qcolor.cpp
                void QColor::setNamedColor(const QString &name) {
                    setColorFromString(qToStringViewIgnoringNull(name));
                }
            -> qcolor.cpp
                template <typename String>
                bool QColor::setColorFromString(String name) {
                    // ...
                    if (get_named_rgb(name.data(), name.size(), &rgb)) {
                    // ...
                }
            -> qcolor.cpp
                static bool get_named_rgb(const char *name, int len, QRgb* rgb)
            -> qcolor.cpp
                static bool get_named_rgb_no_space(const char *name_no_space, QRgb *rgb)
                // reads rgbTbl

        qcolor.cpp:
            static const struct RGBData {
                const char name[21];
                uint  value;
            } rgbTbl[] = {
                { "aliceblue", rgb(240, 248, 255) },
                // ...
            };

    By the rules, rgbTbl should be initialized before get_named_rgb_no_space
    can be called, since they're in the same compilation unit. [INCORRECT]

    Possibly this is a Visual C++ bug; see
    https://blogs.msdn.microsoft.com/vcblog/2016/03/31/visual-c-2015-update-2-bug-fixes/
    and search for "init".
    I'm using (2018-04-29) Visual C++ 19.00.23918, which is Update 2 of VS 2015.
    Let's try Visual Studio Community 2017.

        - install the "C++" (and "C++ for Linux") options
        - goes into C:\Program Files (x86)\Microsoft Visual Studio\2017
        ... may have to reinstall with Windows Defender disabled;
            https://developercommunity.visualstudio.com/content/problem/161692/packageidmicrosoftvisualstudiosetupconfigurepa-4.html

        Once it's properly installed, you should find several copies of cl.exe
        within that tree, and Qt Creator 4.6.0 autodetects it as
            Microsoft Visual C++ Compiler 15.0
        (in several flavours). Note the variations:
            x86         compile on 32-bit for 32-bit
            amd64       compile on 64-bit for 64-bit
            x86_amd64   cross-compile on 32-bit for 64-bit
            amd64_x86   cross-compile on 64-bit for 32-bit

        You can also run

            C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat amd64

        or similar, and then when you run "cl.exe" it'll pick the right one.
        This gives Visual Studio 2017 Developer Command Prompt of 15.6.7 and a
        cl.exe version number of 19.13.26132.

        Using this kit warns you that it may be incompatible with Qt built with
        the MSVC2015 compiler.
        It does put camcops.exe in the wrong directory, e.g.
            ...\camcops\build-camcops-Custom_Qt_5_10_0_VS2017-Release\release
        rather than
            ...\camcops\build-camcops-Custom_Qt_5_10_0_VS2017-Release
        so move it up one directory at the end. (Makefile problem?)

    QColor problem still not fixed.
    Let's try building Qt with VS2017.

        Now, after a cleanup and rebuild, the .exe is built in the right place.
        However, the QColor problem remains.

    OK, what are the rules?
    - http://en.cppreference.com/w/cpp/language/initialization
      I imagine const blah = QColor("purple") comes under
      dynamic initialization / ordered dynamic initialization. So perhaps the
      key phrase is "Initialization of static variables in different
      translation units is indeterminately sequenced."
    - https://stackoverflow.com/questions/1005685/c-static-initialization-order
    - I guess the guarantee that we'd hope for, but which is not correct, is
      that (in qcolor.cpp) rgbTbl is initialized before
      get_named_rgb_no_space() can be called.
    - Reported to Qt.

    FINAL OUTCOME:
    I was wrong, and it is a Microsoft Visual Studio 2017 C++ compiler bug.
    The rgbTbl is subject to static initialization, whilst "const QColor x();"
    is dynamic initialization. Static initialization is meant to be guaranteed
    to be complete before dynamic initialization starts. The Microsoft Visual
    Studio 2017 C++ Compiler (VC++ up to and including 15.6, 15.7) is broken;
    see
        https://bugreports.qt.io/browse/QTBUG-68012

    Decision for CamCOPS
    - keep this #define system; it's very legible and it works around the
      broken compiler.

*/


// ============================================================================
// W3C SVG standard colour names
// ============================================================================
// - https://www.w3.org/TR/SVG/types.html#ColorKeywords
// - https://en.wikipedia.org/wiki/Web_colors
// - all have a=255 (fully opaque)

#define QCOLOR_ALICEBLUE QColor(240, 248, 255)
#define QCOLOR_ANTIQUEWHITE QColor(250, 235, 215)
#define QCOLOR_AQUA QColor(0, 255, 255)
#define QCOLOR_AQUAMARINE QColor(127, 255, 212)
#define QCOLOR_AZURE QColor(240, 255, 255)
#define QCOLOR_BEIGE QColor(245, 245, 220)
#define QCOLOR_BISQUE QColor(255, 228, 196)
#define QCOLOR_BLACK QColor(0, 0, 0)
#define QCOLOR_BLANCHEDALMOND QColor(255, 235, 205)
#define QCOLOR_BLUE QColor(0, 0, 255)
#define QCOLOR_BLUEVIOLET QColor(138, 43, 226)
#define QCOLOR_BROWN QColor(165, 42, 42)
#define QCOLOR_BURLYWOOD QColor(222, 184, 135)
#define QCOLOR_CADETBLUE QColor(95, 158, 160)
#define QCOLOR_CHARTREUSE QColor(127, 255, 0)
#define QCOLOR_CHOCOLATE QColor(210, 105, 30)
#define QCOLOR_CORAL QColor(255, 127, 80)
#define QCOLOR_CORNFLOWERBLUE QColor(100, 149, 237)
#define QCOLOR_CORNSILK QColor(255, 248, 220)
#define QCOLOR_CRIMSON QColor(220, 20, 60)
#define QCOLOR_CYAN QColor(0, 255, 255)
#define QCOLOR_DARKBLUE QColor(0, 0, 139)
#define QCOLOR_DARKCYAN QColor(0, 139, 139)
#define QCOLOR_DARKGOLDENROD QColor(184, 134, 11)
#define QCOLOR_DARKGRAY QColor(169, 169, 169)
#define QCOLOR_DARKGREEN QColor(0, 100, 0)
#define QCOLOR_DARKGREY QColor(169, 169, 169)
#define QCOLOR_DARKKHAKI QColor(189, 183, 107)
#define QCOLOR_DARKMAGENTA QColor(139, 0, 139)
#define QCOLOR_DARKOLIVEGREEN QColor(85, 107, 47)
#define QCOLOR_DARKORANGE QColor(255, 140, 0)
#define QCOLOR_DARKORCHID QColor(153, 50, 204)
#define QCOLOR_DARKRED QColor(139, 0, 0)
#define QCOLOR_DARKSALMON QColor(233, 150, 122)
#define QCOLOR_DARKSEAGREEN QColor(143, 188, 143)
#define QCOLOR_DARKSLATEBLUE QColor(72, 61, 139)
#define QCOLOR_DARKSLATEGRAY QColor(47, 79, 79)
#define QCOLOR_DARKSLATEGREY QColor(47, 79, 79)
#define QCOLOR_DARKTURQUOISE QColor(0, 206, 209)
#define QCOLOR_DARKVIOLET QColor(148, 0, 211)
#define QCOLOR_DEEPPINK QColor(255, 20, 147)
#define QCOLOR_DEEPSKYBLUE QColor(0, 191, 255)
#define QCOLOR_DIMGRAY QColor(105, 105, 105)
#define QCOLOR_DIMGREY QColor(105, 105, 105)
#define QCOLOR_DODGERBLUE QColor(30, 144, 255)
#define QCOLOR_FIREBRICK QColor(178, 34, 34)
#define QCOLOR_FLORALWHITE QColor(255, 250, 240)
#define QCOLOR_FORESTGREEN QColor(34, 139, 34)
#define QCOLOR_FUCHSIA QColor(255, 0, 255)
#define QCOLOR_GAINSBORO QColor(220, 220, 220)
#define QCOLOR_GHOSTWHITE QColor(248, 248, 255)
#define QCOLOR_GOLD QColor(255, 215, 0)
#define QCOLOR_GOLDENROD QColor(218, 165, 32)
#define QCOLOR_GRAY QColor(128, 128, 128)
#define QCOLOR_GREY QColor(128, 128, 128)
#define QCOLOR_GREEN QColor(0, 128, 0)
#define QCOLOR_GREENYELLOW QColor(173, 255, 47)
#define QCOLOR_HONEYDEW QColor(240, 255, 240)
#define QCOLOR_HOTPINK QColor(255, 105, 180)
#define QCOLOR_INDIANRED QColor(205, 92, 92)
#define QCOLOR_INDIGO QColor(75, 0, 130)
#define QCOLOR_IVORY QColor(255, 255, 240)
#define QCOLOR_KHAKI QColor(240, 230, 140)
#define QCOLOR_LAVENDER QColor(230, 230, 250)
#define QCOLOR_LAVENDERBLUSH QColor(255, 240, 245)
#define QCOLOR_LAWNGREEN QColor(124, 252, 0)
#define QCOLOR_LEMONCHIFFON QColor(255, 250, 205)
#define QCOLOR_LIGHTBLUE QColor(173, 216, 230)
#define QCOLOR_LIGHTCORAL QColor(240, 128, 128)
#define QCOLOR_LIGHTCYAN QColor(224, 255, 255)
#define QCOLOR_LIGHTGOLDENRODYELLOW QColor(250, 250, 210)
#define QCOLOR_LIGHTGRAY QColor(211, 211, 211)
#define QCOLOR_LIGHTGREEN QColor(144, 238, 144)
#define QCOLOR_LIGHTGREY QColor(211, 211, 211)
#define QCOLOR_LIGHTPINK QColor(255, 182, 193)
#define QCOLOR_LIGHTSALMON QColor(255, 160, 122)
#define QCOLOR_LIGHTSEAGREEN QColor(32, 178, 170)
#define QCOLOR_LIGHTSKYBLUE QColor(135, 206, 250)
#define QCOLOR_LIGHTSLATEGRAY QColor(119, 136, 153)
#define QCOLOR_LIGHTSLATEGREY QColor(119, 136, 153)
#define QCOLOR_LIGHTSTEELBLUE QColor(176, 196, 222)
#define QCOLOR_LIGHTYELLOW QColor(255, 255, 224)
#define QCOLOR_LIME QColor(0, 255, 0)
#define QCOLOR_LIMEGREEN QColor(50, 205, 50)
#define QCOLOR_LINEN QColor(250, 240, 230)
#define QCOLOR_MAGENTA QColor(255, 0, 255)
#define QCOLOR_MAROON QColor(128, 0, 0)
#define QCOLOR_MEDIUMAQUAMARINE QColor(102, 205, 170)
#define QCOLOR_MEDIUMBLUE QColor(0, 0, 205)
#define QCOLOR_MEDIUMORCHID QColor(186, 85, 211)
#define QCOLOR_MEDIUMPURPLE QColor(147, 112, 219)
#define QCOLOR_MEDIUMSEAGREEN QColor(60, 179, 113)
#define QCOLOR_MEDIUMSLATEBLUE QColor(123, 104, 238)
#define QCOLOR_MEDIUMSPRINGGREEN QColor(0, 250, 154)
#define QCOLOR_MEDIUMTURQUOISE QColor(72, 209, 204)
#define QCOLOR_MEDIUMVIOLETRED QColor(199, 21, 133)
#define QCOLOR_MIDNIGHTBLUE QColor(25, 25, 112)
#define QCOLOR_MINTCREAM QColor(245, 255, 250)
#define QCOLOR_MISTYROSE QColor(255, 228, 225)
#define QCOLOR_MOCCASIN QColor(255, 228, 181)
#define QCOLOR_NAVAJOWHITE QColor(255, 222, 173)
#define QCOLOR_NAVY QColor(0, 0, 128)
#define QCOLOR_OLDLACE QColor(253, 245, 230)
#define QCOLOR_OLIVE QColor(128, 128, 0)
#define QCOLOR_OLIVEDRAB QColor(107, 142, 35)
#define QCOLOR_ORANGE QColor(255, 165, 0)
#define QCOLOR_ORANGERED QColor(255, 69, 0)
#define QCOLOR_ORCHID QColor(218, 112, 214)
#define QCOLOR_PALEGOLDENROD QColor(238, 232, 170)
#define QCOLOR_PALEGREEN QColor(152, 251, 152)
#define QCOLOR_PALETURQUOISE QColor(175, 238, 238)
#define QCOLOR_PALEVIOLETRED QColor(219, 112, 147)
#define QCOLOR_PAPAYAWHIP QColor(255, 239, 213)
#define QCOLOR_PEACHPUFF QColor(255, 218, 185)
#define QCOLOR_PERU QColor(205, 133, 63)
#define QCOLOR_PINK QColor(255, 192, 203)
#define QCOLOR_PLUM QColor(221, 160, 221)
#define QCOLOR_POWDERBLUE QColor(176, 224, 230)
#define QCOLOR_PURPLE QColor(128, 0, 128)
#define QCOLOR_RED QColor(255, 0, 0)
#define QCOLOR_ROSYBROWN QColor(188, 143, 143)
#define QCOLOR_ROYALBLUE QColor(65, 105, 225)
#define QCOLOR_SADDLEBROWN QColor(139, 69, 19)
#define QCOLOR_SALMON QColor(250, 128, 114)
#define QCOLOR_SANDYBROWN QColor(244, 164, 96)
#define QCOLOR_SEAGREEN QColor(46, 139, 87)
#define QCOLOR_SEASHELL QColor(255, 245, 238)
#define QCOLOR_SIENNA QColor(160, 82, 45)
#define QCOLOR_SILVER QColor(192, 192, 192)
#define QCOLOR_SKYBLUE QColor(135, 206, 235)
#define QCOLOR_SLATEBLUE QColor(106, 90, 205)
#define QCOLOR_SLATEGRAY QColor(112, 128, 144)
#define QCOLOR_SLATEGREY QColor(112, 128, 144)
#define QCOLOR_SNOW QColor(255, 250, 250)
#define QCOLOR_SPRINGGREEN QColor(0, 255, 127)
#define QCOLOR_STEELBLUE QColor(70, 130, 180)
#define QCOLOR_TAN QColor(210, 180, 140)
#define QCOLOR_TEAL QColor(0, 128, 128)
#define QCOLOR_THISTLE QColor(216, 191, 216)
#define QCOLOR_TOMATO QColor(255, 99, 71)
#define QCOLOR_TURQUOISE QColor(64, 224, 208)
#define QCOLOR_VIOLET QColor(238, 130, 238)
#define QCOLOR_WHEAT QColor(245, 222, 179)
#define QCOLOR_WHITE QColor(255, 255, 255)
#define QCOLOR_WHITESMOKE QColor(245, 245, 245)
#define QCOLOR_YELLOW QColor(255, 255, 0)
#define QCOLOR_YELLOWGREEN QColor(154, 205, 50)

// ============================================================================
// Other
// ============================================================================
// R, G, B, A: a=0 means fully transparent, a=255 means fully opaque

#define QCOLOR_TRANSPARENT QColor(0, 0, 0, 0)
