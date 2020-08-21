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

// The permissions from the WHO regarding ICD-10 codes do not fully guarantee
// further adaptation of the software (beyond distribution to users) and so is
// likely incompatible with GPL-based licenses. Therefore, the code here (fine)
// should be split from the ICD-10 code text (potentially not fine). We achieve
// that split by using the "downloadable extra strings" system (which is under
// the control of users, not this software).

#include "icd10.h"
#include <QDebug>
#include <QMap>
#include <QObject>

const QString Icd10::XSTRING_TASKNAME("icd10");


// ============================================================================
// Main functions
// ============================================================================

Icd10::Icd10(CamcopsApp& app, QObject* parent,
             bool dummy_creation_no_xstrings) :
    DiagnosticCodeSet(app, XSTRING_TASKNAME, "ICD-10",
                      parent, dummy_creation_no_xstrings)
{
    m_creation_stack.push(DepthItemPair(0, nullptr));  // root: depth 0, no parent
    addIcd10Codes(BASE_CODES);
}


void Icd10::addIcd10Codes(const QStringList& codes)
{
    // Conceptually:
    // - If a new code is longer than the preceding, it should go in as a child
    //   of its predecessor.
    // - If a new code is shorter than the preceding, it should go in as a
    //   child of the most recent predecessor that is shorter than it.
    // - So that's a general rule: insert as a parent of the first predecessor
    //   that is shorter than it. (And ensure that the root code is shorter
    //   than anything!)
    for (const QString& c : codes) {
        const QString desc = xstring(c);
        const int length = c.length();

        const bool show_code_in_full_name = length > 2;
        addIndividualIcd10Code(c, desc, show_code_in_full_name);

        // Any special sub-codes?
        if (length == 5 && (c.startsWith("F00") ||
                            c.startsWith("F01") ||
                            c.startsWith("F02") ||
                            c.startsWith("F03"))) {
            // Dementias that specify subcodes
            addDementia(c, desc);
        } else if (length == 3 && c.startsWith("F1")) {
            // Substance-induced disorders
            addSubstance(c, desc);
        } else if (length == 4 && c.startsWith("F20.")) {
            // Types of schizophrenia
            addSchizophrenia(c, desc);
        } else if (length == 3 && c.startsWith("X") && (c.startsWith("X6") ||
                                                        c.startsWith("X7") ||
                                                        c == "X80" ||
                                                        c == "X81" ||
                                                        c == "X82" ||
                                                        c == "X83" ||
                                                        c == "X84")) {
            // Self harm
            // ... somewhat conservative criteria in case we ever add other
            // X codes, because X85-Y09 is "assault";
            // plus the initial check for "X" for speed when it isn't.
            addSelfHarm(c, desc);
        }
    }
}


void Icd10::addIndividualIcd10Code(const QString& code, const QString& desc,
                                   const bool show_code_in_full_name)
{
    if (code.isEmpty()) {
        qCritical() << Q_FUNC_INFO << "zero-length code! Ignoring";
        return;
    }

    // Establish depth of new one
    int depth = code.length();
    // do any depth modifications here, if required:
    if (depth == 3 && code == "F99") {
        // F99 doesn't sit within the rest of the F9 block; it's its own thing
        depth = 2;
    }

    while (depth <= m_creation_stack.top().first) {
        m_creation_stack.pop();
    }
    DiagnosticCode* parent = m_creation_stack.top().second;
    const bool selectable = code.length() > 2;
    DiagnosticCode* newchild = addCode(parent, code, desc, selectable,
                                       show_code_in_full_name);
    m_creation_stack.push(DepthItemPair(depth, newchild));
}


void Icd10::addSubcodes(const QString& basecode,
                        const QString& basedesc,
                        const QVector<CodeDescriptionPair>& level1)
{
    for (auto extra1 : level1) {
        const QString code = QString("%1%2").arg(basecode, extra1.first);
        const QString desc = QString("%1: %2").arg(
                    basedesc, xstring(extra1.second));
        addIndividualIcd10Code(code, desc);
    }
}


