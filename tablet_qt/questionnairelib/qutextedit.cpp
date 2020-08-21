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

#include "qutextedit.h"
#include <QTimer>
#include "common/textconst.h"
#include "lib/timerfunc.h"
#include "lib/uifunc.h"
#include "qobjects/focuswatcher.h"
#include "questionnairelib/questionnaire.h"
#include "widgets/growingtextedit.h"
#include "widgets/growingplaintextedit.h"


const int WRITE_DELAY_MS = 400;


QuTextEdit::QuTextEdit(FieldRefPtr fieldref, const bool accept_rich_text) :
    m_fieldref(fieldref),
    m_accept_rich_text(accept_rich_text),
    m_allow_tabs_in_content(false),
    m_hint(TextConst::defaultHintText()),
#ifdef QUTEXTEDIT_USE_PLAIN_TEXT_EDITOR
    m_plain_editor(nullptr),
#endif
    m_rich_editor(nullptr),
    m_ignore_widget_signal(false),
    m_focus_watcher(nullptr)
{
    Q_ASSERT(m_fieldref);
    timerfunc::makeSingleShotTimer(m_timer);
    connect(m_timer.data(), &QTimer::timeout,
            this, &QuTextEdit::textChanged);
    connect(m_fieldref.data(), &FieldRef::valueChanged,
            this, &QuTextEdit::fieldValueChanged);
    connect(m_fieldref.data(), &FieldRef::mandatoryChanged,
            this, &QuTextEdit::fieldValueChanged);
}


QuTextEdit* QuTextEdit::setAllowTabsInContent(const bool allow_tabs_in_content)
{
    m_allow_tabs_in_content = allow_tabs_in_content;
    return this;
}


QuTextEdit* QuTextEdit::setHint(const QString& hint)
{
    m_hint = hint;
    return this;
}


void QuTextEdit::setFromField()
{
    fieldValueChanged(m_fieldref.data(), nullptr);
    // special; pretend "it didn't come from us" to disable the efficiency
    // check in fieldValueChanged
}


QPointer<QWidget> QuTextEdit::makeWidget(Questionnaire* questionnaire)
{
    const bool read_only = questionnaire->readOnly();
    // A bit ugly, but since QPlainTextEdit and QTextEdit share no useful
    // parent class...
#ifdef QUTEXTEDIT_USE_PLAIN_TEXT_EDITOR
    if (m_accept_rich_text) {
#endif
        m_rich_editor = new GrowingTextEdit();
        m_rich_editor->setEnabled(!read_only);
        m_rich_editor->setAcceptRichText(m_accept_rich_text);
        m_rich_editor->setPlaceholderText(m_hint);
        m_rich_editor->setTabChangesFocus(!m_allow_tabs_in_content);
        if (!read_only) {
            connect(m_rich_editor.data(), &GrowingTextEdit::textChanged,
                    this, &QuTextEdit::widgetTextChanged);
            // QTextEdit::textChanged - Called *whenever* contents changed.
            // http://doc.qt.io/qt-5.7/qtextedit.html#textChanged
            // Note: no data sent along with the signal

            m_focus_watcher = new FocusWatcher(m_rich_editor.data());
            connect(m_focus_watcher.data(), &FocusWatcher::focusChanged,
                    this, &QuTextEdit::widgetFocusChanged);
        }
        setFromField();
        return QPointer<QWidget>(m_rich_editor);

#ifdef QUTEXTEDIT_USE_PLAIN_TEXT_EDITOR
    } else {
        m_plain_editor = new GrowingPlainTextEdit();
        m_plain_editor->setEnabled(!read_only);
        m_plain_editor->setPlaceholderText(m_hint);
        m_plain_editor->setTabChangesFocus(!m_allow_tabs_in_content);
        if (!read_only) {
            connect(m_plain_editor.data(), &GrowingPlainTextEdit::textChanged,
                    this, &QuTextEdit::widgetTextChanged);
            // QPlainTextEdit::textChanged - Called *whenever* contents changed.
            // http://doc.qt.io/qt-5/qplaintextedit.html#textChanged

            m_focus_watcher = new FocusWatcher(m_plain_editor.data());
            connect(m_focus_watcher.data(), &FocusWatcher::focusChanged,
                    this, &QuTextEdit::widgetFocusChanged);
        }
        setFromField();
        return QPointer<QWidget>(m_plain_editor);
    }
#endif
}


FieldRefPtrList QuTextEdit::fieldrefs() const
{
    return FieldRefPtrList{m_fieldref};
}


void QuTextEdit::widgetTextChanged()
{
    if (m_ignore_widget_signal) {
        // Note: ignore it now, not after the timer! Otherwise impossible
        // (well, harder) to synchronize flag distinguishing "real" and
        // "internally generated" changes with events at the far end of the
        // timer.
        return;
    }
    m_timer->start(WRITE_DELAY_MS);  // will restart if already timing
    // ... goes to textChanged()
}


void QuTextEdit::textChanged()
{
#ifdef QUTEXTEDIT_USE_PLAIN_TEXT_EDITOR
    if (!m_plain_editor && !m_rich_editor) {
        return;
    }
#else
    if (!m_rich_editor) {
        return;
    }
#endif
    QString text;
    if (m_accept_rich_text) {
        text = m_rich_editor->toPlainText();
        if (!text.isEmpty()) {
            text = m_rich_editor->toHtml();
        }
        // ... That forces the text to empty (rather than a bunch of HTML
        // representing nothing) if there is no real text.
    } else {
#ifdef QUTEXTEDIT_USE_PLAIN_TEXT_EDITOR
        text = m_plain_editor->toPlainText();
#else
        text = m_rich_editor->toPlainText();
#endif
    }
    const bool changed = m_fieldref->setValue(text, this);  // Will trigger valueChanged
    if (changed) {
        emit elementValueChanged();
    }
}


void QuTextEdit::fieldValueChanged(const FieldRef* fieldref,
                                   const QObject* originator)
{
#ifdef QUTEXTEDIT_USE_PLAIN_TEXT_EDITOR
    QWidget* pwidget = m_accept_rich_text
            ? static_cast<QWidget*>(m_rich_editor.data())
            : static_cast<QWidget*>(m_plain_editor.data());
#else
    auto pwidget = static_cast<QWidget*>(m_rich_editor.data());
#endif
    if (!pwidget) {
        return;
    }
    uifunc::setPropertyMissing(pwidget, fieldref->missingInput());
    if (originator != this) {
        // In this case we don't want to block all signals, because the
        // GrowingPlainTextEdit/GrowingTextEdit widget needs internal signals.
        // However, we want to stop signal receipt by our own textChanged()
        // slot. So we can set a flag:
        m_ignore_widget_signal = true;
        if (m_accept_rich_text) {
            m_rich_editor->setHtml(fieldref->valueString());
        } else {
#ifdef QUTEXTEDIT_USE_PLAIN_TEXT_EDITOR
            m_plain_editor->setPlainText(fieldref->valueString());
#else
            m_rich_editor->setPlainText(fieldref->valueString());
#endif
        }
        m_ignore_widget_signal = false;
    }
}


void QuTextEdit::widgetFocusChanged(const bool in)
{
    // If focus is leaving the widget, save the field value.
#ifdef QUTEXTEDIT_USE_PLAIN_TEXT_EDITOR
    if (in || (!m_plain_editor && !m_rich_editor)) {
        return;
    }
#else
    if (in || !m_rich_editor) {
        return;
    }
#endif
    const bool change_pending = m_timer->isActive();
    m_timer->stop();  // just in case it's running
    if (change_pending) {
        textChanged();  // maybe
    }
}
