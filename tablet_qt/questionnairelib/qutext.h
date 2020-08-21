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
#include "db/fieldref.h"
#include "common/uiconst.h"
#include "questionnairelib/quelement.h"

class LabelWordWrapWide;


class QuText : public QuElement
{
    // Provides static text, or text from a field.

    Q_OBJECT

protected:
    // Protected constructor, used internally and by derived classes.
    QuText(const QString& text, FieldRefPtr fieldref);

public:

    // Constructor for static text.
    QuText(const QString& text = "");

    // Constructor for dynamic text, from a field.
    QuText(FieldRefPtr fieldref);

    // Set visual style of text.
    QuText* setFontSize(uiconst::FontSize fontsize);
    QuText* setBig(bool setBig = true);
    QuText* setBold(bool setBold = true);
    QuText* setItalic(bool setItalic = true);
    QuText* setWarning(bool setWarning = true);

    // Plain text, rich text, or autodetect?
    QuText* setFormat(Qt::TextFormat format);

    // Show URLs as active hyperlinks?
    QuText* setOpenLinks(bool open_links = true);

    // Set text alignment within the widget.
    QuText* setTextAlignment(Qt::Alignment alignment);

    // Set text alignment within the widget, and widget alignment within the
    // layout.
    QuText* setTextAndWidgetAlignment(Qt::Alignment alignment);

    // Change the "static" text.
    void setText(const QString& text, bool repolish = true);

    friend class SettingsMenu;

protected:
    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;

    // Force the font size manually. For SettingsMenu only, to demonstrate
    // font size.
    void forceFontSize(int fontsize_pt, bool repolish = true);

    // Sets the font size on our widget.
    void setWidgetFontSize(int fontsize_pt, bool repolish = false);

    // Forces our widget to repolish itself.
    void repolishWidget();

protected slots:

    // "The field's data has changed."
    void fieldValueChanged(const FieldRef* fieldref);

protected:
    QString m_text;  // static text
    FieldRefPtr m_fieldref;  // field for dynamic text
    uiconst::FontSize m_fontsize;  // font size (e.g. "big", "small")
    bool m_bold;  // bold?
    bool m_italic;  // italic?
    bool m_warning;  // warning style?
    Qt::TextFormat m_text_format;  // format (e.g. plain/rich/autodetect)
    bool m_open_links;  // offer hyperlinks for URLs?
    Qt::Alignment m_text_alignment;  // alignment of text in widget
    QPointer<LabelWordWrapWide> m_label;  // our widget
    int m_forced_fontsize_pt;  // the override font size, for special occasions
};
