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
#include "questionnairelib/mcqgridsubtitle.h"
#include "questionnairelib/namevalueoptions.h"
#include "questionnairelib/quelement.h"
#include "questionnairelib/questionwithtwofields.h"

class BooleanWidget;
class QuMcqGridSingleBooleanSignaller;


class QuMcqGridSingleBoolean : public QuElement
{
    // Offers a grid of multiple-choice questions, each with a single boolean.
    // For example:
    //
    //      TITLE       MCQ                   BOOLEAN
    //      |
    //      v
    //      How much do you like it?
    // MCQ OPTIONS ---> Not at all ... Lots   Do you own one? <-- BOOLEAN TEXT
    //      1. Banana       O       O   O         X
    //      2. Diamond      O       O   O         .
    //      3. ...
    //      ^
    //      |
    //      QUESTIONS

    Q_OBJECT
    friend class QuMcqGridSingleBooleanSignaller;

public:
    // Constructor
    QuMcqGridSingleBoolean(const QVector<QuestionWithTwoFields>& questions_with_fields,
                           const NameValueOptions& mcq_options,
                           const QString& boolean_text);

    // Destructor
    virtual ~QuMcqGridSingleBoolean() override;

    // Boolean part on left, rather than right?
    QuMcqGridSingleBoolean* setBooleanLeft(bool boolean_left);

    // Set the relative widths of the columnss.
    QuMcqGridSingleBoolean* setWidth(int question_width,
                                     const QVector<int>& mcq_option_widths,
                                     int boolean_width);

    // Set the overall title.
    QuMcqGridSingleBoolean* setTitle(const QString& title);

    // Set subtitles. See McqGridSubtitle.
    QuMcqGridSingleBoolean* setSubtitles(const QVector<McqGridSubtitle>& subtitles);

    // Ask widgets to expand horizontally?
    QuMcqGridSingleBoolean* setExpand(bool expand);

    // Apply a stripy background to the grid?
    QuMcqGridSingleBoolean* setStripy(bool stripy);

protected:
    // Set the widget state from the fields' data.
    void setFromFields();

    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;
    virtual FieldRefPtrList fieldrefs() const override;

    // Return the column number for the specified (zero-based) MCQ option/value.
    int mcqColnum(int value_index) const;

    // Return the column number for the boolean column.
    int booleanColnum() const;

    // Return the column index of the first or the second spacer.
    int spacercol(bool first) const;

    // Internal function to add options to a grid.
    void addOptions(GridLayout* grid, int row);

protected slots:
    // "An MCQ response widget has been clicked."
    void mcqClicked(int question_index, int value_index);

    // "A Boolean response widget has been clicked."
    void booleanClicked(int question_index);

    // "An MCQ field's value, or mandatory status, has changed."
    void mcqFieldValueOrMandatoryChanged(int question_index,
                                         const FieldRef* fieldref);

    // "A Boolean field's value, or mandatory status, has changed."
    void booleanFieldValueOrMandatoryChanged(int question_index,
                                             const FieldRef* fieldref);
protected:
    bool m_boolean_left;  // Boolean part on left, not right?
    QVector<QuestionWithTwoFields> m_questions_with_fields;  // Question/field map
    NameValueOptions m_mcq_options;  // Name/value options for the MCQ part
    QString m_boolean_text;  // Text to display for Boolean column
    int m_question_width;  // Relative width of question column
    QVector<int> m_mcq_option_widths;  // Relative widths of MCQ columns
    int m_boolean_width;  // Relative width of Boolean column
    QString m_title;  // Overall title
    QVector<McqGridSubtitle> m_subtitles;  // Subtitle info
    bool m_expand;  // expand our widgets horizontally?
    bool m_stripy;  // apply a stripy background?

    QVector<QVector<QPointer<BooleanWidget>>> m_mcq_widgets;  // MCQ widgets
    QVector<QPointer<BooleanWidget>> m_boolean_widgets;  // Boolean widgets
    QVector<QuMcqGridSingleBooleanSignaller*> m_signallers;
    // ... objects to signal us when field data/mandatory status changes
};
