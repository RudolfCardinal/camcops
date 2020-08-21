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
#include <QDialog>
#include "questionnairelib/pagepickeritem.h"


class PagePickerDialog : public QDialog
{
    // Choose pages for a Questionnaire.
    // Displays pages that you may be unable to choose, as well.
    // MODAL and BLOCKING.

    Q_OBJECT

    using PagePickerItemList = QVector<PagePickerItem>;

public:
    // Constructor
    PagePickerDialog(QWidget* parent, const PagePickerItemList& pages,
                     const QString& title = "");

    // Call this to offer a choice, return the result of exec(), and write the
    // result to new_page_number.
    virtual int choose(int* new_page_number);

    // Catch generic events
    virtual bool event(QEvent* e) override;

protected slots:
    void itemClicked(int item_index);
protected:
    PagePickerItemList m_pages;
    QString m_title;
    int* m_new_page_number;
    bool m_resized_to_contents;
};
