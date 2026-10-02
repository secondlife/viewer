#!/usr/bin/env python3
"""\
@file perfbot_run.py
@brief Run a number of non interactive Viewers (PerfBots) with
       a variety of options and settings. Pass --help for details.

$LicenseInfo:firstyear=2007&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2021, Linden Research, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
$/LicenseInfo$
"""

import argparse
import subprocess
import os
import math
import time

# Required parameters that are always passed in
# Specify noninteractive mode (SL-15999 for details)
PARAM_NON_INTERACTIVE = "--noninteractive"
# Run multiple Viewers at once
PARAM_MULTI = "--multiple"
# Specify username (first and last) and password
PARAM_LOGIN = "--login"
# SLURL to teleport to after login
PARAM_SLURL = "--slurl"


def gen_niv_script(args):
    print(f"Reading creds from {(args.creds)} folder")
    print(f"Using the non interactive Viewer from {(args.viewer)}")
    print(f"Sleeping for {args.sleep}ms between Viewer launches")

    # Read the lines from the creds file.  Typically this will be
    # stored in the build-secrets-git private repository but you
    # can point to any location with the --creds parameter
    creds_lines = []
    with open(args.creds) as file:
        creds_lines = file.readlines()
        creds_lines = [line.rstrip() for line in creds_lines]
        creds_lines = [line for line in creds_lines if not line.startswith("#") and len(line)]
    # We cannot log in more users than we have credentials for
    if args.num==0:
        args.num = len(creds_lines)
    if args.num > len(creds_lines):
        print(
            f"The number of agents specified ({(args.num)}) exceeds "
            f"the number of valid entries ({(len(creds_lines))}) in "
            f"the creds file "
        )
        return

    print(f"Launching {(args.num)} instances of the Viewer")

    # The Viewer (in dev environments at least) needs a well specified
    # working directory to function properly. We try to guess what it
    # might be based on the full path to the Viewer executable but
    # you can also specify it explicitly with the --cwd parameter
    # (required for dev builds)
    args.viewer = os.path.abspath(args.viewer)
    if len(args.cwd) == 0:
        working_dir = os.path.dirname(os.path.abspath(args.viewer))
    else:
        working_dir = os.path.abspath(args.cwd)
    print(f"Working directory is {working_dir}, cwd {args.cwd}")
    os.chdir(working_dir)

    if args.dryrun:
        print("Running in dry-run mode - no Viewers will be started")
    print("")

    for inst in range(args.num):

        # Format of each cred line is username_first username_last password
        # A space is used to separate each and a # at the start of a line
        # removes it from the pool (useful if someone else is using a subset
        # of the available ones)
        creds = creds_lines[inst].split(" ")
        username_first = creds[0]
        username_last = creds[1]
        password = creds[2]

        # The default layout is an evenly spaced circle in the
        # center of the region.  We may extend this to allow other
        # patterns like a square/rectangle or a spiral. (Hint: it
        # likely won't be needed :))
        center_x = 128
        center_y = 128
        if args.layout == "circle":
            radius = 6
            angle = (2 * math.pi / args.num) * inst
            region_x = int(math.sin(angle) * radius + center_x)
            region_y = int(math.cos(angle) * radius + center_y)
            region_z = 0
        elif args.layout == "square":
            region_x = center_x
            region_y = center_y
        elif args.layout == "spiral":
            region_x = center_x
            region_y = center_y
        slurl = f"secondlife://{args.region}/{region_x}/{region_y}/{region_z}"

        # Build the script line
        script_cmd = [args.viewer]
        script_cmd.append(PARAM_NON_INTERACTIVE)
        script_cmd.append(PARAM_MULTI)
        script_cmd.append(PARAM_LOGIN)
        script_cmd.append(username_first)
        script_cmd.append(username_last)
        script_cmd.append(password)
        script_cmd.append(PARAM_SLURL)
        script_cmd.append(slurl)

        # Display the script we will execute.
        cmd = ""
        for p in script_cmd:
            cmd = cmd + " " + p
        print(cmd)

        # If --dry-run is specified, we do everything (including, most
        # usefully, display the script lines) but do not start the Viewer
        if args.dryrun == False:
            print("opening viewer session with",script_cmd)
            viewer_session = subprocess.Popen(script_cmd)

        # Sleeping a bit between launches seems to help avoid a CPU
        # surge when N Viewers are started simulatanously. The default
        # value can be changed with the --sleep parameter
        time.sleep(args.sleep / 1000)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument(
        "--num",
        type=int,
        default=0,
        dest="num",
        help="How many avatars to add to the script",
    )
    parser.add_argument(
        "--creds",
        default="../../../build-secrets-git/perf/perfbot_creds.txt",
        dest="creds",
        help="Location of the text file containing user credentials",
    )
    parser.add_argument(
        "--viewer",
        default="C:/Program Files/SecondLife/SecondLifeViewer.exe",
        dest="viewer",
        help="Location of the non interactive Viewer build",
    )
    parser.add_argument(
        "--cwd",
        default="",
        dest="cwd",
        help="Location of the current working directory to use",
    )
    parser.add_argument(
        "--region",
        default="Lag Me 5",
        dest="region",
        help="The SLURL for the Second Life region to visit",
    )
    parser.add_argument(
        "--layout",
        default="circle",
        dest="layout",
        choices={"circle", "square", "spiral"},
        help="The geometric layout of the avatar destination locations",
    )
    parser.add_argument(
        "--sleep",
        type=int,
        default=1000,
        dest="sleep",
        help="Time to sleep between launches in milliseconds",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        dest="dryrun",
        help="Dryrun mode - display parameters and script lines but do not start any Viewers",
    )
    args = parser.parse_args()

    gen_niv_script(args)
