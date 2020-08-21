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

#include "textconst.h"
#include <QObject>

// ============================================================================
// Common text via tr()
// ============================================================================

QString TextConst::abnormal() { return tr("Abnormal"); }
QString TextConst::abort() { return tr("Abort"); }
QString TextConst::add() { return tr("Add"); }

QString TextConst::back() { return tr("Back"); }

QString TextConst::cancel() { return tr("Cancel"); }
QString TextConst::category() { return tr("Category"); }
QString TextConst::clinician() { return tr("Clinician/researcher"); }
QString TextConst::clinicianAndRespondentDetails() { return tr("Clinician/researcher’s and respondent’s details"); }
QString TextConst::clinicianComments() { return tr("Clinician/researcher’s comments"); }
QString TextConst::clinicianContactDetails() { return tr("Clinician/researcher’s contact details"); }
QString TextConst::clinicianDetails() { return tr("Clinician/researcher’s details"); }
QString TextConst::clinicianName() { return tr("Clinician/researcher’s name"); }
QString TextConst::clinicianPost() { return tr("Clinician/researcher’s post"); }
QString TextConst::clinicianProfessionalRegistration() { return tr("Clinician/researcher’s professional registration"); }
QString TextConst::clinicianService() { return tr("Clinician/researcher’s service"); }
QString TextConst::clinicianSpecialty() { return tr("Clinician/researcher’s specialty"); }
QString TextConst::cliniciansComments() { return tr("Clinician/researcher’s comments"); }
QString TextConst::comment() { return tr("Comment"); }
QString TextConst::comments() { return tr("Comments"); }
QString TextConst::copy() { return tr("Copy"); }
QString TextConst::correct() { return tr("Correct"); }

QString TextConst::dataCollectionOnlyAnnouncement()
{
    return tr(
       "Reproduction of this task/scale is not permitted. This is a data "
       "collection tool only; use it only in conjunction with a licensed copy "
       "of the original task.");
}
QString TextConst::DATA_COLLECTION_ONLY_SYMBOL("¶");
QString TextConst::dataCollectionOnlySubtitleSuffix()
{
    return tr("Data collection tool only.");
}
QString TextConst::DATA_COLLECTION_ONLY_UNLESS_UPGRADED_SYMBOL("¶+");
QString TextConst::dataCollectionOnlyUnlessUpgradedSubtitleSuffix()
{
    return tr("Data collection tool only unless host "
              "institution adds scale text.");
}
QString TextConst::defaultHintText() { return tr("type text here"); }
QString TextConst::DEFUNCT_SYMBOL("†");
QString TextConst::defunctSubtitleSuffix()
{
    return tr("Defunct.");
}
QString TextConst::delete_() { return tr("Delete"); }
QString TextConst::description() { return tr("Description"); }
QString TextConst::diagnosis() { return tr("Diagnosis"); }

QString TextConst::EXPERIMENTAL_SYMBOL("~");
QString TextConst::experimentalSubtitleSuffix()
{
    return tr("Experimental.");
}

QString TextConst::enterTheAnswers() { return tr("Enter the answers:"); }
QString TextConst::examinerComments() { return tr("Examiner’s comments"); }
QString TextConst::examinerCommentsPrompt()
{
    return tr("Optional — Examiner’s comments (e.g. "
              "difficulties the subject had with the task):");
}

QString TextConst::finished() { return tr("Finished"); }
QString TextConst::fullTask() { return tr("Full task without content restriction"); }

QString TextConst::HAS_CLINICIAN_SYMBOL("C");
QString TextConst::hasClinicianSubtitleSuffix() {
    return tr("Involves assessment by clinician/researcher.");
}
QString TextConst::HAS_RESPONDENT_SYMBOL("R");
QString TextConst::hasRespondentSubtitleSuffix() {
    return tr("Collects information from respondent other than the subject.");
}

QString TextConst::icd10() {
    return tr("World Health Organization International Classification of "
              "Diseases, 10th edition.");
}
QString TextConst::idNumberType() { return tr("ID number type"); }
QString TextConst::inAddition() { return tr("In addition"); }
QString TextConst::incorrect() { return tr("Incorrect"); }

QString TextConst::location() { return tr("Location"); }

