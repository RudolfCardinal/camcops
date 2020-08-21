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

// #define DEBUG_GRID_CREATION

#include "qugridcontainer.h"
#include <QDebug>
#include <QWidget>
#include "layouts/layouts.h"
#include "lib/layoutdumper.h"
#include "lib/sizehelpers.h"
#include "questionnairelib/questionnaire.h"
#include "widgets/basewidget.h"

#ifdef DEBUG_GRID_CREATION
#include "common/cssconst.h"
#endif


/*

MAKING COLUMN WIDTHS EQUAL

-   Prototypical problem with QGridLayout:

    widget1: fixed          widget2: expanding
    setColumnStretch(1)     setColumnStretch(1)
    |--------------------|  |---------------------------------------------|

    ... same stretch, different widths.
    I think that QGridLayout stretches any *spare* in proportion to
    setColumnStretch.

    http://doc.qt.io/qt-5.7/qgridlayout.html#details

    ... "If you want two columns to have the same width, you must set their
    minimum widths and stretch factors to be the same yourself. You do this
    using setColumnMinimumWidth() and setColumnStretch()."

What does not work properly:

-   widget->setMinimumWidth(1);
    grid->setColumnStretch(column, 1);

-   Encapsulating widget/layout so we can add a stretch, in case (for
    example) our left-hand cells have widgets with horizontal size
    policy Maximum, and the right-hand cells have Expanding; then the
    widgets' policies override our desired grid setColumnStretch()
    parameters.
        auto cell_widget = new QWidget();
        auto cell_layout = new QHBoxLayout();
        cell_layout->setContentsMargins(UiConst::NO_MARGINS);
        cell_widget->setLayout(cell_layout);
        QPointer<QWidget> w = e->widget(questionnaire);
        cell_layout->addWidget(w);
        cell_layout->addStretch();
        // ... then add the layout to the grid

-   Setting widget's size policy like:

        QSizePolicy sp = w->sizePolicy();
        sp.setHorizontalStretch(1);
        w->setSizePolicy(sp);

    Compare https://stackoverflow.com/questions/27808440/how-to-make-qt-grid-layout-auto-size-column-widths
    ... not sure you can just modify the returned size policy directly, though!
    sizePolicy() returns QSizePolicy, not a reference or pointer.

-   Notes:

    -   QGridLayoutPrivate::addData uses the widget's
        horizontalStretch() [via QQGridBox::hStretch()] only if no grid column
        stretch is applied.

-   What does work:

    // force widget's horizontal size policy to expanding
    grid->setColumnMinimumWidth(1);
    grid->setColumnStretch(1);

*/


QuGridContainer::QuGridContainer() :
    QuGridContainer(QVector<QuGridCell>())  // delegating constructor
{
}


QuGridContainer::QuGridContainer(const QVector<QuGridCell>& cells) :
    m_cells(cells),
    m_expand(true),
    m_fixed_grid(true)

{
}


QuGridContainer::QuGridContainer(std::initializer_list<QuGridCell> cells) :
    QuGridContainer(QVector<QuGridCell>(cells))  // delegating constructor
{
}


template<typename ElementList>
void constructFromElementList(const int n_columns,
                              ElementList elements,
                              const bool override_element_alignment,
                              QVector<QuGridCell>& cells)
{
    // Better than #define!
    int column = 0;
    int row = 0;
    for (auto e : (elements)) {
        QuGridCell cell(e, row, column, 1, 1, Qt::AlignLeft | Qt::AlignTop,
                        override_element_alignment);
        column = (column + 1) % n_columns;
        if (column == 0) {
            ++row;
        }
        cells.append(cell);
    }
}


QuGridContainer::QuGridContainer(const int n_columns,
                                 const QVector<QuElementPtr>& elements,
                                 const bool override_element_alignment) :
    QuGridContainer(QVector<QuGridCell>())  // delegating constructor
{
    constructFromElementList(n_columns, elements, override_element_alignment,
                             m_cells);
}


QuGridContainer::QuGridContainer(const int n_columns,
                                 const QVector<QuElement*>& elements,
                                 const bool override_element_alignment) :
    QuGridContainer(QVector<QuGridCell>())  // delegating constructor
{
    constructFromElementList(n_columns, elements, override_element_alignment,
                             m_cells);
}


QuGridContainer::QuGridContainer(const int n_columns,
                                 std::initializer_list<QuElementPtr> elements,
                                 const bool override_element_alignment) :
    QuGridContainer(QVector<QuGridCell>())  // delegating constructor
{
    constructFromElementList(n_columns, elements, override_element_alignment,
                             m_cells);
}


