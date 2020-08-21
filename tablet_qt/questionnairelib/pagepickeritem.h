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


class PagePickerItem
{
    // An option presented by a PagePickerDialog.
    // Represents the choice of a single page from those available in a
    // Questionnaire.

public:

    // How should the page be shown/displayed?
    // Determines the icon shown and whether the user can select it.
    enum class PagePickerItemType {
        CompleteSelectable,  // data complete, can jump to it
        IncompleteSelectable,  // data incomplete, can jump to it
        BlockedByPrevious,  // can't select it; data incomplete in previous pages
    };

    // Default constructor, so it can live in a QVector
    PagePickerItem();

    // Usual constructor
    PagePickerItem(const QString& text, int page_number,
                   PagePickerItemType type);

    // Returns the text (e.g. page title)
    QString text() const;

    // Returns the page number
    int pageNumber() const;

    // Returns the type, as above.
    PagePickerItemType type() const;

    // Can the user select (jump to) this page?
    bool selectable() const;

    // Returns the CamCOPS icon filename to display for this page's type.
    QString iconFilename() const;

protected:
    QString m_text;
    int m_page_number;
    PagePickerItemType m_type;
};
