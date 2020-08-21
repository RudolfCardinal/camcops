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

#include "ided3dtrial.h"
#include "lib/containers.h"
#include "lib/datetime.h"
#include "maths/ccrandom.h"
#include "maths/mathfunc.h"
#include "ided3dexemplars.h"
#include "ided3dstage.h"
using ccrandom::drawreplace;
using ccrandom::dwor;
using containers::subtract;
using mathfunc::range;


// Table names
const QString IDED3DTrial::TRIAL_TABLENAME("ided3d_trials");

// Fieldnames: Trial
const QString IDED3DTrial::FN_FK_TO_TASK("ided3d_id");
const QString IDED3DTrial::FN_TRIAL("trial");  // 1-based
const QString IDED3DTrial::FN_STAGE("stage");  // 1-based
const QString FN_CORRECT_LOCATION("correct_location");
const QString FN_INCORRECT_LOCATION("incorrect_location");
const QString FN_CORRECT_SHAPE("correct_shape");
const QString FN_CORRECT_COLOUR("correct_colour");
const QString FN_CORRECT_NUMBER("correct_number");
const QString FN_INCORRECT_SHAPE("incorrect_shape");
const QString FN_INCORRECT_COLOUR("incorrect_colour");
const QString FN_INCORRECT_NUMBER("incorrect_number");
const QString FN_TRIAL_START_TIME("trial_start_time");
const QString FN_RESPONDED("responded");
const QString FN_RESPONSE_TIME("response_time");
const QString FN_RESPONSE_LATENCY_MS("response_latency_ms");
const QString FN_CORRECT("correct");
const QString FN_INCORRECT("incorrect");


IDED3DTrial::IDED3DTrial(CamcopsApp& app, DatabaseManager& db,
                         const int load_pk) :
    DatabaseObject(app, db, TRIAL_TABLENAME),
    m_stage_num_zero_based(-1),
    m_trial_num_zero_based(-1)
{
    addField(FN_FK_TO_TASK, QVariant::Int);
    // More keys
    addField(FN_TRIAL, QVariant::Int, true);  // 1-based trial number within this session
    addField(FN_STAGE, QVariant::Int, true);  // 1-based stage number within this session
    // Locations
    addField(FN_CORRECT_LOCATION, QVariant::Int);
    addField(FN_INCORRECT_LOCATION, QVariant::Int);
    // Stimuli
    addField(FN_CORRECT_SHAPE, QVariant::Int);
    addField(FN_CORRECT_COLOUR, QVariant::Int);  // was string prior to 2.0.0
    addField(FN_CORRECT_NUMBER, QVariant::Int);
    addField(FN_INCORRECT_SHAPE, QVariant::Int);
    addField(FN_INCORRECT_COLOUR, QVariant::Int);  // was string prior to 2.0.0
    addField(FN_INCORRECT_NUMBER, QVariant::Int);
    // Trial
    addField(FN_TRIAL_START_TIME, QVariant::DateTime);
    // Response
    addField(FN_RESPONDED, QVariant::Bool);
    addField(FN_RESPONSE_TIME, QVariant::DateTime);
    addField(FN_RESPONSE_LATENCY_MS, QVariant::Int);
    addField(FN_CORRECT, QVariant::Bool);
    addField(FN_INCORRECT, QVariant::Bool);

    load(load_pk);
}


