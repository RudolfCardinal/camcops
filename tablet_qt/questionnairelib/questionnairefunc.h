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
#include <QPair>
#include <QSharedPointer>
#include "common/aliases_camcops.h"

class Questionnaire;
class QuGridCell;
class Task;


namespace questionnairefunc {

// ============================================================================
// Grids
// ============================================================================

// Make default grids: convenience functions for grid creation.
//
// These functions take {string, element} pairs and make a two-column grid
// like:
//
//              label1      element1
//              label2      element2
//
// - The column span arguments determine the relative width of the left/right
//   columns.
// - label_alignment determines the alignment of text within the text widget
//   (e.g. left-justified, right-justified); see QuText::setAlignment().
// - The column alignment determines the alignment passed to QuGridCell();
//   q.v.

QuElement* defaultGridRawPointer(
    const QVector<GridRowDefinition>& deflist,
    int left_column_span = 1,
    int right_column_span = 1,
    Qt::Alignment label_alignment = Qt::AlignRight | Qt::AlignTop,
    Qt::Alignment left_column_alignment = 0,
    Qt::Alignment right_column_alignment = 0);
QuElementPtr defaultGrid(
    const QVector<GridRowDefinition>& deflist,
    int left_column_span = 1,
    int right_column_span = 1,
    Qt::Alignment label_alignment = Qt::AlignRight | Qt::AlignTop,
    Qt::Alignment left_column_alignment = 0,
    Qt::Alignment right_column_alignment = 0);
QuElementPtr defaultGrid(
    std::initializer_list<GridRowDefinition> defs,
    int left_column_span = 1,
    int right_column_span = 1,
    Qt::Alignment label_alignment = Qt::AlignRight | Qt::AlignTop,
    Qt::Alignment left_column_alignment = 0,
    Qt::Alignment right_column_alignment = 0);
QuElement* defaultGridRawPointer(
    std::initializer_list<GridRowDefinitionRawPtr> defs,
    int left_column_span = 1,
    int right_column_span = 1,
    Qt::Alignment label_alignment = Qt::AlignRight | Qt::AlignTop,
    Qt::Alignment left_column_alignment = 0,
    Qt::Alignment right_column_alignment = 0);

// ============================================================================
// Signals
// ============================================================================

// Connect Questionnaire::editStarted  -> Task::editStarted
//     and Questionnaire::editFinished -> Task::editFinished
void connectQuestionnaireToTask(Questionnaire* questionnaire, Task* task);

}  // namespace questionnairefunc

// If you don't specify an alignment, the default behaviour of QGridLayout is
// to stretch the widget to the cell, which is generally good.
// If you specify an alignment like Qt::AlignRight | Qt::AlignTop, the widget
// "floats" within its grid cell (and is then aligned as you ask). This can
// make the widget look too small.
// To align text right *within* its widget is a different question.
