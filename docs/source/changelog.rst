..  docs/source/changelog.rst

..  Copyright (C) 2012-2020 Rudolf Cardinal (rudolf@pobox.com).
    .
    This file is part of CamCOPS.
    .
    CamCOPS is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    .
    CamCOPS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
    .
    You should have received a copy of the GNU General Public License
    along with CamCOPS. If not, see <http://www.gnu.org/licenses/>.

Change log/history
==================

*Both client and server changes are described here.*

Quick links:

- :ref:`2013 <changelog_2013>`
- :ref:`2014 <changelog_2014>`
- :ref:`2015 <changelog_2015>`
- :ref:`2016 <changelog_2016>`
- :ref:`2017 <changelog_2017>`
- :ref:`2018 <changelog_2018>`
- :ref:`2019 <changelog_2019>`
- :ref:`2020 <changelog_2020>`
- :ref:`2021 <changelog_2021>`


Contributors
------------

- Rudolf Cardinal, 2012–.

  - Everything except as below.

- Joe Kearney, 2018–2019.

  - :ref:`CES-D <cesd>`
  - :ref:`FACT-G <factg>`
  - :ref:`EQ-5D-5L <eq5d5l>`
  - :ref:`SRS <srs>`
  - :ref:`ORS <ors>`
  - :ref:`APEQPT <apeqpt>`
  - :ref:`GBO-GReS <gbo_gres>`
  - :ref:`GBO-GPC <gbo_gpc>`

- Martin Burchell, 2019–.

  - Better Github framework/workflow.
  - :ref:`Elixhauser Comorbidity Index (ElixhauserCI) <elixhauserci>`
  - :ref:`Cambridge-Chicago Compulsivity Trait Scale (CHI-T) <chit>`
  - :ref:`Short UPPS-P Impulsive Behaviour Scale (SUPPS-P) <suppsp>`
  - :ref:`EULAR Sjögren’s Syndrome Patient Reported Index (ESSPRI) <esspri>`
  - :ref:`Ankylosing Spondylitis Disease Activity Score (ASDAS) <asdas>`
  - :ref:`Multidimensional Fatigue Inventory (MFI-20) <mfi20>`
  - :ref:`Short-Form McGill Pain Questionnaire (SF-MPQ2) <sfmpq2>`
  - :ref:`Disease Activity Score-28 (DAS28) <das28>`
  - :ref:`Snaith–Hamilton Pleasure Scale (SHAPS) <shaps>`
  - CPFT Perinatal, MOJO.
  - Back-end data processing and e-mail framework.
  - REDCap interface.
  - Task scheduling.
  - Single-user mode.
  - Improvements to web site front end.
  - ... and lots more.


Original Titanium/Javascript client, Python server with custom MySQL interface (defunct)
----------------------------------------------------------------------------------------


.. _changelog_2013:

2013
~~~~


**Server v1.0, 2013-08-14**

- First version (1.0).


**Client v1.0, 2013-11-13**

- first version
- requires server version 1.0


**Client v1.01, 2013-11-13**

- test of version number increment
- Bugfix: Executive menu had a duff entry in and crashed.


**Server v1.01, 2013-11-20**

- Test of version number increment (1.01).
- Trivial change: ensure empty "\*_SUMMARY_TEMP_current\*" views
  aren't created for anonymous tasks.


**Client 1.02, 2013-11-22 onwards**

- A couple of cosmetic changes.
- Analytics yes/no option.
- Changed app "domain" to org.camcops.\*, so app is org.camcops.camcops
- Signed APK file.
- QuestionTypedVariables improved in a few respects.
- QuestionDiagnosticCode bugfix (didn't appear read-only in read-only mode).
- android:allowBackup explicitly set to false
- ID description/policy check on upload.
- Titanium API now 3.2.0.GA
- Page jump in questionnaires when read-only.
- Two CECAQ3 fields used the wrong keyboard/type.
- Text field/cursor colours improved for iOS/Android.
- Bugfix to QuestionCanvas_webview.


**Server v1.02, 2013-11-28**

- Mostly changes on the app side (q.v.).
- Change to DemoQuestionnaire fields.
- DOB task filter.
- Server analytics with yes/no option.
- Fixed layout on old versions of Internet Explorer.
- get_id_info command in the database interface.
- QoL\* tasks remain in beta; data structure may change.
- Changes for CentOS, including Python version check and altered shebang.
  Using "#!/usr/bin/env python2.7" is perhaps desirable, but Lintian requires
  e.g. "#!/usr/bin/python2.7":
  http://lintian.debian.org/tags/python-script-but-no-python-dep.html
- Clinical text view.


.. _changelog_2014:

2014
~~~~


**Client v1.03, 2014-01-10**

- Requires server version 1.03.
- Fixed Titanium 3.2.0 multiline TextArea regression.
- CGI-SCH task, pending permissions.
- androidtipaint/QuestionCanvas_tipaint improved/fixed for Titanium 3.2.0.
- Questionnaire scrollview made full height (Titanium now capable of it).
- Single-tap/double-tap methods in diagnostic coding, now Titanium bug
  https://jira.appcelerator.org/browse/TIMOB-15540 fixed.
- Photo rotation bug fixed.
- ListView for diagnostic code search.
- QoL-SG phrasing improved.
- Tested on iOS 7.0.3/7.0.4, Android 4.1.1.


**Server v1.03, 2014-01-10**

- CGI-SCH task.


**Client v1.04, 2014-01-14**

- First beta version.
- Bugfix to Patient.js (re address display crash).
- Changes to SetMenu_Deakin_1.js
- Confirmation of CGI-SCH permissions.


**Client v1.05, 2014-01-14**

- Password entry windows improved: return key now accepts data entry.


**Client v1.06, 2014-01-16**

- Requires server version 1.06.
- CPFT_LPS_Referral, CPFT_LPS_Discharge, CPFT_LPS_ResetStartClock tasks.
  This are IN BETA.
- Batch upload empty tables for speed (big improvement).
- NULL-but-optional indicator in widgets:

  - QuestionDateTime, QuestionPickerInline, QuestionPickerPopup,
    QuestionSlider, ImageGroupVertical.

- offerNullButton option in QuestionDateTime, QuestionDiagnosticCode
- Variable column widths in ContainerTable, plus populateVertically option.
- Bugfix in QuestionTypedVariables layout for colWidthPrompt.
- (2014-01-18) Minor layour change in CPFT_LPS_Referral.


**Server v1.06, 2014-01-16**

- REQUIRES DATABASE CHANGE BEFORE INSTALLATION:
  DROP TABLE _dirty_tables;
- CPFT_LPS_Referral, CPFT_LPS_Discharge, CPFT_LPS_ResetStartClock tasks.
  IN BETA; MAY CHANGE.
- QoL\* tasks remain in beta; data structure may change.
- Options in man page.
- cc_patient.py / get_id_generic and similar: bugfix to use Unicode
- Clinical text provided by Photo/PhotoSequence.
- Batch upload empty tables.
- rnc_db: skips creation of tables that exist already (removes a warning).
- Joint PK for _dirty_tables, and change from TEXT to VARCHAR(255)
  for the tablenamefield.
- Bugfix to database.pl / flag_deleted_where_clientpk_not: wasn't
  device-specific! Was used by blob upload on the tablet, i.e.
  dbupload.sendTableRecordwise()


**Server v1.07, 2014-02-14**

- REQUIRES DATABASE CHANGE BEFORE INSTALLATION:
  DROP TABLE _security_webviewer_sessions;
- CPFT\* tasks remain in beta; data structure may change.
- QoL\* tasks remain in beta; data structure may change.
- Additional content for clinical text views.
- Python virtualenv.
- Dumping/reporting options for suitably privileged users.
  Additional user permissions: may_dump_data, may_run_reports.
- Bugfix to Session class to prevent the (incredibly unlikely)
  event of an IP address hop with an identical session token.
- Security improvement to Session class: change token upon login.
- Speedup to Session design (inc. integer PK).
- Typo in CAPS text, Q24.
- Speedup to LSTRING XML processor.
- Speedup via transaction-based database handling in the Python handler.
- Redirect to destination URL after re-authentication.


**Server v1.08, 2014-07-22**

- REQUIRES COMMAND TO UPGRADE EACH DATABASE:
  camcops --maketables /etc/camcops/MYCONFIGNAME.conf
- Automatic version-based database structure upgrade via the --maketables
  command. (Similarly on the tablet side.)
- Distinct patient reports.
- CPFT\* tasks remain in beta; data structure may change.
- QoL\* tasks remain in beta; data structure may change.
- Remote IP addresses stored in audit log (additional field: remote_addr).
- Auditing of clinical text views.
- Some string constant code cleanup.
- Some Perl code cleanup and upload audit simplification.
- perltidy for Perl code.
- Trackers/CTVs clearer in their errors when no data found.
- Ability to apply multiple filters simultaneously.
- Option to force password changes periodically/ad hoc.
- PEP8 compliance for core Python code.
- PEP8 compliance for task code.
- Proper multiple inheritance handling for diagnosis.py, pcl.py.
- Disclaimer/acknowledgement recording.
- Audit all login attempts, plus user addition/deletion.
- OptionParser to ArgumentParser.
- Internal URLs for tasks altered slightly.
- Better internal timezone handling.
- Commit during menu-driven administration to prevent database locking.
- Lock user accounts after multiple login failures.
- HL7 message framework. (Validated internally and against HL7 Inspector.)
- File export message framework, with post-export script option.
- Database title, ID descriptions, and policies now have their primary home
  in the configuration file. Copied to database purely for researcher lookup.
- File locking for the regeneration of summary tables.
- XML export (tasks, trackers, CTVs).
- Unit testing framework (and a couple of bugs fixed).
- Shift to unsigned ints for PKs.
- Option to introspect source code.
- Option to view table definitions from webview.
- Basic non-modifying anonymisation system.
- Bugfix: added vignette to ICD10-PD display.
- Bugfix: HAMD-7 maximum is 26, not 23.
- Bugfix: CECAQ3 failed to calculate some summary scores with no siblings,
  and paternal psychological abuse score was sometimes inappropriately blank.
- Bugfix: filter for incomplete tasks only wasn't working.
- Bugfix: logic bugfix in ICD-10 manic, mixed, schizophrenia.
- Bugfix: categorization text in BMI.
- Bugfix: clinical text for SLUMS reported incorrect maximum.
- BMI thresholds refined in the underweight zone and referenced properly.
- All field comments.
- Manual erasure of individual tasks.
- Manual deletion of entire patients/associated tasks.
- Manual application of special notes.
- CTV is clearer when tasks are incomplete.
- More consistent formatting of null values in HTML. (Note that the quick
  way to view null handling is to specify a nonexistent server PK.) The aim is
  that all user answers should be proceesed via the answer() function, to apply
  typographic indications that the field is null.
- camcopswebview.py renamed to camcops.py.
- Optimization on compile.
- Ensure commit/rollback always occurs, even after exceptions.
- "crash" action to induce a deliberate exception, for testing.
- Configurable save-as filenames for tasks, trackers, and CTVs.
- Server-side validation of fields (field_contents_valid).
- Unit tests prohibit tasks from having summary fields with the same name as
  a main task field.
- Option to disable password autocompletion on the login page.
- Server version number in "office" details.
- Generator function for task list.
- Drop-down lists for filters remember state.
- Basic research dump (likely to be the most useful in practice).


**Client v1.08, 2014-07-23**

- Requires server version 1.08.
- Field renaming within Icd10Schizophrenia to avoid misnomers:

  - tpah_commentary TO hv_commentary
  - tpah_discussing TO hv_discussing
  - tpah_from_body TO hv_from_body

- CPFT\* tasks remain in beta.
- Chaining of tasks.
- Page jump within live questionnaires (allowPageJumpDuringEditing).
- Radio buttons allow double-clicks/taps to unset them (particularly applicable
  for potentially loaded questions).
- Bugfix to HAMD-7: Q4 value 4 and Q5 values 3/4 were not offered, and maximum
  is 26, not 23.
- Bugfix to SLUMS: Q9a, Q9b were scored as 1 point each; should be 2.
- Bugfix calling bad afterwardsFunc() after "move" upload.
- BMI thresholds refined in the underweight zone and referenced properly.
- Textual annotation to ICD-10 F90.0, as the actual text gives you no clue that
  it's a division of hyperkinetic disorders.
- dbcore.js changed to reflect Titanium bugfix.
- Android theme changed to light (with consequent changes to questionnaire
  font size editing screen, etc.).


**Client v1.09, 2014-08-02**

- Requires server version 1.09.
- Sends BLOBs in ways that cannot be confused with (even very bizarre)
  strings.
- PANSS stripped down to data collection tool only, for copyright reasons.
- Not distributed yet.


**Server v1.09, 2014-08-02**

- REQUIRES TABLET CLIENT V1.09.
- Full rewrite of the database upload script to Python.
- Fix MySQL "morning bug" ("MySQL server has gone away") from the Perl upload
  script.
- Logic change to flag_all_records_deleted(), which was not restricted to
  _current/era='NOW' records, but should have been.
- Also rolls back preservation flag changes as part of general rollback.
- BLOB transfer encoding improved; fixes design flaw that was due to the use
  of the Perl CSV module. (Requires tablet client v1.09 as a result.)
- Internal code changes: explicit modules in all cases, removing
  cc_shared.py.
- PANSS stripped down to data collection tool only, for copyright reasons.


**Client v1.10, 2014-08-08**

- Default network timeout changed from 5 s (5000 ms) to 60 s (60000 ms), as
  shorter timeouts were causing large BLOB uploads to fail.
- Minor fix to newline decoding for the mobileweb client.
- Ability to null out dates of birth (for anonymised research use).
- NULL dates now show in the widget as 01 Jan 1900, not the current date (it's
  impossible to show an actual NULL, and the current date is confusing when you
  have neonates).
- QuestionDateTime widget wouldn't successfully NULL itself on Android. (So
  now it NULLs itself but doesn't update its pseudo-date; it just displays the
  NULL icon.)
- First jshint compliance (except for included third-party libraries)...
- ... then jslint compliance.
- Unit testing framework.
- Not distributed yet.


**Client v1.12, 2014-09-11**

- Renamed ExpDetThreshold/ExpectationDetection tasks (and tables) to add
  a "[C/c]ardinal_" prefix, as the names were too vague. THEREFORE requires
  server version 1.12 as well.
- Session-based authentication for tablets to improve speed (i.e. no need for
  bcrypt reauthentication within the same session, as for the web front end).
- Whisker interface.

**Client v1.14, 2014-10-15**

- Requires server version 1.14.
- Server can enforce a minimum tablet version, and tablet can specify a minimum
  server version. Version numbers are in common/VERSION.js for the tablet.
- Bugfix: tablet registration crashed if the Patient table hadn't been created.
  And similar subsequent bug when uploading with no tables.
- CAPE-42 task.


**Server v1.10, 2014-08-16**

- REQUIRES COMMAND TO UPGRADE EACH DATABASE:
  camcops --maketables /etc/camcops/MYCONFIGNAME.conf
- Database upload script could fail to insert but not complain to the tablet.
- Stopped database handler (rnc_db.py) masking any exceptions.
- Improved exception handling in database.py.
- Bug: patient table incorrectly had forename/surname/DOB fields as NOT NULL.
  Sex column also now has that constraint removed (enforced elsewhere but one
  could envisage not enforcing it).
- Tablet-side (webclient) minor fix to newline escaping.
- Removed Unicode from error messages in make_summary_tables(), since they
  also go to the Apache log.
- Bugfix: login failures were redirecting to the page for acknowledging terms
  and conditions. Bug was in login().
- Bugfix: effective deadlock between the process of a mandatory password
  change for new users and acknowledging terms/conditions.
- Make database/username more prominent (bold) in menus. Was easy to ignore.
- pyflakes compliance.


**Server v1.11, 2014-09-06**

- Future necessity to discriminate field types that all use VARCHAR; e.g.
  (and esp.) ISO-8601 dates versus others. So change sqltype to cctype
  internally; see cc_db.add_sqltype().
- Significant simplification of work done in tasks with ancillary tables.
  New cc_task.Ancillary class; q.v.
- Export to CRIS staging database and autocreate draft data dictionary.


**Server v1.12, 2014-09-11**

- REQUIRES TABLET CLIENT V1.12.
- REQUIRES COMMAND TO UPGRADE EACH DATABASE:
  camcops --maketables /etc/camcops/MYCONFIGNAME.conf
- Renamed ExpDetThreshold/ExpectationDetection tasks (and tables) to add
  a "[C/c]ardinal_" prefix, as the names were too vague. THEREFORE requires
  tablet version 1.12 as well.
- Session-based authentication for tablets to improve speed (i.e. no need for
  bcrypt reauthentication within the same session, as for the web front end).


**Server v1.13, 2014-10-02**

- Trivial code changes.


**Server v1.14, 2014-10-15**

- REQUIRES COMMAND TO UPGRADE EACH DATABASE:
  camcops --maketables /etc/camcops/MYCONFIGNAME.conf
- REQUIRES TABLET CLIENT V1.14.
- Server can enforce a minimum tablet version, and tablet can specify a
  minimum server version. Version numbers are in cc_version.py on the server.
- CAPE-42 task.


**Client v1.15, 2014-10-18**

