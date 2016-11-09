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
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import itertools

def xstr(x):
    if x is None:
        return ''
    return str(x)

def export_csv(filename, pd_data):
    if args.verbose:
        print pd_data
    pd_data.to_csv(filename)
    
def export(filename, timers):
    if re.match(".*\.csv$", filename):
        export_csv(filename, timers)
    else:
        print "unknown extension for export",filename

class DerivedSelfTimers:
    def __init__(self, sd, **kwargs):
        self.sd = sd
        self.kwargs = kwargs
    def __getitem__(self, key):
        value = self.sd["Timers"][key]["Time"]
        if "children" in self.kwargs:
            children = self.kwargs["children"]
            if children is not None:
                for child in children[key]:
                    if child in self.sd["Timers"]:
                        value -= self.sd["Timers"][child]["Time"]
        return value

class DerivedTimers:
    def __init__(self,sd,**kwargs):
        self.sd = sd
        self.kwargs = kwargs
    def __getitem__(self, key):
        if key=="Count":
            return len(self.sd["Timers"])
        elif key=="NonRender":
            return self.sd["Timers"]["Frame"]["Time"] - self.sd["Timers"]["Render"]["Time"]
        elif key=="SceneRender":
            return self.sd["Timers"]["Render"]["Time"] - self.sd["Timers"]["UI"]["Time"]
        else:
            raise IndexError()
        
class DerivedFieldGetter:
    def __init__(self,sd,**kwargs):
        self.sd = sd
        self.kwargs = kwargs

    def __getitem__(self, key):
        if key=="Timers":
            return DerivedTimers(self.sd, **self.kwargs)
        elif key=="SelfTimers":
            return DerivedSelfTimers(self.sd, **self.kwargs)
        else:
            raise IndexError()

# optimized to avoid blowing out memory when processing huge files,
# but not especially efficient the way we parse twice.
def get_frame_record(filename,**kwargs):
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
            sd['Derived'] = DerivedFieldGetter(sd, **kwargs)
            yield sd
            
            if frame_count % 100 == 0:
                # avoid accumulating lots of junk under root
                root.clear()

    print "Read",frame_count,"frame records"
    f.close()

def sd_extract_field(sd,key):
    chain = key.split(".")
    t = sd
    try:
        for subkey in chain:
            t = t[subkey]
        return t
    except KeyError:
        return None

def collect_pandas_frame_data(filename, fields, max_records, prune=None, **kwargs):
    # previously generated cvs file?
    if re.match(".*\.csv$", filename):
        return pd.DataFrame.from_csv(filename)

    # otherwise assume we're processing a .slp file
    frame_data = []
    frame_count = 0
    header = sorted(fields)
    iter_rec = iter(get_frame_record(filename,**kwargs))

    for sd in iter_rec:
        values = [sd_extract_field(sd,key) for key in header]
        
        frame_data.append(values)

        frame_count += 1
        if max_records is not None and frame_count >= max_records:
            break

    return pd.DataFrame(frame_data,columns=header)

def get_child_info(filename):
    child_info = {}
    iter_rec = iter(get_frame_record(filename))
    sd = next(iter_rec)
    for timer in sd["Timers"]:
        if "Parent" in sd["Timers"][timer]:
            parent = sd["Timers"][timer]["Parent"]
            child_info.setdefault(parent,[]).append(timer)
    return child_info
    
