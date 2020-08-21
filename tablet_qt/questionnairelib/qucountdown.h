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
#include "questionnairelib/quelement.h"

class QLabel;
class QMediaPlayer;
class QPushButton;
class QTimer;


class QuCountdown : public QuElement
{
    // Offers a countdown timer (which plays a sound on timeout), e.g. for
    // allowing the respondent a certain amount of time for a task.
    // Offers start/stop/reset controls.

    Q_OBJECT
public:
    // Construct with the timer's duration.
    QuCountdown(int time_s);

    // Destructor.
    virtual ~QuCountdown() override;

    // Sets the timeout alarm volume; range [0, 100].
    QuCountdown* setVolume(int volume);

protected:
    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;

    // Update the textual display to show time left, or "FINISHED", etc.
    void updateDisplay();

    // Play a sound (on timeout).
    void bong();

protected slots:

    // "Start the timer."
    void start();

    // "Stop the timer."
    void stop();

    // "Reset the timer to its starting value."
    void reset();

    // "Some time has elapsed."
    void tick();

protected:
    int m_time_s;  // total time
    int m_volume;  // alarm volume
    bool m_running;  // currently running?
    QPointer<QPushButton> m_start_button;  // "Start"
    QPointer<QPushButton> m_stop_button;  // "Stop"
    QPointer<QPushButton> m_reset_button;  // "Reset"
    QPointer<QLabel> m_label;  // text containing time-to-go information
    QSharedPointer<QTimer> m_timer;  // timer
    QSharedPointer<QMediaPlayer> m_player;  // sound player; not owned by other widgets
    double m_seconds_left;  // time left
};
