#!/usr/bin/env python

# noinspection HttpUrlsUsage
"""
camcops_server/cc_modules/cc_cache.py

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

**Cache functions.**

1. The basic cache objects.

2. FIX FOR DOGPILE.CACHE FOR DECORATED FUNCTIONS, 2017-07-28 (PLUS SOME OTHER
   IMPROVEMENTS). SEE
   https://bitbucket.org/zzzeek/dogpile.cache/issues/96/error-in-python-35-with-use-of-deprecated

Crash using type-hinted functions under Python 3.5 with dogpile.cache==0.6.4:

.. code-block:: none

    Traceback (most recent call last):
      File "/usr/lib/python3.5/runpy.py", line 184, in _run_module_as_main
        "__main__", mod_spec)
      File "/usr/lib/python3.5/runpy.py", line 85, in _run_code
        exec(code, run_globals)
      File "/home/rudolf/Documents/code/camcops/server/camcops_server/cc_modules/cc_cache.py", line 64, in <module>
        unit_test_cache()
      File "/home/rudolf/Documents/code/camcops/server/camcops_server/cc_modules/cc_cache.py", line 50, in unit_test_cache
        def testfunc() -> str:
      File "/home/rudolf/dev/venvs/camcops/lib/python3.5/site-packages/dogpile/cache/region.py", line 1215, in decorator
        key_generator = function_key_generator(namespace, fn)
      File "/home/rudolf/dev/venvs/camcops/lib/python3.5/site-packages/dogpile/cache/util.py", line 31, in function_key_generator
        args = inspect.getargspec(fn)
      File "/usr/lib/python3.5/inspect.py", line 1045, in getargspec
        raise ValueError("Function has keyword-only arguments or annotations"
    ValueError: Function has keyword-only arguments or annotations, use getfullargspec() API which can support them

3. CACHING NOTES

- We currently use 'dogpile.cache.memory' as the backend.
  This means that for single-process (single-thread or multithreaded) servers,
  the cache is unique, but that won't work for multi-process (e.g. Gunicorn)
  servers.

- That means that in a multiprocess environment it's fine to continue to use a
  memory cache for file-based stuff (e.g. configs, XML strings), but not for
  database-based stuff (e.g. which ID numbers are valid).

- Correct solutions WITH a cache for those database-based things include:

  - ignoring Python caching and relying on the MySQL query cache -- but this is
    being removed because it's not all that great:

    https://mysqlserverteam.com/mysql-8-0-retiring-support-for-the-query-cache/

  - using memcached (via dogpile.cache.pylibmc)

    https://www.ubergizmo.com/how-to/install-memcached-windows/

  - using redis (via dogpile.cache.redis and
    https://pypi.python.org/pypi/redis/ )

    https://stackoverflow.com/questions/10558465/memcached-vs-redis
    https://redis.io/
    https://web.archive.org/web/20120118030804/http://simonwillison.net/static/2010/redis-tutorial/
    http://oldblog.antirez.com/post/take-advantage-of-redis-adding-it-to-your-stack.html
    https://redis.io/topics/security

    redis unsupported under Windows:
        https://redis.io/download

- The other obvious alternative: don't cache such stuff! This may all be
  premature optimization.

  https://msol.io/blog/tech/youre-probably-wrong-about-caching/

  The actual price is of the order of 0.6-1 ms per query, for the queries
  "find me all the ID number definitions" and "fetch the server settings".

- The answer is probably:

  - continue to use dogpile.cache.memory for simple "fixed" stuff read from
    disk;
  - continue to use Pyramid's per-request caching mechanism (@reify);
  - forget about database caching for now;
  - if it becomes a problem later, move to Redis

- Therefore:

  - there should be no calls to cache_region_static.delete

"""  # noqa


# =============================================================================
# Imports; logging
# =============================================================================

from cardinal_pythonlib.dogpile_cache import kw_fkg_allowing_type_hints as fkg
from dogpile.cache import make_region

# =============================================================================
# The main cache: static for the lifetime of this process.
# =============================================================================

cache_region_static = make_region()
cache_region_static.configure(
    backend='dogpile.cache.memory'
    # Consider later: memcached via dogpile.cache.pylibmc
)

# Can now use:
# @cache_region_static.cache_on_arguments(function_key_generator=fkg)

# https://stackoverflow.com/questions/44834/can-someone-explain-all-in-python
__all__ = ['cache_region_static', 'fkg']  # prevents "Unused import statement"
