#!/usr/bin/python
"""\
@file   setpython.py
@author Nat Goodspeed
@date   2011-07-13
@brief  Set PYTHON environment variable for tests that care.

$LicenseInfo:firstyear=2011&license=viewerlgpl$
Copyright (c) 2011, Linden Research, Inc.
$/LicenseInfo$
"""

import os
import sys
import subprocess

if __name__ == "__main__":
    os.environ["PYTHON"] = sys.executable
    sys.exit(subprocess.call(sys.argv[1:]))