- Requires server version 1.15.
- NHS numbers were being corrupted, i.e. very long (10-digit) numbers.

  - Critical error. Stored correctly in database.
  - SQLite maximum integer is 2^63 - 1 = 9,223,372,036,854,775,807.
  - Javascript safe max is 9,007,199,254,740,991.
  - A valid database was read incorrectly by dbsqlite.js / getAllRows().
  - Ah. Titanium bug: https://jira.appcelerator.org/browse/TIMOB-3050

  - Workaround is either

    (a) float, which won't be quoted by the SQLite quote() function, and
    which MySQL will happily accept (rounding); and all numbers are floats
    anyway in Javascript;

    or

    (b) text, with parseInt() when reading from SQLite to Javascript.
    This will send integer values quoted, but MySQL will convert even e.g.
    '9876543209.999' (with the quotes) to 9876543210 when inserted into a
    BIGINT field, so that's OK. The parseInt() function will truncate, which
    is also fine.

    I guess float is slightly more logical. Let's be quite clear: in
    Javascript, all numbers are floats; they are 64-bit floating point
    values, the largest safe exact integer is Number.MAX_SAFE_INTEGER, or
    9007199254740991.

  - So:

- QuestionTypedVariables applies +/- Number.MAX_SAFE_INTEGER when no other
  limits are specified (in getValidatedInt).
- No negative ID numbers (in Patient.js).
- Changed columnDefSQL() in dbsqlite.js to use REAL for
  DBCONSTANTS.TYPE_INTEGER and DBCONSTANTS.TYPE_BLOBID. No value conversion
  is required.
- Equivalent change in fieldTypeMatches().
- Removed AUTOINCREMENT tag from PKs (SQLite behaviour doesn't require this).
- Added changeColumnTypes() function.
- Database upgrade changes type of patient ID numbers in patient table.
- On the server (MySQL) side, the fields were

  - INT: -2,147,483,648 to 2,147,483,647 or 4,294,967,295 unsigned (4-byte)
  - and need to be
  - BIGINT: -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
    or 18,446,744,073,709,551,615 unsigned (8-byte)


**Server v1.15, 2014-10-20**

- REQUIRES COMMAND TO UPGRADE EACH DATABASE:
  camcops --maketables /etc/camcops/MYCONFIGNAME.conf
- ID number fields become unsigned BIGINT, not unsigned INT.
  Fixes critical error (inability to represent NHS numbers.)
  See VERSION_TRACKER.txt for the tablet software.


**Client v1.16, 2014-10-26**

- Text-as-button widgets:

  - QuestionBooleanText / props.asTextButton
  - QuestionMultipleResponse / props.asTextButton
  - QuestionMCQ / props.asTextButton

- Reworking of corresponding underlying widget code.
- QuestionDateTime supports text entry (including by default).
- Updated moment.js to 2.8.3
- Minor other code changes and improvement of demo questionnaire.


**Server v1.17, 2014-11-12**

- HAM-D: complained inappropriately about '3' codes (meaning 'not measured')
  for weight questions; maximum score adjusted accordingly from 53 to 52;
  comment for Q16B was erroneously labelled Q16A.


**Client v1.17, 2014-11-13**

- HAM-D scoring was wrong for "weight - not measured" option. Fixed. Maximum
  changed from 53 to 52 accordingly.


**Client v1.2, 2014-11-27**

- Requires server version 1.2.
- WEMWBS/SWEMWBS scales.
- QuestionMCQGrid wasn't centring its buttons properly, because McqGroup wasn't
  copying its incoming tipropsArray through properly.
- Bugfix to webclient database handling, in:

  - dbwebclient.js / convertResponseToRecordList()
  - netcore.js / parseServerReply()

- Some improvements to MobileWeb, though Titanium bugs remain, e.g.:

  - https://jira.appcelerator.org/browse/TC-5065
  - https://jira.appcelerator.org/browse/TC-5071

- GAF: applies 0-100 input constraint.
- GAF: interprets raw score of zero as "unknown" for total-score purposes.


**Server v1.2, 2014-11-28**

- REQUIRES COMMAND TO UPGRADE EACH DATABASE:
  camcops --maketables /etc/camcops/MYCONFIGNAME.conf
- WEMWBS/SWEMWBS tasks.
- GAF: interprets raw score of zero as "unknown" for total-score purposes.
- CGI: requires full completion for a valid total score.
- BPRS total score was erroneously including Q19/Q20.
- Scoring clarity expanded (e.g. BPRS, BPRS-E, CGI).
- Exclude manually erased tasks from list (unless "include old versions" is
  selected). See

  - cc_task.get_session_candidate_task_pks_whencreated()
  - cc_task.get_all_current_pks()

- Bugfix to cc_task.make_extra_summary_tables().


**Server v1.21, 2014-12-04**

- REQUIRES COMMAND TO UPGRADE EACH DATABASE:
  camcops --maketables /etc/camcops/MYCONFIGNAME.conf
- Draft support for RiO metadata export (for RiO's batch document upload
  facility). Some information pending, e.g. whether UTF-8 is supported in
  metadata.


**Client v1.21, 2014-12-26**

- Fixes bug found in v1.17.
  Symptom: crash after adding new patient in some circumstances (?when ID check
  failed). Error of "'undefined' is not an object (evaluating
  'this.props.pages[this.currentPage].pageTag') at Questionnaire.js (line 1)"
  Added getPageTag() function to check for invalid index effects.
- Note in passing: to view iPad-based SQLite files, copy them elsewhere with
  e.g. http://www.macroplant.com/iexplorer/
- Curious crash on loading on an iPad whereas fine under the iOS simulator.
  Occurring in

  - dbinit.js
  - storedvars.databaseVersion.setValue(...)
  - this.dbstore()
  - dbcommon.storeRow()
  - dbsqlite.updateByPK()
  - dbsqlite.getFieldValueArgs()

  Segmentation fault (view console with Xcore > Window > Devices > click the
  tiny up-arrow at the bottom left of the right-hand pane for the device).
  Titanium SDK: 3.5.0.Alpha
  http://builds.appcelerator.com.s3.amazonaws.com/index.html

  ... upgraded to 3.5.0.RC (install SDK + change tiapp.xml)

  ... fixed. So a Titanium bug.


.. _changelog_2015:

2015
~~~~

**Server v1.22, 2015-01-07**

- Improved audit search.


**Client v1.30, 2015-01-30**

- Requires server version 1.3.
- IDED3D task.
- Bug related to serialization of moment() objects from webviews.
  Probably introduced in v1.16.
  The moment.js library now includes a moment.toJSON() function, which
  overrides custom work in my json_encoder_replacer() function. However,
  moment.js's version loses information (specifically, time zone, not to
  mention that it's hard as the recipient to detect whether the object should
  be reconverted to a moment() object.) Therefore:
  preprocess_moments_for_json_stringify()
  ... in conversion.js and taskhtmlcommon.jsx.
- Alerts with large content no longer scroll under iOS 8.
  Apparently this is an Apple bug:
  https://jira.appcelerator.org/browse/TIMOB-17745
- Raphael.js upgraded from 2.1.0 to 2.1.3.
- Bugfix: if endUpload() failed, the failure wasn't processed properly.


**Server v1.30, 2015-01-30**

- REQUIRES COMMAND TO UPGRADE EACH DATABASE:
  camcops --maketables /etc/camcops/MYCONFIGNAME.conf
- IDED3D task.
- Cardinal_ExpectationDetection and Cardinal_ExpDetThreshold: ISO-8601 fields
  changed from TEXT to (internal) ISO8601 (i.e. SQL VARCHAR).
- Prohibit manual erasure of non-finalized (live-on-tablet) tasks (for one
  thing, the tablet might re-upload and surprise the erasing user).
- Manually erased records become non-current.
- Fix latent bug by finalizing special notes along with their tasks.
- Forcible finalization/preservation, with _forcibly_preserved flag.
- Option to drop superfluous columns when remaking tables.
- Bugfix: other filters failed if non-current tasks shown
  (get_session_candidate_task_pks_whencreated).


**Client v1.31, 2015-02-10**

