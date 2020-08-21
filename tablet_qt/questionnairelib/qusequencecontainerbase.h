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


class QuSequenceContainerBase : public QuElement
{
    // Abstract base class from which questionnaire containers are implemented
    // that contain a sequence of objects -- i.e. QuHorizontalContainer,
    // QuVerticalContainer, QuFlowContainer (but not QuGridContainer).

    Q_OBJECT
public:
    static const Qt::Alignment DefaultWidgetAlignment;

public:
    // Plain constructor
    QuSequenceContainerBase();

    // Construct and add elements
    QuSequenceContainerBase(const QVector<QuElementPtr>& elements);
    QuSequenceContainerBase(std::initializer_list<QuElementPtr> elements);
    QuSequenceContainerBase(std::initializer_list<QuElement*> elements);

    // Add an element.
    QuSequenceContainerBase* addElement(const QuElementPtr& element);

    // Add an element. If you add a nullptr, it will be ignored.
    QuSequenceContainerBase* addElement(QuElement* element);  // takes ownership

    // Choose whether the container overrides the alignments of its widgets, to
    // the QuFlowContainer's default, when building the Qt container widget.
    // This is the default setting.
    // Otherwise, the QuElement's getWidgetAlignment() is used.
    QuSequenceContainerBase* setOverrideWidgetAlignment(bool override);

    // Set alignment of all our widgets, by calling through to
    // QuElement::setWidgetAlignment() for each.
    // This also (effectively) calls setOverrideWidgetAlignment(false).
    QuSequenceContainerBase* setContainedWidgetAlignments(Qt::Alignment alignment);

protected:
    virtual QPointer<QWidget> makeWidget(Questionnaire* questionnaire) override = 0;
    virtual QVector<QuElementPtr> subelements() const override;

protected:
    QVector<QuElementPtr> m_elements;  // all our elements
    bool m_override_widget_alignment;  // see setOverrideWidgetAlignment() above
};