IDED3DTrial::IDED3DTrial(const IDED3DStage& stage,
                         const int trial_num_zero_based,
                         CamcopsApp& app, DatabaseManager& db) :
    IDED3DTrial::IDED3DTrial(app, db, dbconst::NONEXISTENT_PK)  // delegating constructor
{
    m_stage_num_zero_based = stage.stageNumZeroBased();
    // Keys
    setValue(FN_FK_TO_TASK, stage.taskId());
    setValue(FN_TRIAL, trial_num_zero_based + 1);
    setValue(FN_STAGE, m_stage_num_zero_based + 1);

    // Locations
    QVector<int> possible_locations = range(stage.nPossibleLocations());
    setValue(FN_CORRECT_LOCATION, dwor(possible_locations));
    setValue(FN_INCORRECT_LOCATION, dwor(possible_locations));

    // Stimuli
    int correct_shape = drawreplace(stage.correctStimulusShapes());
    setValue(FN_CORRECT_SHAPE, correct_shape);
    int correct_colour = drawreplace(stage.correctStimulusColours());
    setValue(FN_CORRECT_COLOUR, correct_colour);
    int correct_number = drawreplace(stage.correctStimulusNumbers());
    setValue(FN_CORRECT_NUMBER, correct_number);

    if (stage.incorrectStimulusCanOverlap()) {
        setValue(FN_INCORRECT_SHAPE, drawreplace(stage.incorrectStimulusShapes()));
        setValue(FN_INCORRECT_COLOUR, drawreplace(stage.incorrectStimulusColours()));
        setValue(FN_INCORRECT_NUMBER, drawreplace(stage.incorrectStimulusNumbers()));
    } else {
        // Constraint for compound discriminations: the incorrect stimulus
        // should never match the correct one in any aspect. Remove the correct
        // exemplar from consideration before drawing, as follows.
        setValue(FN_INCORRECT_SHAPE, drawreplace(subtract(
                        stage.incorrectStimulusShapes(),
                        {correct_shape})));
        setValue(FN_INCORRECT_COLOUR, drawreplace(subtract(
                        stage.incorrectStimulusColours(),
                        {correct_colour})));
        setValue(FN_INCORRECT_NUMBER, drawreplace(subtract(
                        stage.incorrectStimulusNumbers(),
                        {correct_number})));
    }

    // Trial
    setValue(FN_TRIAL_START_TIME, QVariant());  // NULL

    // Response
    setValue(FN_RESPONDED, QVariant());
    setValue(FN_RESPONSE_TIME, QVariant());
    setValue(FN_RESPONSE_LATENCY_MS, QVariant());
    setValue(FN_CORRECT, QVariant());
    setValue(FN_INCORRECT, QVariant());

    m_trial_num_zero_based = trial_num_zero_based;

    save();
}


void IDED3DTrial::recordTrialStart()
{
    const QDateTime now = datetime::now();
    setValue(FN_TRIAL_START_TIME, now);
    save();
}


void IDED3DTrial::recordResponse(const bool correct)
{
    const QDateTime now = datetime::now();
    setValue(FN_RESPONDED, true);
    setValue(FN_RESPONSE_TIME, now);
    setValue(FN_RESPONSE_LATENCY_MS,
             valueDateTime(FN_TRIAL_START_TIME).msecsTo(now));
    setValue(FN_CORRECT, correct);
    setValue(FN_INCORRECT, !correct);
    save();
}


int IDED3DTrial::stageZeroBased() const
{
    return m_stage_num_zero_based;
}


bool IDED3DTrial::wasCorrect() const
{
    return valueBool(FN_CORRECT);
}


int IDED3DTrial::correctLocation() const
{
    return valueInt(FN_CORRECT_LOCATION);
}


int IDED3DTrial::correctShape() const
{
    return valueInt(FN_CORRECT_SHAPE);
}


int IDED3DTrial::correctColour() const
{
    return valueInt(FN_CORRECT_COLOUR);
}


int IDED3DTrial::correctNumber() const
{
    return valueInt(FN_CORRECT_NUMBER);
}


int IDED3DTrial::incorrectLocation() const
{
    return valueInt(FN_INCORRECT_LOCATION);
}


int IDED3DTrial::incorrectShape() const
{
    return valueInt(FN_INCORRECT_SHAPE);
}


int IDED3DTrial::incorrectColour() const
{
    return valueInt(FN_INCORRECT_COLOUR);
}


int IDED3DTrial::incorrectNumber() const
{
    return valueInt(FN_INCORRECT_NUMBER);
}


QString IDED3DTrial::summary() const
{
    return QString("Trial: "
                   "correct {shape %1, colour %2, number %3, location %4}, "
                   "incorrect {shape %5, colour %6, number %7, location %8}")
            .arg(correctShape())
            .arg(correctColour())
            .arg(correctNumber())
            .arg(correctLocation())
            .arg(incorrectShape())
            .arg(incorrectColour())
            .arg(incorrectNumber())
            .arg(incorrectLocation());
}
