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
#include <QSharedPointer>

// UNUSED. For a C++ "clone a class instance" system.

template <typename Base, typename Derived>
class Cloneable : virtual public Base
{
public:
    virtual QSharedPointer<Base> clone() const override
    {
        return QSharedPointer<Base>(
            new Derived(static_cast<Derived const &>(*this))
        );
    }
    // Absence of virtual destructor here can lead to a crash,
    // even if Base has a virtual destructor? I'm not sure - either that,
    // or it was a QMediaPlayer destructor problem.
    virtual ~Cloneable()
    {}
};


// For multilevel inheritance, because you do not get automatic conversion
// of the QSharedPointer:
template <typename UltimateBase, typename ImmediateBase, typename Derived>
class MutilevelCloneable : virtual public ImmediateBase
{
public:
    virtual QSharedPointer<UltimateBase> clone() const override
    {
        return QSharedPointer<UltimateBase>(
            new Derived(static_cast<Derived const &>(*this))
        );
    }
    // (Absence of virtual destructor here can lead to a crash, presumably,
    // as above.)
    virtual ~MutilevelCloneable()
    {}
};
