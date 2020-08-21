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

#include "icd9cm.h"
#include <QDebug>
#include <QMap>
#include <QObject>

const QString Icd9cm::XSTRING_TASKNAME("icd9cm");

const QString RANGE_PREFIX("range");  // in string names


// ============================================================================
// Main functions
// ============================================================================

Icd9cm::Icd9cm(CamcopsApp& app, QObject* parent,
               bool dummy_creation_no_xstrings) :
    DiagnosticCodeSet(app, XSTRING_TASKNAME, "ICD-9-CM",
                      parent, dummy_creation_no_xstrings)
{
    m_creation_stack.push(DepthItemPair(0, nullptr));  // root: depth 0, no parent
    addIcd9cmCodes(BASE_CODES);
}


void Icd9cm::addIcd9cmCodes(const QStringList& codes)
{
    // Conceptually: as for Icd10.
    for (const QString& c : codes) {
        const QString desc = xstring(c);

        const bool show_code_in_full_name = !c.startsWith(RANGE_PREFIX);
        addIndividualIcd9cmCode(c, desc, show_code_in_full_name);

        // Any special sub-codes?
        const int length = c.length();
        if (length == 5 && c.startsWith("295.")) {
            // Types of schizophrenia
            addSchizophrenia(c, desc);
        } else if (length == 5 && c.startsWith("296") && (c.endsWith('0') ||
                                                           c.endsWith('1') ||
                                                           c.endsWith('2') ||
                                                           c.endsWith('3') ||
                                                           c.endsWith('4') ||
                                                           c.endsWith('5') ||
                                                           c.endsWith('6'))) {
            // Episodic affective disorders, 296.0 - 296.6
            addEpisodicAffective(c, desc);
        } else if (length == 5 && (c.startsWith("303") ||
                                   c.startsWith("304") ||
                                   c.startsWith("305"))) {
            // Substance-induced disorders
            // The sub-codes often have a different heading from the
            // main stem, in which case we specify them separately with the
            // strings having an "x" suffix.
            QString subdescprefix;
            if (c == "303.0" ||
                    c == "305.0" ||
                    c == "305.2" ||
                    c == "305.3" ||
                    c == "305.4" ||
                    c == "305.5" ||
                    c == "305.6" ||
                    c == "305.7" ||
                    c == "305.8" ||
                    c == "305.9") {
                subdescprefix = xstring(c + "x");
            } else {
                subdescprefix = desc;
            }
            addSubstance(c, subdescprefix);
        }
    }
}


void Icd9cm::addIndividualIcd9cmCode(const QString& code, const QString& desc,
                                     const bool show_code_in_full_name)
{
    if (code.isEmpty()) {
        qCritical() << Q_FUNC_INFO << "zero-length code! Ignoring";
        return;
    }

    // Establish depth of new one
    int depth = code.length();
    // do any depth modifications here, if required:
    if (code.startsWith(RANGE_PREFIX)) {
        // longer description, but higher in the hierarchy
        depth = 1;
    }

    while (depth <= m_creation_stack.top().first) {
        m_creation_stack.pop();
    }
    DiagnosticCode* parent = m_creation_stack.top().second;
    const bool selectable = (
        (code.length() > 4 && !code.startsWith(RANGE_PREFIX)) ||
                // plus some specific ones with no children:
                code == "311" ||
                code == "316" ||
                code == "317" ||
                code == "319"
    );
    DiagnosticCode* newchild = addCode(parent, code, desc, selectable,
                                       show_code_in_full_name);
    m_creation_stack.push(DepthItemPair(depth, newchild));
}


void Icd9cm::addSubcodes(const QString& basecode,
                         const QString& basedesc,
                         const QVector<CodeDescriptionPair>& level1)
{
    for (auto extra1 : level1) {
        const QString code = QString("%1%2").arg(basecode, extra1.first);
        const QString desc = QString("%1, %2").arg(
                    basedesc, xstring(extra1.second));
        addIndividualIcd9cmCode(code, desc);
    }
}


// ============================================================================
// Episodic affective disorders
// ============================================================================

const QVector<Icd9cm::CodeDescriptionPair> Icd9cm::EPISODIC_AFFECTIVE_L1{
    // The 296.x0 - 296.x6 codes
    {"0", "affective_x0"},
    {"1", "affective_x1"},
    {"2", "affective_x2"},
    {"3", "affective_x3"},
    {"4", "affective_x4"},
    {"5", "affective_x5"},
    {"6", "affective_x6"},
};


