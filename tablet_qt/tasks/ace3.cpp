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

#define NOSCROLL_IMAGE_PAGES  // Should be defined. Better UI with it.

#include "ace3.h"
#include <QDebug>
#include "common/textconst.h"
#include "common/uiconst.h"
#include "lib/datetime.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/namevalueoptions.h"
#include "questionnairelib/quboolean.h"
#include "questionnairelib/quflowcontainer.h"
#include "questionnairelib/qugridcontainer.h"
#include "questionnairelib/quverticalcontainer.h"
#include "questionnairelib/qucountdown.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/questionnairefunc.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/quimage.h"
#include "questionnairelib/qulineedit.h"
#include "questionnairelib/qulineeditinteger.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/quphoto.h"
#include "questionnairelib/quspacer.h"
#include "questionnairelib/qutext.h"
#include "tasklib/taskfactory.h"
using mathfunc::eq;
using mathfunc::noneNull;
using mathfunc::sumInt;
using mathfunc::scoreStringWithPercent;
using mathfunc::totalScorePhrase;
using stringfunc::strnum;
using stringfunc::strseq;

const QString Ace3::ACE3_TABLENAME("ace3");

const QString IMAGE_SPOON("ace3/spoon.png");
const QString IMAGE_BOOK("ace3/book.png");
const QString IMAGE_KANGAROO("ace3/kangaroo.png");
const QString IMAGE_PENGUIN("ace3/penguin.png");
const QString IMAGE_ANCHOR("ace3/anchor.png");
const QString IMAGE_CAMEL("ace3/camel.png");
const QString IMAGE_HARP("ace3/harp.png");
const QString IMAGE_RHINOCEROS("ace3/rhinoceros.png");
const QString IMAGE_BARREL("ace3/barrel.png");
const QString IMAGE_CROWN("ace3/crown.png");
const QString IMAGE_CROCODILE("ace3/crocodile.png");
const QString IMAGE_ACCORDION("ace3/accordion.png");
const QString IMAGE_INFINITY("ace3/infinity.png");
const QString IMAGE_CUBE("ace3/cube.png");
const QString IMAGE_DOTS8("ace3/dots8.png");
const QString IMAGE_DOTS10("ace3/dots10.png");
const QString IMAGE_DOTS7("ace3/dots7.png");
const QString IMAGE_DOTS9("ace3/dots9.png");
const QString IMAGE_K("ace3/k.png");
const QString IMAGE_M("ace3/m.png");
const QString IMAGE_A("ace3/a.png");
const QString IMAGE_T("ace3/t.png");

const QString TAG_MEM_RECOGNIZE("mem_recognize");
const QString TAG_PG_LANG_COMMANDS_SENTENCES("pg_lang_commands_sentences");
const QString TAG_PG_MEM_PROMPTED_RECALL("pg_mem_prompted_recall");
const QString TAG_EL_LANG_OPTIONAL_COMMAND("lang_optional_command");
const QString TAG_EL_LANG_NOT_SHOWN("lang_not_shown");
const QString TAG_RECOG_REQUIRED("recog_required");
const QString TAG_RECOG_SUPERFLUOUS("recog_superfluous");
const QString TAG_RECOG_NAME("recog_name");
const QString TAG_RECOG_NUMBER("recog_number");
const QString TAG_RECOG_STREET("recog_street");
const QString TAG_RECOG_TOWN("recog_town");
const QString TAG_RECOG_COUNTY("recog_county");

// Field names, field prefixes, and field counts
const QString FN_AGE_FT_EDUCATION("age_at_leaving_full_time_education");
const QString FN_OCCUPATION("occupation");
const QString FN_HANDEDNESS("handedness");
const QString FP_ATTN_TIME("attn_time");
const int N_ATTN_TIME = 5;
const QString FP_ATTN_PLACE("attn_place");
const int N_ATTN_PLACE = 5;
const QString FP_ATTN_REPEAT_WORD("attn_repeat_word");
const int N_ATTN_REPEAT_WORD = 3;
const QString FN_ATTN_NUM_REGISTRATION_TRIALS("attn_num_registration_trials");
const QString FP_ATTN_SERIAL7("attn_serial7_subtraction");
const int N_ATTN_SERIAL7 = 5;
const QString FP_MEM_RECALL_WORD("mem_recall_word");
const int N_MEM_RECALL_WORD = 3;
const QString FN_FLUENCY_LETTERS_SCORE("fluency_letters_score");
const QString FN_FLUENCY_ANIMALS_SCORE("fluency_animals_score");
const QString FP_MEM_REPEAT_ADDR_TRIAL1("mem_repeat_address_trial1_");
const QString FP_MEM_REPEAT_ADDR_TRIAL2("mem_repeat_address_trial2_");
const QString FP_MEM_REPEAT_ADDR_TRIAL3("mem_repeat_address_trial3_");
const int N_MEM_REPEAT_ADDR = 7;
const QString FP_MEM_FAMOUS("mem_famous");
const int N_MEM_FAMOUS = 4;
const QString FN_LANG_FOLLOW_CMD_PRACTICE("lang_follow_command_practice");
const QString FP_LANG_FOLLOW_CMD("lang_follow_command");
const int N_LANG_FOLLOW_CMD = 3;
const QString FP_LANG_WRITE_SENTENCES_POINT("lang_write_sentences_point");
const int N_LANG_WRITE_SENTENCES_POINT = 2;
const QString FP_LANG_REPEAT_WORD("lang_repeat_word");
const int N_LANG_REPEAT_WORD = 4;
const QString FP_LANG_REPEAT_SENTENCE("lang_repeat_sentence");
const int N_LANG_REPEAT_SENTENCE = 2;
const QString FP_LANG_NAME_PICTURE("lang_name_picture");
const int N_LANG_NAME_PICTURE = 12;
const QString FP_LANG_IDENTIFY_CONCEPT("lang_identify_concept");
const int N_LANG_IDENTIFY_CONCEPT = 4;
const QString FN_LANG_READ_WORDS_ALOUD("lang_read_words_aloud");
const QString FN_VSP_COPY_INFINITY("vsp_copy_infinity");
const QString FN_VSP_COPY_CUBE("vsp_copy_cube");
const QString FN_VSP_DRAW_CLOCK("vsp_draw_clock");
const QString FP_VSP_COUNT_DOTS("vsp_count_dots");
const int N_VSP_COUNT_DOTS = 4;
const QString FP_VSP_IDENTIFY_LETTER("vsp_identify_letter");
const int N_VSP_IDENTIFY_LETTER = 4;
const QString FP_MEM_RECALL_ADDRESS("mem_recall_address");
const int N_MEM_RECALL_ADDRESS = 7;
const QString FP_MEM_RECOGNIZE_ADDRESS_SCORE("mem_recognize_address");  // SCORE; matches versions before 2.0.0
const QString FP_MEM_RECOGNIZE_ADDRESS_CHOICE("mem_recognize_address_choice");  // CHOICE; v2.0.0 onwards
// ... storing raw choices is new in v2.0.0, but the score field is preserved
//     for backwards compatibility
const int N_MEM_RECOGNIZE_ADDRESS = 5;
const QString FN_PICTURE1_BLOBID("picture1_blobid");
// defunct: picture1_rotation
const QString FN_PICTURE2_BLOBID("picture2_blobid");
// defunct: picture2_rotation
const QString FN_COMMENTS("comments");

