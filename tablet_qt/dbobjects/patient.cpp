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

// #define DEBUG_JSON

#include "patient.h"
#include <limits>
#include <QDebug>
#include <QtAlgorithms>  // for qsort()
#include "core/camcopsapp.h"
#include "common/dbconst.h"
#include "common/design_defines.h"
#include "db/ancillaryfunc.h"
#include "db/dbfunc.h"
#include "db/dbnestabletransaction.h"
#include "dbobjects/idnumdescription.h"
#include "dbobjects/patientidnum.h"
#include "dbobjects/patientidnumsorter.h"
#include "dialogs/nvpchoicedialog.h"
#include "lib/containers.h"
#include "lib/datetime.h"
#include "lib/idpolicy.h"
#include "lib/uifunc.h"
#include "questionnairelib/commonoptions.h"
#include "questionnairelib/qubutton.h"
#include "questionnairelib/qudatetime.h"
#include "questionnairelib/questionnaire.h"
#include "questionnairelib/qugridcontainer.h"
#include "questionnairelib/quheading.h"
#include "questionnairelib/quimage.h"
#include "questionnairelib/qulineedit.h"
#include "questionnairelib/qulineeditint64.h"
#include "questionnairelib/qumcq.h"
#include "questionnairelib/qulineeditnhsnumber.h"
#include "questionnairelib/qupage.h"
#include "questionnairelib/qutext.h"
#include "questionnairelib/qutextedit.h"
#include "tasklib/taskfactory.h"
#include "widgets/openablewidget.h"

const QString Patient::TABLENAME("patient");

// Important that these match ID policy names:
const QString FORENAME_FIELD("forename");
const QString SURNAME_FIELD("surname");
const QString DOB_FIELD("dob");
const QString SEX_FIELD("sex");

const QString IDNUM_FIELD_PREFIX("idnum");
const QString IDNUM_FIELD_FORMAT(IDNUM_FIELD_PREFIX + "%1");
const QString ANY_IDNUM_POLICYNAME("anyidnum");
const QString OTHER_IDNUM_POLICYNAME("otheridnum");
const QString OTHER_DETAILS_POLICYNAME("otherdetails");  // the *policy* name!

// Not so important prior to v2.2.8, then more important (part of policies):
const QString ADDRESS_FIELD("address");
const QString GP_FIELD("gp");
const QString OTHER_DETAILS_FIELD("other");  // the *field* name!

const qint64 MIN_ID_NUM_VALUE = 0;
const qint64 MAX_ID_NUM_VALUE = std::numeric_limits<qint64>::max();

const QString TAG_POLICY_APP_OK("app_ok");
const QString TAG_POLICY_APP_FAIL("app_fail");
const QString TAG_POLICY_UPLOAD_OK("upload_ok");
const QString TAG_POLICY_UPLOAD_FAIL("upload_fail");
const QString TAG_POLICY_FINALIZE_OK("finalize_ok");
const QString TAG_POLICY_FINALIZE_FAIL("finalize_fail");
const QString TAG_IDCLASH_OK("idclash_ok");
const QString TAG_IDCLASH_FAIL("idclash_fail");
const QString TAG_IDCLASH_DETAIL("idclash_detail");


// Keys used by server or client (S server, C client, B bidirectional)
// SEE ALSO patient.cpp, for the JSON ones.
const QString KEY_ADDRESS("address");  // C->S, in JSON, v2.3.0
const QString KEY_DOB("dob");  // C->S, in JSON, v2.3.0
const QString KEY_FORENAME("forename");  // C->S, in JSON, v2.3.0
const QString KEY_GP("gp");  // C->S, in JSON, v2.3.0
const QString KEY_IDNUM_PREFIX("idnum");  // C->S, in JSON, v2.3.0
const QString KEY_OTHER("other");  // C->S, in JSON, v2.3.0
const QString KEY_SEX("sex");  // C->S, in JSON, v2.3.0
const QString KEY_SURNAME("surname");  // C->S, in JSON, v2.3.0


// ============================================================================
// Creation
// ============================================================================

