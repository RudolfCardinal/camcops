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

#include <QLabel>
#include <QMap>
#include "common/gui_defines.h"


class LabelWordWrapWide : public QLabel
{
    // Label that word-wraps its text, and prefers to be wide rather than tall.
    // This is a surprisingly tricky thing to do.

    Q_OBJECT
public:

    // Construct with text.
    // The default size policy is preferredPreferredHFWPolicy().
    explicit LabelWordWrapWide(const QString& text, QWidget* parent = nullptr);

    // Default constructor.
    explicit LabelWordWrapWide(QWidget* parent = nullptr);

    virtual QSize sizeHint() const override;
    // Returns the size of non-word-wrapped (unwrapped) text.
    // ... "I would like to be very wide and not very tall."

    // - QLabel::heightForWidth() gives a sensible answer; no need to override.
    //   But sometimes helpful to see when it's being used:
    virtual bool hasHeightForWidth() const override;
    virtual int heightForWidth(int width) const override;

    // - QLabel::minimumSizeHint() gives a sensible answer (the size of the
    //   smallest individual word); no need to override.
    //   ... "I need to be big enough to contain my smallest word."
    //   ... EXCEPT that once resizeEvent() has used setFixedHeight(), it
    //       returns that as the minimum.
    //   ... and then it caches that.
    virtual QSize minimumSizeHint() const override;

    // - However, even with a size policy of Maximum/Fixed/hasHeightForWidth,
    //   the label's height does not increase as its width is decreased, unless
    //   you override resizeEvent().
    virtual void resizeEvent(QResizeEvent* event) override;

    // - resizeEvent() does the trick, but it isn't normally called when, for
    //   example, we set our text. So catch other events:
    bool event(QEvent* e) override;

public slots:
    // Set the text of the label.
    void setText(const QString& text);

protected:
    // Returns the height of our text, given a width.
    int qlabelHeightForWidth(int width) const;

    // If we weren't word-wrapping (i.e. if we were using a single line of
    // text), how big would we be?
    QSize sizeOfTextWithoutWrap() const;

#ifdef GUI_USE_RESIZE_FOR_HEIGHT
    // Set our height to an appropriate fixed value, given our width.
    void forceHeight();
#endif

    // How much extra space do we need to allocate for CSS features like
    // borders?
    QSize extraSizeForCssOrLayout() const;

    // Clear our cached information.
    // - Widgets shouldn't need to cache their size hints; that's done by layouts
    //   for them. See
    //   http://kdemonkey.blogspot.co.uk/2013/11/understanding-qwidget-layout-flow.html
    // - However, for performance... we'll cache some things.
    //   In particular, word-wrapping labels can get asked to calculate their
    //   width for a great many heights (sometimes repeatedly).
    // - Moreover, the application of stylesheets varies with time (so calls
    //   can be made prior to, and then after, application of
    //   stylesheets). So the caches must be cleared whenever things like that
    //   happen.
    void clearCache();

protected:
    mutable QSize m_cached_unwrapped_text_size;  // cached "single-line" text size
    mutable QSize m_cached_extra_for_css_or_layout;  // cached "extra size for CSS"
    mutable QMap<int, int> m_cached_qlabel_height_for_width;  // cached map of width -> height
};
