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

#include "qumcqgrid.h"
#include "common/cssconst.h"
#include "lib/uifunc.h"
#include "questionnairelib/mcqfunc.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcqgridsignaller.h"
#include "widgets/basewidget.h"
#include "widgets/booleanwidget.h"
#include "widgets/labelwordwrapwide.h"


QuMcqGrid::QuMcqGrid(const QVector<QuestionWithOneField>& question_field_pairs,
                     const NameValueOptions& options) :
    m_question_field_pairs(question_field_pairs),
    m_options(options),
    m_question_width(-1),
    m_expand(false),
    m_stripy(true),
    m_show_title(true),
    m_questions_bold(true)
{
    m_options.validateOrDie();
    // each QuestionWithOneField will have asserted on construction

    for (int qi = 0; qi < m_question_field_pairs.size(); ++qi) {
        FieldRefPtr fieldref = m_question_field_pairs.at(qi).fieldref();
        // DANGEROUS OBJECT LIFESPAN SIGNAL: do not use std::bind
        auto sig = new QuMcqGridSignaller(this, qi);
        m_signallers.append(sig);
        connect(fieldref.data(), &FieldRef::valueChanged,
                sig, &QuMcqGridSignaller::valueOrMandatoryChanged);
        connect(fieldref.data(), &FieldRef::mandatoryChanged,
                sig, &QuMcqGridSignaller::valueOrMandatoryChanged);
    }
}


QuMcqGrid::~QuMcqGrid()
{
    while (!m_signallers.isEmpty()) {
        delete m_signallers.takeAt(0);
    }
}


QuMcqGrid* QuMcqGrid::setWidth(const int question_width,
                               const QVector<int>& option_widths)
{
    if (option_widths.size() != m_options.size()) {
        qWarning() << Q_FUNC_INFO << "Bad option_widths; command ignored";
        return this;
    }
    m_question_width = question_width;
    m_option_widths = option_widths;
    return this;
}


QuMcqGrid* QuMcqGrid::setTitle(const QString &title)
{
    m_title = title;
    return this;
}


QuMcqGrid* QuMcqGrid::setSubtitles(const QVector<McqGridSubtitle>& subtitles)
{
    m_subtitles = subtitles;
    return this;
}


QuMcqGrid* QuMcqGrid::setExpand(const bool expand)
{
    m_expand = expand;
    return this;
}


QuMcqGrid* QuMcqGrid::setStripy(const bool stripy)
{
    m_stripy = stripy;
    return this;
}


QuMcqGrid* QuMcqGrid::showTitle(const bool show_title)
{
    m_show_title = show_title;
    return this;
}


QuMcqGrid* QuMcqGrid::setQuestionsBold(bool bold)
{
    m_questions_bold = bold;
    return this;
}


QuMcqGrid* QuMcqGrid::setAlternateNameValueOptions(
        const QVector<int>& question_indexes,
        const NameValueOptions& options)
{
    if (options.size() != m_options.size()) {
        uifunc::stopApp("NameValueOptions size mismatch to "
                        "QuMcqGrid::setAlternateNameValueOptions");
    }
    for (const int idx : question_indexes) {
        m_alternate_options[idx] = options;
    }
    return this;
}



void QuMcqGrid::setFromFields()
{
    for (int qi = 0; qi < m_question_field_pairs.size(); ++qi) {
        fieldValueOrMandatoryChanged(qi, m_question_field_pairs.at(qi).fieldref().data());
    }
}


int QuMcqGrid::colnum(const int value_index) const
{
    return 2 + value_index;
}


void QuMcqGrid::addOptions(GridLayout* grid, const int row)
{
    for (int i = 0; i < m_options.size(); ++i) {
        mcqfunc::addOption(grid, row, colnum(i), m_options.at(i).name());
    }
}


