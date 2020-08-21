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


class FlagGuard
{
    // Class to set a boolean flag to true during the lifetime of this
    // object, then restores it again on destruction.
    // (Use e.g. for functions having multiple exit points where you want a
    // flag saying "I am in this function" to prevent infinite recursion.)
    //
    // See also ReentryDepthGuard.

public:
    FlagGuard(bool& flag);
    ~FlagGuard();
    bool previousState() const;
protected:
    bool& m_flag;
    bool m_previous_state;
};
