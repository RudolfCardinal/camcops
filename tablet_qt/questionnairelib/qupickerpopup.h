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

class ClickableLabelWordWrapWide;


class QuPickerPopup : public QuElement
{
    // Offers a pop-up dialogue of choices, or device equivalent.

    Q_OBJECT
public:

    // Constructor
    QuPickerPopup(FieldRefPtr fieldref, const NameValueOptions& options);

    // Set the title of the pop-up dialogue.
    QuPickerPopup* setPopupTitle(const QString& popup_title);

    // Shuffle the options (when making the widget)?
    QuPickerPopup* setRandomize(bool randomize);

protected:

    // Sets the widget state from our fieldref.
    void setFromField();

    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;
    virtual FieldRefPtrList fieldrefs() const override;

protected slots:

    // "An option has been clicked."
    void clicked();

    // "Field's data has changed."
    void fieldValueChanged(const FieldRef* fieldref);

protected:
    FieldRefPtr m_fieldref;  // our field
    NameValueOptions m_options;  // possible options
    QString m_popup_title;  // title for dialogue box
    bool m_randomize;  // shuffle?
    QPointer<ClickableLabelWordWrapWide> m_label;  // label to display choice
};