def process_by_outfit(pd_data, fig_name):
    outfit_key = "Avatars.Self.OutfitName"
    arc_key = "Avatars.Self.ARCCalculated"
    #time_key = "Timers.UI.Time"
    #time_key = "Timers.Frame.Time"
    time_key = "Derived.Timers.SceneRender"
    curr_outfit = None
    #print "pd_data"
    #print pd_data
    #print "rows"
    #for i, fd in enumerate(pd_data.values):
    #    print i, fd
    outfit_column = pd_data[outfit_key]
    outfit_key_frames = [i for i, outfit in enumerate(outfit_column) if isinstance(outfit,basestring)]
    outfit_spans = [outfit_key_frames[i+1]-outfit_key_frames[i] for i in xrange(len(outfit_key_frames)-1)]
    outfit_spans.append(len(pd_data)-outfit_key_frames[-1])
    assert(len(outfit_key_frames)==len(outfit_spans))
    #for (k,s) in zip(outfit_key_frames,outfit_spans):
    #    print "START",k,"SPAN",s,"OUTFIT",outfit_column[k],"ARC",pd_data[arc_key][k]
    spandict = dict(zip(outfit_key_frames, outfit_spans))
    arcs = []
    times = []
    labels = []
    stddev = []
    timespan = []
    timespans = []
    for outfit, group in itertools.groupby(outfit_key_frames,key=lambda k: outfit_column[k]):
        max_span = 0
        max_key = 0
        for start_frame in group:
            span = spandict[start_frame]
            if span > max_span:
                max_span = span
                max_key = start_frame
        print "OUTFIT",outfit, "ARC", pd_data[arc_key][max_key], "START_FRAME", max_key, "SPAN", max_span 
        timespan = pd_data[time_key][max_key:max_key+max_span]
        print "render avg", np.average(timespan)
        print "render std", np.std(timespan)
        arcs.append(pd_data[arc_key][max_key])
        times.append(np.average(timespan))
        stddev.append(np.std(timespan))
        labels.append(outfit)
        timespans.append(timespan)
    print "CORRCOEFF", np.corrcoef(arcs,times)
    print "POLYFIT", np.polyfit(arcs,times,1)
    #res = plt.scatter(arcs, times, yerr=stddev)
    plt.errorbar(arcs, times, yerr=stddev, fmt='o')
    for label, x, y in zip(labels,arcs,times):
        plt.annotate(label, xy=(x,y))
    plt.gca().set_xlabel(arc_key)
    plt.gca().set_ylabel(time_key)
    if (fig_name) is not None:
        plt.gcf().savefig(fig_name, bbox_inches="tight")
    plt.clf()
    for timespan, label in zip(timespans,labels):
        fig = plt.figure()
        ax = fig.add_subplot(1,1,1)
        ax.hist(timespan, 50, label=label)
        fig.savefig("times_histo_outfit_" + label.replace(" ","_").replace("*","") + ".jpg", bbox_inches="tight")
        #plt.hist(timespan, 50, alpha=0.5, label=label)
    #plt.gcf().savefig("timespan_histo.jpg", bbox_inches="tight")
        
    
if __name__ == "__main__":

    default_fields = [ "Timers.Frame.Time", 
                       "Timers.Render.Time", 
                       "Timers.UI.Time", 
                       "Session.UniqueID", 
                       "Avatars.Self.ARCCalculated", 
                       "Avatars.Self.OutfitName", 
                       "Derived.Timers.NonRender", 
                       "Derived.Timers.SceneRender",
                       "Derived.SelfTimers.Render",
    ]

    child_info = get_child_info("performance.slp")
    all_timers = sorted(["Timers." + key + ".Time" for key in child_info.keys()])
    all_self_timers = sorted(["Derived.SelfTimers." + key for key in child_info.keys()])

    parser = argparse.ArgumentParser(description="analyze viewer performance files")
    parser.add_argument("--verbose", action="store_true", help="verbose flag")
    parser.add_argument("--summarize", action="store_true", help="show summary of results")
    parser.add_argument("--fields", help="specify fields to be extracted or calculated", nargs="+", default=default_fields)
    parser.add_argument("--by_outfit", action="store_true", help="break results down based on active outfit")
    parser.add_argument("--export", help="export results to specified file")
    parser.add_argument("--max_records", type=int, help="limit to number of frames to process") 
    parser.add_argument("infilename", help="name of performance or csv file", nargs="?", default="performance.slp")
    args = parser.parse_args()

    # handle special values for fields
    newargs = []
    for f in args.fields:
        if f == "all_timers":
            newargs.extend(all_timers)
        elif f == "all_self_timers":
            newargs.extend(all_self_timers)
        else:
            newargs.append(f)
    args.fields = newargs
    
    #timer_data = collect_frame_data(args.infilename, args.fields, args.max_records, 0.001)
    pd_data = collect_pandas_frame_data(args.infilename, args.fields, args.max_records, 0.001, children=child_info)
    
    if args.export:
        export(args.export, pd_data)

    if args.summarize:
        res = pd_data.describe()
        print res
        
    if args.by_outfit:
        process_by_outfit(pd_data,"arcs_vs_times.jpg")
        
    print "done"
