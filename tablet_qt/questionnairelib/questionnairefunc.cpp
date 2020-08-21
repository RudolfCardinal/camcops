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

#include "questionnairefunc.h"
#include <QObject>
#include "lib/uifunc.h"
#include "questionnairelib/qugridcontainer.h"
#include "questionnairelib/quelement.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qugridcell.h"
#include "questionnairelib/qutext.h"
#include "tasklib/task.h"

namespace questionnairefunc {


// ============================================================================
// Grids
// ============================================================================

QuElement* defaultGridRawPointer(const QVector<GridRowDefinition>& deflist,
                                 const int left_column_span,
                                 const int right_column_span,
                                 const Qt::Alignment label_alignment,
                                 const Qt::Alignment left_column_alignment,
                                 const Qt::Alignment right_column_alignment)
{
    QVector<QuGridCell> cells;
    int row = 0;
    const int left_col = 0;
    const int right_col = 1;
    const int row_span = 1;
    const int col_span = 1;
    for (GridRowDefinition def : deflist) {
        const QString text = def.first;
        QuElementPtr label_element = QuElementPtr(
                    (new QuText(text))->setTextAlignment(label_alignment));
        const QuGridCell label_cell(label_element, row, left_col,
                                    row_span, col_span,
                                    left_column_alignment);
        QuElementPtr main_element = def.second;
        const QuGridCell main_cell(main_element, row, right_col,
                                   row_span, col_span,
                                   right_column_alignment);
        cells.append(label_cell);
        cells.append(main_cell);
        ++row;
    }
    auto grid = new QuGridContainer(cells);
    grid->setColumnStretch(left_col, left_column_span);
    grid->setColumnStretch(right_col, right_column_span);
    return grid;
}


QuElementPtr defaultGrid(const QVector<GridRowDefinition>& deflist,
                         const int left_column_span,
                         const int right_column_span,
                         const Qt::Alignment label_alignment,
                         const Qt::Alignment left_column_alignment,
                         const Qt::Alignment right_column_alignment)
{
    return QuElementPtr(defaultGridRawPointer(
                            deflist, left_column_span, right_column_span,
                            label_alignment,
                            left_column_alignment, right_column_alignment));
}


QuElementPtr defaultGrid(std::initializer_list<GridRowDefinition> defs,
                         const int left_column_span,
                         const int right_column_span,
                         const Qt::Alignment label_alignment,
                         const Qt::Alignment left_column_alignment,
                         const Qt::Alignment right_column_alignment)
{
    const QVector<GridRowDefinition> deflist(defs);
    return defaultGrid(deflist, left_column_span, right_column_span,
                       label_alignment,
                       left_column_alignment, right_column_alignment);
}


QuElement* defaultGridRawPointer(
        std::initializer_list<GridRowDefinitionRawPtr> defs,
        const int left_column_span,
        const int right_column_span,
        const Qt::Alignment label_alignment,
        const Qt::Alignment left_column_alignment,
        const Qt::Alignment right_column_alignment)
{
    QVector<GridRowDefinition> deflist;
    for (auto rawptrdef : defs) {
        // rawptrdef will be of type GridRowDefinitionRawPtr
        //
        GridRowDefinition sharedptrdef(rawptrdef.first,
                                       QuElementPtr(rawptrdef.second));
        deflist.append(sharedptrdef);
    }
    return defaultGridRawPointer(
                deflist, left_column_span, right_column_span,
                label_alignment,
                left_column_alignment, right_column_alignment);
}


// ============================================================================
// Signals
// ============================================================================

void connectQuestionnaireToTask(Questionnaire* questionnaire, Task* task)
{
    if (!questionnaire || !task) {
        return;
    }
    QObject::connect(questionnaire, &Questionnaire::editStarted,
                     task, &Task::onEditStarted);
    QObject::connect(questionnaire, &Questionnaire::editFinished,
                     task, &Task::onEditFinished);
}


}  // namespace questionnairefunc
