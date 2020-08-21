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

#include <QAbstractButton>
#include "layouts/layouts.h"
class ClickableLabelWordWrapWide;
class ImageButton;


class BooleanWidget : public QAbstractButton
{
    // - Encapsulates a widget that can take a variety of appearances, but
    //   embodies some or all of the states true, false, null (not required),
    //   null (required).
    // - Can display as an image or a text button. Because those things don't
    //   play nicely together, it owns widgets rather than inheriting.
    // - Main signal is: clicked
    // - RESIST the temptation to have this widget do value logic.
    //   That's the job of its owner.

    // - DON'T try multiple inheritance (inheriting from a custom ABC and
    //   a QObject). It just messes up ("can't convert to QObject", when
    //   access is from ABC pointer; etc.).

    Q_OBJECT
public:

    // Current widget state.
    enum class State {
        Disabled,  // disabled
        Null,  // no data, not required
        NullRequired,  // no data, but data is required
        False,  // false
        True,  // true
    };

    // Visual appearance -- the style of the widget.
    enum class Appearance {
        CheckBlack,  // checkbox (tickbox) with black tick (true)/cross (false)
        CheckBlackFalseAppearsBlank,  // checkbox; black; "false" looks blank
        CheckRed,  // checkbox with red tick (true)/cross (false)
        CheckRedFalseAppearsBlank,  // checkbox; red; "false" looks blank
        Radio,  // radio button (indicator is present/absent)
        Text,  // text button (state is shown via colour)
    };

public:

    // ------------------------------------------------------------------------
    // Construction and configuration
    // ------------------------------------------------------------------------

    // Constructor
    BooleanWidget(QWidget* parent = nullptr);

    // Should the widget be read-only (state is unalterable)?
    virtual void setReadOnly(bool read_only = false);

    // Show icons bigger than normal?
    void setSize(bool big = false);

    // Show text in bold?
    void setBold(bool bold);

    // Set the overall widget style (e.g. checkbox, radio button, text button).
    void setAppearance(BooleanWidget::Appearance appearance);

    // ------------------------------------------------------------------------
    // Manipulation of "live" state
    // ------------------------------------------------------------------------

    // Sets the widget state directly.
    void setState(BooleanWidget::State state);

    // Sets the widget state from a value and a mandatory-or-not requirement.
    void setValue(const QVariant& value, bool mandatory,
                  bool disabled = false);

    // Sets the text, for text-button mode.
    void setText(const QString& text);

protected:

    // Standard Qt override.
    virtual void paintEvent(QPaintEvent* e) override;

    // Refreshes the widget's appearance.
    // If full_refresh is true, the widget is rebuilt and its size may change.
    void updateWidget(bool full_refresh);

protected:
    bool m_read_only;  // read-only mode?
    bool m_big;  // icon size
    bool m_bold;  // for text button mode
    Appearance m_appearance;  // overall widget style
    bool m_as_image;  // set from m_appearance; "icon style"?
    State m_state;  // boolean state (allowing various null states too)
    ImageButton* m_imagebutton;  // our image button
    ClickableLabelWordWrapWide* m_textbutton;  // our text button
    VBoxLayout* m_layout;  // our master layout
};
