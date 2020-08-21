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

#include "commonoptions.h"

// ============================================================================
// Database-level constants
// ============================================================================

const QString CommonOptions::NO_CHAR("N");
const QString CommonOptions::YES_CHAR("Y");
const int CommonOptions::UNKNOWN_INT = -1;
const int CommonOptions::NO_INT = 0;
const int CommonOptions::YES_INT = 1;
const int CommonOptions::INCORRECT_INT = 0;
const int CommonOptions::CORRECT_INT = 1;
const QString CommonOptions::SEX_FEMALE("F");
const QString CommonOptions::SEX_MALE("M");
const QString CommonOptions::SEX_UNSPECIFIED("X");


// ============================================================================
// Translatables
// ============================================================================

QString CommonOptions::yes()
{
    return tr("Yes");
}


QString CommonOptions::no()
{
    return tr("No");
}


QString CommonOptions::correct()
{
    return tr("Correct");
}


QString CommonOptions::incorrect()
{
    return tr("Incorrect");
}


QString CommonOptions::false_str()
{
    return tr("False");
}


QString CommonOptions::true_str()
{
    return tr("True");
}


QString CommonOptions::absent()
{
    return tr("Absent");
}


QString CommonOptions::present()
{
    return tr("Present");
}


QString CommonOptions::unknown()
{
    return tr("Unknown");
}


QString CommonOptions::sexFemale()
{
    return tr("Female (F)");
}


QString CommonOptions::sexMale()
{
    return tr("Male (M)");
}


QString CommonOptions::sexUnspecified()
{
    return tr("Other/unspecified (X)");
}


// ============================================================================
// Option sets
// ============================================================================

NameValueOptions CommonOptions::yesNoChar()
{
    return NameValueOptions{
        {yes(), YES_CHAR},
        {no(), NO_CHAR},
    };
}


NameValueOptions CommonOptions::yesNoBoolean()
{
    return NameValueOptions{
        {yes(), true},
        {no(), false},
    };
}


NameValueOptions CommonOptions::yesNoInteger()
{
    return NameValueOptions{
        {yes(), YES_INT},
        {no(), NO_INT},
    };
}


NameValueOptions CommonOptions::noYesChar()
{
    return NameValueOptions{
        {no(), NO_CHAR},
        {yes(), YES_CHAR},
    };
}


NameValueOptions CommonOptions::noYesBoolean()
{
    return NameValueOptions{
        {no(), false},
        {yes(), true},
    };
}


NameValueOptions CommonOptions::noYesInteger()
{
    return NameValueOptions{
        {no(), NO_INT},
        {yes(), YES_INT},
    };
}


NameValueOptions CommonOptions::incorrectCorrectBoolean()
{
    return NameValueOptions{
        {incorrect(), false},
        {correct(), true},
    };
}


NameValueOptions CommonOptions::incorrectCorrectInteger()
{
    return NameValueOptions{
        {incorrect(), INCORRECT_INT},
        {correct(), CORRECT_INT},
    };
}


NameValueOptions CommonOptions::falseTrueBoolean()
{
    return NameValueOptions{
        {false_str(), false},
        {true_str(), true},
    };
}


NameValueOptions CommonOptions::absentPresentBoolean()
{
    return NameValueOptions{
        {absent(), false},
        {present(), true},
    };
}


NameValueOptions CommonOptions::unknownNoYesInteger()
{
    return NameValueOptions{
        {unknown(), UNKNOWN_INT},
        {no(), NO_INT},
        {yes(), YES_INT},
    };
}


NameValueOptions CommonOptions::sexes()
{
    return NameValueOptions{
        {sexFemale(), SEX_FEMALE},
        {sexMale(), SEX_MALE},
        {sexUnspecified(), SEX_UNSPECIFIED},
    };
}


NameValueOptions CommonOptions::optionsCopyingDescriptions(
        const QStringList& descriptions)
{
    NameValueOptions options;
    for (const QString& desc : descriptions) {
        options.append(NameValuePair(desc, desc));
    }
    return options;
}
