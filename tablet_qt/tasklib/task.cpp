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

#include "task.h"
#include <QObject>
#include <QVariant>
#include "core/camcopsapp.h"
#include "common/preprocessor_aid.h"
#include "common/textconst.h"
#include "common/uiconst.h"
#include "common/varconst.h"
#include "db/databasemanager.h"
#include "dbobjects/patient.h"
#include "lib/datetime.h"
#include "lib/version.h"
#include "maths/mathfunc.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/questionnairefunc.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/qulineedit.h"
#include "questionnairelib/qupage.h"
#include "questionnairelib/quspacer.h"
#include "version/camcopsversion.h"
#include "widgets/openablewidget.h"
#include "widgets/screenlikegraphicsview.h"

const QString Task::PATIENT_FK_FIELDNAME("patient_id");
const QString Task::FIRSTEXIT_IS_FINISH_FIELDNAME("firstexit_is_finish");
const QString Task::FIRSTEXIT_IS_ABORT_FIELDNAME("firstexit_is_abort");
const QString Task::WHEN_FIRSTEXIT_FIELDNAME("when_firstexit");
const QString Task::EDITING_TIME_S_FIELDNAME("editing_time_s");

const QString Task::CLINICIAN_SPECIALTY("clinician_specialty");
const QString Task::CLINICIAN_NAME("clinician_name");
const QString Task::CLINICIAN_PROFESSIONAL_REGISTRATION("clinician_professional_registration");
const QString Task::CLINICIAN_POST("clinician_post");
const QString Task::CLINICIAN_SERVICE("clinician_service");
const QString Task::CLINICIAN_CONTACT_DETAILS("clinician_contact_details");

const QString Task::RESPONDENT_NAME("respondent_name");
const QString Task::RESPONDENT_RELATIONSHIP("respondent_relationship");


Task::Task(CamcopsApp& app,
           DatabaseManager& db,
           const QString& tablename,
           const bool is_anonymous,
           const bool has_clinician,
           const bool has_respondent) :
    DatabaseObject(app, db, tablename, dbconst::PK_FIELDNAME, true, true),
    m_patient(nullptr),
    m_editing(false),
    m_is_complete_is_cached(false),
    m_is_anonymous(is_anonymous),
    m_has_clinician(has_clinician),
    m_has_respondent(has_respondent)
{
    // WATCH OUT: you can't call a derived class's overloaded function
    // here; its vtable is incomplete.
    // http://stackoverflow.com/questions/6561429/calling-virtual-function-of-derived-class-from-base-class-constructor

    addField(FIRSTEXIT_IS_FINISH_FIELDNAME, QVariant::Bool);
    addField(FIRSTEXIT_IS_ABORT_FIELDNAME, QVariant::Bool);
    addField(WHEN_FIRSTEXIT_FIELDNAME, QVariant::DateTime);
    addField(Field(EDITING_TIME_S_FIELDNAME, QVariant::Double).setDefaultValue(0.0));

    if (!is_anonymous) {
        addField(PATIENT_FK_FIELDNAME, QVariant::Int);
    }
    if (has_clinician) {
        addField(CLINICIAN_SPECIALTY, QVariant::String);
        addField(CLINICIAN_NAME, QVariant::String);
        addField(CLINICIAN_PROFESSIONAL_REGISTRATION, QVariant::String);
        addField(CLINICIAN_POST, QVariant::String);
        addField(CLINICIAN_SERVICE, QVariant::String);
        addField(CLINICIAN_CONTACT_DETAILS, QVariant::String);
    }
    if (has_respondent) {
        addField(RESPONDENT_NAME, QVariant::String);
        addField(RESPONDENT_RELATIONSHIP, QVariant::String);
    }

    connect(this, &DatabaseObject::dataChanged,
            this, &Task::onDataChanged);
}


// ============================================================================
// General info
// ============================================================================

QString Task::implementationTypeDescription() const
{
    switch (implementationType()) {
    case TaskImplementationType::Full:
#ifdef COMPILER_WANTS_DEFAULT_IN_EXHAUSTIVE_SWITCH
    default:
#endif
        return TextConst::fullTask();
    case TaskImplementationType::UpgradableSkeleton:
        return TextConst::DATA_COLLECTION_ONLY_UNLESS_UPGRADED_SYMBOL;
    case TaskImplementationType::Skeleton:
        return TextConst::DATA_COLLECTION_ONLY_SYMBOL;
    }
}


