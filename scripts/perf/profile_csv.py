#!/usr/bin/env python3
"""\
@file   profile_csv.py
@author Nat Goodspeed
@date   2024-09-12
@brief  Convert a JSON file from Develop -> Render Tests -> Frame Profile to CSV

$LicenseInfo:firstyear=2024&license=viewerlgpl$
Copyright (c) 2024, Linden Research, Inc.
$/LicenseInfo$
"""

import json
from logsdir import Error, latest_file, logsdir
import sys

def convert(path, totals=True, unused=True, file=sys.stdout):
    with open(path) as inf:
        data = json.load(inf)
    # print path to sys.stderr in case user is redirecting stdout
    print(path, file=sys.stderr)

    print('"name", "file1", "file2", "time", "binds", "samples", "triangles"', file=file)

    if totals:
        t = data['totals']
        print(f'"totals", "", "", {t["time"]}, {t["binds"]}, {t["samples"]}, {t["triangles"]}',
              file=file)

    for sh in data['shaders']:
        print(f'"{sh["name"]}", "{sh["files"][0]}", "{sh["files"][1]}", '
              f'{sh["time"]}, {sh["binds"]}, {sh["samples"]}, {sh["triangles"]}', file=file)

    if unused:
        for u in data['unused']:
            print(f'"{u}", "", "", 0, 0, 0, 0', file=file)

def main(*raw_args):
    from argparse import ArgumentParser
    parser = ArgumentParser(description="""
%(prog)s converts a JSON file from Develop -> Render Tests -> Frame Profile to
a more-or-less equivalent CSV file. It expands the totals stats and unused
shaders list to full shaders lines.
""")
    parser.add_argument('-t', '--totals', action='store_false', default=True,
                        help="""omit totals from CSV file""")
    parser.add_argument('-u', '--unused', action='store_false', default=True,
                        help="""omit unused shaders from CSV file""")
    parser.add_argument('path', nargs='?',
                        help="""profile filename to convert (default is most recent)""")

    args = parser.parse_args(raw_args)
    convert(args.path or latest_file(logsdir(), 'profile.*.json'),
            totals=args.totals, unused=args.unused)

if __name__ == "__main__":
    try:
        sys.exit(main(*sys.argv[1:]))
    except (Error, OSError, json.JSONDecodeError) as err:
        sys.exit(str(err))
