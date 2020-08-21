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
#include <QPointer>
class QLineEdit;


class PasswordChangeDialog : public QDialog
{
    // Dialogue to request old and new passwords, and check that they
    // match and are not blank.
    // MODAL and BLOCKING: call exec() then check oldPassword() and
    // newPassword() if it succeeds.

    Q_OBJECT
public:
    PasswordChangeDialog(const QString& text, const QString& title,
                         bool require_old_password,
                         QWidget* parent = nullptr);
    QString oldPassword() const;
    QString newPassword() const;
protected:
    void okClicked();
protected:
    QPointer<QLineEdit> m_editor_old;
    QPointer<QLineEdit> m_editor_new1;
    QPointer<QLineEdit> m_editor_new2;
};