QString Task::menuTitleSuffix() const
{
    QStringList suffixes;
    if (hasClinician()) {
        suffixes += TextConst::HAS_CLINICIAN_SYMBOL;
    }
    if (hasRespondent()) {
        suffixes += TextConst::HAS_RESPONDENT_SYMBOL;
    }
    switch (implementationType()) {
    case TaskImplementationType::Full:
        break;
    case TaskImplementationType::UpgradableSkeleton:
        suffixes += TextConst::DATA_COLLECTION_ONLY_UNLESS_UPGRADED_SYMBOL;
        break;
    case TaskImplementationType::Skeleton:
        suffixes += TextConst::DATA_COLLECTION_ONLY_SYMBOL;
        break;
    }
    if (isExperimental()) {
        suffixes += TextConst::EXPERIMENTAL_SYMBOL;
    }
    if (isDefunct()) {
        suffixes += TextConst::DEFUNCT_SYMBOL;
    }
    return suffixes.isEmpty() ? ""
                              : QString(" <i>[%1]</i>").arg(suffixes.join(""));
}


QString Task::menutitle() const
{
    return QString("%1 (%2)%3").arg(longname(), shortname(), menuTitleSuffix());
}


QString Task::menuSubtitleSuffix() const
{
    auto makeSuffix = [](const QString& title,
                         const QString& subtitle) -> QString {
        return QString("%1: %2").arg(title, subtitle);
    };

    QStringList suffixes;
    if (hasClinician()) {
        suffixes += makeSuffix(
            TextConst::HAS_CLINICIAN_SYMBOL,
            TextConst::hasClinicianSubtitleSuffix()
        );
    }
    if (hasRespondent()) {
        suffixes += makeSuffix(
            TextConst::HAS_RESPONDENT_SYMBOL,
            TextConst::hasRespondentSubtitleSuffix()
        );
    }
    switch (implementationType()) {
    case TaskImplementationType::Full:
        break;
    case TaskImplementationType::UpgradableSkeleton:
        suffixes += makeSuffix(
            TextConst::DATA_COLLECTION_ONLY_UNLESS_UPGRADED_SYMBOL,
            TextConst::dataCollectionOnlyUnlessUpgradedSubtitleSuffix()
        );
        break;
    case TaskImplementationType::Skeleton:
        suffixes += makeSuffix(
            TextConst::DATA_COLLECTION_ONLY_SYMBOL,
            TextConst::dataCollectionOnlySubtitleSuffix()
        );
        break;
    }
    if (isExperimental()) {
        suffixes += makeSuffix(
            TextConst::EXPERIMENTAL_SYMBOL,
            TextConst::experimentalSubtitleSuffix()
        );
    }
    if (isDefunct()) {
        suffixes += makeSuffix(
            TextConst::DEFUNCT_SYMBOL,
            TextConst::defunctSubtitleSuffix()
        );
    }
    return suffixes.isEmpty() ? ""
                              : QString(" <i>[%1]</i>").arg(suffixes.join(" "));
}


QString Task::menusubtitle() const
{
    return description() + menuSubtitleSuffix();
}


bool Task::isCrippled() const
{
    return implementationType() == TaskImplementationType::Skeleton ||
            !hasExtraStrings();
}


bool Task::hasExtraStrings() const
{
    return m_app.hasExtraStrings(xstringTaskname());
}


QString Task::infoFilenameStem() const
{
    return m_tablename;
}


QString Task::xstringTaskname() const
{
    return m_tablename;
}


QString Task::instanceTitle(bool with_pid) const
{
    if (isAnonymous() || !with_pid) {
        return QString("%1; %2").arg(
            shortname(),
            whenCreated().toString(datetime::SHORT_DATETIME_FORMAT));
    }
    Patient* pt = patient();
    return QString("%1; %2; %3").arg(
        shortname(),
        pt ? pt->surnameUpperForename() : tr("MISSING PATIENT"),
        whenCreated().toString(datetime::SHORT_DATETIME_FORMAT));
}


