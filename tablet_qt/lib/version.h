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
#include <QString>

// https://stackoverflow.com/questions/22240973/major-and-minor-macros-defined-in-sys-sysmacros-h-pulled-in-by-iterator
#pragma push_macro("major")
#pragma push_macro("minor")
#undef major
#undef minor

// Represents a semantic version: http://semver.org/
//
// ... with the additional constraint of minor/patch versions being
// limited to integers in the range 0-99.

class Version {
public:
    Version();  // public default constructor required for QVariant use

    // Construct from numbers
    Version(unsigned int major, unsigned int minor, unsigned int patch);

    // Construct from a "major.minor.patch" or similar string.
    Version(const QString& version_string);

    // public copy constructor required for QVariant use; default is OK

    // Get the major, minor, and patch components:
    unsigned int major() const;
    unsigned int minor() const;
    unsigned int patch() const;

    // Return a string of the form "major.minor.patch".
    QString toString() const;

    // Coerces it in a slightly inelegant way to a float: MM.mmpp
    // (where MM is major, mm is minor, pp is patch).
    // Since the minor/patch numbers are constrained to 0-99, this works.
    double toFloat() const;

    // A string version of toFloat(), to 4dp.
    QString toFloatString() const;

    // Returns a QVariant.
    // (This relies on us having registered Version with QVariant.)
    QVariant toVariant() const;

    // Is it valid?
    bool isValid() const;

    // Comparisons:
    friend bool operator<(const Version& v1, const Version& v2);
    friend bool operator<=(const Version& v1, const Version& v2);
    friend bool operator==(const Version& v1, const Version& v2);
    friend bool operator>=(const Version& v1, const Version& v2);
    friend bool operator>(const Version& v1, const Version& v2);
    friend bool operator!=(const Version& v1, const Version& v2);

    // Debugging description
    friend QDebug operator<<(QDebug debug, const Version& v);

    // Converts from a QVariant.
    // (This relies on us having registered Version with QVariant.)
    static Version fromVariant(const QVariant& variant);

    // Creates a Version from a version string.
    static Version fromString(const QString& version_string);

    // Makes an invalid Version (0.0.0).
    static Version makeInvalidVersion();

protected:
    // Sets to version 0.0.0 (invalid).
    void setInvalid();

    // Sets ourself from numbers.
    void setFromNumbers(unsigned int major, unsigned int minor,
                        unsigned int patch);
protected:
    bool m_valid;
    unsigned int m_major;
    unsigned int m_minor;
    unsigned int m_patch;
};

#pragma pop_macro("minor")
#pragma pop_macro("major")

Q_DECLARE_METATYPE(Version)


