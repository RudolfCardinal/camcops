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
#include "questionnairelib/namevalueoptions.h"
#include "questionnairelib/quelement.h"

class QComboBox;
class QLabel;


class QuPickerInline : public QuElement
{
    // Offers a drop-down list of choices, or device equivalent.

    Q_OBJECT
public:

    // Constructor
    QuPickerInline(FieldRefPtr fieldref, const NameValueOptions& options);

    // Shuffle the options (when making the widget)?
    QuPickerInline* setRandomize(bool randomize);

protected:

    // Sets the widget state from our fieldref.
    void setFromField();

    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;
    virtual FieldRefPtrList fieldrefs() const override;

protected slots:

    // "Chosen item in the QComboBox has changed."
    void currentIndexChanged(int index);

    // "Field's data has changed."
    void fieldValueChanged(const FieldRef* fieldref);

protected:
    FieldRefPtr m_fieldref;  // our field
    NameValueOptions m_options;  // possible options
    bool m_randomize;  // shuffle?
    QPointer<QComboBox> m_cbox;  // combo box widget
};
