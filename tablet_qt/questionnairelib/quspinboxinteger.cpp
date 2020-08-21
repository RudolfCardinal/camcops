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

// #define DEBUG_SIGNALS

#include "quspinboxinteger.h"
#include <QSpinBox>
#include "lib/uifunc.h"
#include "questionnairelib/questionnaire.h"


QuSpinBoxInteger::QuSpinBoxInteger(FieldRefPtr fieldref, const int minimum,
                                   const int maximum) :
    m_fieldref(fieldref),
    m_minimum(minimum),
    m_maximum(maximum)
{
    Q_ASSERT(m_fieldref);
    connect(m_fieldref.data(), &FieldRef::valueChanged,
            this, &QuSpinBoxInteger::fieldValueChanged);
    connect(m_fieldref.data(), &FieldRef::mandatoryChanged,
            this, &QuSpinBoxInteger::fieldValueChanged);
}


void QuSpinBoxInteger::setFromField()
{
    fieldValueChanged(m_fieldref.data(), nullptr);
    // special; pretend "it didn't come from us" to disable the efficiency
    // check in fieldValueChanged
}


QPointer<QWidget> QuSpinBoxInteger::makeWidget(Questionnaire* questionnaire)
{
    const bool read_only = questionnaire->readOnly();
    m_spinbox = new QSpinBox();
    m_spinbox->setEnabled(!read_only);
    m_spinbox->setRange(m_minimum, m_maximum);
    m_spinbox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_spinbox->setMinimumHeight(uiconst::g_min_spinbox_height);  // room for spin arrows
    m_spinbox->setButtonSymbols(uiconst::SPINBOX_SYMBOLS);
    m_spinbox->setInputMethodHints(Qt::ImhFormattedNumbersOnly);

    // QSpinBox has two signals named valueChanged, differing only
    // in the parameter they pass (int versus QString&). You get
    // "no matching function for call to ... unresolved overloaded function
    // type..."
    // Disambiguate like this:
    if (!read_only) {
        void (QSpinBox::*vc_signal)(int) = &QSpinBox::valueChanged;
        connect(m_spinbox.data(), vc_signal,
                this, &QuSpinBoxInteger::widgetValueChanged);
#ifdef DEBUG_SIGNALS
        void (QSpinBox::*vc_sig_str)(const QString&) = &QSpinBox::valueChanged;
        connect(m_spinbox.data(), vc_sig_str,
                this, &QuSpinBoxInteger::widgetValueChangedString);
#endif
    }
    setFromField();
    return QPointer<QWidget>(m_spinbox);
}


FieldRefPtrList QuSpinBoxInteger::fieldrefs() const
{
    return FieldRefPtrList{m_fieldref};
}


void QuSpinBoxInteger::widgetValueChanged(const int value)
{
#ifdef DEBUG_SIGNALS
    qDebug() << Q_FUNC_INFO << value;
#endif
    const bool changed = m_fieldref->setValue(value, this);  // Will trigger valueChanged
    if (changed) {
        emit elementValueChanged();
    }
}


void QuSpinBoxInteger::widgetValueChangedString(const QString& text)
{
    qDebug() << Q_FUNC_INFO << text;
}


void QuSpinBoxInteger::fieldValueChanged(const FieldRef* fieldref,
                                         const QObject* originator)
{
    if (!m_spinbox) {
        return;
    }
    uifunc::setPropertyMissing(m_spinbox, fieldref->missingInput());
    if (originator != this) {
        const QSignalBlocker blocker(m_spinbox);
        m_spinbox->setValue(fieldref->valueInt());
    }
}
