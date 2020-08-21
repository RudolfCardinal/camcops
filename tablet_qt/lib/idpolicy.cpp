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

#include "idpolicy.h"
#include <QObject>
#include <QRegularExpression>  // replacing QRegExp; http://doc.qt.io/qt-5.7/qregexp.html#details
#include "common/dbconst.h"
#include "common/design_defines.h"
#include "common/preprocessor_aid.h"
#include "dbobjects/patient.h"

// ============================================================================
// Constants
// ============================================================================

const int BAD_TOKEN = 0;
const int TOKEN_LPAREN = -1;
const int TOKEN_RPAREN = -2;
const int TOKEN_AND = -3;
const int TOKEN_OR = -4;
const int TOKEN_NOT = -5;
const int TOKEN_ANY_IDNUM = -6;
const int TOKEN_OTHER_IDNUM = -7;
const int TOKEN_FORENAME = -8;
const int TOKEN_SURNAME = -9;
const int TOKEN_SEX = -10;
const int TOKEN_DOB = -11;
const int TOKEN_ADDRESS = -12;
const int TOKEN_GP = -13;
const int TOKEN_OTHER_DETAILS = -14;
// Tokens for ID numbers are from 1 upwards.

const QString TOKENIZE_RE_STR(
    // http://stackoverflow.com/questions/6162600/
    // http://stackoverflow.com/questions/20508534/c-multiline-string-raw-literal
    // http://doc.qt.io/qt-5/qregularexpression.html#details

    "\\s*"    // discard leading whitespace
    "("         // start capture group
        "\\w+"      // word character
    "|"         // alternator
        "\\("       // left parenthesis
    "|"         // alternator
        "\\)"       // right parenthesis
    ")"         // end capture group, and we can have lots of them

    // In full:
    // \s*(\w+|\(|\))
);


// ============================================================================
// Static data
// ============================================================================

QMap<int, QString> makeTokenToNameDict()
{
    // function to create static data

    QMap<int, QString> token_to_name;

    // Everything except ID numbers:

    token_to_name[TOKEN_LPAREN] = "(";
    token_to_name[TOKEN_RPAREN] = ")";
    token_to_name[TOKEN_AND] = "and";
    token_to_name[TOKEN_OR] = "or";
    token_to_name[TOKEN_NOT] = "not";

    token_to_name[TOKEN_ANY_IDNUM] = ANY_IDNUM_POLICYNAME;
    token_to_name[TOKEN_OTHER_IDNUM] = OTHER_IDNUM_POLICYNAME;

    token_to_name[TOKEN_FORENAME] = FORENAME_FIELD;
    token_to_name[TOKEN_SURNAME] = SURNAME_FIELD;
    token_to_name[TOKEN_SEX] = SEX_FIELD;
    token_to_name[TOKEN_DOB] = DOB_FIELD;
    token_to_name[TOKEN_ADDRESS] = ADDRESS_FIELD;
    token_to_name[TOKEN_GP] = GP_FIELD;
    token_to_name[TOKEN_OTHER_DETAILS] = OTHER_DETAILS_POLICYNAME;

    return token_to_name;
}


QMap<QString, int> makeNameToTokenDict(const QMap<int, QString>& token_to_name)
{
    // function to create static data

    QMap<QString, int> name_to_token;
    QMapIterator<int, QString> it(token_to_name);
    while (it.hasNext()) {
        it.next();
        name_to_token[it.value()] = it.key();
    }
    return name_to_token;
}


const QMap<int, QString> IdPolicy::s_token_to_name = makeTokenToNameDict();
const QMap<QString, int> IdPolicy::s_name_to_token = makeNameToTokenDict(IdPolicy::s_token_to_name);


// ============================================================================
// IdPolicy class
// ============================================================================

IdPolicy::IdPolicy(const QString& policy_text) :
    m_policy_text(policy_text)
{
    tokenize(policy_text);
}


int IdPolicy::nameToToken(const QString& name) const
{
    // One of our pre-cached tokens?
    if (s_name_to_token.contains(name)) {
        return s_name_to_token[name];
    }
    // An ID number token?
    if (name.startsWith(IDNUM_FIELD_PREFIX)) {
        const QString number = name.right(name.length() - IDNUM_FIELD_PREFIX.length());
        bool ok = false;
        const int which_idnum = number.toInt(&ok);
        if (ok) {
            return which_idnum;
        }
    }
    // Failed.
    return BAD_TOKEN;
}


