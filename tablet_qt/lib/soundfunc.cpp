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

#include "soundfunc.h"
#include <QDebug>
#include <QObject>
#include <QMediaPlayer>
#include "common/textconst.h"
#include "lib/uifunc.h"
#include "maths/mathfunc.h"

namespace soundfunc {

const QString UNABLE_TO_CREATE_MEDIA_PLAYER("Unable");


void makeMediaPlayer(QSharedPointer<QMediaPlayer>& player)
{
    // qDebug() << "Default QMediaPlayer() flags: " << static_cast<int>(QMediaPlayer::Flags());
    // default "flags" argument to QMediaPlayer() is 0, i.e. no flags set.

    qDebug() << "About to call QMediaPlayer()...";
    player = QSharedPointer<QMediaPlayer>(new QMediaPlayer(),
                                          &QObject::deleteLater);
    // http://doc.qt.io/qt-5/qsharedpointer.html
    // Failing to use deleteLater() can cause crashes, as there may be
    // outstanding events relating to this object.
    // ... but it's not enough; see finishMediaPlayer().
    qDebug() << "... QMediaPlayer() has returned.";

    /*

    Near-crash; very long pause 2019-09-06. Looks like it relates to GStreamer:

        1   ??                                                                                                                                                                                                                                                   0x7ffff7bcefd7
        2   ??                                                                                                                                                                                                                                                   0x7ffff7bcfba8
        3   ??                                                                                                                                                                                                                                                   0x7ffff7bcfbda
        4   g_type_add_interface_static                                                                                                                                                                                                                          0x7ffff7bd10d6
        5   gst_bin_get_type                                                                                                                                                                                                                                     0x7ffff6d009b0
        6   ??                                                                                                                                                                                                                                                   0x7ffff6cf74fc
        7   g_option_context_parse                                                                                                                                                                                                                               0x7ffff13a58a8
        8   gst_init_check                                                                                                                                                                                                                                       0x7ffff6cf84cf
        9   gst_init                                                                                                                                                                                                                                             0x7ffff6cf8524
        10  QGstreamerPlayerServicePlugin::create(QString const&)                                                                                                                                                                                                0x555555f98cd2
        11  QPluginServiceProvider::requestService(QByteArray const&, QMediaServiceProviderHint const&)                                                                                                                                                          0x555556581e9e
        12  QMediaPlayer::QMediaPlayer(QObject *, QFlags<QMediaPlayer::Flag>)                                                                                                                                                                                    0x5555565aed69
        13  soundfunc::makeMediaPlayer                                                                                                                                                                                      soundfunc.cpp                    31  0x5555558a5214
        14  CardinalExpectationDetection::startTask                                                                                                                                                                         cardinalexpectationdetection.cpp 770 0x555555adbc7b
        ... <More>

    or

        1  sched_yield                                                                                                                                                                                                     syscall-template.S               78  0x7ffff0613e57
        2  ??                                                                                                                                                                                                                                                   0x7fffc78841b9
        3  ??                                                                                                                                                                                                                                                   0x7fffc78708e0
        4  ??                                                                                                                                                                                                                                                   0x7fffc75db94f
        5  ??                                                                                                                                                                                                                                                   0x7fffc75d0f00
        6  vaTerminate                                                                                                                                                                                                                                          0x7fffccb2f542
        7  ??                                                                                                                                                                                                                                                   0x7fffcd7ec3ad
        8  g_object_unref                                                                                                                                                                                                                                       0x7ffff6a8a012
        9  gst_object_replace                                                                                                                                                                                                                                   0x7ffff6cf9a10
        10 ??                                                                                                                                                                                                                                                   0x7fffcd7c17a3
        11 ??                                                                                                                                                                                                                                                   0x7fffcd7cc16b
        12 ??                                                                                                                                                                                                                                                   0x7ffff7269f11
        13 gst_element_change_state                                                                                                                                                                                                                             0x7ffff6d26d5e
        14 ??                                                                                                                                                                                                                                                   0x7ffff6d27499
        15 QGstreamerVideoOverlay::QGstreamerVideoOverlay(QObject *, QByteArray const&)                                                                                                                                                                         0x555555faef15
        16 QGstreamerVideoWidgetControl::QGstreamerVideoWidgetControl(QObject *, QByteArray const&)                                                                                                                                                             0x555555fb9c55
        17 QGstreamerPlayerService::QGstreamerPlayerService(QObject *)                                                                                                                                                                                          0x555555f991c5
        18 QGstreamerPlayerServicePlugin::create(QString const&)                                                                                                                                                                                                0x555555f98d01
        19 QPluginServiceProvider::requestService(QByteArray const&, QMediaServiceProviderHint const&)                                                                                                                                                          0x55555658206e
        20 QMediaPlayer::QMediaPlayer(QObject *, QFlags<QMediaPlayer::Flag>)                                                                                                                                                                                    0x5555565aef39
        21 soundfunc::makeMediaPlayer                                                                                                                                                                                      soundfunc.cpp                    31  0x5555558a5199
        22 CardinalExpectationDetection::startTask                                                                                                                                                                         cardinalexpectationdetection.cpp 760 0x555555adb996
        ... <More>

    This is potentially related to
    https://github.com/OpenBoard-org/OpenBoard/issues/4. On shrike:

        $ gst-inspect-1.0 --version

        gst-inspect-1.0 version 1.14.5
        GStreamer 1.14.5
        https://launchpad.net/distros/ubuntu/+source/gstreamer1.0

    */

    if (!player) {
        uifunc::alert(TextConst::unableToCreateMediaPlayer());
    }
}


void finishMediaPlayer(const QSharedPointer<QMediaPlayer>& player)
{
    // The following seems to prevent a crash (even with deleteLater set up by
    // the QSharedPointer<QMediaPlayer>(new ...) call) whereby ongoing events
    // try to go to a non-existing QMediaPlayer.
    // Looks like this is not an uncommon problem:
    //  https://www.google.com/?ion=1&espv=2#q=delete%20qmediaplayer%20crash
    // The crash comes from QMetaObject, from the Qt event loop.

    if (player) {
        player->stop();
    }
}


void setVolume(const QSharedPointer<QMediaPlayer>& player,
               const int volume_percent)
{
    player->setVolume(volume_percent);
}


void setVolume(const QSharedPointer<QMediaPlayer>& player,
               const double volume_proportion)
{
    player->setVolume(mathfunc::proportionToIntPercent(volume_proportion));
}


}  // namespace soundfunc
