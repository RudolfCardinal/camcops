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
#include <QList>
#include <QPointer>
#include "db/fieldref.h"
#include "questionnairelib/namevalueoptions.h"
#include "questionnairelib/quelement.h"

class BooleanWidget;


class QuMcq : public QuElement
{
    // Offers a single multiple-choice question.
    // There are a variety of display formats.

    Q_OBJECT
public:

    // Constructor
    QuMcq(FieldRefPtr fieldref, const NameValueOptions& options);

    // Shuffle the options (when making the widget)?
    QuMcq* setRandomize(bool randomize);

    // Show the instruction "Pick one:"?
    QuMcq* setShowInstruction(bool show_instruction = true);

    // Layout as horizontal:
    //
    //      ( ) Option 1 ( ) Option 2 ( ) Option 3
    //
    // ... rather than vertical:
    //
    //      ( ) Option 1
    //      ( ) Option 2
    //      ( ) Option 3
    //
    QuMcq* setHorizontal(bool horizontal = true);

    // Show as text buttons:
    //
    //      +----------+
    //      | Option 1 |
    //      +----------+
    //      +----------+
    //      | Option 2 |
    //      +----------+
    //      +----------+
    //      | Option 3 |
    //      +----------+
    //
    // or
    //
    //      +----------+ +----------+ +----------+
    //      | Option 1 | | Option 2 | | Option 3 |
    //      +----------+ +----------+ +----------+
    //
    // ... rather than radio buttons (as above)?
    QuMcq* setAsTextButton(bool as_text_button = true);

    // Make text bold?
    QuMcq* setBold(bool bold = true);

protected:
    // Set widget state from field data.
    void setFromField();

    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;
    virtual FieldRefPtrList fieldrefs() const override;

protected slots:
    // "Button with index 'index' has been clicked."
    void clicked(int index);

    // "Field data has changed. Update the widgets."
    void fieldValueChanged(const FieldRef* fieldref);

protected:
    FieldRefPtr m_fieldref;  // field
    NameValueOptions m_options;  // possible options
    bool m_randomize;  // shuffle the order?
    bool m_show_instruction;  // show instruction?
    bool m_horizontal;  // horizontal layout?
    bool m_as_text_button;  // text button (rather than radio button) layout?
    bool m_bold;  // text in bold?
    QVector<QPointer<BooleanWidget>> m_widgets;  // our widget collection
};