void Icd10::addSubcodes(const QString& basecode,
                        const QString& basedesc,
                        const QVector<CodeDescriptionPair>& level1,
                        const QVector<CodeDescriptionPair>& level2)
{
    for (auto extra1 : level1) {
        const QString l1code = QString("%1%2").arg(basecode, extra1.first);
        const QString l1desc = QString("%1: %2").arg(
                    basedesc, xstring(extra1.second));
        addIndividualIcd10Code(l1code, l1desc);
        for (auto extra2 : level2) {
            const QString l2code = QString("%1%2").arg(l1code, extra2.first);
            const QString l2desc = QString("%1: %2").arg(
                        l1desc, xstring(extra2.second));
            addIndividualIcd10Code(l2code, l2desc);
        }
    }
}
// ============================================================================
// Dementia
// ============================================================================

const QVector<Icd10::CodeDescriptionPair> Icd10::DEMENTIA_L1{
    // The F0x.x0 - F0x.x4 codes
    {"0", "dementia_x0"},
    {"1", "dementia_x1"},
    {"2", "dementia_x2"},
    {"3", "dementia_x3"},
    {"4", "dementia_x4"},
};

const QVector<Icd10::CodeDescriptionPair> Icd10::DEMENTIA_L2{
    // The F0x.xx0 - F0x.xx2 codes
    {"0", "dementia_xx0"},
    {"1", "dementia_xx1"},
    {"2", "dementia_xx2"},
};


void Icd10::addDementia(const QString& basecode, const QString& basedesc)
{
    addSubcodes(basecode, basedesc, DEMENTIA_L1, DEMENTIA_L2);
}


// ============================================================================
// Substance-induced
// ============================================================================

const QVector<Icd10::CodeDescriptionPair> Icd10::SUBSTANCE_L1{
    // Possible to be slightly more memory-efficient, but it just gets a bit
    // complicated to read! This is clearer.
    {".0", "substance_0"},
    {".00", "substance_00"},
    {".01", "substance_01"},
    {".02", "substance_02"},
    {".03", "substance_03"},
    {".04", "substance_04"},
    {".05", "substance_05"},
    {".06", "substance_06"},
    {".07", "substance_07"},  // ALCOHOL ONLY (F10.07)
    {".1", "substance_1"},
    {".2", "substance_2"},
    {".20", "substance_20"},
    {".200", "substance_200"},
    {".201", "substance_201"},
    {".202", "substance_202"},
    {".21", "substance_21"},
    {".22", "substance_22"},
    {".23", "substance_23"},
    {".24", "substance_24"},
    {".241", "substance_241"},
    {".242", "substance_242"},
    {".25", "substance_25"},
    {".26", "substance_26"},  // FOR ALCOHOL: APPEND substance_26_alcohol_suffix
    {".3", "substance_3"},
    {".30", "substance_30"},
    {".31", "substance_31"},
    {".4", "substance_4"},
    {".40", "substance_40"},
    {".41", "substance_41"},
    {".5", "substance_5"},
    {".50", "substance_50"},
    {".51", "substance_51"},
    {".52", "substance_52"},
    {".53", "substance_53"},
    {".54", "substance_54"},
    {".55", "substance_55"},
    {".56", "substance_56"},
    {".6", "substance_6"},
    {".7", "substance_7"},
    {".70", "substance_70"},
    {".71", "substance_71"},
    {".72", "substance_72"},
    {".73", "substance_73"},
    {".74", "substance_74"},
    {".75", "substance_75"},
    {".8", "substance_8"},
    {".9", "substance_9"},
};


void Icd10::addSubstance(const QString& basecode, const QString& basedesc)
{
    const bool alcohol = basecode == "F10";
    for (auto cdp : SUBSTANCE_L1) {
        const QString subcode = cdp.first;
        if (!alcohol && subcode == ".07") {
            continue;
        }
        QString subdesc = xstring(cdp.second);
        if (alcohol && subcode == ".26") {
            subdesc += xstring("substance_26_alcohol_suffix");
        }
        const QString code = QString("%1%2").arg(basecode, subcode);
        const QString desc = QString("%1: %2").arg(basedesc, subdesc);
        addIndividualIcd10Code(code, desc);
    }
}


