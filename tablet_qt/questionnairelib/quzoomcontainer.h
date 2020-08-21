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
#include "questionnairelib/quelement.h"


class QuZoomContainer : public QuElement
{
    // Contains another element and allows it to be scaled or zoomed.
    //
    // NOT OF PRODUCTION QUALITY. TRIED FOR EQ5D5L.
    // NOT IN CURRENT USE 2020-04-14.

public:
    // Construct and add the element
    QuZoomContainer(QuElementPtr element);
    QuZoomContainer(QuElement* element);

protected:
    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override;
    virtual QVector<QuElementPtr> subelements() const override;

protected:
    QuElementPtr m_element;  // our contained element
};