- Requires server version 1.31.
- dbsqlite.renameColumns() and dbsqlite.changeColumnTypes() fail more politely
  with non-existing columns (remember that not all tables may exist, even if
  the app has been launched before, so don't throw an error).
- IDED3D: Minor config text bugfix.
- IDED3D: Save stimulus shapes to database as SVG.
- IDED3D: Occasional missing sounds.
  Reaches "playsound: filename =" message.
  I suspect this is a Titanium bug, but am not certain.
- IDED3D: Correct/incorrect sounds changed to more distinctive chords with
  more similar subjective volumes.
- IDED3D: Change colours for the colour-blind? A/w Annette.


**Server v1.31, 2015-02-10**

- REQUIRES COMMAND TO UPGRADE EACH DATABASE:
  camcops --maketables /etc/camcops/MYCONFIGNAME.conf
- IDED3D task: extra field to store shapes (ided3d.shape_definitions_svg).
- Patient deletion reports tasks that will be deleted.
- Ability to edit patient details, for finalized records.
- HL7 resending triggered by cancelling, not deleting, existing messages
  (in cc_task.Task.delete_from_hl7_message_log(), etc.)


**Server v1.32, 2015-02-15**

- REQUIRES COMMAND TO UPGRADE EACH DATABASE:
  camcops --maketables /etc/camcops/MYCONFIGNAME.conf
- Enforces sensible MySQL engine settings.
- Switches tables to Barracuda format to avoid uploading bug when rows too
  large.


**Server v1.33, 2015-02-19**

- Tweaks to RiO metadata export, based on feedback from Servelec.


**Server v1.34, 2015-03-01**

- Long text (e.g. ProgressNote) crashed PDF generator when in a table.
  Tasks prone to this (ProgressNote, PsychiatricClerking) reworked to avoid
  tables.
- Bug in RecipientDefinition.report_error() fixed.


**Client v1.32, 2015-03-10 to 2015-04-22**

- setRemoteBackup(false) call, to disable back to Apple iCloud; see dbinit.js
- Intermittent crash on Android 4.4.4 (build 23.0.1.A.4.30).
  Relates to database access?

  - Always create all tables at task start. (A crash due to a missing table was
    still possible, and the kind of thing it's easy to miss on a development
    machine that tends to have everything precreated. Mind you, not sure that
    was the actual bug; see next point.) See ensure_database_ok().
  - Explicitly close all recordsets (cursors) opened on all db.execute()
    operations.
  - Did not relate to database access in 10k soak test, and crash occurred
    outside updateByPK function. Maybe relating to visual display. Key error:

    - E/BufferQueue(  292): [org.camcops.camcops/org.appcelerator.titanium.TiActivity] dequeueBuffer: can't dequeue multiple buffers without setting the buffer count

  - This? https://code.google.com/p/android/issues/detail?id=63738
    Android source is:
    https://android.googlesource.com/platform/frameworks/native/+/jb-dev/libs/gui/BufferQueue.cpp
    But crash also occurred inside updateByPK function (unless from a different
    thread).

  - No... relates to setBackgroundImage() calls.
    - https://jira.appcelerator.org/browse/TC-5369
  - Attempt at change:

    - Get rid of all setBackgroundImage() calls for situations calling for
      multiple alternative images (e.g. radio buttons). Also
      setBackgroundSelectedImage().
    - Replace with method of loading all alternative images at the start, and
      using hide()/show() calls.
    - Affects ValidityIndicator; StateRadio; StateCheck.
    - setImage() calls also removed from ImageGroupVertical.
    - Residual setImage() calls, which may also be suspect if the Android file
      system is duff:

      - QuestionCanvas\_\*
      - QuestionImage
      - QuestionPhoto

    - NOT successful. If anything, crashes more frequent.
      Therefore, most likely a memory problem? E.g. ACE-III "learning address"
      page: 26 x QuestionBooleanText, each with up to 4 potential images loaded,
      each ~3k on disk, would give 312k (when image caching would reduce that
      to 12k); might be larger in memory, and if the "imageref_ashmem create
      failed" message is showing the size -- which it is; see
      https://code.google.com/p/skia/source/browse/trunk/src/images/SkImageRef_ashmem.cpp?spec=svn11558&r=11558
      ... then it's about 36k per image, i.e. we were using 3.7 Mb for that page.
      That's then perhaps less surprising.

  - Reverted.
  - New technique

    - imagecache.js
    - Cache cleared from questionnaire.js
    - Applied to ValidityIndicator, StateRadio, StateCheck
      ... except you can't pass Blobs to Titanium.UI.createButton, only to
      createImageView
      ... so ImageView used instead of button for now (which loses the "currently
      being touched" facility). See AS_BUTTONS flag in qcommon.js.
    - However, ImageVerticalGroup goes to preloading method for performance
      reasons.

- Allow user to specify the number of lines used for fixed-height multiline
  text entry: multilineDefaultNLines.


**Client v1.33, 2015-04-26**

- Bugfix: CGI didn't offer all options for Question 3 (drug effects)!


**Client v1.34, 2015-04-26**

- Probable bugfix: IDED3D performed its stage failure check before its stage
  success check at the end of trials (should be the other way round).


**Client v1.40, 2015-05-27**

- Requires server version 1.40.
- FROM-LP framework set menu
- O'Brien group set menu 1
- Brief COPE
- CBI-R
- ZBI (data collection tool only with option for institution to supply text)
- HADS (data collection tool only with option for institution to supply text)
- AUDIT-C
- CGI-I
- Patient Satisfaction Scale
- Referrer Satisfaction Scale (generic + specific)
- Friends and Family Test
- IRAC
- MDS-UPDRS (data collection tool only)
- GDS-15
- AUDIT and AUDIT-C corrected to be clinician-colour pages, and instruction
  page added.
- extrastrings framework - at registration, the tablet downloads sets of extra
  strings from its server. This allows the conversion of crippled tasks to
  fully-functional ones, subject to the hosting institution's right to offer
  the strings up to its tablets (which is a matter for the institution, the
  strings not being distributed with CamCOPS).
- clinician_service field as part of clinician block (and used for service
  feedback); corresponding storedvars.defaultClinicianService variable.
- boldPrompt option to QuestionTypedVariables
- editing_time_s field as standard on all tasks


.. _changelog_2016:

2016
~~~~

**Server v1.40, 2016-01-28**

- From May 2015 to 28 Jan 2016.
- REQUIRES COMMAND TO UPGRADE EACH DATABASE:
  camcops --maketables /etc/camcops/MYCONFIGNAME.conf
- NOTE THAT THE camcops_meta command is now available, e.g.
  `camcops_meta --filespecs /etc/camcops/camcops_*.conf --ccargs maketables`
- Brief COPE Inventory.
- CBI-R.
- ZBI (data collection tool only with option for institution to supply text).
- HADS (data collection tool only with option for institution to supply
  text).
- AUDIT-C
- CGI-I
- Patient Satisfaction Scale
- Referrer Satisfaction Scale (generic + specific)
- Friends and Family Test
- IRAC
- MDS-UPDRS (data collection tool only)
- GDS-15
- DEMQOL
- DEMQOL-Proxy
- Default "respondent" framework, for DEMQOL-Proxy.
- Bugfix to ProgressNote: get_task_html() crashed because "answer" was not
  imported.
- EXTRA_STRING_FILES system, with "get_extra_strings" command to database
  API.
- PHQ-9 database comment fixed for Q10.
- comment_fmt for HADS fields. Note MySQL: SHOW FULL COLUMNS FROM table.
- IES-R (skeleton only).
- WSAS (skeleton only).
- PDSS (skeleton only).
- PSWQ.
- Y-BOCS, Y-BOCS-SC (skeleton only).
- DAD (skeleton only).
- BADLS (skeleton only).
- NPI-Q (skeleton only).
- FRS.
- INECO Frontal Screening (IFS) (skeleton only).
- Add clinician to GAF.
- Diagnosis reports.
- Device report.
- update_multiple_databases.py script
- Unit tests to ensure no overlap for task longnames/shortnames/tables; see
  cc_task.unit_tests().
- clinician_service field as part of clinician block
- xhtml2pdf @page size changed from "a4" to "A4" in cc_html.py to remove
  "WARNING:xhtml2pdf:Unknown size value for @page"; see
  https://github.com/chrisglass/xhtml2pdf/issues/71 ... however, no effect.
- Switch from xhtml2pdf, bypassing Weasyprint, to wkhtmltopdf (via pdfkit) as
  the (default and now only) PDF renderer.
- Abstract base class for PCL tasks wasn't inheriting from object; now is.
- editing_time_s field for all tasks.
- Indexing of ID number fields in patient table.
- Python package format internally.
- Did not implement SVG logos for PDF generation; made files larger not
  smaller. Stick with PNG.
- Remove delayed imports; bug-prone.
  http://stackoverflow.com/questions/744373
  Except in cc_hl7, which imports phq9 for testing (which imports specific
  things from cc_task, which imports cc_hl7).
  ... subsequently revisited; delayed imports now remain only for unit tests,
  where they are more convenient.
- Refresh button for tasks (because browsers keep asking you twice if you hit
  F5).
- EXTRA_STRING_FILES can use globs (in cc_string.py).
- Support MPLCONFIGDIR (default: /var/cache/camcops/matplotlib) to speed up
  matplotlib/pyplot loading.
- Updated to current pythonlib.
- Python build toolchain.
- Moved to Python 3.

  - Of note: comparison of None to int now fails.

- Supplied with Gunicorn, to enable front-end web servers like Apache to
  talk to CamCOPS, and have CamCOPS upgrade/restart, without having to (a)
  restart Apache, or (b) integrate a specific Python version into Apache
  with mod_wsgi. The new system runs in a virtual environment, entirely
  separated (in terms of code) from the front-end web server, communicating
  with it via a private port or Unix socket.
- Disable HTTP client-side caching for added security.
  See also http://codebutler.github.io/firesheep
- Change to relative URL addressing to make that work simply (without having
  to tell CamCOPS where it's mounted).
- ALLOW_INSECURE_COOKIES debugging option.
- Fix nasty bug in rnc_web using "extraheaders=[]" in function signature,
  allowing headers (e.g. cookies) to accumulate over multiple calls (and leak
  across clients). 2016-01-09.
  (But note what is NOT a bug: multiple Chrome "incognito" tabs share each
  other's cookies: https://code.google.com/p/chromium/issues/detail?id=24690)
- Removed the "Tablet device" filter option for tasks (it generates long
  complex-looking lists of IDs and isn't helpful for end users). Removal
  done simply by taking the option out of the form, in
  cc_session.get_current_filter_html().
- New server environment variable options (see instructions.txt):
  - MPLCONFIGDIR
  - CAMCOPS_DEBUG_TO_HTTP_CLIENT
  - CAMCOPS_PROFILE
  - CAMCOPS_SERVE_STATIC_FILES
- Changes to config variables:
  - RESOURCES_DIRECTORY -- removed
  - INTROSPECTION_DIRECTORY -- removed
  - CAMCOPS_LOGO_FILE_ABSOLUTE -- added (optional)
  - MAIN_STRING_FILE -- added (optional)
  - EXTRA_STRING_FILES -- added (optional)
- Added process ID to log output.
- Task counting report.
- Restructure Task/Ancillary classes to be more concise.
- Better unit testing inc. checking for __dict__/fieldname conflicts.
- camcops_meta.py script for e.g. upgrading multiple databases.
- PyMySQL==0.7.1 (upgraded from PyMySQL==0.6.7) to eliminate error on
  inserting BLOBs ("TypeError: can't use a string pattern on a bytes-like
  object").


**Server v1.41, 2016-01-29**

- Bugfix to large research data dumps (were timing out due to inefficient
  SQL). Changed cc_task.get_ancillary_items(), with some back-end functions
  in rnc_db too (changed fetch_all_objects_from_db_where(); added
  create_object_from_list() ).


**Server v1.50, 2016-07-29**

- Change _device VARCHAR(255) fields to _device_id INT.
- Change \*_user VARCHAR(255) fields to \*_user_id INT.
- Note that this leaves only a few "odd" things from the point of standard
  RDBMSs:

  - multiple keys on server side (adding _device_id and _era) to reflect
    multiple devices with write-only access;
  - history on server side (adding _current, and forward/backward PK chain)
  - the "_era" field is textual (ISO-8601), because
    (a) no database seems to store DATETIME values with milli-/microsecond
    accuracy and proper timezone information (in the sense that you can
    recreate the timezone of origin);
    (b) we can use a non-NULL special value, in our case "NOW", as it makes
    things simpler for end users to use "a = b" consistently and not have
    to do "a = b OR a IS NULL AND b IS NULL".
  - patient IDs are unchecked and are allowed to be incomplete (to reflect
    our need to operate with incomplete information, and in anonymous as well
    as fully-identified environments), and duplicate patient records are
    allowed (across, but not within, device/era combinations).
- Static type checking for server Python code.


**Known problems and bugs at the end of the Titanium client**

- visually disabled elements not yet implemented (starting point only in
  qcommon.js)
- wait class imperfect and may leak
- ti.imagefactory module does not support x86 architecture, just arm
- Titanium iOS re-layout is very slow. Visible e.g. when changing questionnaire
  font sizes, but more important for multiline multiline text areas.
  Bug report: https://jira.appcelerator.org/browse/TC-3560
- mobileweb edition not yet working
- Alerts with large content no longer scroll under iOS 8.
  Apparently this is an Apple bug:
  https://jira.appcelerator.org/browse/TIMOB-17745


**Where were version numbers stored in the Titanium client?**

1. App version number is stored in tiapp.xml

2. Tablet's minimum server version requirement is in
   Resources/common/VERSION.js

3. Server version number is stored in server/cc_modules/cc_version.py
   (as is the server's minimum tablet version requirement).

4. Server changelog is stored in server/changelog.debian

5. The web page also has a record of the most recent version, in
   download/index.html

Indirectly:

- Tablet app: Resources/common/VERSION.js reads the app version using
  Titanium.App.version, which is determined by tiapp.xml. In turn,
  it exports this as CAMCOPS_VERSION.
- Tablet build: SHIP_ANDROID reads VERSION.js
- Server build: MAKE_PACKAGE reads cc_constants.py

Human-readable details are shown in this file.


.. _changelog_2017:

Current C++/SQLite client, Python/SQLAlchemy server
---------------------------------------------------

2017
~~~~

**Client v2.0.0 beta**

- Development of C++ version from scratch. Replaces Titanium version.
- Released as beta to Google Play on 2017-07-17.


**Client v2.0.1 beta**

- More const checking.
- Bugfix to stone/pound/ounce conversion.
- Bugfix to raw SQL dump.
- ID numbers generalized so you can have >8 (= table structure change).


**Client v2.0.2 beta**

- Cosmetic bug fixes, mainly for phones, including a re-layout of the ACE-III
  address learning for very small screens.
- Bugfix: deleting a patient didn't deselect that patient.
- Default software keyboard for date entry changed.
- Bugfix for canvas widget on Android (size was going wrong).
- Automatic adjustment for high-DPI screens as standard in `QuBoolean` (its
  image option), `QuCanvas`, `QuImage`, `QuThermometer`.


**Client v2.0.3 beta, 2017-08-07**

- Trivial type fix to patient_wanted_copy_of_letter (String → Bool) in the
  unused task CPFTLPSDischarge.


**Server v2.1.0 beta, 2017-10-17**

- Major changes, including...
- SQLAlchemy for database work
- Group concept
- HOWEVER, HL7 EXPORT AND ANONYMOUS STAGING DATABASE SUPPORT DISABLED;
  further release pending.


**Client v2.0.4 beta, 2017-10-22**

- Bugfix: BLOB FKs weren’t being set properly from `BlobFieldRef` helper
  functions.


**Client v2.0.5 beta, 2017-10-23**

- Bugfix: if the server’s ID number definitions were consecutively numbered,
  the client got confused and renumbered them from 1.


**Server v2.1.1 beta, 2017-10-23**

- Bugfix: WSAS “is complete?” flag failed to recognize the “retired or work
  irrelevant for other reasons” flag.


.. _changelog_2018:

2018
~~~~

**Client v2.2.0 beta, 2018-01-04 to 2018-02-03**

- *To solve the problem of clients and servers being upgraded independently:*
  Reads tables from server during registration (see server v2.2.0). Implemented
  a “minimum server version” option internally for each task (see contemporary
  server changelog). Minimum server version increased from v2.0.0 to v2.2.0.

- Bugfix: adding a new patient from a task list didn’t wipe the task list until
  the patient was re-changed (failure to call `setSelectedPatient` from
  `ChoosePatientMenu::addPatient`; in fact, the patient name details changed
  without changing the underlying patient selection).

- Bugfix: don’t think the patient ID number table was being made routinely
  (!?).

- New :ref:`CIS-R <cisr>` task.

- Internal fix to `DynamicQuestionnaire` to defer first-page creation until
  after constructor.

- Menu additions for CPFT Affective Disorders Research Database.


**Server v2.2.0, 2018-01-04 to 2018-04-24**

- *To solve the problem of clients and servers being upgraded independently:*
  Maintains a minimum client (tablet) version per task; during registration,
  offers the client the list of its tables and the minimum number. This allows
  a newer client to recognize that the server is older and has ‘missing’
  tables, and act accordingly. See
  :func:`camcops_server.cc_modules.client_api.ensure_valid_table_name`. Minimum
  tablet version remains v1.14.0.

- An obvious question: with that mechanism in place, is there any merit to the
  client maintaining a list of minimum server versions for each task? The
  change to the client’s “minimum server version” to 2.2.0 (for client v2.2.0)
  means that future clients will always have the “supported versions”
  information from the server. So, might a client advance mean that it might
  want to refuse old versions of the server, even though the server might be
  happy to accept? (That’s the only situation when a client’s per-table minimum
  server version would come into play.) Well, perhaps it’s possible, even if
  it’s very unlikely (and would probably indicate bad backwards compatibility
  on the client’s part! Let’s implement it for symmetry. Actually, thinking
  further, it might be quite useful: if you upgrade a task and add extra
  fields, using this would potentially allow the client to work with older
  servers unless a specific task is used. Implemented; see client changelog
  above. The default for all tasks is the client-wide minimum server version.

- New report to find patients by ICD-10 or ICD-9-CM diagnosis (inclusion and
  exclusion diagnoses) and age.

- Bugfix where reports would only be produced in HTML format.

- New CIS-R task.


**Server v2.2.1, 2018-04-24 to 2018-06-11**

- Username added to login audit.

- SQLAlchemy `Engine` scope fixed (was per-request; that was wrong and caused
  ‘Too many connections’ errors; now per URL across the application, as it
  should be; see ``cc_config.py``).

- Links to de-identified versions of tasks.

- Group administrators can now change passwords for other users in their group,
  as long as the other user isn't a groupadmin or superuser.

- A released (CPFT) version of 2.2.0 raised a "The resource could not be found"
  error when using the ``/view_groups`` URL, heading to ``groups_view.mako``.

  - Initially: not sure why; development version works fine. No files obviously
    missing. Only that page not working, of all the main menu pages. This was
    as the superuser. The problem was an exception being raised from the
    ``template.render_unicode()`` call in
    ``CamcopsMakoLookupTemplateRenderer.__call__``. Aha -- problem may have
    been a completely full disk. No; disk was completely full, but that wasn't
    the problem.

  - v2.2.1 released just in case I'd missed something.

  - No, it was a problem manifesting in groups_table.mako, which used
    ``u.username for u in group.users`` giving ``AttributeError: 'NoneType'
    object has no attribute 'username'``. Now, that is defined in `Group` as
    ``users = association_proxy("user_group_memberships", "user")``.

  - The problems looks to be in the data: there was an entry in the
    ``_security_user_group`` table with user_id = NULL (and group_id = 3).

  - *Not yet sure where that duff value came from.* Template updated to cope
    with the problem, regardless. (Perhaps the value came from an earlier
    version of ``merge_db.py``?)


**Server v2.2.2, 2018-06-19**

- Fixed bug in Diagnosis report for non-superusers (see
  :meth:`camcops_server.tasks.diagnosis.get_diagnosis_inc_exc_report_query`);
  the exclusion "where" restriction was being applied wrongly and joining the
  exclusion query to the main query, giving far too many rows.


**Client v2.2.1 beta, 2018-08-06**

- Background striping for the `QuMcqGrid*` classes.

- Bugfix: `android:allowBackup="false"` added back to AndroidManifest.xml


**Client v2.2.3, server v2.2.3, 2018-06-23**

- :ref:`Khandaker/Insight medical history <khandaker_insight_medical>` task.

- Client requires server v2.2.3. (Was a global requirement; should have been
  task-specific. REVERTED to minimum server version 2.2.0 in client 2.2.6.)


**Client v2.2.4, 2018-07-18**

- Bugfix to Android client for older Android versions.

  - On startup, CamCOPS was crashing with "Unfortunately, CamCOPS has stopped."
    on older Android versions (e.g. 4.4.x).

  - The USB debugging stream showed: ``java.lang.UnsatisfiedLinkError: dlopen
    failed: could not load library "libcrypto.so.1.1" needed by
    "libcamcops.so"; caused by library "libcrypto.so.1.1" not found``.

  - Thoughts: see comments in ``changelog.rst``.

    .. Thoughts:
      - We were adding ``libcrypto.so`` and ``libssl.so`` (as part of the Qt
        Creator Build Settings). This used to work but now doesn't, presumably due
        to a change in Qt Creator. (The files were being packaged; try copying the
        ``.apk`` file and unzipping it.) The original files are symlinks to
        ``libcrypto.so.1.1`` and ``libssl.so.1.1``. Adding the ``*.1.1`` files via
        ``ANDROID_EXTRA_LIBS`` in ``camcops.pro`` is prohibited (the packaging
        process complains about files that are not ``lib*.so``). In
        ``libcamcops.so`` there are references to ``libcrypto.so.1.1``, but that
        file is missing.
    ..
      - Others have noticed this or a similar problem:
    ..
        - https://forum.qt.io/topic/35847/qt5-2-androiddeployqt-openssl-library-versioning (2013)
        - https://bugreports.qt.io/browse/QTCREATORBUG-11237
        - https://bugreports.qt.io/browse/QTCREATORBUG-11062
        - https://bugreports.qt.io/browse/QTBUG-47065
    ..
      - No change after manually deleting the build directory (not just cleaning)
        and rebuilding.
    ..
      - So, another way round: why is ``libcamcops.so`` asking for a versioned
        library? It turns out that the problem is that OpenSSL was built with
        versioned libraries.
    ..
        Instructions at http://doc.qt.io/qt-5/opensslsupport.html suggest using
        ``make CALC_VERSIONS="SHLIB_COMPAT=; SHLIB_SOVER=" build_libs`` when
        building OpenSSL for Android, also as per
        https://stackoverflow.com/questions/24204366/how-to-build-openssl-as-unversioned-shared-lib-for-android,
        but that is for an older version of OpenSSL (e.g. 1.0.2d). Lots of things
        failed, but I succeeded by automatically editing the Makefile in a hacky
        way. See changes in :ref:`build_qt`. We now have unversioned libraries for
        Android.
    ..
      - I'm less clear what changed as it was working well beforehand!
    ..
        - In retrospect: may have been changing the Android API level from 23 to
          28.
    ..
      - Then we got this crash: ``java.lang.UnsatisfiedLinkError: dlopen failed:
        cannot locate symbol "EVP_MD_CTX_new" referenced by "libcamcops.so"...``.
        Deleted ``qmake`` and rebuilt via
        ``build_qt.py --build_android_arm_v7_32``. Didn't build for Android
        (``undefined reference to WebPInitAlphaProcessingNEON``). Upgraded Qt to
        5.11.1. Built fine (Linux, Android). Same crash. But as before,
        libcrypto.so and libssl.so are being loaded.
    ..
      - We are using android-ndk-r11c (March 2016); Qt's preference is 10e (May
        2015) (as per http://doc.qt.io/qt-5/androidgs.html). The current version
        (as of 2018-07-04) is r17b (June 2018); see
        https://developer.android.com/ndk/downloads/. Still, 11c has worked
        throughout.
    ..
      - I suspect the root cause is approximately as follows.
    ..
        - At present, the Qt build uses dynamic linking to OpenSSL. (That's why
          the Linux version needs to find libcrypto.so and libssl.so.)
    ..
        - In the Linux build of CamCOPS, OpenSSL is included statically. (That's
          why direct calls from cryptofunc.cpp to EVP* calls work.)
          (Certainly,
          ``objdump -t build-camcops-CustomLinux-Debug/camcops | grep EVP`` shows a
          bunch of stuff, and ``EVP_MD_CTX_new`` is present for ``objdump -T`` as
          well as ``objdump -t``, as a real function.)
    ..
        - In the Android build, CamCOPS is built to a library, libcamcops.so.
          Presumably that's why it can build without OpenSSL EVP* functions in it,
          without complaining. But then it needs OpenSSL functions via DLL?
          Certainly, ``objdump -t
          build-camcops-Android_ARM-Release/android-build/libs/armeabi-v7a/libcamcops.so``
          shows no symbols. However, ``strings`` shows EVP stuff, and ``objdump
          -T`` shows ``EVP_MD_CTX_new`` as ``DF *UND* ... OPENSSL_1_1_0
          EVP_MD_CTX_new``.
    ..
        - See also
          https://stackoverflow.com/questions/32737355/elf-dynamic-symbol-table.
    ..
        - OK, so we need to deal with the DLL zone. Dealt with. Runs on Linux with
          DLL mode and without; see OPENSSL_VIA_QLIBRARY.
    ..
      - No, perhaps I was wrong, because:
    ..
        - Now we get ``java.lang.UnsatisfiedLinkError: dlopen failed: cannot locate
          symbol "HMAC_CTX_new" referenced by "libcamcops.so"``. So that's
          progress. But ``HMAC_CTX_new`` isn't in my source code. If we do
          ``objdump -T libcamcops.so | grep OPENSSL_1_1_0``, we get
    ..
          .. code-block::
    ..
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 OBJ_nid2sn
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 EVP_CIPHER_CTX_new
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 EVP_CIPHER_iv_length
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 EVP_CIPHER_CTX_free
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 EVP_CipherInit_ex
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 EVP_CIPHER_key_length
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 EVP_sha512
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 RAND_bytes
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 EVP_aes_256_cbc
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 EVP_CIPHER_nid
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 EVP_CIPHER_block_size
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 EVP_CipherFinal_ex
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 HMAC_CTX_new
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 HMAC_Update
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 PKCS5_PBKDF2_HMAC_SHA1
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 HMAC_Final
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 HMAC_CTX_free
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 HMAC_Init_ex
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 EVP_get_cipherbyname
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 RAND_add
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 EVP_sha1
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 EVP_CIPHER_CTX_set_padding
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 EVP_CipherUpdate
            00000000      DF *UND*	00000000  OPENSSL_1_1_0 EVP_MD_size
    ..
          So possibilities include that I'm calling some of these inadvertently by
          using types within cryptofunc.cpp; but more likely that sqlcipher is
          calling them. We're not going to get far this way; the explicit DLL
          approach was probably silly. Instead, see
          https://stackoverflow.com/questions/25147714/qt-openssl-android and note
          that we may need to insert into ``AndroidManifest.xml`` the following:
    ..
          .. code-block:: xml
    ..
            <meta-data android:name="android.app.load_local_libs" android:value="-- %%INSERT_LOCAL_LIBS%% --:lib/libssl.so:lib/libcrypto.so"/>
            Note this bit:                                                                                  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    ..
          No, that didn't work. We ended up with two copies of the libraries, in
          "...camcops" and "...camcops-1", but it didn't fix the problem. Perhaps
          we need both static linkage (for CamCOPS internal calls to OpenSSL,
          including SQLCipher) and dynamic linkage (for the parts of Qt that use
          OpenSSL). Changes made to ``camcops.pro``. No, that didn't work either;
          doesn't link (missing e.g. ``signal``). See
          https://stackoverflow.com/questions/37122126/whats-the-exact-significance-of-android-ndk-platform-version-compared-to-api-le;
          perhaps this is all down to a change in the Qt setting for Android NDK
          level, from 23 to 26, without a change in the OpenSSL Android NDK build
          level.
    ..
          Not yet explored:
    ..
          - https://github.com/openssl/openssl/issues/3826
          - Note that SQLCipher may be moving from OpenSSL to mbedTLS:
            https://github.com/praeclarum/sqlite-net/issues/597
          - https://stackoverflow.com/questions/25049603/dlopen-failed-cannot-locate-symbol-signal?rq=1
    ..
          Trying NDK 10e (rather than 11c):
    ..
          - Download and unzip to ~/dev/
          - Change ``build_qt.py`` default.
          - In Qt Creator, change :menuselection:`Tools --> Options --> Devices --> Android --> Android Settings --> Android NDK location."
          - ABANDONDED/REVERTED; see below.
    ..
          Aha. It's this, perhaps:
    ..
          - https://android-developers.googleblog.com/2016/06/android-changes-for-ndk-developers.html
          - So, must be API 23 or lower, or dlopen() calls will fail, which is
            exactly what we're seeing.
    ..
          The Sony tablet is Android 4.4.2 (API level 19), and fails; my Samsung
          phone is Android 6.0.1 (API level 23) and works fine. So it is something
          about the Android API. So, checking
          https://wiki.qt.io/Qt_for_Android_known_issues: nothing obvious. But
          upgrading the Sony Xperia Z2 tablet (to 6.0.1, the next available) made
          the same binaries work.

  - Upshot: Android API 19 (Android 4.4.x) no longer works. API level 23
    (Android 6.0.1) is fine; intermediates untested. It's a little unclear
    what's changed (unless I was just behind on testing for old versions of
    Android and the problem had been there for a while). One possibility was
    that the shared OpenSSL libraries were being compiled for ``android-23``
    (as per :ref:`build_qt`) and that was not the same as ``minSdkVersion`` in
    ``AndroidManifest.xml``. The problems are explained well at
    https://stackoverflow.com/questions/21888052/what-is-the-relation-between-app-platform-androidminsdkversion-and-androidtar/41079462#41079462,
    where APP_PLATFORM is equivalent to the API version used by :ref:`build_qt`
    to compile OpenSSL etc.

  - The upshot from that article is that libraries compiled with the Android
    NDK (like OpenSSL in our case) must be compiled with for the same SDK
    version (``APP_PLATFORM``) as ``minSdkVersion``.

  - We were using ``minSdkVersion="16"``, so I tried setting
    ``DEFAULT_ANDROID_API_NUM = 16`` in :ref:`build_qt`, and recompiling for
    Android using ``build_qt.py --build_android_arm_v7_32``, continuing to use
    NDK r11c. I moved ``targetSdkVersion`` back to 26 (soon to be the Google
    Play minimum). This works on Android 6.0.1 (API 23, using debug mode).
    However, it still crashes (as above) with Android 4.4.x (API 18).
    As of Feb 2018, about 58% of Android in the wild is API 23 or higher
    (https://en.wikipedia.org/wiki/Android_version_history), and about 82% is
    API 21 and higher. It is certainly better to fail to run than to crash, so
    let's say that we will set API 23 (Android 6.0) as the minimum for now.


**Server v2.2.4, 2018-06-29**

- Update to libraries:

  - alembic from 0.9.6 to 0.9.9
  - cardinal_pythonlib from 1.0.16 to 1.0.18
  - colorlog from 3.1.0 to 3.1.4
  - CherryPy from 11.0.0 to 16.0.2
  - deform from 2.0.4 to 2.0.5
  - distro from 1.0.4 to 1.3.0
  - dogpile.cache from 0.6.4 to 0.6.6
  - gunicorn from 19.7.1 to 19.8.1
  - matplotlib from 2.1.0 to 2.2.0
  - mysqlclient from 1.3.12 to 1.3.13
  - numpy from 1.13.3 to 1.14.5
  - pendulum from 1.3.0 to 2.0.2
  - pyramid from 1.9.1 to 1.9.2
  - pyramid_debugtoolbar from 4.3 to 4.4
  - python-dateutil from 2.6.1 to 2.7.3
  - pytz from 2017.2 to 2018.5
  - scipy from 1.0.0rc1 to 1.1.0
  - sqlalchemy from 1.2.0b2 to 1.2.8
  - typing from 3.6.2 to 3.6.4

- Bugfix to SQLAlchemy/Alembic handling, such that tables are always created
  with ``CHARSET utf8mb4 COLLATE utf8mb4_unicode_ci`` rather than the erroneous
  ``COLLATE utf8mb4_unicode_ci CHARSET utf8mb4``. See :ref:`MySQL: Illegal mix
  of collations <mysql_illegal_mix_of_collations>`.


**Server v2.2.5, 2018-07-23**

- Python package: ``camcops-server``.


**Server and client v2.2.6, 2018-07-26**

- Logic bugfix and improved clarity in client ``Task::isTaskUploadable``.

- Client minimum server version returned to 2.2.0 (from 2.2.3); specific
  Khandaker1MedicalHistory requirement of 2.2.3 added.

- Fixed inadvertently broken server: the ``upgrade_db`` command defaulted to
  showing SQL only, not doing the job!

- BDI shows alert for non-zero suicidality question.

- BDI shows custom somatic symptom score (Khandaker Insight study) for BDI-II.

- BDI shows question topics (taken from freely available published work),
  though no task content is present.

- Added missing server string ``camcops/data_collection_only``.

- ``CssClass`` constants.

- CISR client now shows more detail in its summary view.

- Bugfix to CISR client logic; code fallthrough for
  CONC3_CONC_PREVENTED_ACTIVITIES.

- Client returns to maximized mode after returning from fullscreen, if it was
  maximized before.

- Client calls ``ensurePolished()`` for ``sizeHint()`` functions of widgets
  containing text, which should make initial sizing more accurate.

- Fix to fullscreen modes under Windows (see ``compilation_windows.txt``).

- Whisker test task (2018-08-15).

- Windows distribution (2018-08-16).


**Server v2.2.7, 2018-07-31**

- Bugfix relating to offset-naive versus offset-aware datetimes in
  ``cc_user.SecurityLoginFailure.clear_dummy_login_failures_if_necessary``.


**Client v2.2.7, 2018-08-17**

- Bugfix to CISR: field ``sleep_gain3`` was missing from field list.

- Search facility for all-tasks list.

- OS information.


**Client v2.2.8 to 2.3.0 (from 2018-09-10)**

- Bugfix to CISR client: page colour was clinician, now patient.

- Bugfix to PHQ9: question 10 was still mandatory in the Questionnaire
  even if zero score for other questions.

- Client moved from Qt 5.11.1 to Qt 5.12.0 (2018-09-24).

  - Code changes: ``tablet_qt/layouts/flowlayout.cpp`` temporarily switches off
    ``-Werror=missing-field-initializer`` warning which arises from
    ``qcborvalue.h`` when including ``#include <QtWidgets>``; this is
    https://bugreports.qt.io/browse/QTBUG-68889. The compiler was ``g++`` from
    GCC 4.9, part of Android NDK r11c. We disable with ``#pragma GCC diagnostic
    ignored "-Wmissing-field-initializer"``

  - Checked for Linux, Android; Windows checks pending.

- "Page jump" button only shown in questionnaires if (allowed and) there is
  more than one page, or the questionnaire is dynamic.

- New variant on ``QuBoolean``/``BooleanWidget`` to display "false as blank".
  Used in FACT-G task.

- ``QuPage::indexTitle()`` for different titles (if desired) on the page jump
  index to the heading at the top of the page.

- Markedly improved error messages when you aim the client at a web server but
  not the CamCOPS client API.

- Rounding of DPI prior to icon sizing (we were using e.g. 96.0126 DPI, which
  is probably the system reporting inaccurately).

- ID number validation system and NHS number validation.

- Removed all defunct preprocessor references to
  ``LIMIT_TO_8_IDNUMS_AND_USE_PATIENT_TABLE``,
  ``DUPLICATE_ID_DESCRIPTIONS_INTO_PATIENT_TABLE``, and
  ``ALLOW_SEND_ANALYTICS``.

- ID policy supports "NOT" and other new tokens; see server changelog.

  If an old client is used with a new server, the server may offer "invalid"
  policies (as seen by the old client); these will refuse uploads, as per
  ``IdPolicy::complies()``. If a new client is used with an old server, there
  should be no problem.

- ``CardinalExpDetThreshold`` was missing ``ancillaryTables()`` and
  ``ancillaryTableFKToTaskFieldname()``.

- Turn off patient ID information in debug stream for ``MenuItem``,

- Add network status messages to debug stream.

- **New task:** :ref:`CORE-10 <core10>`.

- **New task:** :ref:`CESD <cesd>`.

- **New task:** :ref:`CESD-R <cesdr>`.

- **New task:** :ref:`PTSD Checklist for DSM-5 (PCL-5) <pcl5>`.

- **New task:** :ref:`FACT-G <factg>`.

- **New task:** :ref:`EQ-5D-5L <eq5d5l>`.

- Client validates patients with the server on upload. This supports future
  "predefined patient" support. This is a "client asks", not "server tells"
  feature at present.

- Version bumped to 2.3.0. If server is at least 2.3.0, uses the new "validate
  patients on upload" feature (2018-11-13). Minimum server version remains at
  2.2.0.

- Word wrap on for log box by default (better legibility in upload).

- Since the server can now report PID when providing error messages (patients
  that don't validate), the "upload" function is now restricted to unlocked
  devices.

- Databases were not being vacuumed (call was being made after database thread
  had been shut down). Fixed.

- Fixed bug: patient was not deselected (in ``NetworkManager::uploadNext()``)
  with a "copy" upload, but that failed to take account of patients/tasks
  marked as "individually finished". Now always deselected (also triggers
  refresh of anonymous task list).

- ProgressNote now reports itself as incomplete if the note is empty, in
  addition to if it is NULL. Corresponding change on the server.

- Bug found in upload process relating to BLOB upload in the "per-record"
  fashion. Specifically, when the client set the ``_move_off_tablet`` flag on
  a BLOB (in ``NetworkManager::applyPatientMoveOffTabletFlagsToTasks()``), it
  then asked the server "which records to send?" via
  :func:`camcops_server.cc_modules.client_api.op_which_keys_to_send`. This
  took account of actual modifications, but not changes to the
  ``_move_off_tablet`` flag; so the record wasn't resent; so older client BLOBs
  that were not being modified in that upload were not correctly marked as
  preserved. Solution: new ``TabletParam.MOVE_OFF_TABLET_VALUES`` parameter to
  this command. Modifications to ``NetworkManager::requestRecordwisePkPrune()``
  and, on the server,
  :func:`camcops_server.cc_modules.client_api.op_which_keys_to_send`. To make
  this safe retrospectively, the server insists on all records being sent if
  this field is not present in the request.


**Server v2.2.8 to 2.3.0 (2018-09-14 to 2018-11-26)**

- ``GROUP_NAME_MAX_LEN``, ``DEVICE_NAME_MAX_LEN`` and ``USER_NAME_MAX_LEN``
  changed from 255 to 191 because MySQL<=5.6 only supports indexing of 191
  characters in ``utf8mb4`` mode on ``InnoDB`` tables;
  see https://dev.mysql.com/doc/refman/5.7/en/charset-unicode-conversion.html

- Shebang changed for ``build_qt.py``

- SQLAlchemy ``NAMING_CONVENTION`` changed in ``cc_sqlalchemy.py`` as some
  fields were yielding index/constraint names that were too long... then
  reverted and specific changes made for
  ``cpft_lps_discharge.management_specialling_behavioural_disturbance``.

- Removed introspection options; replaced with better docs.

- Documentation now at https://camcops.readthedocs.io/.

- ``cardinal_pythonlib`` to 1.0.38

- ``alembic`` to 1.0.0

- ``create_database_migration.py`` checks the database version is OK first.

- Make Alembic compare MySQL ``TINYINT(1)`` to be equal to ``Boolean()`` in the
  metadata, so its default suggestions are more helpful.

- ``CherryPy`` from 16.0.2 to 18.0.1, but this did not fix
  https://github.com/cherrypy/cherrypy/issues/1618. However, it is a non-fatal
  error; just carry on.

- Better server docstrings.

- All summary tables report the CamCOPS server version that calculated the
  summary, in the field ``camcops_server_version``.

- If no extra string files at all are found, the server aborts.

- Typo fixed in demo Apache config re Unix domain sockets (inappropriately
  had "https" and a trailing slash).

- Upload API: improved
  :func:`camcops_server.cc_modules.client_api.upload_record` to use
  :func:`camcops_server.cc_modules.client_api.upload_record_core`, in common
  with :func:`camcops_server.cc_modules.client_api.upload_table`.

- Bugfix to MOCA server display: trail picture was shown twice, clock picture
  not at all.

- Probable bugfix to code that handles very old tablet versions, now moved to
  :func:`camcops_server.cc_modules.client_api.process_upload_record_special`.
  Code was unlikely to trigger; comparison of a Table to a tablename would have
  failed.

- ID number validation system and NHS number validation.

- ID policy supports ``NOT``, ``address``, ``gp``, ``otherdetails``, and
  ``otheridnum``; see :ref:`patient identification <patient_identification>`.

  This makes it easier for research studies to support a "no PID" rule, as a
  per-group setting.

- Bugfix to validation colours for ``groups_table.mako``.

- Group admin facility to list all users' e-mail addresses with ``mailto:``
  URLs.

- Server renamed from ``camcops`` to ``camcops_server``; package, executable,
  etc. Similarly ``camcops_meta`` to ``camcops_server_meta``.
  **Note that this may break automatic launch scripts, e.g. via supervisord;
  you should review these.**

- Added dependency ``bcrypt==3.1.4`` to ``setup.py``.

- :meth:`camcops_server.cc_modules.cc_config.CamcopsConfig.get_dbsession_context`
  re-raises exceptions.

- Better upgrade/downgrade database facilities for developers.

- Task report was claiming to slice by creation date but was slicing by
  addition (upload) date; fixed (to creation date).

- Task index. See ``cc_taskindex.py``, with corresponding changes in e.g.
  ``cc_taskcollection.py`` and ``client_api.py``. Significant speedup on the
  server.

  - Design note: we should not have a client-side index that gets uploaded.
    This would be a bit risky (trusting clients with the server's index); also,
    the client's index couldn't use server PKs (which we'd want); etc.

- Upload speedup by optimizing existing upload method and via new one-step
  upload.

- Fixed bug where predecessor records of individually-preserved client records
  were not themselves preserved properly.

- Documentation of structured upload testing method in ``client_api.py``
  (q.v.).

- Improvements to Debian/RPM packaging, including use of ``venv`` from the
  Python 3.3+ standard library rather than ``virtualenv``.


.. _changelog_2019:

2019
~~~~

**Server v2.3.1 and client v2.3.1 (2018-11-27 to 2019-03-24)**

- ``cardinal_pythonlib`` to 1.0.49.

  - Fixes misconversion of previous 24-hour filter times to their morning
    equivalents, in the task filter view. To test, set e.g. a start time of
    01:30 and an end time of 23:30; save the filter; re-edit the filter and
    re-save it; check the end time stays correct.

  - Improved e-mail handling, pro tem.

  - For ``build_qt.py`` under Windows, implement a directory change via
    Python and not ``tar`` for the "untar" operation.

  - Request-logging middleware.

- Fixed trivial bugs and added clarity about item sequencing.

  - The bug was: PhotoSequence used zero-based numbering for the ``seqnum``
    field until something was re-ordered, at which point it went to one-based
    numbering (``renumberPhotos()`` versus ``addPhoto()``). The server assumed
    zero-based numbering. Similarly in the diagnosis tasks (``renumberItems()``
    versus ``addItem()``).

  - Regardless of the mathematical or computing merits, our experience of
    research users is that they are more far comfortable with one-based
    numbering. (Both 0-based and 1-based approaches are clearly possible. A
    nice essay by van Glabeek, 1999, in support of 1-based numbering is "Do we
    count from 0 or from 1? The ordinal use of cardinal expressions" at
    http://kilby.stanford.edu/~rvg/ordinal.html. Part of his point is that the
    ambiguity arises when we move from an ordinal, "1st", to a cardinal, "1".
    Dijkstra's more famous 1982 argument for 0-based numbering, at
    http://www.cs.utexas.edu/users/EWD/transcriptions/EWD08xx/EWD831.html, is
    pretty weak for this purpose.)

  - So, we will use **one-based numbering for sequence numbers of database
    objects** for all future tasks and as the decision for previously
    inconsistent tasks. We will allow zero-based numbering to persist for older
    specialist tasks. Changes therefore as follows:

  - ``photo.py``: clarified column comment to make 1-based numbering explicit;
    HTML display now uses ``seqnum`` not ``seqnum + 1``.

  - ``photosequence.cpp``: fixed bug in ``addPhoto()`` so it uses 1-based
    numbering

  - ``diagnosis.py``: clarified column comment to make 1-based numbering
    explicit; HTML display now uses ``seqnum`` not ``seqnum + 1``.

  - ``diagnosistaskbase.cpp``: fixed bug in ``addItem()`` so it uses 1-based
    numbering

  - ``cardinal_expdetthreshold.py``: no change except cosmetically to clarify
    zero-based trial numbering; not worth changing

  - ``cardinalexpdetthreshold.cpp``: no change; continues with zero-based
    trial numbering; not worth changing

  - ``cardinal_expectationdetection.py``: no change except cosmetically to
    clarify zero-based trial numbering; not worth changing

  - ``cardinalexpectationdetection.cpp``: no change; continues with zero-based
    trial numbering; not worth changing

  - ``ided3d.py``: was already happily using 1-based numbering with clear
    database/XML comments

  - ``ided3d.cpp``: was already happily using 1-based numbering in the
    database, and maintaining clearly labelled 0- and 1-based numbering for
    internal purposes (e.g. ``ided3dtrial.h``; ``ided3dstage.h``).

- SQL Server support.

  - Bugfixes for operation under SQL Server.

  - **The minimum SQL Server version is 2008** (below that, there’s no time
    zone conversion support; see
    :func:`camcops_server.cc_modules.cc_sqla_coltypes.isotzdatetime_to_utcdatetime_sqlserver`).

  - SQL Server testing: see :ref:`Windows 10 specimen installation
    <server_installation_win10_specimen>`.

  - Fully operational **except** ``upgrade_db`` command triggers reindexing for
    revision ``0013`` and that executes a ``DELETE`` query that gets stuck.
    Trigger problem. See :ref:`DELETE takes forever
    <sql_server_delete_takes_forever>`. The ``create_db`` command works fine,
    and so does manual reindexing, but this remains a problem.

- SNOMED-CT support.

  - For copyright reasons, SNOMED-CT codes for tasks held in a separate file
    and cross-referenced by arbitrary strings (not the codes themselves).

  - For ICD-9-CM and ICD-10 codes, we preconvert them from an Athena OHDSI
    data set (if the user is permitted to use that).

  - Command-line options for the client to print its ICD diagnostic codes.
    These are then added to the server, hugely reducing the number of codes
    we need to cache (e.g. ICD-9-CM: from 7,911 to 573; ICD-10: from 199,611 to
    3,318).

    - Internal design note: creating a DiagnosticCodeSet requires xstrings
      from the CamcopsApp for its descriptions. Options are therefore (1) defer
      code printing until the database is open (but that means security checks
      required!); (2) have all calls to CamcopsApp::xstring() return blanks
      until the database is open (but then an overhead for everything); (3)
      have DiagnosticCodeSet not ask for xstrings if it's being created in "no
      xstring" mode. Went with (3).

- Config file documentation moved from demo file to docs.

- ``cherrypy`` from 18.0.1 to 18.1.0, to fix
  https://github.com/cherrypy/cherrypy/issues/1618; nope, still not fixed. Must
  be soon...

- ``Pygments==2.2.0`` to ``Pygments==2.3.1``, in the hope that it fixes some
  C++ lexing failures that work in the online Pygments demo
  (http://pygments.org/). Nope...

- Removed relative imports, as per PEP8 and
  https://stackoverflow.com/questions/4209641/absolute-vs-explicit-relative-import-of-python-module.

- Log tidy-up (for delayed evaluation via ``BraceStyleAdapter``).

- Suppress ``wkhtmltopdf`` output with ``--quiet`` option; see
  :data:`camcops_server.cc_modules.cc_constants.WKHTMLTOPDF_OPTIONS`.

- CardinalExpDet task complains less when drawing graphs with missing data.

- Shift to Python ``csv`` module for generating TSVs, using the ``excel-tab``
  dialect. This works well.

- Bugfix: newlines were not being unescaped properly on receipt from the
  client (they were remaining in the database as escaped two-character ``\n``
  strings). Call to ``unescape_newlines()`` added to
  :func:`camcops_server.cc_modules.cc_convert.decode_single_value`. This
  function reverses ``convert::toSqlLiteral()`` in the client.

- Substantially improved export facilities, including whole-database export,
  push exports, and e-mail exports.

  - Design decision: keep details in config file, or shift to web-based
    configuration?

    - Unimportant: need to launch from command line. (Could launch via a named
      database record.) Not a factor.

    - Less important: more work in writing/managing web-based configuration.
      Fractional point for config file.

    - Relevant: configuration on the fly? This is dangerous but could be
      useful. However, the only thing likely to be edited is a destination
      e-mail address. In general, export configuration feels like a slightly
      high-risk thing to have editable only, even by a web-based superuser;
      for example, it allows the specification of arbitrary shell scripts, and
      if this were done online, the editing user might be unable to check that
      filename. (Moreover, an editing user who is using ssh might find it
      inconvenient to use a web interface.) Point for config file.

    - Relevant: audit trail. We want the export log to be able to refer to an
      export config. We probably want a true record of the export config used.
      However, we don't want to duplicate thousands of config records (e.g. the
      same config being run once per minute for years).

      - Solution for database: create a new record when an export config is
        changed; have run records refer to them by PK.

      - Solution for config file: create a new record when an export config is
        changed... have run records refer to them by PK...

    - Decision: config file, with snapshot copied to database for auditing.

  - **Breaking changes:**

    - ``[server]`` config file section renamed to ``[site]``.
    - Python web server options moved from command-line to config file, in a
      new ``[server]`` section.
    - ``[recipients]`` config file section renamed ``[export]``
    - ``HL7_LOCKFILE`` changed to a broader ``EXPORT_LOCKDIR`` system and moved
      from the ``[server]`` to the ``[export]`` section
    - Then other changes to the actual export definitions (see docs for the
      :ref:`server config file <server_config_file>`), each in sections named
      ``[recipient:XXX]`` in the config file.
    - Database drops old HL7-specific tables and adds a new set of export
      tables (also: more extensible for future methods).

- ``QuSlider`` takes a new ``setSymmetric()`` option to remove the colour to
  the left of (horizontal) or below (vertical) the slider "handle".

- ``Questionnaire`` takes ``QuPage*`` as well as ``QuPagePtr`` as arguments to
  its constructor.

- ``TickSlider`` and ``QuSlider`` allow their labels to overspill the edges
  and therefore work much better.

- ``QuElement`` supports an alignment parameter and all layouts (e.g.
  ``QuPage``, ``QuFlowContainer``) respect this and sometimes add additional
  options.

- Bugfix regarding Alembic.

  - ``alembic==1.0.0`` to ``alembic==1.0.7``, in the hope it gets constraint
    names right. Made no difference, so onwards:

  - The problem materializes when MySQL's 64-character limit on constraints
    (the same as as for other identifiers) is exceeded.

  - A prototypical problem is the table ``cpft_lps_discharge`` and its field
    ``management_specialling_behavioural_disturbance``, defined as a CamCOPS
    ``BoolColumn("management_specialling_behavioural_disturbance",
    constraint_name="ck_cpft_lps_discharge_msbd")``.

  - The :class:`camcops_server.cc_modules.cc_sqla_coltypes.BoolColumn` class
    sets its ``type_`` parameter, effectively, to
    ``Boolean(name=conv(NAME_PASSED))``.

  - The :func:`conv` function, which is :func:`sqlalchemy.sql.elements.conv`,
    is meant to mark the string as already having been converted via a naming
    convention. It's documented at
    https://docs.sqlalchemy.org/en/latest/core/constraints.html#sqlalchemy.schema.conv.

  - When we ask SQLAlchemy to make the table directly, via ``camcops_server
    create_db``, it issues the constraint as ``CONSTRAINT
    ck_cpft_lps_discharge_msbd CHECK
    (management_specialling_behavioural_disturbance IN (0, 1))``.

  - So far, so good.

  - Alembic is aware of the metadata and its naming convention via CamCOPS's
    ``env.py``.

  - When Alembic is called via ``camcops_server upgrade_db``, it sees this
    column as ``Column('management_specialling_behavioural_disturbance',
    Boolean(name='ck_cpft_lps_discharge_msbd'), table=None)``.

    - We established this by temporarily editing
      :meth:`alembic.operations.ops.CreateTableOp.create_table`.

  - The resulting SQL constraint is ``CONSTRAINT
    ck_cpft_lps_discharge_management_specialling_behavioural_disturbance CHECK
    (management_specialling_behavioural_disturbance IN (0, 1))``.

  - Note that our naming convention,
    :data:`camcops_server.cc_modules.cc_sqlalchemy.NAMING_CONVENTION`, contains
    ``"ck": "ck_%(table_name)s_%(column_0_name)s"``.

  - So the Alembic-generated SQL uses our naming convention, and "neither
    Alembic nor SQLAlchemy currently create names for constraint objects where
    the name is otherwise unspecified"
    (https://docs.sqlalchemy.org/en/latest/core/constraints.html#configuring-constraint-naming-conventions),
    so it's not likely to be coming from anywhere else.

  - The bug looks like Alembic is ignoring the :func:`conv` indicator.

  - This is with ``alembic==1.0.0`` or ``alembic==1.0.7``.

  - Searching the Alembic code for ``conv`` and then ``if conv`` leads to
    ``operations/base.py`` which contains ``op.f``. This appears to be what
    we want:
    https://alembic.sqlalchemy.org/en/latest/ops.html#alembic.operations.Operations.f.

  - It is likely that the file of interest, ``0001_start.py``, was created
    before SQLAlchemy 0.9.4, when ``op.f`` became part of autogenerated output
    (according to the Alembic docs).

  - So the solution: add ``op.f`` to relevant parts of ``0001_start.py``.
    Find relevant columns in the source by searching for ``constraint_name=``.
    Yup! That fixes it. When all are fixed, there should be an equal number of
    ``sa.Boolean(name=op.f(`` lines.

  - An example in the correct format from ``0001_start.py`` is therefore
    ``sa.Column('management_specialling_behavioural_disturbance',
    sa.Boolean(name=op.f("ck_cpft_lps_discharge_msbd")), nullable=True)``.

  - Also renamed the constraint on
    ``deakin_1_healthreview.willing_to_participate_in_further_studies`` from
    ``wtpifs`` to ``ck_deakin_1_healthreview_wtpifs`` to match our convention.
    (It's OK to rename these; they will affect new creation, but even if this
    were not part of the first Alembic revision, downgrading is by dropping a
    whole table, not dropping it constraint by constraint.)

- When running an older version of CamCOPS (e.g. 2.2.7) on a Surface Book 2 /
  Windows 10: in no-keyboard Tablet mode, touches are not detected in the
  camera mode. Trackpad works fine. This was fixed by recompiling on this
  machine.

  .. todo::

    Does this mean that a QML ``onClicked`` event behaves differently with
    respect to touch events depending on whether it's compiled on a touch-aware
    or a touch-unaware computer? That might represent a Qt bug; investigate and
    report if so. In the meantime, **compile for Windows on a Surface Book 2 or
    similar**.

  Also relevant:

  - https://stackoverflow.com/questions/42447545/mouse-works-but-touch-doesnot-work-in-qml/42454302

- **New task:** :ref:`Perinatal POEM (Patient-rated Outcome and Experience Measure)
  <perinatal_poem>`.

- **New task:** :ref:`Goal-Based Outcomes -- Goal Record Sheet (GBO-GReS)
  <gbo_gres>`.

- **New task:** :ref:`Goal-Based Outcomes -- Goal Progress Chart (GBO-GPC)
  <gbo_gpc>`.

- **New task:** :ref:`Assessment Patient Experience Questionnaire for Psychological
  Therapies (APEQPT) <apeqpt>`.

- **New task:** :ref:`Outcome Rating Scale (ORS) <ors>`.

- **New task:** :ref:`Session Rating Scale (SRS) <srs>`.

- Bugfixes 2019-03-01: upload from very old tablets (e.g. v1.33) was broken.
  Errors included ``Unknown 'idnum1' field when uploading patient table``.
  Also placed a size limit on an audit entry (one was >0.5 Mb).

  Note that this apparent bug isn't really a bug (noticed when uploading from
  the old Titanium client):

  .. code-block:: none

    1 subject failed against an upload policy of
        forename AND surname AND dob AND sex
    145 subjects failed against an upload policy of
        sex AND ((forename AND surname AND dob) OR anyidnum)

  The Titanium client did not recognize ``anyidnum`` -- and all patients will
  fail against an invalid policy. So that makes sense.

- ``merge_db`` function made much more conservative about importing groups,
  ID number types -- user must specify the mapping manually to avoid
  inadvertent errors.

- Updated trackers to cope with blank (``None``) values, e.g. from
  :ref:`GBO-GRaS <gbo_gras>` task.

- Menu header functions updated so that anonymous tasks show the "anonymous"
  icon properly.

- Report: tasks by month/username.

- ``check_index`` command.

- Removed support for Python 3.5 since we want ``typing.Collection``.
  Minimum is now Python 3.6. (That also allows f-strings.)

- Bugfix: when password change frequency was >0, got "TypeError: can't compare
  offset-naive and offset-aware datetimes" from
  :meth:`camcops_server.cc_modules.cc_user.User.set_password_change_flag_if_necessary`.
  Added
  :meth:`camcops_server.cc_modules.cc_request.CamcopsRequest.now_utc_no_tzinfo`.

- Changes to session management, to

  - commit ASAP for ``last_activity_utc``, to avoid holding database locks
    (causing problems with very slow computers?); see
    :meth:`camcops_server.cc_modules.cc_session.CamcopsSession.get_session`.

  - avoid touching the database for static requests; see
    :meth:`camcops_server.cc_modules.cc_request.CamcopsRequest.complete_request_add_cookies`.

- Bugfix: policy validation used a combinatorial approach that became extremely
  slow when lots of ID numbers were in use (looking like a crash and sometimes
  causing database timeouts and follow-on errors). Rewritten 2019-03-23.

- **New task:** :ref:`Edinburgh Postnatal Depression Scale (EPDS) <epds>`.
  (Database revision 0019.)

- ID number fields made mandatory in patient editing questionnaire on the
  client. (Reduces the chance of uploading a blank ID number, which wouldn't
  help anyone.)

- f-strings

- Server released to CPFT on 2019-03-24.


**Client and server v2.3.2 (2018-03-25 to 2018-04-04)**

- Session information in ORS and SRS summaries.

- EPDS provides a CTV summary.

- EPDS moved within CPFT Perinatal Service menu from "generic measures" to
  "specific conditions".

- Bugfix to C++ scoring function ``Icd10Depressive::main_complete()``; some
  combinations were being labelled as "unknown" when more accuracy was
  possible.

- Fixed CORE-10 alignment problem.

- That was a more general problem of different name/value pairings sharing the
  same names. Fixable on that basis. See
  ``QuMcqGrid::setAlternateNameValueOptions``.

- **New task:** :ref:`Postpartum Bonding Questionnaire (PBQ) <pbq>`.

- On the server, group administrators can change passwords and upload groups
  for users that they manage (meaning those users who are a member of one of
  their groups, and who are not a group administrator or superuser).

- User deletion failed if the audit trail referred to the user (but no other
  checks failed). Was failing at a low (database) level with a foreign key
  constraint. Now performs a check for audit trail in
  :func:`camcops_server.cc_modules.webview.any_records_use_user`.

- Facility to hide individual special/sticky notes (with audit trail), so
  they're not shown in HTML (+ PDF) and XML views. See e-mail RNC/JK/RE,
  2018-10-12.

- ``EMAIL_HOST_USERNAME`` no longer mandatory -- surprisingly, some servers
  accept e-mails without a username.

- Bugfix to
  :meth:`camcops_server.cc_modules.cc_session.CamcopsSession.n_sessions_active_since`,
  which wasn't converting to UTC properly.


**Client and server v2.3.3**

- Windows service.

- Bump from ``cardinal_pythonlib==1.0.49`` to ``'cardinal_pythonlib==1.0.53``
  for a bugfix (1.0.51) then SQL Server custom functions (1.0.52), then new
  MIME types (1.0.53).

- Improvement to default behaviour of ``tools/create_database_migration.py``:
  modified :func:`camcops_server.alembic.env.filter_column_ops` to skip
  modifications where ``modify_type is None``. I'm not sure why these are now
  coming in droves from Alembic (it might be that this is what happens when a
  comment is changed).

- In the process, indexed ``_exported_tasks.start_at_utc`` (somehow missed
  from ``0014_new_export_mechanism.py``).

- Database revision to add all column comments. Note also:

  - Alembic misses out ``existing_nullable=False`` for fields with
    ``autoincrement=True``
  - Manual checks are required for ``mysql.VARCHAR(...)`` as these can either
    be ``sa.String(length=...)`` or ``sa.Unicode(length=...)``.

- Attempted fix for :ref:`DELETE takes forever
  <sql_server_delete_takes_forever>` bug under SQL Server when reindexing as
  part of the ``upgrade_db`` command.

  - Search for ``if_sqlserver_disable_constraints_triggers``.

  .. todo:: check this fixes the SQL Server "DELETE" bug +++

- Excel XLSX and OpenOffice/LibreOffice ODS formats supported for basic
  download.

- :ref:`Internationalization <dev_internationalization>`.

  - Client
  - ``MenuWindow`` changes to permit dynamic language change
  - Server
  - Client strings, core server to Danish

- Discovered camera bug (on Ubuntu system): opening camera system crashed
  client with error ``fatal: unknown(0): Failed to create OpenGL context for
  format QSurfaceFormat``; see https://bugreports.qt.io/browse/QTBUG-47191;
  this is with Qt 5.12.0.

  - I think this was because I'd upgraded the OS but not rebooted.
  - Multiple attempts to fix this, but it applied to all OpenGL programs (e.g.
    the Qt 3D bar graph demo) and the right thing to do was to make CamCOPS
    check for OpenGL rather than just assume its presence. See ``QuPhoto`` and
    ``openglfunc.cpp``.

- Bugfix to ``SingleTaskMenu``: if you had a patient unselected, then locked
  the app, the task list wasn't appropriately refreshed.

- Bugfix for SQL ``DATETIME`` columns when used via database URLs like
  ``mysql+mysqldb://..`` rather than ``mysql+pymysql://``.

  - Bug report: https://github.com/RudolfCardinal/camcops/issues/2 -- with
    thanks to Martin Burchell.
  - Symptom: when running e.g. ``camcops_server make_superuser``, crashes with
    error ``sqlalchemy.exc.OperationalError:
    (_mysql_exceptions.OperationalError) (1292, "Incorrect datetime value:
    '2019-05-27T15:31:37.009078+00:00' for column 'last_password_change_utc' at
    row 1")``.
  - Reason: the ``mysqlclient`` (MySQLdb) interface, at least
    ``mysqlclient==1.3.13``, mis-handles Pendulum objects.
  - See also:

    - :ref:`DB_URL <DB_URL>`.
    - https://crateanon.readthedocs.io/en/latest/installation/database_drivers.html

  - Fixes:

    - Specific: in :meth:`camcops_server.cc_modules.cc_user.User.set_password`,
      change ``self.last_password_change_utc = req.now_utc`` to
      ``self.last_password_change_utc = req.now_utc_no_tzinfo``.
    - Similarly for ``last_login_at_utc``.
    - There are probably others, too. But then a generic fix: see modifications
      to driver conversion systems, as per https://pypi.org/project/pendulum/,
      applied in ``camcops_server.cc_modules.cc_db`` upon import. Tested and
      working.

- Caching for ``Task.isComplete()`` on the client.

- SQLAlchemy upgraded from 1.2.8 to 1.3.0 in response to security
  vulnerabilities.

- New option :menuselection:`Settings --> Fetch all server info`.

- Ensure dialogs (e.g. initial password prompt) show the title in full, and
  don't clip text that they contain (e.g. good: upload dialogue; bad: ?some of
  the cancellation dialogs). See ``uifunc::minimumSizeForTitle()``. Not
  perfect (some guesswork), but better.

- Client option to drop unknown tables.

- ``cc_text.py`` and better server string framework for internationalization;
  see :ref:`String locations in CamCOPS <dev_string_locations>`.

- ``QuPage`` and corresponding questionnaire updates to provide a method for
  more complex validation when the user clicks "Finish" or tries to navigate
  away from a page. Initial use: preventing forward slashes in the server
  hostname.

- ``make_xml_skeleton.py`` development tool

- **New task:** :ref:`Lynall M-E — IAM study — medical history
  <lynall_iam_medical>`

- **New task:** :ref:`Kirby Monetary Choice Questionnaire (MCQ) <kirby_mcq>`

- **New task:** :ref:`Assessment Patient Experience Questionnaire for CPFT
  Perinatal Services (APEQ-CPFT-Perinatal) <apeq_cpft_perinatal>`.

- **New task:** :ref:`Maternal Antenatal Attachment Scale (MAAS) <maas>`.

- **General release.** But Android bug; see 2.3.4.


**Client 2.3.4 (released 20 Jun 2019)**

- The Google Play Store will soon require 64-bit builds
  (https://android-developers.googleblog.com/2019/01/get-your-apps-ready-for-64-bit.html).
  In order to get 64-bit ARM compilation working for Android:

  - lots of work to ``build_qt.py``
  - Default Android NDK from r11c to r20, which means moving from gcc to clang
  - OpenSSL from 1.1.0g to 1.1.1c to cope with clang
  - SQLCipher from 3.4.2 to 4.2.0 to cope with OpenSSL 1.1.1

    - Note the need for ``PRAGMA cipher_compatibility`` or ``PRAGMA
      cipher_migrate``; see
      https://discuss.zetetic.net/t/upgrading-to-sqlcipher-4/3283

    - **Databases from older versions of CamCOPS will be read and migrated
      automatically, but will then not be openable directly by older versions
      again.**

  - Qt from 5.12.0 to 5.12.3 plus some Git tweaks to get upstream support for
    Android NDK r20.

  - Qt not compiling; bug raised at
    https://bugreports.qt.io/browse/QTBUG-76445.

- V2.3.3 for Android was crashing on startup. From debugging views, error was
  "dlsym failed: undefined symbol main"; "Could not find main method";
  subsequently "SIGSEGV" and "backtrace".

  - ``objdump -t libcamcops.so | grep main`` gave

    .. code-block:: none

        001d4144 l     F .text	00000178              .hidden main

    whereas in a basic test app, ``objdump -t libbasic_qt_app.so | grep main``
    gave

    .. code-block:: none

        00002e70 g     F .text	00000118              main

    Can also use the ``nm`` tool, or ``readelf -a``, which is very clear (and
    probably others too).

    So why is ``main()`` hidden?

    The problem was ``-fvisibility=hidden`` in ``camcops.pro``; fixed with
    ``VISIBLE_SYMBOL`` macro in ``preprocessor_aid.h``.


**Client and server v2.3.5, released 16 Sep 2019**

- Target Android API from 26 to 28 as now required by Google.

- Bugfix to trackers, which were ignoring zero values; see
  :meth:`camcops_server.cc_modules.cc_tracker.Tracker.get_single_plot_html`.

- Slightly hacky bugfix to ``sizehelpers::labelExtraSizeRequired()``, to
  mitigate odd bug in which questions in a `QuMcqGrid` were over-word-wrapped.
  Debugging sequence:

  - commented sizehelpers.h
  - tried a size policy of expandingFixedHFWPolicy() in mcqfunc::addQuestion()
    -- no joy, reverted
  - uncommented "#define OFFER_LAYOUT_DEBUG_BUTTON" in questionnaire.cpp
  - dump the layout from a PHQ9 (to the debug console)
  - The first question ("1. Little interest or pleasure in doing things") is:

    .. code-block:: none

        LabelWordWrapWide<0x000056045feb4eb0 'question'>, visible, pos[DOWN] (0, 129), size[DOWN] (407 x 48), hasHeightForWidth()[UP] true, heightForWidth(407[DOWN])[UP] 58, minimumSize (0 x 0), maximumSize (16777215 x 16777215), sizeHint[UP] (407 x 29), minimumSizeHint[UP] (109 x 29), sizePolicy[UP] (Expanding, Fixed) [hasHeightForWidth=true], stylesheet: false, [WARNING: geometry().height() < heightForWidth(geometry().width())] [alignment from layout: AlignLeft | AlignVCenter]

    - ... where "[DOWN]" means "imposed from above, i.e. from the layout" and
      "[UP]" means "determined by the widget or its contents and told to the
      layout"
    - ... so this indicates the widget is saying "I'd like to be 407 x 29"
      (sizeHint) and "(horizontal expanding) 407 is a reasonable size; you can
      enlarge or shrink me if you want, but I'd like to be as large as
      possible; (HFW set) my height depends on my width; (vertical fixed) once
      my height is set from my width, that is fixed"
    - ... on a screen in which [via GIMP] the label is 407 x 48 within a cell
      that should be about 899 x 48
    - ... so, the widget is not asking for enough horizontal space.

  - This, therefore, points the finger at LabelWordWrapWide::sizeHint().
  - This is already ensuring the CSS etc. is applied, via ensurePolished().
    That should deal with CSS-defined margins and the like.
  - uncomment "#define DEBUG_CALCULATIONS" in labelwordwrapwide.cpp
  - "1. Little interest or pleasure in doing" = 331 wide; "things" = 56 wide;
    another space = 9 wide -- that's suggest about 396 for the text. Then there
    is a left border of 12 pixels (ish) and probably the right one is identical
    -- so the width should perhaps be about 420, and it's asking for 407.
  - So where's the deficit? Could either be the margins or the text itself.
  - LabelWordWrapWide::sizeOfTextWithoutWrap() should provide the text size
    itself. This may not be spot on; but running a grep on the output gives (in
    the last version -- font sizes etc. may change as Qt lays stuff out) 397
    wide. Which sounds OK.
  - grep "text_size" calcs.txt | grep "1. Little" gives:

    .. code-block:: none

      camcops[18336]: 2019-07-05T23:36:18.533: debug: ../tablet_qt/widgets/labelwordwrapwide.cpp(432): virtual QSize LabelWordWrapWide::sizeHint() const - text_size QSize(397, 19) -> QSize(407, 29) ... text: "1. Little interest or pleasure in doing things"

    so that suggests that 10 width is being added for margins, and that is too
    small; so the bug looks like it is in
    LabelWordWrapWide::extraSizeForCssOrLayout(), which, via a similar grep, is
    returning QSize(10, 10).

  - In turn that suggests the problem is in
    sizehelpers::labelExtraSizeRequired(), or the cache system.
  - Disabling "#define LWWW_USE_STYLE_CACHE"... no difference. So probably not
    the cache system there.
  - However, LabelWordWrapWide::extraSizeForCssOrLayout() is returning
    QSize(10, 10), which looks too small.
  - uncomment "#define DEBUG_WIDGET_MARGINS" in sizehelpers.cpp
  - looks like the margins are coming from the stylesheet, not the layout:

    .. code-block:: none

        camcops[22749]: 2019-07-06T00:12:30.198: debug: ../tablet_qt/lib/sizehelpers.cpp(217): QSize sizehelpers::widgetExtraSizeForCssOrLayout(const QWidget*, const QStyleOption*, const QSize&, bool, QStyle::ContentsType)widget "LabelWordWrapWide<0x000056458065d9c0 'question'>"; child_size QSize(0, 0); stylesheet_extra_size QSize(10, 10); extra_for_layout_margins QSize(0, 0) => total_extra QSize(10, 10)

    ... which is right because QuMcqGrid::makeWidget() does "grid->setContentsMargins(uiconst::NO_MARGINS);"
  - So we're looking at
    https://doc.qt.io/archives/qt-4.8/qstyle.html#sizeFromContents
  - The actual extra comes from the CSS: "#mcq_grid #question { padding: 5px; }"
  - As a test, doubling the width in sizehelpers::labelExtraSizeRequired()
    works. Reverted that.
  - comment out "#define ADD_EXTRA_FOR_LAYOUT_OR_CSS" in
    labelwordwrapwide.cpp -- and see comments there; previous problems with
    QLabel. Nope, didn't help; reverted.
  - set "show_widget_stylesheets = true" in layoutdumper::DumperConfig header
    and re-dump -- but the widget doesn't have its own stylesheet (just the
    global one, via uiconst::CSS_CAMCOPS_QUESTIONNAIRE) so that doesn't help.
  - The odd thing is that I think the CSS is setting 5px padding, but 10px is
    coming out.
  - Check by changing that bit of CSS to 0 -- yes.
  - So... the calculated extra size reflects the CSS, but the actual painting
    doesn't! padding 0 -> 0 displayed on left; padding 5px -> 10px displayed on
    left; padding 10px -> 15px displayed on left; padding 15px -> 20 px
    displayed on left. So consistently +5. Odd. Similarly larger on the other
    sides.
  - Is it because QPushButton has padding: 5px, and
    sizehelpers::labelExtraSizeRequired() uses QStyle::CT_PushButton? No,
    setting QPushButton padding to 0 made no difference.
  - Anyway, back to doubling the width in
    sizehelpers::labelExtraSizeRequired(). A hack... And reverted other
    debugging options.

- Client asks for information to be re-fetched (not the client to be
  re-registered -- which is a privileged operation) when the server information
  doesn't match stored copies.

- Better SNOMED coding for the clinical tasks :ref:`Progress note
  <progress_note>` and :ref:`Psychiatric clerking <clerking>`.

- Bugfix: server group editing page crashed if no ID numbers defined. Changed
  in :func:`camcops_server.cc_modules.TokenizedPolicy.set_valid_idnums`.

- Client: For ACE-III and similar: when pages don't scroll, offer facility to
  zoom widgets. See ``ZoomableWidget``. Used in ACE-III for letters, picture
  naming, etc.

- Server: restore autogeneration of CRIS and CRATE data dictionaries. See
  ``cc_anon.py`` etc.

- HTML and PDF titles for tasks.

- User list shows, for group administrators, which groups they administer.

- Bugfix: client C++ functions ``mathfunc::countWhere()`` and
  ``mathfunc::countWhereNot()`` now respect NULL values, via
  ``mathfunc::containsRespectingNull()``. This behaviour now matches
  :meth:`camcops_server.cc_modules.cc_task.Task.count_where` and
  :meth:`camcops_server.cc_modules.cc_task.Task.count_wherenot`. Applicable to
  the :ref:`SHAPS <shaps>` task.

- **New task:** :ref:`Elixhauser Comorbidity Index (ElixhauserCI) <elixhauserci>`.
  (Database revision 0029.)

- **New task:** :ref:`Cambridge-Chicago Compulsivity Trait Scale (CHI-T) <chit>`.
  (Database revision 0030.)

- **New task:** :ref:`Short UPPS-P Impulsive Behaviour Scale (SUPPS-P) <suppsp>`.
  (Database revision 0031.)

- **New task:** :ref:`EULAR Sjögren’s Syndrome Patient Reported Index (ESSPRI) <esspri>`.
  (Database revision 0032.)

- **New task:** :ref:`Ankylosing Spondylitis Disease Activity Score (ASDAS) <asdas>`.
  (Database revision 0033.)

- **New task:** :ref:`Multidimensional Fatigue Inventory (MFI-20) <mfi20>`.
  (Database revision 0034.)

- **New task:** :ref:`Short-Form McGill Pain Questionnaire (SF-MPQ2) <sfmpq2>`.
  (Database revision 0035.)

- **New task:** :ref:`Disease Activity Score-28 (DAS28) <das28>`.
  (Database revision 0036.)

- **New task:** :ref:`Snaith–Hamilton Pleasure Scale (SHAPS) <shaps>`.
  (Database revision 0037.)

- Add optional waist circumference to :ref:`BMI  <bmi>`.
  (Database revision 0038.)

- Add ``setMinimiumDate()`` and ``setMaximumDate()`` to ``QuDateTime``.
  This also fixes the broken default minimum date of 1st January 1880.

- Set ``strict_undefined=True`` for Mako template lookups, so they crash
  immediately on typos.

  - ... nope; reverted. Tricky to get e.g. ``not_found.mako`` to inherit
    ``generic_failure.mako`` and override ``msg`` and ``extra_html`` without
    having unknown variables being handled (as undefined) rather than raising
    an error.

- **New tasks:** :ref:`Khandaker GM — MOJO study <khandaker_mojo>`.
  (Database revisions 0039-0041.)

- More consistent numbering/naming convention for custom tasks:

  - Numbering may be used in code (filenames, class names), if desired, which
    helps the programmer (it groups lots of files relating to the same task
    together quickly when searching).
  - Numbering not used in menus or task names, because it's slightly confusing
    for the user (are we numbering tasks overall? Studies? Tasks within
    studies?).
  - Unnamed studies may be named "S1", "S2", ...
  - Board format remains, overall: PI, study name, task name.
  - Historical table names not changed.
  - Future table names: try to avoid numbers.

- Regression: crash in creating SVG figures from
  ``cardinal_expdetthreshold.py`` and ``cardinal_expectationdetection.py``.
  Details in comments here. Likely due to a matplotlib change.

..  Not helped by matplotlib upgrade from 3.0.2 to 3.1.1. However, no problem
    with ``ace3.py``, which also uses ``fontdict``.
    .
    The error was:
    .
    .. code-block:: none
    .
      File "/home/rudolf/dev/venvs/camcops/lib/python3.6/site-packages/matplotlib/offsetbox.py", line 808, in get_extent
        "lp", self._text._fontproperties, ismath=False)
      File "/home/rudolf/dev/venvs/camcops/lib/python3.6/site-packages/matplotlib/backends/backend_agg.py", line 210, in get_text_width_height_descent
        font = self._get_agg_font(prop)
      File "/home/rudolf/dev/venvs/camcops/lib/python3.6/site-packages/matplotlib/backends/backend_agg.py", line 245, in _get_agg_font
        fname = findfont(prop)
      File "/home/rudolf/dev/venvs/camcops/lib/python3.6/site-packages/matplotlib/font_manager.py", line 1238, in findfont
        rc_params)
      File "/home/rudolf/dev/venvs/camcops/lib/python3.6/site-packages/matplotlib/font_manager.py", line 1270, in _findfont_cached
        + self.score_size(prop.get_size(), font.size))
      File "/home/rudolf/dev/venvs/camcops/lib/python3.6/site-packages/matplotlib/font_manager.py", line 1076, in score_family
        family1 = family1.lower()
    AttributeError: 'dict' object has no attribute 'lower'
    .
    or
    .
    .. code-block:: none
    .
      File "/home/rudolf/dev/venvs/camcops/lib/python3.6/site-packages/matplotlib/backends/backend_svg.py", line 1180, in get_text_width_height_descent
        return self._text2path.get_text_width_height_descent(s, prop, ismath)
      File "/home/rudolf/dev/venvs/camcops/lib/python3.6/site-packages/matplotlib/textpath.py", line 89, in get_text_width_height_descent
        font = self._get_font(prop)
      File "/home/rudolf/dev/venvs/camcops/lib/python3.6/site-packages/matplotlib/textpath.py", line 38, in _get_font
        fname = font_manager.findfont(prop)
      File "/home/rudolf/dev/venvs/camcops/lib/python3.6/site-packages/matplotlib/font_manager.py", line 1238, in findfont
        rc_params)
      File "/home/rudolf/dev/venvs/camcops/lib/python3.6/site-packages/matplotlib/font_manager.py", line 1270, in _findfont_cached
        + self.score_size(prop.get_size(), font.size))
      File "/home/rudolf/dev/venvs/camcops/lib/python3.6/site-packages/matplotlib/font_manager.py", line 1076, in score_family
        family1 = family1.lower()
    AttributeError: 'dict' object has no attribute 'lower'
    .
    .
    Is also not specific to SVG, as it still happens (and the ACE-III is still
    OK) when setting ``USE_SVG_IN_HTML = False``.
    .
    Not affecting self-testing (which probably skips those figures for a blank
    task).
    .
    In :class:`camcops_server.tasks.CardinalExpDetThreshold`, the problem was
    from a call to :meth:`matplotlib.axes.Axes.legend` with argument
    ``prop=req.fontprops``. The documentation at
    https://matplotlib.org/3.1.1/api/_as_gen/matplotlib.axes.Axes.legend.html
    suggests that a dictionary is OK.
    .
    Looks like that ends up at :meth:`matplotlib.legend.Legend.__init__`.
    .
    Aha. Bug found. In
    :meth:`camcops_server.cc_modules.cc_request.CamcopsRequest.fontprops`, this:
    .
    .. code-block:: python
    .
        return FontProperties(self.fontdict)
    .
    should have been this:
    .
    .. code-block:: python
    .
        return FontProperties(**self.fontdict)
    .
    The odd thing is that the change was between 2017-09-10 and 2017-09-11 and
    it was certainly working after that, so perhaps ``matplotlib`` used to
    accept a dictionary or **kwargs and no longer does.

**Client and server v2.3.6, released 31 Oct 2019**

- Group name character restrictions with validation; see
  :func:`camcops_server.cc_modules.cc_group.is_group_name_valid`.

- **Restricting task strings to groups:** see :ref:`RESTRICTED_TASKS
  <RESTRICTED_TASKS>`.

- **New task:** :ref:`Lynall M-E — IAM study — life events <lynall_iam_life>`.

- Removed now-pointless command ``camcops_server demo_mysql_dump_script`` (out
  of scope and ``camcops_backup_mysql_database`` is better).

- Removed now-pointless command ``camcops_server demo_mysql_create_db`` (it's
  in the help at :ref:`Create a database <create_database>`).

- Group administrators can now **erase tasks entirely** from the database
  :ref:`Erase task instance entirely <erase_task_entirely>`. This was to cope
  with the fact that a dodgy network leads to a tradeoff between data loss and
  data duplication and CamCOPS tries very hard not to lose data. Notes are in
  the source for this file.

.. Notes:
  - Re possibility of duplication ?due to network dropout:
..
    - Facility to delete individual tasks from the server, via a safety check
      form and then
      :meth:`camcops_server.cc_modules.cc_task.Task.delete_entirely`.
..
    - There was not a specific "delete task" function that's accessible to
      users. Duplicates sounded concerning but we can think this through. On
      the client:
..
      - everything begins with NetworkManager::upload() and chugs through a
        series of steps via ::uploadNext() (e.g. checking the server knows
        about our device)
..
      - If we're using one-step upload, then we end up at
        NetworkManager::uploadOneStep(), followed by NextUploadStage::Finished
        (which wipes local data) -- so if the upload succeeds, data is wiped,
        and if it doesn't, it's not. It is probably possible that if the server
        accepts the upload data (writing it to its database) but then the
        connection is dropped before the server can say "OK, received", that
        the client will not delete the data, leading to duplication. I presume
        that is what's happened. (Definitely better than the other option of
        deleting from the client without confirmation, though!)
..
      - In a multi-step upload, there is a multi-stage conversation which ends
        up with the client say "OK, commit my changes", via ::endUpload(), and
        the server saying "OK". I imagine that a connection failure during that
        last phase might lead to the server saving/committing but the "done"
        message not getting back to the client. This is probably less likely
        than with the one-step upload, because it's a very brief process.
..
    What sort of failure messages were you seeing? Was it all explicable by
    dodgy wi-fi?
..
    If this looks the likely cause -- we should implement a privileged
    operation (with deliberately difficult validation steps as for some of the
    other unsafe operations) to call Task.delete_entirely(), which does the
    business. (At present that is only called when an entire patient is
    deleted.) I think that will be OK because I think there is very little
    chance of any "partial" uploads; the system should prevent those
    effectively.
..
    I think that sounds safer than any of the alternatives.
..
    Likewise, if this is the probable root cause, perhaps we should add a
    warning (+/- change the default upload method) to say that "if you have a
    dodgy network connection, the chance of duplicates is probably lower with
    the multi-step upload".

- Split out Athena OHDSI and SNOMED core code to
  ``cardinal_pythonlib``, now ``cardinal_pythonlib==1.0.70``.

- Excel XLSX and OpenOffice/LibreOffice ODS formats supported for report
  download.

- Fixed missing and incorrect options for question 2 of
  :ref:`Maternal Antenatal Attachment Scale (MAAS) <maas>`.

- **Task chains.** Example in MOJO study menu.

- Improved task title/subtitle labelling.

- Removed clinician details (which were unused) from

  - :ref:`Ankylosing Spondylitis Disease Activity Score (ASDAS) <asdas>`
  - :ref:`Cambridge-Chicago Compulsivity Trait Scale (CHI-T) <chit>`
  - :ref:`EULAR Sjögren’s Syndrome Patient Reported Index (ESSPRI) <esspri>`
  - :ref:`Multidimensional Fatigue Inventory (MFI-20) <mfi20>`
  - :ref:`Short-Form McGill Pain Questionnaire (SF-MPQ2) <sfmpq2>`
  - :ref:`Short UPPS-P Impulsive Behaviour Scale (SUPPS-P) <suppsp>`

  Note that this would **break** uploads from client v2.3.5 to server v2.3.6+,
  but all of these tasks entered with the MOJO study and none were in live use
  beforehand.

- Diagnosis in summary for :ref:`Khandaker GM — MOJO — Medical questionnaire
  <khandaker_mojo_medical>`.

- References to "clinician" replaced by "clinician/researcher".

- Reports for CPFT Perinatal service.

  - APEQ_CPFT_Perinatal reports:

    - summary of question and %people responding each possibility
    - plus "summary of comments"

  - POEM: as per APEQ_CPFT_Perinatal

  - Core-10 report:

    For those with >=2 scores, "start" mean and "finish" mean, where "start" is
    the first and "finish" is the latest.

  - MAAS: as per Core-10, but also for subscales

  - PBQ: as per Core-10, but also for subscales

- Bugfix: Automatically create EXPORT_LOCKDIR on server startup


.. _changelog_2020:

2020
~~~~

**Client and server v2.3.7, released 3 Mar 2020**

- ``pyexcel-ods3`` and ``pyexcel-xlsx`` for spreadsheet export; faster and much
  smaller for ODS files. See ``cc_tsv.py``.

- Option to send research data dumps by e-mail, to offload work to the
  back-end. Similarly, option to queue data dumps for download.

- New ``camcops_server purge_jobs`` command.

- Memory problems were in part due to Celery leaving lots of workers active.

  - Useful tools: ``htop``, ``pgrep``, ``pkill``.
  - ``htop`` shows that Celery worker processes (like ``.../celery worker --app
    camcops_server...`` are at first children of a master ``.../celery worker``
    processes, which is the child of ``camcops_server launch_workers``.
    However, after killing the top-level process (e.g. with Ctrl-C), the
    top-level process and the "owner" Celery worker die, leaving the workers
    themselves as children of ``/lib/systemd/systemd --user``. If every
    start-and-kill leaves 8 orphan workers, you can run out of memory quite
    quickly. Use e.g. ``pkill -f camcops_server`` to wipe them all out.
  - This may have explained some MySQL locking problems too.
  - Celery from 4.2.1 to 4.3.0 as this has "bug fixes mainly for Celery Beat,
    Canvas, a number of critical fixes for hanging workers and fixes for
    several severe memory leaks"
    (https://docs.celeryproject.org/en/latest/whatsnew-4.3.html).
    Didn't fix this problem, though.
  - Remember the signals (see ``kill -L``), which include:

    ============== ======== ================= ========= =======================
    Signal         Signal#  ``kill`` example  Shortcut  Meaning
    ============== ======== ================= ========= =======================
    SIGINT         2        ``kill -2``       Ctrl-C    Interrupt; weakest
    SIGTERM, TERM  15       ``kill``                    Terminate; medium
    SIGKILL        9        ``kill -9``                 Hard kill; never fails
    ============== ======== ================= ========= =======================

    See e.g.
    https://unix.stackexchange.com/questions/251195/difference-between-less-violent-kill-signal-hup-1-int-2-and-term-15.

  - Celery expects TERM;
    https://docs.celeryproject.org/en/latest/userguide/workers.html#stopping-the-worker.

  - And calling ``kill <celery_worker_master_pid>`` works; it prints
    ``worker: Warm shutdown (MainProcess)`` and everything exits nicely.

  - The same happens with ``kill -SIGINT``.

  - The problem is probably that Python's :func:`subprocess.call` sends
    SIGKILL to its child when the Python program is interrupted by SIGINT.
    See:

    - https://bugs.python.org/issue25942.
    - https://github.com/python/cpython/pull/4283

  - We are using Python 3.6; changes afoot for 3.7?

- ``cardinal_pythonlib==1.0.83`` for memory efficiency, then for the
  :func:`nice_call` function that sorts this out a bit, etc.

- R script file export for basic data dumps.

- Create ``camcops_server.__version__``.

- RabbitMQ into Debian/RPM package requirements, and installation docs.

- Slightly pointless option to print database schema from command line
  as PlantUML +/- PNG (but the PNG is huge).

- :ref:`MFI-20 <mfi20>` added as a full task (with usage restrictions)
  following the kind permission of the lead author.

- Bugfixes re "no PID" ID policies:

  - Client and server tables using "other" as fieldname whereas policy/docs
    use "otherdetails". This is OK but policy mapping was wrong.

  - With no DOB present, there was an error at upload: ``Server reported an
    error: Patient JSON contains invalid non-string``. Bug was in
    :func:`camcops_server.cc_modules.client_api.op_validate_patients.ensure_string`.

  - Added button to nullify DOB for the "no-DOB" policies.

- Documentation link from app fixed for FFT, CGI-I, IRAC, RSS
  (patient-specific), RSS (survey), PSS.

- Bugfix to demo supervisord config file: indented comments are not OK on at
  least some versions of supervisor (2020-02-20; on Ubuntu 18.04).

- :ref:`DAS28 <das28>` CRP and ESR changed from integer to floating point
  (Database revision 0045).

- Bugfix to QuLineEditDouble, where the default minimum value was positive,
  preventing zero or negative numbers from being entered.

- Restrict alcohol units for :ref:`Khandaker GM — MOJO — Medical questionnaire
  <khandaker_mojo_medical>`.

- Bugfix to ``Thermometer`` widget (e.g. for EQ-5D-5L). Height suffered from
  an integer rounding problem (lots of little images stacked).
  Significant rewrite of widget code.
  Also removed ``QUTHERMOMETER_USE_THERMOMETER_WIDGET`` option (now always
  defined, effectively).

**Client and server v2.3.8, released 15 Sep 2020**

- Fixed openpyxl conflict when installing from Debian package

- Fixed blank labels on form to delete user and translation of "Danger" on
  other deletion forms.

- Bugfix to
  :meth:`camcops_server.cc_modules.cc_patient.gen_patient_idnums_even_noncurrent`.
  This created a set of PatientIdNum instances, comparing them in the usual way
  of "do they represent the same ID number?" (implemented via the ``__hash__``
  and ``__eq_`` functions of PatientIdNum). However, here, we are after
  distinct database records; we therefore want the additional condition that
  two things are "unequal" if their primary keys are different. We do this by
  checking PK instead.

  Similar changes to
  :meth:`camcops_server.cc_modules.cc_db.GenericTabletRecordMixin.gen_ancillary_instances_even_noncurrent`
  and
  :meth:`camcops_server.cc_modules.cc_db.GenericTabletRecordMixin.gen_blobs_even_noncurrent`,
  which now operate by PK.

  Achieved via the generic function
  :meth:`camcops_server.cc_modules.cc_db.GenericTabletRecordMixin._gen_unique_lineage_objects`.

- Bugfix to :ref:`Khandaker GM — MOJO — Medical questionnaire
  <khandaker_mojo_medical>` where the diagnosis date / years since diagnosis fields were not
  marked as mandatory.

- :ref:`Export of tasks to REDCap <redcap>`.
  (Database revision 0046).

- Docker support for the server.

- Better clarity of error messages for administrators in
  :class:`camcops_server.cc_modules.cc_forms.DeliveryModeNode`.

- Cosmetic fix: if ID numbers are always present, the tracker consistency view
  shouldn't say "all blank or X" (makes users think it might be blank when it's
  not). Changed in
  :func:`camcops_server.cc_modules.cc_tracker.consistency_idnums`.

- Cosmetic fix: an ``axis_min`` of zero was being ignored (the test was
  inappropriately an implicit cast to boolean rather than ``is not None`` in
  :meth:`camcops_server.cc_modules.cc_tracker.Tracker.get_single_plot_html`.
  Observed in QoLSG task. Also would have been true of ``axis_max``.

- Cosmetic fix: in PDF tracker generation, the PNG (rather than the SVG) was
  being used. May relate to ``wkhtmltopdf`` version? PNG fallback removed via
  the ``provide_png_fallback_for_svg`` option in
  :class:`camcops_server.cc_modules.cc_request.CamcopsRequest`.

- In the process of fixing a "pixellated font" problem via wkhtmltopdf, which
  seems to have problems with the "opacity" style in SVG (in wkhtmltopdf
  version 0.12.5), sorted out z-order to make plotting more efficient (and
  avoided opacity).

- Option to download column info (from ``INFORMATION_SCHEMA.COLUMNS``) with
  basic and SQLite dumps.

- There appears to be a bug in Deform (currently ``deform==2.0.8``) that
  emerges when Chameleon is upgraded (e.g. from ``Chameleon==3.4`` to
  ``Chameleon==3.8.0``, probably as Chameleon fixes some bugs in its
  implementation of the TAL language). Specifically, a bunch of HTML attributes
  like ``<select multiple>`` and ``<input type="checkbox" checked>`` are
  mis-rendered as ``multiple="False"`` or ``checked="False"``, which reverses
  their meaning. This manifests as, for example, single-select dropdowns
  allowing multiple selections (fixed temporarily via
  :class:`camcops_server.cc_modules.cc_forms.BugfixSelectWidget`) and things
  being ticked when they shouldn't be (e.g. ``CheckboxChoiceWidget`` -- not so
  obviously fixable), and the wrong defaults (e.g. ``RadioChoiceWidget``).

  Temporary fix: pin Chameleon to 3.4.

  Consider: Deform seems to be out of regular maintenance and is rated
  "L2" (low) at e.g. https://python.libhunt.com/wtforms-alternatives. Should we
  use WTForms (https://wtforms.readthedocs.io/)? That's rated L5.

  2020-07-24: No, Deform has caught up. See https://pypi.org/project/deform/.
  Move to ``deform==2.0.10`` and ``Chameleon==3.8.1``.

**Client and server v2.4.0, released 18 Dec 2020**

- Support for :ref:`Task Schedules <scheduling_tasks>` (on the server) and
  :ref:`Single User Mode <single_user_mode>` on the client
  to allow patients to complete tasks at home on their own devices.
  (Database revisions 0047-0052.) Version bumped to 2.4.0 as this is a major
  new feature.

..  Design notes were:
    .
    - (SERVER + CLIENT) Concept of “tasks that need doing” in the context of a
      research study.
    .
      - define patients on server (per group)
    .
        - share main patient/patient_idnum tables
    .
        - use the “server device” to create them, and always in era “NOW”
    .
      - ScheduledTask -- "task needs doing"
    .
        - patient (by ID number); group; task; due_from; due_by;
          skip_if_not_done_by; cancelled?
    .
        - Example: "PHQ9 due for Mr X after 1 July; due by 7 July; must be
          completed by 1 Aug"
    .
      - then for metacreation: “StudySchedule” or “TaskPanel”
    .
        - ... a list of tasks, each with: task; due_from_relative_to_start_date;
          due_by_relative_to_start_date
    .
        - example: “In our study, we want a PHQ9 and GAD7 at the start, a PHQ9 at
          3 months, and a PHQ9 and GAD7 at 6 months.”
    .
      - PatientSchedule
    .
        - instantiate a “StudySchedule”/“TaskPanel” with patient, group, start
          date
    .
        - e.g. “Mr Jones starts today.... enrol!”
    .
      - ALTERNATIVELY: do we need ScheduledTask if the main thing is a
        person/panel association?
    .
      - Tablets should fetch “what needs doing” for any patients defined on the
        tablet, and display them nicely.
      - Tasks must be complete to satisfy the requirement.
    .
      - Database field type: represent :class:`pendulum.Duration` in ISO-8601
        format, which is ``P[n]Y[n]M[n]DT[n]H[n]M[n]S``. The
        ``pendulum.Duration.min`` and ``pendulum.Duration.max`` values are
        ``Duration(weeks=-142857142, days=-5)`` and ``Duration(weeks=142857142,
        days=6)`` respectively. A possible database output standard is
        ``PT[x.y]S``, with floating-point seconds; this maps from the
        :func:`pendulum.Duration.total_seconds` function.
    .
        - See new functions
          :func:`cardinal_pythonlib.datetimefunc.duration_to_iso` and
          :func:`cardinal_pythonlib.datetimefunc.duration_from_iso`.
    .
        - New column type
          :class:`camcops_server.cc_modules.cc_sqla_coltypes.PendulumDurationAsIsoTextColType`.
    .
    - … Relating to that: consider, on the client, a “single-patient” mode
      (distinct from the current “researcher” mode), tied to a specific server.
      “This tablet client is attached to a specific patient and will operate in
      a patient-friendly, single-patient mode. Show me what needs completing.”
      The operating concept would be: if you would like someone geographically
      far away to be able to download CamCOPS and complete a set of tasks for
      you, how could you organize so that would be simplest for them? The
      minimum would that you’d create login details for them, and give them a
      URL, username, and password. *See "client" above.**

- New passwords on the client app must now be at least 8 characters long.

- Penetration testing of the server:

  - Set ``X-Frame-Options`` HTTP header to ``DENY`` (fixing alert
    ``X-Frame-Options Header Not Set`` fom ZAP).

  - Set ``X-Content-Type-Option`` HTTP header to ``nosniff`` (fixing alert
    ``X-Content-Type-Options Header Missing`` from ZAP).

  - ``autocomplete`` fields for some username and password forms. (It
    complained at some point about not having these! Seems stylistic rather
    than a vulnerability.)

  - Changed HTML name of anti-CSRF token from ``csrf`` to ``csrf_token`` to
    stop false ZAP alert ``Absence of Anti-CSRF Tokens``.

  - :func:`camcops_server.cc_modules.cc_request.validate_url` (fixing alert
    ``SQL Injection`` via URL redirection).

  - NB occasionally the label "CSRF Token" became visible, e.g. on the login
    page, but it went away again (Chrome cache problem during bug-fixing?).

- ``account_locked.mako`` wasn't formatting its strings correctly.

- Bugfix where the names of patients with no surname were not displayed.

- **New task:** :ref:`Bath Ankylosing Spondylitis Disease Activity Index
  (BASDAI) <basdai>`. (Database revision 0053.)

- Bugfix to :ref:`GMC Patient Questionnaire <gmcpq>` such that 'X' is now a
  permitted value for q10 (Sex) on the server, in line with the app.
  (Database revision 0054.)

- compilation error fix - cryptofunc - error: ‘runtime_error’ is not a member
  of ‘std’

- **New task:** :ref:`Routine Assessment of Patient Index Data (RAPID3)
  <rapid3>`. (Database revision 0055.)

- Usability improvements to :ref:`Khandaker GM — MOJO — Medical questionnaire
  <khandaker_mojo_medical>` and :ref:`Khandaker GM — MOJO — Medications and
  therapies <khandaker_mojo_medicationtherapy>`. (Database revisions 0056-0057)


.. _changelog_2021:

2021
~~~~

**Client v2.4.1, released 9 Feb 2021**

- iOS client now available for development

**Client and server v2.4.2, released 19 Mar 2021**

- iOS client now available from Apple App Store. Note that due to the way the
  Apple review process works, the version deployed is actually several commits
  older than the v2.4.2 git tag. It differs from the Windows and Android
  releases in the following ways:

  - It will erroneously report its version as 2.4.1
  - It won't report incompatibility with server >= v2.4.0. See
    https://github.com/RudolfCardinal/camcops/issues/137
  - Minor cosmetic differences in the display of dialogs
  - MoCA functionality still present (see below)

- :ref:`MoCA <moca>` functionally disabled due to rule changes. Old data
  won't be deleted.

- Ability to customise the emails sent to patients assigned to a task schedule.
  (Database revision 0058)

**Client v2.4.3, released 19 Mar 2021**

- Identical to v2.4.2. Version number changes only to work around Apple Store
  constraints.

**Server v2.4.4, released 29 Mar 2021**

- Fix two bugs when deleting patients created on the server:

  - It was impossible to delete an already registered patient
    https://github.com/RudolfCardinal/camcops/issues/143
    (Database revision 0060)

  - Deleting a patient created on the server would delete ID numbers from
    unrelated patients. This is because the entries in the patient_idnum table
    were being created with the id field set to zero.
    https://github.com/RudolfCardinal/camcops/issues/144
    (Database revision 0061 assigns correct ids to any entries that require them)

- Fix bug where patient ID numbers were not always displayed correctly in the
  list of patients and their task schedules.
  https://github.com/RudolfCardinal/camcops/issues/147

**Client v2.4.5, released 30 Mar 2021**

- Support for macOS client

**Client and server v2.4.6, released 7 Mar 2021**

- Fixes for penetration testing, per report by Falanx Cyber Defence Ltd, 28 Apr
  2021 (reference  FB05540-CP05394, commissioned by CPFT). Vulnerabilities
  rated high (H), medium (M), or low (L). None rated critical or informational.

  - H1. "Cross-site scripting."

    - URLs (for a local test server) that did bad things:

      .. code-block:: none

        https://127.0.0.1:8088/report?report_id=diagnoses_finder_icd10&viewtype=html&rows_per_page=&page=1&which_idnum=1&diagnoses_inclusion=k88py%3e%3cinput%20type%3dtext%20autofocus%20onfocus%3dalert(1)%2f%2fttwcp&diagnoses_exclusion=%27%22%3E%3Csvg%2Fonload%3Dconfirm%285%29%3B%3E%7B%7B1337%2A1337%7D%7Df276e115d231089110f46050a3a77a39&age_minimum=1&age_maximum=2
        https://127.0.0.1:8088/report?report_id=diagnoses_finder_icd10&viewtype=html&rows_per_page=&page=1&which_idnum=1&diagnoses_inclusion='%22%3e%3csvg%2fonload%3dconfirm(5)%3b%3e%7b%7b1337*1337%7d%7df276e115d231089110f46050a3a77a39ffeds%3cinput%20type%3dtext%20autofocus%20onfocus%3dalert(1)%2f%2fany2w&diagnoses_exclusion=%27%22%3E%3Csvg%2Fonload%3Dconfirm%285%29%3B%3E%7B%7B1337%2A1337%7D%7Df276e115d231089110f46050a3a77a39&age_minimum=1&age_maximum=2

    - References:

      - https://owasp.org/www-community/attacks/xss/ [very good overview]
      - https://owasp.org/www-project-top-ten/OWASP_Top_Ten_2017/Top_10-2017_A7-Cross-Site_Scripting_(XSS)

    - Note that these were only available to authenticated/authorized users;
      but still concerning.

    - RESPONSE:

      - The execution aspects ceased to execute because when Content Security
        Policy were turned up to maximum (see L2 below).
      - All GET string parameters now go through validation (in
        CamcopsRequest). That fixes these errors.

  - M1. "Weak password policies."

    - Tester wants 10-character minimum password length, not 8.
    - Additional advisory re user education on how to pick strong passwords.
    - Note that the account lockout control makes direct password-guessing
      attacks unlikely, so obtaining passwords would require some sort of other
      attack vector such as SQL injection.
    - Recommendation for salted/hashed password storage, which we already do.
      Named secure algorithms include bcrypt, which we already use.
    - References given:

      - https://www.ncsc.gov.uk/guidance/password-guidance-simplifying-your-approach
        [Note that this advises password managers; contrast L3.]
      - https://www.owasp.org/index.php/Authentication_Cheat_Sheet
        [Note that this advises 8 characters as the minimum password length.]
      - https://crackstation.net/hashing-security.htm
        [Just general well-known stuff on salting/hashing.]

    - It's unclear where "10 characters" came from. Also, bcrypt is a
      deliberately slow algorithm that makes bulk hashing hard. Anyway...

    - RESPONSE:

      - ``MINIMUM_PASSWORD_LENGTH`` changed from 8 to 10 (server and client).
      - Advisory note added (on server).
      - Server and client passwords checked against NCSC-endorsed deny list
        (https://www.ncsc.gov.uk/blog-post/passwords-passwords-everywhere).

  - M2. "Out-of-date software versions."

    - This related to the front-end server, not CamCOPS. The recommendation was
      to upgrade from Apache 2.4.41, based on
      https://httpd.apache.org/security/vulnerabilities_24.html -- on
      2021-04-30, the latest security-fix version is 2.4.44.

    - RESPONSE:

      - The latest version of Apache 2.4.41 packaged with Ubuntu has fixes for
        the two vulnerabilities (CVE-2020-1927 and CVE-2020-1934) backported
        (http://changelogs.ubuntu.com/changelogs/pool/main/a/apache2/apache2_2.4.41-4ubuntu3.1/changelog)
      - No action required

  - M3. "Forced browsing via URL manipulation."

    - This relates to the ability for non-privileged users to visit the
      ``/view_ddl`` path, which shows the structure of the database used by
      CamCOPS.

    - "User has no explicit mapping to /view_ddl under normal circumstances."
      "Visiting DDL reveals sensitive information to low privilege user."

    - References given:

      - https://www.owasp.org/index.php/Forced_browsing
        ["Forced browsing is an attack where the aim is to enumerate and access
        resources that are not referenced by the application, but are still
        accessible."]
      - http://searchsecurity.techtarget.co.uk/answer/Forced-browsing-Understanding-and-halting-simple-browser-attacks

    - The relevant view is :func:`camcops_server.cc_modules.webview.view_ddl`.
      The menu item is in ``camcops_server/templates/menu/main_menu.mako``.

    - RESPONSE: we disagree that this was a vulnerability, as this is
      open-source software and the entire database structure is a matter of
      public record. Denying it is only "security through obscurity". However,
      the menu items are only presented to users with "dump" authority
      (``User.authorized_to_dump``), so it is consistent to restrict to that
      group. Restricted accordingly.

  - L1. Software enumeration.

    - They advise against releasing any software version information in server
      response headers or in banners of other services.

    - Reference:
      https://owasp.org/www-project-web-security-testing-guide/latest/4-Web_Application_Security_Testing/01-Information_Gathering/02-Fingerprint_Web_Server
      [re web server version information, specifically].

    - Weaknesses were:

      - "Server: gunicorn/<VERSION>" in a happy (HTTP 200 OK) response.
      - "<address>Apache/<VERSION> (Ubuntu) Server at <HOSTNAME> Port
        443</address>" from a HTTP 404 Not Found page.

    - CherryPy already had this locally configurable (with a default providing
      no version information) via the :ref:`CHERRYPY_SERVER_NAME
      <CHERRYPY_SERVER_NAME>` variable.

    - The HTTP spec is:

      - https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Server
      - https://tools.ietf.org/html/rfc7231#section-7.4.2

    - So, for Gunicorn:

      - https://stackoverflow.com/questions/16010565/how-to-prevent-gunicorn-from-returning-a-server-http-header
      - https://adamj.eu/tech/2021/01/03/override-gunicorns-server-header-from-django/
      - https://github.com/benoitc/gunicorn/issues/825

    - RESPONSE:

      - gunicorn upgraded to a version that doesn't show its version; it still
        shows "gunicorn" but that is reasonable as part of the spec.

      - Any "not found" error within the CamCOPS path, i.e. responded to by
        CamCOPS, follows the same rules as successful page loads, so is OK.

      - Any "not found" error outside that path is the responsibility of the
        "enclosing" web server (e.g. Apache) and is outside the scope of
        CamCOPS.

  - L2. "Missing server security headers."

    - Missing:

      - ``X-XSS-Protection``
      - ``Content-Security-Policy``
      - ``Strict-Transport-Security``

    - References:

      - https://scotthelme.co.uk/hardening-your-http-response-headers/
        [gives some content security policy advice]
      - https://owasp.org/www-project-secure-headers/
        [helpful defaults for all sorts of things; NB also `Venom
        <https://github.com/ovh/venom>`_]

    - The tricky bit was CSP. Achieved, without any dangerous exceptions, by
      using nonces for inline Javascript/CSS. We need on-the-fly CSS
      generation, and embedded Javascript, for the PDF system, so they can't be
      split into separate files.

    - To replace

      .. code-block:: html

        <body onload="some_function();">...</body>

      use e.g.

      .. code-block:: html

        <script nonce="xxx" type="text/javascript">
            window.onload = some_function;
        </script>

      or

      .. code-block:: html

        <script nonce="xxx" type="text/javascript">
            document.addEventListener("DOMContentLoaded", some_function, false);
        </script>

      as per
      https://stackoverflow.com/questions/7561315/alternative-to-body-onload-init/7561332

    - RESPONSE: Implemented, as far as possible. See also below.

    - Problem: Deform adds both ``<script>`` and ``<style>`` elements to
      some of its widgets. These don't have the nonce, so don't execute. And
      the ``<script>`` and ``<style>`` tags are quite embedded, e.g. in
      ``deform/templates/form.pt``, with no obvious extensible options. See
      https://github.com/RudolfCardinal/camcops/issues/162. At the moment,
      CamCOPS does not use "maximum security" CSP headers. However, when Deform
      supports this (https://github.com/Pylons/deform/issues/512), we can turn
      them up via the ``DEFORM_SUPPORTS_CSP_NONCE`` switch in CamCOPS.

  - L3. "Form auto-complete active."

    - They dislike having the ``autocomplete`` attribute permitted for password
      fields.

    - Their reference:
      https://portswigger.net/kb/issues/00500800_password-field-with-autocomplete-enabled.

    - However, there is disagrement, e.g. based on the following references.

      - There is security debate on both sides of this argument (the dangers of
        local storage versus the dangers of what users do if their browser
        doesn't provide password management functions). That is, some people
        argue that this advice is wrong on security grounds. Even the security
        advisory they link to notes that browsers may, and generally do, ignore
        the directive they suggest, ``autocomplete="off`` (for these reasons).
      - For example, see
        https://developer.mozilla.org/en-US/docs/Web/Security/Securing_your_site/Turning_off_form_autocompletion#the_autocomplete_attribute_and_login_fields.
      - Some casual discussion that summarises this is at
        https://security.stackexchange.com/questions/34067/ -- that one should
        disable autocomplete to please auditors, but not for good security!
      - Another professional penetration testing view is at
        https://www.pivotpointsecurity.com/blog/autocomplete-and-application-security-testing/.

    - RESPONSE: As it happens, we already make this user-configurable via the
      :ref:`DISABLE_PASSWORD_AUTOCOMPLETE <DISABLE_PASSWORD_AUTOCOMPLETE>`
      option, for which the default is true (i.e. the default is to set
      ``autocomplete="off"`` for the password field).

  - L4. "Logout button not present on authenticated pages."

    - They want a highly visible logout button on every authenticated page,
      because this makes it more likely that users will log out.

    - Reference:
      https://owasp.org/www-project-web-security-testing-guide/latest/4-Web_Application_Security_Testing/06-Session_Management_Testing/06-Testing_for_Logout_Functionality.
      [best practice being described as "directly visible", not "highly
      visible", and visible without scrolling].

    - RESPONSE: Implemented.

  - L5. "Concurrent user sessions."

    - Allowing a user to have multiple sessions means are less likely to notice
      if someone else has hacked their account. This is a tradeoff between
      security and usability. Their recommendation is only one active session
      (of course, you can have multiple tabs).

    - References:

      - https://www.owasp.org/index.php/Testing_for_Session_Management
      - https://www.owasp.org/index.php/Session_Management_Cheat_Sheet
        [section on "Simultaneous Session Logins"]

    - Note that in particular we want to be able to have a user running
      a webviewer session but simultaneously being able to upload from one or
      maybe several client devices (which will have different IP addresses).
      We use :class:`camcops_server.cc_modules.cc_session.CamcopsSession` to
      represent sessions.

    - RESPONSE: now only one "human" session at a time; if you log in again,
      previous sessions are terminaed. (However, you can have an ongoing human
      session that is not terminated by one or many API-based uploads.)

  - L6. "Out-of-date jQuery version."

    - jQuery was 2.0.3, which has some known vulnerabilities.
    - They note that attacks via this route are highly complex, and require
      some other conditions to exploit, and those other conditions were not
      found.
    - References:

      - http://code.jquery.com/jquery/ [repository of jQuery code by version]
      - https://jquery.com/download/ [how to download]
      - https://snyk.io/vuln/npm:jquery [list of vulnerabilities by version]
      - http://www.cvedetails.com/vulnerability-list/vendor_id-6538/Jquery.html
        [another index of vulnerabilities]

    - Now, jQuery is being used via Deform
      (https://docs.pylonsproject.org/projects/deform/;
      https://pypi.org/project/deform/), and via our ``/deform_static`` path,
      which we redirect to ``deform:static``, which means "look within the
      deform package for the directory static/".

    - jQuery was 2.0.3, via ``jquery-2.0.3.min.js``, loaded from our
      ``base_web.mako``. This follows the advice at
      https://docs.pylonsproject.org/projects/deform/en/2.0-branch/basics.html.

    - A simple replacement of the jQuery library with v3.6.0 doesn't work (for
      example, on the "filter tasks" page, the "expanding" categories don't
      expand).

    - RESPONSE: not changed; low concern noted; but raised as a Deform issue at
      https://github.com/Pylons/deform/issues/511, and tracked at
      https://github.com/RudolfCardinal/camcops/issues/161.

- Also fixed REDCap example for PHQ9 (from ``task.q10 + 1``, which could fail
  if ``q10`` was ``None``, to ``task.q10 + 1 if task.q10 is not None else
  None``).

- Also, now Deform is at 2.0.15, it has provided all its font files. So we can
  delete the hacks for the path
  ``/deform_static/fonts/glyphicons-halflings-regular.woff2`` and corresponding
  code (``bugfix_deform_missing_glyphs``). The path still works, but now it
  goes to Deform's static files.

- Removed title string from
  :class:`camcops_server.cc_modules.cc_forms.CSRFToken`. Generally that was
  hidden but it appeared in some circumstances.

**Client and server v2.4.7, IN PROGRESS**

- Default filters in Mako templates, via the ``default_filters`` argument, as
  this is a safer method. As a result, anything with ``| h`` filtering now has
  none, and anything that had none now has ``| n`` (no filtering). The system
  is described at https://docs.makotemplates.org/en/latest/filtering.html (and
  some clarity about permitted filter names is added by
  http://b.93z.org/notes/automatic-html-escaping-in-mako/).
