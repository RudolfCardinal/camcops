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
#include "questionnairelib/quelement.h"

class FocusWatcher;
class GrowingPlainTextEdit;
class GrowingTextEdit;

// #define QUTEXTEDIT_USE_PLAIN_TEXT_EDITOR  // not resizing properly!


class QuTextEdit : public QuElement
{
    // Offers an expanding editor for entry of large quantities of text.
    // (For a smaller version, see QuLineEdit.)

    Q_OBJECT
public:

    // Constructor.
    // - accept_rich_text: see http://doc.qt.io/qt-5/qtextedit.html#acceptRichText-prop
    QuTextEdit(FieldRefPtr fieldref, bool accept_rich_text = false);

    // Allow tabs in content? Generally a bad idea as users may expect the Tab
    // key to navigate between fields.
    QuTextEdit* setAllowTabsInContent(bool allow_tabs_in_content);

    // Set hint, shown when the field is otherwise empty.
    QuTextEdit* setHint(const QString& hint);

protected:

    // Sets the widget state from our fieldref.
    void setFromField();

    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;
    virtual FieldRefPtrList fieldrefs() const override;

protected slots:
    // "The user has typed something into the widget."
    // Calls textChanged() after a delay for the benefit of fast typists.
    void widgetTextChanged();

    // "The text has changed (a short while ago)."
    void textChanged();

    // "The field's data has changed."
    void fieldValueChanged(const FieldRef* fieldref,
                           const QObject* originator = nullptr);

    // "The widget has gained or lost focus."
    void widgetFocusChanged(bool in);

protected:
    FieldRefPtr m_fieldref;  // our field
    bool m_accept_rich_text;  // accept rich text?
    bool m_allow_tabs_in_content;  // accept tabs as content?
    QString m_hint;
#ifdef QUTEXTEDIT_USE_PLAIN_TEXT_EDITOR
    QPointer<GrowingPlainTextEdit> m_plain_editor;  // editor widget
#endif
    QPointer<GrowingTextEdit> m_rich_editor;  // editor widget
    bool m_ignore_widget_signal;  // temporarily ignore signals from widget?
    QPointer<FocusWatcher> m_focus_watcher;  // allows us to detect focus change
    QSharedPointer<QTimer> m_timer;  // timer so we write only after a flurry of keypresses has stopped
};
