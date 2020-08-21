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
#include <QLineEdit>  // for EchoMode
#include "db/fieldref.h"
#include "questionnairelib/quelement.h"

class FocusWatcher;
class QLineEdit;


class QuLineEdit : public QuElement
{
    // Offers a one-line text editor, for a string.
    // (For a bigger version, see QuTextEdit.)

    Q_OBJECT
public:
    // Constructor
    QuLineEdit(FieldRefPtr fieldref);

    // Sets the hint text (what's shown, greyed out, in the editor when the
    // line editor has no user-entered text in it).
    QuLineEdit* setHint(const QString& hint);

    // Sets the echo mode; e.g. show passwords as blobs.
    QuLineEdit* setEchoMode(QLineEdit::EchoMode echo_mode);

protected:
    // "Update our contents from the data in our field."
    virtual void setFromField();

    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;
    virtual FieldRefPtrList fieldrefs() const override;

    // Called from makeWidget(). Does nothing here; override to specialize.
    virtual void extraLineEditCreation(QLineEdit* editor);

protected slots:
    // "A key has been pressed."
    // Initiates delay (to prevent rapid typists from getting cross); then
    // calls widgetTextChangedMaybeValid().
    virtual void keystroke();

    // "A key was pressed a short while ago."
    // If a validator is in use, checks for validity.
    // If valid, calls widgetTextChangedAndValid(),
    virtual void widgetTextChangedMaybeValid();

    // Writes new data to our field.
    virtual void widgetTextChangedAndValid();

    // "The field's data has changed."
    virtual void fieldValueChanged(const FieldRef* fieldref,
                                   const QObject* originator = nullptr);

    // "The widget has gained or lost focus."
    virtual void widgetFocusChanged(bool in);

protected:
    FieldRefPtr m_fieldref;  // our field
    QString m_hint;  // hint text
    QPointer<QLineEdit> m_editor;  // our editor widget
    QPointer<FocusWatcher> m_focus_watcher;  // used to detect focus change
    QSharedPointer<QTimer> m_timer;  // used for typing delay, as above
    QLineEdit::EchoMode m_echo_mode;  // echo mode, as above
};
