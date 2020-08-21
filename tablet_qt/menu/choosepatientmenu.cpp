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

// #define DEBUG_VERBOSE

#include "choosepatientmenu.h"
#include <QDebug>
#include <QPushButton>
#include "dbobjects/patient.h"
#include "dbobjects/patientsorter.h"
#include "dialogs/nvpchoicedialog.h"
#include "dialogs/scrollmessagebox.h"
#include "lib/stringfunc.h"
#include "lib/uifunc.h"
#include "menulib/menuheader.h"


ChoosePatientMenu::ChoosePatientMenu(CamcopsApp& app) :
    MenuWindow(app, uifunc::iconFilename(uiconst::ICON_CHOOSE_PATIENT))
{
}


void ChoosePatientMenu::extraLayoutCreation()
{
    connect(&m_app, &CamcopsApp::selectedPatientDetailsChanged,
            this, &ChoosePatientMenu::refreshPatientList,
            Qt::UniqueConnection);
    connect(&m_app, &CamcopsApp::refreshPatientList,
            this, &ChoosePatientMenu::refreshPatientList,
            Qt::UniqueConnection);

    // Set other header buttons
    m_p_header->offerAdd(true);

    connect(m_p_header, &MenuHeader::addClicked,
            this, &ChoosePatientMenu::addPatient,
            Qt::UniqueConnection);
}


QString ChoosePatientMenu::title() const
{
    return tr("Choose patient");
}


void ChoosePatientMenu::makeItems()
{
    // Patient
    PatientPtrList patients = m_app.getAllPatients();
    m_items = {
        MenuItem(tr("Special functions")).setLabelOnly(),
        MenuItem(
            txtMergeTitle(),
            std::bind(&ChoosePatientMenu::mergePatients, this),
            "",  // icon
            tr("Choose one patient, then select this option to merge with another")  // subtitle
        ).setNotIfLocked(),
        MenuItem(tr("Patients")).setLabelOnly(),
    };
    // qDebug() << Q_FUNC_INFO << "-" << patients.size() << "patient(s)";
    for (const PatientPtr& patient : patients) {
        m_items.append(MenuItem(patient));
    }
}


void ChoosePatientMenu::viewItem()
{
    editPatient(true);
}


void ChoosePatientMenu::editItem()
{
    editPatient(false);
}


void ChoosePatientMenu::deleteItem()
{
    deletePatient();
}


void ChoosePatientMenu::addPatient()
{
#ifdef DEBUG_VERBOSE
    qDebug() << Q_FUNC_INFO;
#endif
    // The patient we create here needs to stay in scope for the duration of
    // editing! The simplest way is to use a member object to hold the pointer.

    PatientPtr patient = PatientPtr(new Patient(m_app, m_app.db()));
    patient->save();
    // v2.2.0 fix:
    // MUST call m_app.setSelectedPatient before
    // CamcopsApp::open(..., patient), because when the editor closes,
    // CamcopsApp::close() will be called, and that will call
    // selectedPatientDetailsChanged() on the patient in question; this gives
    // the *impression* of the selected patient changing (e.g. name display
    // changes) but the underlying selection won't have, which is Bad; a
    // previously selected patient's tasks continue to show up but with the
    // newly-created patient's name.
    m_app.setSelectedPatient(patient->id());
    OpenableWidget* widget = patient->editor(false);
    m_app.openSubWindow(widget, TaskPtr(nullptr), false, patient);
}


void ChoosePatientMenu::editPatient(const bool read_only)
{
#ifdef DEBUG_VERBOSE
    qDebug() << Q_FUNC_INFO;
#endif
    PatientPtr patient = currentPatient();
    if (!patient) {
        uifunc::alert("Bug: null patient pointer in ChoosePatientMenu::editPatient");
        return;
    }
    OpenableWidget* widget = patient->editor(read_only);
    m_app.openSubWindow(widget, TaskPtr(nullptr), false, patient);
}


