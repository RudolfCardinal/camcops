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

class CamcopsApp;

// http://doc.qt.io/qt-5/qtwidgets-itemviews-simpletreemodel-example.html


// Represents a diagnostic code in a tree structure.
// For example, in ICD-10, we have
//      code = "F20.0", description = "Paranoid schizophrenia".
// This sits in a tree structure (e.g. its parent is F20).

class DiagnosticCode
{
public:
    // Column 0 in a QTreeView gets the expand/collapse artwork, so we want
    // our "display" one there.
    static constexpr int COLUMN_FULLNAME = 0;
    static constexpr int COLUMN_CODE = 1;
    static constexpr int COLUMN_DESCRIPTION = 2;
    static constexpr int COLUMN_SELECTABLE = 3;
    static constexpr int N_COLUMNS = 4;
    // http://stackoverflow.com/questions/22318787/const-int-variable-cannot-appear-in-a-constant-expression

public:
    // Creates a code, referring to its parent (pass parent = nullptr for the
    // root of the tree).
    // Args:
    //  code: the code, e.g. "F20.0"
    //  description: description, as above
    //  parent: parent in the tree, or nullptr if this is the root
    //  depth: depth, usually 0 for the root
    //  selectable: may the user choose this node (or is it e.g. too generic)?
    //  show_code_in_full_name: incorporate the code as well as the description
    //      into the full name?
    DiagnosticCode(const QString& code, const QString& description,
                   DiagnosticCode* parent, int depth, bool selectable,
                   bool show_code_in_full_name = true);
    ~DiagnosticCode();

    // ------------------------------------------------------------------------
    // For our tree structure:
    // ------------------------------------------------------------------------

    // Adds a child.
    void appendChild(DiagnosticCode* child);

    // Returns the child at the specified position (or nullptr).
    DiagnosticCode* child(int row) const;

    // Returns the parent (or nullptr).
    DiagnosticCode* parent() const;

    // How many children?
    int childCount() const;

    // How many descendants?
    int descendantCount() const;

    // Which row number is this, in the parent's list of children?
    int row() const;

    // How many columns (for a QTreeView representation)?
    int columnCount() const;

    // Returns data for the specified column (see constants above; e.g.
    // column 1 is the code).
    QVariant data(int column) const;

    // ------------------------------------------------------------------------
    // Actual data
    // ------------------------------------------------------------------------

    // Returns the code
    QString code() const;

    // Returns the description
    QString description() const;

    // Returns the fullname (code + description, or just description).
    // For pick-lists.
    QString fullname() const;

    // ------------------------------------------------------------------------
    // More tree info:
    // ------------------------------------------------------------------------

    // Returns the depth.
    int depth() const;  // for convenience only

    // Is this node selectable?
    bool selectable() const;

    // Does this node have children?
    bool hasChildren() const;

protected:
    DiagnosticCode* m_parent;
    QVector<DiagnosticCode*> m_children;  // owns its children
    QString m_code;
    QString m_description;
    int m_depth;
    bool m_selectable;
    bool m_show_code_in_full_name;

public:
    // Debugging description
    friend QDebug operator<<(QDebug debug, const DiagnosticCode& dc);

    // Debugging description
    friend QTextStream& operator<<(QTextStream& stream, const DiagnosticCode& dc);
};
