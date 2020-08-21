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
#include <QString>


// ========================================================================
// PRINCIPLES: (see internationalization.rst)
// ========================================================================
//
// - If a string is used only once, it can live in the source code (*).
//
// - If a string is re-used on the tablet, it should live here (*).
//
// - If a string is shared between the tablet and server (but in a way that
//   is not task-specific), it should live in the "camcops" namespace of the
//   downloaded strings, unless it is "mission-critical" for the tablet.
//   Things that live in the "camcops" XML namespace should be referenced in
//   the "appstrings" namespace, in common/appstrings.h
//
// - If a string is task-specific, it should live in the task's namespace of
//   the downloaded strings. (It can be referenced by a free-floating name
//   string, or if used more than once, by a local variable.)
//
// (*) Strings that live in the source code, and involve some language, should
// use the Qt tr() mechanism to support internationalization. (The downloaded
// string system already supports this, as users can swap in a different string
// file.)
//
// Note also that proper use of the internationalization system via Qt Linguist
// leads to function calls rather than constants; see internationalization.rst.


class TextConst : public QObject
{
    Q_OBJECT
public:

    // ========================================================================
    // Common text
    // ========================================================================
    
    static QString abnormal();
    static QString abort();
    static QString add();
    
    static QString back();
    
    static QString cancel();
    static QString category();
    static QString clinician();
    static QString clinicianAndRespondentDetails();
    static QString clinicianComments();
    static QString clinicianContactDetails();
    static QString clinicianDetails();
    static QString clinicianName();
    static QString clinicianPost();
    static QString clinicianProfessionalRegistration();
    static QString clinicianService();
    static QString clinicianSpecialty();
    static QString cliniciansComments();
    static QString comment();
    static QString comments();
    static QString copy();
    static QString correct();
    
    static QString dataCollectionOnlyAnnouncement();
    static QString DATA_COLLECTION_ONLY_SYMBOL;
    static QString dataCollectionOnlySubtitleSuffix();
    static QString DATA_COLLECTION_ONLY_UNLESS_UPGRADED_SYMBOL;
    static QString dataCollectionOnlyUnlessUpgradedSubtitleSuffix();
    static QString defaultHintText();
    static QString DEFUNCT_SYMBOL;
    static QString defunctSubtitleSuffix();
    static QString delete_();  // "delete" is a C++ keyword; "DELETE" also breaks Visual C++
    static QString description();
    static QString diagnosis();
    
    static QString EXPERIMENTAL_SYMBOL;
    static QString experimentalSubtitleSuffix();

    static QString HAS_CLINICIAN_SYMBOL;
    static QString hasClinicianSubtitleSuffix();
    static QString HAS_RESPONDENT_SYMBOL;
    static QString hasRespondentSubtitleSuffix();
    
    static QString finished();
    static QString fullTask();
    
    static QString enterTheAnswers();
    static QString examinerComments();
    static QString examinerCommentsPrompt();
    
    static QString icd10();
    static QString idNumberType();
    static QString inAddition();
    static QString incorrect();
    
    static QString location();
    
    static QString meetsCriteria();
    static QString mild();
    static QString mildToModerate();
    static QString moderate();
    static QString moderateToSevere();
    static QString moderatelySevere();
    static QString moveDown();
    static QString moveUp();
    
    static QString na();  // as in "N/A", short for "not applicable"
    static QString no();
    static QString none();
    static QString noDetailSeeFacsimile();
    static QString noSummarySeeFacsimile();
    static QString normal();
    static QString notApplicable();
    static QString notRecalled();
    static QString notSpecified();
    static QString note();
    
    static QString of();
    static QString off();
    static QString ok();
    static QString on();
    
    static QString page();
    static QString part();
    static QString patient();
    static QString pleaseWait();
    static QString pressNextToContinue();
    
    static QString question();
    
    static QString rating();
    static QString reallyAbort();
    static QString recalled();
    static QString respondentDetails();
    static QString respondentNameSecondPerson();
    static QString respondentNameThirdPerson();
    static QString respondentRelationshipSecondPerson();
    static QString respondentRelationshipThirdPerson();
    
    static QString saving();
    static QString score();
    static QString seeFacsimile();
    static QString seeFacsimileForMoreDetail();
    static QString service();
    static QString severe();
    static QString sex();
    static QString startChainQuestion();
    static QString startChainTitle();
    static QString soundTestFor();
    
    static QString thankYou();
    static QString thankYouTouchToExit();
    static QString totalScore();
    static QString touchToStart();

    static QString txtAnd();  // "and" is a C++ keyword
    // ... since C++98;
    // https://stackoverflow.com/questions/2419805/when-did-and-become-an-operator-in-c
    static QString txtFalse();  // "false" is a C++ keyword
    static QString txtTrue();  // "true" is a C++ keyword

    static QString unableToCreateMediaPlayer();
    static QString unknown();
    
    static QString verySevere();
    
    static QString wrong();
    
    static QString yes();
    
    // ========================================================================
    // Terms and conditions
    // ========================================================================
    
    static QString termsConditions();
    
    // ========================================================================
    // Test text
    // ========================================================================
    
    static QString LOREM_IPSUM_1;
    static QString LOREM_IPSUM_2;
    static QString LOREM_IPSUM_3;

};


extern const TextConst textconst;