bool Task::isAnonymous() const
{
    return m_is_anonymous;
}


bool Task::hasClinician() const
{
    return m_has_clinician;
}


bool Task::hasRespondent() const
{
    return m_has_respondent;
}


bool Task::isTaskPermissible(QString& why_not_permissible) const
{
    const QVariant commercial = m_app.var(varconst::IP_USE_COMMERCIAL);
    const QVariant clinical = m_app.var(varconst::IP_USE_CLINICAL);
    const QVariant educational = m_app.var(varconst::IP_USE_EDUCATIONAL);
    const QVariant research = m_app.var(varconst::IP_USE_RESEARCH);

    auto not_definitely_false = [](const QVariant& v) -> bool {
        return !mathfunc::eq(v, false);
    };
    auto is_unknown = [](const QVariant& v) -> bool {
        return v.isNull() || v.toInt() == CommonOptions::UNKNOWN_INT;
    };

    const QString PROHIBITED_YES(tr(
        " You have said you ARE using this software in that context "
        "(see Settings). To use this task, you must seek permission "
        "from the copyright holder (see Task Information)."));
    const QString PROHIBITED_UNKNOWN(tr(
        " You have NOT SAID whether you are using this "
        "software in that context (see Settings)."));
    const QString PERMISSIBLE(tr("Task permissible"));

    if (prohibitsCommercial() && not_definitely_false(commercial)) {
        why_not_permissible =
            tr("Task not allowed for commercial use (see Task Information).") +
            (is_unknown(commercial) ? PROHIBITED_UNKNOWN
                                    : PROHIBITED_YES);
        return false;
    }
    if (prohibitsClinical() && not_definitely_false(clinical)) {
        why_not_permissible =
            tr("Task not allowed for research use (see Task Information).") +
            (is_unknown(clinical) ? PROHIBITED_UNKNOWN
                                  : PROHIBITED_YES);
        return false;
    }
    if (prohibitsEducational() && not_definitely_false(educational)) {
        why_not_permissible =
            tr("Task not allowed for educational use (see Task Information).") +
            (is_unknown(educational) ? PROHIBITED_UNKNOWN
                                     : PROHIBITED_YES);
        return false;
    }
    if (prohibitsResearch() && not_definitely_false(research)) {
        why_not_permissible =
            tr("Task not allowed for research use (see Task Information).") +
            (is_unknown(research) ? PROHIBITED_UNKNOWN
                                  : PROHIBITED_YES);
        return false;
    }

    why_not_permissible = PERMISSIBLE;
    return true;
}


Version Task::minimumServerVersion() const
{
    return camcopsversion::MINIMUM_SERVER_VERSION;
}


bool Task::isTaskUploadable(QString& why_not_uploadable) const
{
    bool server_has_table;
    Version min_client_version;
    Version min_server_version;
    const Version overall_min_server_version = Task::minimumServerVersion();
    const Version server_version = m_app.serverVersion();
    const QString table = tablename();
    bool may_upload = m_app.mayUploadTable(
                table, server_version,
                server_has_table, min_client_version, min_server_version);
#if 0
    qDebug() << "table" << table
             << "server_version" << server_version
             << "may_upload" << may_upload
             << "server_has_table" << server_has_table
             << "min_client_version" << min_client_version
             << "min_server_version" << min_server_version;
#endif
    if (may_upload) {
        why_not_uploadable = tr("Task uploadable");
    } else {
        if (!server_has_table) {
            why_not_uploadable = tr(
                    "Table '%1' absent on server.").arg(table);
        } else if (camcopsversion::CAMCOPS_CLIENT_VERSION < min_client_version) {
            why_not_uploadable = tr(
                    "Server requires client version >=%1 for table '%2', "
                    "but we are only client version %3."
                    ).arg(min_client_version.toString(),
                          table,
                          camcopsversion::CAMCOPS_CLIENT_VERSION.toString());
        } else if (server_version < overall_min_server_version) {
            why_not_uploadable = tr(
                    "This client requires server version >=%1, "
                    "but the server is only version %2."
                    ).arg(overall_min_server_version.toString(),
                          server_version.toString());
        } else if (server_version < min_server_version) {
            why_not_uploadable = tr(
                    "This client requires server version >=%1 for table '%2', "
                    "but the server is only version %3."
                    ).arg(min_server_version.toString(),
                          table,
                          server_version.toString());
        } else {
            why_not_uploadable = "? [bug in Task::isTaskUploadable, "
                                 "versus CamcopsApp::mayUploadTable]";
        }
    }
    return may_upload;
}