Patient::Patient(CamcopsApp& app, DatabaseManager& db, const int load_pk) :
    DatabaseObject(app, db, TABLENAME, dbconst::PK_FIELDNAME, true, false),
    m_page(nullptr),
    m_questionnaire(nullptr)
{
    // ------------------------------------------------------------------------
    // Define fields
    // ------------------------------------------------------------------------
    addField(FORENAME_FIELD, QVariant::String);
    addField(SURNAME_FIELD, QVariant::String);
    addField(SEX_FIELD, QVariant::String);
    addField(DOB_FIELD, QVariant::Date);
    addField(ADDRESS_FIELD, QVariant::String);
    addField(GP_FIELD, QVariant::String);
    addField(OTHER_DETAILS_FIELD, QVariant::String);

    // ------------------------------------------------------------------------
    // Load from database (or create/save), unless this is a specimen
    // ------------------------------------------------------------------------
    load(load_pk);  // MUST ALWAYS CALL from derived Task constructor.
}


// ============================================================================
// Ancillary management
// ============================================================================

void Patient::loadAllAncillary(const int pk)
{
    const OrderBy order_by{{PatientIdNum::FN_WHICH_IDNUM, true}};
    ancillaryfunc::loadAncillary<PatientIdNum, PatientIdNumPtr>(
                m_idnums, m_app, m_db,
                PatientIdNum::FK_PATIENT, order_by, pk);
}


QVector<DatabaseObjectPtr> Patient::getAncillarySpecimens() const
{
    return QVector<DatabaseObjectPtr>{
        PatientIdNumPtr(new PatientIdNum(m_app, m_db)),
    };
}


QVector<DatabaseObjectPtr> Patient::getAllAncillary() const
{
    QVector<DatabaseObjectPtr> ancillaries;
    for (const PatientIdNumPtr& idnum : m_idnums) {
        ancillaries.append(idnum);
    }
    return ancillaries;
}


// ============================================================================
// Information about patients
// ============================================================================

int Patient::id() const
{
    return pkvalueInt();
}


QString Patient::forename() const
{
    const QString forename = valueString(FORENAME_FIELD);
    return forename.isEmpty() ? "?" : forename;
}


QString Patient::surname() const
{
    const QString surname = valueString(SURNAME_FIELD);
    return surname.isEmpty() ? "?" : surname;
}


QString Patient::sex() const
{
    const QString sex = valueString(SEX_FIELD);
    return sex.isEmpty() ? "?" : sex;
}


bool Patient::isFemale() const
{
    return sex() == CommonOptions::SEX_FEMALE;
}


bool Patient::isMale() const
{
    return sex() == CommonOptions::SEX_MALE;
}


QDate Patient::dob() const
{
    return valueDate(DOB_FIELD);
}


QString Patient::dobText() const
{
    return datetime::textDate(value(DOB_FIELD));
}


int Patient::ageYears() const
{
    return datetime::ageYears(value(DOB_FIELD));
}


bool Patient::hasIdnum(int which_idnum) const
{
    return !idnumVariant(which_idnum).isNull();
}


QVector<int> Patient::whichIdnumsPresent() const
{
    QVector<int> which;
    for (const PatientIdNumPtr& idnum : m_idnums) {
        if (idnum->idnumIsPresent()) {
            which.append(idnum->whichIdNum());
        }
    }
    return which;
}


QVector<int> Patient::whichIdnumsHaveEntries() const
{
    QVector<int> which;
    for (const PatientIdNumPtr& idnum : m_idnums) {
        which.append(idnum->whichIdNum());
    }
    return which;
}


QVariant Patient::idnumVariant(const int which_idnum) const
{
    if (!dbconst::isValidWhichIdnum(which_idnum)) {
        return QVariant();
    }
    for (const PatientIdNumPtr& idnum : m_idnums) {
        if (idnum->whichIdNum() == which_idnum) {
            return idnum->idnumAsVariant();
        }
    }
    return QVariant();
}


qint64 Patient::idnumInteger(const int which_idnum) const
{
    return idnumVariant(which_idnum).toLongLong();  // 0 in case of failure
}


bool Patient::hasForename() const
{
    return !valueString(FORENAME_FIELD).isEmpty();
}


bool Patient::hasSurname() const
{
    return !valueString(SURNAME_FIELD).isEmpty();
}


bool Patient::hasSex() const
{
    return !valueString(SEX_FIELD).isEmpty();
}


bool Patient::hasDob() const
{
    return !value(DOB_FIELD).isNull();
}


bool Patient::hasAddress() const
{
    return !valueString(ADDRESS_FIELD).isEmpty();
}


bool Patient::hasGP() const
{
    return !valueString(GP_FIELD).isEmpty();
}


