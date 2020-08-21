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
#include <QVector>
#include "questionnairelib/namevaluepair.h"


class NameValueOptions
{
/*
    Encapsulates a list of name/value pairs.

    We don't allow duplicate values.
    There are some circumstances when intuitively this would be helpful, e.g.
    we are offering several wrong answers, and don't care which one is
    selected; such as

        Q. What is 2 + 2?

        a) One [-> score 0]
        b) Two [-> score 0]
        c) Three [-> score 0]
        d) Four [-> score 1]
        e) Five [-> score 0]

    You might think it'd be OK to support that situation. HOWEVER, it's not.
    It would mean that the user's choice would be irrecoverable from the
    data, which is not acceptable. In this situation, store a value for the
    choice, and calculate the score separately, e.g. with

        One -> 'A'
        Two -> 'B'
        Three -> 'C'
        Four -> 'D'
        Five -> 'E'

        int score(char value)
        {
            switch (value) {
                // ...
            }
        }

    It's fine (although odd) to have duplicate names.

*/

public:

    // Default constructor
    NameValueOptions();

    // Construct with values.
    NameValueOptions(std::initializer_list<NameValuePair> options);

    // Add a new name/value pair, at the end.
    void append(const NameValuePair& nvp);

    // If a name/value pair exists with the same value, replace it with nvp.
    // Otherwise, if append_if_not_found is true, append nvp.
    void replace(const NameValuePair& nvp, bool append_if_not_found = false);

    // How many name/value pairs do we have?
    int size() const;

    // Return the name/value pair at the given (zero-based) index.
    const NameValuePair& at(int index) const;

    // Return the first index associated with the specified name, or -1 on
    // failure.
    int indexFromName(const QString& name) const;

    // Return the index associated with the specified value, or -1 on failure.
    int indexFromValue(const QVariant& value) const;

    // Check there are no duplicate values, or crash the app.
    void validateOrDie();

    // Is the index valid, i.e. in the range [0, size() - 1]?
    bool validIndex(int index) const;

    // Randomize the order (in place).
    void shuffle();

    // Reverse the order (in place).
    void reverse();

    // Returns the name for a given index, or "" if the index is invalid.
    QString name(int index) const;

    // Returns the name for a given index, or QVariant() if the index is
    // invalid.
    QVariant value(int index) const;

    // Returns the name for a given value, or a default string if there isn't
    // one.
    QString nameFromValue(const QVariant& value,
                          const QString& default_ = "") const;

    // Returns the first value for a given name, or a default if there isn't
    // one.
    QVariant valueFromName(const QString& name,
                           const QVariant& default_ = QVariant()) const;

public:

    // Returns a NameValueOptions like {{"1", 1}, {"2", 2}, {"3", 3}...}
    // where the number progresses from "first" to "last" in steps of "step".
    static NameValueOptions makeNumbers(int first, int last, int step = 1);

protected:
    // Stores the options.
    QVector<NameValuePair> m_options;

public:
    // Debugging description.
    friend QDebug operator<<(QDebug debug, const NameValueOptions& gi);
};
