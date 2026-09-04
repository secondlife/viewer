#!/usr/bin/env python3
"""\
@file   logsdir.py
@author Nat Goodspeed
@date   2024-09-12
@brief  Locate the Second Life logs directory for the current user on the
        current platform.

$LicenseInfo:firstyear=2024&license=viewerlgpl$
Copyright (c) 2024, Linden Research, Inc.
$/LicenseInfo$
"""

import os
from pathlib import Path
import platform

class Error(Exception):
    pass

# logic used by SLVersionChecker
def logsdir():
    app = 'SecondLife'
    system = platform.system()
    if (system == 'Darwin'):
        base_dir = os.path.join(os.path.expanduser('~'),
                                'Library','Application Support',app)
    elif (system == 'Linux'):
        base_dir = os.path.join(os.path.expanduser('~'),
                                '.' + app.lower())
    elif (system == 'Windows'):
        appdata = os.getenv('APPDATA')
        base_dir = os.path.join(appdata, app)
    else:
        raise ValueError("Unsupported platform '%s'" % system)

    return os.path.join(base_dir, 'logs')

def latest_file(dirpath, pattern):
    files = Path(dirpath).glob(pattern)
    sort = [(p.stat().st_mtime, p) for p in files if p.is_file()]
    sort.sort(reverse=True)
    try:
        return sort[0][1]
    except IndexError:
        raise Error(f'No {pattern} files in {dirpath}')
