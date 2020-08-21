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
#include "tasklib/task.h"

class CamcopsApp;
class Questionnaire;
class OpenableWidget;
class TaskFactory;

void initializePhotoSequence(TaskFactory& factory);


class PhotoSequence : public Task
{
    Q_OBJECT
public:
    PhotoSequence(CamcopsApp& app, DatabaseManager& db,
                  int load_pk = dbconst::NONEXISTENT_PK);
    // ------------------------------------------------------------------------
    // Class overrides
    // ------------------------------------------------------------------------
    virtual QString shortname() const override;
    virtual QString longname() const override;
    virtual QString description() const override;
    virtual QString infoFilenameStem() const override;
    virtual bool isCrippled() const override { return false; }
    // ------------------------------------------------------------------------
    // Ancillary management
    // ------------------------------------------------------------------------
    virtual QStringList ancillaryTables() const override;
    virtual QString ancillaryTableFKToTaskFieldname() const override;
    virtual void loadAllAncillary(int pk) override;
    virtual QVector<DatabaseObjectPtr> getAncillarySpecimens() const override;
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
protected:
    int numPhotos() const;
    // ------------------------------------------------------------------------
    // Signal handlers
    // ------------------------------------------------------------------------
    void refreshQuestionnaire();
    void addPage(int page_index);
    void rebuildPage(QuPage* page, int page_index);
    void renumberPhotos();
    void addPhoto();
    void deletePhoto(int index);
    void movePhotoForwards(int index);
    void movePhotoBackwards(int index);
    // ------------------------------------------------------------------------
    // Text
    // ------------------------------------------------------------------------
    static QString txtPhoto();
    static QString txtPhotos();
    static QString txtAdd();
    // ------------------------------------------------------------------------
    // Data
    // ------------------------------------------------------------------------
protected:
    QVector<PhotoSequencePhotoPtr> m_photos;
    QPointer<Questionnaire> m_questionnaire;
public:
    static const QString PHOTOSEQUENCE_TABLENAME;
};