void Icd9cm::addEpisodicAffective(const QString& basecode,
                                  const QString& basedesc)
{
    addSubcodes(basecode, basedesc, EPISODIC_AFFECTIVE_L1);
}


// ============================================================================
// Substance-induced
// ============================================================================

const QVector<Icd9cm::CodeDescriptionPair> Icd9cm::SUBSTANCE_L1{
    // The 304.x0 - 304.x3 (and 305.x0 - 305.x3) codes
    {"0", "substance_x0"},
    {"1", "substance_x1"},
    {"2", "substance_x2"},
    {"3", "substance_x3"},
};


void Icd9cm::addSubstance(const QString& basecode, const QString& basedesc)
{
    addSubcodes(basecode, basedesc, SUBSTANCE_L1);
}


// ============================================================================
// Schizophrenia
// ============================================================================

const QVector<Icd9cm::CodeDescriptionPair> Icd9cm::SCHIZOPHRENIA_L1{
    // The 295.x0 - 295.x5 codes
    {"0", "schizophrenia_x0"},
    {"1", "schizophrenia_x1"},
    {"2", "schizophrenia_x2"},
    {"3", "schizophrenia_x3"},
    {"4", "schizophrenia_x4"},
    {"5", "schizophrenia_x5"},
};


void Icd9cm::addSchizophrenia(const QString& basecode, const QString& basedesc)
{
    addSubcodes(basecode, basedesc, SCHIZOPHRENIA_L1);
}


// ============================================================================
// Main codes
// ============================================================================

