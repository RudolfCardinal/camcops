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

#include "diagnosticcodeset.h"
#include "core/camcopsapp.h"

const QString BAD_STRING("[bad_string]");


DiagnosticCodeSet::DiagnosticCodeSet(CamcopsApp& app,
                                     const QString& setname,
                                     const QString& title,
                                     QObject* parent,
                                     bool dummy_creation_no_xstrings) :
    QAbstractItemModel(parent),
    m_app(app),
    m_setname(setname),
    m_title(title),
    m_root_item(nullptr),
    m_dummy_creation_no_xstrings(dummy_creation_no_xstrings)
{
    m_root_item = new DiagnosticCode("", "", nullptr, 0, false, false);
}


DiagnosticCodeSet::~DiagnosticCodeSet()
{
    delete m_root_item;
}


QModelIndex DiagnosticCodeSet::index(const int row, const int column,
                                     const QModelIndex& parent_index) const
{
    if (!hasIndex(row, column, parent_index)) {
        return {};
    }

    DiagnosticCode* parent_item;
    if (!parent_index.isValid()) {
        parent_item = m_root_item;
    } else {
        parent_item = static_cast<DiagnosticCode*>(
                    parent_index.internalPointer());
    }

    DiagnosticCode* child_item = parent_item->child(row);
    if (child_item) {
        return createIndex(row, column, child_item);
    }
    return {};
}


QModelIndex DiagnosticCodeSet::parent(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return {};
    }

    auto child_item = static_cast<DiagnosticCode*>(index.internalPointer());
    DiagnosticCode* parent_item = child_item->parent();

    if (!parent_item || parent_item == m_root_item) {
        return QModelIndex();
    }

    return createIndex(parent_item->row(), 0, parent_item);
}


int DiagnosticCodeSet::rowCount(const QModelIndex& parent_index) const
{
    DiagnosticCode* parent_item;
    if (parent_index.column() > 0) {
        return 0;
    }

    if (!parent_index.isValid()) {
        parent_item = m_root_item;
    } else {
        parent_item = static_cast<DiagnosticCode*>(
                    parent_index.internalPointer());
    }

    return parent_item->childCount();
}


int DiagnosticCodeSet::columnCount(const QModelIndex& parent_index) const
{
    if (parent_index.isValid()) {
        return static_cast<DiagnosticCode*>(
                    parent_index.internalPointer())->columnCount();
    }
    return m_root_item->columnCount();
}


QVariant DiagnosticCodeSet::data(const QModelIndex &index,
                                 const int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    auto item = static_cast<DiagnosticCode*>(index.internalPointer());

    return item->data(index.column());
}


Qt::ItemFlags DiagnosticCodeSet::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return 0;
    }
    // return QAbstractItemModel::flags(index);
    auto item = static_cast<DiagnosticCode*>(index.internalPointer());
    Qt::ItemFlags flags = Qt::ItemIsEnabled;
    if (item->selectable()) {
        flags |= Qt::ItemIsSelectable;
    }
    return flags;
}


QVariant DiagnosticCodeSet::headerData(const int section,
                                       const Qt::Orientation orientation,
                                       const int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return m_root_item->data(section);
    }

    return QVariant();
}


QString DiagnosticCodeSet::title() const
{
    return m_title;
}


QModelIndex DiagnosticCodeSet::firstMatchCode(const QString& code) const
{
    // http://www.qtcentre.org/threads/15572-How-can-I-traverse-all-of-the-items-in-a-QStandardItemModel
    // Walk the tree:

    QStack<DiagnosticCode*> stack;
    stack.push(m_root_item);
    while (!stack.isEmpty()) {
        DiagnosticCode* item = stack.pop();

        // Do something with item:
        if (item->code() == code) {
            return createIndex(item->row(), 0, item);
        }

        for (int i = item->childCount() - 1; i >= 0; --i) {
            stack.push(item->child(i));
        }
    }
    return QModelIndex();
}


QDebug operator<<(QDebug debug, const DiagnosticCodeSet& d)
{
    debug << "DiagnosticCodeSet: m_setname" << d.m_setname
          << "m_title" << d.m_title << "\n";
    if (d.m_root_item) {
        debug << *d.m_root_item;  // will recurse
    } else {
        debug << "... no items";
    }
    debug << "... end\n";
    return debug;
}


QTextStream& operator<<(QTextStream& stream, const DiagnosticCodeSet& d)
{
    // For output to stdout from the console app.
    if (d.m_root_item) {
        stream << *d.m_root_item;  // will recurse
    }
    return stream;
}


QString DiagnosticCodeSet::xstringTaskname() const
{
    return m_setname;
}


QString DiagnosticCodeSet::xstring(const QString& stringname)
{
    if (m_dummy_creation_no_xstrings) {
        return "";
    }
    return m_app.xstring(m_setname, stringname);
}


int DiagnosticCodeSet::size() const
{
    return m_root_item->descendantCount();
}


DiagnosticCode* DiagnosticCodeSet::addCode(
        DiagnosticCode* parent, const QString& code,
        const QString& description, const bool selectable,
        const bool show_code_in_full_name)
{
    if (parent == nullptr) {
        parent = m_root_item;
    }
    auto c = new DiagnosticCode(
                code, description,
                parent, parent->depth() + 1, selectable,
                show_code_in_full_name);
    parent->appendChild(c);
    return c;
}
