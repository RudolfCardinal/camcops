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

This file allows you to test custom layouts by swapping between

(1) GUI_USE_RESIZE_FOR_HEIGHT

    ... widgets use the trick of
            MyWidget::resizeEvent(QResizeEvent* event) {
                int current_width = width();
                int new_height = heightForWidth(current_width);
                setFixedHeight(new_height);
                updateGeometry();
            }
    ... standard Qt layouts are used, e.g. QVBoxLayout, QHBoxLayout
    ... you also have to implement custom functionality on the widgets that are
        parents of height-for-width widgets in the layout heirarchy (and their
        parent, etc.), which is the particular annoyance

and (2) GUI_USE_HFW_LAYOUT

    ... widgets don't use the resizeEvent()
    ... we have custom layouts, e.g. VBoxLayoutHfw and HBoxLayoutHfw, which
        respect the height-for-width system in a different way
    ... generally preferable?

Files that make up a complete set of classes:

    common/gui_defines.h    - this master switch, for testing
    layouts/layouts.h        - choose Qt or custom layouts via master switch;
                              refer to them as VBoxLayout, HBoxLayout, etc.
                              and this file will choose the actual
                              implementation (*)
    lib/sizehelpers.h   }   - shortcuts for size policies/resize functions
    lib/sizehelpers.cpp }

    widgets/margins.h   }   - simple class to hold margins
    widgets/margins.cpp }

    widgets/basewidget.h    } widget class that'll act as a parent widget
    widgets/basewidget.cpp  }   for a height-for-width widget (*)

    layouts/boxlayouthfw.h      } replacements for QBoxLayout, QHBoxLayout,
    layouts/boxlayouthfw.cpp    }   QVBoxLayout, so you can leave your widget
    layouts/hboxlayouthfw.h     }   classes alone except for implementing
    layouts/hboxlayouthfw.cpp   }   hasHeightForWidth() and heightForWidth(),
    layouts/vboxlayouthfw.h     }   and have them sized properly for you
    layouts/vboxlayouthfw.cpp   }

    layouts/gridlayouthfw.h     } similar replacement for QGridLayout
    layouts/gridlayouthfw.cpp   }

    layouts/flowlayouthfw.h     } replacement for the Qt FlowLayout example
    layouts/flowlayouthfw.cpp   }   that handles height-for-width better

    layouts/qtlayouthelpers.h   - reimplemented (and re-namespaced) private Qt
                                    layout functions, used by the new layouts

    widgets/verticalscrollarea.h    } vertical scroll area that also implements
    widgets/verticalscrollarea.cpp  }   height-for-width

    widgets/labelwordwrapwide.h     } replacement for QLabel that tries to use
    widgets/labelwordwrapwide.cpp   }   as much width as possible before
                                        wrapping

    widgets/aspectratiopixmap.h   } widget to keep an image's aspect ratio
    widgets/aspectratiopixmap.cpp }   fixed

    (*) uses gui_defines.h to implement a master switch for testing

Relevant web discussions include:

    Qt docs: http://doc.qt.io/qt-5/layout.html#layout-issues
    2009-01-16: http://stackoverflow.com/questions/452333/how-to-maintain-widgets-aspect-ratio-in-qt
    2011-11-21: http://stackoverflow.com/questions/8211982/qt-resizing-a-qlabel-containing-a-qpixmap-while-keeping-its-aspect-ratio
    2012-12-31: http://stackoverflow.com/questions/14104871/qlabel-cutting-off-text-on-resize
    2013-01-09: http://stackoverflow.com/questions/14238138/heightforwidth-label
    2014-06-17: http://stackoverflow.com/questions/24264320/qt-layouts-keep-widget-aspect-ratio-while-resizing
    2015-03-30: http://www.qtcentre.org/threads/62059-QLabel-Word-Wrapping-adds-unnecessary-line-breaks
    2015-07-21: http://stackoverflow.com/questions/31535143/how-to-prevent-qlabel-from-unnecessary-word-wrapping

*/

// The master switch: leave it, or comment it out
#define GUI_USE_HFW_LAYOUT

// Switches that follow from it:
#ifndef GUI_USE_HFW_LAYOUT
#define GUI_USE_RESIZE_FOR_HEIGHT
#endif

// No silly combinations, please:
#if defined(GUI_USE_HFW_LAYOUT) == defined(GUI_USE_RESIZE_FOR_HEIGHT)
#error Define GUI_USE_HFW_LAYOUT xor GUI_USE_RESIZE_FOR_HEIGHT
#endif


/*

Notes for classes using these:
- The ONLY difference you should implement based on GUI_USE_HFW_LAYOUT
  is which layout class to use. Don't alter anything else

*/
