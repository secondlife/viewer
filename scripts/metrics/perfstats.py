#!runpy.sh

"""\

This module contains tools for manipulating and validating the avatar skeleton file.

$LicenseInfo:firstyear=2016&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2016, Linden Research, Inc.

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
from lxml import etree
import sys
from llbase import llsd

def process_frame_record(elem):
    frame_data = {}
    
def summarize(filename, timers):
    print "collecting timers",timers

    summary = {}

    print "do summarize here"
    f = open(filename)

    # get an iterable
    context = etree.iterparse(f, events=("start", "end"))

    # turn it into an iterator
    context = iter(context)

    # get the root element
    event, root = context.next()

    frame_count = 0
    for event, elem in context:
        if event == "end" and elem.tag == "llsd":
            # process frame record here
            xmlstr = etree.tostring(elem, encoding="utf8", method="xml")
            sd = llsd.parse_xml(xmlstr)
            for t in timers: 
                if t in sd:
                    time_val = sd[t]['Time']
                    if not t in summary:
                        summary[t] = {'n':0, 'sum':0.0}
                    summary[t]['n'] += 1
                    summary[t]['sum'] += time_val
            
            frame_count += 1
            if frame_count % 100 == 0:
                # avoid accumulating lots of junk under root
                root.clear()

    print "Read",frame_count,"frame records"
    f.close()

    for t in timers:
        if t in summary:
            avg = summary[t]['sum']/summary[t]['n']
            print t,"avg",avg,"count",summary[t]['n']
    return summary

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="analyze viewer performance files")
    parser.add_argument("--verbose", action="store_true",help="verbose flag")
    parser.add_argument("infilename", help="name of performance file", default="performance.slp")
    parser.add_argument("--timers", help="specify timers to be examined", nargs="+")
    parser.add_argument("--summary", action="store_true", help="print summary stats for the requested timers") 
    args = parser.parse_args()

    if args.infilename and args.summary:
        summarize(args.infilename, args.timers)

    print "done"
