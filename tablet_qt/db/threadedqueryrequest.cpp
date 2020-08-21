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

#include "threadedqueryrequest.h"

ThreadedQueryRequest::ThreadedQueryRequest(
        const SqlArgs& sqlargs,
        QueryResult::FetchMode fetch_mode,
        const bool store_column_names,
        const bool suppress_errors,
        const bool thread_abort_request_not_query) :
    sqlargs(sqlargs),
    fetch_mode(fetch_mode),
    store_column_names(store_column_names),
    suppress_errors(suppress_errors),
    thread_abort_request_not_query(thread_abort_request_not_query)
{
}


ThreadedQueryRequest::ThreadedQueryRequest() :
    fetch_mode(QueryResult::FetchMode::NoAnswer),
    store_column_names(false),
    suppress_errors(false),
    thread_abort_request_not_query(false)
{
}


QDebug operator<<(QDebug debug, const ThreadedQueryRequest& r)
{
    debug.nospace()
            << "ThreadedQueryRequest(sqlargs=" << r.sqlargs
            << ", fetch_mode=" << QueryResult::fetchModeDescription(r.fetch_mode)
            << ", store_column_names=" << r.store_column_names
            << ", suppress_errors=" << r.suppress_errors
            << ", thread_abort_request_not_query=" << r.thread_abort_request_not_query
            << ")";
    return debug;
}
