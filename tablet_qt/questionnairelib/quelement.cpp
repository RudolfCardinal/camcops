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

#include "quelement.h"
#include <QWidget>
#include "db/fieldref.h"

const Qt::Alignment DEFAULT_QUELEMENT_WIDGET_ALIGNMENT = Qt::Alignment();
// If you use Qt::AlignLeft, widgets will not span the full width.
// This is relevant for QuHeading, QuSlider, etc. -- things that want maximum
// space.


QuElement::QuElement() :
    m_widget(nullptr),
    m_visible(true),
    m_widget_alignment(DEFAULT_QUELEMENT_WIDGET_ALIGNMENT)
{
}


QuElement::~QuElement()
{
}


QuElement* QuElement::addTag(const QString& tag)
{
    m_tags.append(tag);
    return this;
}


bool QuElement::hasTag(const QString &tag) const
{
    return m_tags.contains(tag);
}


QPointer<QWidget> QuElement::widget(Questionnaire* questionnaire)
{
    if (!m_widget) {
        // not yet made, or deleted by Qt
        m_widget = makeWidget(questionnaire);
        // Note: there is no QWidget::setAlignment(); see
        // http://doc.qt.io/qt-5/qwidget.html
        if (!m_visible) {
            // Widgets always default to visible. No point calling setVisible()
            // if we want them to be visible; it causes delays and screen
            // repaints.
            m_widget->setVisible(m_visible);
        }
    }
    return m_widget;
}


QVector<QuElementPtr> QuElement::subelements() const
{
    return QVector<QuElementPtr>();
}


QVector<QuElement*> QuElement::subelementsRaw() const
{
    QVector<QuElement*> raw;
    for (const QuElementPtr& e : subelements()) {
        raw.append(e.data());
    }
    return raw;
}


QVector<QuElementPtr> QuElement::subelementsWithChildrenFlattened() const
{
    QVector<QuElementPtr> all_children;
    const QVector<QuElementPtr> sub = subelements();
    for (const QuElementPtr& e : sub) {
        all_children.append(e);
        all_children.append(e->subelementsWithChildrenFlattened());
    }
    return all_children;
}


QVector<QuElement*> QuElement::subelementsWithChildrenFlattenedRaw() const
{
    QVector<QuElement*> raw;
    for (const QuElementPtr& e : subelementsWithChildrenFlattened()) {
        raw.append(e.data());
    }
    return raw;
}


bool QuElement::missingInput() const
{
    const FieldRefPtrList frefs = fieldrefs();
    for (const FieldRefPtr& f : frefs) {
        if (f->missingInput()) {
            return true;
        }
    }
    return false;
}


FieldRefPtrList QuElement::fieldrefs() const
{
    return FieldRefPtrList();
}


void QuElement::show()
{
    setVisible(true);
}


void QuElement::hide()
{
    setVisible(false);
}


bool QuElement::visible() const
{
    return m_visible;
}


QuElement* QuElement::setVisible(const bool visible)
{
    // qDebug() << Q_FUNC_INFO << visible;
    if (visible == m_visible) {
        return this;
    }
    m_visible = visible;
    if (m_widget) {
        m_widget->setVisible(visible);
    }
    return this;
}


QuElement* QuElement::setWidgetAlignment(const Qt::Alignment alignment)
{
    m_widget_alignment = alignment;
    return this;
}


Qt::Alignment QuElement::getWidgetAlignment() const
{
    return m_widget_alignment;
}


void QuElement::closing()
{
}
