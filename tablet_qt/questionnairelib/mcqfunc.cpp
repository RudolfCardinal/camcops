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

#include "mcqfunc.h"
#include <QDebug>
#include <QString>
#include "common/cssconst.h"
#include "common/uiconst.h"
#include "db/fieldref.h"
#include "lib/sizehelpers.h"
#include "widgets/booleanwidget.h"
#include "widgets/labelwordwrapwide.h"
#include "widgets/verticalline.h"
#include "namevalueoptions.h"

namespace mcqfunc {

// ============================================================================
// Alignment
// ============================================================================

// In grids, this is the title in cell (0, 0).
const Qt::Alignment title_text_align = Qt::AlignLeft | Qt::AlignTop;
const Qt::Alignment title_widget_align = Qt::AlignLeft | Qt::AlignTop;

// In grids, these are the response option descriptions in row 0.
const Qt::Alignment option_text_align = Qt::AlignHCenter | Qt::AlignBottom;
const Qt::Alignment option_widget_align = Qt::AlignHCenter | Qt::AlignBottom;
// If you don't apply a widget alignment, the label widget takes the entire
// cell -- which is fine for the most part (the text alignment does the rest)
// -- but not when you want a *bottom* alignment.

// In grids, these are the questions down the left-hand side
const Qt::Alignment question_text_align = Qt::AlignLeft | Qt::AlignVCenter;
const Qt::Alignment question_widget_align = Qt::AlignLeft | Qt::AlignVCenter;
// Don't do right align; disrupts natural reading flow.
// For small questions (vertically shorter than response widgets), vertical
// centre alignment looks best. For long ones, it doesn't matter (as the
// question likely fills its cell vertically in any case, being the tallest
// thing in its row).

// In grids, these are the things you touch to respond.
const Qt::Alignment response_widget_align = Qt::AlignHCenter | Qt::AlignTop;
// The vertical alignment is relevant when questions are anything but very
// short. Assuming the label is properly spaced (but see LabelWordWrapWide for
// probable Qt bug), top alignment looks good. With the bug, there is an
// argument for AlignVCenter.

// In grids, these are the stem questions over the array of responses, e.g.
// for QuMcqGridDouble.
const Qt::Alignment stem_text_align = Qt::AlignHCenter | Qt::AlignBottom;
const Qt::Alignment stem_widget_align = Qt::AlignHCenter | Qt::AlignBottom;

// ============================================================================
// Background to part of a QGridLayout
// ============================================================================
/*

- Layouts don't draw.
- They are unresponsive to CSS.
  http://doc.qt.io/qt-5.7/stylesheet-reference.html
- Use setSpacing() and related to set/remove spacing between widgets.
- So one possibility is:
    - setSpacing(0)
    - set background colour of options
    - add some sort of other spacing (e.g. padding) for the actual widgets

- Is another possibility:
    - a grey background as a background?

*/

void addVerticalLine(GridLayout* grid, const int col, const int n_rows)
{
    auto vline = new VerticalLine(uiconst::MCQGRID_VLINE_WIDTH);
    vline->setObjectName(cssconst::VLINE);
    grid->addWidget(vline, 0, col, n_rows, 1);
}


void addQuestion(GridLayout* grid, const int row, const QString& question,
                 bool bold)
{
    auto q = new LabelWordWrapWide(question);
    q->setAlignment(question_text_align);
    q->setObjectName(bold ? cssconst::QUESTION : cssconst::OPTION);
    grid->addWidget(q, row, 0, question_widget_align);
}


void addTitle(GridLayout* grid, const int row, const QString& title)
{
    if (!title.isEmpty()) {
        auto w = new LabelWordWrapWide(title);
        w->setAlignment(title_text_align);
        w->setObjectName(cssconst::TITLE);
        grid->addWidget(w, row, 0, title_widget_align);
    }
}


void addSubtitle(GridLayout* grid, const int row, const QString& subtitle)
{
    if (!subtitle.isEmpty()) {
        auto w = new LabelWordWrapWide(subtitle);
        w->setAlignment(title_text_align);
        w->setObjectName(cssconst::SUBTITLE);
        grid->addWidget(w, row, 0, title_widget_align);
    }
}


void addStem(GridLayout* grid, const int row, const int firstcol,
             const int colspan, const QString& stem)
{
    if (!stem.isEmpty()) {
        auto w = new LabelWordWrapWide(stem);
        w->setAlignment(stem_text_align);
        w->setObjectName(cssconst::STEM);
        grid->addWidget(w, row, firstcol, 1, colspan, stem_widget_align);
    }
}


void addOption(GridLayout* grid, const int row, const int col,
               const QString& option)
{
    auto w = new LabelWordWrapWide(option);
    w->setAlignment(option_text_align);
    w->setObjectName(cssconst::OPTION);
    grid->addWidget(w, row, col, option_widget_align);
}


void addOptionBackground(GridLayout* grid, const int row, const int firstcol,
                         const int ncols, const int nrows)
{
    auto bg = new QWidget();
    bg->setObjectName(cssconst::OPTION_BACKGROUND);
    grid->addWidget(bg, row, firstcol, nrows, ncols);
}


void addStripeBackground(GridLayout* grid, const int row, const int firstcol,
                         const int ncols, const int nrows)
{
    const bool even = row % 2 == 0;
    auto bg = new QWidget();
    bg->setObjectName(even ? cssconst::STRIPE_BACKGROUND_EVEN
                           : cssconst::STRIPE_BACKGROUND_ODD);
    grid->addWidget(bg, row, firstcol, nrows, ncols);
}


void setResponseWidgets(const NameValueOptions& options,
                        const QVector<QPointer<BooleanWidget>>& question_widgets,
                        const FieldRef* fieldref)
{
    if (!fieldref) {
        qWarning() << Q_FUNC_INFO << "Bad fieldref!";
        return;
    }
    const QVariant value = fieldref->value();
    const bool null = value.isNull();
    const int index = options.indexFromValue(value);
    if (!null && index == -1) {
        qWarning().nospace()
                << Q_FUNC_INFO << " - unknown value " << value
                << " (options are " << options << ")";
        // But we must PROCEED so that the widgets are shown.
    }
    for (int vi = 0; vi < question_widgets.size(); ++vi) {
        const QPointer<BooleanWidget>& w = question_widgets.at(vi);
        if (!w) {
            qCritical() << Q_FUNC_INFO << "- defunct pointer!";
            continue;
        }
        if (vi == index) {
            w->setState(BooleanWidget::State::True);
        } else if (index == -1) {  // null but not selected
            w->setState(fieldref->mandatory()
                        ? BooleanWidget::State::NullRequired
                        : BooleanWidget::State::Null);
        } else {
            w->setState(BooleanWidget::State::False);
        }
    }
}


void toggleBooleanField(FieldRef* fieldref, const bool allow_unset)
{
    // Used by "clicked" receivers.
    if (!fieldref) {
        qWarning() << Q_FUNC_INFO << "bad pointer! Ignored";
        return;
    }
    const QVariant value = fieldref->value();
    QVariant newvalue;
    if (value.isNull()) {  // NULL -> true
        newvalue = true;
    } else if (value.toBool()) {  // true -> false
        newvalue = false;
    } else {  // false -> either NULL or true
        newvalue = allow_unset ? QVariant() : true;
    }
    fieldref->setValue(newvalue);  // Will trigger valueChanged
}


}  // namespace mcqfunc
