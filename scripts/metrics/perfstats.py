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
    
def collect_timer_data(filename, timers, avatar_fields, max_records, prune=None):
    #print "collect_timer_data", timers,"avatar_fields",avatar_fields
    all_timers = []
    times = {}
    frame_data = []
    frame_count = 0
    header = []
    iter_rec = iter(get_frame_record(filename))
    av_record = None
    av_keys = []
    for sd in iter_rec:
        times = {}
        if timers is None and frame_count==0:
            timers = sorted(sd['Timers'].keys())
        else:
            timers = sorted(timers)
        for t in timers:
            try:
                times[t] = sd['Timers'][t]['Time']
            except KeyError:
                times[t] = None
        times_list = [times[i] for i in timers]

        av_list = []
        if avatar_fields is not None:
            av_data = {}
            av_record = None
            if 'Avatars' in sd['Extra']:
                for i, av in enumerate(sd['Extra']['Avatars']):
                    if not av['Self']:
                        continue
                    av_record = av
            for av_field in sorted(avatar_fields):
                av_key = "Avatar.Self." + av_field
                if frame_count==0:
                    av_keys.append(av_key)
                if av_record is not None:
                    av_data[av_key] = av_record.get(av_field,None)
                else:
                    av_data[av_key] = None
            av_list = [av_data[av_key] for av_key in av_keys]
            if frame_count==0:
                header = timers + [av_key for av_key in av_keys]
            

        #print "header",header
        #print "times_list",times_list
        #print "av_list",av_list
        #print "av_keys",av_keys
        if frame_count==0:    
            frame_data.append(header)
        frame_data.append(times_list + av_list)

        frame_count += 1
        if max_records is not None and frame_count >= max_records:
            break
    return frame_data

def summarize(filename, timers, max_records=None):
    print "collecting timers",timers

    summary = {}

    print "do summarize here, max_records", max_records

    frame_count = 0
    for sd in get_frame_record(filename):
        frame_count += 1
        if max_records is not None and frame_count >= max_records:
            print "read enough records"
            break
        if args.verbose:
            print "FRAME #",frame_count
            #print sd
        for t in timers: 
            if t in sd['Timers']:
                time_val = sd['Timers'][t]['Time']
                if args.verbose:
                    print t,time_val
                if not t in summary:
                    summary[t] = {'n':0, 'sum':0.0}
                summary[t]['n'] += 1
                summary[t]['sum'] += time_val

    for t in timers:
        if t in summary:
            avg = summary[t]['sum']/summary[t]['n']
            print t,"avg",avg,"count",summary[t]['n']
    return summary

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="analyze viewer performance files")
    parser.add_argument("--verbose", action="store_true",help="verbose flag")
    parser.add_argument("--timers", help="specify timers to be examined", nargs="+")
    parser.add_argument("--avatar_fields", help="specify avatar fields to be examined", nargs="+")
    parser.add_argument("--export", help="export results to specified file")
    parser.add_argument("--summary", action="store_true", help="print summary stats for the requested timers") 
    parser.add_argument("--max_records", type=int, help="limit to number of frames to process") 
    parser.add_argument("infilename", help="name of performance file", nargs="?", default="performance.slp")
    args = parser.parse_args()

    timer_data = collect_timer_data(args.infilename, args.timers, args.avatar_fields, args.max_records, 0.001)

    if args.export:
        export(args.export, timer_data)
        
    if args.infilename and args.summary:
        summarize(args.infilename, args.timers, args.max_records)


    print "done"
