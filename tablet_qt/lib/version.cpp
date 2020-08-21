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

#include "version.h"
#include <QStringList>
#include "lib/convert.h"


Version::Version()
{
    setFromNumbers(0, 0, 0);
}


Version::Version(const unsigned int major,
                 const unsigned int minor,
                 const unsigned int patch)
{
    setFromNumbers(major, minor, patch);
}


Version::Version(const QString& version_string)
{
    const QStringList parts = version_string.split(".");
    if (parts.size() != 3) {
        setInvalid();
        return;
    }
    bool ok;
    unsigned int major = parts.at(0).toUInt(&ok);
    unsigned int minor = 0;
    unsigned int patch = 0;
    if (ok) {
        minor = parts.at(1).toUInt(&ok);
    }
    if (ok) {
        patch = parts.at(2).toUInt(&ok);
    }
    if (!ok) {
        setInvalid();
        return;
    }
    setFromNumbers(major, minor, patch);
    if (!isValid()) {
        qWarning() << "... invalid version string was:" << version_string;
    }
}


void Version::setFromNumbers(const unsigned int major,
                             const unsigned int minor,
                             const unsigned int patch)
{
    if (minor >= 100 || patch >= 100) {
        qWarning() << Q_FUNC_INFO << "Refusing to create invalid version with:"
                   << "major" << major
                   << "minor" << minor
                   << "patch" << patch
                   << "(creating 0.0.0=invalid instead)";
        setInvalid();
        return;
    }
    m_major = major;
    m_minor = minor;
    m_patch = patch;
    m_valid = true;
}


void Version::setInvalid()
{
    m_valid = false;
    m_major = 0;
    m_minor = 0;
    m_patch = 0;
}


bool Version::isValid() const
{
    return m_valid;
}


QString Version::toString() const
{
    return QString("%1.%2.%3")
            .arg(m_major)
            .arg(m_minor)
            .arg(m_patch);
    // NOT: arg(m_minor, 2, QChar('0'))
    // ... no leading zeros for semantic versioning; http://semver.org/
}



double Version::toFloat() const
{
    // Will be zero (the lowest possible value) for an invalid version.
    return m_major +
            static_cast<double>(m_minor) / 100 +
            static_cast<double>(m_patch) / 10000;
}


QString Version::toFloatString() const
{
    return QString::number(toFloat(), 'f', 4);
}


QVariant Version::toVariant() const
{
    QVariant v;
    v.setValue(*this);
    return v;
}


bool operator<(const Version& v1, const Version& v2)
{
    return v1.toFloat() < v2.toFloat();
}


bool operator<=(const Version& v1, const Version& v2)
{
    return v1.toFloat() <= v2.toFloat();
}


bool operator==(const Version& v1, const Version& v2)
{
    return v1.m_major == v2.m_major &&
           v1.m_minor == v2.m_minor &&
           v1.m_patch == v2.m_patch;
}


bool operator>=(const Version& v1, const Version& v2)
{
    return v1.toFloat() >= v2.toFloat();
}


bool operator>(const Version& v1, const Version& v2)
{
    return v1.toFloat() > v2.toFloat();
}


Version Version::fromVariant(const QVariant& variant)
{
    return variant.value<Version>();
}


Version Version::fromString(const QString& s)
{
    QStringList stringparts = s.split(".");
    const int nparts = stringparts.length();
    if (nparts == 0 || nparts > 3) {
        return makeInvalidVersion();
    }
    QVector<unsigned int> numbers(3, 0);
    bool ok = true;
    for (int i = 0; i < nparts; ++i) {
        numbers[i] = stringparts.at(i).toUInt(&ok);
        if (!ok) {  // Not numeric
            return makeInvalidVersion();
        }
    }
    Version version;
    version.setFromNumbers(numbers.at(0),
                           numbers.at(1),
                           numbers.at(2));
    return version;
}


Version Version::makeInvalidVersion()
{
    return Version(0, 0, 0);
}


bool operator!=(const Version& v1, const Version& v2)
{
    return v1.m_major != v2.m_major ||
           v1.m_minor != v2.m_minor ||
           v1.m_patch != v2.m_patch;
}


QDebug operator<<(QDebug debug, const Version& v)
{
    debug.noquote() << v.toString();
    return debug;
}
