..  docs/source/misc/to_do.rst

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

Things to do
============

..  contents::
    :local:
    :depth: 3


Tasks
-----

**Priority: IAM**

- :ref:`Childhood Trauma Questionnaire, Short Form (CTQ-SF) <ctqsf>`

  .. todo:: IN PROGRESS +++ a/w funding before proceeding


**To be prioritized**

- Test Your Memory (TYM) task (Jerry M. Brown).
- Continuous performance task, as per RNC's similar previous task (Linda P
  and team). A/w ETB.


**Then**

- Lewy body dementia checklist -- need details from JO'B, as per 14 Jan 2018
  meeting.


**Not a priority**

- Cardinal_ExpDet* tasks: generate noise on the fly?

- PDSQ screener (see Clark talk 2018-09-20, MQ Data Science)


**Consider**

- review Guo 2015 "measurement-based care":

  - HADS/HDRS already
  - QIDS -- permissible?
  - Maudsley Treatment Inventory?
  - see notes on Cleare lecture 15/11/2018

- new task: ReQoL (https://www.reqol.org.uk/p/overview.html)
- new task: mini-ACE (subset of the ACE-III)
- new task: Andy Foster / eating disorders; e-mail of 24/5/16
- new task: AQ10 autistic spectrum screening
- discarded tasks - revitalize: ASRM
- discarded tasks - revitalize: BARS
- discarded tasks - revitalize: BFCRS
- discarded tasks - revitalize: CSI
- discarded tasks - revitalize: FAB
- discarded tasks - revitalize: GASS
- discarded tasks - revitalize: LSHSA
- discarded tasks - revitalize: LSHSLAROI2005
- discarded tasks - revitalize: LUNSERS
- discarded tasks - revitalize: MADRS
- discarded tasks - revitalize: SAS


Client core
-----------

**Priority**

- Create 64-bit ARM build, then release to Google Play Store (deadline 1 Aug
  2019). Work in progress: ``build_qt.py --build_android_arm_v8_64``.

- Test task upload (and date filtering) under Windows/SQL Server.

- "Single patient" mode to go along with task scheduling.

  - e.g. activate CamCOPS at first use with a URL to a server including a
    security token; this should then lead to the CamCOPS client registering
    with the server and knowing the owner's identity
  - in single-patient mode, automatic fetching of scheduling info (but
    must cope if network down)
  - in single-patient mode, automatic uploading of completed tasks (but
    must cope if network down)
  - facility to switch in and out of single-patient mode
  - helpful summary of "things you need to do", so it walks the user through
  - **See "server" below.**

**Medium priority**

- iOS build.

- Apple App Store.

- Have facility to upload and/or automatically feed patient details into the
  server, then have clients restrict to these predefined patients. Since we are
  aiming to minimize PID on the client, this could be implemented by having the
  client validate its patients with the server, and refusing to upload if they
  don't match. This would be a per-group setting.

  - Client validation check implemented.
  - Just needs server-side extensions to
    :func:`camcops_server.cc_modules.cc_patient.is_candidate_patient_valid`,
    including a per-group setting for "should we validate", and a way of
    getting suitable data in.
  - Note that any patient unification should be external to CamCOPS (i.e.
    addressing the question of "are these two patients the same person").
  - Maybe implement as (1) a ``_known_patients`` table and (2) a
    ``_known_patient_idnums`` table?
  - Needs slightly more thought about what constitutes a "match" given a
    variable set of input and a variable set of known information.
    (Could have an ID policy for the known information too...)
  - Should "known" patients be across groups, or per-group?

**Not a priority**

- MacOS build.

- Think about a web-based client, e.g. via VNC (but this is complex and loads
  servers/networks considerably). Potentially more promising is Qt for
  WebAssembly (in preview May 2018), which compiles to a variety of portable
  quasi-assembly language; the browser downloads and runs it. However, at
  present there is no threading or DNS lookup
  (http://blog.qt.io/blog/2018/05/22/qt-for-webassembly/).

- Desktop-style menu for desktop clients. (Faster to navigate around.)

- Current Android back button behaviour may not be optimal.

- Maybe implement pinch zoom for some subclasses of OpenableWidget, e.g.
  MenuWindow and Questionaire. See
  http://doc.qt.io/qt-5/qtwidgets-gestures-imagegestures-example.html

- QuAudioRecording: questionnaire element to record audio

- QuVideoRecording: questionnaire element to record video

- Qt have fixed bug https://bugreports.qt.io/browse/QTBUG-35545 as of Qt
  5.12.0 beta 1, so may be possible to improve dialogue boxes again on Android
  (but possibly our workaround sorted it; can't remember); check.


Server
------

**Priority**

- (SERVER + CLIENT) Concept of “tasks that need doing” in the context of a
  research study.

  - define patients on server (per group)

    - share main patient/patient_idnum tables

    - use the “server device” to create them, and always in era “NOW”

  - ScheduledTask -- "task needs doing"

    - patient (by ID number); group; task; due_from; due_by;
      skip_if_not_done_by; cancelled?

    - Example: "PHQ9 due for Mr X after 1 July; due by 7 July; must be
      completed by 1 Aug"

  - then for metacreation: “StudySchedule” or “TaskPanel”

    - ... a list of tasks, each with: task; due_from_relative_to_start_date;
      due_by_relative_to_start_date

    - example: “In our study, we want a PHQ9 and GAD7 at the start, a PHQ9 at
      3 months, and a PHQ9 and GAD7 at 6 months.”

  - PatientSchedule

    - instantiate a “StudySchedule”/“TaskPanel” with patient, group, start date

    - e.g. “Mr Jones starts today.... enrol!”

  - ALTERNATIVELY: do we need ScheduledTask if the main thing is a person/panel
    association?

  - Tablets should fetch “what needs doing” for any patients defined on the
    tablet, and display them nicely.
  - Tasks must be complete to satisfy the requirement.

  - Database field type: represent :class:`pendulum.Duration` in ISO-8601
    format, which is ``P[n]Y[n]M[n]DT[n]H[n]M[n]S``. The
    ``pendulum.Duration.min`` and ``pendulum.Duration.max`` values are
    ``Duration(weeks=-142857142, days=-5)`` and ``Duration(weeks=142857142,
    days=6)`` respectively. A possible database output standard is
    ``PT[x.y]S``, with floating-point seconds; this maps from the
    :func:`pendulum.Duration.total_seconds` function.

    - See new functions :func:`cardinal_pythonlib.datetimefunc.duration_to_iso`
      and :func:`cardinal_pythonlib.datetimefunc.duration_from_iso`.

    - New column type
      :class:`camcops_server.cc_modules.cc_sqla_coltypes.PendulumDurationAsIsoTextColType`.

- … Relating to that: consider, on the client, a “single-patient” mode
  (distinct from the current “researcher” mode), tied to a specific server.
  “This tablet client is attached to a specific patient and will operate in a
  patient-friendly, single-patient mode. Show me what needs completing.” The
  operating concept would be: if you would like someone geographically far away
  to be able to download CamCOPS and complete a set of tasks for you, how could
  you organize so that would be simplest for them? The minimum would that you’d
  create login details for them, and give them a URL, username, and password.
  **See "client" above.**

- What's the optimal packaging method for the server? Is it DEB/RPM for Linux,
  and PyInstaller + Inno Setup (or just Inno Setup) for Windows?

- Improve installation ease and docs.

**Medium**

- Self-test: possible to avoid recreating database structure each time? Makes
  it very slow.
  (MB 2020-08-19: Certainly possible with pytest / pytest-django. I haven't
  checked if there's an equivalent for pyramid. No doubt this would require a
  lot of work up front and would be good not to have to maintain our own test
  runner.)

**Not a priority**

- Consider: see ``DEBUG_TEMPLATE_SOURCE`` -- would it improve performance to
  have a Mako template cache directory always set, via the config file? (There
  is still memory caching at present.)

- Fix Alembic migration autogeneration -- too much non-change junk.

- Tracker improvements.

  - In
    :meth:`camcops_server.cc_modules.cc_tracker.Tracker.get_all_plots_for_one_task_html`,
    consider improvements to allow tracker information to be associated with
    a user-specified date (see e.g. GBO), rather than the creation time (with
    fallback to the creation time if not specified).

  - Consider cross-task trackers, e.g. GBO-GPC and GBO-GRaS both contributing
    to a "goal 1 progress" tracker. Simplest way might be to collect specimen
    and x/y information from all tasks, keyed by tracker name, with some
    defaults for existing trackers?

- Implement (from command line) “export to anonymisation staging database” =
  with patient info per table. (Extend ``cc_dump.py``. See
  ``generate_anonymisation_staging_db()``, and it's also temporarily disabled
  in the master command-line handler.) Framework very partly done; search for
  ``db_patient_id_per_row``.

  - Best to implement by fixed column names for all ID numbers, e.g.
    ``_patient_idnum1``, ``_patient_idnum17``, etc.? NULL if absent.

- FHIR support via ``fhirclient``.

  - https://en.wikipedia.org/wiki/Fast_Healthcare_Interoperability_Resources
  - https://www.hl7.org/fhir/overview.html
  - CamCOPS will be a FHIR server, not a client.

- More generic e-mails to administrators, via backend task. (E-mail framework
  now in place.)


Documentation
-------------

- Finish manual esp. web site user guide.


Wishlist and blue-sky thoughts
------------------------------

**Server-side “all tasks in full” view, like a clinical text view but for researchers?**

A “research multi-task view” would be an easy extension to the task collection
classes used for trackers and CTVs, if there is demand.

**Improvements to “camcops merge_db” facility**

The merge facility doesn’t yet allow you to say “ID#8 in database A means
something different to ID#8 in database B; don’t merge that”. Should it?
(Example: “research ID” that is group-specific, versus “NHS number” that
isn’t.) More generally: should some ID numbers be visible only to certain
groups?

**Server-side ability to edit existing (finalized) task instances?**

Would be done in a generic way, i.e. offer table with {fieldname, comment, old
value, new value}; constrain to min/max or permitted values where applicable;
at first “submit”, show differences and ask for confirmation; audit changes.
For BLOBs, allow option to upload file (or leave unchanged).

**Client-side index of tasks by patient ID, to speed up lookup on the tablet?**

Might be worthwhile on the client side as the number of tasks grows. (The
server already has indexing by patient ID.)

**MRI triggering on task side**

For example: CamCOPS tasks running on a desktop and communicating via TCP/IP
with a tool that talks to an MRI scanner for pulse synchronization and
response.


Considered but rejected
-----------------------

- Client-side task index, to speed up the client's patient summary view. (This
  is not a performance problem!)

- Tasks record the language operational on the client at the moment of their
  creation. (Would need the client to remove this field for older server
  versions at the moment of upload.) A reason not to: users can switch language
  mid-way, and we're not going to track all those potential changes.


Documentation to-do list
------------------------

Things to do collected from elsewhere in this documentation:

.. todolist::
