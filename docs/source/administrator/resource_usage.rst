..  docs/source/administrator/resource_usage.rst

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

Resource usage
==============

..  contents::
    :local:
    :depth: 3


Method
------

The following values are taken from a Docker development configuration of
CamCOPS 2.3.7 on 15 Aug 2020. In general the command sequence was:

.. code-block:: bash

    docker container ls  # which container ID?
    docker exec -it CONTAINER /bin/bash  # run a shell in that container
    apt-get update && apt install htop smemstat  # install tools
    # htop  # memory usage; see https://hisham.hm/htop/
    # smem -k  # better memory usage; see https://www.selenic.com/smem/
    smemstat -l  # even better than smem
    du -bch /camcops  # disk usage

Note that 1 GB = 10^9 bytes and 1 GiB = 2^30 bytes = 1.07 GB.


Memory
------

..  Use htop, then Shift-H to hide user threads, then F5 to toggle process
    tree view. The RES column is "resident size" (memory usage) in kb, and
    also corresponds directly to "MEM%".
    .
    https://serverfault.com/questions/517483/how-to-read-memory-usage-in-htop
    https://askubuntu.com/questions/176001/what-do-virt-res-and-shr-mean-in-the-top-command
    https://serverfault.com/questions/238302/memory-usage-numbers-in-top-htop
    .
    To avoid double-counting:
    .
    https://unix.stackexchange.com/questions/34795/correctly-determining-memory-usage-in-linux
    https://docs.docker.com/config/containers/runmetrics/
    smem (as above)
    smemstat (as above)

**Web server**

*Baseline*

A freshly started "web server" process using CherryPy fired up 19 sub-threads.
The parent and each child used 351 Mb (for a notional total of 20 351 Mb = 7
Gb). The ``htop`` utility reported a total of 6.5 Gb in use. This quantity
would, of course, vary with the number of threads you configure.

However, the child processes will share memory (and memory will be
double-counted, or counted 20-fold in this example). So, more accurate
assessments are as follows.

The ``smem -k`` command reports 305 MB as its PSS (proportional set size)
estimate of actual memory usage for the ``camcops_server`` process.

Using ``cat /sys/fs/cgroup/memory/memory.usage_in_bytes`` gave a total memory
usage of 477 MB (but this is a rough-and-ready estimate).

Using ``docker stats`` gave a total memory usage for this container of 328 MiB.

*After activity*

The Docker container size grew to 386 MiB after some load.

**Workers**

With a default configuration on an 8-CPU machine (which allows 8 CPUs per
Docker container, and starts as many Celery workers as CPUs), the ``smem``
command reports 10 processes totalling 1.74 GB. The ``docker stats`` command
reports 1.69 GiB (these are consistent: 1.74 GB = 1.62 GiB).

Once things have started happening, memory use goes up a bit. For example,
``smemstat -l`` can show one parent at 287 MiB, one child at 158 Mb, and eight
grandchildren (the actual worker processes) mostly at 146 MiB but one at 388
MiB (presumably the one that's done some work).

.. _celery_memory_leak:

*Celery-related memory leak*

In fact, there is a problem here. Even with the concurrency set to a single
worker (via :ref:`CELERY_WORKER_EXTRA_ARGS <CELERY_WORKER_EXTRA_ARGS>`), that
single worker grew to 1.55 GB (with 2.03 GiB for the Docker container as a
whole).

This looks like a memory leak. It is probably a Python problem brought out by
Celery. See:

- https://chase-seibert.github.io/blog/2013/08/03/diagnosing-memory-leaks-python.html
  (2013)
- https://medium.com/@aaron.reyna/python-celery-ram-intensive-tasks-and-a-memory-leak-c2681ee98c9
  (2018)
- https://docs.celeryproject.org/en/latest/userguide/workers.html#max-tasks-per-child-setting
- possibly related: https://github.com/celery/celery/issues/4843 (2018)

A solution: use ``--maxtasksperchild=20`` in the :ref:`CELERY_WORKER_EXTRA_ARGS
<CELERY_WORKER_EXTRA_ARGS>` config parameter. This successfully caps memory
usage at around 750 MiB for a one-worker container.

**Scheduler**

As a Docker container, this takes about 570 MiB at baseline (via ``docker
stats``). This seems static.

**Monitor**

As a Docker container, this takes about 571 MiB at baseline (via ``docker
stats``). This seems fairly static (e.g. up to 578 MiB after a bit of work).

**RabbitMQ**

As a Docker container, this takes about 101 MiB at baseline (via ``docker
stats``). This seems fairly static (e.g. up to 104 MiB after some work).

**MySQL**

As a Docker container, this takes about 209 MiB at baseline (via ``docker
stats``). Some work pushed this to 302 MiB.


Disk
----

The ``/camcops`` tree within a standard Docker configuration takes up about 718
MiB. This includes source code, the compiled virtual environment with
dependencies installed, and some config files.
