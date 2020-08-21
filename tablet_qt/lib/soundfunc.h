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
#include <QSharedPointer>
class QMediaPlayer;


namespace soundfunc {

extern const QString UNABLE_TO_CREATE_MEDIA_PLAYER;

// Creates a media player that will be deleted later via QObject::deleteLater().
void makeMediaPlayer(QSharedPointer<QMediaPlayer>& player);

// Ensure the media player is stopped. See code for rationale.
void finishMediaPlayer(const QSharedPointer<QMediaPlayer>& player);

// Sets the volume of a media player, using a scale of 0-100.
void setVolume(const QSharedPointer<QMediaPlayer>& player, int volume_percent);

// Sets the volume of a media player, using a scale of 0-1.
void setVolume(const QSharedPointer<QMediaPlayer>& player, double volume_proportion);


}  // namespace soundfunc