// ============================================================================
// Schizophrenia
// ============================================================================

const QVector<Icd10::CodeDescriptionPair> Icd10::SCHIZOPHRENIA_L1{
    // The F20.x0 - F20.x9 codes
    {"0", "schizophrenia_x0"},
    {"1", "schizophrenia_x1"},
    {"2", "schizophrenia_x2"},
    {"3", "schizophrenia_x3"},
    {"4", "schizophrenia_x4"},
    {"5", "schizophrenia_x5"},
    // no 6 or 7
    {"8", "schizophrenia_x8"},
    {"9", "schizophrenia_x9"},
};


void Icd10::addSchizophrenia(const QString& basecode, const QString& basedesc)
{
    addSubcodes(basecode, basedesc, SCHIZOPHRENIA_L1);
}


// ============================================================================
// Self-harm
// ============================================================================

const QVector<Icd10::CodeDescriptionPair> Icd10::SELFHARM_L1{
    // The Xxx.0 - Xxx.9 codes
    {".0", "selfharm_0"},
    {".1", "selfharm_1"},
    {".2", "selfharm_2"},
    {".3", "selfharm_3"},
    {".4", "selfharm_4"},
    {".5", "selfharm_5"},
    {".6", "selfharm_6"},
    {".7", "selfharm_7"},
    {".8", "selfharm_8"},
    {".9", "selfharm_9"},
};


const QVector<Icd10::CodeDescriptionPair> Icd10::SELFHARM_L2{
    // The Xxx.x0 - Xxx.x9 codes
    {"0", "selfharm_x0"},
    {"1", "selfharm_x1"},
    {"2", "selfharm_x2"},
    {"3", "selfharm_x3"},
    {"4", "selfharm_x4"},
    // no 5-7
    {"8", "selfharm_x8"},
    {"9", "selfharm_x9"},
};


void Icd10::addSelfHarm(const QString& basecode, const QString& basedesc)
{
    addSubcodes(basecode, basedesc, SELFHARM_L1, SELFHARM_L2);
}


// ============================================================================
// Main codes
// ============================================================================

