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
#include "menulib/menuwindow.h"
class QMediaPlayer;
class WhiskerInboundMessage;


class TestMenu : public MenuWindow
{
    Q_OBJECT

public:
    TestMenu(CamcopsApp& app);
    ~TestMenu() override;
    virtual QString title() const override;
protected:
    virtual void makeItems() override;
    void testPhq9Creation();
    void testDebugConsole();
    void testSound();
    void testHttp();
    void testHttps();
    void testIcd10CodeSetCreation();
    void testIcd9cmCodeSetCreation();
    void doneSeeConsole();
    void testProgress();
    void testWait();
    void testScrollMessageBox();
    void expensiveFunction();
    void testSizeFormatter();
    void testConversions();
    void testEigenFunctions();
    void testRandom();
    void testOtherMaths();
    void testLogisticRegression();
    void testGLMGaussian();
    void testGLMPoisson();
    void testTranslation();
    void switchToEnglish();
    void switchToDanish();

protected:
    QSharedPointer<QMediaPlayer> m_player;
};
