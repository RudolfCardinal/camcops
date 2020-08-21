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
#include <type_traits>  // for std::is_base_of
#include <QSharedPointer>

// see taskfactory.h

class CamcopsApp;
class MenuWindow;


// ============================================================================
// MenuProxy<T>: encapsulates MenuWindow-derived classes, for MenuItem
// instances that say "go to another menu".
// Search for MAKE_MENU_MENU_ITEM.
// ============================================================================

class MenuProxyBase
{
public:
    MenuProxyBase();
    virtual ~MenuProxyBase();
    virtual MenuWindow* create(CamcopsApp& app) = 0;
};


template<class Derived> class MenuProxy : public MenuProxyBase
{
    static_assert(std::is_base_of<MenuWindow, Derived>::value,
                  "You can only use MenuWindow-derived classes here");
public:
    MenuProxy() {}
    virtual MenuWindow* create(CamcopsApp& app) override
    {
        return new Derived(app);
    }
};


using MenuProxyPtr = QSharedPointer<MenuProxyBase>;
