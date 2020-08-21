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

#include "namevalueoptions.h"
#include <QDebug>
#include "lib/convert.h"
#include "lib/uifunc.h"
#include "maths/ccrandom.h"


NameValueOptions::NameValueOptions()
{
}


NameValueOptions::NameValueOptions(
        std::initializer_list<NameValuePair> options) :
    m_options(options)
{
}


void NameValueOptions::append(const NameValuePair& nvp)
{
    m_options.append(nvp);
}


void NameValueOptions::replace(const NameValuePair& nvp,
                               bool append_if_not_found)
{
    const int n = m_options.size();
    const QVariant v = nvp.value();
    for (int i = 0; i < n; ++i) {
        if (m_options.at(i).value() == v) {
            m_options[i] = nvp;
            return;
        }
    }
    if (append_if_not_found) {
        append(nvp);
    }
}


int NameValueOptions::size() const
{
    return m_options.size();
}


const NameValuePair& NameValueOptions::at(const int index) const
{
    return m_options.at(index);
}


int NameValueOptions::indexFromName(const QString& name) const
{
    const int n = m_options.size();
    for (int i = 0; i < n; ++i) {
        if (m_options.at(i).name() == name) {
            return i;
        }
    }
    return -1;
}


int NameValueOptions::indexFromValue(const QVariant& value) const
{
    if (value.isNull()) {
        return -1;
    }
    const int n = m_options.size();
    for (int i = 0; i < n; ++i) {
        if (m_options.at(i).value() == value) {
            return i;
        }
    }
    return -1;
}


void NameValueOptions::validateOrDie()
{
    QVector<QVariant> values;
    for (const NameValuePair& nvp : m_options) {
        const QVariant& v = nvp.value();
        if (values.contains(v)) {
            QString error = QString("NameValueOptions::validateOrDie: "
                                    "Duplicate value %1 found for name %2")
                    .arg(convert::prettyValue(v),
                         nvp.name());
            uifunc::stopApp(error);
        }
        values.append(v);
    }
}


bool NameValueOptions::validIndex(const int index) const
{
    return index >= 0 && index < m_options.size();
}


void NameValueOptions::shuffle()
{
    ccrandom::shuffle(m_options);
}


void NameValueOptions::reverse()
{
    std::reverse(m_options.begin(), m_options.end());
}


QString NameValueOptions::name(const int index) const
{
    if (!validIndex(index)) {
        return "";
    }
    return m_options.at(index).name();
}


QVariant NameValueOptions::value(const int index) const
{
    if (!validIndex(index)) {
        return QVariant();
    }
    return m_options.at(index).value();
}


NameValueOptions NameValueOptions::makeNumbers(const int first,
                                               const int last,
                                               const int step)
{
    NameValueOptions nvo;
    if (first < last && step > 0) {
        for (int i = first; i <= last; i += step) {
            nvo.append(NameValuePair{QString::number(i), i});
        }
    } else if (last < first && step < 0) {
        for (int i = first; i >= last; i += step) {
            nvo.append(NameValuePair{QString::number(i), i});
        }
    } else {
        uifunc::stopApp("Bad arguments to NameValueOptions");
    }
    return nvo;
}


QString NameValueOptions::nameFromValue(const QVariant& value,
                                        const QString& default_) const
{
    const int idx = indexFromValue(value);
    if (idx == -1) {
        return default_;
    }
    return name(idx);
}


QVariant NameValueOptions::valueFromName(const QString& name,
                                         const QVariant& default_) const
{
    const int idx = indexFromName(name);
    if (idx == -1) {
        return default_;
    }
    return value(idx);
}


// ========================================================================
// For friends
// ========================================================================

QDebug operator<<(QDebug debug, const NameValueOptions& nvo)
{
    QDebug& d = debug.nospace();
    d << "NameValueOptions{";
    bool first = true;
    for (const NameValuePair& nvp : nvo.m_options) {
        if (!first) {
            d << ", ";
        } else {
            first = false;
        }
        d << nvp.name() << ": " << nvp.value() << "}";
    }
    d << "}";
    return debug;
}
