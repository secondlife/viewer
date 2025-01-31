#!/usr/bin/env python3
"""\
@file   watchdog.py
@author Nat Goodspeed
@date   2025-01-28
@brief  Monitor viewer termination; send crash data on crash.

$LicenseInfo:firstyear=2025&license=viewerlgpl$
Copyright (c) 2025, Linden Research, Inc.
$/LicenseInfo$
"""

import __main__
from contextlib import suppress
import json
import logging
import os
from pathlib import Path
import platform
import sys
from tkinter.messagebox import showerror

mydir = Path(__file__).parent
myname = Path(__main__.__file__).stem

class Error(Exception):
    pass

def main(logdir):
##  from argparse import ArgumentParser
##  parser = ArgumentParser(description="%(prog)s")
##  #parser.add_argument()
##  args = parser.parse_args(raw_args)

    with suppress(FileExistsError):
        os.makedirs(logdir)

    # Set up logging
    logging.basicConfig(filename=Path(logdir) / (myname + '.log'),
                        format='{asctime} {message}', style='{',
                        datefmt='%Y-%m-%d %H:%M:%S',
                        level=logging.INFO)
    logging.info(72*'=')

    # Our job is to notice when the viewer terminates. We spend most of our
    # lifetime hanging on a read of stdin. When the viewer terminates, in any
    # way, stdin will close and the read will complete. If the viewer writes
    # 'OK' first, it's intentional, a normal termination. If not -- well,
    # something went wrong.
    OK = sys.stdin.buffer.read(2)
    if OK != b'OK':
        raise Error('Viewer crashed!')

    logging.info("viewer terminated")

if __name__ == "__main__":
    try:
        sys.exit(main(*sys.argv[1:]))
    except Error as err:
        err = str(err)
        logging.error(err)
        showerror(myname, err)
        sys.exit(err)
