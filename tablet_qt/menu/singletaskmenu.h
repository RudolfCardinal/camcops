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
#include <QString>
#include "menulib/menuwindow.h"
class Patient;


class SingleTaskMenu : public MenuWindow
{
    // This is the menu class that serves all tasks.

    Q_OBJECT
public:
    SingleTaskMenu(const QString& tablename, CamcopsApp& app);
    virtual QString title() const override;
protected:
    virtual void extraLayoutCreation() override;
    virtual void makeItems() override;
    virtual void afterBuild() override;
public slots:  // http://stackoverflow.com/questions/19129133/qt-signals-and-slots-permissions
    void addTask();
    void selectedPatientChanged(const Patient* patient);
protected:
    void showTaskStatus() const;
    void refreshTaskList();
protected:
    QString m_tablename;
    bool m_anonymous;
};
