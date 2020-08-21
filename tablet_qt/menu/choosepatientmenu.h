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
#include "common/aliases_camcops.h"
#include "menulib/menuwindow.h"


class ChoosePatientMenu : public MenuWindow
{
    Q_OBJECT
public:
    ChoosePatientMenu(CamcopsApp& app);
    virtual QString title() const override;
protected:
    virtual void extraLayoutCreation() override;
    virtual void makeItems() override;
    virtual void viewItem() override;
    virtual void editItem() override;
    virtual void deleteItem() override;
    void editPatient(bool read_only);
    void deletePatient();
public slots:
    void addPatient();
    void refreshPatientList();
protected:
    void mergePatients();
    static QString txtMergeTitle();
};
