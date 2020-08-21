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
#include <QValidator>

// Validator for quint64 numbers. Checks against the range [bottom, top].

class StrictUInt64Validator : public QValidator
{
    Q_OBJECT
    Q_PROPERTY(int bottom READ bottom WRITE setBottom NOTIFY bottomChanged)
    Q_PROPERTY(int top READ top WRITE setTop NOTIFY topChanged)
public:
    StrictUInt64Validator(bool allow_empty = false, QObject* parent = nullptr);
    StrictUInt64Validator(quint64 bottom, quint64 top,
                          bool allow_empty = false,
                          QObject* parent = nullptr);
    virtual ~StrictUInt64Validator() override;

    QValidator::State validate(QString& input, int& pos) const override;

    void setBottom(quint64 bottom);
    void setTop(quint64 top);
    virtual void setRange(quint64 bottom, quint64 top);

    quint64 bottom() const { return m_b; }
    quint64 top() const { return m_t; }

signals:
    void bottomChanged(quint64 bottom);
    void topChanged(quint64 top);

private:
    Q_DISABLE_COPY(StrictUInt64Validator)

private:
    quint64 m_b;
    quint64 m_t;
    bool m_allow_empty;
};