// ============================================================================
// Tables
// ============================================================================

QStringList Task::allTables() const
{
    QStringList all_tables(tablename());
    all_tables += ancillaryTables();
    return all_tables;
}


void Task::makeTables()
{
    makeTable();
    makeAncillaryTables();
}


int Task::count(const WhereConditions& where) const
{
    return m_db.count(m_tablename, where);
}


int Task::countForPatient(const int patient_id) const
{
    if (isAnonymous()) {
        return 0;
    }
    WhereConditions where;
    where.add(PATIENT_FK_FIELDNAME,  patient_id);
    return count(where);
}


void Task::upgradeDatabase(const Version& old_version,
                           const Version& new_version)
{
    Q_UNUSED(old_version)
    Q_UNUSED(new_version)
}

// ============================================================================
// Database object functions
// ============================================================================

bool Task::load(const int pk)
{
    if (pk == dbconst::NONEXISTENT_PK) {
        return false;
    }
    return DatabaseObject::load(pk);
}


bool Task::save()
{
    // Sanity checks before we permit saving
    if (!isAnonymous() && value(PATIENT_FK_FIELDNAME).isNull()) {
        uifunc::stopApp("Task has no patient ID (and is not anonymous); "
                        "cannot save");
    }
    return DatabaseObject::save();
}


// ============================================================================
// Specific info
// ============================================================================

bool Task::isCompleteCached() const
{
    if (!m_is_complete_is_cached) {
        m_is_complete_cached_value = isComplete();
        m_is_complete_is_cached = true;
    }
    return m_is_complete_cached_value;
}


void Task::onDataChanged()
{
    m_is_complete_is_cached = false;
}


QStringList Task::summary() const
{
    return QStringList{tr("MISSING SUMMARY")};
}


QStringList Task::detail() const
{
    QStringList result = completenessInfo() + summary();
    result.append("");  // blank line
    result += recordSummaryLines();
    return result;
}


OpenableWidget* Task::editor(const bool read_only)
{
    Q_UNUSED(read_only)
    qWarning() << "Base class Task::edit called - not a good thing!";
    return nullptr;
}


// ============================================================================
// Assistance functions
// ============================================================================

QDateTime Task::whenCreated() const
{
    return value(dbconst::CREATION_TIMESTAMP_FIELDNAME)
        .toDateTime();
}


QStringList Task::completenessInfo() const
{
    QStringList result;
    if (!isCompleteCached()) {
        result.append(incompleteMarker());
    }
    return result;
}


QString Task::xstring(const QString& stringname,
                      const QString& default_str) const
{
    return m_app.xstring(xstringTaskname(), stringname, default_str);
}


QString Task::appstring(const QString& stringname,
                        const QString& default_str) const
{
    return m_app.appstring(stringname, default_str);
}


QStringList Task::fieldSummaries(const QString& xstringprefix,
                                 const QString& xstringsuffix,
                                 const QString& spacer,
                                 const QString& fieldprefix,
                                 const int first,
                                 const int last,
                                 const QString& suffix) const
{
    using stringfunc::strseq;
    const QStringList xstringnames = strseq(xstringprefix, first, last,
                                            xstringsuffix);
    const QStringList fieldnames = strseq(fieldprefix, first, last);
    QStringList list;
    for (int i = 0; i < fieldnames.length(); ++i) {
        const QString& fieldname = fieldnames.at(i);
        const QString& xstringname = xstringnames.at(i);
        list.append(fieldSummary(fieldname, xstring(xstringname),
                                 spacer, suffix));
    }
    return list;
}


