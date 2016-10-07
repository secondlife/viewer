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
import re

def xstr(x):
    if x is None:
        return ''
    return str(x)

def export_csv(filename, timer_data):
    if args.verbose:
        print timer_data
    f = open(filename,"w")
    for fd in timer_data:
        print >>f, ",".join([xstr(d) for d in fd])
    f.close()
    
def export(filename, timers):
    if re.match(".*\.csv$", filename):
        export_csv(filename, timers)
    else:
        print "unknown extension for export",filename

# optimized to avoid blowing out memory when processing huge files,
# but not especially efficient the way we parse twice.
def get_frame_record(filename):
    f = open(filename)

    # get an iterable
    context = etree.iterparse(f, events=("start", "end"))

    # turn it into an iterator
    context = iter(context)

    # get the root element
    event, root = context.next()

    frame_count = 0
    my_total = 0.0
    for event, elem in context:
        if event == "end" and elem.tag == "llsd":
            # process frame record here
            frame_count += 1

            xmlstr = etree.tostring(elem, encoding="utf8", method="xml")
            sd = llsd.parse_xml(xmlstr)
            yield sd
            
            if frame_count % 100 == 0:
                # avoid accumulating lots of junk under root
                root.clear()

    print "Read",frame_count,"frame records"
    f.close()

class derived:
    @staticmethod
    def timer_count(sd):
        return len(sd['Timers'])

def sd_extract_field(sd,key):
    chain = key.split(".")
    t = sd
    try:
        for subkey in chain:
            t = t[subkey]
        return t
    except:
        return None

def collect_frame_data(filename, fields, max_records, prune=None):
    frame_data = []
    frame_count = 0
    header = sorted(fields)
    iter_rec = iter(get_frame_record(filename))

    for sd in iter_rec:
        values = []
        values = [sd_extract_field(sd,key) for key in header]
        
        if frame_count==0:    
            frame_data.append(header)
        frame_data.append(values)

        frame_count += 1
        if max_records is not None and frame_count >= max_records:
            break

    return frame_data

if __name__ == "__main__":

    default_fields = [ "Timers.Frame.Time", "Timers.Render.Time", "Timers.UI.Time", "Session.UniqueID", "Avatars.Self.ARCCalculated" ]

    parser = argparse.ArgumentParser(description="analyze viewer performance files")
    parser.add_argument("--verbose", action="store_true",help="verbose flag")
    parser.add_argument("--fields", help="specify fields to be extracted or calculated", nargs="+", default=default_fields)
    parser.add_argument("--export", help="export results to specified file")
    parser.add_argument("--max_records", type=int, help="limit to number of frames to process") 
    parser.add_argument("infilename", help="name of performance file", nargs="?", default="performance.slp")
    args = parser.parse_args()

    timer_data = collect_frame_data(args.infilename, args.fields, args.max_records, 0.001)

    if args.export:
        export(args.export, timer_data)
        
    print "done"
