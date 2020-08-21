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
#include "layouts/layouts.h"
#include "questionnairelib/namevalueoptions.h"
#include "questionnairelib/mcqgridsubtitle.h"
#include "questionnairelib/quelement.h"
#include "questionnairelib/questionwithtwofields.h"

class BooleanWidget;
class QuMcqGridDoubleSignaller;


class QuMcqGridDouble : public QuElement
{
    // Offers a grid of pairs of multiple-choice questions, where several
    // sets of questions share the same possible responses. For example:
    //
    //      TITLE
    //      |
    //      v
    //      Survey       How much do you like it?    How expensive is it?   <-- STEMS
    //                   Not at all ... Lots         Cheap ... Expensive    <-- OPTIONS (1, 2)
    //      1. Banana        O       O   O             O    O      O
    //      2. Diamond       O       O   O             O    O      O
    //      3. ...
    //
    //      ^
    //      |
    //      QUESTIONS
    //
    // See QuMcqGrid for the basics.

    Q_OBJECT
    friend class QuMcqGridDoubleSignaller;
public:

    // Constructor
    QuMcqGridDouble(const QVector<QuestionWithTwoFields>& questions_with_fields,
                    const NameValueOptions& options1,
                    const NameValueOptions& options2);

    // Destructor.
    virtual ~QuMcqGridDouble() override;

    // Set widths of question and option columns.
    QuMcqGridDouble* setWidth(int question_width,
                              const QVector<int>& option1_widths,
                              const QVector<int>& option2_widths);

    // Set the title.
    QuMcqGridDouble* setTitle(const QString& title);

    // Set the subtitles. See McqGridSubtitle.
    QuMcqGridDouble* setSubtitles(const QVector<McqGridSubtitle>& subtitles);

    // Ask widgets to expand horizontally?
    QuMcqGridDouble* setExpand(bool expand);

    // Apply a stripy background to the grid?
    QuMcqGridDouble* setStripy(bool stripy);

    // Set the stem text (see above).
    QuMcqGridDouble* setStems(const QString& stem1, const QString& stem2);

protected:
    // Set the widget state from the fields' data.
    void setFromFields();

    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;
    virtual FieldRefPtrList fieldrefs() const override;

    // Return the spacer column number for the first question or the second.
    int spacercol(bool first_field) const;

    // Return the column number for a given option/value index (zero-based),
    // in either the first field/question, or the second.
    int colnum(bool first_field, int value_index) const;

    // Internal function to add options to a grid.
    void addOptions(GridLayout* grid, int row);

protected slots:
    // "One of the response widgets was clicked/touched."
    void clicked(int question_index, bool first_field, int value_index);

    // "A field's value, or a field's mandatory status, has changed."
    void fieldValueOrMandatoryChanged(int question_index, bool first_field,
                                      const FieldRef* fieldref);

protected:
    QVector<QuestionWithTwoFields> m_questions_with_fields;  // question/field mapping
    NameValueOptions m_options1;  // options for stem 1
    NameValueOptions m_options2;  // options for stem 2
    int m_question_width;  // relative column width for question column
    QVector<int> m_option1_widths;  // relative column widths for options 1
    QVector<int> m_option2_widths;  // relative column widths for options 2
    QString m_title;  // overall title
    QVector<McqGridSubtitle> m_subtitles;  // subtitle info
    bool m_expand;  // expand our widgets horizontally?
    bool m_stripy;  // apply a stripy background?
    QString m_stem1;  // stem 1 text
    QString m_stem2;  // stem 2 text

    QVector<QVector<QPointer<BooleanWidget>>> m_widgets1;  // widgets for 1
    QVector<QVector<QPointer<BooleanWidget>>> m_widgets2;  // widgets for 2
    QVector<QuMcqGridDoubleSignaller*> m_signallers;
        // ... objects to signal us when field data/mandatory status changes
};