QStringList Task::fieldSummariesYesNo(const QString& xstringprefix,
                                      const QString& xstringsuffix,
                                      const QString& spacer,
                                      const QString& fieldprefix,
                                      const int first,
                                      const int last,
                                      const QString& suffix) const
{
    using stringfunc::strseq;
    const QStringList xstringnames = strseq(xstringprefix, first, last,
                                            xstringsuffix);
    const QStringList fieldnames = strseq(fieldprefix, first, last);
    QStringList list;
    for (int i = 0; i < fieldnames.length(); ++i) {
        const QString& fieldname = fieldnames.at(i);
        const QString& xstringname = xstringnames.at(i);
        list.append(fieldSummaryYesNo(fieldname, xstring(xstringname),
                                      spacer, suffix));
    }
    return list;
}


QStringList Task::clinicianDetails(const QString& separator) const
{
    if (!hasClinician()) {
        return QStringList();
    }
    return QStringList{
        fieldSummary(CLINICIAN_SPECIALTY, TextConst::clinicianSpecialty(),
                     separator),
        fieldSummary(CLINICIAN_NAME, TextConst::clinicianName(), separator),
        fieldSummary(CLINICIAN_PROFESSIONAL_REGISTRATION,
                     TextConst::clinicianProfessionalRegistration(),
                     separator),
        fieldSummary(CLINICIAN_POST, TextConst::clinicianPost(), separator),
        fieldSummary(CLINICIAN_SERVICE, TextConst::clinicianService(),
                     separator),
        fieldSummary(CLINICIAN_CONTACT_DETAILS,
                     TextConst::clinicianContactDetails(), separator),
    };
}


QStringList Task::respondentDetails() const
{
    if (!hasRespondent()) {
        return QStringList();
    }
    return QStringList{
        fieldSummary(RESPONDENT_NAME, TextConst::respondentNameThirdPerson()),
        fieldSummary(RESPONDENT_RELATIONSHIP,
                     TextConst::respondentRelationshipThirdPerson()),
    };
}


// ============================================================================
// Editing
// ============================================================================

void Task::setupForEditingAndSave(const int patient_id)
{
    if (!isAnonymous()) {
        setPatient(patient_id);
    }
    setDefaultClinicianVariablesAtFirstUse();
    setDefaultsAtFirstUse();
    save();
}


double Task::editingTimeSeconds() const
{
    return valueDouble(EDITING_TIME_S_FIELDNAME);
}


void Task::setDefaultClinicianVariablesAtFirstUse()
{
    if (!m_has_clinician) {
        return;
    }
    setValue(CLINICIAN_SPECIALTY, m_app.varString(varconst::DEFAULT_CLINICIAN_SPECIALTY));
    setValue(CLINICIAN_NAME, m_app.varString(varconst::DEFAULT_CLINICIAN_NAME));
    setValue(CLINICIAN_PROFESSIONAL_REGISTRATION, m_app.varString(varconst::DEFAULT_CLINICIAN_PROFESSIONAL_REGISTRATION));
    setValue(CLINICIAN_POST, m_app.varString(varconst::DEFAULT_CLINICIAN_POST));
    setValue(CLINICIAN_SERVICE, m_app.varString(varconst::DEFAULT_CLINICIAN_SERVICE));
    setValue(CLINICIAN_CONTACT_DETAILS, m_app.varString(varconst::DEFAULT_CLINICIAN_CONTACT_DETAILS));
}


OpenableWidget* Task::makeGraphicsWidget(
        QGraphicsScene* scene,
        const QColor& background_colour,
        const bool fullscreen,
        const bool esc_can_abort)
{
    auto view = new ScreenLikeGraphicsView(scene);
    view->setBackgroundColour(background_colour);
    auto widget = new OpenableWidget();
    widget->setWidgetAsOnlyContents(view, 0, fullscreen, esc_can_abort);
    return widget;
}


OpenableWidget* Task::makeGraphicsWidgetForImmediateEditing(
        QGraphicsScene* scene,
        const QColor& background_colour,
        const bool fullscreen,
        const bool esc_can_abort)
{
    OpenableWidget* widget = makeGraphicsWidget(scene, background_colour,
                                                fullscreen, esc_can_abort);
    connect(widget, &OpenableWidget::aborting,
            this, &Task::onEditFinishedAbort);
    onEditStarted();
    return widget;
}


