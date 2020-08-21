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
#include <QString>
#include <QVariant>

extern const QString KIRBY_DEFAULT_CURRENCY;
extern const bool KIRBY_DEFAULT_CURRENCY_SYMBOL_FIRST;


class KirbyRewardPair
{
public:
    // Must accept zero arguments to live in a QVector.
    KirbyRewardPair(
            int sir = 0, int ldr = 0, int delay_days = 0,
            const QVariant& chose_ldr = false,  // result, if desired
            const QString& currency = KIRBY_DEFAULT_CURRENCY,
            bool currency_symbol_first = KIRBY_DEFAULT_CURRENCY_SYMBOL_FIRST);

    // Return the question.
    QString sirString() const;
    QString ldrString() const;
    QString question() const;
    QString answer() const;

    // Return a currency amount, formatted.
    QString money(int amount) const;

    // Implied value of k if indifferent, according to V = A(1 + kD)
    // where A is amount and D is delay. The units of k are days ^ -1.
    double kIndifference() const;

    // Was the choice consistent with the k value given?
    // - If no choice has been recorded, returns false.
    // - If the k value equals the implied indifference point exactly (meaning
    //   that the subject should not care), return true.
    bool choiceConsistent(double k) const;

public:
    // Data
    int sir;  // small immediate reward
    int ldr;  // large delayed reward
    int delay_days;  // delay to large delayed reward

    QString currency;  // currency symbol
    bool currency_symbol_first;  // true as in £10, or false as in 3€?

    QVariant chose_ldr;  // used only for results representation
};