bool Patient::hasOtherDetails() const
{
    return !valueString(OTHER_DETAILS_FIELD).isEmpty();
}


Patient::AttributesType Patient::policyAttributes(
        const QVector<int>& policy_mentioned_idnums) const
{
    AttributesType map;
    map[FORENAME_FIELD] = hasForename();
    map[SURNAME_FIELD] = hasSurname();
    map[SEX_FIELD] = hasSex();
    map[DOB_FIELD] = hasDob();
    map[ADDRESS_FIELD] = hasAddress();
    map[GP_FIELD] = hasGP();
    map[OTHER_DETAILS_POLICYNAME] = hasOtherDetails();
    bool any_idnum = false;
    bool other_idnum = false;
    for (const PatientIdNumPtr& idnum : m_idnums) {
        const bool present = idnum->idnumIsPresent();
        const int which_idnum = idnum->whichIdNum();
        map[IDNUM_FIELD_FORMAT.arg(which_idnum)] = present;
        any_idnum = any_idnum || present;
        if (!policy_mentioned_idnums.contains(which_idnum)) {
            // "other"
            other_idnum = other_idnum || present;
        }
    }
    map[ANY_IDNUM_POLICYNAME] = any_idnum;
    map[OTHER_IDNUM_POLICYNAME] = other_idnum;
    return map;
}


QJsonObject Patient::jsonDescription() const
{
    QJsonObject j;

    auto assignStringOrNull = [this, &j](const QString& key,
                                         const QString& field) {
        const QString value = valueString(field);
        // You can assign QJsonValue::Null or a string (amongst other things),
        // but you have to use if/else; you can't use ?: (ternary operator) since
        // they are of different types.
        if (value.isEmpty()) {
            j[key] = QJsonValue::Null;
        } else {
            j[key] = value;
        }
    };

    assignStringOrNull(KEY_FORENAME, FORENAME_FIELD);
    assignStringOrNull(KEY_SURNAME, SURNAME_FIELD);
    assignStringOrNull(KEY_SEX, SEX_FIELD);

    const QVariant dob = value(DOB_FIELD);
    if (dob.isNull()) {
        j[KEY_DOB] = QJsonValue::Null;
    } else {
        j[KEY_DOB] = datetime::dateToIso(dob.toDate());
    }

    assignStringOrNull(KEY_ADDRESS, ADDRESS_FIELD);
    assignStringOrNull(KEY_GP, GP_FIELD);

    for (const PatientIdNumPtr& idnum : m_idnums) {
        if (!idnum->idnumIsPresent()) {
            continue;
        }
        const int which_idnum = idnum->whichIdNum();
        const qint64 idnum_value = idnum->idnumAsInteger();
        const QString idkey = QString("%1%2").arg(
                    KEY_IDNUM_PREFIX, QString::number(which_idnum));
        j[idkey] = idnum_value;
    }

#ifdef DEBUG_JSON
    qDebug().noquote() << "Patient jsonDescription():" << j;
#endif
    return j;
}


bool Patient::compliesWith(const IdPolicy& policy) const
{
    return policy.complies(policyAttributes(
                               policy.specificallyMentionedIdNums()));
}


bool Patient::compliesWithTablet() const
{
    return compliesWith(TABLET_ID_POLICY);
}


bool Patient::compliesWithUpload() const
{
    return compliesWith(m_app.uploadPolicy());
}


bool Patient::compliesWithFinalize() const
{
    return compliesWith(m_app.finalizePolicy());
}


QString Patient::shortIdnumSummary() const
{
    QStringList details;
    for (const PatientIdNumPtr& idnum : m_idnums) {
        details.append(QString("%1 %2")
                       .arg(m_app.idShortDescription(idnum->whichIdNum()),
                            idnum->idnumAsString()));
    }
    if (details.isEmpty()) {
        return tr("[No ID numbers]");
    }
    return details.join(", ");
}


