#!/usr/bin/env python

"""
camcops_server/tasks/deakin_s1_healthreview.py

===============================================================================

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
    along with CamCOPS. If not, see <https://www.gnu.org/licenses/>.

===============================================================================

"""

from sqlalchemy.sql.schema import Column
from sqlalchemy.sql.sqltypes import Integer, String, Text, UnicodeText

from camcops_server.cc_modules.cc_constants import CssClass
from camcops_server.cc_modules.cc_html import tr_qa
from camcops_server.cc_modules.cc_request import CamcopsRequest
from camcops_server.cc_modules.cc_sqla_coltypes import (
    BoolColumn,
    CamcopsColumn,
    MIN_ZERO_CHECKER,
    PermittedValueChecker,
    ZERO_TO_FOUR_CHECKER,
)
from camcops_server.cc_modules.cc_task import Task, TaskHasPatientMixin


# =============================================================================
# DeakinS1HealthReview
# =============================================================================

FREQUENCY_COMMENT = (
    "Frequency (0 did not use, 1 occasionally, 2 monthly, 3 weekly, 4 daily)"
)


class DeakinS1HealthReview(TaskHasPatientMixin, Task):
    """
    Server implementation of the DeakinS1HealthReview task.
    """
    __tablename__ = "deakin_1_healthreview"  # historically fixed
    shortname = "Deakin_S1_HealthReview"

    ethnicity = CamcopsColumn(
        "ethnicity", Integer,
        permitted_value_checker=PermittedValueChecker(minimum=1, maximum=16),
        comment="Ethnicity code, per GMC Patient Questionnaire (1-16)"
    )
    ethnicity_text = CamcopsColumn(
        "ethnicity_text", UnicodeText,
        exempt_from_anonymisation=True,
        comment="Ethnicity, description"
    )  # Seems to be unused by the client!
    ethnicity_other_details = Column(
        "ethnicity_other_details", UnicodeText,
        comment="Ethnicity, other, details"
    )

    handedness = CamcopsColumn(
        "handedness", String(length=1),  # was Text
        permitted_value_checker=PermittedValueChecker(permitted_values=["L", "R"]),  # noqa
        comment="Handedness (L, R)"
    )
    education = CamcopsColumn(
        "education", Text,
        exempt_from_anonymisation=True
    )

    allergies = BoolColumn("allergies")
    allergy_asthma = BoolColumn("allergy_asthma")
    allergy_pollen_dust = BoolColumn("allergy_pollen_dust")
    allergy_dermatitis = BoolColumn("allergy_dermatitis")
    allergy_food = BoolColumn("allergy_food")
    allergy_dander = BoolColumn("allergy_dander")
    allergy_other = BoolColumn("allergy_other")
    allergy_details = Column("allergy_details", Text)

    vaccinations_last3months = BoolColumn("vaccinations_last3months")
    vaccination_details = Column("vaccination_details", Text)

    infections_last3months = BoolColumn("infections_last3months")
    infection_recent_respiratory = BoolColumn("infection_recent_respiratory")
    infection_recent_gastroenteritis = BoolColumn(
        "infection_recent_gastroenteritis",
        constraint_name="ck_deakin_1_healthreview_inf_recent_gastro"
    )
    infection_recent_urinary = BoolColumn("infection_recent_urinary")
    infection_recent_sexual = BoolColumn("infection_recent_sexual")
    infection_recent_hepatitis = BoolColumn("infection_recent_hepatitis")
    infection_recent_other = BoolColumn("infection_recent_other")
    infection_recent_details = Column("infection_recent_details", Text)

    infections_chronic = BoolColumn("infections_chronic")
    infection_chronic_respiratory = BoolColumn("infection_chronic_respiratory")
    infection_chronic_gastroenteritis = BoolColumn(
        "infection_chronic_gastroenteritis",
        constraint_name="ck_deakin_1_healthreview_inf_chronic_gastro"
    )
    infection_chronic_urinary = BoolColumn("infection_chronic_urinary")
    infection_chronic_sexual = BoolColumn("infection_chronic_sexual")
    infection_chronic_hepatitis = BoolColumn("infection_chronic_hepatitis")
    infection_chronic_other = BoolColumn("infection_chronic_other")
    infection_chronic_details = Column("infection_chronic_details", Text)

    immune_disorders = BoolColumn("immune_disorders")
    immunity_ms = BoolColumn("immunity_ms")
    immunity_sle = BoolColumn("immunity_sle")
    immunity_arthritis = BoolColumn("immunity_arthritis")
    immunity_hiv = BoolColumn("immunity_hiv")
    immunity_graves = BoolColumn("immunity_graves")
    immunity_diabetes = BoolColumn("immunity_diabetes")
    immunity_other = BoolColumn("immunity_other")
    immunity_details = Column("immunity_details", Text)

    family_history = BoolColumn("family_history")
    familyhistory_ms = BoolColumn("familyhistory_ms")
    familyhistory_sle = BoolColumn("familyhistory_sle")
    familyhistory_arthritis = BoolColumn("familyhistory_arthritis")
    familyhistory_graves = BoolColumn("familyhistory_graves")
    familyhistory_diabetes = BoolColumn("familyhistory_diabetes")
    familyhistory_psychosis_sz = BoolColumn("familyhistory_psychosis_sz")
    familyhistory_bipolar = BoolColumn("familyhistory_bipolar")
    familyhistory_details = Column("familyhistory_details", Text)

    health_anything_else = BoolColumn("health_anything_else")
    health_anything_else_details = Column(
        "health_anything_else_details", UnicodeText
    )

    drug_history = Column("drug_history", UnicodeText)
    first_antipsychotic_medication = Column(
        "first_antipsychotic_medication", UnicodeText
    )

    recreational_drug_in_last_3_months = BoolColumn(
        "recreational_drug_in_last_3_months",
        constraint_name="ck_deakin_1_healthreview_recdruglast3mo"
    )

    recdrug_tobacco_frequency = CamcopsColumn(
        "recdrug_tobacco_frequency", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    recdrug_tobacco_cigsperweek = CamcopsColumn(
        "recdrug_tobacco_cigsperweek", Integer,
        permitted_value_checker=MIN_ZERO_CHECKER,
        comment="Tobacco: cigarettes per week"
    )
    recdrug_tobacco_prevheavy = BoolColumn("recdrug_tobacco_prevheavy")

    recdrug_cannabis_frequency = CamcopsColumn(
        "recdrug_cannabis_frequency", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    recdrug_cannabis_jointsperweek = CamcopsColumn(
        "recdrug_cannabis_jointsperweek", Integer,
        permitted_value_checker=MIN_ZERO_CHECKER,
        comment="Cannabis: joints per week"
    )
    recdrug_cannabis_prevheavy = BoolColumn("recdrug_cannabis_prevheavy")

    recdrug_alcohol_frequency = CamcopsColumn(
        "recdrug_alcohol_frequency", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    recdrug_alcohol_unitsperweek = CamcopsColumn(
        "recdrug_alcohol_unitsperweek", Integer,
        permitted_value_checker=MIN_ZERO_CHECKER,
        comment="Alcohol: units per week")
    recdrug_alcohol_prevheavy = BoolColumn("recdrug_alcohol_prevheavy")

    recdrug_mdma_frequency = CamcopsColumn(
        "recdrug_mdma_frequency", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    recdrug_mdma_prevheavy = BoolColumn("recdrug_mdma_prevheavy")

    recdrug_cocaine_frequency = CamcopsColumn(
        "recdrug_cocaine_frequency", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    recdrug_cocaine_prevheavy = BoolColumn("recdrug_cocaine_prevheavy")

    recdrug_crack_frequency = CamcopsColumn(
        "recdrug_crack_frequency", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    recdrug_crack_prevheavy = BoolColumn("recdrug_crack_prevheavy")

    recdrug_heroin_frequency = CamcopsColumn(
        "recdrug_heroin_frequency", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    recdrug_heroin_prevheavy = BoolColumn("recdrug_heroin_prevheavy")

    recdrug_methadone_frequency = CamcopsColumn(
        "recdrug_methadone_frequency", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    recdrug_methadone_prevheavy = BoolColumn("recdrug_methadone_prevheavy")

    recdrug_amphetamines_frequency = CamcopsColumn(
        "recdrug_amphetamines_frequency", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    recdrug_amphetamines_prevheavy = BoolColumn(
        "recdrug_amphetamines_prevheavy",
        constraint_name="ck_deakin_1_healthreview_amphetprevheavy"
    )

    recdrug_benzodiazepines_frequency = CamcopsColumn(
        "recdrug_benzodiazepines_frequency", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    recdrug_benzodiazepines_prevheavy = BoolColumn(
        "recdrug_benzodiazepines_prevheavy",
        constraint_name="ck_deakin_1_healthreview_benzoprevheavy"
    )

    recdrug_ketamine_frequency = CamcopsColumn(
        "recdrug_ketamine_frequency", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    recdrug_ketamine_prevheavy = BoolColumn("recdrug_ketamine_prevheavy")

    recdrug_legalhighs_frequency = CamcopsColumn(
        "recdrug_legalhighs_frequency", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    recdrug_legalhighs_prevheavy = BoolColumn("recdrug_legalhighs_prevheavy")

    recdrug_inhalants_frequency = CamcopsColumn(
        "recdrug_inhalants_frequency", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    recdrug_inhalants_prevheavy = BoolColumn("recdrug_inhalants_prevheavy")

    recdrug_hallucinogens_frequency = CamcopsColumn(
        "recdrug_hallucinogens_frequency", Integer,
        permitted_value_checker=ZERO_TO_FOUR_CHECKER,
        comment=FREQUENCY_COMMENT
    )
    recdrug_hallucinogens_prevheavy = BoolColumn(
        "recdrug_hallucinogens_prevheavy",
        constraint_name="ck_deakin_1_healthreview_hallucinogenprevheavy"
    )

    recdrug_details = Column("recdrug_details", UnicodeText)

    recdrug_prevheavy = BoolColumn("recdrug_prevheavy")
    recdrug_prevheavy_details = Column(
        "recdrug_prevheavy_details", UnicodeText
    )

    mri_claustrophobic = BoolColumn("mri_claustrophobic")
    mri_difficulty_lying_1_hour = BoolColumn("mri_difficulty_lying_1_hour")
    mri_nonremovable_metal = BoolColumn("mri_nonremovable_metal")
    mri_metal_from_operations = BoolColumn("mri_metal_from_operations")
    mri_tattoos_nicotine_patches = BoolColumn("mri_tattoos_nicotine_patches")
    mri_worked_with_metal = BoolColumn("mri_worked_with_metal")
    mri_previous_brain_scan = BoolColumn("mri_previous_brain_scan")
    mri_previous_brain_scan_details = Column(
        "mri_previous_brain_scan_details", UnicodeText
    )
    other_relevant_things = BoolColumn("other_relevant_things")
    other_relevant_things_details = Column(
        "other_relevant_things_details", UnicodeText
    )

    willing_to_participate_in_further_studies = BoolColumn(
        "willing_to_participate_in_further_studies",
        constraint_name="ck_deakin_1_healthreview_wtpifs"
    )

    @staticmethod
    def longname(req: "CamcopsRequest") -> str:
        _ = req.gettext
        return _("Deakin JB — Antibody-mediated psychosis study — health review")  # noqa

    def is_complete(self) -> bool:
        return (
            self.all_fields_not_none([
                "ethnicity",
                "handedness",
                "education",
                "allergies",
                "vaccinations_last3months",
                "infections_last3months",
                "infections_chronic",
                "immune_disorders",
                "health_anything_else",
                "recreational_drug_in_last_3_months",
                "recdrug_prevheavy",
                "mri_claustrophobic",
                "mri_difficulty_lying_1_hour",
                "mri_nonremovable_metal",
                "mri_metal_from_operations",
                "mri_tattoos_nicotine_patches",
                "mri_worked_with_metal",
                "mri_previous_brain_scan",
                "other_relevant_things",
                "willing_to_participate_in_further_studies"
            ]) and
            self.field_contents_valid()
        )

    def get_drug_frequency_row(self, fieldname: str) -> str:
        drug_frequency_dict = {
            0: "Did not use",
            1: "Occasionally",
            2: "Monthly",
            3: "Weekly",
            4: "Daily"
        }
        frequency = drug_frequency_dict.get(getattr(self, fieldname), None)
        return tr_qa(fieldname, frequency)

    def get_task_html(self, req: CamcopsRequest) -> str:
        def twocol_bool_row(fieldname: str) -> str:
            return self.get_twocol_bool_row(req, fieldname)

        return f"""
            <div class="{CssClass.SUMMARY}">
                <table class="{CssClass.SUMMARY}">
                    {self.get_is_complete_tr(req)}
                </table>
            </div>
            <table class="{CssClass.TASKDETAIL}">
                <tr>
                    <th width="50%">Question</th>
                    <th width="50%">Answer</th>
                </tr>
        """ + (
            self.get_twocol_val_row("ethnicity") +
            # UNUSED BY CLIENT! # self.get_twocol_string_row("ethnicity_text") +  # noqa
            self.get_twocol_string_row("ethnicity_other_details") +

            self.get_twocol_string_row("handedness") +

            self.get_twocol_string_row("education") +

            twocol_bool_row("allergies") +
            twocol_bool_row("allergy_asthma") +
            twocol_bool_row("allergy_pollen_dust") +
            twocol_bool_row("allergy_dermatitis") +
            twocol_bool_row("allergy_food") +
            twocol_bool_row("allergy_dander") +
            twocol_bool_row("allergy_other") +
            self.get_twocol_string_row("allergy_details") +

            twocol_bool_row("vaccinations_last3months") +
            self.get_twocol_string_row("vaccination_details") +

            twocol_bool_row("infections_last3months") +
            twocol_bool_row("infection_recent_respiratory") +
            twocol_bool_row("infection_recent_gastroenteritis") +
            twocol_bool_row("infection_recent_urinary") +
            twocol_bool_row("infection_recent_sexual") +
            twocol_bool_row("infection_recent_hepatitis") +
            twocol_bool_row("infection_recent_other") +
            self.get_twocol_string_row("infection_recent_details") +

            twocol_bool_row("infections_chronic") +
            twocol_bool_row("infection_chronic_respiratory") +
            twocol_bool_row("infection_chronic_gastroenteritis") +
            twocol_bool_row("infection_chronic_urinary") +
            twocol_bool_row("infection_chronic_sexual") +
            twocol_bool_row("infection_chronic_hepatitis") +
            twocol_bool_row("infection_chronic_other") +
            self.get_twocol_string_row("infection_chronic_details") +

            twocol_bool_row("immune_disorders") +
            twocol_bool_row("immunity_ms") +
            twocol_bool_row("immunity_sle") +
            twocol_bool_row("immunity_arthritis") +
            twocol_bool_row("immunity_hiv") +
            twocol_bool_row("immunity_graves") +
            twocol_bool_row("immunity_diabetes") +
            twocol_bool_row("immunity_other") +
            self.get_twocol_string_row("immunity_details") +

            twocol_bool_row("family_history") +
            twocol_bool_row("familyhistory_ms") +
            twocol_bool_row("familyhistory_sle") +
            twocol_bool_row("familyhistory_arthritis") +
            twocol_bool_row("familyhistory_graves") +
            twocol_bool_row("familyhistory_diabetes") +
            twocol_bool_row("familyhistory_psychosis_sz") +
            twocol_bool_row("familyhistory_bipolar") +
            self.get_twocol_string_row("familyhistory_details") +

            twocol_bool_row("health_anything_else") +
            self.get_twocol_string_row("health_anything_else_details") +

            self.get_twocol_string_row("drug_history") +
            self.get_twocol_string_row("first_antipsychotic_medication") +

            twocol_bool_row("recreational_drug_in_last_3_months") +
            self.get_drug_frequency_row("recdrug_tobacco_frequency") +
            self.get_twocol_val_row("recdrug_tobacco_cigsperweek") +
            twocol_bool_row("recdrug_tobacco_prevheavy") +
            self.get_drug_frequency_row("recdrug_cannabis_frequency") +
            self.get_twocol_val_row("recdrug_cannabis_jointsperweek") +
            twocol_bool_row("recdrug_cannabis_prevheavy") +
            self.get_drug_frequency_row("recdrug_alcohol_frequency") +
            self.get_twocol_val_row("recdrug_alcohol_unitsperweek") +
            twocol_bool_row("recdrug_alcohol_prevheavy") +
            self.get_drug_frequency_row("recdrug_mdma_frequency") +
            twocol_bool_row("recdrug_mdma_prevheavy") +
            self.get_drug_frequency_row("recdrug_cocaine_frequency") +
            twocol_bool_row("recdrug_cocaine_prevheavy") +
            self.get_drug_frequency_row("recdrug_crack_frequency") +
            twocol_bool_row("recdrug_crack_prevheavy") +
            self.get_drug_frequency_row("recdrug_heroin_frequency") +
            twocol_bool_row("recdrug_heroin_prevheavy") +
            self.get_drug_frequency_row("recdrug_methadone_frequency") +
            twocol_bool_row("recdrug_methadone_prevheavy") +
            self.get_drug_frequency_row("recdrug_amphetamines_frequency") +
            twocol_bool_row("recdrug_amphetamines_prevheavy") +
            self.get_drug_frequency_row("recdrug_benzodiazepines_frequency") +
            twocol_bool_row("recdrug_benzodiazepines_prevheavy") +
            self.get_drug_frequency_row("recdrug_ketamine_frequency") +
            twocol_bool_row("recdrug_ketamine_prevheavy") +
            self.get_drug_frequency_row("recdrug_legalhighs_frequency") +
            twocol_bool_row("recdrug_legalhighs_prevheavy") +
            self.get_drug_frequency_row("recdrug_inhalants_frequency") +
            twocol_bool_row("recdrug_inhalants_prevheavy") +
            self.get_drug_frequency_row("recdrug_hallucinogens_frequency") +
            twocol_bool_row("recdrug_hallucinogens_prevheavy") +
            self.get_twocol_string_row("recdrug_details") +
            twocol_bool_row("recdrug_prevheavy") +
            self.get_twocol_string_row("recdrug_prevheavy_details") +

            twocol_bool_row("mri_claustrophobic") +
            twocol_bool_row("mri_difficulty_lying_1_hour") +
            twocol_bool_row("mri_nonremovable_metal") +
            twocol_bool_row("mri_metal_from_operations") +
            twocol_bool_row("mri_tattoos_nicotine_patches") +
            twocol_bool_row("mri_worked_with_metal") +
            twocol_bool_row("mri_previous_brain_scan") +
            self.get_twocol_string_row("mri_previous_brain_scan_details") +
            twocol_bool_row("other_relevant_things") +
            self.get_twocol_string_row("other_relevant_things_details") +
            twocol_bool_row(
                "willing_to_participate_in_further_studies") +

            "</table>"
        )
