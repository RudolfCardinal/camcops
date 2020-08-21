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
#include <QMediaPlayer>
#include "questionnairelib/quelement.h"

class QAbstractButton;
class QWidget;


class QuAudioPlayer : public QuElement
{
    // Questionnaire element to play audio.
    // Offers a play/stop button, +/- a volume control.

    Q_OBJECT
public:

    // Construct with a URL (e.g. a Qt resource URL for an audio file, such as
    // "qrc:///resources/camcops/sounds/bach_brandenburg_3_3.mp3").
    QuAudioPlayer(const QString& url);

    // Destructor
    virtual ~QuAudioPlayer() override;

    // Sets the volume. Use the range [0, 100]; the input will be bounded to
    // this.
    QuAudioPlayer* setVolume(int volume);

    // Should the widget display a volume control?
    QuAudioPlayer* setOfferVolumeControl(bool offer_volume_control = true);

protected:
    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;
    virtual void closing() override;

protected slots:
    // "Start playing."
    void play();

    // "Stop playing."
    void stop();

    // Incoming signals may include "playback finished".
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);

public slots:
    // "Set the volume, via a signal/slot."
    void setVolumeNoReturn(int volume);

protected:
    QString m_url;  // URL of sound resource
    int m_volume;  // range [0, 100]
    bool m_offer_volume_control;  // offer a volume control?
    QPointer<QAbstractButton> m_button_speaker;  // button shown when not playing
    QPointer<QAbstractButton> m_button_speaker_playing;  // button shown when playing
    QSharedPointer<QMediaPlayer> m_player;  // not owned by other widgets
    bool m_playing;  // currently playing?
};