void Patient::updateQuestionnaireIndicators(const FieldRef* fieldref,
                                            const QObject* originator)
{
    // qDebug() << Q_FUNC_INFO;
    Q_UNUSED(fieldref)
    Q_UNUSED(originator)
    if (!m_questionnaire) {
        return;
    }

    const bool tablet_ok = compliesWithTablet();
    m_questionnaire->setVisibleByTag(TAG_POLICY_APP_OK, tablet_ok);
    m_questionnaire->setVisibleByTag(TAG_POLICY_APP_FAIL, !tablet_ok);
    fieldRef(SURNAME_FIELD)->setMandatory(!tablet_ok);
    fieldRef(FORENAME_FIELD)->setMandatory(!tablet_ok);
    fieldRef(SEX_FIELD)->setMandatory(!tablet_ok);
    fieldRef(DOB_FIELD)->setMandatory(!tablet_ok);
    for (const PatientIdNumPtr& idnum : m_idnums) {
        // idnum->fieldRef(PatientIdNum::FN_IDNUM_VALUE)->setMandatory(!tablet_ok);
        idnum->fieldRef(PatientIdNum::FN_IDNUM_VALUE)->setMandatory(true);
        // ... mandatory was "!tablet_ok", but that makes it easier to create blank
        // ID number entries, which help nobody; changed 2019-03-23.
    }

    const bool upload_ok = compliesWithUpload();
    m_questionnaire->setVisibleByTag(TAG_POLICY_UPLOAD_OK, upload_ok);
    m_questionnaire->setVisibleByTag(TAG_POLICY_UPLOAD_FAIL, !upload_ok);

    const bool finalize_ok = compliesWithFinalize();
    m_questionnaire->setVisibleByTag(TAG_POLICY_FINALIZE_OK, finalize_ok);
    m_questionnaire->setVisibleByTag(TAG_POLICY_FINALIZE_FAIL, !finalize_ok);

    bool id_ok = true;
    QStringList clashing_ids;
    const QVector<int> which_idnums_present = whichIdnumsPresent();
    for (const int n : which_idnums_present) {
        if (othersClashOnIdnum(n)) {
            clashing_ids.append(m_app.idShortDescription(n));
            id_ok = false;
        }
    }
    const QString idclash_text = id_ok
            ? "No clashes"
            : ("The following IDs clash: " + clashing_ids.join(", "));
    m_questionnaire->setVisibleByTag(TAG_IDCLASH_OK, id_ok);
    m_questionnaire->setVisibleByTag(TAG_IDCLASH_FAIL, !id_ok);
    QuElement* element = m_questionnaire->getFirstElementByTag(
                TAG_IDCLASH_DETAIL, false);
    auto textelement = dynamic_cast<QuText*>(element);
    if (textelement) {
        textelement->setText(idclash_text);
    }
}


bool Patient::othersClashOnIdnum(const int which_idnum) const
{
    // Answers the question: do any other patients share the ID number whose
    // *index* (e.g. 1-8) is which_idnum?
    using dbfunc::delimit;
    if (!dbconst::isValidWhichIdnum(which_idnum)) {
        uifunc::stopApp("Bug: Bad which_idnum to Patient::othersClashOnIdnum");
    }
    const QVariant idvar = idnumVariant(which_idnum);
    if (idvar.isNull()) {
        return false;
    }
    const qint64 idnum = idnumInteger(which_idnum);
    const int patient_pk = id();
    const SqlArgs sqlargs(
        QString("SELECT COUNT(*) FROM %1 WHERE %2 = ? AND %3 = ? AND %4 <> ?")
            .arg(delimit(PatientIdNum::PATIENT_IDNUM_TABLENAME),
                 delimit(PatientIdNum::FN_WHICH_IDNUM),
                 delimit(PatientIdNum::FN_IDNUM_VALUE),
                 delimit(PatientIdNum::FK_PATIENT)),
        ArgList{which_idnum, idnum, patient_pk}
    );
    const int c = m_db.fetchInt(sqlargs);
    return c > 0;
}


bool Patient::anyIdClash() const
{
    // With a single SQL statement, answers the question: "Are there any other
    // patients (that is, patients with a different PK) that share any ID
    // numbers with this patient)?"
    using dbfunc::delimit;
    ArgList args;
    const QString sql = QString(
                "SELECT COUNT(*) "
                "FROM %1 otherpt "
                "INNER JOIN %1 thispt "
                "  ON otherpt.%2 = thispt.%2 "  // which_idnum
                "  AND otherpt.%3 = thispt.%3 "  // idnum value; will automatically ignore NULLs
                "  AND otherpt.%4 <> thispt.%4 "  // patient PK
                "WHERE thispt.%4 = ?")  // patient PK
            .arg(delimit(PatientIdNum::PATIENT_IDNUM_TABLENAME),  // %1
                 delimit(PatientIdNum::FN_WHICH_IDNUM),  // %2
                 delimit(PatientIdNum::FN_IDNUM_VALUE),  // %3
                 delimit(PatientIdNum::FK_PATIENT));  // %4
    args.append(id());
    const SqlArgs sqlargs(sql, args);
    const int c = m_db.fetchInt(sqlargs);
    return c > 0;
}