// Subtotals. No magic numbers...
const int TOTAL_OVERALL = 100;
const int TOTAL_ATTN = 18;
const int TOTAL_MEM = 26;
const int TOTAL_FLUENCY = 14;
const int TOTAL_LANG = 26;
const int TOTAL_VSP = 16;

const int MIN_AGE = 0;
const int MAX_AGE_Y = 120;
const int FLUENCY_TIME_SEC = 60;

// We can't store a char in a variant, so an alternative would be QChar,
// but QString is just as simple.
const QString CHOICE_A("A");
const QString CHOICE_B("B");
const QString CHOICE_C("C");


void initializeAce3(TaskFactory& factory)
{
    static TaskRegistrar<Ace3> registered(factory);
}


Ace3::Ace3(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, ACE3_TABLENAME, false, true, false),  // ... anon, clin, resp
    m_questionnaire(nullptr)
{
    addField(FN_AGE_FT_EDUCATION, QVariant::Int);
    addField(FN_OCCUPATION, QVariant::String);
    addField(FN_HANDEDNESS, QVariant::String);
    addFields(strseq(FP_ATTN_TIME, 1, N_ATTN_TIME), QVariant::Int);
    addFields(strseq(FP_ATTN_PLACE, 1, N_ATTN_PLACE), QVariant::Int);
    addFields(strseq(FP_ATTN_REPEAT_WORD, 1, N_ATTN_REPEAT_WORD), QVariant::Int);
    addField(FN_ATTN_NUM_REGISTRATION_TRIALS, QVariant::Int);
    addFields(strseq(FP_ATTN_SERIAL7, 1, N_ATTN_SERIAL7), QVariant::Int);
    addFields(strseq(FP_MEM_RECALL_WORD, 1, N_MEM_RECALL_WORD), QVariant::Int);
    addField(FN_FLUENCY_LETTERS_SCORE, QVariant::Int);
    addField(FN_FLUENCY_ANIMALS_SCORE, QVariant::Int);
    addFields(strseq(FP_MEM_REPEAT_ADDR_TRIAL1, 1, N_MEM_REPEAT_ADDR), QVariant::Int);
    addFields(strseq(FP_MEM_REPEAT_ADDR_TRIAL2, 1, N_MEM_REPEAT_ADDR), QVariant::Int);
    addFields(strseq(FP_MEM_REPEAT_ADDR_TRIAL3, 1, N_MEM_REPEAT_ADDR), QVariant::Int);
    addFields(strseq(FP_MEM_FAMOUS, 1, N_MEM_FAMOUS), QVariant::Int);
    addField(FN_LANG_FOLLOW_CMD_PRACTICE, QVariant::Int);
    addFields(strseq(FP_LANG_FOLLOW_CMD, 1, N_LANG_FOLLOW_CMD), QVariant::Int);
    addFields(strseq(FP_LANG_WRITE_SENTENCES_POINT, 1, N_LANG_WRITE_SENTENCES_POINT), QVariant::Int);
    addFields(strseq(FP_LANG_REPEAT_WORD, 1, N_LANG_REPEAT_WORD), QVariant::Int);
    addFields(strseq(FP_LANG_REPEAT_SENTENCE, 1, N_LANG_REPEAT_SENTENCE), QVariant::Int);
    addFields(strseq(FP_LANG_NAME_PICTURE, 1, N_LANG_NAME_PICTURE), QVariant::Int);
    addFields(strseq(FP_LANG_IDENTIFY_CONCEPT, 1, N_LANG_IDENTIFY_CONCEPT), QVariant::Int);
    addField(FN_LANG_READ_WORDS_ALOUD, QVariant::Int);
    addField(FN_VSP_COPY_INFINITY, QVariant::Int);
    addField(FN_VSP_COPY_CUBE, QVariant::Int);
    addField(FN_VSP_DRAW_CLOCK, QVariant::Int);
    addFields(strseq(FP_VSP_COUNT_DOTS, 1, N_VSP_COUNT_DOTS), QVariant::Int);
    addFields(strseq(FP_VSP_IDENTIFY_LETTER, 1, N_VSP_IDENTIFY_LETTER), QVariant::Int);
    addFields(strseq(FP_MEM_RECALL_ADDRESS, 1, N_MEM_RECALL_ADDRESS), QVariant::Int);
    addFields(strseq(FP_MEM_RECOGNIZE_ADDRESS_SCORE, 1, N_MEM_RECOGNIZE_ADDRESS), QVariant::Int);
    addFields(strseq(FP_MEM_RECOGNIZE_ADDRESS_CHOICE, 1, N_MEM_RECOGNIZE_ADDRESS), QVariant::Char);
    addField(FN_PICTURE1_BLOBID, QVariant::Int);  // FK to BLOB table
    addField(FN_PICTURE2_BLOBID, QVariant::Int);  // FK to BLOB table
    addField(FN_COMMENTS, QVariant::String);

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Ace3::shortname() const
{
    return "ACE-III";
}


QString Ace3::longname() const
{
    return tr("Addenbrooke’s Cognitive Examination, revision 3");
}


QString Ace3::description() const
{
    return tr("100-point clinician-administered assessment of attention/"
              "orientation, memory, fluency, language, and visuospatial "
              "domains.");
}


// ============================================================================
// Instance info
// ============================================================================

bool Ace3::isComplete() const
{
    return noneNull(values(strseq(FP_ATTN_TIME, 1, N_ATTN_TIME))) &&
        noneNull(values(strseq(FP_ATTN_PLACE, 1, N_ATTN_PLACE))) &&
        noneNull(values(strseq(FP_ATTN_REPEAT_WORD, 1, N_ATTN_REPEAT_WORD))) &&
        noneNull(values(strseq(FP_ATTN_SERIAL7, 1, N_ATTN_SERIAL7))) &&
        noneNull(values(strseq(FP_MEM_RECALL_WORD, 1, N_MEM_RECALL_WORD))) &&
        !valueIsNull(FN_FLUENCY_LETTERS_SCORE) &&
        !valueIsNull(FN_FLUENCY_ANIMALS_SCORE) &&
        noneNull(values(strseq(FP_MEM_REPEAT_ADDR_TRIAL3, 1, N_MEM_REPEAT_ADDR))) &&
        noneNull(values(strseq(FP_MEM_FAMOUS, 1, N_MEM_FAMOUS))) &&
        !valueIsNull(FN_LANG_FOLLOW_CMD_PRACTICE) &&
        (eq(value(FN_LANG_FOLLOW_CMD_PRACTICE), 0) ||
            noneNull(values(strseq(FP_LANG_FOLLOW_CMD, 1, N_LANG_FOLLOW_CMD)))) &&
        // ... failed practice, or completed all three actual tests
        noneNull(values(strseq(FP_LANG_WRITE_SENTENCES_POINT, 1, N_LANG_WRITE_SENTENCES_POINT))) &&
        noneNull(values(strseq(FP_LANG_REPEAT_WORD, 1, N_LANG_REPEAT_WORD))) &&
        noneNull(values(strseq(FP_LANG_REPEAT_SENTENCE, 1, N_LANG_REPEAT_SENTENCE))) &&
        noneNull(values(strseq(FP_LANG_NAME_PICTURE, 1, N_LANG_NAME_PICTURE))) &&
        noneNull(values(strseq(FP_LANG_IDENTIFY_CONCEPT, 1, N_LANG_IDENTIFY_CONCEPT))) &&
        !valueIsNull(FN_LANG_READ_WORDS_ALOUD) &&
        !valueIsNull(FN_VSP_COPY_INFINITY) &&
        !valueIsNull(FN_VSP_COPY_CUBE) &&
        !valueIsNull(FN_VSP_DRAW_CLOCK) &&
        noneNull(values(strseq(FP_VSP_COUNT_DOTS, 1, N_VSP_COUNT_DOTS))) &&
        noneNull(values(strseq(FP_VSP_IDENTIFY_LETTER, 1, N_VSP_IDENTIFY_LETTER))) &&
        noneNull(values(strseq(FP_MEM_RECALL_ADDRESS, 1, N_MEM_RECALL_ADDRESS))) &&
        isRecognitionComplete();
}


QStringList Ace3::summary() const
{
    const int a = getAttnScore();
    const int m = getMemScore();
    const int f = getFluencyScore();
    const int l = getLangScore();
    const int v = getVisuospatialScore();
    const int t = a + m + f + l + v;
    auto scorelambda = [](int score, int out_of) -> QString {
        return ": " + scoreStringWithPercent(score, out_of) + ".";
    };
    QStringList lines;
    lines.append(totalScorePhrase(t, TOTAL_OVERALL));
    lines.append(xstring("cat_attn") + scorelambda(a, TOTAL_ATTN));
    lines.append(xstring("cat_mem") + scorelambda(m, TOTAL_MEM));
    lines.append(xstring("cat_fluency") + scorelambda(f, TOTAL_FLUENCY));
    lines.append(xstring("cat_lang") + scorelambda(l, TOTAL_LANG));
    lines.append(xstring("cat_vsp") + scorelambda(v, TOTAL_VSP));
    return lines;
}


OpenableWidget* Ace3::editor(const bool read_only)
{
    int pagenum = 1;
    auto makeTitle = [this, &pagenum](const QString& title) -> QString {
        return xstring("title_prefix") + QString(" %1").arg(pagenum++) + ": "
                + title;
    };
    auto textRaw = [](const QString& string) -> QuElement* {
        return new QuText(string);
    };
    auto text = [this, textRaw](const QString& stringname) -> QuElement* {
        return textRaw(xstring(stringname));
    };
    auto explanation = [this](const QString& stringname) -> QuElement* {
        return (new QuText(xstring(stringname)))->setItalic();
    };
    auto heading = [this](const QString& stringname) -> QuElement* {
        return new QuHeading(xstring(stringname));
    };
    auto subheading = [this](const QString& stringname) -> QuElement* {
        return (new QuText(xstring(stringname)))->setBold()->setBig();
    };
    auto instructionRaw = [](const QString& string) -> QuElement* {
        return (new QuText(string))->setBold();
    };
    auto instruction = [this, instructionRaw](const QString& stringname) -> QuElement* {
        return instructionRaw(xstring(stringname));
    };
    auto boolean = [this](const QString& stringname, const QString& fieldname,
                          bool mandatory = true,
                          bool bold = false) -> QuElement* {
        return (new QuBoolean(xstring(stringname),
                              fieldRef(fieldname, mandatory)))->setBold(bold);
    };
    auto boolimg = [this](const QString& filenamestem, const QString& fieldname,
                          bool mandatory = true) -> QuElement* {
        return new QuBoolean(uifunc::resourceFilename(filenamestem), QSize(),
                             fieldRef(fieldname, mandatory));
    };
    auto warning = [](const QString& string) -> QuElement* {
        return (new QuText(string))->setWarning();
    };

    // ------------------------------------------------------------------------
    // Preamble; age-leaving-full-time-education; handedness
    // ------------------------------------------------------------------------

    const NameValueOptions options_handedness{
        {xstring("left_handed"), "L"},
        {xstring("right_handed"), "R"},
    };
    QuPagePtr page_preamble((new QuPage{
        instruction("instruction_need_paper"),
        getClinicianQuestionnaireBlockRawPointer(),
        instruction("preamble_instruction"),
        questionnairefunc::defaultGridRawPointer({
            {xstring("q_age_leaving_fte"),
             new QuLineEditInteger(fieldRef(FN_AGE_FT_EDUCATION), MIN_AGE, MAX_AGE_Y)},
            {xstring("q_occupation"),
             new QuLineEdit(fieldRef(FN_OCCUPATION))},
            {xstring("q_handedness"),
             (new QuMcq(fieldRef(FN_HANDEDNESS), options_handedness))->setHorizontal(true)},
        }, uiconst::DEFAULT_COLSPAN_Q, uiconst::DEFAULT_COLSPAN_A),
    })
        ->setTitle(makeTitle(tr("Preamble")))
        ->setType(QuPage::PageType::Clinician));

    // ------------------------------------------------------------------------
    // Attention/orientation/three word recall
    // ------------------------------------------------------------------------

    const QDateTime now = datetime::now();
    QString season;
    switch (datetime::now().date().month()) {
        // 1 = Jan, 12 = Dec
    case 12:
    case 1:
    case 2:
        season = xstring("season_winter");
        break;
    case 3:
    case 4:
    case 5:
        season = xstring("season_spring");
        break;
    case 6:
    case 7:
    case 8:
        season = xstring("season_summer");
        break;
    case 9:
    case 10:
    case 11:
        season = xstring("season_autumn");
        break;
    default:
        season = "?(season_bug)";
        break;
    }
    const QString correct_date = "     " + now.toString("dddd d MMMM yyyy") +
            "; " + season;
    // ... e.g. "Monday 2 January 2016; winter";
    // http://doc.qt.io/qt-5/qdate.html#toString

    const NameValueOptions options_registration{
        {"1", 1},
        {"2", 2},
        {"3", 3},
        {"4", 4},
        {">4", 0},
    };
    QuPagePtr page_attn((new QuPage{
        heading("cat_attn"),
        // Orientation
        instruction("attn_q_time"),
        new QuFlowContainer{
            boolean("attn_time1", strnum(FP_ATTN_TIME, 1)),
            boolean("attn_time2", strnum(FP_ATTN_TIME, 2)),
            boolean("attn_time3", strnum(FP_ATTN_TIME, 3)),
            boolean("attn_time4", strnum(FP_ATTN_TIME, 4)),
            boolean("attn_time5", strnum(FP_ATTN_TIME, 5)),
        },
        explanation("instruction_time"),
        (new QuText(correct_date))->setItalic(),
        instruction("attn_q_place"),
        new QuFlowContainer{
            boolean("attn_place1", strnum(FP_ATTN_PLACE, 1)),
            boolean("attn_place2", strnum(FP_ATTN_PLACE, 2)),
            boolean("attn_place3", strnum(FP_ATTN_PLACE, 3)),
            boolean("attn_place4", strnum(FP_ATTN_PLACE, 4)),
            boolean("attn_place5", strnum(FP_ATTN_PLACE, 5)),
        },
        explanation("instruction_place"),
        // Lemon, key, ball (registration)
        heading("cat_attn"),
        instruction("attn_q_words"),
        explanation("attn_instruction_words"),
        new QuFlowContainer{
            boolean("mem_word1", strnum(FP_ATTN_REPEAT_WORD, 1)),
            boolean("mem_word2", strnum(FP_ATTN_REPEAT_WORD, 2)),
            boolean("mem_word3", strnum(FP_ATTN_REPEAT_WORD, 3)),
        },
        new QuFlowContainer{
            text("attn_q_register_n_trials"),
            (new QuMcq(fieldRef(FN_ATTN_NUM_REGISTRATION_TRIALS, false),  // not mandatory
                                 options_registration))->setHorizontal(true),
        },
        // Serial 7s
        heading("cat_attn"),
        instruction("attn_q_serial_sevens"),
        explanation("attn_instruction_sevens"),
        new QuFlowContainer{
            boolean("attn_subtraction1", strnum(FP_ATTN_SERIAL7, 1)),
            boolean("attn_subtraction2", strnum(FP_ATTN_SERIAL7, 2)),
            boolean("attn_subtraction3", strnum(FP_ATTN_SERIAL7, 3)),
            boolean("attn_subtraction4", strnum(FP_ATTN_SERIAL7, 4)),
            boolean("attn_subtraction5", strnum(FP_ATTN_SERIAL7, 5)),
        },
        // Lemon, key, ball (recall)
        heading("cat_mem"),
        instruction("mem_q_recall_words"),
        explanation("mem_instruction_recall"),
        new QuFlowContainer{
            boolean("mem_word1", strnum(FP_MEM_RECALL_WORD, 1)),
            boolean("mem_word2", strnum(FP_MEM_RECALL_WORD, 2)),
            boolean("mem_word3", strnum(FP_MEM_RECALL_WORD, 3)),
        },
    })->setTitle(makeTitle(tr("Attention")))->setType(QuPage::PageType::Clinician));

    // ------------------------------------------------------------------------
    // Fluency
    // ------------------------------------------------------------------------

    const NameValueOptions options_fluency_letters{
        {"0–1", 0},
        {"2–3", 1},
        {"4–5", 2},
        {"6–7", 3},
        {"8–10", 4},
        {"11–13", 5},
        {"14–17", 6},
        {"≥18", 7}

    };
    const NameValueOptions options_fluency_animals{
        {"0–4", 0},
        {"5–6", 1},
        {"7–8", 2},
        {"9–10", 3},
        {"11–13", 4},
        {"14–16", 5},
        {"17–21", 6},
        {"≥22", 7}
    };
    QuPagePtr page_fluency((new QuPage{
        heading("cat_fluency"),
        // Letters
        subheading("fluency_subhead_letters"),
        instruction("fluency_q_letters"),
        new QuCountdown(FLUENCY_TIME_SEC),
        explanation("fluency_instruction_letters"),
        text("fluency_prompt_letters_cor"),
        (new QuMcq(fieldRef(FN_FLUENCY_LETTERS_SCORE),
                             options_fluency_letters))->setHorizontal(true),
        new QuSpacer(),
        // Animals
        subheading("fluency_subheading_animals"),
        instruction("fluency_q_animals"),
        new QuCountdown(FLUENCY_TIME_SEC),
        explanation("fluency_instruction_animals"),
        text("fluency_prompt_animals_cor"),
        (new QuMcq(fieldRef(FN_FLUENCY_ANIMALS_SCORE),
                             options_fluency_animals))->setHorizontal(true),
    })->setTitle(makeTitle(tr("Fluency")))->setType(QuPage::PageType::Clinician));

    // ------------------------------------------------------------------------
    // Learning the address; famous people
    // ------------------------------------------------------------------------

    QuPagePtr page_repeat_addr_famous((new QuPage{
        heading("cat_mem"),
        instruction("memory_q_address"),
        explanation("memory_instruction_address_1"),
        explanation("memory_instruction_address_2"),
        // Address 1
        new QuVerticalContainer{
            instructionRaw(xstring("trial") + " 1"),
            new QuFlowContainer{
                boolean("address_1", strnum(FP_MEM_REPEAT_ADDR_TRIAL1, 1), false),
                boolean("address_2", strnum(FP_MEM_REPEAT_ADDR_TRIAL1, 2), false),
            },
            new QuFlowContainer{
                boolean("address_3", strnum(FP_MEM_REPEAT_ADDR_TRIAL1, 3), false),
                boolean("address_4", strnum(FP_MEM_REPEAT_ADDR_TRIAL1, 4), false),
                boolean("address_5", strnum(FP_MEM_REPEAT_ADDR_TRIAL1, 5), false),
            },
            boolean("address_6", strnum(FP_MEM_REPEAT_ADDR_TRIAL1, 6), false),
            boolean("address_7", strnum(FP_MEM_REPEAT_ADDR_TRIAL1, 7), false),
        },
        // Address 2
        new QuVerticalContainer{
            instructionRaw(xstring("trial") + " 2"),
            new QuFlowContainer{
                boolean("address_1", strnum(FP_MEM_REPEAT_ADDR_TRIAL2, 1), false),
                boolean("address_2", strnum(FP_MEM_REPEAT_ADDR_TRIAL2, 2), false),
            },
            new QuFlowContainer{
                boolean("address_3", strnum(FP_MEM_REPEAT_ADDR_TRIAL2, 3), false),
                boolean("address_4", strnum(FP_MEM_REPEAT_ADDR_TRIAL2, 4), false),
                boolean("address_5", strnum(FP_MEM_REPEAT_ADDR_TRIAL2, 5), false),
            },
            boolean("address_6", strnum(FP_MEM_REPEAT_ADDR_TRIAL2, 6), false),
            boolean("address_7", strnum(FP_MEM_REPEAT_ADDR_TRIAL2, 7), false),
        },
        // Address 3
        new QuVerticalContainer{
            instructionRaw(xstring("trial") + " 3"),
            new QuFlowContainer{
                boolean("address_1", strnum(FP_MEM_REPEAT_ADDR_TRIAL3, 1), true),
                boolean("address_2", strnum(FP_MEM_REPEAT_ADDR_TRIAL3, 2), true),
            },
            new QuFlowContainer{
                boolean("address_3", strnum(FP_MEM_REPEAT_ADDR_TRIAL3, 3), true),
                boolean("address_4", strnum(FP_MEM_REPEAT_ADDR_TRIAL3, 4), true),
                boolean("address_5", strnum(FP_MEM_REPEAT_ADDR_TRIAL3, 5), true),
            },
            boolean("address_6", strnum(FP_MEM_REPEAT_ADDR_TRIAL3, 6), true),
            boolean("address_7", strnum(FP_MEM_REPEAT_ADDR_TRIAL3, 7), true),
        },
        heading("cat_mem"),
        boolean("famous_1", strnum(FP_MEM_FAMOUS, 1), true, true),
        boolean("famous_2", strnum(FP_MEM_FAMOUS, 2), true, true),
        boolean("famous_3", strnum(FP_MEM_FAMOUS, 3), true, true),
        boolean("famous_4", strnum(FP_MEM_FAMOUS, 4), true, true),
        explanation("instruction_famous"),
    })
        ->setTitle(makeTitle(tr("Address learning; famous people")))
        ->setType(QuPage::PageType::Clinician));

    // ------------------------------------------------------------------------
    // Commands; writing sentences
    // ------------------------------------------------------------------------

    QuPagePtr page_commands_sentences(( new QuPage{
        heading("cat_lang"),
        explanation("lang_q_command_1"),
        boolean("lang_command_practice", FN_LANG_FOLLOW_CMD_PRACTICE, true, true),
        explanation("lang_q_command_2"),
        boolean("lang_command1", strnum(FP_LANG_FOLLOW_CMD, 1), true, true)->addTag(TAG_EL_LANG_OPTIONAL_COMMAND),
        boolean("lang_command2", strnum(FP_LANG_FOLLOW_CMD, 2), true, true)->addTag(TAG_EL_LANG_OPTIONAL_COMMAND),
        boolean("lang_command3", strnum(FP_LANG_FOLLOW_CMD, 3), true, true)->addTag(TAG_EL_LANG_OPTIONAL_COMMAND),
        warning(tr("Other commands not shown; subject failed practice trial"))
                                            ->addTag(TAG_EL_LANG_NOT_SHOWN),
        heading("cat_lang"),
        instruction("lang_q_sentences"),
        boolean("lang_sentences_point1", strnum(FP_LANG_WRITE_SENTENCES_POINT, 1)),
        boolean("lang_sentences_point2", strnum(FP_LANG_WRITE_SENTENCES_POINT, 2)),
    })
        ->setTitle(makeTitle(tr("Commands; writing sentences")))
        ->addTag(TAG_PG_LANG_COMMANDS_SENTENCES)
        ->setType(QuPage::PageType::Clinician));

    // ------------------------------------------------------------------------
    // Repetition; preparing clinician for pictures
    // ------------------------------------------------------------------------

    QuPagePtr page_repetition(( new QuPage{
        // Repeating words
        heading("cat_lang"),
        instruction("lang_q_repeat"),
        boolean("lang_repeat_word1", strnum(FP_LANG_REPEAT_WORD, 1)),
        boolean("lang_repeat_word2", strnum(FP_LANG_REPEAT_WORD, 2)),
        boolean("lang_repeat_word3", strnum(FP_LANG_REPEAT_WORD, 3)),
        boolean("lang_repeat_word4", strnum(FP_LANG_REPEAT_WORD, 4)),
        explanation("lang_instruction_repeat"),
        // Repeating sentences
        heading("cat_lang"),
        instruction("lang_q_repeat"),
        boolean("lang_sentence1", strnum(FP_LANG_REPEAT_SENTENCE, 1)),
        boolean("lang_sentence2", strnum(FP_LANG_REPEAT_SENTENCE, 2)),
        explanation("lang_instruction_sentences_1"),
        explanation("lang_instruction_sentences_2"),
        new QuSpacer(),
        // Preparation for clinician for pictures
        instruction("advance_warning_1"),
        explanation("advance_warning_2"),
        explanation("advance_warning_3"),
        explanation("advance_warning_4"),
        explanation("advance_warning_5"),
        explanation("advance_warning_6"),
        explanation("advance_warning_7"),
        explanation("advance_warning_8"),
    })
        ->setTitle(makeTitle(tr("Repetition")))
        ->setType(QuPage::PageType::Clinician));

    // ------------------------------------------------------------------------
    // Naming pictures
    // ------------------------------------------------------------------------

    QuPagePtr page_name_pictures((new QuPage{
        // Naming pictures
        heading("cat_lang"),
        instruction("lang_q_identify_pic"),
        new QuGridContainer(3, {
            boolimg(IMAGE_SPOON, strnum(FP_LANG_NAME_PICTURE, 1)),
            boolimg(IMAGE_BOOK, strnum(FP_LANG_NAME_PICTURE, 2)),
            boolimg(IMAGE_KANGAROO, strnum(FP_LANG_NAME_PICTURE, 3)),
            boolimg(IMAGE_PENGUIN, strnum(FP_LANG_NAME_PICTURE, 4)),
            boolimg(IMAGE_ANCHOR, strnum(FP_LANG_NAME_PICTURE, 5)),
            boolimg(IMAGE_CAMEL, strnum(FP_LANG_NAME_PICTURE, 6)),
            boolimg(IMAGE_HARP, strnum(FP_LANG_NAME_PICTURE, 7)),
            boolimg(IMAGE_RHINOCEROS, strnum(FP_LANG_NAME_PICTURE, 8)),
            boolimg(IMAGE_BARREL, strnum(FP_LANG_NAME_PICTURE, 9)),
            boolimg(IMAGE_CROWN, strnum(FP_LANG_NAME_PICTURE, 10)),
            boolimg(IMAGE_CROCODILE, strnum(FP_LANG_NAME_PICTURE, 11)),
            boolimg(IMAGE_ACCORDION, strnum(FP_LANG_NAME_PICTURE, 12)),
        }),
        // Choosing pictures by concept
        instruction("lang_q_identify_concept"),
        boolean("lang_concept1", strnum(FP_LANG_IDENTIFY_CONCEPT, 1)),
        boolean("lang_concept2", strnum(FP_LANG_IDENTIFY_CONCEPT, 2)),
        boolean("lang_concept3", strnum(FP_LANG_IDENTIFY_CONCEPT, 3)),
        boolean("lang_concept4", strnum(FP_LANG_IDENTIFY_CONCEPT, 4)),
        explanation("lang_instruction_identify_concept"),
    })
        ->setTitle(makeTitle(tr("Naming pictures")))
#ifdef NOSCROLL_IMAGE_PAGES
        ->allowScroll(false, true)  // no scrolling; zoomable
#endif
        ->setType(QuPage::PageType::ClinicianWithPatient));

    // ------------------------------------------------------------------------
    // Reading irregular words
    // ------------------------------------------------------------------------

    QuPagePtr page_read_words_aloud((new QuPage{
        // Reading irregular words aloud
        heading("cat_lang"),
        instruction("lang_q_read_aloud"),
        new QuSpacer(),
        subheading("lang_read_aloud_words"),  // the words
        new QuSpacer(),
        boolean("lang_read_aloud_all_correct", FN_LANG_READ_WORDS_ALOUD),
        explanation("lang_instruction_read_aloud"),
    })
        ->setTitle(makeTitle(tr("Reading irregular words")))
        ->setType(QuPage::PageType::ClinicianWithPatient));

    // ------------------------------------------------------------------------
    // Infinity
    // ------------------------------------------------------------------------

    QuPagePtr page_infinity((new QuPage{
        heading("cat_vsp"),
        instruction("vsp_q_infinity"),
        new QuImage(uifunc::resourceFilename(IMAGE_INFINITY)),
        boolean("vsp_infinity_correct", FN_VSP_COPY_INFINITY),
    })
        ->setTitle(makeTitle(tr("Infinity")))
        ->setType(QuPage::PageType::ClinicianWithPatient));

    // ------------------------------------------------------------------------
    // Cube
    // ------------------------------------------------------------------------

    const NameValueOptions options_cube = NameValueOptions::makeNumbers(0, 2);
    QuPagePtr page_cube((new QuPage{
        instruction("vsp_q_cube"),
        new QuImage(uifunc::resourceFilename(IMAGE_CUBE)),
        text("vsp_score_cube"),
        (new QuMcq(fieldRef(FN_VSP_COPY_CUBE), options_cube))->setHorizontal(true),
    })
        ->setTitle(makeTitle(tr("Cube")))
        ->setType(QuPage::PageType::ClinicianWithPatient));

    // ------------------------------------------------------------------------
    // Clock
    // ------------------------------------------------------------------------

    const NameValueOptions options_clock = NameValueOptions::makeNumbers(0, 5);
    QuPagePtr page_clock((new QuPage{
                              instruction("vsp_q_clock"),
                              explanation("vsp_instruction_clock"),
                              text("vsp_score_clock"),
                              (new QuMcq(fieldRef(FN_VSP_DRAW_CLOCK), options_clock))->setHorizontal(true),
    })
        ->setTitle(makeTitle(tr("Clock")))
        ->setType(QuPage::PageType::Clinician));

    // ------------------------------------------------------------------------
    // Dots
    // ------------------------------------------------------------------------

    QuPagePtr page_dots((new QuPage{
        heading("cat_vsp"),
        instruction("vsp_q_dots"),
        new QuGridContainer(2, {
            boolimg(IMAGE_DOTS8, strnum(FP_VSP_COUNT_DOTS, 1)),
            boolimg(IMAGE_DOTS10, strnum(FP_VSP_COUNT_DOTS, 2)),
            boolimg(IMAGE_DOTS7, strnum(FP_VSP_COUNT_DOTS, 3)),
            boolimg(IMAGE_DOTS9, strnum(FP_VSP_COUNT_DOTS, 4)),
        }),
    })
        ->setTitle(makeTitle(tr("Dot counting")))
#ifdef NOSCROLL_IMAGE_PAGES
        ->allowScroll(false, true)  // no scrolling; zoomable
#endif
        ->setType(QuPage::PageType::ClinicianWithPatient));

    // ------------------------------------------------------------------------
    // Letters
    // ------------------------------------------------------------------------

    QuPagePtr page_letters((new QuPage{
        heading("cat_vsp"),
        instruction("vsp_q_letters"),
        new QuGridContainer(2, {
            boolimg(IMAGE_K, strnum(FP_VSP_IDENTIFY_LETTER, 1)),
            boolimg(IMAGE_M, strnum(FP_VSP_IDENTIFY_LETTER, 2)),
            boolimg(IMAGE_A, strnum(FP_VSP_IDENTIFY_LETTER, 3)),
            boolimg(IMAGE_T, strnum(FP_VSP_IDENTIFY_LETTER, 4)),
        }),
    })
        ->setTitle(makeTitle(tr("Noisy letters")))
#ifdef NOSCROLL_IMAGE_PAGES
        ->allowScroll(false, true)  // no scrolling; zoomable
#endif
        ->setType(QuPage::PageType::ClinicianWithPatient));

    // ------------------------------------------------------------------------
    // Back to clinician
    // ------------------------------------------------------------------------

    QuPagePtr page_back_to_clinician((new QuPage{
        textRaw(tr("Please make sure the subject can’t see the screen "
                   "before you proceed. (Memory prompts coming up.)")),
    })
        ->setTitle(makeTitle(tr("[reminder to clinician]")))
        ->setType(QuPage::PageType::Clinician));

    // ------------------------------------------------------------------------
    // Address recall: free
    // ------------------------------------------------------------------------

    QuPagePtr page_recall_address_free((new QuPage{
        heading("cat_mem"),
        instruction("mem_q_recall_address"),
        new QuVerticalContainer{
            new QuFlowContainer{
                boolean("address_1", strnum(FP_MEM_RECALL_ADDRESS, 1)),
                boolean("address_2", strnum(FP_MEM_RECALL_ADDRESS, 2)),
            },
            new QuFlowContainer{
                boolean("address_3", strnum(FP_MEM_RECALL_ADDRESS, 3)),
                boolean("address_4", strnum(FP_MEM_RECALL_ADDRESS, 4)),
                boolean("address_5", strnum(FP_MEM_RECALL_ADDRESS, 5)),
            },
            boolean("address_6", strnum(FP_MEM_RECALL_ADDRESS, 6)),
            boolean("address_7", strnum(FP_MEM_RECALL_ADDRESS, 7)),
        },
    })
        ->setTitle(makeTitle(tr("Free recall")))
        ->setType(QuPage::PageType::Clinician));

    // ------------------------------------------------------------------------
    // Address recall: cued
    // ------------------------------------------------------------------------

    const NameValueOptions options_recall_name({
        {xstring("mem_recall_option1_line1"), CHOICE_A},
        {xstring("mem_recall_option2_line1"), CHOICE_B},  // correct
        {xstring("mem_recall_option3_line1"), CHOICE_C},
    });
    const NameValueOptions options_recall_number({
        {xstring("mem_recall_option1_line2"), CHOICE_A},
        {xstring("mem_recall_option2_line2"), CHOICE_B},  // correct
        {xstring("mem_recall_option3_line2"), CHOICE_C},
    });
    const NameValueOptions options_recall_street({
        {xstring("mem_recall_option1_line3"), CHOICE_A},
        {xstring("mem_recall_option2_line3"), CHOICE_B},
        {xstring("mem_recall_option3_line3"), CHOICE_C},  // correct
    });
    const NameValueOptions options_recall_town({
        {xstring("mem_recall_option1_line4"), CHOICE_A},
        {xstring("mem_recall_option2_line4"), CHOICE_B},  // correct
        {xstring("mem_recall_option3_line4"), CHOICE_C},
    });
    const NameValueOptions options_recall_county({
        {xstring("mem_recall_option1_line5"), CHOICE_A},  // correct
        {xstring("mem_recall_option2_line5"), CHOICE_B},
        {xstring("mem_recall_option3_line5"), CHOICE_C},
    });
    FieldRefPtr fr_recallprompted_name = fieldRef(strnum(FP_MEM_RECOGNIZE_ADDRESS_CHOICE, 1));
    FieldRefPtr fr_recallprompted_number = fieldRef(strnum(FP_MEM_RECOGNIZE_ADDRESS_CHOICE, 2));
    FieldRefPtr fr_recallprompted_street = fieldRef(strnum(FP_MEM_RECOGNIZE_ADDRESS_CHOICE, 3));
    FieldRefPtr fr_recallprompted_town = fieldRef(strnum(FP_MEM_RECOGNIZE_ADDRESS_CHOICE, 4));
    FieldRefPtr fr_recallprompted_county = fieldRef(strnum(FP_MEM_RECOGNIZE_ADDRESS_CHOICE, 5));
    connect(fr_recallprompted_name.data(), &FieldRef::valueChanged,
            this, &Ace3::updateAddressRecognition);
    connect(fr_recallprompted_number.data(), &FieldRef::valueChanged,
            this, &Ace3::updateAddressRecognition);
    connect(fr_recallprompted_street.data(), &FieldRef::valueChanged,
            this, &Ace3::updateAddressRecognition);
    connect(fr_recallprompted_town.data(), &FieldRef::valueChanged,
            this, &Ace3::updateAddressRecognition);
    connect(fr_recallprompted_county.data(), &FieldRef::valueChanged,
            this, &Ace3::updateAddressRecognition);

    QuPagePtr page_recall_address_prompted((new QuPage{
        instruction("no_need_for_extra_recall")->addTag(TAG_RECOG_SUPERFLUOUS),
        instruction("mem_q_recognize_address")->addTag(TAG_RECOG_REQUIRED),
        textRaw(tr("Name:"))->addTag(TAG_RECOG_NAME),
        (new QuMcq(fr_recallprompted_name, options_recall_name))
                    ->setHorizontal(true)
                    ->addTag(TAG_RECOG_NAME),
        textRaw(tr("Number:"))->addTag(TAG_RECOG_NUMBER),
        (new QuMcq(fr_recallprompted_number, options_recall_number))
                    ->setHorizontal(true)
                    ->addTag(TAG_RECOG_NUMBER),
        textRaw(tr("Street:"))->addTag(TAG_RECOG_STREET),
        (new QuMcq(fr_recallprompted_street, options_recall_street))
                    ->setHorizontal(true)
                    ->addTag(TAG_RECOG_STREET),
        textRaw(tr("Town:"))->addTag(TAG_RECOG_TOWN),
        (new QuMcq(fr_recallprompted_town, options_recall_town))
                    ->setHorizontal(true)
                    ->addTag(TAG_RECOG_TOWN),
        textRaw(tr("County:"))->addTag(TAG_RECOG_COUNTY),
        (new QuMcq(fr_recallprompted_county, options_recall_county))
                    ->setHorizontal(true)
                    ->addTag(TAG_RECOG_COUNTY),

    })
        ->setTitle(makeTitle(tr("Cued recall")))
        ->addTag(TAG_PG_MEM_PROMPTED_RECALL)
        ->setType(QuPage::PageType::Clinician));

    // ------------------------------------------------------------------------
    // Comments
    // ------------------------------------------------------------------------

    QuPagePtr page_comments((new QuPage{
        instructionRaw(TextConst::examinerCommentsPrompt()),
        (new QuLineEdit(fieldRef(FN_COMMENTS, false)))
            ->setHint(TextConst::examinerComments()),
    })
        ->setTitle(makeTitle(tr("Comments")))
        ->setType(QuPage::PageType::Clinician));

    // ------------------------------------------------------------------------
    // Photo 1
    // ------------------------------------------------------------------------

    QuPagePtr page_photo_1((new QuPage{
        instruction("picture1_q"),
        explanation("picture_instruction1"),
        explanation("picture_instruction2"),
        new QuPhoto(blobFieldRef(FN_PICTURE1_BLOBID, false)),
    })
        ->setTitle(makeTitle(tr("Photo 1")))
        ->setType(QuPage::PageType::Clinician));

    // ------------------------------------------------------------------------
    // Photo 2
    // ------------------------------------------------------------------------

    QuPagePtr page_photo_2((new QuPage{
        instruction("picture2_q"),
        explanation("picture_instruction1"),
        explanation("picture_instruction2"),
        new QuPhoto(blobFieldRef(FN_PICTURE2_BLOBID, false)),
    })
        ->setTitle(makeTitle(tr("Photo 2")))
        ->setType(QuPage::PageType::Clinician));

    // ------------------------------------------------------------------------
    // Questionnaire
    // ------------------------------------------------------------------------

    m_questionnaire = new Questionnaire(m_app, {
        page_preamble, page_attn, page_fluency, page_repeat_addr_famous,
        page_commands_sentences, page_repetition,
        page_name_pictures, page_read_words_aloud,
        page_infinity, page_cube, page_clock,
        page_dots, page_letters, page_back_to_clinician,
        page_recall_address_free, page_recall_address_prompted,
        page_comments, page_photo_1, page_photo_2,
    });
    m_questionnaire->setReadOnly(read_only);

    // ------------------------------------------------------------------------
    // Signals and initial dynamic state
    // ------------------------------------------------------------------------

    FieldRefPtr fr_lang_practice = fieldRef(FN_LANG_FOLLOW_CMD_PRACTICE);
    connect(fr_lang_practice.data(), &FieldRef::valueChanged,
            this, &Ace3::langPracticeChanged);
    langPracticeChanged(fr_lang_practice.data());

    for (int i = 1; i <= N_MEM_RECALL_ADDRESS; ++i) {
        FieldRefPtr fr = fieldRef(strnum(FP_MEM_RECALL_ADDRESS, i));
        connect(fr.data(), &FieldRef::valueChanged,
                this, &Ace3::updateAddressRecognition);
    }

    updateAddressRecognition();

    // ------------------------------------------------------------------------
    // Done
    // ------------------------------------------------------------------------

    return m_questionnaire;
}


// ============================================================================
// Task-specific calculations
// ============================================================================

int Ace3::getAttnScore() const
{
    return sumInt(values(strseq(FP_ATTN_TIME, 1, N_ATTN_TIME) +
                         strseq(FP_ATTN_PLACE, 1, N_ATTN_PLACE) +
                         strseq(FP_ATTN_REPEAT_WORD, 1, N_ATTN_REPEAT_WORD) +
                         strseq(FP_ATTN_SERIAL7, 1, N_ATTN_SERIAL7)));
}


int Ace3::getMemRecognitionScore() const
{
    const int recall1 = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 1));
    const int recall2 = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 2));
    const int recall3 = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 3));
    const int recall4 = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 4));
    const int recall5 = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 5));
    const int recall6 = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 6));
    const int recall7 = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 7));
    const int recog1 = valueInt(strnum(FP_MEM_RECOGNIZE_ADDRESS_SCORE, 1));
    const int recog2 = valueInt(strnum(FP_MEM_RECOGNIZE_ADDRESS_SCORE, 2));
    const int recog3 = valueInt(strnum(FP_MEM_RECOGNIZE_ADDRESS_SCORE, 3));
    const int recog4 = valueInt(strnum(FP_MEM_RECOGNIZE_ADDRESS_SCORE, 4));
    const int recog5 = valueInt(strnum(FP_MEM_RECOGNIZE_ADDRESS_SCORE, 5));
    int score = 0;
    score += (recall1 && recall2) ? 1 : recog1;  // forename, surname
    score += recall3 ? 1 : recog2;  // number
    score += (recall4 && recall5) ? 1 : recog3;  // streetname, streettype
    score += recall6 ? 1 : recog4;  // city
    score += recall7 ? 1 : recog5; // county
    return score;
}


