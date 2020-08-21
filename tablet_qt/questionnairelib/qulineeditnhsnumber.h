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
#include "questionnairelib/qulineeditint64.h"


class QuLineEditNHSNumber : public QuLineEditInt64
{
    // Offers a one-line text editor, for a UK NHS number.

    Q_OBJECT
public:

    // Constructor for unconstrained NHS numbers
    // - allow_empty: OK to be blank?
    QuLineEditNHSNumber(FieldRefPtr fieldref, bool allow_empty = true);

    // Don't allow a version with minimum/maximum, by deleting this signature.
    QuLineEditNHSNumber(FieldRefPtr fieldref, int minimum, int maximum,
                        bool allow_empty) = delete;

protected:
    virtual void extraLineEditCreation(QLineEdit* editor) override;
};