int Patient::numTasks() const
{
    int n = 0;
    const int patient_id = id();
    if (patient_id == dbconst::NONEXISTENT_PK) {
        return 0;
    }
    TaskFactory* factory = m_app.taskFactory();
    for (const TaskPtr& p_specimen : factory->allSpecimensExceptAnonymous()) {
        n += p_specimen->countForPatient(patient_id);  // copes with anonymous
    }
    return n;
}


void Patient::deleteFromDatabase()
{
    // Delete any associated tasks
    const int patient_id = id();
    if (patient_id == dbconst::NONEXISTENT_PK) {
        return;
    }
    DbNestableTransaction trans(m_db);
    TaskFactory* factory = m_app.taskFactory();
    for (const TaskPtr& p_task : factory->fetchAllTasksForPatient(patient_id)) {
        p_task->deleteFromDatabase();
    }
    // Delete ourself
    DatabaseObject::deleteFromDatabase();
}


bool Patient::matchesForMerge(const Patient* other) const
{
    Q_ASSERT(other);
    auto sameOrOneNull = [this, &other](const QString& fieldname) -> bool {
        QVariant a = value(fieldname);
        QVariant b = other->value(fieldname);
        return a.isNull() || b.isNull() || a == b;
    };
    auto sameOrOneBlank = [this, &other](const QString& fieldname) -> bool {
        QString a = valueString(fieldname);
        QString b = other->valueString(fieldname);
        return a.isEmpty() || b.isEmpty() || a == b;
    };

    if (id() == other->id()) {
        qWarning() << Q_FUNC_INFO << "Asked to compare two patients with the "
                                     "same PK for merge! Bug.";
        return false;
    }
    // All ID numbers must match or be blank:
    for (const PatientIdNumPtr& this_id : m_idnums) {
        const int which_idnum = this_id->whichIdNum();
        if (this_id->idnumIsPresent() &&
                other->hasIdnum(which_idnum) &&
                other->idnumInteger(which_idnum) != idnumInteger(which_idnum)) {
            return false;
        }
    }
    // Forename, surname, DOB, sex must match or be blank:
    return sameOrOneBlank(FORENAME_FIELD) &&
            sameOrOneBlank(SURNAME_FIELD) &&
            sameOrOneNull(DOB_FIELD) &&
            sameOrOneBlank(SEX_FIELD) &&
            sameOrOneBlank(ADDRESS_FIELD) &&
            sameOrOneBlank(GP_FIELD) &&
            sameOrOneBlank(OTHER_DETAILS_FIELD);
}


QString Patient::descriptionForMerge() const
{
    return QString("<b>%1</b><br>%2<br>%3").arg(surnameUpperForename(),
                                                sexAgeDob(),
                                                shortIdnumSummary());
}


QString Patient::forenameSurname() const
{
    return QString("%1 %2").arg(forename(), surname());
}


QString Patient::surnameUpperForename() const
{
    return QString("%1, %2").arg(surname().toUpper(), forename());
}


QString Patient::sexAgeDob() const
{
    return QString("%1, %2y, DOB %3").arg(sex(),
                                          QString::number(ageYears()),
                                          dobText());
}


QString Patient::ageSexDob() const
{
    // "A 37-year-old woman..."
    return QString("%1y, %2, DOB %3").arg(QString::number(ageYears()),
                                          sex(),
                                          dobText());
}


QString Patient::twoLineDetailString() const
{
    return QString("%1 (%2)\n%3").arg(surnameUpperForename(),
                                      sexAgeDob(),
                                      shortIdnumSummary());
}


QString Patient::oneLineHtmlDetailString() const
{
    return QString("<b>%1</b> (%2); %3").arg(surnameUpperForename(),
                                             sexAgeDob(),
                                             shortIdnumSummary());
}


// ============================================================================
// Editing and other manipulations
// ============================================================================

OpenableWidget* Patient::editor(const bool read_only)
{
    buildPage(read_only);
    m_questionnaire = new Questionnaire(m_app, {m_page});
    m_questionnaire->setReadOnly(read_only);
    updateQuestionnaireIndicators();
    return m_questionnaire;
}