int Ace3::getMemScore() const
{
    return sumInt(values(strseq(FP_MEM_RECALL_WORD, 1, N_MEM_RECALL_WORD) +
                         strseq(FP_MEM_REPEAT_ADDR_TRIAL3, 1, N_MEM_REPEAT_ADDR) +
                         strseq(FP_MEM_FAMOUS, 1, N_MEM_FAMOUS) +
                         strseq(FP_MEM_RECALL_ADDRESS, 1, N_MEM_RECALL_ADDRESS))) +
            getMemRecognitionScore();
}


int Ace3::getFluencyScore() const
{
    return valueInt(FN_FLUENCY_LETTERS_SCORE) +
            valueInt(FN_FLUENCY_ANIMALS_SCORE);
}


int Ace3::getFollowCommandScore() const
{
    if (!valueInt(FN_LANG_FOLLOW_CMD_PRACTICE)) {
        return 0;
    }
    return sumInt(values(strseq(FP_LANG_FOLLOW_CMD, 1, N_LANG_FOLLOW_CMD)));
}


int Ace3::getRepeatWordScore() const
{
    const int n = sumInt(values(strseq(FP_LANG_REPEAT_WORD, 1, 4)));
    return n >= 4 ? 2 : (n == 3 ? 1 : 0);
}


