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
#include <QSvgWidget>


class SvgWidgetClickable : public QSvgWidget
{
    // Widget that displays an SVG graphics image, and is clickable.
    // Used for response elements (e.g. in ID/ED-3D task).
    //
    // We have a choice of deriving from QGraphicsSvgItem or QSvgWidget.
    // The main differences are:
    //
    // - QSvgWidget can be used in a more general context
    // - QSvgWidget owns its QSvgRenderer, but you can access it, while if you
    //   use a QGraphicsSvgItem, you have to manage the lifetime of the
    //   renderer separately, which is something of a pain.
    //
    // For clicks:
    // - https://stackoverflow.com/questions/36372615/how-can-i-capture-click-events-signals-of-a-qgraphicssvgitem
    //
    // For changing the background colour when pressed:
    // - The ":pseudo" stylesheet selector doesn't work; possibly that's
    //   only for QAbstractButton.
    // - You can't both override paintEvent() and call the base class
    //   implementation?
    //   - https://stackoverflow.com/questions/13897026/accessing-a-qpainter-in-base-class
    //   - http://doc.qt.io/qt-4.8/qpainter.html#begin
    // - Aha! You can. You just have to ensure the first QPainter is destroyed
    //   first. Done.

    Q_OBJECT

public:
    // Default constructor
    SvgWidgetClickable(QWidget* parentitem = nullptr);

    // Construct with SVG from a file.
    SvgWidgetClickable(const QString& filename,
                       QWidget* parentitem = nullptr);

    // Sets the SVG image from a string.
    void setSvgFromString(const QString& svg);

    // Sets the SVG image from a file.
    void setSvgFromFile(const QString& filename);

    // Sets the widget's normal background colour.
    void setBackgroundColour(const QColor& colour);

    // Sets the widget's background colour when it's being pressed.
    void setPressedBackgroundColour(const QColor& colour);

    // Should mouse events go "through" this widget (i.e. treat it like an
    // overlay)?
    void setTransparentForMouseEvents(bool transparent);

protected:
    // Standard Qt widget overrides.
    virtual void mousePressEvent(QMouseEvent* event) override;
    virtual void mouseMoveEvent(QMouseEvent* event) override;
    virtual void mouseReleaseEvent(QMouseEvent* event) override;
    virtual void paintEvent(QPaintEvent* event) override;

signals:
    // "Start of mouse/touch press."
    void pressed();

    // "A click has occurred." Press -> release = click.
    void clicked();

protected:
    QColor m_background_colour;  // normal background colour
    QColor m_pressed_background_colour;  // background colour whilst being pressed
    bool m_pressed;  // being pressed?
    bool m_pressing_inside;  // being pressed and cursor remains inside us?
};
