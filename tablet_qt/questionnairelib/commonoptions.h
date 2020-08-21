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
#include <QString>
#include "questionnairelib/namevalueoptions.h"

// Encapsulates a number of common strings and NameValueOptions used by
// questionnaires.
//
// Translation doesn't work for static variables, because static variables
// are initialized before QApplication is instantiated.
// We want translation here, so we use a class.
// Anything that needs translation must go via a function.
// Because this is a class, we use static (not extern).

class CommonOptions
{
    Q_DECLARE_TR_FUNCTIONS(CommonOptions)

    // ========================================================================
    // Database-level constants
    // ========================================================================
public:
    static const QString NO_CHAR;
    static const QString YES_CHAR;
    static const int UNKNOWN_INT;
    static const int NO_INT;
    static const int YES_INT;
    static const int INCORRECT_INT;
    static const int CORRECT_INT;
    static const QString SEX_FEMALE;
    static const QString SEX_MALE;
    static const QString SEX_UNSPECIFIED;

    // ========================================================================
    // Translated text
    // ========================================================================
public:
    static QString yes();
    static QString no();
    static QString correct();
    static QString incorrect();
    static QString false_str();
    static QString true_str();
    static QString absent();
    static QString present();
    static QString unknown();
    static QString sexFemale();
    static QString sexMale();
    static QString sexUnspecified();

    // ========================================================================
    // Option sets
    // ========================================================================
public:
    static NameValueOptions yesNoChar();
    static NameValueOptions yesNoBoolean();
    static NameValueOptions yesNoInteger();
    static NameValueOptions noYesChar();
    static NameValueOptions noYesBoolean();
    static NameValueOptions noYesInteger();

    static NameValueOptions incorrectCorrectBoolean();
    static NameValueOptions incorrectCorrectInteger();
    static NameValueOptions falseTrueBoolean();
    static NameValueOptions absentPresentBoolean();
    static NameValueOptions unknownNoYesInteger();

    static NameValueOptions sexes();

    static NameValueOptions optionsCopyingDescriptions(
            const QStringList& descriptions);
};
