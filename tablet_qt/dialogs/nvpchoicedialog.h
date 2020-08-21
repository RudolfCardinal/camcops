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
#include <QSize>
#include <QString>
#include "common/uiconst.h"
#include "lib/uifunc.h"
#include "questionnairelib/namevalueoptions.h"

class QShowEvent;
class QVariant;


class NvpChoiceDialog : public QDialog
{
    // Dialog to choose between a set of name/value pairs (offering the names,
    // returning the chosen value via a pointer).
    // MODAL and BLOCKING.

    Q_OBJECT
public:
    // Constructor
    NvpChoiceDialog(QWidget* parent, const NameValueOptions& options,
                    const QString& title = "");

    // Choose whether any existing choice should be indicated graphically.
    void showExistingChoice(
            bool show_existing_choice = true,
            const QString& icon_filename = uifunc::iconFilename(uiconst::CBS_OK),
            const QSize& icon_size = uiconst::g_small_iconsize);

    // Call this to offer a choice, return the result of exec(), and write the
    // result to new_value.
    virtual int choose(QVariant* new_value);

    // Catch generic events
    virtual bool event(QEvent* e) override;

protected slots:
    void itemClicked(int index);
protected:
    NameValueOptions m_options;
    QString m_title;
    bool m_show_existing_choice;
    QString m_icon_filename;
    QSize m_icon_size;
    QVariant* m_p_new_value;
    bool m_resized_to_contents;
};
