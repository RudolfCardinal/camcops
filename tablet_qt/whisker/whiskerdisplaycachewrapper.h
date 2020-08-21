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
#include <QString>
class WhiskerManager;


class WhiskerDisplayCacheWrapper
{
    // Create this object on the stack in a scope block {}, so that it starts
    // Whisker's display caching (for a particular Whisker display document)
    // when it's created and stops caching when it's destroyed.

public:
    WhiskerDisplayCacheWrapper(WhiskerManager* manager,
                               const QString& doc);
    ~WhiskerDisplayCacheWrapper();
private:
    WhiskerManager* m_manager;
    QString m_doc;
};
