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

#include "qulineedit.h"
#include <QTimer>
#include <QValidator>
#include "common/textconst.h"
#include "lib/timerfunc.h"
#include "lib/uifunc.h"
#include "qobjects/focuswatcher.h"
#include "questionnairelib/questionnaire.h"


const int WRITE_DELAY_MS = 400;


QuLineEdit::QuLineEdit(FieldRefPtr fieldref) :
    m_fieldref(fieldref),
    m_hint(TextConst::defaultHintText()),
    m_editor(nullptr),
    m_focus_watcher(nullptr),
    m_echo_mode(QLineEdit::Normal)
{
    Q_ASSERT(m_fieldref);
    timerfunc::makeSingleShotTimer(m_timer);
    connect(m_timer.data(), &QTimer::timeout,
            this, &QuLineEdit::widgetTextChangedMaybeValid);
    connect(m_fieldref.data(), &FieldRef::valueChanged,
            this, &QuLineEdit::fieldValueChanged);
    connect(m_fieldref.data(), &FieldRef::mandatoryChanged,
            this, &QuLineEdit::fieldValueChanged);
}


QuLineEdit* QuLineEdit::setHint(const QString& hint)
{
    m_hint = hint;
    return this;
}


QuLineEdit* QuLineEdit::setEchoMode(const QLineEdit::EchoMode echo_mode)
{
    m_echo_mode = echo_mode;
    return this;
}


void QuLineEdit::setFromField()
{
    fieldValueChanged(m_fieldref.data(), nullptr);
    // special; pretend "it didn't come from us" to disable the efficiency
    // check in fieldValueChanged
}


QPointer<QWidget> QuLineEdit::makeWidget(Questionnaire* questionnaire)
{
    const bool read_only = questionnaire->readOnly();
    m_editor = new QLineEdit();
    m_editor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_editor->setEnabled(!read_only);
    m_editor->setPlaceholderText(m_hint);
    m_editor->setEchoMode(m_echo_mode);
    extraLineEditCreation(m_editor.data());  // allow subclasses to modify
    if (!read_only) {
        connect(m_editor.data(), &QLineEdit::textChanged,
                this, &QuLineEdit::keystroke);
        connect(m_editor.data(), &QLineEdit::editingFinished,
                this, &QuLineEdit::widgetTextChangedAndValid);
        // QLineEdit::textChanged: emitted whenever text changed.
        // QLineEdit::textEdited: NOT emitted when the widget's value is set
        //      programmatically.
        // QLineEdit::editingFinished: emitted when Return/Enter is pressed,
        //      or the editor loses focus. In the former case, only fires if
        //      validation is passed.

        // So, if we lose focus without validation, how are we going to revert
        // to something sensible?
        m_focus_watcher = new FocusWatcher(m_editor.data());
        connect(m_focus_watcher.data(), &FocusWatcher::focusChanged,
                this, &QuLineEdit::widgetFocusChanged);
    }
    setFromField();
    return QPointer<QWidget>(m_editor);
}


void QuLineEdit::extraLineEditCreation(QLineEdit* editor)
{
    Q_UNUSED(editor)
}


FieldRefPtrList QuLineEdit::fieldrefs() const
{
    return FieldRefPtrList{m_fieldref};
}


void QuLineEdit::keystroke()
{
    m_timer->start(WRITE_DELAY_MS);  // will restart if already timing
    // ... goes to widgetTextChangedMaybeValid()
}


void QuLineEdit::widgetTextChangedMaybeValid()
{
    if (!m_editor) {
        return;
    }
    const QValidator* validator = m_editor->validator();
    if (validator) {
        int pos = 0;
        QString text = m_editor->text();
        if (validator->validate(text, pos) != QValidator::Acceptable) {
            // duff
            return;
        }
    }
    widgetTextChangedAndValid();
}


void QuLineEdit::widgetTextChangedAndValid()
{
    if (!m_editor) {
        return;
    }
    m_timer->stop();  // just in case it's running
    // To cope with setting things to null, we need to use a QVariant.
    // We use null rather than a blank string, because QuLineEdit may be used
    // to set numeric fields (where "" will be converted to 0).
    const QString text = m_editor->text();
    const QVariant value = text.isEmpty() ? QVariant() : QVariant(text);
    const bool changed = m_fieldref->setValue(value, this);  // Will trigger valueChanged
    if (changed) {
        emit elementValueChanged();
    }
}


void QuLineEdit::fieldValueChanged(const FieldRef* fieldref,
                                   const QObject* originator)
{
    if (!m_editor) {
        return;
    }
    uifunc::setPropertyMissing(m_editor, fieldref->missingInput());
    if (originator != this) {
        // Now we're detecting textChanged, we have to block signals for this:
        const QSignalBlocker blocker(m_editor);
        const QString text = fieldref->isNull() ? "" : fieldref->valueString();
        // qDebug() << Q_FUNC_INFO << "setting to" << text;
        m_editor->setText(text);
    }
}


void QuLineEdit::widgetFocusChanged(const bool in)
{
    // If focus is leaving the widget, and its state is duff, restore from the
    // field value.
    if (in || !m_editor) {
        return;
    }
    m_timer->stop();  // just in case it's running
    const QValidator* validator = m_editor->validator();
    if (!validator) {
        return;
    }
    QString text = m_editor->text();
    int pos = 0;
    if (validator->validate(text, pos) != QValidator::Acceptable) {
        // Something duff
        setFromField();
    }
}