const QStringList Icd9cm::BASE_CODES{
    "range_290_294",
    "290",
    "290.0",
    "290.1",
    "290.10",
    "290.11",
    "290.12",
    "290.13",
    "290.2",
    "290.20",
    "290.21",
    "290.3",
    "290.4",
    "290.40",
    "290.41",
    "290.42",
    "290.43",
    "290.8",
    "290.9",
    "291",
    "291.0",
    "291.1",
    "291.2",
    "291.3",
    "291.4",
    "291.5",
    "291.8",
    "291.81",
    "291.82",
    "291.89",
    "291.9",
    "292",
    "292.0",
    "292.1",
    "292.11",
    "292.12",
    "292.2",
    "292.8",
    "292.81",
    "292.82",
    "292.83",
    "292.84",
    "292.85",
    "292.89",
    "292.9",
    "293",
    "293.0",
    "293.1",
    "293.8",
    "293.81",
    "293.82",
    "293.83",
    "293.84",
    "293.89",
    "293.9",
    "294",
    "294.0",
    "294.1",
    "294.10",
    "294.11",
    "294.2",
    "294.20",
    "294.21",
    "294.8",
    "294.9",

    "range_295_299",
    "295",
    "295.0",
    "295.1",
    "295.2",
    "295.3",
    "295.4",
    "295.5",
    "295.6",
    "295.7",
    "295.8",
    "295.9",
    "296",
    "296.0",
    "296.1",
    "296.2",
    "296.3",
    "296.4",
    "296.5",
    "296.6",
    "296.7",
    "296.8",
    "296.80",
    "296.81",
    "296.82",
    "296.89",
    "296.9",
    "296.90",
    "296.99",
    "297",
    "297.0",
    "297.1",
    "297.2",
    "297.3",
    "297.8",
    "297.9",
    "298",
    "298.0",
    "298.1",
    "298.2",
    "298.3",
    "298.4",
    "298.8",
    "298.9",
    "299",
    "299.0",
    "299.00",
    "299.01",
    "299.1",
    "299.10",
    "299.11",
    "299.8",
    "299.80",
    "299.81",
    "299.9",
    "299.90",
    "299.91",

    "range_300_316",
    "300",
    "300.0",
    "300.00",
    "300.01",
    "300.02",
    "300.09",
    "300.1",
    "300.10",
    "300.11",
    "300.12",
    "300.13",
    "300.14",
    "300.15",
    "300.16",
    "300.19",
    "300.2",
    "300.20",
    "300.21",
    "300.22",
    "300.23",
    "300.29",
    "300.3",
    "300.4",
    "300.5",
    "300.6",
    "300.7",
    "300.8",
    "300.81",
    "300.82",
    "300.89",
    "300.9",
    "301",
    "301.0",
    "301.1",
    "301.10",
    "301.11",
    "301.12",
    "301.13",
    "301.2",
    "301.20",
    "301.21",
    "301.22",
    "301.3",
    "301.4",
    "301.5",
    "301.50",
    "301.51",
    "301.59",
    "301.6",
    "301.7",
    "301.8",
    "301.81",
    "301.82",
    "301.83",
    "301.84",
    "301.89",
    "301.9",
    "302",
    "302.0",
    "302.1",
    "302.2",
    "302.3",
    "302.4",
    "302.5",
    "302.50",
    "302.51",
    "302.52",
    "302.53",
    "302.6",
    "302.7",
    "302.70",
    "302.71",
    "302.72",
    "302.73",
    "302.74",
    "302.75",
    "302.76",
    "302.79",
    "302.8",
    "302.81",
    "302.82",
    "302.83",
    "302.84",
    "302.85",
    "302.89",
    "302.9",
    "303",
    "303.0",
    "303.0x",
    "303.9",
    "304",
    "304.0",
    "304.1",
    "304.2",
    "304.3",
    "304.4",
    "304.5",
    "304.6",
    "304.7",
    "304.8",
    "304.9",
    "305",
    "305.0",
    "305.0x",
    "305.1",
    "305.2",
    "305.2x",
    "305.3",
    "305.3x",
    "305.4",
    "305.4x",
    "305.5",
    "305.5x",
    "305.6",
    "305.6x",
    "305.7",
    "305.7x",
    "305.8",
    "305.8x",
    "305.9",
    "305.9x",
    "306",
    "306.0",
    "306.1",
    "306.2",
    "306.3",
    "306.4",
    "306.50",
    "306.51",
    "306.52",
    "306.53",
    "306.59",
    "306.6",
    "306.7",
    "306.8",
    "306.9",
    "307",
    "307.0",
    "307.1",
    "307.2",
    "307.20",
    "307.21",
    "307.22",
    "307.23",
    "307.3",
    "307.4",
    "307.40",
    "307.41",
    "307.42",
    "307.43",
    "307.44",
    "307.45",
    "307.46",
    "307.47",
    "307.48",
    "307.49",
    "307.5",
    "307.50",
    "307.51",
    "307.52",
    "307.53",
    "307.54",
    "307.59",
    "307.6",
    "307.7",
    "307.8",
    "307.80",
    "307.81",
    "307.89",
    "307.9",
    "308",
    "308.0",
    "308.1",
    "308.2",
    "308.3",
    "308.4",
    "308.9",
    "309",
    "309.0",
    "309.1",
    "309.2",
    "309.21",
    "309.22",
    "309.23",
    "309.24",
    "309.28",
    "309.29",
    "309.3",
    "309.4",
    "309.8",
    "309.81",
    "309.82",
    "309.83",
    "309.89",
    "309.9",
    "310",
    "310.0",
    "310.1",
    "310.2",
    "310.8",
    "310.81",
    "310.89",
    "310.9",
    "311",
    "312",
    "312.0",
    "312.00",
    "312.01",
    "312.02",
    "312.03",
    "312.1",
    "312.10",
    "312.11",
    "312.12",
    "312.13",
    "312.2",
    "312.20",
    "312.21",
    "312.22",
    "312.23",
    "312.3",
    "312.30",
    "312.31",
    "312.32",
    "312.33",
    "312.34",
    "312.35",
    "312.39",
    "312.4",
    "312.8",
    "312.81",
    "312.82",
    "312.89",
    "312.9",
    "313",
    "313.0",
    "313.1",
    "313.2",
    "313.21",
    "313.22",
    "313.23",
    "313.3",
    "313.8",
    "313.81",
    "313.82",
    "313.83",
    "313.89",
    "313.9",
    "314",
    "314.0",
    "314.00",
    "314.01",
    "314.1",
    "314.2",
    "314.8",
    "314.9",
    "315",
    "315.0",
    "315.00",
    "315.01",
    "315.02",
    "315.09",
    "315.1",
    "315.2",
    "315.3",
    "315.31",
    "315.32",
    "315.34",
    "315.35",
    "315.39",
    "315.4",
    "315.5",
    "315.8",
    "315.9",
    "316",

    "range_317_319",
    "317",
    "318",
    "318.0",
    "318.1",
    "318.2",
    "319",

    "range_V71_V82",
    "V71.09",
};
