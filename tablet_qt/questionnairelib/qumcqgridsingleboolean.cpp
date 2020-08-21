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

#include "qumcqgridsingleboolean.h"
#include "common/cssconst.h"
#include "questionnairelib/mcqfunc.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qumcqgridsinglebooleansignaller.h"
#include "widgets/basewidget.h"
#include "widgets/booleanwidget.h"
#include "widgets/labelwordwrapwide.h"


QuMcqGridSingleBoolean::QuMcqGridSingleBoolean(
        const QVector<QuestionWithTwoFields>& questions_with_fields,
        const NameValueOptions& mcq_options,
        const QString& boolean_text) :
    m_boolean_left(false),
    m_questions_with_fields(questions_with_fields),
    m_mcq_options(mcq_options),
    m_boolean_text(boolean_text),
    m_question_width(-1),
    m_boolean_width(-1),
    m_expand(false),
    m_stripy(true)
{
    m_mcq_options.validateOrDie();
    // each QuestionWithTwoFields will have asserted on construction

    for (int qi = 0; qi < m_questions_with_fields.size(); ++qi) {
        FieldRefPtr mcq_fieldref = m_questions_with_fields.at(qi).firstFieldRef();
        // DANGEROUS OBJECT LIFESPAN SIGNAL: do not use std::bind
        auto sig = new QuMcqGridSingleBooleanSignaller(this, qi);
        m_signallers.append(sig);

        connect(mcq_fieldref.data(), &FieldRef::valueChanged,
                sig, &QuMcqGridSingleBooleanSignaller::mcqFieldValueOrMandatoryChanged);
        connect(mcq_fieldref.data(), &FieldRef::mandatoryChanged,
                sig, &QuMcqGridSingleBooleanSignaller::mcqFieldValueOrMandatoryChanged);

        FieldRefPtr bool_fieldref = m_questions_with_fields.at(qi).secondFieldRef();
        connect(bool_fieldref.data(), &FieldRef::valueChanged,
                sig, &QuMcqGridSingleBooleanSignaller::booleanFieldValueOrMandatoryChanged);
        connect(bool_fieldref.data(), &FieldRef::mandatoryChanged,
                sig, &QuMcqGridSingleBooleanSignaller::booleanFieldValueOrMandatoryChanged);
    }
}


QuMcqGridSingleBoolean::~QuMcqGridSingleBoolean()
{
    while (!m_signallers.isEmpty()) {
        delete m_signallers.takeAt(0);
    }
}


QuMcqGridSingleBoolean* QuMcqGridSingleBoolean::setBooleanLeft(
        const bool boolean_left)
{
    m_boolean_left = boolean_left;
    return this;
}


QuMcqGridSingleBoolean* QuMcqGridSingleBoolean::setWidth(
        const int question_width,
        const QVector<int>& mcq_option_widths,
        const int boolean_width)
{
    if (mcq_option_widths.size() != m_mcq_options.size()) {
        qWarning() << Q_FUNC_INFO << "Bad mcq_option_widths; command ignored";
        return this;
    }
    m_question_width = question_width;
    m_mcq_option_widths = mcq_option_widths;
    m_boolean_width = boolean_width;
    return this;
}


QuMcqGridSingleBoolean* QuMcqGridSingleBoolean::setTitle(const QString &title)
{
    m_title = title;
    return this;
}


QuMcqGridSingleBoolean* QuMcqGridSingleBoolean::setSubtitles(
        const QVector<McqGridSubtitle>& subtitles)
{
    m_subtitles = subtitles;
    return this;
}


QuMcqGridSingleBoolean* QuMcqGridSingleBoolean::setExpand(const bool expand)
{
    m_expand = expand;
    return this;
}


QuMcqGridSingleBoolean* QuMcqGridSingleBoolean::setStripy(const bool stripy)
{
    m_stripy = stripy;
    return this;
}


void QuMcqGridSingleBoolean::setFromFields()
{
    for (int qi = 0; qi < m_questions_with_fields.size(); ++qi) {
        mcqFieldValueOrMandatoryChanged(
                    qi, m_questions_with_fields.at(qi).firstFieldRef().data());
        booleanFieldValueOrMandatoryChanged(
                    qi, m_questions_with_fields.at(qi).secondFieldRef().data());
    }
}


int QuMcqGridSingleBoolean::mcqColnum(const int value_index) const
{
    return (m_boolean_left ? 4 : 2) + value_index;
}


int QuMcqGridSingleBoolean::booleanColnum() const
{
    return m_boolean_left ? 2 : (3 + m_mcq_options.size());
}


int QuMcqGridSingleBoolean::spacercol(const bool first) const
{
    return first ? 1 : ((m_boolean_left ? mcqColnum(0) : booleanColnum()) - 1);
}


void QuMcqGridSingleBoolean::addOptions(GridLayout* grid, const int row)
{
    for (int i = 0; i < m_mcq_options.size(); ++i) {
        mcqfunc::addOption(grid, row, mcqColnum(i), m_mcq_options.at(i).name());
    }
    mcqfunc::addOption(grid, row, booleanColnum(), m_boolean_text);
}