QString IdPolicy::tokenToName(const int token) const
{
    if (token > 0) {
        return IDNUM_FIELD_FORMAT.arg(token);
    }
    if (s_token_to_name.contains(token)) {
        return s_token_to_name[token];
    }
    qWarning() << Q_FUNC_INFO << "Bad token!";
    return "BAD_TOKEN";
}


void IdPolicy::tokenize(QString policy_text)
{
    m_valid = true;

    // Single line, whitespace trimmed, newlines/tabs etc. removed
    policy_text = policy_text.simplified();

    // QString::split() splits using the regex to match BOUNDARIES.
    // Our regex matches CONTENT, so we use QRegularExpression functions.
    // https://dangelog.wordpress.com/2012/04/07/qregularexpression/
    const QRegularExpression re(TOKENIZE_RE_STR);
    QRegularExpressionMatchIterator it = re.globalMatch(policy_text);
    QStringList words;
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString word = match.captured(1);
        words << word;
    }
    for (const QString& word : words) {
        const QString element = word.toLower();
        const int token = nameToToken(element);
        if (token == BAD_TOKEN) {
            reportSyntaxError(QString("unknown word: %1").arg(word));
            invalidate();
            return;
        }
        m_tokens.append(token);
    }
    // check syntax:
    AttributesType blank_attributes;
    for (const QString& name : s_name_to_token.keys()) {
        blank_attributes[name] = false;
    }
    if (idPolicyChunk(m_tokens, blank_attributes) == ChunkValue::SyntaxError) {
        invalidate();
    }
}


void IdPolicy::invalidate()
{
    m_valid = false;
    m_tokens.clear();
}


QString IdPolicy::original() const
{
    return m_policy_text;
}


QString IdPolicy::pretty() const
{
    if (!m_valid) {
        return QObject::tr("[Invalid policy]");
    }
    return stringify(m_tokens);
}


QString IdPolicy::stringify(const QVector<int> &tokens) const
{
    QString policy;
    for (int i = 0; i < tokens.length(); ++i) {
        const int token = tokens.at(i);
        if (i > 0 && token != TOKEN_RPAREN
                && tokens.at(i - 1) != TOKEN_LPAREN) {
            policy += " ";
        }
        const QString element = tokenToName(token);
        const bool upper = (token == TOKEN_AND || token == TOKEN_OR);
        policy += upper ? element.toUpper() : element.toLower();
    }
    return policy;
}


void IdPolicy::reportSyntaxError(const QString &msg) const
{
    qWarning().nospace() << "Syntax error in policy (" << msg
                         << "); policy text is: " << m_policy_text;
}


bool IdPolicy::complies(const AttributesType& attributes) const
{
    // A duff policy doesn't match anything:
    if (!m_valid) {
        return false;
    }
    // A valid empty policy matches anything:
    if (m_tokens.empty()) {
        return true;
    }
    // Parse the whole policy:
    ChunkValue value = idPolicyChunk(m_tokens, attributes);
    // ... which recurses for sub-chunks if required.
    return value == ChunkValue::True;
}


IdPolicy::ChunkValue IdPolicy::idPolicyChunk(
        const QVector<int>& tokens,
        const AttributesType& attributes) const
{
    // qDebug() << Q_FUNC_INFO << stringify(tokens);
    if (!m_valid) {
        return ChunkValue::SyntaxError;
    }
    bool want_content = true;
    bool processing_and = false;
    bool processing_or = false;
    int index = 0;
    ChunkValue value = ChunkValue::Unknown;
    while (index < tokens.length()) {
        if (want_content) {
            // We want content (a field token)
            ChunkValue nextchunk = idPolicyContent(tokens, attributes, index);
            if (nextchunk == ChunkValue::Unknown ||
                    nextchunk == ChunkValue::SyntaxError) {
                return ChunkValue::SyntaxError;
            }
            if (value == ChunkValue::Unknown) {
                // First element in this chunk
                value = nextchunk;
            } else if (processing_and) {
                if (nextchunk != ChunkValue::True) {
                    value = ChunkValue::False;
                }
            } else if (processing_or) {
                if (nextchunk == ChunkValue::True) {
                    value = ChunkValue::True;
                }
            } else {
                reportSyntaxError("invalid expression");
                return ChunkValue::SyntaxError;
            }
            processing_and = false;
            processing_or = false;
        } else {
            // We want an operator
            OperatorValue op = idPolicyOp(tokens, index);
            switch (op) {
            case OperatorValue::And:
                processing_and = true;
                break;
            case OperatorValue::Or:
                processing_or = true;
                break;
            case OperatorValue::None:
                reportSyntaxError("missing operator");
                return ChunkValue::SyntaxError;
            }
        }
        want_content = !want_content;
    }
    if (value == ChunkValue::Unknown || want_content) {
        reportSyntaxError("policy incomplete");
        return ChunkValue::SyntaxError;
    }
    return value;
}


