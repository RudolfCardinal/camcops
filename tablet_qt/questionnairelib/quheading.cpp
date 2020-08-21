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

#include "quheading.h"
#include "common/cssconst.h"
#include "common/uiconst.h"
#include "layouts/layouts.h"
#include "lib/uifunc.h"
#include "lib/sizehelpers.h"
#include "questionnairelib/questionnaire.h"
#include "widgets/basewidget.h"
#include "widgets/labelwordwrapwide.h"


QuHeading::QuHeading(const QString& text, FieldRefPtr fieldref) :
    QuText(text, fieldref)  // uses protected constructor of base class
{
    m_fontsize = uiconst::FontSize::Heading;
    m_bold = false;
    setWidgetAlignment(Qt::Alignment());
    // ... makes it span the full width of the page.
}


QuHeading::QuHeading(const QString& text) :
    QuHeading(text, nullptr)
{
}


QuHeading::QuHeading(FieldRefPtr fieldref) :
    QuText("", fieldref)
{
}


QPointer<QWidget> QuHeading::makeWidget(Questionnaire* questionnaire)
{
    // Call parent (which sets m_label), ignore result:
    QuText::makeWidget(questionnaire);

    // Add background, and return m_container (containing m_label):
    m_container = new BaseWidget();
    m_container->setSizePolicy(sizehelpers::expandingFixedHFWPolicy());
    auto layout = new HBoxLayout();
    m_container->setObjectName(cssconst::QUHEADING);
    m_container->setLayout(layout);
    layout->addWidget(m_label, 0, Qt::AlignLeft | Qt::AlignTop);
    layout->addStretch();
    return m_container;
}
