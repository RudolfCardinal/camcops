#!/usr/bin/env python3
# tools/update_multiple_databases.py

import argparse
import glob
import os
import subprocess
import sys

THIS_DIRECTORY = os.path.dirname(os.path.abspath(__file__))
PROJECT_BASE_DIRECTORY = os.path.abspath(os.path.join(THIS_DIRECTORY,
                                                      os.pardir))
DEFAULT_CAMCOPS = os.path.join(PROJECT_BASE_DIRECTORY, 'camcops.py')

# too dangerous # DEFAULT_FILESPEC = '/etc/camcops/*'
DEFAULT_CCARGS = ['maketables']


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description="Run commands across multiple CamCOPS databases")
    parser.add_argument(
        '--filespecs', nargs='+', required=True,
        help="List of CamCOPS config files (wildcards OK)")
    parser.add_argument(
        '--ccargs', nargs='+', default=DEFAULT_CCARGS,
        help="List of CamCOPS arguments, to which '--' will be prefixed; "
             "default: {}".format(" ".join(DEFAULT_CCARGS)))
    parser.add_argument(
        '--python', default=sys.executable,
        help="Python interpreter (default: {})".format(sys.executable))
    parser.add_argument(
        '--camcops', default=DEFAULT_CAMCOPS,
        help="CamCOPS executable (default: {})".format(DEFAULT_CAMCOPS))
    parser.add_argument(
        '-d', '--dummyrun', action="store_true",
        help="Dummy run (show filenames only)")
    parser.add_argument('-v', '--verbose', action="store_true", help="Verbose")
    args = parser.parse_args()
    if args.verbose:
        print("Arguments: {}".format(args))

    did_something = False
    for filespec in args.filespecs:
        for filename in glob.glob(filespec):
            did_something = True
            print("Processing: {}".format(filename))
            cmdargs = [args.python, args.camcops, filename] + [
                '--{}'.format(x) for x in args.ccargs]
            if args.verbose:
                print("Executing command: {}".format(cmdargs))
            if args.dummyrun:
                continue
            subprocess.check_call(cmdargs)
    if not did_something:
        print("Nothing to do; no files found")