const QStringList Icd10::BASE_CODES{
    // F
    "F",

    "F0",
    "F00",
    "F00.0",
    "F00.1",
    "F00.2",
    "F00.9",
    "F01",
    "F01.0",
    "F01.1",
    "F01.2",
    "F01.3",
    "F01.8",
    "F01.9",
    "F02",
    "F02.0",
    "F02.1",
    "F02.2",
    "F02.3",
    "F02.4",
    "F02.8",
    "F03",
    "F03.0",
    "F04",
    "F05",
    "F05.0",
    "F05.1",
    "F05.8",
    "F05.9",
    "F06",
    "F06.0",
    "F06.1",
    "F06.2",
    "F06.3",
    "F06.4",
    "F06.5",
    "F06.6",
    "F06.7",
    "F06.8",
    "F06.9",
    "F07",
    "F07.0",
    "F07.1",
    "F07.2",
    "F07.8",
    "F07.9",
    "F09",

    "F1",
    "F10",
    "F11",
    "F12",
    "F13",
    "F14",
    "F15",
    "F16",
    "F17",
    "F18",
    "F19",

    "F2",
    "F20",
    "F20.0",
    "F20.1",
    "F20.2",
    "F20.3",
    "F20.4",
    "F20.5",
    "F20.6",
    "F20.8",
    "F20.9",
    "F21",
    "F22",
    "F22.0",
    "F22.8",
    "F22.9",
    "F23",
    "F23.0",
    "F23.1",
    "F23.2",
    "F23.3",
    "F23.8",
    "F23.9",
    "F23.90",
    "F23.91",
    "F24",
    "F25",
    "F25.0",
    "F25.00",
    "F25.01",
    "F25.1",
    "F25.10",
    "F25.11",
    "F25.2",
    "F25.20",
    "F25.21",
    "F25.8",
    "F25.80",
    "F25.81",
    "F25.9",
    "F25.90",
    "F25.91",
    "F28",
    "F29",

    "F3",
    "F30",
    "F30.0",
    "F30.1",
    "F30.2",
    "F30.20",
    "F30.21",
    "F30.8",
    "F30.9",
    "F31",
    "F31.0",
    "F31.1",
    "F31.2",
    "F31.20",
    "F31.21",
    "F31.3",
    "F31.30",
    "F31.31",
    "F31.4",
    "F31.5",
    "F31.50",
    "F31.51",
    "F31.6",
    "F31.7",
    "F31.8",
    "F31.9",
    "F32",
    "F32.0",
    "F32.00",
    "F32.01",
    "F32.1",
    "F32.10",
    "F32.11",
    "F32.2",
    "F32.3",
    "F32.30",
    "F32.31",
    "F32.8",
    "F32.9",
    "F33",
    "F33.0",
    "F33.00",
    "F33.01",
    "F33.1",
    "F33.10",
    "F33.11",
    "F33.2",
    "F33.3",
    "F33.30",
    "F33.31",
    "F33.4",
    "F33.8",
    "F33.9",
    "F34",
    "F34.0",
    "F34.1",
    "F34.8",
    "F34.9",
    "F38",
    "F38.0",
    "F38.00",
    "F38.1",
    "F38.10",
    "F38.8",
    "F39",

    "F4",
    "F40",
    "F40.0",
    "F40.00",
    "F40.01",
    "F40.1",
    "F40.2",
    "F40.8",
    "F40.9",
    "F41",
    "F41.0",
    "F41.00",
    "F41.01",
    "F41.1",
    "F41.2",
    "F41.3",
    "F41.8",
    "F41.9",
    "F42",
    "F42.0",
    "F42.1",
    "F42.2",
    "F42.8",
    "F42.9",
    "F43",
    "F43.0",
    "F43.00",
    "F43.01",
    "F43.02",
    "F43.1",
    "F43.2",
    "F43.20",
    "F43.21",
    "F43.22",
    "F43.23",
    "F43.24",
    "F43.25",
    "F43.28",
    "F43.8",
    "F43.9",
    "F44",
    "F44.0",
    "F44.1",
    "F44.2",
    "F44.3",
    "F44.4",
    "F44.5",
    "F44.6",
    "F44.7",
    "F44.8",
    "F44.80",
    "F44.81",
    "F44.82",
    "F44.88",
    "F44.9",
    "F45",
    "F45.0",
    "F45.1",
    "F45.2",
    "F45.3",
    "F45.30",
    "F45.31",
    "F45.32",
    "F45.33",
    "F45.34",
    "F45.38",
    "F45.4",
    "F45.8",
    "F45.9",
    "F48",
    "F48.0",
    "F48.1",
    "F48.8",
    "F48.9",

    "F5",
    "F50",
    "F50.0",
    "F50.1",
    "F50.2",
    "F50.3",
    "F50.4",
    "F50.5",
    "F50.8",
    "F50.9",
    "F51",
    "F51.0",
    "F51.1",
    "F51.2",
    "F51.3",
    "F51.4",
    "F51.5",
    "F51.8",
    "F51.9",
    "F52",
    "F52.0",
    "F52.1",
    "F52.10",
    "F52.11",
    "F52.2",
    "F52.3",
    "F52.4",
    "F52.5",
    "F52.6",
    "F52.7",
    "F52.8",
    "F52.9",
    "F53",
    "F53.0",
    "F53.1",
    "F53.8",
    "F53.9",
    "F54",
    "F55",
    "F59",

    "F6",
    "F60",
    "F60.0",
    "F60.1",
    "F60.2",
    "F60.3",
    "F60.30",
    "F60.31",
    "F60.4",
    "F60.5",
    "F60.6",
    "F60.7",
    "F60.8",
    "F60.9",
    "F61",
    "F62",
    "F62.0",
    "F62.1",
    "F62.8",
    "F62.9",
    "F63",
    "F63.0",
    "F63.1",
    "F63.2",
    "F63.3",
    "F63.8",
    "F63.9",
    "F64",
    "F64.0",
    "F64.1",
    "F64.2",
    "F64.8",
    "F64.9",
    "F65",
    "F65.0",
    "F65.1",
    "F65.2",
    "F65.3",
    "F65.4",
    "F65.5",
    "F65.6",
    "F65.8",
    "F65.9",
    "F66",
    "F66.0",
    "F66.1",
    "F66.2",
    "F66.8",
    "F66.9",
    "F66.90",
    "F66.91",
    "F66.92",
    "F66.98",
    "F68",
    "F68.0",
    "F68.1",
    "F68.8",
    "F69",

    "F7",
    "F70",
    "F70.0",
    "F70.1",
    "F70.8",
    "F70.9",
    "F71",
    "F71.0",
    "F71.1",
    "F71.8",
    "F71.9",
    "F72",
    "F72.0",
    "F72.1",
    "F72.8",
    "F72.9",
    "F73",
    "F73.0",
    "F73.1",
    "F73.8",
    "F73.9",
    "F78",
    "F78.0",
    "F78.1",
    "F78.8",
    "F78.9",
    "F79",
    "F79.0",
    "F79.1",
    "F79.8",
    "F79.9",

    "F8",
    "F80",
    "F80.0",
    "F80.1",
    "F80.2",
    "F80.3",
    "F80.8",
    "F80.9",
    "F81",
    "F81.0",
    "F81.1",
    "F81.2",
    "F81.3",
    "F81.8",
    "F81.9",
    "F82",
    "F83",
    "F84",
    "F84.0",
    "F84.1",
    "F84.10",
    "F84.11",
    "F84.12",
    "F84.2",
    "F84.3",
    "F84.4",
    "F84.5",
    "F84.8",
    "F84.9",
    "F88",
    "F89",
    "F9",
    "F90",
    "F90.0",
    "F90.1",
    "F90.8",
    "F90.9",
    "F91",
    "F91.0",
    "F91.1",
    "F91.2",
    "F91.3",
    "F91.8",
    "F91.9",
    "F92",
    "F92.0",
    "F92.8",
    "F92.9",
    "F93",
    "F93.0",
    "F93.1",
    "F93.2",
    "F93.3",
    "F93.8",
    "F93.80",
    "F93.9",
    "F94",
    "F94.0",
    "F94.1",
    "F94.2",
    "F94.8",
    "F94.9",
    "F95",
    "F95.0",
    "F95.1",
    "F95.2",
    "F95.8",
    "F95.9",
    "F98",
    "F98.0",
    "F98.00",
    "F98.01",
    "F98.02",
    "F98.1",
    "F98.10",
    "F98.11",
    "F98.12",
    "F98.2",
    "F98.3",
    "F98.4",
    "F98.40",
    "F98.41",
    "F98.42",
    "F98.5",
    "F98.6",
    "F98.8",
    "F98.9",

    "F99",

    // X
    "R",
    "R40",
    "R40.0",
    "R40.1",
    "R40.2",
    "R41",
    "R41.0",
    "R41.1",
    "R41.2",
    "R41.3",
    "R41.8",
    "R42",
    "R43",
    "R43.0",
    "R43.1",
    "R43.2",
    "R43.8",
    "R44",
    "R44.0",
    "R44.1",
    "R44.2",
    "R44.3",
    "R44.8",
    "R45",
    "R45.0",
    "R45.1",
    "R45.2",
    "R45.3",
    "R45.4",
    "R45.5",
    "R45.6",
    "R45.7",
    "R45.8",
    "R46",
    "R46.0",
    "R46.1",
    "R46.2",
    "R46.3",
    "R46.4",
    "R46.5",
    "R46.6",
    "R46.7",
    "R46.8",

    // X
    "X",
    "X60",
    "X61",
    "X62",
    "X63",
    "X64",
    "X65",
    "X66",
    "X67",
    "X68",
    "X69",
    "X70",
    "X71",
    "X72",
    "X73",
    "X74",
    "X75",
    "X76",
    "X77",
    "X78",
    "X79",
    "X80",
    "X81",
    "X82",
    "X83",
    "X84",

    // Z
    "Z",
    "Z00.4",
    "Z03.2",
    "Z71.1",
};
