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
#include "core/camcopsapp.h"
#include "questionnairelib/qupage.h"
#include "widgets/basewidget.h"

class ImageButton;
class QAbstractButton;
class QLabel;
class QPushButton;


class QuestionnaireHeader : public BaseWidget
{
    // Provides a questionnaire's title and its control buttons (e.g. page
    // movement, cancellation).

    Q_OBJECT
public:
    // Construct, deciding which buttons to offer, etc.
    QuestionnaireHeader(
            QWidget* parent, const QString& title,
            bool read_only, bool offer_page_jump, bool within_chain,
            const QString& css_name, bool debug_allowed = false);

    // Decide whether we offer previous/next/finish buttons.
    // ("Next" and "Finish" should not be simultaneously shown!)
    void setButtons(bool previous, bool next, bool finish);

    // Sets the icon to display for the finish button (e.g. tick for
    // config-edit-settings questionnaires or stop  symbol for task
    // questionnaires).
    void setFinishButtonIcon(const QString& base_filename);

signals:
    // "User has clicked 'cancel'."
    void cancelClicked();

    // "User has clicked 'jump to page'."
    void jumpClicked();

    // "User has clicked 'previous page'."
    void previousClicked();

    // "User has clicked 'next page'."
    void nextClicked();

    // "User has clicked 'finish'."
    void finishClicked();

    // Send layout to debugging stream.
    void debugLayout();

protected:
    QString m_title;  // title text
    QPointer<QPushButton> m_button_debug;  // button for "debug layout"
    QPointer<QAbstractButton> m_button_jump;  // "jump"
    QPointer<QAbstractButton> m_button_previous;  // "previous page"
    QPointer<QAbstractButton> m_button_next;  // "next page"
    QPointer<ImageButton> m_button_finish;  // "finish"
    QPointer<QLabel> m_icon_no_next;  // icon to show when next unavailable, e.g. warning triangle
};
