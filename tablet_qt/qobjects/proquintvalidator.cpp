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

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QString>
#include <QUrl>
#include <QValidator>

#include "proquintvalidator.h"


ProquintValidator::ProquintValidator(QObject* parent) : QValidator(parent)
{
}

QValidator::State ProquintValidator::validate(QString &input, int &) const
{
    const QString consonant = "[bdfghjklmnprstvz]";
    const QString vowel = "[aiou]";
    const QString quint = QString("%1%2%3%4%5").arg(
        consonant, vowel, consonant, vowel, consonant
    );
    const QString check_character = consonant;
    QRegularExpression proquint_regex(
        QString("%1-%2-%3-%4-%5-%6-%7-%8-%9").arg(
            quint,quint,quint,quint,quint,quint,quint,quint,check_character
        )
    );

    const QRegularExpressionMatch match = proquint_regex.match(input);

    if (!match.hasMatch()) {
        return QValidator::Intermediate;
    }

    if (!validateLuhnMod16(input)) {
        return QValidator::Intermediate;
    }

    return QValidator::Acceptable;
}

bool ProquintValidator::validateLuhnMod16(QString input) const
{
    const QMap<QChar, int> lookup_table {
        {'b', 0x0},
        {'d', 0x1},
        {'f', 0x2},
        {'g', 0x3},
        {'h', 0x4},
        {'j', 0x5},
        {'k', 0x6},
        {'l', 0x7},
        {'m', 0x8},
        {'n', 0x9},
        {'p', 0xa},
        {'r', 0xb},
        {'s', 0xc},
        {'t', 0xd},
        {'v', 0xe},
        {'z', 0xf},
        {'a', 0x0},
        {'i', 0x1},
        {'o', 0x2},
        {'u', 0x3},
    };

    // https://en.wikipedia.org/wiki/Luhn_mod_N_algorithm
    QString proquint = input.trimmed().replace("-", "");

    int factor = 1;
    int sum = 0;

    for (QString::const_reverse_iterator it = proquint.rbegin();
            it != proquint.rend(); ++it) {
        const int value = lookup_table.value(*it) * factor;
        sum += (value / 16 + value % 16);

        factor = (factor == 2) ? 1 : 2;
    }

    const int remainder = sum % 16;

    return remainder == 0;
}
