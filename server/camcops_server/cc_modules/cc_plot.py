#!/usr/bin/env python

"""
camcops_server/cc_modules/cc_plot.py

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

**Plotting functions.**

PROPER WAY TO USE MATPLOTLIB:

- https://jbarillari.blogspot.co.uk/2009/09/threadsafety-and-matplotlibpylab.html?m=1
- https://sjohannes.wordpress.com/2010/06/11/using-matplotlib-in-a-web-application/amp/
- https://matplotlib.org/faq/howto_faq.html#howto-webapp
- https://matplotlib.org/examples/api/agg_oo.html#api-agg-oo

In summary: matplotlib is easy to use in a way that has global state, but that
will break in a threading application. Using the Figure() API is safe. Thus:

.. code-block:: python

    from matplotlib.backends.backend_agg import FigureCanvasAgg as FigureCanvas
    from matplotlib.figure import Figure

    fig = Figure()
    canvas = FigureCanvas(fig)
    ax = fig.add_subplot(111)
    ax.plot([1, 2, 3])
    ax.set_title('hi mom')
    ax.grid(True)
    ax.set_xlabel('time')
    ax.set_ylabel('volts')
    canvas.print_figure('test')

"""  # noqa

# =============================================================================
# Basic imports
# =============================================================================

import atexit
import logging
import os
import shutil
import tempfile

from cardinal_pythonlib.logs import BraceStyleAdapter

log = BraceStyleAdapter(logging.getLogger(__name__))


# =============================================================================
# Constants
# =============================================================================

ENVVAR_HOME = "HOME"
ENVVAR_MPLCONFIGDIR = "MPLCONFIGDIR"

# =============================================================================
# Import matplotlib
# =============================================================================

# We need to use os.environ, since per-request stuff won't be initialized yet.
# That goes for anything that affects imports (to avoid the complexity of
# delayed imports).
if ENVVAR_MPLCONFIGDIR in os.environ:
    # 1+2. Use a writable static directory (speeds pyplot loads hugely).
    _mpl_config_dir = os.environ[ENVVAR_MPLCONFIGDIR]
else:
    # 1. Make a temporary directory (must be a directory per process, I'm sure)
    _mpl_config_dir = tempfile.mkdtemp()
    # 2. Ensure temporary directory is removed when this process exits.
    atexit.register(lambda: shutil.rmtree(_mpl_config_dir, ignore_errors=True))

# 3. Tell matplotlib about this directory prior to importing it
#    http://matplotlib.org/faq/environment_variables_faq.html
os.environ[ENVVAR_MPLCONFIGDIR] = _mpl_config_dir

# 4. Another nasty matplotlib hack
#    matplotlib.font_manager reads os.environ.get('HOME') directly, and
#    searches ~/.fonts for fonts. That's fine unless a user is calling with
#    sudo -u USER, leaving $HOME as it was but removing the permissions - then
#    matplotlib crashes out with e.g.
#       PermissionError: [Errno 13] Permission denied: '/home/rudolf/.fonts/SABOI___.TTF'  # noqa
#    Note that an empty string won't help either, since the check is
#    "is not None".
#    You can't assign None to an os.environ member; see
#    http://stackoverflow.com/questions/3575165; do this:
if ENVVAR_HOME in os.environ:
    _old_home = os.environ[ENVVAR_HOME]
    del os.environ[ENVVAR_HOME]
else:
    _old_home = None

# 5. Import matplotlib
log.debug("Importing matplotlib (can be slow) (MPLCONFIGDIR={})...",
          _mpl_config_dir)
# noinspection PyUnresolvedReferences
import matplotlib  # noqa: E402,F401

# 6. Restore $HOME
if _old_home is not None:
    os.environ[ENVVAR_HOME] = _old_home

# 7. Set the backend
# REPLACED BY OO METHOD # matplotlib.use("Agg")  # also the default backend
# ... http://matplotlib.org/faq/usage_faq.html#what-is-a-backend
# ... http://matplotlib.org/faq/howto_faq.html
# matplotlib.use("cairo") # cairo backend corrupts some SVG figures

# Load this once so we can tell the user we're importing it and it's slow
# REPLACED BY OO METHOD # import matplotlib.pyplot  # noqa

log.debug("... finished importing matplotlib")

# REPLACED BY OO METHOD # # THEN DO e.g. # import matplotlib.pyplot as plt
