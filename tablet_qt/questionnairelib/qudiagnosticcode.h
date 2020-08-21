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
#include <QPointer>
#include <QSharedPointer>
#include "db/fieldref.h"
#include "questionnairelib/quelement.h"

class DiagnosticCodeSet;
class LabelWordWrapWide;
class QLabel;
class QPushButton;
class QWidget;


class QuDiagnosticCode : public QuElement
{
    // Allows tree browsing and search of diagnostic codes from a structured
    // classification system.

    Q_OBJECT
public:

    // Construct with:
    // - a code set (e.g. "ICD-10");
    // - a fieldref for the code (e.g. "F20.0")
    // - a fieldref for the description (e.g. "Paranoid schizophrenia")
    QuDiagnosticCode(DiagnosticCodeSetPtr codeset,
                     FieldRefPtr fieldref_code,
                     FieldRefPtr fieldref_description);

    // Should we offer a button to set the code/description to null?
    QuDiagnosticCode* setOfferNullButton(bool offer_null_button);

protected:

    // Sets the widget state from our fieldrefs.
    virtual void setFromField();

    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;
    virtual FieldRefPtrList fieldrefs() const override;

protected slots:

    // User clicked the "set diagnosis" button.
    virtual void setButtonClicked();

    // User clicked the "set to null" button.
    virtual void nullButtonClicked();

    // Having clicked the "set diagnosis" button, we've popped up a widget to
    // choose a diagnosis; the user has chosen a diagnosis, and the popup
    // widget is telling us what it is.
    virtual void widgetChangedCode(const QString& code,
                                   const QString& description);

    // "Fieldref reports that the field's data has changed."
    virtual void fieldValueChanged(const FieldRef* fieldref_code);

protected:
    DiagnosticCodeSetPtr m_codeset;  // our code set
    FieldRefPtr m_fieldref_code;  // fieldref for the code
    FieldRefPtr m_fieldref_description;  // fieldref for the description
    bool m_offer_null_button;  // see setOfferNullButton()

    QPointer<Questionnaire> m_questionnaire;  // our questionnaire
    QPointer<QLabel> m_missing_indicator;  // indicator for "missing data"
    QPointer<LabelWordWrapWide> m_label_code;  // text of code
    QPointer<LabelWordWrapWide> m_label_description;  // text of description
};
