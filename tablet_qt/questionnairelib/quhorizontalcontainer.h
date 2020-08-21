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
#include "questionnairelib/qusequencecontainerbase.h"


class QuHorizontalContainer : public QuSequenceContainerBase
{
    // Allows the arrangements of other elements into a horizontal layout.

    Q_OBJECT

public:
    // Plain constructor
    QuHorizontalContainer();

    // Construct with elements
    QuHorizontalContainer(const QVector<QuElementPtr>& elements);
    QuHorizontalContainer(std::initializer_list<QuElementPtr> elements);
    QuHorizontalContainer(std::initializer_list<QuElement*> elements);  // takes ownership

    // Should we add a "stretch" to the right-hand side of the layout?
    // This makes the difference between:
    //
    //      | W1 W2 W3 W4 stretch_____________ |
    //
    // and
    //
    //      | W1        W2        W3        W4 |
    //
    QuHorizontalContainer* setAddStretchRight(bool add_stretch_right);

protected:
    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;

protected:
    bool m_add_stretch_right;  // add stretch on the right?
};
