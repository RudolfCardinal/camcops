..  docs/source/administrator/server_configuration.rst

..  Copyright (C) 2012-2019 Rudolf Cardinal (rudolf@pobox.com).
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

.. _Apache: http://httpd.apache.org/
.. _CherryPy: https://cherrypy.org/
.. _Gunicorn: http://gunicorn.org/
.. _HTTPS: http://en.wikipedia.org/wiki/HTTP_Secure
.. _Linux: http://en.wikipedia.org/wiki/Linux
.. _MySQL: http://www.mysql.com/
.. _Pyramid: https://trypyramid.com/
.. _Python: https://www.python.org/
.. _Supervisor: http://supervisord.org/
.. _Ubuntu: http://www.ubuntu.com/

.. _server_configuration:

Configuring the server
======================

The majority of CamCOPS configuration takes place via the :ref:`server
configuration file <server_config_file>`. Here, we deal with everything else.

..  contents::
    :local:
    :depth: 3

Overview
--------

- CamCOPS will operate offline on your tablet, and you can view simple
  summaries of your tasks there.

- You should set up a server to receive data from your CamCOPS tablet(s).

  - The simplest way (involving only free software) is to set up a Linux_
    (e.g. Ubuntu_) computer with an Apache_ web server and a MySQL_ database
    (plus the Python_ interpreter, which comes with most Linux
    distributions) – collectively known as a LAMP stack [#lamp]_. Alternative
    (e.g. Windows) configurations are possible, but we’ve not used them so do
    not provide details here.

  - You must also obtain an HTTPS_ certificate (or create one for free
    [#snakeoil]_ – not recommended), and configure your web server to do all
    its CamCOPS work via HTTPS only (not HTTP), so that your information is
    always encrypted in transit from the tablets to your server. The CamCOPS
    tablet app will not use unencrypted links. (Where do certificates live on
    disk? See [#linuxflavours]_.)

  - Then, CamCOPS provides software so your web server can receive data from
    your tablets, and offers a :ref:`web front end <website_general>` so you
    can view your tasks in HTML and PDF format, and download data in various
    formats.

  - For very advanced analysis, you can use the MySQL database directly.

  - You must also consider the server’s security carefully; see :ref:`Security
    <security_design>`.

- Having set up your server, you should point your tablet(s) to it (see
  :ref:`configuring the tablet application <configure_client>`).


Data flow
---------

A generic CamCOPS server is arranged like this:

.. image:: images/server_diagram.png

Most servers will use a single database and a single CamCOPS instance, because
this is simpler and more powerful. In that simpler case, ignore all reference
to “instances” in the figure above.

There are two web servers in this diagram.

- The **front-end web server** talks to the “world”. Users connect to it.
  Choose a high-performance web server capable of serving static files.

  - A good choice, regardless of OS, is Apache_.

- The **back-end web server** is bundled with CamCOPS. Its code lives in the
  CamCOPS Python virtual environment. Its job is to serve CamCOPS, and only
  CamCOPS, to an internal TCP/IP port (or UNIX socket). The front-end web
  server then routes appropriate requests through to it. CamCOPS offers a
  choice:

  - Gunicorn_ is probably the best choice for deployment under Linux/UNIX. It’s
    pretty quick. It relies on the UNIX `fork()` function [#fork]_, so it
    doesn’t run under Windows.

  - CherryPy_ is a good cross-platform choice, and definitely the best choice
    under Windows.

  - Pyramid_ is the web framework that CamCOPS uses, and it comes with its own
    demonstration server. This is handy for running the very helpful Pyramid
    debug toolbar [#pyramiddebugtoolbar]_ during testing, but it is definitely
    not a good choice for deployment.


Configure your firewall
-----------------------

Ensure your firewall is configured properly:

- You’ll need to allow HTTPS through, for tablet communications and the web
  viewer. (The default port is 443; see :ref:`TCP/IP ports <tcpip_ports>`. It’s
  possible to use another, but it will confuse users.)

- You’ll probably want remote SSH access, either through the default port (22),
  or a secret port known only to you.

- Other ports are up to you. If you want to run a plain web server as well,
  that’ll normally be on port 80. Access to CamCOPS should only be via HTTPS,
  not plain HTTP.

- Disable access to everything you don’t need.


.. _create_database:


Create a database
-----------------

The method to create a database depends on the database engine you plan to use.
Here’s a method for MySQL to create a database named `camcops`.

First, from the Linux command line, log in to MySQL as root:

.. code-block:: bash

    mysql --host=127.0.0.1 --port=3306 --user=root --password
    # ... or the usual short form: mysql -u root -p

Then in MySQL:

.. code-block:: mysql

    # Create the database:

    CREATE DATABASE camcops;

    # Ideally, create another user that only has access to the CamCOPS database.
    # You should do this, so that you don’t use the root account unnecessarily.

    GRANT ALL PRIVILEGES ON camcops.* TO 'YYYYYY_REPLACE_ME'@'localhost' IDENTIFIED BY 'ZZZZZZ_REPLACE_ME';

    # For future use: if you plan to explore your database directly for analysis,
    # you may want to create a read-only user. Though it may not be ideal (check:
    # are you happy the user can see the audit trail?), you can create a user with
    # read-only access to the entire database like this:

    GRANT SELECT camcops.* TO 'QQQQQQ_REPLACE_ME'@'localhost' IDENTIFIED BY 'PPPPPP_REPLACE_ME';

    # All done. Quit MySQL:

    exit


Create/edit a CamCOPS config file
---------------------------------

See :ref:`“The CamCOPS server configuration file” <server_config_file>`.

The file is typically called `/etc/camcops/camcops.conf` and should be readable
by the web server user, such as `www-data` under Ubuntu [#linuxflavours]_.


Create the database structure
-----------------------------

To create tables and indexes, use the command:

.. code-block:: bash

    camcops upgrade_db --config CONFIG

where CONFIG is the filename of your configuration file. If your configuration
file is only readable as `www-data`, you will need to run this with ``sudo``:

.. code-block:: bash

    sudo -u www-data camcops upgrade_db --config CONFIG


Create a superuser
------------------

Use the command:

.. code-block:: bash

    camcops make_superuser --config CONFIG

where CONFIG is the filename of your configuration file. (Again, use ``sudo``
as above if your configuration file requires privileged access to read.)


Start CamCOPS
-------------

Under Linux, this is best done via Supervisor_, which launches programs, keeps
log files for them, and restarts them when the computer is rebooted.

To generate a specimen Supervisor configuration file for CamCOPS, run the
command

.. code-block:: bash

    camcops demo_supervisor_config > my_demo_camcops_supervisor_config.conf

Here's an example, which you would typically save as
`/etc/supervisor/conf.d/camcops.conf`:

..  literalinclude:: demo_supervisor_config.txt
    :language: ini

This is where you choose which back-end web server CamCOPS should use (see
above), by choosing the command you pass to `camcops`. For high-performance
work under Linux, use Gunicorn, with the `serve_gunicorn` command; see the
:ref:`options for the camcops command <camcops_cli>`.

.. _configure_apache:

Point the front-end web server to CamCOPS
-----------------------------------------

Under Linux, a typicaly front-end web server is Apache_.

To generate a specimen Apache configuration file for CamCOPS, run the command

.. code-block:: bash

    camcops demo_apache_config > demo_apache_config_chunk.txt

Here's an example to mount CamCOPS at the URL path `/camcops`, which you would
edit into the Apache config file [#linuxflavours]_:

.. If you re-paste the demo below, make sure you do it from a copy installed
   to /usr/share/camcops, not your user's home directory, otherwise the paths
   will be silly.


..  literalinclude:: demo_apache_config.txt
    :language: apacheconf

Once you are happy with your Apache config file:

- Ensure file ownerships/permissions are correct (including, on CentOS, SELinux
  permissions [#selinux]_).

  - On Ubuntu, if you use `/srv/www` as your `DocumentRoot`, you may need to
    do:

    .. code-block:: bash

        sudo chown -R www-data:www-data /srv/www

  - On CentOS, assuming you use `/var/www` as your `DocumentRoot`, you may need
    to do:

    .. code-block:: bash

        ls -alZ /var/www # shows owners and SELinux security context

        sudo chown -R apache:apache /var/www
        sudo chcon -R -h system_u:object_r:httpd_sys_content_t /var/www
        sudo chown -R apache:apache /etc/camcops
        sudo chcon -R -h system_u:object_r:httpd_sys_content_t /etc/camcops
        sudo chown -R apache:apache /var/cache/camcops
        sudo chcon -R -h system_u:object_r:httpd_sys_content_t /var/cache/camcops
        sudo chown -R apache:apache /usr/share/camcops/server/static
        sudo chcon -R -h system_u:object_r:httpd_sys_content_t /usr/share/camcops/server/static

- Restart Apache: ``sudo apachectl restart``.

- Ensure Apache restarts on boot.

  - On Ubuntu, this should be automatic.
  - On CentOS, run:

    .. code-block:: bash

        sudo chkconfig --level 2345 httpd on

Browse to the web site
-----------------------

If you have configured things correctly, the rest of the configuration should
be possible via the CamCOPS web site.

Assuming you used `/camcops` as the base URL path,

- Browse to `https://YOURHOST/camcops/webview`. This should work.

- Browse to `http://YOURHOST/camcops/webview`. This should *not* work; you
  shouldn’t allow access via plain HTTP.

- Check that a tablet device can register with the server and upload some data
  while using the URL `https://YOURHOST/camcops/database`.


Troubleshooting access to the web site
--------------------------------------

1.  If something isn't working, begin by trying the following (as a user that
    can definitely read the config file):

    .. code-block:: bash

        cat /PATH/TO/YOUR_CONFIG_FILE  # can I read it?
        camcops serve_pyramid --config /PATH/TO/YOUR_CONFIG_FILE

    Note the URL and port, likely ``localhost`` on port 8000, and in a separate
    command prompt, try:

    .. code-block:: bash

        wget http://127.0.0.1:8000

    The server should report a "GET / HTTP" message and the ``wget`` command should
    return HTML with a "login failed" message, but if so, this shows that CamCOPS
    is reading the config file and serving data correctly.

2.  If a UNIX socket method wasn't working, try a TCP/IP port method.

    - If a TCP/IP method works and a Unix socket doesn't, with Apache, then
      check the Apache config file and make sure the "internal" unique dummy
      URL associated with the socket is using "http", not "https". See the
      demo Apache config file.

3.  If, when using Apache, you get errors like ``Page not found! //login``,
    then there is a slash error; potentially you have an incorrect slash at
    the end of the Unix domain socket "dummy" URL.

Configure backups
-----------------

Your backup strategy is up to you. However, one option is to use a script to
dump all MySQL databases. A sample script is produced by the command

.. code-block:: bash

    camcops demo_mysql_dump_script

but even better is this tool:

.. code-block:: bash

    camcops_backup_mysql_database

If you use this strategy, you will need to save this script and edit the copy.
Be sure your copy of the script is readable only by root and the backup user,
as it contains a password. You can then run your script regularly from
`/etc/crontab` (see ``man cron``, ``man crontab``).

Obviously, you will also need the dumped files to be backed up to a physically
secure location regularly.

More than one CamCOPS instance
------------------------------

This is simple to set up, but fiddly to maintain. Try to avoid it! Using one
database and :ref:`groups <groups>` is much better. But if you have to:

- Create a second CamCOPS database (from the MySQL command line) as above.

  - **Be careful:** MySQL users are system-wide. So don’t think you can have a
    user named `camcopsmaster` with password X for one database, and a user
    named `camcopsmaster` with password Y for another database; attempting this
    will merely change the password for that (single) user.

- Create a second CamCOPS configuration file, e.g. copying
  `/etc/camcops/camcops.conf` to `/etc/camcops/camcops2.conf` and editing it to
  point to the new database.

- Run ``camcops`` from the command line, pointing it to the new configuration
  file, to create the tables and a superuser (as above).

- Add a second instance to the Apache configuration file and restart Apache.


Database performance tuning
---------------------------

Ignore this section unless you actually have performance problems.

MySQL/InnoDB commit
~~~~~~~~~~~~~~~~~~~

Network latency can be improved considerably by altering the MySQL/InnoDB
log-on-commit behaviour. This is governed by the
`innodb_flush_log_at_trx_commit` variable. The default is 1, which is the
safest; it is required for ACID compliance. However, setting it to 2 makes
database write operations much faster.

This can by done by editing the MySQL configuration file [#linuxflavours]_ to
add this line:

.. code-block:: ini

    [mysqld]

    innodb_flush_log_at_trx_commit = 2

after which you would need to restart MySQL [#linuxflavours]_. Alternatively
you can change it dynamically at the MySQL command line with:

.. code-block:: mysql

    SET GLOBAL innodb_flush_log_at_trx_commit = 2;

    # Use SHOW VARIABLES; to show the current values.


See also:

- http://stackoverflow.com/questions/14121464/mysql-is-slow-with-innodb-during-insert-compared-to-myisam

- http://dev.mysql.com/doc/refman/5.5/en/innodb-parameters.html

Cosmetics
---------

Your institutional logo
~~~~~~~~~~~~~~~~~~~~~~~

Your logos (see the :ref:`configuration file <server_config_file>`) will be
scaled to 45% of the active page width. You may need to add blank space to the
left if they look funny. See picture below.

.. image:: images/scaling_logos.png


.. rubric:: Footnotes

.. [#linuxflavours]
    See :ref:`Linux flavours <linux_flavours>` for a reminder of some common
    differences between Linux operating systems.

.. [#lamp]
    http://en.wikipedia.org/wiki/LAMP_(software_bundle)

.. [#snakeoil]
    This is referred to as creating a “snake oil” certificate. See e.g.
    https://en.wikipedia.org/wiki/Snake_oil_(cryptography);
    http://www.akadia.com/services/ssh_test_certificate.html

.. [#pyramiddebugtoolbar]
    https://docs.pylonsproject.org/projects/pyramid_debugtoolbar/

.. [#fork]
    https://en.wikipedia.org/wiki/Fork_(system_call)

.. [#selinux]
    See http://wiki.apache.org/httpd/13PermissionDenied and
    https://access.redhat.com/site/documentation/en-US/Red_Hat_Enterprise_Linux/6/html/Managing_Confined_Services/chap-Managing_Confined_Services-The_Apache_HTTP_Server.html