QPointer<QWidget> QuMcqGrid::makeWidget(Questionnaire* questionnaire)
{
    const bool read_only = questionnaire->readOnly();
    m_widgets.clear();

    /*
    - Labels, by default, have their text contents left-aligned and vertically
      centred. Use label->setAlignment().
      http://doc.qt.io/qt-5/qlabel.html#alignment-prop
    - That's fine for everything except headers, which we'd like bottom
      alignment for.
    - And top alignment for the main title.
    */

    auto grid = new GridLayout();
    grid->setContentsMargins(uiconst::NO_MARGINS);
    grid->setHorizontalSpacing(uiconst::MCQGRID_HSPACING);
    grid->setVerticalSpacing(uiconst::MCQGRID_VSPACING);

    const int n_subtitles = m_subtitles.size();
    const int n_options = m_options.size();
    const int n_rows = 1 + n_subtitles + m_question_field_pairs.size();
    const int n_cols = m_options.size() + 2;
    const Qt::Alignment response_align = mcqfunc::response_widget_align;
    int row = 0;

    // First column: titles, subtitles, questions
    // Second and subsequent columns: options

    // Title row
    if (m_show_title) {
        mcqfunc::addOptionBackground(grid, row, 0, n_cols);
        mcqfunc::addTitle(grid, row, m_title);
        addOptions(grid, row);
        ++row;  // new row after title/option text
    }

    // Main question rows (with any preceding subtitles)
    // qi: question index
    // s: subtitle index
    // vi: value index
    for (int qi = 0; qi < m_question_field_pairs.size(); ++qi) {

        // Any preceding subtitles?
        for (int s = 0; s < n_subtitles; ++s) {
            const McqGridSubtitle& sub = m_subtitles.at(s);
            if (sub.pos() == qi) {
                // Yes. Add a subtitle row.
                mcqfunc::addOptionBackground(grid, row, 0, n_cols);
                mcqfunc::addSubtitle(grid, row, sub.string());
                if (sub.repeatOptions()) {
                    addOptions(grid, row);
                }
                ++row;  // new row after subtitle
            }
        }

        if (m_stripy) {
            mcqfunc::addStripeBackground(grid, row, 0, n_cols);
        }

        // The question
        mcqfunc::addQuestion(grid, row,
                             m_question_field_pairs.at(qi).question(),
                             m_questions_bold);

        // The response widgets
        QVector<QPointer<BooleanWidget>> question_widgets;
        for (int vi = 0; vi < n_options; ++vi) {
            QPointer<BooleanWidget> w = new BooleanWidget();
            w->setAppearance(BooleanWidget::Appearance::Radio);
            w->setReadOnly(read_only);
            if (!read_only) {
                // Safe object lifespan signal: can use std::bind
                connect(w, &BooleanWidget::clicked,
                        std::bind(&QuMcqGrid::clicked, this, qi, vi));
            }
            grid->addWidget(w, row, colnum(vi), response_align);
            question_widgets.append(w);
        }
        m_widgets.append(question_widgets);

        ++row;  // new row after question/response widgets
    }

    // Set widths, if asked
    if (m_question_width > 0 && m_option_widths.size() == m_options.size()) {
        grid->setColumnStretch(0, m_question_width);
        for (int i = 0; i < n_options; ++i) {
            grid->setColumnStretch(colnum(i), m_option_widths.at(i));
        }
    }

    // Vertical lines
    mcqfunc::addVerticalLine(grid, 1, n_rows);

    QPointer<QWidget> widget = new BaseWidget();
    widget->setLayout(grid);
    widget->setObjectName(cssconst::MCQ_GRID);
    if (m_expand) {
        widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    } else {
        widget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    }

    setFromFields();

    return widget;
}


FieldRefPtrList QuMcqGrid::fieldrefs() const
{
    FieldRefPtrList refs;
    for (const QuestionWithOneField& q : m_question_field_pairs) {
        refs.append(q.fieldref());
    }
    return refs;
}


void QuMcqGrid::clicked(const int question_index,
                        const int value_index)
{
    if (question_index < 0 || question_index >= m_question_field_pairs.size()) {
        qWarning() << Q_FUNC_INFO << "Bad question_index:" << question_index;
        return;
    }
    if (!m_options.validIndex(value_index)) {
        qWarning() << Q_FUNC_INFO << "- out of range";
        return;
    }
    const NameValueOptions& options =
            m_alternate_options.contains(question_index)
            ? m_alternate_options[question_index]
            : m_options;
    const QVariant newvalue = options.value(value_index);
    FieldRefPtr fieldref = m_question_field_pairs.at(question_index).fieldref();
    fieldref->setValue(newvalue);  // Will trigger valueChanged
    emit elementValueChanged();
}


void QuMcqGrid::fieldValueOrMandatoryChanged(const int question_index,
                                             const FieldRef* fieldref)
{
    if (question_index < 0 ||
            question_index >= m_question_field_pairs.size()) {
        qWarning().nospace()
                << Q_FUNC_INFO
                << " Bad question_index: " << question_index
                << " (m_question_field_pairs.size() == "
                << m_question_field_pairs.size()
                << ")";
        return;
    }
    if (question_index >= m_widgets.size()) {
        // Doesn't require a warning; for example, we may set our
        // mandatory values before the grid is displayed and builds its
        // widgets.
        return;
    }
    const QVector<QPointer<BooleanWidget>>& question_widgets = m_widgets.at(
                question_index);

    const NameValueOptions& options =
            m_alternate_options.contains(question_index)
            ? m_alternate_options[question_index]
            : m_options;
    mcqfunc::setResponseWidgets(options, question_widgets, fieldref);
}
