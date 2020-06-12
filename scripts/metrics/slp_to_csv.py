#!runpy.sh

"""\

This module contains tools for processing Second Life performance logs

$LicenseInfo:firstyear=2020&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2020, Linden Research, Inc.

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

# Convert a "classic" (non-arctan) format performance log (normally performance.slp) into a csv file.
# The result contains columns for all the times, followed by columns for all the call counts.
# To minimize confusion, this is broken out from perfstats.py, which deals with arctan-format logs.
# To minimize overhead, this is based on brute-force regex munging with no parsing of the LLSD.
# If the format of .slp files ever changes, this will fail utterly. 

# TODO: to support more formats, probably easiest to stick the results in a pandas frame and save that.

import re
import argparse
import sys

parser = argparse.ArgumentParser(description="analyze viewer performance files")
parser.add_argument("infilename", help="name of performance file to read", nargs = "?", default="performance.slp")
parser.add_argument("outfilename", help="name of csv file to create", nargs = "?", default="performance.csv")
args = parser.parse_args()

keys = set()
count = 0
outf = open(args.outfilename,"w")
with open(args.infilename,"r") as f:
    while True:
        l = f.readline()
        if not l:
            break
        count += 1
        calls = dict()
        times = dict()
        for m in re.finditer( r'<key>(.*?)</key><map><key>Calls</key><integer>(.*?)</integer><key>Time</key><real>(.*?)</real></map>', l ):
            (key,call,time) = m.groups()
            if count == 1:
                keys.add(key)
            calls[key] = call
            times[key] = time
        if count == 1:
            skeys = list(sorted(keys))
            cols = [key + " - Times" for key in skeys]
            cols.extend([key + " - Calls" for key in skeys])
            print >>outf, ",".join(cols)
        vals = []
        vals.extend([times[key] for key in skeys])
        vals.extend([calls[key] for key in skeys])
        print >>outf, ",".join(vals)
        
    