QuElement* Task::getClinicianQuestionnaireBlockRawPointer()
{
    return questionnairefunc::defaultGridRawPointer({
        {TextConst::clinicianSpecialty(),
         new QuLineEdit(fieldRef(CLINICIAN_SPECIALTY))},
        {TextConst::clinicianName(),
         new QuLineEdit(fieldRef(CLINICIAN_NAME))},
        {TextConst::clinicianProfessionalRegistration(),
         new QuLineEdit(fieldRef(CLINICIAN_PROFESSIONAL_REGISTRATION))},
        {TextConst::clinicianPost(),
         new QuLineEdit(fieldRef(CLINICIAN_POST))},
        {TextConst::clinicianService(),
         new QuLineEdit(fieldRef(CLINICIAN_SERVICE))},
        {TextConst::clinicianContactDetails(),
         new QuLineEdit(fieldRef(CLINICIAN_CONTACT_DETAILS))},
    }, uiconst::DEFAULT_COLSPAN_Q, uiconst::DEFAULT_COLSPAN_A);
}


QuElementPtr Task::getClinicianQuestionnaireBlockElementPtr()
{
    return QuElementPtr(getClinicianQuestionnaireBlockRawPointer());
}


QuPagePtr Task::getClinicianDetailsPage()
{
    return QuPagePtr(
        (new QuPage{getClinicianQuestionnaireBlockRawPointer()})
            ->setTitle(TextConst::clinicianDetails())
            ->setType(QuPage::PageType::Clinician)
    );
}


bool Task::isClinicianComplete() const
{
    if (!m_has_clinician) {
        return false;
    }
    return !valueIsNullOrEmpty(CLINICIAN_NAME);
}


bool Task::isRespondentComplete() const
{
    if (!m_has_respondent) {
        return false;
    }
    return !valueIsNullOrEmpty(RESPONDENT_NAME) &&
            !valueIsNullOrEmpty(RESPONDENT_RELATIONSHIP);
}


QVariant Task::respondentRelationship() const
{
    if (!m_has_respondent) {
        return QVariant();
    }
    return value(RESPONDENT_RELATIONSHIP);
}


QuElement* Task::getRespondentQuestionnaireBlockRawPointer(
        const bool second_person)
{
    const QString name = second_person
            ? TextConst::respondentNameSecondPerson()
            : TextConst::respondentNameThirdPerson();
    const QString relationship = second_person
            ? TextConst::respondentRelationshipSecondPerson()
            : TextConst::respondentRelationshipThirdPerson();
    return questionnairefunc::defaultGridRawPointer({
        {name, new QuLineEdit(fieldRef(RESPONDENT_NAME))},
        {relationship, new QuLineEdit(fieldRef(RESPONDENT_RELATIONSHIP))},
    }, uiconst::DEFAULT_COLSPAN_Q, uiconst::DEFAULT_COLSPAN_A);
}


QuElementPtr Task::getRespondentQuestionnaireBlockElementPtr(
        const bool second_person)
{
    return QuElementPtr(getRespondentQuestionnaireBlockRawPointer(second_person));
}


QuPagePtr Task::getRespondentDetailsPage(const bool second_person)
{
    return QuPagePtr(
        (new QuPage{getRespondentQuestionnaireBlockRawPointer(second_person)})
            ->setTitle(TextConst::respondentDetails())
            ->setType(second_person ? QuPage::PageType::Patient
                                    : QuPage::PageType::Clinician)
    );
}


QuPagePtr Task::getClinicianAndRespondentDetailsPage(const bool second_person)
{
    return QuPagePtr(
        (new QuPage{
            getClinicianQuestionnaireBlockRawPointer(),
            questionnairefunc::defaultGridRawPointer({
                {"", new QuSpacer()},
            }, uiconst::DEFAULT_COLSPAN_Q, uiconst::DEFAULT_COLSPAN_A),
            getRespondentQuestionnaireBlockRawPointer(second_person)
        })
            ->setTitle(TextConst::clinicianAndRespondentDetails())
            ->setType(second_person ? QuPage::PageType::ClinicianWithPatient
                                    : QuPage::PageType::Clinician)
    );
}


