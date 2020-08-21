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

#include "qusequencecontainerbase.h"
#include <QWidget>
#include "common/cssconst.h"
#include "layouts/flowlayouthfw.h"
#include "lib/sizehelpers.h"
#include "questionnairelib/questionnaire.h"
#include "widgets/basewidget.h"


const Qt::Alignment QuSequenceContainerBase::DefaultWidgetAlignment =
        Qt::AlignLeft | Qt::AlignVCenter;
// Note that a widget alignment of Qt::Alignment(), the default, makes the
// layout EQUISPACE the widgets, which can look daft for horizontal layouts.
// A better default is Qt::AlignLeft | Qt::AlignVCenter.
// - See also QuElement, which holds the widget's preferred alignment.
// - http://www.qtcentre.org/threads/53609-QHBoxLayout-widget-spacing
// - http://stackoverflow.com/questions/4539406/nonstatic-member-as-a-default-argument-of-a-nonstatic-member-function
// - http://en.cppreference.com/w/cpp/language/default_arguments


QuSequenceContainerBase::QuSequenceContainerBase(
        const QVector<QuElementPtr>& elements) :
    m_elements(elements),
    m_override_widget_alignment(true)
{
}


QuSequenceContainerBase::QuSequenceContainerBase() :
    QuSequenceContainerBase(QVector<QuElementPtr>())  // delegating constructor
{
}


QuSequenceContainerBase::QuSequenceContainerBase(
        std::initializer_list<QuElementPtr> elements) :
    QuSequenceContainerBase(QVector<QuElementPtr>(elements))  // delegating constructor
{
}


QuSequenceContainerBase::QuSequenceContainerBase(
        std::initializer_list<QuElement*> elements) :
    QuSequenceContainerBase()  // delegating constructor
{
    for (auto e : elements) {
        addElement(e);
    }
}


QuSequenceContainerBase* QuSequenceContainerBase::addElement(const QuElementPtr& element)
{
    m_elements.append(element);
    return this;
}


QuSequenceContainerBase* QuSequenceContainerBase::addElement(QuElement* element)  // takes ownership
{
    if (element) {
        m_elements.append(QuElementPtr(element));
    }
    return this;
}


QuSequenceContainerBase* QuSequenceContainerBase::setOverrideWidgetAlignment(bool override)
{
    m_override_widget_alignment = override;
    return this;
}



QuSequenceContainerBase* QuSequenceContainerBase::setContainedWidgetAlignments(
        const Qt::Alignment alignment)
{
    for (auto e : m_elements) {
        e->setWidgetAlignment(alignment);
    }
    m_override_widget_alignment = false;
    return this;
}


QVector<QuElementPtr> QuSequenceContainerBase::subelements() const
{
    return m_elements;
}