QString TextConst::meetsCriteria() { return tr("Meets criteria"); }
QString TextConst::mild() { return tr("Mild"); }
QString TextConst::mildToModerate() { return tr("Mild to moderate"); }
QString TextConst::moderatelySevere() { return tr("Moderately severe"); }
QString TextConst::moderateToSevere() { return tr("Moderate to severe"); }
QString TextConst::moderate() { return tr("Moderate"); }
QString TextConst::moveDown() { return tr("Move down"); }
QString TextConst::moveUp() { return tr("Move up"); }

QString TextConst::na() { return tr("N/A"); }
QString TextConst::none() { return tr("None"); }
QString TextConst::no() { return tr("No"); }
QString TextConst::noDetailSeeFacsimile() { return tr("No detail available; see the facsimile instead"); }
QString TextConst::noSummarySeeFacsimile() { return tr("No summary available; see the facsimile instead"); }
QString TextConst::normal() { return tr("Normal"); }
QString TextConst::notApplicable() { return tr("Not applicable"); }
QString TextConst::notRecalled() { return tr("Not recalled"); }
QString TextConst::notSpecified() { return tr("<not specified>"); }
QString TextConst::note() { return tr("Note"); }

QString TextConst::of() { return tr("of"); }
QString TextConst::off() { return tr("Off"); }
QString TextConst::ok() { return tr("OK"); }
QString TextConst::on() { return tr("On"); }

QString TextConst::page() { return tr("page"); }
QString TextConst::part() { return tr("Part"); }
QString TextConst::patient() { return tr("Patient"); }
QString TextConst::pleaseWait() { return tr("Please wait..."); }
QString TextConst::pressNextToContinue()
{
    return tr(
        "Press the NEXT button (the green right-facing arrow at the TOP RIGHT "
        "of the screen) to continue."
    );
}

QString TextConst::question() { return tr("Question"); }

QString TextConst::rating() { return tr("Rating"); }
QString TextConst::reallyAbort() { return tr("Really abort?"); }
QString TextConst::recalled() { return tr("Recalled"); }
QString TextConst::respondentDetails() { return tr("Respondent’s details"); }
QString TextConst::respondentNameSecondPerson() { return tr("Your name"); }
QString TextConst::respondentNameThirdPerson() { return tr("Respondent’s name"); }
QString TextConst::respondentRelationshipSecondPerson() { return tr("Your relationship to the patient"); }
QString TextConst::respondentRelationshipThirdPerson() { return tr("Respondent’s relationship to patient"); }

QString TextConst::saving() { return tr("Saving, please wait..."); }
QString TextConst::score() { return tr("Score"); }
QString TextConst::seeFacsimile() { return tr("See facsimile."); }
QString TextConst::seeFacsimileForMoreDetail() { return tr("See facsimile for more detail."); }
QString TextConst::service() { return tr("Service"); }
QString TextConst::severe() { return tr("Severe"); }
QString TextConst::sex() { return tr("Sex"); }
QString TextConst::startChainQuestion() { return tr("Start new task chain?"); }
QString TextConst::startChainTitle() { return tr("Start task chain"); }
QString TextConst::soundTestFor() { return tr("Sound test for"); }

QString TextConst::thankYou() { return tr("Thank you!"); }
QString TextConst::thankYouTouchToExit() { return tr("Thank you! Please touch here to exit."); }
QString TextConst::totalScore() { return tr("Total score"); }
QString TextConst::touchToStart() { return tr("When you’re ready, touch here to start."); }

QString TextConst::txtAnd() { return tr("and"); }
QString TextConst::txtTrue() { return tr("True"); }
QString TextConst::txtFalse() { return tr("False"); }

QString TextConst::unableToCreateMediaPlayer() { return tr("Unable to create media player!"); }
QString TextConst::unknown() { return tr("Unknown"); }

QString TextConst::verySevere() { return tr("Very severe"); }

QString TextConst::wrong() { return tr("Wrong"); }

QString TextConst::yes() { return tr("Yes"); }


// ============================================================================
// Terms and conditions
// ============================================================================

