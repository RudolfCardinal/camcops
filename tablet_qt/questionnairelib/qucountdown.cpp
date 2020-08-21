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

// #define DEBUG_TICKS

#include "qucountdown.h"
#include <functional>
#include <QHBoxLayout>
#include <QLabel>
#include <QMediaPlayer>
#include <QPushButton>
#include <QTimer>
#include "common/cssconst.h"
#include "lib/soundfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/questionnaire.h"

const int PERIOD_MS = 100;  // should divide into whole seconds!
const int DP = 1;


QuCountdown::QuCountdown(const int time_s) :
    m_time_s(time_s),
    m_volume(uiconst::MAX_VOLUME_QT),
    m_running(false),
    m_timer(new QTimer())
{
    m_timer->setTimerType(Qt::PreciseTimer);  // ms accuracy
    connect(m_timer.data(), &QTimer::timeout,
            this, &QuCountdown::tick);
}


QuCountdown::~QuCountdown()
{
    soundfunc::finishMediaPlayer(m_player);
}


QuCountdown* QuCountdown::setVolume(const int volume)
{
    m_volume = qBound(uiconst::MIN_VOLUME_QT, volume, uiconst::MAX_VOLUME_QT);
    if (m_player) {
        m_player->setVolume(m_volume);
    }
    return this;
}


QPointer<QWidget> QuCountdown::makeWidget(Questionnaire* questionnaire)
{
    QPointer<QWidget> widget = new QWidget();
    widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    auto layout = new QHBoxLayout();
    layout->setContentsMargins(uiconst::NO_MARGINS);
    widget->setLayout(layout);

    const bool read_only = questionnaire->readOnly();

    m_start_button = new QPushButton(tr("Start"));
    m_start_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(m_start_button);

    m_stop_button = new QPushButton(tr("Stop"));
    m_stop_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(m_stop_button);

    m_reset_button = new QPushButton(tr("Reset"));
    m_reset_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(m_reset_button);

    layout->addStretch();

    m_label = new QLabel();
    m_label->setObjectName(cssconst::COUNTDOWN_LABEL);
    layout->addWidget(m_label);

    if (read_only) {
        m_start_button->setEnabled(false);
        m_stop_button->setEnabled(false);
        m_reset_button->setEnabled(false);
    } else {
        connect(m_start_button, &QPushButton::clicked,
                this, &QuCountdown::start);
        connect(m_stop_button, &QPushButton::clicked,
                this, &QuCountdown::stop);
        connect(m_reset_button, &QPushButton::clicked,
                this, &QuCountdown::reset);

        soundfunc::makeMediaPlayer(m_player);
        if (m_player) {
            m_player->setMedia(QUrl(uiconst::SOUND_COUNTDOWN_FINISHED));
        }
    }

    reset();

    return widget;
}


void QuCountdown::start()
{
    m_timer->start(PERIOD_MS);  // period in ms
    m_running = true;
    updateDisplay();
}


void QuCountdown::stop()
{
    m_timer->stop();
    m_running = false;
    updateDisplay();
}


void QuCountdown::reset()
{
    if (m_running) {
        stop();
    }
    m_seconds_left = m_time_s;
    updateDisplay();
}


void QuCountdown::tick()
{
    m_seconds_left -= PERIOD_MS / 1000.0;
    if (m_seconds_left <= 0) {
        // Finished!
#ifdef DEBUG_TICKS
        qDebug() << Q_FUNC_INFO << "- finished";
#endif
        bong();
        stop();  // will call updateDisplay()
    } else {
#ifdef DEBUG_TICKS
        qDebug() << Q_FUNC_INFO << "-" << m_seconds_left
                 << "seconds left";
#endif
        updateDisplay();
    }
}


void QuCountdown::bong()
{
    if (!m_player) {
        return;
    }
    m_player->play();
}


void QuCountdown::updateDisplay()
{
    if (!m_label) {
        return;
    }
    QString text;
    if (m_seconds_left < 0) {
        text = tr("FINISHED");
    } else {
        text = QString("%1 s").arg(QString::number(m_seconds_left, 'f', DP));
        if (!m_running) {
            text += tr(" (not running)");
        }
    }
    m_label->setText(text);
}
