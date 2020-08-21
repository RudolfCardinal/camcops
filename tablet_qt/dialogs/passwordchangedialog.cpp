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

#include "passwordchangedialog.h"
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include "lib/uifunc.h"


PasswordChangeDialog::PasswordChangeDialog(const QString& text,
                                           const QString& title,
                                           const bool require_old_password,
                                           QWidget* parent) :
    QDialog(parent),
    m_editor_old(nullptr),
    m_editor_new1(nullptr),
    m_editor_new2(nullptr)
{
    setWindowTitle(title);
    setMinimumSize(uifunc::minimumSizeForTitle(this));

    auto mainlayout = new QVBoxLayout();

    auto prompt = new QLabel(text);
    mainlayout->addWidget(prompt);

    if (require_old_password) {
        mainlayout->addWidget(new QLabel(tr("Enter old password:")));
        m_editor_old = new QLineEdit();
        m_editor_old->setEchoMode(QLineEdit::Password);
        mainlayout->addWidget(m_editor_old);
    }
    mainlayout->addWidget(new QLabel(tr("Enter new password:")));
    m_editor_new1 = new QLineEdit();
    m_editor_new1->setEchoMode(QLineEdit::Password);
    mainlayout->addWidget(m_editor_new1);

    mainlayout->addWidget(new QLabel(tr("Enter new password again for confirmation:")));
    m_editor_new2 = new QLineEdit();
    m_editor_new2->setEchoMode(QLineEdit::Password);
    mainlayout->addWidget(m_editor_new2);

    auto buttonbox = new QDialogButtonBox(
                QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonbox, &QDialogButtonBox::accepted,
            this, &PasswordChangeDialog::okClicked);
    connect(buttonbox, &QDialogButtonBox::rejected,
            this, &PasswordChangeDialog::reject);
    mainlayout->addWidget(buttonbox);

    setLayout(mainlayout);
}


QString PasswordChangeDialog::oldPassword() const
{
    if (!m_editor_old) {
        return "";
    }
    return m_editor_old->text();
}


QString PasswordChangeDialog::newPassword() const
{
    if (!m_editor_new1) {
        return "";
    }
    return m_editor_new1->text();
}


void PasswordChangeDialog::okClicked()
{
    if (!m_editor_new1 || !m_editor_new2) {
        return;
    }
    const QString newpw1 = m_editor_new1->text();
    const QString newpw2 = m_editor_new2->text();
    if (newpw1.isEmpty()) {
        uifunc::alert(tr("Can't set an empty password"));
        return;
    }
    if (newpw1 != newpw2) {
        uifunc::alert(tr("New passwords don't match"));
        return;
    }
    accept();
}
