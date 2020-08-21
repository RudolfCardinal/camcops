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
#include <QWidget>
#include "questionnairelib/qutext.h"


class QuHeading : public QuText
{
    // Provides text with a heading style, plus a shaded background that
    // stretches right.

    Q_OBJECT

protected:
    // Protected constructor, used internally and by derived classes.
    QuHeading(const QString& text, FieldRefPtr fieldref);

public:
    // Constructor to display static text.
    QuHeading(const QString& text);

    // Constructor to display dynamic text, from a field.
    QuHeading(FieldRefPtr fieldref);

protected:
    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;

protected:
    QPointer<QWidget> m_container;  // our widget
};
