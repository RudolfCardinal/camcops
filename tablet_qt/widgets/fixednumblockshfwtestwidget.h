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
#include <QColor>
#include <QWidget>


class FixedNumBlocksHfwTestWidget : public QWidget
{
    // Test widget that has a fixed aspect ratio. It adjusts its height
    // according to its width.

    Q_OBJECT
public:
    FixedNumBlocksHfwTestWidget(
            int num_blocks = 20,
            const QSize& block_size = QSize(20, 30),  // unequal is more complex
            qreal preferred_aspect_ratio = 1.6,  // golden ratio
            const QColor& block_colour = QColor(100, 100, 100),
            const QColor& background_colour = QColor(0, 0, 100),
            const QColor& text_colour = QColor(255, 255, 255),
            QWidget* parent = nullptr);
    virtual QSize sizeHint() const override;
    virtual QSize minimumSizeHint() const override;
    virtual bool hasHeightForWidth() const override;
    virtual int heightForWidth(int width) const override;
    virtual void paintEvent(QPaintEvent* event) override;
protected:
    int m_n_blocks;
    QSize m_block_size;
    qreal m_preferred_aspect_ratio;
    QColor m_block_colour;
    QColor m_background_colour;
    QColor m_text_colour;
    QSize m_preferred_size;  // calculated
};
