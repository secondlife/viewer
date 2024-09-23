#!/usr/bin/env python3
"""\
@file   profile_pretty.py
@author Nat Goodspeed
@date   2024-09-12
@brief  Pretty-print a JSON file from Develop -> Render Tests -> Frame Profile

$LicenseInfo:firstyear=2024&license=viewerlgpl$
Copyright (c) 2024, Linden Research, Inc.
$/LicenseInfo$
"""

import json
from logsdir import Error, latest_file, logsdir
import sys

def pretty(path):
    with open(path) as inf:
        data = json.load(inf)
    # print path to sys.stderr in case user is redirecting stdout
    print(path, file=sys.stderr)
    json.dump(data, sys.stdout, indent=4)

def main(*raw_args):
    from argparse import ArgumentParser
    parser = ArgumentParser(description="""
%(prog)s pretty-prints a JSON file from Develop -> Render Tests -> Frame Profile.
The file produced by the viewer is a single dense line of JSON.
""")
    parser.add_argument('path', nargs='?',
                        help="""profile filename to pretty-print (default is most recent)""")

    args = parser.parse_args(raw_args)
    pretty(args.path or latest_file(logsdir(), 'profile.*.json'))

if __name__ == "__main__":
    try:
        sys.exit(main(*sys.argv[1:]))
    except (Error, OSError, json.JSONDecodeError) as err:
        sys.exit(str(err))