QuGridContainer::QuGridContainer(const int n_columns,
                                 std::initializer_list<QuElement*> elements,
                                 const bool override_element_alignment) :
    QuGridContainer(QVector<QuGridCell>())  // delegating constructor
{
    constructFromElementList(n_columns, elements, override_element_alignment,
                             m_cells);
}


QuGridContainer* QuGridContainer::addCell(const QuGridCell& cell)
{
    m_cells.append(cell);
    return this;
}


QuGridContainer* QuGridContainer::setColumnStretch(
        const int column, const int stretch)
{
    m_column_stretch[column] = stretch;
    return this;
}


QuGridContainer* QuGridContainer::setFixedGrid(const bool fixed_grid)
{
    m_fixed_grid = fixed_grid;
    return this;
}


QuGridContainer* QuGridContainer::setExpandHorizontally(const bool expand)
{
    m_expand = expand;
    return this;
}


QPointer<QWidget> QuGridContainer::makeWidget(Questionnaire* questionnaire)
{
    // m_expand: using preferredFixedHFWPolicy() doesn't prevent it expanding
    // right, even if the contained widgets are small.
    // Instead, use a horizontal container with a stretch. That works.

    QPointer<QWidget> widget = new BaseWidget();
    widget->setSizePolicy(sizehelpers::expandingFixedHFWPolicy());

#ifdef DEBUG_GRID_CREATION
    qDebug() << Q_FUNC_INFO;
    qDebug() << "... m_fixed_grid =" << m_fixed_grid;
    widget->setObjectName(CssConst::DEBUG_GREEN);
#endif
    auto grid = new GridLayout();
    grid->setContentsMargins(uiconst::NO_MARGINS);
    if (m_expand) {
        widget->setLayout(grid);
    } else {
        auto hbox = new HBoxLayout();
        hbox->addLayout(grid);
        hbox->addStretch();
        widget->setLayout(hbox);
    }
    for (const QuGridCell& c : m_cells) {
        QuElementPtr e = c.element;
        QPointer<QWidget> w = e->widget(questionnaire);
        if (!w) {
            qWarning() << Q_FUNC_INFO << "Element failed to create a widget!";
            continue;
        }

#ifdef DEBUG_GRID_CREATION
        w->setObjectName(CssConst::DEBUG_RED);
        qDebug() << "... cell:" << c;
#endif
        if (m_fixed_grid) {
            // Set widget to horizontal expanding
            QSizePolicy sp = w->sizePolicy();
            sp.setHorizontalPolicy(QSizePolicy::Expanding);
            w->setSizePolicy(sp);
#ifdef DEBUG_GRID_CREATION
            qDebug().noquote()
                    << "... forcing widget horizontal size policy to Expanding";
#endif

            // Set column minimum width, and column stretch (may be modified
            // below).
            grid->setColumnMinimumWidth(c.column, 1);
            grid->setColumnStretch(c.column, 1);
        }
#ifdef DEBUG_GRID_CREATION
        {
            QSizePolicy sp = w->sizePolicy();
            qDebug().noquote() << "... widget sizePolicy():"
                               << LayoutDumper::toString(sp);
        }
#endif
        const auto alignment = c.override_element_alignment
                ? c.alignment
                : e->getWidgetAlignment();
        grid->addWidget(w, c.row, c.column,
                        c.row_span, c.column_span, alignment);
    }
    QMapIterator<int, int> it(m_column_stretch);
    while (it.hasNext()) {
        it.next();
        const int column = it.key();
        const int stretch = it.value();
#ifdef DEBUG_GRID_CREATION
        qDebug().nospace() << "... setColumnStretch(" << column
                           << "," << stretch << ")";
#endif
        grid->setColumnStretch(column, stretch);
    }

    return widget;
}


QVector<QuElementPtr> QuGridContainer::subelements() const
{
    QVector<QuElementPtr> elements;
    for (auto cell : m_cells) {
        elements.append(cell.element);
    }
    return elements;
}


QDebug operator<<(QDebug debug, const QuGridContainer& grid)
{
    debug.nospace()
            << "QuGridContainer(m_cells=" << grid.m_cells
            << ", m_column_stretch=" << grid.m_column_stretch
            << ", m_expand=" << grid.m_expand
            << ", m_fixed_grid=" << grid.m_fixed_grid;
    return debug;
}
