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
#include <QSize>
#include "db/fieldref.h"
#include "questionnairelib/quelement.h"

class BooleanWidget;


class QuBoolean : public QuElement
{
    // Element to control a single Boolean field.
    //
    // Displays one of the following formats:
    //
    // - text button
    //
    //      +------+        QuBoolean("", fieldref);
    //      | text |        setAsTextButton(true);
    //      +------+
    //
    // - response widget ("indicator") and text
    //
    //      +-+             QuBoolean("text", fieldref);
    //      |✓| text
    //      +-+
    //
    // - response widget ("indicator") and icon
    //
    //      +-+             QuBoolean(iconfilename, QSize(), fieldref);
    //      |×| icon
    //      +-+
    //
    // The formats can be customized further.

    Q_OBJECT

protected:
    // Protected constructor
    QuBoolean(const QString& text, const QString& filename, const QSize& size,
              FieldRefPtr fieldref);

public:

    // Construct with: text to display; fieldref
    QuBoolean(const QString& text, FieldRefPtr fieldref);

    // Construct with: icon filename, icon size, fieldref.
    // If size == QSize(), that means "the file's intrinsic image size"
    QuBoolean(const QString& filename, const QSize& size,
              FieldRefPtr fieldref);

    // Is the content (text or image) clickable, as well as the response
    // widget?
    QuBoolean* setContentClickable(bool clickable = true);

    // Should the indicator (response widget) be on the left of the content
    // (text or image) or not ("not" meaning: put it on the right instead).
    QuBoolean* setIndicatorOnLeft(bool indicator_on_left = true);

    // Should the indicator be bigger than usual? See BooleanWidget::setSize().
    QuBoolean* setBigIndicator(bool big = true);

    // Should the text be bigger than usual (uiconst::FontSize::Big rather than
    // uiconst::FontSize::Normal)?
    QuBoolean* setBigText(bool big = true);

    // Should the text be bold?
    QuBoolean* setBold(bool bold = true);

    // Should the text be italic?
    QuBoolean* setItalic(bool italic = true);

    // If you call setAllowUnset(true), cycle NULL -> true -> false -> NULL.
    // Otherwise (the default), cycle NULL -> true -> false -> true.
    QuBoolean* setAllowUnset(bool allow_unset = true);

    // Puts the widget into "text button" mode (see above).
    QuBoolean* setAsTextButton(bool as_text_button = true);

    // Adjust the image for the current DPI setting, so it appears a standard
    // physical size?
    QuBoolean* setAdjustImageForDpi(bool adjust_image_for_dpi = true);

    // This is a bit unusual. If set to true, the "false" state appears blank.
    // This allows you to make a tick appear/disappear (not be replaced by
    // a cross).
    //
    // You almost certainly do not want to combine it with setAllowUnset(true),
    // because it may become visually hard to distinguish NULL from false. (If
    // the field is also mandatory, it will be visually possible, but still
    // confusing.)
    QuBoolean* setFalseAppearsBlank(bool false_appears_blank = true);

protected:
    // Sets the widget state from our fieldref.
    void setFromField();

    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;
    virtual FieldRefPtrList fieldrefs() const override;

protected slots:
    // "Our internal widget was clicked."
    void clicked();

    // "Fieldref reports that the field's data has changed."
    void fieldValueChanged(const FieldRef* fieldref);

protected:
    QString m_text;  // text (label)
    QString m_image_filename;  // filename for icon
    QSize m_image_size;  // size of icon
    bool m_adjust_image_for_dpi;  // rescale the image?
    FieldRefPtr m_fieldref;  // our fieldref
    bool m_content_clickable;  // is the text or icon clickable?
    bool m_indicator_on_left;  // "indicator widget" not "widget indicator"?
    bool m_big_indicator;  // big indicator?
    bool m_big_text;  // big text?
    bool m_bold;   // bold text?
    bool m_italic;  // italic text?
    bool m_allow_unset;  // allow setting back to NULL?
    bool m_as_text_button;  // text button, not tickbox indicator?
    bool m_false_appears_blank;  // false appears unticked?
    QPointer<BooleanWidget> m_indicator;  // tickbox indicator
};
