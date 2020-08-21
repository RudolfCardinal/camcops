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

#include "quzoomcontainer.h"
#include <QDebug>
#include <QWidget>
#include "lib/uifunc.h"
#include "questionnairelib/questionnaire.h"
#include "widgets/zoomablewidget.h"


QuZoomContainer::QuZoomContainer(QuElementPtr element) :
    m_element(element)
{
    if (!element) {
        uifunc::stopApp("No element passed to QuZoomContainer");
    }
}


QuZoomContainer::QuZoomContainer(QuElement* element) :
    QuZoomContainer(QuElementPtr(element))
{
}


QVector<QuElementPtr> QuZoomContainer::subelements() const
{
    return QVector<QuElementPtr>{m_element};
}


QPointer<QWidget> QuZoomContainer::makeWidget(Questionnaire* questionnaire)
{
    QPointer<QWidget> contents = m_element->widget(questionnaire);
    auto zoom = new ZoomableWidget(
                contents
                // ... more options here
    );
    return zoom;
}
