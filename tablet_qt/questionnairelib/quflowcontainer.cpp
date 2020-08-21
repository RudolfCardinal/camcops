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

// #define DEBUG_LAYOUT

#include "quflowcontainer.h"
#include <QWidget>
#include "layouts/flowlayouthfw.h"
#include "questionnairelib/questionnaire.h"
#include "widgets/basewidget.h"

#ifdef DEBUG_LAYOUT
#include "common/cssconst.h"
#endif


QuFlowContainer::QuFlowContainer() :
    QuSequenceContainerBase()
{
}


QuFlowContainer::QuFlowContainer(const QVector<QuElementPtr>& elements) :
    QuSequenceContainerBase(elements)
{
}


QuFlowContainer::QuFlowContainer(std::initializer_list<QuElementPtr> elements) :
    QuSequenceContainerBase(elements)
{
}


QuFlowContainer::QuFlowContainer(std::initializer_list<QuElement*> elements) :
    QuSequenceContainerBase(elements)
{
}


QPointer<QWidget> QuFlowContainer::makeWidget(
        Questionnaire* questionnaire)
{
    QPointer<QWidget> widget(new BaseWidget());
    // NO, THIS BREAKS HFW: // widget->setSizePolicy(sizehelpers::expandingFixedHFWPolicy());

    auto layout = new FlowLayoutHfw();
#ifdef DEBUG_LAYOUT
    widget->setObjectName(cssconst::DEBUG_YELLOW);
#endif
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
        layout->addWidget(w, alignment);  // this is QLayout::setAlignment
    }
    return widget;
}
