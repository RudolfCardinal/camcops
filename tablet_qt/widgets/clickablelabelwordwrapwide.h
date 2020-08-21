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

#include <QPushButton>
#include "common/gui_defines.h"
#include "layouts/layouts.h"
class LabelWordWrapWide;


class ClickableLabelWordWrapWide : public QPushButton
{
    // Label showing text that responds to clicks and word-wraps its text in a
    // height-for-width manner to take up width in preference to height.

    Q_OBJECT
public:

    // Construct with text.
    // - stretch: add stretch at the bottom of our layout?
    ClickableLabelWordWrapWide(const QString& text, bool stretch = false,
                               QWidget* parent = nullptr);

    // Construct without text. You can use setText() later.
    ClickableLabelWordWrapWide(bool stretch = false,
                               QWidget* parent = nullptr);

    // Set text format (e.g. plain text, rich text).
    virtual void setTextFormat(Qt::TextFormat format);

    // Should we word-wrap the text?
    virtual void setWordWrap(bool on);

    // Set alignment of text within our label widget (and of our label widget
    // within our layout).
    virtual void setAlignment(Qt::Alignment alignment);

    // Should URLs in the text behave like active hyperlinks?
    virtual void setOpenExternalLinks(bool open);

    // Set text for this label.
    virtual void setText(const QString& text);

    // Standard Qt widget overrides.
    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;
#ifdef GUI_USE_RESIZE_FOR_HEIGHT
    virtual void resizeEvent(QResizeEvent* event) override;
#endif

protected:
    // Translates the size required by m_label to the size required by the
    // whole QPushButton.
    QSize translateSize(const QSize& size) const;

protected:
    LabelWordWrapWide* m_label;  // our label (showing text)
    VBoxLayout* m_layout;  // our layout
};
