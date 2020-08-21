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
#include "db/queryresult.h"
#include "db/sqlargs.h"

// Object representing an SQL query request.
// It gets passed in a queue to the database worker thread, for multithreaded
// database access.

struct ThreadedQueryRequest
{
public:
    ThreadedQueryRequest(const SqlArgs& sqlargs,
                         QueryResult::FetchMode fetch_mode,
                         bool store_column_names,
                         bool suppress_errors = false,
                         bool thread_abort_request_not_query = false);
    ThreadedQueryRequest();  // required to be in a QVector
public:
    // SQL and arguments
    SqlArgs sqlargs;

    // How to fetch, e.g. do we care about the answer?
    QueryResult::FetchMode fetch_mode;

    // Should the query result store column names?
    bool store_column_names;

    // Suppress errors (rather than shouting them to the debugging stream)?
    bool suppress_errors;

    // Special flag meaning "this is not a query; we are shutting down".
    bool thread_abort_request_not_query;

protected:
    // Debugging description.
    friend QDebug operator<<(QDebug debug, const ThreadedQueryRequest& r);
};
