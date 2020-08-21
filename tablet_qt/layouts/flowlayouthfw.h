// As per http://doc.qt.io/qt-5/qtwidgets-layouts-flowlayout-example.html

/*===========================================================================
==
== Copyright (C) 2016 The Qt Company Ltd.
== Contact: https://www.qt.io/licensing/
==
== This file is part of the examples of the Qt Toolkit.
==
== $QT_BEGIN_LICENSE:BSD$
== Commercial License Usage
== Licensees holding valid commercial Qt licenses may use this file in
== accordance with the commercial license agreement provided with the
== Software or, alternatively, in accordance with the terms contained in
== a written agreement between you and The Qt Company. For licensing terms
== and conditions see https://www.qt.io/terms-conditions. For further
== information use the contact form at https://www.qt.io/contact-us.
==
== BSD License Usage
== Alternatively, you may use this file under the terms of the BSD license
== as follows:
==
== "Redistribution and use in source and binary forms, with or without
== modification, are permitted provided that the following conditions are
== met:
==   * Redistributions of source code must retain the above copyright
==     notice, this list of conditions and the following disclaimer.
==   * Redistributions in binary form must reproduce the above copyright
==     notice, this list of conditions and the following disclaimer in
==     the documentation and/or other materials provided with the
==     distribution.
==   * Neither the name of The Qt Company Ltd nor the names of its
==     contributors may be used to endorse or promote products derived
==     from this software without specific prior written permission.
==
==
== THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
== "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
== LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
== A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
== OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
== SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
== LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
== DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
== THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
== (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
== OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
==
== $QT_END_LICENSE$
==
===========================================================================*/

#pragma once
#include <QLayout>
#include <QMap>  // RNC
#include <QRect>
#include <QStyle>


class FlowLayoutHfw : public QLayout
{
    // Flow layout, as per FlowLayout Qt demo, but modified to support
    // height-for-width better (better as in: word-wrapping labels etc. get the
    // maximum width allowed, aiming for the minimum height).

    // NOTE THAT THIS IS NOT A "TOP-LEVEL" HFW WIDGET THAT CAN RESIZE ITS
    // PARENT WIDGET (compare BoxLayoutHfw, GridLayoutHfw). So it should be
    // displayed within one of those.

    Q_OBJECT  // RNC
public:
    explicit FlowLayoutHfw(QWidget* parent, int margin = -1,
                           int h_spacing = -1, int v_spacing = -1);
    explicit FlowLayoutHfw(int margin = -1,
                           int h_spacing = -1, int v_spacing = -1);
    ~FlowLayoutHfw() override;

    // QLayout supplies: void addWidget(QWidget* w);  // no alignment option
    // QVBoxLayout/QHBoxLayout supply:
    //    void addWidget(QWidget* widget, int stretch, Qt::Alignment alignment);
    // Alignment does make sense, specifically top alignment.
    void addWidget(QWidget* w);  // RNC
    void addWidget(QWidget* w, Qt::Alignment alignment);  // RNC
    virtual void addItem(QLayoutItem* item) override;

    void setHorizontalAlignmentOfContents(Qt::Alignment halign);
    virtual int horizontalSpacing() const;
    virtual int verticalSpacing() const;
    virtual Qt::Orientations expandingDirections() const override;
    virtual bool hasHeightForWidth() const override;
    virtual int heightForWidth(int) const override;
    virtual int count() const override;
    virtual QLayoutItem* itemAt(int index) const override;
    virtual QSize minimumSize() const override;
    virtual void setGeometry(const QRect& rect) override;
    virtual QSize sizeHint() const override;
    virtual QLayoutItem* takeAt(int index) override;
    virtual void invalidate() override;  // RNC

protected:  // RNC (was private)

    // The main thinking function.
    virtual QSize doLayout(const QRect& rect, bool test_only) const;

    // Autocalculate spacing between items when none is specified explicitly.
    int smartSpacing(QStyle::PixelMetric pm) const;

    // Vertical coordinate of a given item, taking into account vertical
    // alignment.
    int itemTop(int row_top, int item_height, int row_height,
                Qt::Alignment valignment) const;  // RNC

    // Number of pixels to shift an entire row right, to satisfy the horizontal
    // alignment.
    int rowShiftToRight(int layout_width, int width_of_all_items) const;

    QVector<QLayoutItem*> m_item_list;  // our widgets
    int m_h_space;  // horizontal spacing between items
    int m_v_space;  // vertical spacing between rows
    mutable QSize m_size_hint;  // cached size hint
    mutable QMap<int, int> m_width_to_height;  // cached width-to-height map
    Qt::Alignment m_halign;  // horizontal alignment

    struct ItemCalc {
        QLayoutItem* item;  // the layout item
        QWidget* widget;  // the widget
        int layout_row;  // the row number this item will sit in
        QSize item_size;  // the item's size
        QPoint layout_cell_top_left;  // the item's top-left coordinate
    };
};
