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
#include <QVector>
#include <QVariant>
class QJsonArray;
class QJsonObject;
class QSqlQuery;


// Represents the results of an SQL query.

class QueryResult
{
public:
    // How to fetch data
    enum class FetchMode {
        NoAnswer,  // do not store a reply
        NoFetch,  // store a reply with fact of success/failure, but no data
        FetchFirst,  // fetch first row
        FetchAll  // fetch everything
    };

public:
    // Normal constructor
    // - Note that later access by column name requires store_column_names to
    //   be true.
    QueryResult(QSqlQuery& query, bool success,
                FetchMode fetch_mode, bool store_column_names = false);

    // Default constructor (required to put this object in a QVector)
    QueryResult();

    // Did we succeed?
    bool succeeded() const;

    // How many columns?
    int nCols() const;

    // How many rows?
    int nRows() const;

    // Are there zero rows (or columns)?
    bool isEmpty() const;

    // Return all column names
    QStringList columnNames() const;

    // Retrieve a row (efficient)
    QVector<QVariant> row(int row) const;

    // Retrieve a whole column (inefficient)
    QVector<QVariant> col(int col) const;

    // Return the value at a specified row/column
    QVariant at(int row, int col) const;

    // Return the value at a specified row and for a named column
    // (requires store_column_names = true in constructor).
    QVariant at(int row, const QString& colname) const;

    // Returns the first column of the first row.
    QVariant firstValue() const;

    // Returns a whole column as a list of integers.
    QVector<int> columnAsIntList(int col) const;

    // Returns the first column as a list of integers.
    QVector<int> firstColumnAsIntList() const;

    // Returns a whole column as a list of strings.
    QStringList columnAsStringList(int col) const;

    // Returns the first column as a list of strings.
    QStringList firstColumnAsStringList() const;

    // Returns the last insert ID; if the query was an INSERT, this will be
    // the new PK.
    QVariant lastInsertId() const;

    // Returns a CSV header for this result set.
    QString csvHeader(const char sep = ',') const;

    // Returns a CSV row for this result set.
    QString csvRow(int row, const char sep = ',') const;

    // Returns CSV for the whole result set.
    QString csv(const char sep = ',', const char linesep = '\n') const;

    // Describes a FetchMode.
    static QString fetchModeDescription(FetchMode fetch_mode);

    // Returns the result set as JSON.
    QJsonArray jsonRows() const;

    // Returns one row of the result set as JSON.
    QJsonObject jsonRow(int row) const;

protected:
    // Requires that column names were saved at construction, or stops the
    // whole app.
    void requireColumnNames() const;

protected:
    // Did the query succeed?
    bool m_success;

    // How many columns?
    int m_n_cols;  // cached

    // How many rows?
    int m_n_rows;  // cached

    // Column names, if saved
    QStringList m_column_names;

    // Raw data
    QVector<QVector<QVariant>> m_data;

    // Last INSERT ID, from the query
    QVariant m_last_insert_id;

public:
    // Debugging description
    friend QDebug operator<<(QDebug debug, const QueryResult& qr);
};
