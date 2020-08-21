..  docs/source/administrator/server_third_party_installation.rst

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


(MANUAL) Installing third-party software
========================================

.. include:: include_this_is_manual_see_docker.rst

This sections contains some instructions and tips on installing third-party
software. It's particularly relevant for Windows, which doesn't have a good "X
depends on Y" software package system.

..  contents::
    :local:
    :depth: 3


.. _windows_install_python:

Installing Python (with tkinter) for Windows
--------------------------------------------

Install Python (https://www.python.org); for example, Python 3.6.7 for Windows.
If you have a 64-bit computer, use the "x86-64" version. Download and run the
installer. (These days, the installer includes Tk/tkinter without asking, which
is helpful.) By default, it will install to your user directory, so consider
customizing the installation as follows:

Optional features:

- [✓] Documentation
- [✓] pip
- [✓] tcl/tk and IDLE
- [✓] Python test suite
- [✓] py launcher [✓] for all users (requires elevation)

Advanced options:

- [✓] Install for all users
- [✓] Associate files with Python (requires the py launcher)
- [✓] Create shortcuts for installed applications
- [✓] Add Python to environment variables
- [✓] Precompile standard library
- Customize install location = ``C:\Python36``

Also, you may want to allow the installer to extend the system ``MAX_PATH``
length limit
(https://python.readthedocs.io/en/stable/using/windows.html#removing-the-max-path-limitation).



.. _windows_install_imagemagick:

Installing ImageMagick for Windows
----------------------------------

See
http://docs.wand-py.org/en/latest/guide/install.html#install-imagemagick-on-windows.

Note that for Wand 0.4.x, you need ImageMagick 6.x (7.x won't work).
ImageMagick 7 support is in Wand 0.5, as yet unreleased as of 2018-12-02.

If, despite installing ImageMagick, CamCOPS fails to start regardless with the
message:

.. code-block:: none

    ImportError: MagickWand shared library not found.
    You probably had not installed ImageMagick library.
    Try to install:
      http://docs.wand-py.org/en/latest/guide/install.html#install-imagemagick-on-windows

then

- one possibility is that your Python interpreter and your ImageMagick
  libraries do not match in terms of 32- versus 64-bitness.

  To check Python, run ``python`` then check if ``sys.maxsize > 2**32`` (see
  https://stackoverflow.com/questions/1405913/); if so, it's 64-bit Python. To
  check ImageMagick, a quick way is to run its ImageMagick Display (IMDisplay)
  program, then :menuselection:`Help --> About`.

- Another possibility is that you failed to tick **"Install development headers
  and libraries for C and C++"** (see the Wand instructions). Retry with that
  ticked.

- ImageMagick 7.x doesn't work with Wand 0.4.x; you need ImageMagick 6.x (e.g.
  6.9.10) (see https://stackoverflow.com/questions/25003117/;
  http://docs.wand-py.org/en/latest/changes.html). Binary downloads are at
  https://www.imagemagick.org/download/binaries/. This fixed it for us. Use
  e.g.
  ``https://imagemagick.org/download/binaries/ImageMagick-6.9.10-14-Q16-x64-dll.exe``
  for a 64-bit system.



.. _windows_install_sql_server:

Installing SQL Server for Windows
---------------------------------

A short guide to installing the Developer edition of SQL Server:

- SQL Server 2017 Developer Edition won't install with the Visual C++ 2017
  redistributables installed (see
  https://dba.stackexchange.com/questions/190090/), so uninstall that
  first.

- Install the free SQL Server 2017 Developer Edition from
  https://www.microsoft.com/en-us/sql-server/sql-server-downloads; basic
  install, default options. After installation, it should say "Installation
  has completed successfully!" and "SQL Server Configuration Manager"
  should be available as a program. The defaults are an instance name of
  ``MSSQLSERVER``, and a connection string of
  ``Server=localhost;Database=master;Trusted_Connection=True;``. The
  "Connect Now" button should work. The Windows service "SQL Server
  (MSSQLSERVER)" should be present and running. (Re-install if it doesn't
  get that far the first time!)

- Install SSMS (SQL Server Management Studio) too (e.g.
  https://docs.microsoft.com/en-us/sql/ssms/download-sql-server-management-studio-ssms).
  May need to reboot then restart the installer. After installation,
  "Microsoft SQL Server Management Studio" should be available.

- Run SSMS and provide "localhost" as the server name; this should connect.


.. _windows_create_sql_server_database:

Creating an SQL Server database with an ODBC connection
-------------------------------------------------------

First install SQL Server (see :ref:`Installing SQL Server for Windows
<windows_install_sql_server>`). Then, to create a database named
``camcops_db`` and create an ODBC connection to it:

- Create a database named ``camcops_db``: :menuselection:`[right-click]
  Databases --> New database` and supply the name.

- Create a user named ``camcops_user``: :menuselection:`Security -->
  [right-click] Logins -> New login`; supply the name; choose "SQL Server
  authentication" and specify a password; untick "User must change password at
  next login".

- Give the user permission to access the database. Right-click the new user and
  choose "Properties". Under "User Mapping", tick the "Map" tickbox for the
  ``camcops_db`` database. In the box marked "Database role membership for:
  camcops", tick ``db_owner`` or some other suitable combination (e.g.
  ``db_ddladmin + db_datareader + db_datawriter``). See
  https://docs.microsoft.com/en-us/sql/relational-databases/security/authentication-access/database-level-roles?view=sql-server-2017.

- Ensure the server allows logins via username/password combinations.

  - Right-click the top-level database object in the SSMS tree.
  - :menuselection:`Properties --> Security`
  - Ensure "Server authentication" is set to "SQL Server and Windows
    Authentication mode" (not "Windows Authentication mode").
  - Restart SQL Server (from Windows Services; the "SQL Server (MSSQLSERVER)"
    services).

  Without this, you will get errors like ``[Microsoft][ODBC Driver 13 for SQL
  Server][SQL Server]Login failed for user 'camcops_user'. (18456)``.

- Create an ODBC data source.

  - :menuselection:`Start --> ODBC Data Sources (64-bit)`.
  - :menuselection:`System DSN --> Add --> ODBC Driver 13 for SQL Server`.
  - Let's call this data source ``camcops_dsn``.
  - Give it a description (e.g. "CamCOPS test database").
  - The SQL Server will be "localhost". Next.
  - Use "SQL Server authentication using a login ID and password entered by
    the user". Next.
  - "Change the default database to" ``camcops_db``. Next. Finish.
  - As you're saving it, you'll see that it has not enabled Multiple Active
    Result Sets (MARS), and you were not offered the option to do so.
  - Therefore, you also need to do this from a *privileged* Windows command
    prompt (via "run as administrator"):

    .. code-block:: bat

        odbcconf /a {CONFIGSYSDSN "ODBC Driver 13 for SQL Server" "DSN=camcops_dsn|MARS_Connection=Yes"}

  - To check it worked, run *ODBC Data Source Administrator (64-bit)* again,
    choose and configure your DSN again, and click "Next" until you get to the
    end, without changing anything; you should now see that MARS is enabled.

If you use the ``pyodbc`` driver for SQLAlchemy, the SQLAlchemy URL for the
database should now be:

.. code-block:: none

    mssql+pyodbc://camcops_user:PASSWORD@camcops_dsn


SQL Server tips
---------------

Show running queries
~~~~~~~~~~~~~~~~~~~~

Modified from
https://blog.sqlauthority.com/2009/01/07/sql-server-find-currently-running-query-t-sql/:

.. code-block:: sql

    SELECT
        sqltext.text,
        req.start_time,
        req.session_id,
        req.status,
        req.command,
        req.cpu_time,
        req.total_elapsed_time  -- this is in milliseconds
    FROM sys.dm_exec_requests req
    CROSS APPLY sys.dm_exec_sql_text(req.sql_handle) AS sqltext
    ORDER BY req.start_time ASC

For details, see
https://docs.microsoft.com/en-us/sql/relational-databases/system-dynamic-management-views/sys-dm-exec-requests-transact-sql?view=sql-server-2017.

Note that this query contributes exactly one row to its own results.



.. _sql_server_delete_takes_forever:

DELETE takes forever
~~~~~~~~~~~~~~~~~~~~

(By "forever" I mean at more than half an hour to delete zero rows.)

- Lots of foreign key checks? See

  - https://stackoverflow.com/questions/56070/delete-statement-hangs-on-sql-server-for-no-apparent-reason
  - https://stackoverflow.com/questions/10901299/delete-statement-in-sql-is-very-slow

- Use the query above to show all running queries and find the ``session_id``
  for the query that's freezing.

  To show more detail for that session:

  .. code-block:: sql

    SELECT *
    FROM sys.dm_exec_requests
    WHERE session_id = <session_id>

In an example we had, a query ``DELETE FROM _idnum_index`` was taking a
phenomenally long time and was suspended; serially, a lot of queries were being
executed like ``SELECT tr.name AS [Name], tr.object_id AS [ID] FROM
sys.triggers AS tr WHERE (tr.parent_class = 0) ORDER BY [Name] ASC``. So that's
an indication that the ``DELETE`` is causing a large set of triggers to be
searched.

- Remember that any working CamCOPS server its DDL (for any supported database
  engine), so you can use a working Linux/MySQL server to show DDL for SQL
  Server.

- Remember the ``DB_ECHO`` parameter in the CamCOPS config file for "routine"
  SQL, and the ``--show_sql_only`` parameter to the ``upgrade_db`` command.

Potential solutions:

- https://stackoverflow.com/questions/155246/how-do-you-truncate-all-tables-in-a-database-using-tsql#156813

- https://stackoverflow.com/questions/123558/sql-server-2005-t-sql-to-temporarily-disable-a-trigger#123966


MySQL tips
----------

Create an entity relationship (ER) diagram for a MySQL database
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In MySQL Workbench, :menuselection:`Database --> Reverse Engineer`. Choose
the connection and database. The default is to create a diagram of all tables.
At the "Select Objects to Reverse Engineer / Import MySQL Table Objects" stage,
click "Show Filter" to restrict which tables are used (left column to include,
right column to exclude).
