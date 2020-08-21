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

#include "quthermometeritem.h"


QuThermometerItem::QuThermometerItem()
{
}


QuThermometerItem::QuThermometerItem(const QString& active_filename,
                                     const QString& inactive_filename,
                                     const QString& text,
                                     const QVariant& value,
                                     Qt::Alignment text_alignment) :
    m_active_filename(active_filename),
    m_inactive_filename(inactive_filename),
    m_text(text),
    m_value(value),
    m_text_alignment(text_alignment)
{
    Q_ASSERT(!m_value.isNull());
}


QString QuThermometerItem::activeFilename() const
{
    return m_active_filename;
}


QString QuThermometerItem::inactiveFilename() const
{
    return m_inactive_filename;
}


QString QuThermometerItem::text() const
{
    return m_text;
}


QVariant QuThermometerItem::value() const
{
    return m_value;
}


Qt::Alignment QuThermometerItem::textAlignment() const
{
    return m_text_alignment;
}
