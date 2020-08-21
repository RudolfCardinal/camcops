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
#include <QSharedPointer>
#include <QVariant>
#include "db/fieldref.h"
#include "questionnairelib/quelement.h"
#include "questionnairelib/questionwithonefield.h"

class BooleanWidget;
class LabelWordWrapWide;
class QSignalMapper;


class QuMultipleResponse : public QuElement
{
    // Offers an n-from-many question. For example:
    //
    //      Which are your TWO favourites, from the list:
    //
    //      [X] Banana
    //      [ ] Diamond
    //      [ ] Apple
    //      [X] Bapple
    //      [ ] Gru
    //
    // or horizontally:
    //
    //      Choose 2:
    //
    //      [X] Banana  [ ] Diamond  [ ] Apple  [X] Bapple  [ ] Gru
    //
    // or in text button style:
    //
    //      +--------+
    //      | Banana |
    //      +--------+
    //      +---------+
    //      | Diamond |
    //      +---------+
    //      +-------+
    //      | Apple |
    //      +-------+
    //      +--------+
    //      | Bapple |
    //      +--------+
    //      +-----+
    //      | Gru |
    //      +-----+
    //
    // or with horizontal text buttons:
    //
    //      +--------+ +---------+ +-------+ +--------+ +-----+
    //      | Banana | | Diamond | | Apple | | Bapple | | Gru |
    //      +--------+ +---------+ +-------+ +--------+ +-----+

    Q_OBJECT
public:

    // Construct in the empty state.
    QuMultipleResponse();

    // Construct from a list of questions/fields.
    QuMultipleResponse(const QVector<QuestionWithOneField>& items);
    QuMultipleResponse(std::initializer_list<QuestionWithOneField> items);

    // Add an item.
    QuMultipleResponse* addItem(const QuestionWithOneField& item);

    // Shuffle the options (when making the widget)?
    QuMultipleResponse* setRandomize(bool randomize);

    // Show the instruction?
    QuMultipleResponse* setShowInstruction(bool show_instruction);

    // Set the instruction; if not set, defaultInstruction() is used.
    QuMultipleResponse* setInstruction(const QString& instruction);

    // Display in horizontal format?
    QuMultipleResponse* setHorizontal(bool horizontal);

    // Display in text button format?
    QuMultipleResponse* setAsTextButton(bool as_text_button);

    // Show text in bold?
    QuMultipleResponse* setBold(bool bold);

public slots:

    // Set the minimum number of answers; negative for "not specified".
    QuMultipleResponse* setMinimumAnswers(int minimum_answers);

    // Set the maximum number of answers; negative for "not specified".
    QuMultipleResponse* setMaximumAnswers(int maximum_answers);

protected:
    // Set widget state from field data.
    void setFromFields();

    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;
    virtual FieldRefPtrList fieldrefs() const override;

    // Return the minimum number of answers.
    int minimumAnswers() const;

    // Return the maximum number of answers.
    int maximumAnswers() const;

    // Return the number of answers currently set to true.
    int nTrueAnswers() const;

    // Return a default instruction based on the minimum/maximum number of
    // answers.
    QString defaultInstruction() const;

    // Is this a valid zero-based question index?
    bool validIndex(int index);

    virtual bool missingInput() const override;

    // Update the widget to reflect a change in the min/max number of answers.
    void minOrMaxChanged();

protected slots:
    // "A response widget has been clicked."
    void clicked(int index);

    // "A field's data has changed."
    void fieldValueChanged();

protected:
    QVector<QuestionWithOneField> m_items;  // question/field mapping
    int m_minimum_answers;  // negative for "not specified"
    int m_maximum_answers;  // negative for "not specified"
    bool m_randomize;  // shuffle the options?
    bool m_show_instruction;  // show the instruction?
    QString m_instruction;  // instruction text (otherwise default is used)
    bool m_horizontal;  // horizontal layout?
    bool m_as_text_button;  // text button style?
    bool m_bold;  // bold text?

    QVector<QPointer<BooleanWidget>> m_widgets;  // our response widgets
    LabelWordWrapWide* m_instruction_label;  // our instruction label
};
