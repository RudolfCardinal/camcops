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
#include <QPointer>
#include <QString>
#include "common/aliases_camcops.h"
#include "tasklib/task.h"

class CamcopsApp;
class OpenableWidget;
class Questionnaire;


class DiagnosisTaskBase : public Task
{
    Q_OBJECT
public:
    DiagnosisTaskBase(CamcopsApp& app, DatabaseManager& db,
                      const QString& tablename,
                      int load_pk = dbconst::NONEXISTENT_PK);
    // ------------------------------------------------------------------------
    // Class overrides
    // ------------------------------------------------------------------------
    // Deferred to subclasses.
    // ------------------------------------------------------------------------
    // Ancillary management
    // ------------------------------------------------------------------------
    virtual QVector<DatabaseObjectPtr> getAllAncillary() const override;
    // ------------------------------------------------------------------------
    // Instance overrides
    // ------------------------------------------------------------------------
    virtual bool isComplete() const override;
    virtual QStringList summary() const override;
    virtual QStringList detail() const override;
    virtual OpenableWidget* editor(bool read_only = false) override;
    // ------------------------------------------------------------------------
    // Task-specific calculations
    // ------------------------------------------------------------------------
    // ------------------------------------------------------------------------
    // DiagnosisTaskBase extras
    // ------------------------------------------------------------------------
protected:
    virtual DiagnosticCodeSetPtr makeCodeset() const = 0;
    virtual DiagnosisItemBasePtr makeItem() const = 0;
    void addItem();
    void deleteItem(int index);
    void moveUp(int index);
    void moveDown(int index);
    QVariant getCode(int index) const;
    bool setCode(int index, const QVariant& value);
    QVariant getDescription(int index) const;
    bool setDescription(int index, const QVariant& value);
    QVariant getComment(int index) const;
    bool setComment(int index, const QVariant& value);
    void refreshQuestionnaire();
    void rebuildPage(QuPage* page);
    void renumberItems();
public:
    static const QString RELATES_TO_DATE;
protected:
    QVector<DiagnosisItemBasePtr> m_items;
    QPointer<Questionnaire> m_questionnaire;
    QVector<QuElementPtr> m_core_elements;
    DiagnosticCodeSetPtr m_codeset;
};
