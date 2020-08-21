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

#include "diagnosticcode.h"
#include "core/camcopsapp.h"
#include "diagnosis/diagnosticcodeset.h"


DiagnosticCode::DiagnosticCode(const QString& code, const QString& description,
                               DiagnosticCode* parent,
                               const int depth,
                               const bool selectable,
                               const bool show_code_in_full_name) :
    m_parent(parent),
    m_code(code),
    m_description(description),
    m_depth(depth),
    m_selectable(selectable),
    m_show_code_in_full_name(show_code_in_full_name)
{
}

DiagnosticCode::~DiagnosticCode()
{
    qDeleteAll(m_children);
}


void DiagnosticCode::appendChild(DiagnosticCode* child)
{
    m_children.append(child);
}


DiagnosticCode* DiagnosticCode::child(const int row) const
{
    return m_children.value(row);  // will give nullptr if row out of bounds
}


DiagnosticCode* DiagnosticCode::parent() const
{
    return m_parent;
}


int DiagnosticCode::childCount() const
{
    return m_children.size();  // count() or size()
}


int DiagnosticCode::descendantCount() const
{
    int n = 0;
    for (auto c : m_children) {
        n += 1 + c->descendantCount();
    }
    return n;
}


int DiagnosticCode::columnCount() const
{
    return N_COLUMNS;
}


int DiagnosticCode::row() const
{
    if (m_parent) {
        return m_parent->m_children.indexOf(
                    const_cast<DiagnosticCode*>(this));
    }
    return 0;
}


QVariant DiagnosticCode::data(const int column) const
{
    // qDebug() << Q_FUNC_INFO << "column" << column;
    switch (column) {
    case COLUMN_CODE:
        return code();
    case COLUMN_DESCRIPTION:
        return description();
    case COLUMN_FULLNAME:
        return fullname();
    case COLUMN_SELECTABLE:
        return selectable();
    default:
        Q_ASSERT(false);
        return code();
    }
}


QString DiagnosticCode::code() const
{
    return m_code;
}


int DiagnosticCode::depth() const
{
    return m_depth;
}


bool DiagnosticCode::selectable() const
{
    return m_selectable;
}


bool DiagnosticCode::hasChildren() const
{
    return childCount() > 0;
}


QString DiagnosticCode::description() const
{
    return m_description;
}


QString DiagnosticCode::fullname() const
{
    if (m_show_code_in_full_name) {
        return QString("%1: %2").arg(m_code, m_description);
    }
    return m_description;
}


QDebug operator<<(QDebug debug, const DiagnosticCode& dc)
{
    for (int i = 0; i < dc.depth(); ++i) {
        debug.nospace() << "... ";
    }
    debug.nospace()
            << dc.code()
            << " (" << dc.description()
            << ") [depth " << dc.depth()
            << "] " << (dc.selectable() ? "selectable" : "not selectable")
            << "\n";
    for (auto c : dc.m_children) {
        debug << *c;
    }
    return debug;
}


QTextStream& operator<<(QTextStream& stream, const DiagnosticCode& dc)
{
    if (dc.selectable()) {
        stream << dc.code() << endl;
    }
    for (auto c : dc.m_children) {
        stream << *c;
    }
    return stream;
}