NameValueOptions Task::makeOptionsFromXstrings(const QString& xstring_prefix,
                                               const int first, const int last,
                                               const QString& xstring_suffix)
{
    using stringfunc::strnum;
    NameValueOptions options;
    if (first > last) {  // descending order
        for (int i = first; i >= last; --i) {
            options.append(NameValuePair(
                               xstring(strnum(xstring_prefix, i, xstring_suffix)),
                               i));
        }
    } else {  // ascending order
        for (int i = first; i <= last; ++i) {
            options.append(NameValuePair(
                               xstring(strnum(xstring_prefix, i, xstring_suffix)),
                               i));
        }
    }
    return options;
}


void Task::onEditStarted()
{
    m_editing = true;
    m_editing_started = datetime::now();
}


void Task::onEditFinished(const bool aborted)
{
    if (!m_editing) {
        qDebug() << Q_FUNC_INFO << "wasn't editing";
        return;
    }
    m_editing = false;
    // Time
    const QDateTime now = datetime::now();
    double editing_time_s = valueDouble(EDITING_TIME_S_FIELDNAME);
    editing_time_s += datetime::doubleSecondsFrom(m_editing_started, now);
    setValue(EDITING_TIME_S_FIELDNAME, editing_time_s);
    // Exit flags
    if (!valueBool(FIRSTEXIT_IS_FINISH_FIELDNAME)
            && !valueBool(FIRSTEXIT_IS_ABORT_FIELDNAME)) {
        // First exit, so set flags:
        setValue(WHEN_FIRSTEXIT_FIELDNAME, now);
        setValue(FIRSTEXIT_IS_ABORT_FIELDNAME, aborted);
        setValue(FIRSTEXIT_IS_FINISH_FIELDNAME, !aborted);
    }
    save();
    if (aborted) {
        emit editingAborted();
    } else {
        emit editingFinished();
    }
}


void Task::onEditFinishedProperly()
{
    onEditFinished(false);
}


void Task::onEditFinishedAbort()
{
    onEditFinished(true);
}


// ============================================================================
// Patient functions (for non-anonymous tasks)
// ============================================================================

void Task::setPatient(const int patient_id)
{
    // It's a really dangerous thing to set a patient ID invalidly, so this
    // function will just stop the app if something stupid is attempted.
    if (isAnonymous()) {
        uifunc::stopApp("Attempt to set patient ID for an anonymous task");
    }
    if (!value(PATIENT_FK_FIELDNAME).isNull()) {
        uifunc::stopApp("Setting patient ID, but it was already set");
    }
    setValue(PATIENT_FK_FIELDNAME, patient_id);
    m_patient.clear();
}


void Task::moveToPatient(const int patient_id)
{
    // This is used for patient merges.
    // It is therefore more liberal than setPatient().
    if (isAnonymous()) {
        qWarning() << "Attempt to set patient ID for an anonymous task";
        return;
    }
    setValue(PATIENT_FK_FIELDNAME, patient_id);
    m_patient.clear();
}


Patient* Task::patient() const
{
    if (!m_patient && !isAnonymous()) {
        const QVariant patient_id_var = value(PATIENT_FK_FIELDNAME);
        if (!patient_id_var.isNull()) {
            const int patient_id = patient_id_var.toInt();
            m_patient = QSharedPointer<Patient>(
                    new Patient(m_app, m_db, patient_id));
        }
    }
    return m_patient.data();
}


QString Task::getPatientName() const
{
    Patient* pt = patient();
    if (!pt) {
        return "";
    }
    return pt->forenameSurname();
}


bool Task::isFemale() const
{
    Patient* pt = patient();
    return pt ? pt->isFemale() : false;
}


bool Task::isMale() const
{
    Patient* pt = patient();
    return pt ? pt->isMale() : false;
}


// ============================================================================
// Translatable text
// ============================================================================

QString Task::incompleteMarker()
{
    return tr("<b>(INCOMPLETE)</b>");
}
