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
#include "questionnairelib/questionwithonefield.h"

class BooleanWidget;
class QuMcqGridSignaller;


class QuMcqGrid : public QuElement
{
    // Offers a grid of multiple-choice questions, where several questions
    // share the same possible responses. For example:
    //
    //      How much do you like it?                     <- TITLE
    //                   Not at all  A bit  Lots         <- OPTIONS
    //      Fruit                                        <- SUBTITLE
    //      1. Banana        O         O     O
    //      Jewels                                       <- SUBTITLE
    //      2. Diamond       O         O     O
    //      3. Ruby          O         O     O
    //
    //      ^
    //      |
    //      QUESTIONS


    Q_OBJECT
    friend class QuMcqGridSignaller;

public:
    // Constructor
    QuMcqGrid(const QVector<QuestionWithOneField>& question_field_pairs,
              const NameValueOptions& options);

    // Destructor
    virtual ~QuMcqGrid() override;

    // Set widths:
    // - question_width: relative width of question column
    // - option_widths: relative widths of option columns
    QuMcqGrid* setWidth(int question_width, const QVector<int>& option_widths);

    // Sets the title
    QuMcqGrid* setTitle(const QString& title);

    // Sets the subtitles.
    // - You can have multiple subtitle rows.
    // - The "options" display may be repeated on subtitle rows. See
    //   McqGridSubtitle.
    QuMcqGrid* setSubtitles(const QVector<McqGridSubtitle>& subtitles);

    // Ask widgets to expand horizontally?
    QuMcqGrid* setExpand(bool expand);

    // Apply a stripy background to the grid?
    QuMcqGrid* setStripy(bool stripy);

    // Show the title (as the first row)? Default is true.
    QuMcqGrid* showTitle(bool show_title);

    // Show the questions in bold? Default is true.
    QuMcqGrid* setQuestionsBold(bool bold);

    // Without changing the displayed options, sets alternative hidden
    // name/value options for specific questions. Typically used for questions
    // that appear the same (e.g. Always - Sometimes - Never) but are sometimes
    // scored ascending and sometimes scored descending.
    // The NameValueOptions passed must have the same length as the main one
    // passed to the constructor.
    QuMcqGrid* setAlternateNameValueOptions(
            const QVector<int>& question_indexes,
            const NameValueOptions& options);

protected:
    // Set the widget state from the fields' data.
    void setFromFields();

    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;
    virtual FieldRefPtrList fieldrefs() const override;

    // Returns the column number containing the specified (zero-based)
    // option/value index.
    int colnum(int value_index) const;

    // Internal function to add options to a grid.
    void addOptions(GridLayout* grid, int row);

protected slots:
    // "One of the response widgets was clicked/touched."
    void clicked(int question_index, int value_index);

    // "A field's value, or a field's mandatory status, has changed."
    void fieldValueOrMandatoryChanged(int question_index,
                                      const FieldRef* fieldref);

protected:
    QVector<QuestionWithOneField> m_question_field_pairs;  // Question/field mapping
    NameValueOptions m_options;  // Name/value pairs for options
    int m_question_width;  // relative width for question column
    QVector<int> m_option_widths;  // relative widths for option columns
    QString m_title;  // title text
    QVector<McqGridSubtitle> m_subtitles;  // subtitle info
    bool m_expand;  // expand our widgets horizontally?
    bool m_stripy;  // apply a stripy background?
    bool m_show_title;  // show the title?
    bool m_questions_bold;  // show questions in bold?
    QVector<QVector<QPointer<BooleanWidget>>> m_widgets;  // our response widgets
    QVector<QuMcqGridSignaller*> m_signallers;
        // ... objects to signal us when field data/mandatory status changes
    QMap<int, NameValueOptions> m_alternate_options;
};