int Ace3::getLangScore() const
{
    return getFollowCommandScore() +  // 3 points
            sumInt(values(strseq(FP_LANG_WRITE_SENTENCES_POINT, 1, N_LANG_WRITE_SENTENCES_POINT))) +  // 2 points
            getRepeatWordScore() +  // 2 points
            sumInt(values(strseq(FP_LANG_REPEAT_SENTENCE, 1, N_LANG_REPEAT_SENTENCE))) +  // 2 points
            sumInt(values(strseq(FP_LANG_NAME_PICTURE, 1, N_LANG_NAME_PICTURE))) +  // 12 points
            sumInt(values(strseq(FP_LANG_IDENTIFY_CONCEPT, 1, N_LANG_IDENTIFY_CONCEPT))) +  // 4 points
            valueInt(FN_LANG_READ_WORDS_ALOUD);  // 1 point
}


int Ace3::getVisuospatialScore() const
{
    return valueInt(FN_VSP_COPY_INFINITY) +  // 1 point
            valueInt(FN_VSP_COPY_CUBE) +  // 2 points
            valueInt(FN_VSP_DRAW_CLOCK) +  // 5 points
            sumInt(values(strseq(FP_VSP_COUNT_DOTS, 1, N_VSP_COUNT_DOTS))) +
            sumInt(values(strseq(FP_VSP_IDENTIFY_LETTER, 1, N_VSP_IDENTIFY_LETTER)));
}