void ChoosePatientMenu::deletePatient()
{
    qDebug() << Q_FUNC_INFO;
    PatientPtr patient = currentPatient();
    if (!patient) {
        uifunc::alert("Bug: null patient pointer in ChoosePatientMenu::editPatient");
        return;
    }
    QString patient_details = patient->twoLineDetailString();

    // First check
    {
        ScrollMessageBox msgbox(
                    QMessageBox::Warning,
                    tr("Delete patient"),
                    tr("Delete this patient?") + "\n\n" +  patient_details,
                    this);
        QAbstractButton* delete_button = msgbox.addButton(
                    tr("Yes, delete"), QMessageBox::YesRole);
        msgbox.addButton(tr("No, cancel"), QMessageBox::NoRole);
        msgbox.exec();
        if (msgbox.clickedButton() != delete_button) {
            return;
        }
    }

    // Second check
    int n_tasks = patient->numTasks();
    if (n_tasks > 0) {
        ScrollMessageBox msgbox(
                    QMessageBox::Warning,
                    tr("Delete patient WITH TASKS"),
                    tr("Delete this patient?") + "\n\n" + patient_details +
                        "\n\n" + tr("THERE ARE %1 ASSOCIATED TASKS!").arg(n_tasks),
                    this);
        // NB can't use HTML "<b></b>" in the text there.
        QAbstractButton* delete_button = msgbox.addButton(
                    tr("Yes, delete despite tasks"), QMessageBox::YesRole);
        msgbox.addButton(tr("No, cancel"), QMessageBox::NoRole);
        msgbox.exec();
        if (msgbox.clickedButton() != delete_button) {
            return;
        }
    }

    // Delete
    qInfo() << "Deleting patient:" << patient_details;
    patient->deleteFromDatabase();
    qInfo() << "... patient deleted";
    m_app.deselectPatient();
    refreshPatientList();
}


void ChoosePatientMenu::refreshPatientList()
{
    rebuild(false);  // no need to rebuild header
}


void ChoosePatientMenu::mergePatients()
{
    auto reportFail = [this](const QString& text) -> void {
        ScrollMessageBox::warning(this, txtMergeTitle(), text);
    };

    // Is one selected?
    if (!m_app.isPatientSelected()) {
        reportFail(tr("Select a patient first, then choose this option to "
                      "merge with another."));
        return;
    }

    // Get all others
    const PatientPtrList all_patients = m_app.getAllPatients();
    PatientPtrList other_patients;
    Patient* selected_patient = m_app.selectedPatient();
    for (const PatientPtr& other : all_patients) {
        if (other->id() != selected_patient->id() &&
                other->matchesForMerge(selected_patient)) {
            other_patients.append(other);
        }
    }
    if (other_patients.isEmpty()) {
        reportFail(tr(
                "No other patients available that match the selected "
                "patient. (Information can be present in one patient and "
                "missing from the other, but where information is present, "
                "it must match.)"));
        return;
    }

    // Offer the user a choice of the others
    std::sort(other_patients.begin(), other_patients.end(), PatientSorter());
    NameValueOptions options;
    for (const PatientPtr& other : other_patients) {
        options.append(NameValuePair(other->descriptionForMerge(),
                                     other->pkvalue()));
    }
    NvpChoiceDialog dlg(this, options, tr("Choose other patient"));
    QVariant chosen_other_pk;
    if (dlg.choose(&chosen_other_pk) != QDialog::Accepted) {
        return;  // user pressed cancel, or some such
    }
    PatientPtr chosen_other = nullptr;
    for (PatientPtr& other : other_patients) {
        if (other->pkvalue() == chosen_other_pk) {
            chosen_other = other;
            break;
        }
    }
    Q_ASSERT(chosen_other);

    // Confirm
    QStringList confirm_lines{
        stringfunc::bold(tr("Please confirm:")),
        stringfunc::bold(tr("MERGE:")),
        selected_patient->descriptionForMerge(),
        stringfunc::bold(tr("WITH:")),
        chosen_other->descriptionForMerge(),
        stringfunc::bold("?"),
    };
    QString confirm_text = confirm_lines.join("<br><br>");
    const QString yes = tr("Yes, merge");
    const QString no = tr("No, cancel");
    if (!uifunc::confirm(confirm_text, txtMergeTitle(), yes, no, this)) {
        return;
    }
    confirm_lines.prepend(tr("ARE YOU SURE?"));
    confirm_text = confirm_lines.join("<br><br>");
    if (!uifunc::confirm(confirm_text, txtMergeTitle(), yes, no, this)) {
        return;
    }

    // Perform the merge
    qInfo() << Q_FUNC_INFO << "Copying patient information and moving tasks...";
    selected_patient->mergeInDetailsAndTakeTasksFrom(chosen_other.data());
    qInfo() << Q_FUNC_INFO << "Deleting other patient...";
    chosen_other->deleteFromDatabase();
    qInfo() << Q_FUNC_INFO << "Merge complete.";

    // Refresh list, etc.
    m_app.deselectPatient();
    refreshPatientList();
}


QString ChoosePatientMenu::txtMergeTitle()
{
    return tr("Merge patients");
}
