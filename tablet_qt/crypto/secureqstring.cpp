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

#include "secureqstring.h"
// #include "secureqbytearray.h"

/*

SecureQString::SecureQString()
{
}


SecureQString::SecureQString(const QChar* unicode, int size) :
    QString(unicode, size)
{
}


SecureQString::SecureQString(QChar ch) :
    QString(ch)
{
}


SecureQString::SecureQString(int size, QChar ch) :
    QString(size, ch)
{
}


SecureQString::SecureQString(QLatin1String str) :
    QString(str)
{
}


SecureQString::SecureQString(const QString& other) :
    QString(other)
{
}


SecureQString::SecureQString(const SecureQString& other) :
    QString(other)
{
}


SecureQString::SecureQString(QString&& other) :
    QString(other)
{
}


SecureQString::SecureQString(SecureQString&& other) :
    QString(other)
{
}


SecureQString::SecureQString(const char* str) :
    QString(str)
{
}


SecureQString::SecureQString(const QByteArray& ba) :
    QString(ba)
{
}


SecureQString::SecureQString(const SecureQByteArray& ba) :
    QString(ba)
{
}

SecureQString::~SecureQString()
{
    // Wipe it with zeros.
    for (int i = 0; i < length(); ++i) {
        (*this)[i] = 0;
    }
}

SecureQString& SecureQString::operator=(const SecureQString& other)
{
    // https://en.wikipedia.org/wiki/Rule_of_three_(C%2B%2B_programming)
    SecureQString tmp(other);         // re-use copy-constructor
    *this = std::move(tmp); // re-use move-assignment
    return *this;
}

*/
