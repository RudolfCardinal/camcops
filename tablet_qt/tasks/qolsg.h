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
#include <QColor>
#include <QPointer>
#include <QString>
#include "graphics/graphicsfunc.h"
#include "tasklib/task.h"

class AdjustablePie;
class CamcopsApp;
class OpenableWidget;
class QGraphicsScene;
class TaskFactory;

void initializeQolSG(TaskFactory& factory);


class QolSG : public Task
{
    Q_OBJECT
public:
    QolSG(CamcopsApp& app, DatabaseManager& db,
          int load_pk = dbconst::NONEXISTENT_PK);
    // ------------------------------------------------------------------------
    // Class overrides
    // ------------------------------------------------------------------------
    virtual QString shortname() const override;
    virtual QString longname() const override;
    virtual QString description() const override;
    virtual QString infoFilenameStem() const override;
    virtual bool isEditable() const override { return false; }
    virtual bool isCrippled() const override { return false; }
    // ------------------------------------------------------------------------
    // Instance overrides
    // ------------------------------------------------------------------------
    virtual bool isComplete() const override;
    virtual QStringList summary() const override;
    virtual QStringList detail() const override;
    virtual OpenableWidget* editor(bool read_only = false) override;
    // ------------------------------------------------------------------------
    // Internals
    // ------------------------------------------------------------------------
public:
    struct LotteryOption {
        LotteryOption() {}
        LotteryOption(const QString& label, const QColor& fill_colour,
                      const QColor& text_colour) :
            label(label),
            fill_colour(fill_colour),
            text_colour(text_colour)
        {}
        QString label;
        QColor fill_colour;
        QColor text_colour;
    };
protected:
    void startTask();
    void askCategory();
    void thanks();
    void clearScene();
    graphicsfunc::AdjustablePieAndProxy makePie(const QPointF& centre,
                                                int n_sectors);
    void showFixed(bool left, const LotteryOption& option);
    void showLottery(bool left, const LotteryOption& option1,
                     const LotteryOption& option2, qreal starting_p);
    void showGambleInstruction(bool lottery_on_left,
                               const QString& category_chosen);
    void lotteryTouched(qreal p);
protected slots:
    void giveChoice(const QString& category_chosen);
    void recordChoice();
    void finished();
    void pieAdjusted(const QVector<qreal>& proportions);
protected:
    QPointer<QGraphicsScene> m_scene;
    QPointer<OpenableWidget> m_widget;
    QPointer<AdjustablePie> m_pie;
    bool m_pie_touched_at_least_once;
    qreal m_last_p;
public:
    static const QString QOLSG_TABLENAME;
};