QString TextConst::termsConditions() {
    return tr(
        "1. By using the Cambridge Cognitive and Psychiatric Assessment Kit "
        "application or web interface (“CamCOPS”), you are agreeing in full "
        "to these Terms and Conditions of Use. If you do not agree to these "
        "terms, do not use the software.\n\n"

        "2. Content that is original to CamCOPS is licensed as follows.\n\n"

        "CamCOPS is free software: you can redistribute it and/or modify "
        "it under the terms of the GNU General Public License as published by "
        "the Free Software Foundation, either version 3 of the License, or "
        "(at your option) any later version.\n\n"

        "CamCOPS is distributed in the hope that it will be useful, "
        "but WITHOUT ANY WARRANTY; without even the implied warranty of "
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
        "GNU General Public License for more details.\n\n"

        "You should have received a copy of the GNU General Public License "
        "along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.\n\n"

        "3. Content created by others and distributed with CamCOPS may be in "
        "the public domain, or distributed under other licenses or "
        "permissions. THERE MAY BE CRITERIA THAT APPLY TO YOU THAT MEAN YOU "
        "ARE NOT PERMITTED TO USE SPECIFIC TASKS. IT IS YOUR RESPONSIBILITY "
        "TO CHECK THAT YOU ARE LEGALLY ENTITLED TO USE EACH TASK. You agree "
        "that the authors of CamCOPS are not responsible for any consequences "
        "that arise from your use of an unauthorized task.\n\n"

        "4. While efforts have been made to ensure that CamCOPS is reliable "
        "and accurate, you agree that the authors and distributors of CamCOPS "
        "are not responsible for errors, omissions, or defects in the "
        "content, nor liable for any direct, indirect, incidental, special "
        "and/or consequential damages, in whole or in part, resulting from "
        "your use or any user’s use of or reliance upon its content.\n\n"

        "5. Content contained in or accessed through CamCOPS should not be "
        "relied upon for medical purposes in any way. This software is not "
        "designed for use by the general public. If medical advice is "
        "required you should seek expert medical assistance. You agree that "
        "you will not rely on this software for any medical purpose.\n\n"

        "6. Regarding the European Union Council Directive 93/42/EEC of 14 "
        "June 1993 concerning medical devices (amended by further directives "
        "up to and including Directive 2007/47/EC of 5 September 2007) "
        "(“Medical Devices Directive”): CamCOPS is not intended for "
        "the diagnosis and/or monitoring of human disease. If it is used for "
        "such purposes, it must be used EXCLUSIVELY FOR CLINICAL "
        "INVESTIGATIONS in an appropriate setting by persons professionally "
        "qualified to do so. It has NOT undergone a conformity assessment "
        "under the Medical Devices Directive, and thus cannot be marketed or "
        "put into service as a medical device. You agree that you will not "
        "use it as a medical device.\n\n"

        "7. THIS SOFTWARE IS DESIGNED FOR USE BY QUALIFIED CLINICIANS ONLY. "
        "BY CONTINUING TO USE THIS SOFTWARE YOU ARE CONFIRMING THAT YOU ARE "
        "A QUALIFIED CLINICIAN, AND THAT YOU RETAIN RESPONSIBILITY FOR "
        "DIAGNOSIS AND MANAGEMENT.\n\n"

        "These terms and conditions were last revised on 2017-01-23."
    );
    // This should match the DISCLAIMER_CONTENT string in the server_string()
    // function of camcops_server/cc_modules/cc_text.py.
}

// ============================================================================
// Test text
// ============================================================================

QString TextConst::LOREM_IPSUM_1(
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Praesent "
    "sed cursus mauris. Ut vulputate felis quis dolor molestie convallis. "
    "Donec lectus diam, accumsan quis tortor at, congue laoreet augue. Ut "
    "mollis consectetur elit sit amet tincidunt. Vivamus facilisis, mi et "
    "eleifend ullamcorper, lorem metus faucibus ante, ut commodo urna "
    "neque bibendum magna. Lorem ipsum dolor sit amet, consectetur "
    "adipiscing elit. Praesent nec nisi ante. Sed vitae sem et eros "
    "elementum condimentum. Proin porttitor purus justo, sit amet "
    "vulputate velit imperdiet nec. Nam posuere ipsum id nunc accumsan "
    "tristique. Etiam pellentesque ornare tortor, a scelerisque dui "
    "accumsan ac. Ut tincidunt dolor ultrices, placerat urna nec, "
    "scelerisque mi."
);
QString TextConst::LOREM_IPSUM_2(
    "Nunc vitae neque eu odio feugiat consequat ac id neque. "
    "Suspendisse id libero massa."
);
QString TextConst::LOREM_IPSUM_3(
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Praesent "
    "sed cursus mauris. Ut vulputate felis quis dolor molestie convallis. "
    "Donec lectus diam, accumsan quis tortor at, congue laoreet augue. Ut "
    "mollis consectetur elit sit amet tincidunt."
);


// ============================================================================
// Instance
// ============================================================================

const TextConst textconst;