void Patient::buildPage(bool read_only)
{
    auto addIcon = [this](const QString& name, const QString& tag) {
        auto image = new QuImage(uifunc::iconFilename(name),
                                 uiconst::g_iconsize);
        image->setAdjustForDpi(false);  // uiconst::ICONSIZE already corrects for this
        image->addTag(tag);
        m_page->addElement(image);
    };

    if (!m_page) {
        m_page = QuPagePtr(new QuPage());
    } else {
        m_page->clearElements();
    }
    m_page->setTitle(read_only ? tr("View patient") : tr("Edit patient"));

    auto grid = new QuGridContainer();
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 2);
    int row = 0;
    const Qt::Alignment ralign = Qt::AlignRight | Qt::AlignTop;
    const Qt::Alignment lalign = Qt::AlignLeft | Qt::AlignTop;
    // Don't apply alignment to the editing widgets; let them fill their cells.
    const int rowspan = 1;
    const int colspan = 1;

    // ------------------------------------------------------------------------
    // Basic demographics
    // ------------------------------------------------------------------------
    grid->addCell(QuGridCell(new QuText(tr("Surname")),
                             row, 0, rowspan, colspan, ralign));
    grid->addCell(QuGridCell(new QuLineEdit(fieldRef(SURNAME_FIELD, false)),
                             row++, 1));
    grid->addCell(QuGridCell(new QuText(tr("Forename")),
                             row, 0, rowspan, colspan, ralign));
    grid->addCell(QuGridCell(new QuLineEdit(fieldRef(FORENAME_FIELD, false)),
                             row++, 1));
    grid->addCell(QuGridCell(new QuText(tr("Sex")),
                             row, 0, rowspan, colspan, ralign));
    grid->addCell(QuGridCell(
        (new QuMcq(fieldRef(SEX_FIELD),  // properly mandatory
                   CommonOptions::sexes()))->setHorizontal(true),
        row++, 1));
    grid->addCell(QuGridCell(new QuText(tr("Date of birth")),
                             row, 0, rowspan, colspan, ralign));
    grid->addCell(QuGridCell(
        (new QuDateTime(fieldRef(DOB_FIELD, false)))
                      ->setMode(QuDateTime::DefaultDate)
                      ->setOfferNullButton(true),  // in case policy disallows DOB
        row++, 1));
    grid->addCell(QuGridCell(new QuText(tr("Address")),
                             row, 0, rowspan, colspan, ralign));
    grid->addCell(QuGridCell(new QuTextEdit(fieldRef(ADDRESS_FIELD, false)),
                             row++, 1));
    grid->addCell(QuGridCell(new QuText(tr("General practitioner (GP)")),
                             row, 0, rowspan, colspan, ralign));
    grid->addCell(QuGridCell(new QuTextEdit(fieldRef(GP_FIELD, false)),
                             row++, 1));
    grid->addCell(QuGridCell(new QuText(tr("Other details")),
                             row, 0, rowspan, colspan, ralign));
    grid->addCell(QuGridCell(new QuTextEdit(fieldRef(OTHER_DETAILS_FIELD, false)),
                             row++, 1));

    // ------------------------------------------------------------------------
    // Patient ID numbers
    // ------------------------------------------------------------------------
    m_page->addElement(grid);
    auto idgrid = new QuGridContainer();
    idgrid->setColumnStretch(0, 1);
    idgrid->setColumnStretch(1, 1);
    idgrid->setColumnStretch(2, 4);
    row = 0;
    for (const PatientIdNumPtr& idnum : m_idnums) {
        const int which_idnum = idnum->whichIdNum();
        IdNumDescriptionConstPtr idinfo = m_app.getIdInfo(which_idnum);
        auto delete_id = new QuButton(
                QString("%1 %2 (%3)").arg(
                    tr("Delete ID#"),
                    QString::number(which_idnum),
                    m_app.idDescription(which_idnum)
                ),
                std::bind(&Patient::deleteIdNum, this, which_idnum));
        idgrid->addCell(QuGridCell(delete_id,
                                   row, 0, rowspan, colspan, lalign));
        auto id_label = new QuText(m_app.idDescription(which_idnum));
        idgrid->addCell(QuGridCell(id_label,
                                   row, 1, rowspan, colspan, ralign));

        auto id_fr = idnum->fieldRef(PatientIdNum::FN_IDNUM_VALUE, true);
        // ... mandatory was false, but that makes it easier to create blank
        // ID number entries, which help nobody; changed 2019-03-23.
        QuLineEditInt64* num_editor;
        if (idinfo->validateAsNhsNumber()) {
            num_editor = new QuLineEditNHSNumber(id_fr);
        } else {
            num_editor = new QuLineEditInt64(
                        id_fr, MIN_ID_NUM_VALUE, MAX_ID_NUM_VALUE);
        }
        idgrid->addCell(QuGridCell(num_editor, row++, 2));
    }
    auto add_id = new QuButton(
                tr("Add ID number"),
                std::bind(&Patient::addIdNum, this));
    idgrid->addCell(QuGridCell(add_id,
                               row++, 0, rowspan, colspan, lalign));
    m_page->addElement(idgrid);

    // ------------------------------------------------------------------------
    // ID policy indicators
    // ------------------------------------------------------------------------
    m_page->addElement(new QuHeading(tr("Minimum ID required for app:")));
    m_page->addElement(new QuText(TABLET_ID_POLICY.pretty()));
    addIcon(uiconst::CBS_OK, TAG_POLICY_APP_OK);
    addIcon(uiconst::ICON_STOP, TAG_POLICY_APP_FAIL);

    m_page->addElement(new QuHeading(tr("Minimum ID required for upload to server:")));
    m_page->addElement(new QuText(m_app.uploadPolicy().pretty()));
    addIcon(uiconst::CBS_OK, TAG_POLICY_UPLOAD_OK);
    addIcon(uiconst::ICON_STOP, TAG_POLICY_UPLOAD_FAIL);

    m_page->addElement(new QuHeading(tr("Minimum ID required to finalize on server:")));
    m_page->addElement(new QuText(m_app.finalizePolicy().pretty()));
    addIcon(uiconst::CBS_OK, TAG_POLICY_FINALIZE_OK);
    addIcon(uiconst::ICON_STOP, TAG_POLICY_FINALIZE_FAIL);

    m_page->addElement(new QuHeading(tr("ID numbers must not clash with another patient:")));
    m_page->addElement((new QuText("?"))->addTag(TAG_IDCLASH_DETAIL));
    addIcon(uiconst::CBS_OK, TAG_IDCLASH_OK);
    addIcon(uiconst::ICON_STOP, TAG_IDCLASH_FAIL);

    // ------------------------------------------------------------------------
    // Signals
    // ------------------------------------------------------------------------
    QStringList fields{FORENAME_FIELD, SURNAME_FIELD, DOB_FIELD, SEX_FIELD,
                       ADDRESS_FIELD, GP_FIELD, OTHER_DETAILS_FIELD};
    for (const QString& fieldname : fields) {
        FieldRefPtr fr = fieldRef(fieldname);
        connect(fr.data(), &FieldRef::valueChanged,
                this, &Patient::updateQuestionnaireIndicators);
    }
    for (const PatientIdNumPtr& idnum : m_idnums) {
        FieldRefPtr fr = idnum->fieldRef(PatientIdNum::FN_IDNUM_VALUE);
        connect(fr.data(), &FieldRef::valueChanged,
                this, &Patient::updateQuestionnaireIndicators);
    }
}


