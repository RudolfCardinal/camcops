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

#include "quaudioplayer.h"
#include <QAbstractButton>
#include <QDial>
#include <QUrl>
#include <QHBoxLayout>
#include <QWidget>
#include "common/textconst.h"
#include "common/uiconst.h"
#include "lib/soundfunc.h"
#include "lib/uifunc.h"
#include "widgets/imagebutton.h"


QuAudioPlayer::QuAudioPlayer(const QString& url) :
    m_url(url),
    m_volume(50),
    m_offer_volume_control(false),
    m_button_speaker(nullptr),
    m_button_speaker_playing(nullptr),
    m_player(nullptr),
    m_playing(false)
{
    // qDebug() << "QuAudioPlayer::QuAudioPlayer()";
}


QuAudioPlayer::~QuAudioPlayer()
{
    // qDebug() << "QuAudioPlayer::~QuAudioPlayer()";
    soundfunc::finishMediaPlayer(m_player);
}


QuAudioPlayer* QuAudioPlayer::setVolume(const int volume)
{
    // qDebug().nospace() << "QuAudioPlayer::setVolume(" << volume << ")";
    m_volume = qBound(uiconst::MIN_VOLUME_QT, volume, uiconst::MAX_VOLUME_QT);
    if (m_player) {
        m_player->setVolume(m_volume);
    }
    return this;
}


QuAudioPlayer* QuAudioPlayer::setOfferVolumeControl(
        const bool offer_volume_control)
{
    m_offer_volume_control = offer_volume_control;
    return this;
}


QPointer<QWidget> QuAudioPlayer::makeWidget(Questionnaire* questionnaire)
{
    Q_UNUSED(questionnaire)

    // Parentheses with new?
    // http://stackoverflow.com/questions/620137/do-the-parentheses-after-the-type-name-make-a-difference-with-new
    QPointer<QWidget> widget = new QWidget();
    widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    auto layout = new QHBoxLayout();
    layout->setContentsMargins(uiconst::NO_MARGINS);
    widget->setLayout(layout);

    m_button_speaker = new ImageButton(uiconst::CBS_SPEAKER);
    m_button_speaker_playing = new ImageButton(uiconst::CBS_SPEAKER_PLAYING);
    layout->addWidget(m_button_speaker);
    layout->addWidget(m_button_speaker_playing);
    connect(m_button_speaker, &QAbstractButton::clicked,
            this, &QuAudioPlayer::play);
    connect(m_button_speaker_playing, &QAbstractButton::clicked,
            this, &QuAudioPlayer::stop);
    m_button_speaker->show();
    m_button_speaker_playing->hide();

    if (m_offer_volume_control) {
        auto dial = new QDial();
        dial->setFixedSize(uiconst::g_dial_diameter_px,
                           uiconst::g_dial_diameter_px);
        dial->setNotchesVisible(true);
        dial->setRange(uiconst::MIN_VOLUME_QT, uiconst::MAX_VOLUME_QT);
        dial->setValue(m_volume);
        connect(dial, &QDial::valueChanged,
                this, &QuAudioPlayer::setVolumeNoReturn);
        layout->addWidget(dial);
    }

    layout->addStretch();

    soundfunc::makeMediaPlayer(m_player);
    if (m_player) {
        m_player->setMedia(QUrl(m_url));
        m_player->setVolume(m_volume);
        connect(m_player.data(), &QMediaPlayer::mediaStatusChanged,
                this, &QuAudioPlayer::mediaStatusChanged);
    }

    return widget;
}


void QuAudioPlayer::play()
{
    if (!m_player || m_playing) {
        return;
    }
    qDebug().nospace() << "Playing: " << m_url
                       << " (volume " << m_volume << ")";
    m_player->play();
    m_button_speaker->hide();
    m_button_speaker_playing->show();
    m_playing = true;
}


void QuAudioPlayer::stop()
{
    if (!m_player || !m_playing) {
        return;
    }
    qDebug() << "Stopping:" << m_url;
    m_player->stop();
    m_button_speaker->show();
    m_button_speaker_playing->hide();
    m_playing = false;
}


void QuAudioPlayer::mediaStatusChanged(const QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        qDebug() << "Playback finished for:" << m_url;
        stop();
    }
}


void QuAudioPlayer::setVolumeNoReturn(const int volume)
{
    if (!m_player) {
        return;
    }
    setVolume(volume);
    // but don't return anything, because that makes the media player stop!
}


void QuAudioPlayer::closing()
{
    if (!m_player) {
        return;
    }
    stop();
}
