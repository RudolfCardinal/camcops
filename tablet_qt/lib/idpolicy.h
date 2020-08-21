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
#include <QMap>
#include <QString>
#include <QVector>


// Represents an ID policy from the CamCOPS server (or a special one from
// the client).

class IdPolicy
{
    // Attributes: mapping "attribute name" to "is the attribute present?"
    using AttributesType = QMap<QString, bool>;

public:
    // Construct the policy from text like "sex AND dob AND idnum1..."
    IdPolicy(const QString& policy_text);

    // Does a set of attributes (from the patient) comply with the policy?
    bool complies(const AttributesType& attributes) const;

    // Return the original policy string.
    QString original() const;

    // Return a prettified version (standardized case, etc.).
    QString pretty() const;

    // Return all ID numbers specificallly mentioned somehow in the policy.
    // This does not include those referred to indirectly via "allidnums".
    QVector<int> specificallyMentionedIdNums() const;

public:
    // Result of parsing a "chunk" (a whole policy or a clause thereof).
    // For example, parsing "dob" will return ChunkValue::True if the patient
    // has a DOB, or ::False if he/she doesn't. Parsing "NOT dob" will return
    // the opposite. Parsing "sex AND dob"... you get the idea.
    enum class ChunkValue {
        True,
        False,
        Unknown,
        SyntaxError,
    };

    // Represents a logical operator.
    enum class OperatorValue {
        And,
        Or,
        None,
    };

protected:
    // Converts a token name to a token number.
    int nameToToken(const QString& name) const;

    // Converts a token number to a token name.
    QString tokenToName(int token) const;

    // Parses policy text and writes m_tokens and m_valid.
    void tokenize(QString policy_text);

    // Report a warning to the debug log about a syntax error.
    void reportSyntaxError(const QString& msg) const;

    // Clear all tokens internally, marking the policy as invalid.
    void invalidate();

    // Checks a set of attributes against the policy, or part of the policy.
    ChunkValue idPolicyChunk(const QVector<int>& tokens,
                             const AttributesType& attributes) const;

    // Returns the truth value of a Boolean chunk of the policy. (Can recurse
    // if the policy contains parentheses.)
    ChunkValue idPolicyContent(const QVector<int>& tokens,
                               const AttributesType& attributes,
                               int& index) const;

    // Returns an operator from the policy, or a no-operator-found indicator.
    OperatorValue idPolicyOp(const QVector<int>& tokens, int& index) const;

    // Returns a boolean indicator corresponding to whether the token's
    // information is present in the patient attributes (or a failure
    // indicator).
    ChunkValue idPolicyElement(const AttributesType& attributes,
                               int token) const;

    // Returns a string version of the specified sequence of tokens.
    QString stringify(const QVector<int>& tokens) const;

protected:
    // Maps token integers to names, except for idnum*:
    const static QMap<int, QString> s_token_to_name;
    // Maps names to token integers, except for idnum*:
    const static QMap<QString, int> s_name_to_token;

    QString m_policy_text;  // original text
    QVector<int> m_tokens;  // list of token integers
    bool m_valid;  // is this policy valid? Set by tokenize().
};


extern const IdPolicy TABLET_ID_POLICY;