IdPolicy::ChunkValue IdPolicy::idPolicyContent(const QVector<int>& tokens,
                                               const AttributesType& attributes,
                                               int& index) const
{
    // qDebug() << Q_FUNC_INFO << "tokens =" << stringify(tokens) << "; index =" << index;
    if (index >= tokens.length()) {
        reportSyntaxError("policy incomplete; missing content at end");
        return ChunkValue::SyntaxError;
    }
    int token = tokens.at(index++);
    switch (token) {
    case TOKEN_RPAREN:
    case TOKEN_AND:
    case TOKEN_OR:
        reportSyntaxError("chunk can't start with AND/OR/')'");
        return ChunkValue::SyntaxError;
    case TOKEN_LPAREN:
        {
            // The recursive bit.
            int subchunkstart = index;  // one past the opening bracket
            // Find closing parenthesis:
            int depth = 1;
            while (depth > 0) {
                if (index >= tokens.length()) {
                    reportSyntaxError("unmatched left parenthesis");
                    return ChunkValue::SyntaxError;
                }
                int subtoken = tokens.at(index++);
                if (subtoken == TOKEN_LPAREN) {
                    depth += 1;
                } else if (subtoken == TOKEN_RPAREN) {
                    depth -= 1;
                }
            }
            // At this point, subchunkstart points one past the opening
            // parenthesis, and searchidx points one past the closing
            // parenthesis. We want to exclude the closing parenthesis too.
            int subchunklen = index - subchunkstart - 1;
            QVector<int> subchunk = tokens.mid(subchunkstart, subchunklen);
            return idPolicyChunk(subchunk, attributes);
        }
    case TOKEN_NOT:
        {
            ChunkValue nextchunk = idPolicyContent(tokens, attributes, index);
            switch (nextchunk) {
            case ChunkValue::SyntaxError:
            case ChunkValue::Unknown:
#ifdef COMPILER_WANTS_DEFAULT_IN_EXHAUSTIVE_SWITCH
            default:
#endif
                return nextchunk;
            case ChunkValue::False:  // invert
                return ChunkValue::True;
            case ChunkValue::True:  // invert
                return ChunkValue::False;
            }
        }
    default:
        // meaningful token
        return idPolicyElement(attributes, token);
    }
}


IdPolicy::OperatorValue IdPolicy::idPolicyOp(const QVector<int>& tokens,
                                             int& index) const
{
    // qDebug() << Q_FUNC_INFO << "tokens =" << stringify(tokens) << "; index =" << index;
    if (index >= tokens.length()) {
        reportSyntaxError("policy incomplete; missing operator at end");
        return OperatorValue::None;
    }
    const int token = tokens.at(index++);
    switch (token) {
    case TOKEN_AND:
        return OperatorValue::And;
    case TOKEN_OR:
        return OperatorValue::Or;
    default:
        return OperatorValue::None;
    }
}


IdPolicy::ChunkValue IdPolicy::idPolicyElement(const AttributesType& attributes,
                                               const int token) const
{
    const QString name = tokenToName(token);
    if (token <= 0) {
        if (!attributes.contains(name)) {
            qWarning() << "Policy contains element" << name
                       << "but patient information is unaware of that attribute";
            return ChunkValue::Unknown;
        }
        return attributes[name] ? ChunkValue::True : ChunkValue::False;
    }
    if (attributes.contains(name)) {
        return attributes[name] ? ChunkValue::True : ChunkValue::False;
    }
    // But if it's absent, that's just a missing ID, not a syntax error:
    return ChunkValue::False;
}


QVector<int> IdPolicy::specificallyMentionedIdNums() const
{
    QVector<int> mentioned;
    for (const int t : m_tokens) {
        if (t > 0) {
            mentioned.append(t);
        }
    }
    return mentioned;
}



// ============================================================================
// Tablet ID policy
// ============================================================================

const IdPolicy TABLET_ID_POLICY(
        "sex AND ((forename AND surname AND dob) OR anyidnum)");

// ... clinical environment: forename/surname/dob/sex, and we can await an
//     ID number later
// ... research environment: sex and one ID number for pseudonymised
//     applications