void Patient::mergeInDetailsAndTakeTasksFrom(const Patient* other)
{
    DbNestableTransaction trans(m_db);

    const int this_pk = id();
    const int other_pk = other->id();

    // Copy information from other to this
    qInfo() << Q_FUNC_INFO << "Copying information from patient" << other_pk
            << "to patient" << this_pk;
    QStringList fields{
        FORENAME_FIELD,
        SURNAME_FIELD,
        DOB_FIELD,
        SEX_FIELD,
        ADDRESS_FIELD,
        GP_FIELD,
        OTHER_DETAILS_FIELD,
    };

    // ------------------------------------------------------------------------
    // ID numbers
    // ------------------------------------------------------------------------
    bool please_sort = false;
    for (const PatientIdNumPtr& other_id : other->m_idnums) {
        if (other_id->idnumIsPresent()) {
            const int which_idnum = other_id->whichIdNum();
            const qint64 other_idnum_value = other_id->idnumAsInteger();
            bool found = false;
            for (const PatientIdNumPtr& this_id : m_idnums) {
                if (this_id->whichIdNum() == which_idnum) {
                    found = true;
                    if (this_id->idnumIsPresent() &&
                            this_id->idnumAsInteger() != other_idnum_value) {
                        qWarning().nospace()
                                << Q_FUNC_INFO
                                << " Something has gone wrong! ID number mismatch for ID#"
                                << which_idnum
                                << " (this: " << this_id->idnumAsVariant()
                                << ", other: " << other_id->idnumAsVariant()
                                << ") yet we shouldn't be copying if there is mismatch!";
                    }
                    this_id->setIdnumValue(other_id->idnumAsInteger(), true);  // save
                }
            }
            if (!found) {
                PatientIdNumPtr new_id(new PatientIdNum(this_pk, which_idnum,
                                                        m_app, m_db));
                m_idnums.append(new_id);
                please_sort = true;
            }
        }
    }
    if (please_sort) {
        sortIdNums();
    }

    // ------------------------------------------------------------------------
    // Other patient details
    // ------------------------------------------------------------------------
    for (const QString& fieldname : fields) {
        QVariant this_value = value(fieldname);
        QVariant other_value = other->value(fieldname);
        if (this_value.isNull() || (this_value.toString().isEmpty() &&
                                    !other_value.toString().isEmpty())) {
            setValue(fieldname, other_value);
        }
    }
    save();

    // ------------------------------------------------------------------------
    // Move tasks from other to this
    // ------------------------------------------------------------------------
    qInfo() << Q_FUNC_INFO << "Moving tasks from patient" << other_pk
            << "to patient" << this_pk;
    TaskFactory* factory = m_app.taskFactory();
    for (const TaskPtr& p_task : factory->fetchAllTasksForPatient(other_pk)) {
        p_task->moveToPatient(this_pk);
        p_task->save();
    }

    qInfo() << Q_FUNC_INFO << "Move complete";
}


