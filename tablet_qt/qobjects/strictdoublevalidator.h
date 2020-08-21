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
#include <QDoubleValidator>


class StrictDoubleValidator : public QDoubleValidator
{
    // - Validates a double (floating-point) being typed in.
    // - Checks the characters against the specified bottom/top (min/max) values.
    // - The default maximum number of decimal places, 1000, matches
    //   QDoubleValidator; see
    //   http://doc.qt.io/qt-5/qdoublevalidator.html#decimals-prop

    // http://stackoverflow.com/questions/19571033/allow-entry-in-qlineedit-only-within-range-of-qdoublevalidator
    // ... but that doesn't work properly (it prohibits valid things on the
    // way to success).
    Q_OBJECT
public:
    StrictDoubleValidator(double bottom, double top, int decimals = 1000,
                          bool allow_empty = false, QObject* parent = nullptr);
    virtual QValidator::State validate(QString& s, int& pos) const override;
protected:
    bool m_allow_empty;
};