int Ace3::totalScore() const
{
    return getAttnScore() +
            getMemScore() +
            getFluencyScore() +
            getLangScore() +
            getVisuospatialScore();
}


bool Ace3::isRecognitionComplete() const
{
    const int recall1 = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 1));
    const int recall2 = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 2));
    const int recall3 = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 3));
    const int recall4 = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 4));
    const int recall5 = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 5));
    const int recall6 = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 6));
    const int recall7 = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 7));
    const bool recog1present = !valueIsNull(strnum(FP_MEM_RECOGNIZE_ADDRESS_CHOICE, 1));
    const bool recog2present = !valueIsNull(strnum(FP_MEM_RECOGNIZE_ADDRESS_CHOICE, 2));
    const bool recog3present = !valueIsNull(strnum(FP_MEM_RECOGNIZE_ADDRESS_CHOICE, 3));
    const bool recog4present = !valueIsNull(strnum(FP_MEM_RECOGNIZE_ADDRESS_CHOICE, 4));
    const bool recog5present = !valueIsNull(strnum(FP_MEM_RECOGNIZE_ADDRESS_CHOICE, 5));
    return (
        ((recall1 && recall2) || recog1present) &&  // forename, surname
        (recall3 || recog2present) &&  // number
        ((recall4 && recall5) || recog3present) &&  // streetname, streettype
        (recall6 || recog4present) &&  // city
        (recall7 || recog5present)  // county
    );
}


