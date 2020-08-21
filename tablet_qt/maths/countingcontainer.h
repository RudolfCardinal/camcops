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
#include <QDebug>
#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

// A container that counts items you put into it.

template <typename T>
class CountingContainer
{
public:
    CountingContainer() {}

    // Add an iteam.
    void add(const T& item)
    {
        if (!m_map.contains(item)) {
            m_map[item] = 0;
        }
        ++m_map[item];
    }

    // Return the map from object to count.
    QMap<T, int> results() const
    {
        return m_map;
    }

    // Return a string summary.
    QString asString(bool sorted = true) const
    {
        QStringList items;
        if (sorted) {
            QList<T> keys = m_map.keys();
            // qSort obsolete. See
            // https://lists.qt-project.org/pipermail/development/2013-September/013122.html
            // Was:
            //      qSort(keys);
            std::sort(keys.begin(), keys.end());
            for (const T& key : keys) {
                items.append(QString("%1: %2").arg(key).arg(m_map.value(key)));
            }

        } else {
            QMapIterator<T, int> it(m_map);
            while (it.hasNext()) {
                it.next();
                items.append(QString("%1: %2").arg(it.key()).arg(it.value()));
            }
        }
        return QString("CountingContainer(%1)").arg(items.join(", "));
    }

protected:
    QMap<T, int> m_map;

public:
    // Debugging description.
    friend QDebug operator<<(QDebug debug, const CountingContainer<T>& c)
    {
        debug.noquote() << c.asString();
        return debug;
    }
};
