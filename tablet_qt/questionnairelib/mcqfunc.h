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
#include <QList>
#include <QPointer>
#include "layouts/layouts.h"

class BooleanWidget;
class FieldRef;
class NameValueOptions;
class QString;


namespace mcqfunc {

// ============================================================================
// Assistance functions for questionnaire items.
// ============================================================================

// Add a vertical line to a grid layout (in column col, for its full vertical
// extent n_rows).
void addVerticalLine(GridLayout* grid, int col, int n_rows);

// Adds text to a grid in our default question style, in column 0.
void addQuestion(GridLayout* grid, int row, const QString& question,
                 bool bold = true);

// Adds text to a grid in our default title style, in column 0.
void addTitle(GridLayout* grid, int row, const QString& title);

// Adds text to a grid in our default subtitle style, in column 0.
void addSubtitle(GridLayout* grid, int row, const QString& subtitle);

// Adds text to a grid in our default question stem style.
void addStem(GridLayout* grid, int row, int firstcol, int colspan,
             const QString& stem);

// Adds text to a grid in our default option style.
void addOption(GridLayout* grid, int row, int col, const QString& option);

// Add shading to a grid in our default option background style.
void addOptionBackground(GridLayout* grid, int row,
                         int firstcol, int ncols, int nrows = 1);

// Add shading to a grid in our default stripe style (which alternates between
// odd and even rows).
void addStripeBackground(GridLayout* grid, int row,
                         int firstcol, int ncols, int nrows = 1);

// Retrieves a value from fieldref. Maps it to an index in options.
// Sets each of the widgets in question_widget to set/unset (zero to one set,
// the rest unset) according to that index.
void setResponseWidgets(
        const NameValueOptions& options,
        const QVector<QPointer<BooleanWidget>>& question_widgets,
        const FieldRef* fieldref);

// Toggles the boolean state of the value in fieldref.
// Used by "clicked" receivers.
// If allow_unset is true, uses a three-state system including NULL.
void toggleBooleanField(FieldRef* fieldref, bool allow_unset = false);

// ============================================================================
// Alignment constants for different standard element styles
// ============================================================================

extern const Qt::Alignment question_text_align;
extern const Qt::Alignment question_widget_align;

extern const Qt::Alignment title_text_align;
extern const Qt::Alignment title_widget_align;

extern const Qt::Alignment option_text_align;
extern const Qt::Alignment option_widget_align;

extern const Qt::Alignment response_widget_align;

extern const Qt::Alignment stem_text_align;
extern const Qt::Alignment stem_widget_align;

}  // namespace mcqfunc