QPointer<QWidget> QuMcqGridSingleBoolean::makeWidget(Questionnaire* questionnaire)
{
    const bool read_only = questionnaire->readOnly();
    m_mcq_widgets.clear();
    m_boolean_widgets.clear();

    auto grid = new GridLayout();
    grid->setContentsMargins(uiconst::NO_MARGINS);
    grid->setHorizontalSpacing(uiconst::MCQGRID_HSPACING);
    grid->setVerticalSpacing(uiconst::MCQGRID_VSPACING);

    const int n_subtitles = m_subtitles.size();
    const int n_rows = 1 + n_subtitles + m_questions_with_fields.size();
    const int n_cols = m_mcq_options.size() + 4;
    const Qt::Alignment response_align = mcqfunc::response_widget_align;
    const int n_options = m_mcq_options.size();
    int row = 0;

    // Title row
    mcqfunc::addOptionBackground(grid, row, 0, n_cols);
    mcqfunc::addTitle(grid, row, m_title);
    addOptions(grid, row);
    ++row;  // new row after title/option text

    // Main question rows (with any preceding subtitles)
    for (int qi = 0; qi < m_questions_with_fields.size(); ++qi) {

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
                                 m_questions_with_fields.at(qi).question());

        // The response widgets
        QVector<QPointer<BooleanWidget>> question_widgets;
        for (int vi = 0; vi < n_options; ++vi) {
            QPointer<BooleanWidget> w = new BooleanWidget();
            w->setAppearance(BooleanWidget::Appearance::Radio);
            w->setReadOnly(read_only);
            if (!read_only) {
                // Safe object lifespan signal: can use std::bind
                connect(w, &BooleanWidget::clicked,
                        std::bind(&QuMcqGridSingleBoolean::mcqClicked,
                                  this, qi, vi));
            }
            grid->addWidget(w, row, mcqColnum(vi), response_align);
            question_widgets.append(w);
        }
        m_mcq_widgets.append(question_widgets);

        QPointer<BooleanWidget> bw = new BooleanWidget();
        bw->setAppearance(BooleanWidget::Appearance::CheckRed);
        bw->setReadOnly(read_only);
        if (!read_only) {
            // Safe object lifespan signal: can use std::bind
            connect(bw, &BooleanWidget::clicked,
                    std::bind(&QuMcqGridSingleBoolean::booleanClicked,
                              this, qi));
        }
        grid->addWidget(bw, row, booleanColnum(), response_align);
        m_boolean_widgets.append(bw);

        ++row;  // new row after question/response widgets
    }

    // Set widths, if asked
    if (m_question_width > 0 &&
            m_mcq_option_widths.size() == m_mcq_options.size()) {
        grid->setColumnStretch(0, m_question_width);
        for (int i = 0; i < m_mcq_option_widths.size(); ++i) {
            grid->setColumnStretch(mcqColnum(i), m_mcq_option_widths.at(i));
        }
        grid->setColumnStretch(booleanColnum(), m_boolean_width);
    }

    // Vertical lines
    mcqfunc::addVerticalLine(grid, spacercol(true), n_rows);
    mcqfunc::addVerticalLine(grid, spacercol(false), n_rows);

    QPointer<QWidget> widget = new BaseWidget();
    widget->setLayout(grid);
    widget->setObjectName(cssconst::MCQ_GRID_SINGLE_BOOLEAN);
    if (m_expand) {
        widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    } else {
        widget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    }

    setFromFields();

    return widget;
}


FieldRefPtrList QuMcqGridSingleBoolean::fieldrefs() const
{
    FieldRefPtrList refs;
    for (const QuestionWithTwoFields& q : m_questions_with_fields) {
        refs.append(q.firstFieldRef());
        refs.append(q.secondFieldRef());
    }
    return refs;
}


void QuMcqGridSingleBoolean::mcqClicked(const int question_index,
                                        const int value_index)
{
    if (question_index < 0 ||
            question_index >= m_questions_with_fields.size()) {
        qWarning() << Q_FUNC_INFO << "Bad question_index:" << question_index;
        return;
    }
    if (!m_mcq_options.validIndex(value_index)) {
        qWarning() << Q_FUNC_INFO << "- out of range";
        return;
    }
    const QVariant newvalue = m_mcq_options.value(value_index);
    FieldRefPtr fieldref = m_questions_with_fields.at(question_index)
            .firstFieldRef();
    const bool changed = fieldref->setValue(newvalue);  // Will trigger valueChanged
    if (changed) {
        emit elementValueChanged();
    }
}


void QuMcqGridSingleBoolean::booleanClicked(const int question_index)
{
    if (question_index < 0 ||
            question_index >= m_questions_with_fields.size()) {
        qWarning() << Q_FUNC_INFO << "Bad question_index:" << question_index;
        return;
    }
    FieldRefPtr fieldref = m_questions_with_fields.at(question_index)
            .secondFieldRef();
    mcqfunc::toggleBooleanField(fieldref.data());
    emit elementValueChanged();
}


void QuMcqGridSingleBoolean::mcqFieldValueOrMandatoryChanged(
        const int question_index, const FieldRef* fieldref)
{
    if (question_index < 0 ||
            question_index >= m_questions_with_fields.size()) {
        qWarning() << Q_FUNC_INFO << "Bad question_index:" << question_index;
        return;
    }
    if (question_index >= m_mcq_widgets.size()) {
        return;
    }
    const QVector<QPointer<BooleanWidget>>& question_widgets = m_mcq_widgets.at(
                question_index);

    mcqfunc::setResponseWidgets(m_mcq_options, question_widgets, fieldref);
}


void QuMcqGridSingleBoolean::booleanFieldValueOrMandatoryChanged(
        const int question_index, const FieldRef* fieldref)
{
    if (question_index < 0 ||
            question_index >= m_questions_with_fields.size()) {
        qWarning() << Q_FUNC_INFO << "Bad question_index:" << question_index;
        return;
    }
    if (question_index >= m_boolean_widgets.size()) {
        return;
    }
    QPointer<BooleanWidget> bw = m_boolean_widgets.at(question_index);

    bw->setValue(fieldref->value(), fieldref->mandatory());
}
