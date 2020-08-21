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
#include <QPointer>
#include "menulib/menuwindow.h"
#include "crypto/secureqstring.h"
class Questionnaire;

// #define SETTINGSMENU_OFFER_SPECIFIC_FETCHES


class SettingsMenu : public MenuWindow
{
    Q_OBJECT
public:
    SettingsMenu(CamcopsApp& app);
    virtual QString title() const override;
protected:
    virtual void makeItems() override;
    OpenableWidget* configureServer(CamcopsApp& app);
    void serverSettingsSaved();
    bool validateServerSettings(QStringList& errors, const QuPage* page);

    OpenableWidget* configureIntellectualProperty(CamcopsApp& app);
    void ipClinicalChanged();
    void ipSaved();
    void ipCancelled();

    OpenableWidget* configureUser(CamcopsApp& app);
    void userSettingsSaved();
    void userSettingsCancelled();

    OpenableWidget* setQuestionnaireFontSize(CamcopsApp& app);
    void fontSizeChanged();
    void fontSettingsSaved();
    void fontSettingsCancelled();
    void resetFontSize();
    QString demoText(const QString& text, uiconst::FontSize fontsize_type) const;
    void dpiOverrideChanged();

    void setPrivilege();
    void changeAppPassword();
    void changePrivPassword();
    void deleteAllExtraStrings();
    void registerWithServer();
    void fetchAllServerInfo();
#ifdef SETTINGSMENU_OFFER_SPECIFIC_FETCHES
    void fetchIdDescriptions();
    void fetchExtraStrings();
#endif
    OpenableWidget* viewServerInformation(CamcopsApp& app);
    void viewDataCounts();
    void viewSystemCounts();
    void dropUnknownTables();
    void viewDataDbAsSql();
    void viewSystemDbAsSql();
    void debugDataDbAsSql();
    void debugSystemDbAsSql();
    void saveDataDbAsSql();
    void saveSystemDbAsSql();
    void chooseLanguage();

    // Internal helpers:
    QString makeTitle(const QString& part1, const QString& part2 = "",
                      bool colon = false) const;
    QString makeHint(const QString& part1, const QString& part2) const;
    QVariant serverPasswordGetter();
    bool serverPasswordSetter(const QVariant& value);
    void viewDbAsSql(DatabaseManager& db, const QString& title);
    void debugDbAsSql(DatabaseManager& db, const QString& prefix);
    void saveDbAsSql(DatabaseManager& db, const QString& save_title,
                     const QString& finish_prefix);
    void viewCounts(DatabaseManager& db, const QString& title);
protected:
    mutable SecureQString m_temp_plaintext_password;
    bool m_plaintext_pw_live;
    QPointer<Questionnaire> m_fontsize_questionnaire;
    QPointer<Questionnaire> m_ip_questionnaire;
    FieldRefPtr m_fontsize_fr;
    FieldRefPtr m_ip_clinical_fr;
    FieldRefPtr m_dpi_override_logical_fr;
    FieldRefPtr m_dpi_override_logical_x_fr;
    FieldRefPtr m_dpi_override_logical_y_fr;
    FieldRefPtr m_dpi_override_physical_fr;
    FieldRefPtr m_dpi_override_physical_x_fr;
    FieldRefPtr m_dpi_override_physical_y_fr;
};
