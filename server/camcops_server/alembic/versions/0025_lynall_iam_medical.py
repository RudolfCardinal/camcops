#!/usr/bin/env python

"""
camcops_server/alembic/versions/0025_lynall_iam_medical.py

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

DATABASE REVISION SCRIPT

lynall_1_iam_medical

Revision ID: 0025
Revises: 0024
Creation date: 2019-06-03 10:46:48.698930

"""

# =============================================================================
# Imports
# =============================================================================

from alembic import op
import sqlalchemy as sa
import camcops_server.cc_modules.cc_sqla_coltypes


# =============================================================================
# Revision identifiers, used by Alembic.
# =============================================================================

revision = '0025'
down_revision = '0024'
branch_labels = None
depends_on = None


# =============================================================================
# The upgrade/downgrade steps
# =============================================================================

# noinspection PyPep8,PyTypeChecker
def upgrade():
    op.create_table('lynall_1_iam_medical',
        sa.Column('q1_age_first_inflammatory_sx', sa.Integer(), nullable=True, comment='Age (y) at onset of first symptoms of inflammatory disease'),
        sa.Column('q2_when_psych_sx_started', sa.Integer(), nullable=True, comment='Timing of onset of psych symptoms (1 = NA, 2 = before physical symptoms [Sx], 3 = same time as physical Sx but before diagnosis [Dx], 4 = around time of Dx, 5 = weeks or months after Dx, 6 = years after Dx)'),
        sa.Column('q3_worst_symptom_last_month', sa.Integer(), nullable=True, comment='Worst symptom in last month (1 = fatigue, 2 = low mood, 3 = irritable, 4 = anxiety, 5 = brain fog/confused, 6 = pain, 7 = bowel Sx, 8 = mobility, 9 = skin, 10 = other, 11 = no Sx in past month)'),
        sa.Column('q4a_symptom_timing', sa.Integer(), nullable=True, comment='Timing of brain/psych Sx relative to physical Sx (1 = brain before physical, 2 = brain after physical, 3 = same time, 4 = no relationship, 5 = none of the above)'),
        sa.Column('q4b_days_psych_before_phys', sa.Integer(), nullable=True, comment='If Q4a == 1, number of days that brain Sx typically begin before physical Sx'),
        sa.Column('q4c_days_psych_after_phys', sa.Integer(), nullable=True, comment='If Q4a == 2, number of days that brain Sx typically begin after physical Sx'),
        sa.Column('q5_antibiotics', sa.Boolean(), nullable=True, comment='Medication for infection (e.g. antibiotics) in past 3 months? (0 = no, 1 = yes)'),
        sa.Column('q6a_inpatient_last_y', sa.Boolean(), nullable=True, comment='Inpatient in the last year? (0 = no, 1 = yes)'),
        sa.Column('q6b_inpatient_weeks', sa.Integer(), nullable=True, comment='If Q6a is true, approximate number of weeks spent as an inpatient in the past year'),
        sa.Column('q7a_sx_last_2y', sa.Boolean(), nullable=True, comment='Symptoms within the last 2 years? (0 = no, 1 = yes)'),
        sa.Column('q7b_variability', sa.Integer(), nullable=True, comment='If Q7a is true, degree of variability of symptoms (1-10 where 1 = highly variable [from none to severe], 10 = there all the time)'),
        sa.Column('q8_smoking', sa.Integer(), nullable=True, comment='Current smoking status (0 = no, 1 = yes but not every day, 2 = every day)'),
        sa.Column('q9_pregnant', sa.Boolean(), nullable=True, comment='Currently pregnant (0 = no or N/A, 1 = yes)'),
        sa.Column('q10a_effective_rx_physical', sa.UnicodeText(), nullable=True, comment='Most effective treatments for physical Sx'),
        sa.Column('q10b_effective_rx_psych', sa.UnicodeText(), nullable=True, comment='Most effective treatments for brain/psychiatric Sx'),
        sa.Column('q11a_ph_depression', sa.Boolean(), nullable=True, comment='Personal history of depression?'),
        sa.Column('q11b_ph_bipolar', sa.Boolean(), nullable=True, comment='Personal history of bipolar disorder?'),
        sa.Column('q11c_ph_schizophrenia', sa.Boolean(), nullable=True, comment='Personal history of schizophrenia?'),
        sa.Column('q11d_ph_autistic_spectrum', sa.Boolean(), nullable=True, comment="Personal history of autism/Asperger's?"),
        sa.Column('q11e_ph_ptsd', sa.Boolean(), nullable=True, comment='Personal history of PTSD?'),
        sa.Column('q11f_ph_other_anxiety', sa.Boolean(), nullable=True, comment='Personal history of other anxiety disorders?'),
        sa.Column('q11g_ph_personality_disorder', sa.Boolean(), nullable=True, comment='Personal history of personality disorder?'),
        sa.Column('q11h_ph_other_psych', sa.Boolean(), nullable=True, comment='Personal history of other psychiatric disorder(s)?'),
        sa.Column('q11h_ph_other_detail', sa.UnicodeText(), nullable=True, comment='If q11h_ph_other_psych is true, this is the free-text details field'),
        sa.Column('q12a_fh_depression', sa.Boolean(), nullable=True, comment='Family history of depression?'),
        sa.Column('q12b_fh_bipolar', sa.Boolean(), nullable=True, comment='Family history of bipolar disorder?'),
        sa.Column('q12c_fh_schizophrenia', sa.Boolean(), nullable=True, comment='Family history of schizophrenia?'),
        sa.Column('q12d_fh_autistic_spectrum', sa.Boolean(), nullable=True, comment="Family history of autism/Asperger's?"),
        sa.Column('q12e_fh_ptsd', sa.Boolean(), nullable=True, comment='Family history of PTSD?'),
        sa.Column('q12f_fh_other_anxiety', sa.Boolean(), nullable=True, comment='Family history of other anxiety disorders?'),
        sa.Column('q12g_fh_personality_disorder', sa.Boolean(), nullable=True, comment='Family history of personality disorder?'),
        sa.Column('q12h_fh_other_psych', sa.Boolean(), nullable=True, comment='Family history of other psychiatric disorder(s)?'),
        sa.Column('q12h_fh_other_detail', sa.UnicodeText(), nullable=True, comment='If q12h_fh_other_psych is true, this is the free-text details field'),
        sa.Column('q13a_behcet', sa.Boolean(), nullable=True, comment='Behçet’s syndrome? (0 = no, 1 = yes)'),
        sa.Column('q13b_oral_ulcers', sa.Boolean(), nullable=True, comment='(If Behçet’s) Oral ulcers? (0 = no, 1 = yes)'),
        sa.Column('q13c_oral_age_first', sa.Integer(), nullable=True, comment='(If Behçet’s + oral) Age (y) at first oral ulcers'),
        sa.Column('q13d_oral_scarring', sa.Boolean(), nullable=True, comment='(If Behçet’s + oral) Oral scarring? (0 = no, 1 = yes)'),
        sa.Column('q13e_genital_ulcers', sa.Boolean(), nullable=True, comment='(If Behçet’s) Genital ulcers? (0 = no, 1 = yes)'),
        sa.Column('q13f_genital_age_first', sa.Integer(), nullable=True, comment='(If Behçet’s + genital) Age (y) at first genital ulcers'),
        sa.Column('q13g_genital_scarring', sa.Boolean(), nullable=True, comment='(If Behçet’s + genital) Genital scarring? (0 = no, 1 = yes)'),
        sa.Column('patient_id', sa.Integer(), nullable=False, comment='(TASK) Foreign key to patient.id (for this device/era)'),
        sa.Column('when_created', camcops_server.cc_modules.cc_sqla_coltypes.PendulumDateTimeAsIsoTextColType(length=32), nullable=False, comment='(TASK) Date/time this task instance was created (ISO 8601)'),
        sa.Column('when_firstexit', camcops_server.cc_modules.cc_sqla_coltypes.PendulumDateTimeAsIsoTextColType(length=32), nullable=True, comment='(TASK) Date/time of the first exit from this task (ISO 8601)'),
        sa.Column('firstexit_is_finish', sa.Boolean(), nullable=True, comment='(TASK) Was the first exit from the task because it was finished (1)?'),
        sa.Column('firstexit_is_abort', sa.Boolean(), nullable=True, comment='(TASK) Was the first exit from this task because it was aborted (1)?'),
        sa.Column('editing_time_s', sa.Float(), nullable=True, comment='(TASK) Time spent editing (s)'),
        sa.Column('_pk', sa.Integer(), autoincrement=True, nullable=False, comment='(SERVER) Primary key (on the server)'),
        sa.Column('_device_id', sa.Integer(), nullable=False, comment='(SERVER) ID of the source tablet device'),
        sa.Column('_era', sa.String(length=32), nullable=False, comment="(SERVER) 'NOW', or when this row was preserved and removed from the source device (UTC ISO 8601)"),
        sa.Column('_current', sa.Boolean(), nullable=False, comment='(SERVER) Is the row current (1) or not (0)?'),
        sa.Column('_when_added_exact', camcops_server.cc_modules.cc_sqla_coltypes.PendulumDateTimeAsIsoTextColType(length=32), nullable=True, comment='(SERVER) Date/time this row was added (ISO 8601)'),
        sa.Column('_when_added_batch_utc', sa.DateTime(), nullable=True, comment='(SERVER) Date/time of the upload batch that added this row (DATETIME in UTC)'),
        sa.Column('_adding_user_id', sa.Integer(), nullable=True, comment='(SERVER) ID of user that added this row'),
        sa.Column('_when_removed_exact', camcops_server.cc_modules.cc_sqla_coltypes.PendulumDateTimeAsIsoTextColType(length=32), nullable=True, comment='(SERVER) Date/time this row was removed, i.e. made not current (ISO 8601)'),
        sa.Column('_when_removed_batch_utc', sa.DateTime(), nullable=True, comment='(SERVER) Date/time of the upload batch that removed this row (DATETIME in UTC)'),
        sa.Column('_removing_user_id', sa.Integer(), nullable=True, comment='(SERVER) ID of user that removed this row'),
        sa.Column('_preserving_user_id', sa.Integer(), nullable=True, comment='(SERVER) ID of user that preserved this row'),
        sa.Column('_forcibly_preserved', sa.Boolean(), nullable=True, comment='(SERVER) Forcibly preserved by superuser (rather than normally preserved by tablet)?'),
        sa.Column('_predecessor_pk', sa.Integer(), nullable=True, comment='(SERVER) PK of predecessor record, prior to modification'),
        sa.Column('_successor_pk', sa.Integer(), nullable=True, comment='(SERVER) PK of successor record  (after modification) or NULL (whilst live, or after deletion)'),
        sa.Column('_manually_erased', sa.Boolean(), nullable=True, comment='(SERVER) Record manually erased (content destroyed)?'),
        sa.Column('_manually_erased_at', camcops_server.cc_modules.cc_sqla_coltypes.PendulumDateTimeAsIsoTextColType(length=32), nullable=True, comment='(SERVER) Date/time of manual erasure (ISO 8601)'),
        sa.Column('_manually_erasing_user_id', sa.Integer(), nullable=True, comment='(SERVER) ID of user that erased this row manually'),
        sa.Column('_camcops_version', camcops_server.cc_modules.cc_sqla_coltypes.SemanticVersionColType(length=147), nullable=True, comment='(SERVER) CamCOPS version number of the uploading device'),
        sa.Column('_addition_pending', sa.Boolean(), nullable=False, comment='(SERVER) Addition pending?'),
        sa.Column('_removal_pending', sa.Boolean(), nullable=True, comment='(SERVER) Removal pending?'),
        sa.Column('_group_id', sa.Integer(), nullable=False, comment='(SERVER) ID of group to which this record belongs'),
        sa.Column('id', sa.Integer(), nullable=False, comment='(TASK) Primary key (task ID) on the tablet device'),
        sa.Column('when_last_modified', camcops_server.cc_modules.cc_sqla_coltypes.PendulumDateTimeAsIsoTextColType(length=32), nullable=True, comment='(STANDARD) Date/time this row was last modified on the source tablet device (ISO 8601)'),
        sa.Column('_move_off_tablet', sa.Boolean(), nullable=True, comment='(SERVER/TABLET) Record-specific preservation pending?'),
        sa.ForeignKeyConstraint(['_adding_user_id'], ['_security_users.id'], name=op.f('fk_lynall_1_iam_medical__adding_user_id')),
        sa.ForeignKeyConstraint(['_device_id'], ['_security_devices.id'], name=op.f('fk_lynall_1_iam_medical__device_id')),
        sa.ForeignKeyConstraint(['_group_id'], ['_security_groups.id'], name=op.f('fk_lynall_1_iam_medical__group_id')),
        sa.ForeignKeyConstraint(['_manually_erasing_user_id'], ['_security_users.id'], name=op.f('fk_lynall_1_iam_medical__manually_erasing_user_id')),
        sa.ForeignKeyConstraint(['_preserving_user_id'], ['_security_users.id'], name=op.f('fk_lynall_1_iam_medical__preserving_user_id')),
        sa.ForeignKeyConstraint(['_removing_user_id'], ['_security_users.id'], name=op.f('fk_lynall_1_iam_medical__removing_user_id')),
        sa.PrimaryKeyConstraint('_pk', name=op.f('pk_lynall_1_iam_medical')),
        mysql_charset='utf8mb4 COLLATE utf8mb4_unicode_ci',
        mysql_engine='InnoDB',
        mysql_row_format='DYNAMIC'
    )
    with op.batch_alter_table('lynall_1_iam_medical', schema=None) as batch_op:
        batch_op.create_index(batch_op.f('ix_lynall_1_iam_medical__current'), ['_current'], unique=False)
        batch_op.create_index(batch_op.f('ix_lynall_1_iam_medical__device_id'), ['_device_id'], unique=False)
        batch_op.create_index(batch_op.f('ix_lynall_1_iam_medical__era'), ['_era'], unique=False)
        batch_op.create_index(batch_op.f('ix_lynall_1_iam_medical__group_id'), ['_group_id'], unique=False)
        batch_op.create_index(batch_op.f('ix_lynall_1_iam_medical__pk'), ['_pk'], unique=False)
        batch_op.create_index(batch_op.f('ix_lynall_1_iam_medical_id'), ['id'], unique=False)
        batch_op.create_index(batch_op.f('ix_lynall_1_iam_medical_patient_id'), ['patient_id'], unique=False)
        batch_op.create_index(batch_op.f('ix_lynall_1_iam_medical_when_last_modified'), ['when_last_modified'], unique=False)


def downgrade():
    op.drop_table('lynall_1_iam_medical')
