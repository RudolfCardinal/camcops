..  docs/source/misc/licenses.rst

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

.. |denovo| replace:: *de novo*


Licences
========

This section gives citations to, and licence details for, software used by
CamCOPS.

..  contents::
    :local:
    :depth: 3


.. _licences_camcops:

CamCOPS
-------

..  literalinclude:: licence_camcops.txt
    :language: none


.. _licences_other:
.. _licences_qt:

Qt
--

The CamCOPS client (tablet/desktop app) is written using the Qt C++ framework.
See https://www.qt.io/.

Qt is used here under the LGPL. See:

- https://www.qt.io/qt-licensing-terms/

  - note that some parts of Qt are only available under the GPL for open-source
    users: https://www.qt.io/licensing-comparison/

- https://doc.qt.io/qt-5/licenses-used-in-qt.html

- https://www.gnu.org/licenses/lgpl-3.0.en.html


OpenSSL
-------

Qt uses OpenSSL for its cryptography. See https://www.openssl.org/.

This product includes software developed by the OpenSSL Project for use in the
OpenSSL Toolkit (http://www.openssl.org/).

..  literalinclude:: licence_openssl_ssleay.txt
    :language: none


OpenSSL License
~~~~~~~~~~~~~~~

..  literalinclude:: licence_openssl.txt
    :language: none


Original SSLeay License
~~~~~~~~~~~~~~~~~~~~~~~

..  literalinclude:: licence_ssleay.txt
    :language: none


SQLCipher
---------

CamCOPS uses SQLCipher for encrypted SQLite databases. See
https://www.zetetic.net/sqlcipher/.

..  literalinclude:: licence_sqlcipher.txt
    :language: none


Eigen
-----

The CamCOPS client uses Eigen for matrix algebra (e.g. for implementing
generalized linear models). See http://eigen.tuxfamily.org.

- Guennebaud G, Jacob B, et al. (2010). Eigen v3. http://eigen.tuxfamily.org

- Eigen is free software licensed under the Mozilla Public License (MPL) v2.0
  (https://www.mozilla.org/en-US/MPL/2.0/); see
  http://eigen.tuxfamily.org/index.php?title=Main_Page#License.


FloatingPoint
-------------

The CamCOPS client uses Google's FloatingPoint class for “nearly equal”
testing. This is from the Google C++ Testing Framework.

See:

- https://stackoverflow.com/questions/17333/what-is-the-most-effective-way-for-float-and-double-comparison
- https://raw.githubusercontent.com/google/googletest/master/googletest/include/gtest/internal/gtest-internal.h
- https://raw.githubusercontent.com/google/googletest/master/googletest/include/gtest/internal/gtest-port.h

..  literalinclude:: licence_floatingpoint.txt
    :language: none


QCustomPlot
-----------

The CamCOPS client uses QCustomPlot for some graphs. This is licensed under the
GNU GPL v3+ (as for CamCOPS). See https://www.qcustomplot.com/.


.. _licence_snomed:

SNOMED CT
---------

CamCOPS does not include SNOMED CT codes, but supports them if a local
administrator is permitted to install them and does so. See :ref:`SNOMED CT
<snomed>`.

The licensing terms are reproduced here for convenience, but the original
should be checked in all situations:

..  literalinclude:: licence_snomed_ct.txt
    :language: none

Note that SNOMED International define a SNOMED CT identifier as:

    "A unique integer identifier applied to each SNOMED CT component (Concept,
    Description, or Relationship)."

    (https://confluence.ihtsdotools.org/display/DOCGLOSS/SNOMED+CT+Identifier,
    accessed 2019-11-21.)


Sounds
------

For sounds relating to specific tasks, see each task’s information file. For
the CamCOPS general sounds:

- Sound test 1 (bach_brandenburg_3_3.mp3):

  - excerpt from Bach JS, *Brandenburg
    Concerto No. 3, third movement (Allegro)*, by the Advent Chamber Orchestra,
    from
    `<http://freemusicarchive.org/music/Advent_Chamber_Orchestra/Selections_from_the_2005-2006_Season/>`_;

  - licensed under the EFF Open Audio License
    (https://commons.wikimedia.org/wiki/EFF_OAL), reported by the source site
    as equivalent to CC-BY-SA-2.0
    (https://creativecommons.org/licenses/by-sa/3.0/us/).

- Sound test 2 (mozart_laudate.mp3):

  - excerpt from Mozart WA, *Vesperae solennes
    de confessore* (K.339), fifth movement, *Laudate Dominum*, by the Advent
    Chamber Orchestra, from
    `<http://freemusicarchive.org/music/Advent_Chamber_Orchestra/Selections_from_the_December_2006_Concert/Advent_Chamber_Orchestra_-_11_-_Mozart_-_Laudate_Dominum>`_;

  - licensed under the EFF Open Audio License
    (https://commons.wikimedia.org/wiki/EFF_OAL), reported by the source site
    as equivalent to CC-BY-SA-2.0
    (https://creativecommons.org/licenses/by-sa/3.0/us/).

- Other sounds generated |denovo| in Audacity (http://www.audacityteam.org/).


Images
------

For images relating to specific tasks, see each task’s information file. For
the CamCOPS general images:

..  Something about URLs makes Sphinx go wrong with e.g.
    WARNING: Block quote ends without a blank line; unexpected unindent.
    The practical answer seems to be to stop word-wrapping the lines in the
    table that complain.
..  More generally, sometimes URLs with underscores in generate errors about
    "bad target name" or similar. Try replacing http://dodgy_url with
    `<http://dodgy_url>`_

=============================== ===============================================
File                            Source
=============================== ===============================================
addiction.png	                Cigarette symbol from
                                https://openclipart.org/detail/23552/cigarette-symbol
                                (public domain, as per https://openclipart.org/share).
                                Glass from
                                https://commons.wikimedia.org/wiki/File:Wheat_beer_glass_silhouette.svg
                                (by BenFrantzDale~commonswiki, CC-SA-3.0).
                                Rest |denovo|.
add.png	                        |denovo|
affective.png	                Modified from
                                https://commons.wikimedia.org/wiki/File:Drama-icon.svg
                                (by User:Booyabazooka; GFDL).
alltasks.png	                |denovo|
anonymous.png	                |denovo|
back.png	                    |denovo|
branch-closed.png	            |denovo|
branch-end.png	                |denovo|
branch-more.png	                |denovo|
branch-open.png	                |denovo|
camcops.png	                    Brain from `<http://www.public-domain-photos.com/free-cliparts/people/bodypart/brain_jon_phillips_01-4366.htm>`_ (public domain). Rest |denovo|.
camera.png	                    |denovo|
cancel.png	                    |denovo|
catatonia.png	                After Cardinal RN, Everitt BJ. Neural systems
                                of motivation. Encyclopedia of Behavioral
                                Neuroscience, Elsevier/Academic Press, Oxford.
chain.png	                    |denovo|
check_disabled.png	            |denovo|
check_false_black.png	        |denovo|
check_false_red.png	            |denovo|
check_true_black.png	        |denovo|
check_true_red.png	            |denovo|
check_unselected.png	        |denovo|
check_unselected_required.png	|denovo|
choose_page.png	                |denovo|
choose_patient.png	            |denovo|
clinical.png	                |denovo|
cognitive.png	                |denovo|
delete.png	                    Pencil modified from http://www.clker.com/clipart-pencil-28.html> (public domain, as per http://www.clker.com/disclaimer.html). Rest |denovo|.
edit.png	                    Pencil modified from http://www.clker.com/clipart-pencil-28.html (public domain, as per http://www.clker.com/disclaimer.html). Rest |denovo|.
executive.png	                Built using chess icons
                                https://commons.wikimedia.org/wiki/File:Chess_qdt45.svg,
                                https://commons.wikimedia.org/wiki/File:Chess_rlt45.svg,
                                and
                                https://commons.wikimedia.org/wiki/File:Chess_ndt45.svg
                                (by en:User:Cburnett; GFDL, BSD, and GPL).
fast_forward.png	            |denovo|
field_incomplete_mandatory.png	|denovo|
field_incomplete_optional.png	|denovo|
field_problem.png	            |denovo|
finishflag.png	                Modified from
                                http://www.clker.com/clipart-finish-flags.html
                                (public domain, as per
                                http://www.clker.com/disclaimer.html).
finish.png	                    |denovo|
global.png	                    From https://commons.wikimedia.org/wiki/File:Globe_Atlantic.svg (by the US Government; public domain).
hasChild.png	                |denovo|
hasParent.png	                |denovo|
info.png	                    Modified from https://en.wikipedia.org/wiki/File:Info_icon_002.svg (by Amada44; unrestricted use).
language.png                    From http://www.languageicon.org; open-source
                                licence terms described there and at
                                https://en.wikipedia.org/wiki/Language_Icon
locked.png	                    Modified from https://commons.wikimedia.org/wiki/File:Ambox_padlock_gray.svg (by User:HuBoro; public domain).
magnify.png	                    Modified from https://commons.wikimedia.org/wiki/File:Magnifying_glass_icon.svg (by Derferman; public domain).
next.png	                    |denovo|
ok.png	                        |denovo|
patient_summary.png	            |denovo|
personality.png	                Prism/rainbow from
                                https://commons.wikimedia.org/wiki/File:Prism-rainbow-black.svg
                                (by Suidroot; CC-SA-3.0). “Children crossing”
                                from
                                http://www.clker.com/clipart-children-crossing.html
                                (public domain, as per
                                http://www.clker.com/disclaimer.html).
privileged.png	                |denovo|
psychosis.png	                |denovo|
radio_disabled.png	            |denovo|
radio_selected.png	            |denovo|
radio_unselected.png	        |denovo|
radio_unselected_required.png	|denovo|
read_only.png	                Pencil modified from
                                http://www.clker.com/clipart-pencil-28.html
                                (public domain, as per
                                http://www.clker.com/disclaimer.html). Rest
                                |denovo|.
reload.png	                    |denovo|
research.png	                Mortarboard from
                                https://en.wikipedia.org/wiki/File:French_university_icon.svg
                                [CC-SA-3.0, by Λua∫Wise (Operibus anteire)].
                                Test tube from
                                http://www.clker.com/clipart-26081.html (public
                                domain, as per
                                http://www.clker.com/disclaimer.html).
rotate_anticlockwise.png	    |denovo|
rotate_clockwise.png	        |denovo|
sets_clinical.png	            |denovo|
sets_research.png	            Mortarboard from https://en.wikipedia.org/wiki/File:French_university_icon.svg [CC-SA-3.0, by Λua∫Wise (Operibus anteire)]. Rest |denovo|.
settings.png	                Modified from
                                https://www.clker.com/clipart-gear-grey.html
                                (public domain, as per
                                http://www.clker.com/disclaimer.html).
spanner.png	                    |denovo|
speaker_playing.png	            |denovo|
speaker.png	                    |denovo|
stairs.png                      From ``noun_Stairs_5827.svg``, from
                                https://thenounproject.com/term/stairs/5827/;
                                "Stairs" by Nathan Driskell from the Noun
                                Project; CC-BY license
                                (https://creativecommons.org/licenses/by/3.0/us/legalcode).
stop.png	                    |denovo|
thumbs.png                      Modified from
                                http://www.clker.com/cliparts/F/w/2/b/Q/2/thumbsup.svg
                                (public domain, as per http://www.clker.com/disclaimer.html).
time_now.png	                |denovo|
treeview.png	                |denovo|
unlocked.png	                Modified from
                                https://commons.wikimedia.org/wiki/File:Ambox_padlock_gray.svg
                                (by User:HuBoro; public domain).
upload.png	                    Globe from https://openclipart.org/download/121609/1298353280.svg (public domain, as per https://openclipart.org/share). Server from https://commons.wikimedia.org/wiki/File:Server-database-mysql.svg, in turn from https://commons.wikimedia.org/wiki/File:Drive-harddisk.svg (by Sasa Stefanovic; public domain).
vline.png	                    |denovo|
warning.png	                    |denovo|
whisker.png	                    |denovo|
zoom.png	                    Modified from https://commons.wikimedia.org/wiki/File:Magnifying_glass_icon.svg (by Derferman; public domain).
=============================== ===============================================
