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

// #define DEBUG_PAINT

#include "fixednumblockshfwtestwidget.h"
#include <cmath>
#include <QBrush>
#include <QDebug>
#include <QPainter>
#include <QPen>
#include <QtMath>
#include "lib/sizehelpers.h"


FixedNumBlocksHfwTestWidget::FixedNumBlocksHfwTestWidget(
        const int num_blocks,
        const QSize& block_size,
        qreal preferred_aspect_ratio,
        const QColor& block_colour,
        const QColor& background_colour,
        const QColor& text_colour,
        QWidget* parent) :
    QWidget(parent),
    m_n_blocks(num_blocks),
    m_block_size(block_size),
    m_preferred_aspect_ratio(preferred_aspect_ratio),
    m_block_colour(block_colour),
    m_background_colour(background_colour),
    m_text_colour(text_colour)
{
    Q_ASSERT(m_n_blocks > 0);

    setSizePolicy(sizehelpers::preferredFixedHFWPolicy());

    // At some point -- for efficiency, now -- we need to calculate our
    // preferred width/height.
    //
    // Constraints:
    //
    // width_px = width_blocks * block_width
    // height_px = height_blocks * block_height
    // width_blocks * height_blocks >= m_n_blocks
    // preferred_aspect_ratio ~= width_px / height_px
    //
    // Find (width_px, height_px).
    //
    // This could be done as an integer linear programming problem.
    // - This may be a useful approach for finally sorting out the layouts...
    // - https://en.wikipedia.org/wiki/List_of_optimization_software, e.g.
    //   - https://www.coin-or.org/
    //     - https://github.com/coin-or/Clp
    //   - https://www.alglib.net/download.php
    //
    // A cruder but very likely much faster way:
    // - iterate through all values of width_blocks from 1 to m_n_blocks;
    // - pick the one with the smallest squared error in terms of aspect ratio.
    //
    // Iterating w_blocks down rather than up can be used to give a slight
    // preference for width over height (which is probably sensible).

    // Set non-crazy defaults:
    qreal best_w_blocks = m_n_blocks >= 2 ? (num_blocks / 2) : 1;
    qreal best_h_blocks = qCeil(static_cast<qreal>(m_n_blocks) /
                                static_cast<qreal>(best_w_blocks));
    // Hunt for something better:
    qreal best_sq_error = std::numeric_limits<qreal>::infinity();
    const qreal tolerance = 1e-3;
    for (int w_blocks = m_n_blocks; w_blocks > 0; --w_blocks) {
        const int h_blocks = qCeil(static_cast<qreal>(m_n_blocks) /
                                   static_cast<qreal>(w_blocks));
        const qreal w_px = w_blocks * m_block_size.width();
        const qreal h_px = h_blocks * m_block_size.height();
        const qreal aspect_ratio = w_px / h_px;
        const qreal sq_error = pow(aspect_ratio - m_preferred_aspect_ratio, 2);
        if (sq_error < best_sq_error) {
            // Found an improvement.
            best_sq_error = sq_error;
            best_w_blocks = w_blocks;
            best_h_blocks = h_blocks;
            if (sq_error < tolerance) {
                // Perfect enough.
                break;
            }
        }
    }

    m_preferred_size.rwidth() = best_w_blocks * m_block_size.width();
    m_preferred_size.rheight() = best_h_blocks * m_block_size.height();
}


QSize FixedNumBlocksHfwTestWidget::sizeHint() const
{
    return m_preferred_size;
}


QSize FixedNumBlocksHfwTestWidget::minimumSizeHint() const
{
    return m_block_size;
}


bool FixedNumBlocksHfwTestWidget::hasHeightForWidth() const
{
    return true;
}


int FixedNumBlocksHfwTestWidget::heightForWidth(const int width) const
{
    const int w_blocks = qMin(width / m_block_size.width(), m_n_blocks);
    if (w_blocks == 0) {
        // Avoid later attempts to divide by zero
        qWarning() << Q_FUNC_INFO << "problem: w_blocks == 0";
        return m_block_size.height();
    }
    const int h_blocks = qCeil(static_cast<qreal>(m_n_blocks) /
                               static_cast<qreal>(w_blocks));
    return h_blocks * m_block_size.height();
}


void FixedNumBlocksHfwTestWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    const QSize s = size();
    QRectF rect(QPoint(0, 0), s);

    const int w_px = s.width();
    const int h_px = s.height();
    const int hfw_px = heightForWidth(w_px);
    const int w_blocks = qMin(w_px / m_block_size.width(), m_n_blocks);
    if (w_blocks == 0) {
        // Avoid later attempts to divide by zero
        qWarning() << Q_FUNC_INFO << "problem: w_blocks == 0";
        return;
    }
    const int h_blocks = qCeil(static_cast<qreal>(m_n_blocks) /
                               static_cast<qreal>(w_blocks));
    const QString hfw_description = hfw_px == h_px
        ? "matches HFW"
        : QString("MISMATCH to HFW %1").arg(hfw_px);
    const QString description = QString(
        "Fixed #blocks; %1 x %2 px (%3); %4 x %5 blocks")
            .arg(w_px)
            .arg(h_px)
            .arg(hfw_description)
            .arg(w_blocks)
            .arg(h_blocks);

    const QPen text_pen(m_text_colour);
    const QBrush bg_brush(m_background_colour, Qt::SolidPattern);
    const QBrush block_brush(m_block_colour, Qt::SolidPattern);

#ifdef DEBUG_PAINT
    qDebug().nospace()
            << Q_FUNC_INFO
            << ": size = " << s
            << ", geometry = " << geometry()
            << ", w_blocks = " << w_blocks
            << ", h_blocks = " << h_blocks;
#endif

    QPainter painter(this);
    // Backgroud
    painter.setBrush(bg_brush);
    painter.drawRect(rect);
    // Blocks
    painter.setBrush(block_brush);
    for (int i = 0; i < m_n_blocks; ++i) {
        const int block_x = i % w_blocks;
        const int block_y = i / w_blocks;
        const int x = block_x * m_block_size.width();
        const int y = block_y * m_block_size.height();
        const QRectF block_rect(QPoint(x, y), m_block_size);
        painter.drawRect(block_rect);
    }
    // Text
    painter.setPen(text_pen);
    painter.drawText(
        rect,
        Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
        description
    );
}
