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
#include <QAbstractItemModel>
#include <QString>
#include "diagnosticcode.h"

class CamcopsApp;


// Qt model representing a set of diagnostic codes.

class DiagnosticCodeSet : public QAbstractItemModel
{
    Q_OBJECT
public:
    DiagnosticCodeSet(CamcopsApp& app, const QString& setname,
                      const QString& title, QObject* parent = nullptr,
                      bool dummy_creation_no_xstrings = false);
    ~DiagnosticCodeSet() override;

    // Returns item data for the specified index/role (e.g. code; description)
    QVariant data(const QModelIndex& index, int role) const override;

    // Returns flags for the specified index/role; e.g. selectable?
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(
            int row, int column,
            const QModelIndex& parent_index = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent_index) const override;
    int columnCount(const QModelIndex& parent_index) const override;

    // How many codes descend from the root?
    int size() const;

    // Returns the code set title
    QString title() const;

    // Returns the index of the first DiagnosticCode whose code exactly
    // matches the supplied code.
    QModelIndex firstMatchCode(const QString& code) const;

    // Returns the set name (used for xstring)
    QString xstringTaskname() const;

protected:
    // Returns an xstring for this diagnostic code set
    QString xstring(const QString& stringname);

    // Adds a diagnostic code.
    DiagnosticCode* addCode(DiagnosticCode* parent,
                            const QString& code,
                            const QString& description,
                            bool selectable = true,
                            bool show_code_in_full_name = true);
protected:
    CamcopsApp& m_app;
    QString m_setname;  // for xstring
    QString m_title;  // cosmetic
    DiagnosticCode* m_root_item;
    bool m_dummy_creation_no_xstrings;  // don't use xstrings; for command-line debugging

public:
    friend QDebug operator<<(QDebug debug, const DiagnosticCodeSet& d);
    friend QTextStream& operator<<(QTextStream& stream, const DiagnosticCodeSet& d);
};
