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

// #define DEBUG_IS_COMPLETE

#include "photo.h"
#include "common/textconst.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/quphoto.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"

const QString Photo::PHOTO_TABLENAME("photo");

const QString DESCRIPTION("description");
const QString PHOTO_BLOBID("photo_blobid");
// const QString ROTATION("rotation");  // DEFUNCT in v2


void initializePhoto(TaskFactory& factory)
{
    static TaskRegistrar<Photo> registered(factory);
}


Photo::Photo(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    Task(app, db, PHOTO_TABLENAME, false, true, false)  // ... anon, clin, resp
{
    addField(DESCRIPTION, QVariant::String);
    addField(PHOTO_BLOBID, QVariant::Int);  // FK to BLOB table
    // addField(ROTATION, QVariant::Int);  // DEFUNCT in v2

    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Class info
// ============================================================================

QString Photo::shortname() const
{
    return "Photo";
}


QString Photo::longname() const
{
    return tr("Photograph");
}


QString Photo::description() const
{
    return tr("Photograph with accompanying detail.");
}


QString Photo::infoFilenameStem() const
{
    return "clinical";
}


// ============================================================================
// Instance info
// ============================================================================

bool Photo::isComplete() const
{
#ifdef DEBUG_IS_COMPLETE
    qDebug() << "valueIsNullOrEmpty(DESCRIPTION)" << valueIsNullOrEmpty(DESCRIPTION);
    qDebug() << "valueIsNull(PHOTO_BLOBID)" << valueIsNull(PHOTO_BLOBID);
#endif
    return !valueIsNullOrEmpty(DESCRIPTION) && !valueIsNull(PHOTO_BLOBID);
}


QStringList Photo::summary() const
{
    return QStringList{valueString(DESCRIPTION)};
}


QStringList Photo::detail() const
{
    return completenessInfo() + summary();
}


OpenableWidget* Photo::editor(const bool read_only)
{
    QuPagePtr page((new QuPage{
        new QuText(tr(
            "1. Ensure consent is documented, if applicable.\n"
            "2. Take a photograph.\n"
            "3. Enter a description."
        )),
        new QuText(TextConst::description()),
        new QuTextEdit(fieldRef(DESCRIPTION)),
        new QuPhoto(blobFieldRef(PHOTO_BLOBID, false)),
    })->setTitle(tr("Clinical photograph")));

    auto questionnaire = new Questionnaire(m_app, {page});
    questionnaire->setType(QuPage::PageType::Clinician);
    questionnaire->setReadOnly(read_only);
    return questionnaire;
}
