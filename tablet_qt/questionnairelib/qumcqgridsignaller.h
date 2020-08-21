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
#include <QObject>

class FieldRef;
class QuMcqGrid;


class QuMcqGridSignaller : public QObject {

    // Signals to QuMCQGrid that one of its fields has changed data or
    // mandatory state.

    // This should be a private (nested) class of QuMCQGrid, but you can't nest
    // Q_OBJECT classes ("Error: Meta object features not supported for nested
    // classes").

    Q_OBJECT
public:

    // Constructor:
    // - recipient: to what are we signalling?
    // - question_index: information to convey
    QuMcqGridSignaller(QuMcqGrid* recipient, int question_index);

public slots:
    // Signalled to by a FieldRef. Passes the signal to its QuMcqGrid.
    void valueOrMandatoryChanged(const FieldRef* fieldref);

protected:
    QuMcqGrid* m_recipient;
    int m_question_index;
};
