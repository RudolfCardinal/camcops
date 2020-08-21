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
#include <QCoreApplication>  // for Q_DECLARE_TR_FUNCTIONS
#include <QPair>
#include <QStack>
#include "diagnosticcodeset.h"

class CamcopsApp;


// Represents the ICD-9-CM (= DSM-IV) diagnostic system.

class Icd9cm : public DiagnosticCodeSet
{
    Q_OBJECT

public:
    Icd9cm(CamcopsApp& app, QObject* parent = nullptr,
           bool dummy_creation_no_xstrings = false);

    using CodeDescriptionPair = QPair<QString, QString>;
    using DepthItemPair = QPair<int, DiagnosticCode*>;
private:
    void addIcd9cmCodes(const QStringList& codes);
    void addIndividualIcd9cmCode(const QString& code, const QString& desc,
                                 bool show_code_in_full_name = true);
    void addSubcodes(const QString& basecode,
                     const QString& basedesc,
                     const QVector<CodeDescriptionPair>& level1);

    QStack<DepthItemPair> m_creation_stack;  // depth, index (of parents)

    void addEpisodicAffective(const QString& basecode,
                              const QString& basedesc);
    void addSubstance(const QString& basecode, const QString& basedesc);
    void addSchizophrenia(const QString& basecode, const QString& basedesc);

    static const QStringList BASE_CODES;
    static const QVector<CodeDescriptionPair> EPISODIC_AFFECTIVE_L1;
    static const QVector<CodeDescriptionPair> SUBSTANCE_L1;
    static const QVector<CodeDescriptionPair> SCHIZOPHRENIA_L1;
public:
    static const QString XSTRING_TASKNAME;
};