void Patient::addIdNum()
{
    const QVector<int> present = whichIdnumsHaveEntries();
    const QVector<int> possible = m_app.whichIdNumsAvailable();
    const QVector<int> unused = containers::setSubtract(possible, present);
    if (unused.isEmpty()) {
        QString msg = tr("All ID numbers offered by the server are already here!");
        if (present.isEmpty()) {
            msg += tr(" (There are no ID numbers at all – have you "
                      "registered with a server?)");
        }
        uifunc::alert(msg);
        return;
    }
    NameValueOptions options;
    for (const int which_idnum : unused) {
        const QString description = QString("<b>%1</b> <i>[%2 %3]</i>").arg(
            m_app.idDescription(which_idnum),
            TextConst::idNumberType(),
            QString::number(which_idnum)
        );
        const NameValuePair pair(description, which_idnum);
        options.append(pair);
    }
    NvpChoiceDialog dlg(m_questionnaire, options, tr("Add ID number"));
    QVariant chosen_idnum_var;
    if (dlg.choose(&chosen_idnum_var) != QDialog::Accepted) {
        return;  // user pressed cancel, or some such
    }
    const int chosen_idnum = chosen_idnum_var.toInt();
    PatientIdNumPtr new_id = PatientIdNumPtr(new PatientIdNum(
                            id(), chosen_idnum, m_app, m_db));  // will save
    m_idnums.append(new_id);
    sortIdNums();
}


void Patient::deleteIdNum(const int which_idnum)
{
    const QString strnum = QString::number(which_idnum);
    const QString text(tr("Really delete ID number") + " " + strnum + " (" +
                       m_app.idDescription(which_idnum) + ")?");
    const QString title(tr("Delete ID#") + strnum + "?");
    if (!uifunc::confirm(text, title,
                         tr("Yes, delete it"), tr("No, cancel"),
                         m_questionnaire)) {
        return;
    }
    for (int i = 0; i < m_idnums.size(); ++i) {
        PatientIdNumPtr idnum = m_idnums.at(i);
        if (idnum->whichIdNum() == which_idnum) {
            idnum->deleteFromDatabase();
            m_idnums.remove(i);
            sortIdNums();
            return;
        }
    }
}


void Patient::sortIdNums()
{
    std::sort(m_idnums.begin(), m_idnums.end(), PatientIdNumSorter());
    if (m_questionnaire) {
        buildPage(m_questionnaire->readOnly());
        updateQuestionnaireIndicators();
        m_questionnaire->refreshCurrentPage();
    }
}
