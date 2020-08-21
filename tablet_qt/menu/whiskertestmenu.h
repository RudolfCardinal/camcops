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
#include <QTextStream>
#include "menulib/menuwindow.h"
class LogBox;
class WhiskerInboundMessage;
class WhiskerManager;


class WhiskerTestMenu : public MenuWindow
{
    Q_OBJECT
public:
    WhiskerTestMenu(CamcopsApp& app);
    virtual QString title() const override;
protected:
    virtual void makeItems() override;
    OpenableWidget* configureWhisker(CamcopsApp& app);
    void connectWhisker();
    void disconnectWhisker();
    void testWhiskerNetworkLatency();
    void runDemoWhiskerTask();
protected slots:
    void demoWhiskerTaskMain();
    void eventReceived(const WhiskerInboundMessage& msg);
    void keyEventReceived(const WhiskerInboundMessage& msg);
    void clientMessageReceived(const WhiskerInboundMessage& msg);
    void otherMessageReceived(const WhiskerInboundMessage& msg);
    void taskCancelled();
protected:
    class StatusStream : public QTextStream {
    public:
        StatusStream(WhiskerTestMenu& parent);
        ~StatusStream();
    private:
        QString m_str;
        WhiskerTestMenu& m_parent;
    };
    void ensureWhiskerManager();
    void ensureWhiskerConnected();
    void ensureLogBox();
    void deleteLogBox();
    void status(const QString& msg);
    StatusStream stream();
    QVariant getValue(const QVariant* member) const;
    bool setValue(QVariant* member, const QVariant& value);  // returns: changed?
protected:
    QPointer<WhiskerManager> m_whisker;
    QPointer<LogBox> m_logbox;

    QVariant m_host;
    QVariant m_main_port;

    QVariant m_display_num;
    QVariant m_use_video;
    QVariant m_use_two_videos;
    QVariant m_media_directory;
    QVariant m_bmp_filename_1;
    QVariant m_bmp_filename_2;
    QVariant m_video_filename_1;
    QVariant m_video_filename_2;
    QVariant m_input_line_num;
    QVariant m_output_line_num;
};
