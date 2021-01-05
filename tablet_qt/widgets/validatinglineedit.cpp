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

#include <QColor>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QValidator>
#include <QWidget>

#include "validatinglineedit.h"

const QColor& GOOD_FOREGROUND = Qt::black;
const QColor& GOOD_BACKGROUND = Qt::green;
const QColor& BAD_FOREGROUND = Qt::white;
const QColor& BAD_BACKGROUND = Qt::red;


ValidatingLineEdit::ValidatingLineEdit(QValidator* validator, QWidget* parent) :
    QVBoxLayout(parent)
{
    m_line_edit = new QLineEdit();
    m_line_edit->setValidator(validator);

    connect(m_line_edit, &QLineEdit::textChanged, this,
            &ValidatingLineEdit::textChanged);
    m_label = new QLabel();

    addWidget(m_line_edit);
    addWidget(m_label);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
}


void ValidatingLineEdit::textChanged()
{
    int pos = 0;
    QString text = getTrimmedText();;

    m_state = m_line_edit->validator()->validate(text, pos);

    const QColor background = isValid() ? GOOD_BACKGROUND : BAD_BACKGROUND;
    const QColor foreground = isValid() ? GOOD_FOREGROUND : BAD_FOREGROUND;
    const QString feedback = isValid() ? tr("Valid") : tr("Invalid");

    QPalette palette;
    palette.setColor(QPalette::Base, background);
    palette.setColor(QPalette::Text, foreground);

    m_line_edit->setPalette(palette);
    m_label->setText(feedback);
    m_label->setAlignment(Qt::AlignRight);

    emit validated();
}


QString ValidatingLineEdit::getTrimmedText()
{
    return m_line_edit->text().trimmed();
}


QValidator::State ValidatingLineEdit::getState()
{
    return m_state;
}


bool ValidatingLineEdit::isValid()
{
    return m_state == QValidator::Acceptable;
}


void ValidatingLineEdit::setInputMethodHints(Qt::InputMethodHints hints)
{
    m_line_edit->setInputMethodHints(hints);
}
