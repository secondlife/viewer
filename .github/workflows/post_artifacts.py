#!/usr/bin/env python3
"""\
@file   post_artifacts.py
@author Nat Goodspeed
@date   2023-08-18
@brief  Unpack and post artifacts from a GitHub Actions build

$LicenseInfo:firstyear=2023&license=viewerlgpl$
Copyright (c) 2023, Linden Research, Inc.
$/LicenseInfo$
"""

import github
import json
import os
import sys

class Error(Exception):
    pass

def main(*raw_args):
    buildstr = os.getenv('BUILD')
    build = json.loads(buildstr)
    from pprint import pprint
    pprint(build)

if __name__ == "__main__":
    try:
        sys.exit(main(*sys.argv[1:]))
    except Error as err:
        sys.exit(str(err))