// ============================================================================
// Signal handlers
// ============================================================================

void Ace3::langPracticeChanged(const FieldRef* fieldref)
{
    // qDebug() << Q_FUNC_INFO;
    if (!m_questionnaire) {
        return;
    }
    const QVariant value = fieldref->value();
    const bool visible = !eq(value, false);
    const bool mandatory = value.toBool();
    for (int i = 1; i <= N_LANG_FOLLOW_CMD; ++i) {
        fieldRef(strnum(FP_LANG_FOLLOW_CMD, i))->setMandatory(mandatory);
    }
    m_questionnaire->setVisibleByTag(TAG_EL_LANG_OPTIONAL_COMMAND, visible,
                                     false, TAG_PG_LANG_COMMANDS_SENTENCES);
    m_questionnaire->setVisibleByTag(TAG_EL_LANG_NOT_SHOWN, !visible,
                                     false, TAG_PG_LANG_COMMANDS_SENTENCES);
}


void Ace3::updateAddressRecognition()
{
    // Parameter "const FieldRef* fieldref" not needed;
    // http://doc.qt.io/qt-5/signalsandslots.html
    // "... a slot may have a shorter signature than the signal it receives"

    if (!m_questionnaire) {
        return;
    }

    // We show something if we failed to recall all parts of it.
    // This function also updates the scores, so this code is all in one place.
    const bool name_correct = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 1)) &&
            valueInt(strnum(FP_MEM_RECALL_ADDRESS, 2));
    const bool number_correct = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 3));
    const bool street_correct = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 4)) &&
            valueInt(strnum(FP_MEM_RECALL_ADDRESS, 5));
    const bool town_correct = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 6));
    const bool county_correct = valueInt(strnum(FP_MEM_RECALL_ADDRESS, 7));
    const bool recog_required = !name_correct || !number_correct ||
            !street_correct || !town_correct || !county_correct;
    const bool recog_superfluous = !recog_required;
    m_questionnaire->setVisibleByTag(TAG_RECOG_NAME, !name_correct,
                                     false, TAG_PG_MEM_PROMPTED_RECALL);
    m_questionnaire->setVisibleByTag(TAG_RECOG_NUMBER, !number_correct,
                                     false, TAG_PG_MEM_PROMPTED_RECALL);
    m_questionnaire->setVisibleByTag(TAG_RECOG_STREET, !street_correct,
                                     false, TAG_PG_MEM_PROMPTED_RECALL);
    m_questionnaire->setVisibleByTag(TAG_RECOG_TOWN, !town_correct,
                                     false, TAG_PG_MEM_PROMPTED_RECALL);
    m_questionnaire->setVisibleByTag(TAG_RECOG_COUNTY, !county_correct,
                                     false, TAG_PG_MEM_PROMPTED_RECALL);
    m_questionnaire->setVisibleByTag(TAG_RECOG_REQUIRED, recog_required,
                                     false, TAG_PG_MEM_PROMPTED_RECALL);
    m_questionnaire->setVisibleByTag(TAG_RECOG_SUPERFLUOUS, recog_superfluous,
                                     false, TAG_PG_MEM_PROMPTED_RECALL);

    // - bool to int 0/1 is guaranteed, so no need for " ? 1 : 0";
    //   http://stackoverflow.com/questions/5369770/bool-to-int-conversion
    const int recogscore1 = name_correct || valueString(
                strnum(FP_MEM_RECOGNIZE_ADDRESS_CHOICE, 1)) == CHOICE_B;
    const int recogscore2 = number_correct || valueString(
                strnum(FP_MEM_RECOGNIZE_ADDRESS_CHOICE, 2)) == CHOICE_B;
    const int recogscore3 = street_correct || valueString(
                strnum(FP_MEM_RECOGNIZE_ADDRESS_CHOICE, 3)) == CHOICE_C;
    const int recogscore4 = town_correct || valueString(
                strnum(FP_MEM_RECOGNIZE_ADDRESS_CHOICE, 4)) == CHOICE_B;
    const int recogscore5 = county_correct || valueString(
                strnum(FP_MEM_RECOGNIZE_ADDRESS_CHOICE, 5)) == CHOICE_A;
    setValue(strnum(FP_MEM_RECOGNIZE_ADDRESS_SCORE, 1), recogscore1);
    setValue(strnum(FP_MEM_RECOGNIZE_ADDRESS_SCORE, 2), recogscore2);
    setValue(strnum(FP_MEM_RECOGNIZE_ADDRESS_SCORE, 3), recogscore3);
    setValue(strnum(FP_MEM_RECOGNIZE_ADDRESS_SCORE, 4), recogscore4);
    setValue(strnum(FP_MEM_RECOGNIZE_ADDRESS_SCORE, 5), recogscore5);
}
