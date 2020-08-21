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

#include "quverticalcontainer.h"
#include <QWidget>
#include "layouts/layouts.h"
#include "lib/sizehelpers.h"
#include "questionnairelib/questionnaire.h"
#include "widgets/basewidget.h"

QuVerticalContainer::QuVerticalContainer() :
    QuSequenceContainerBase()
{
}


QuVerticalContainer::QuVerticalContainer(
        const QVector<QuElementPtr>& elements) :
    QuSequenceContainerBase(elements)
{
}


QuVerticalContainer::QuVerticalContainer(
        std::initializer_list<QuElementPtr> elements) :
    QuSequenceContainerBase(elements)
{
}


QuVerticalContainer::QuVerticalContainer(
        std::initializer_list<QuElement*> elements) :  // takes ownership
    QuSequenceContainerBase(elements)
{
}


QPointer<QWidget> QuVerticalContainer::makeWidget(Questionnaire* questionnaire)
{
    QPointer<QWidget> widget(new BaseWidget());
    widget->setSizePolicy(sizehelpers::expandingFixedHFWPolicy());

    auto layout = new VBoxLayout();

    // widget->setObjectName(CssConst::DEBUG_YELLOW);
    layout->setContentsMargins(uiconst::NO_MARGINS);
    widget->setLayout(layout);
    for (int i = 0; i < m_elements.size(); ++i) {
        auto e = m_elements.at(i);
        const auto alignment = m_override_widget_alignment
                ? DefaultWidgetAlignment
                : e->getWidgetAlignment();
        QPointer<QWidget> w = e->widget(questionnaire);
        if (!w) {
            qWarning() << Q_FUNC_INFO << "Element failed to create a widget!";
            continue;
        }
        layout->addWidget(w, 0, alignment);
    }
    return widget;
}
