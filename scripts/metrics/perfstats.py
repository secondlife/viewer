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
        ignore = []
        if "ignore" in self.kwargs:
            ignore = self.kwargs["ignore"]
        if "children" in self.kwargs:
            children = self.kwargs["children"]
            if children is not None:
                for child in children.get(key,[]):
                    if child in self.sd["Timers"]:
                        if not child in ignore:
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

def collect_pandas_frame_data(filename, fields, max_records, **kwargs):
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

def get_timer_info(filename):
    iter_rec = iter(get_frame_record(filename))
    sd = next(iter_rec)
    child_info = {}
    parent_info = {}
    for timer in sd["Timers"]:
        if "Parent" in sd["Timers"][timer]:
            parent = sd["Timers"][timer]["Parent"]
            child_info.setdefault(parent,[]).append(timer)
            parent_info[timer] = parent
    timer_keys = [timer for timer in sd["Timers"]]
    reparented_timers = []
    directly_reparented = []
    for timer in timer_keys:
        reparented = False
        curr_timer_data = sd["Timers"][timer]
        curr_timer = timer
        while curr_timer:
            # follow chain of parents to see if any have been reparented
            if curr_timer == "root":
                break
            if "EverReparented" in curr_timer_data and curr_timer_data["EverReparented"]:
                if curr_timer == timer:
                    directly_reparented.append(timer)
                reparented = True
                break
            if not "EverReparented" in curr_timer_data:
                print "timer",curr_timer,"has no EverReparented"
            if curr_timer in parent_info:
                curr_timer = parent_info[curr_timer]
                curr_timer_data = sd["Timers"][curr_timer]
            else:
                break
        if reparented:
            reparented_timers.append(timer)
            
    return (child_info, parent_info, timer_keys, reparented_timers, directly_reparented)
    
def get_child_info(filename):
    child_info = {}
    iter_rec = iter(get_frame_record(filename))
    sd = next(iter_rec)
    for timer in sd["Timers"]:
        if "Parent" in sd["Timers"][timer]:
            parent = sd["Timers"][timer]["Parent"]
            child_info.setdefault(parent,[]).append(timer)
    return child_info

def get_all_timer_keys(filename):
    iter_rec = iter(get_frame_record(filename))
    sd = next(iter_rec)
    return [timer for timer in sd["Timers"]]

def get_outfit_spans(pd_data):
    time_key = "Timers.Frame.Time"
    results = []
    outfit_key = "Avatars.Self.OutfitName"
    arc_key = "Avatars.Self.ARCCalculated"
    curr_outfit = None
    outfit_column = pd_data[outfit_key]
    outfit_key_frames = [i for i, outfit in enumerate(outfit_column) if isinstance(outfit,basestring)]
    outfit_spans = [outfit_key_frames[i+1]-outfit_key_frames[i] for i in xrange(len(outfit_key_frames)-1)]
    outfit_spans.append(len(pd_data)-outfit_key_frames[-1])
    assert(len(outfit_key_frames)==len(outfit_spans))
    spandict = dict(zip(outfit_key_frames, outfit_spans))
    for outfit, group in itertools.groupby(outfit_key_frames,key=lambda k: outfit_column[k]):
        max_span = 0
        max_key = 0
        # TODO allow multiple spans for same outfit, if sufficiently long
        for start_frame in group:
            span = spandict[start_frame]
            if span > max_span:
                max_span = span
                max_key = start_frame
        timespan = pd_data[time_key][max_key:max_key+max_span]
        low, high = np.percentile(timespan,5.0), np.percentile(timespan,95.0)
        print "OUTFIT",outfit, "ARC", pd_data[arc_key][max_key], "START_FRAME", max_key, "SPAN", max_span 
        outfit_rec = {"outfit":outfit, "arc": pd_data[arc_key][max_key], "start_frame": max_key, "span": max_span,
                      "avg": np.average(timespan.clip(low,high)), "std": np.std(timespan) } 
        results.append(outfit_rec)
    return results
    
def process_by_outfit(pd_data):
    arc_key = "Avatars.Self.ARCCalculated"
    time_key = "Timers.Frame.Time"

    arcs = []
    avg = []
    labels = []
    stddev = []
    timespan = []
    timespans = []
    xspans = []
    errorbars_low = []
    errorbars_high = []
    outfit_spans = get_outfit_spans(pd_data) 
    for outfit_span in outfit_spans:
        max_key = outfit_span["start_frame"]
        max_span = outfit_span["span"]
        outfit = outfit_span["outfit"]
        print "OUTFIT",outfit, "ARC", pd_data[arc_key][max_key], "START_FRAME", max_key, "SPAN", max_span 
        timespan = pd_data[time_key][max_key:max_key+max_span]
        print "render avg", np.average(timespan)
        print "render std", np.std(timespan)
        arcs.append(outfit_span["arc"])
        avg.append(outfit_span["avg"])
        stddev.append(outfit_span["std"])
        errorbars_low.append(outfit_span["avg"]-np.percentile(timespan,25.0))
        errorbars_high.append(np.percentile(timespan,75.0)-outfit_span["avg"])
        labels.append(outfit)
        timespans.append(timespan)
    print "CORRCOEFF", np.corrcoef(arcs,avg)
    print "POLYFIT", np.polyfit(arcs,avg,1)
    #res = plt.scatter(arcs, avg, yerr=stddev)
    plt.errorbar(arcs, avg, yerr=(errorbars_low, errorbars_high), fmt='o')
    for label, x, y in zip(labels,arcs,avg):
        plt.annotate(label, xy=(x,y))
    for xspan, y in zip(xspans,avg):
        plt.plot((xspan[0],y),(xspan[1],y),'k-')
    plt.gca().set_xlabel(arc_key)
    plt.gca().set_ylabel(time_key)
    plt.gcf().savefig("arcs_vs_times.jpg", bbox_inches="tight")
    plt.clf()
    nrows = len(timespans)
    all_times = [t for timespan in timespans for t in timespan]
    all_low, all_high = np.percentile(all_times,0), np.percentile(all_times,98.0)
    sorted_lists = sorted(itertools.izip(timespans, labels, avg), key=lambda x: x[2])
    timespans, labels, avg = [[x[i] for x in sorted_lists] for i in range(3)]
    fig = plt.figure(figsize=(6, 2*nrows))
    fig.subplots_adjust(wspace=1.0)
    for i, (timespan, label, avg_val) in enumerate(zip(timespans,labels, avg)):
        #fig = plt.figure()
        ax = fig.add_subplot(nrows,1,i+1)
        low, high = np.percentile(timespan,2.0), np.percentile(timespan,98.0)
        #clipped_timespan = timespan.clip(low,high)
        ax.hist(timespan, 100, label=label, range=(all_low,all_high), alpha=0.3)
        plt.title(label)
        plt.axvline(x=avg_val)
        #fig.savefig("times_histo_outfit_" + label.replace(" ","_").replace("*","") + ".jpg", bbox_inches="tight")
    plt.tight_layout()
    fig.savefig("times_histo_outfits.jpg")
        
def plot_time_series(pd_data,fields):
    time_key = "Timers.Frame.Time"
    print "plot_time_series",fields
    outfit_spans = get_outfit_spans(pd_data)
    for f in fields:
        #pd_data[f] = pd_data[f].clip(0,0.05)
        ax = pd_data.plot(y=f, alpha=0.3)
        for outfit_span in outfit_spans:
            x0,x1 = outfit_span["start_frame"], outfit_span["start_frame"] + outfit_span["span"]
            y = outfit_span["avg"]
            print "annotate",(x0,x1),y
            plt.plot((x0,x1),(y,y),"b-")
        ax.set_ylim([0,.1])
        fig = ax.get_figure()
        fig.savefig("time_series_" + f + ".jpg")
    
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

    parser = argparse.ArgumentParser(description="analyze viewer performance files")
    parser.add_argument("--verbose", action="store_true", help="verbose flag")
    parser.add_argument("--summarize", action="store_true", help="show summary of results")
    parser.add_argument("--fields", help="specify fields to be extracted or calculated", nargs="+", default=default_fields)
    parser.add_argument("--timers", help="specify timer keys to be added to fields", nargs="+", default=[])
    parser.add_argument("--child_timers", help="include children of specified timer keys in fields", nargs="+", default=[])
    parser.add_argument("--no_reparented", action="store_true", help="ignore timers that have been reparented directly or indirectly")
    parser.add_argument("--export", help="export results to specified file")
    parser.add_argument("--max_records", type=int, help="limit to number of frames to process") 
    parser.add_argument("--by_outfit", action="store_true", help="break results down based on active outfit")
    parser.add_argument("--plot_time_series", nargs="+", default=[], help="show timers by frame")
    parser.add_argument("infilename", help="name of performance or csv file", nargs="?", default="performance.slp")
    args = parser.parse_args()

    print "start get_timer_info"
    child_info, parent_info, all_keys, reparented_timers, directly_reparented = get_timer_info("performance.slp")
    print "done get_timer_info"

    if args.no_reparented:
        print len(reparented_timers),"are reparented, of which",len(directly_reparented),"reparented directly"
        print ", ".join(sorted(reparented_timers))
        print
        print ", ".join(sorted(directly_reparented))
        all_keys = [key for key in all_keys if key not in reparented_timers]

    all_timers = sorted(["Timers." + key + ".Time" for key in all_keys])
    all_calls = sorted(["Timers." + key + ".Calls" for key in all_keys])
    all_self_timers = sorted(["Derived.SelfTimers." + key for key in all_keys])


    # handle special values for fields
    newargs = []
    for f in args.fields:
        if f == "all_timers":
            newargs.extend(all_timers)
        elif f == "all_self_timers":
            newargs.extend(all_self_timers)
        elif f == "all_calls":
            newargs.extend(all_calls)
        else:
            newargs.append(f)
    args.fields = newargs

    ignore_list = []
    if args.no_reparented:
        ignore_list = reparented_timers

    for t in args.timers:
        args.fields.append("Timers." + t + ".Time")

    for t in args.child_timers:
        args.fields.append("Timers." + t + ".Time")
        if t in child_info:
            for c in child_info[t]:
                if not c in ignore_list:
                    args.fields.append("Timers." + c + ".Time")

    print "FIELDS",args.fields
        
    #timer_data = collect_frame_data(args.infilename, args.fields, args.max_records, 0.001)
    pd_data = collect_pandas_frame_data(args.infilename, args.fields, args.max_records, children=child_info, ignore=ignore_list)

    if args.verbose:
        print "Timer Ancestry"
        for key in sorted(child_info.keys()):
            print key,child_info[key]
    
    if args.export:
        export(args.export, pd_data)

    if args.summarize:
        res = pd_data.describe()
        print res
        
    if args.by_outfit:
        process_by_outfit(pd_data)

    if args.plot_time_series:
        plot_time_series(pd_data, args.plot_time_series)

    print "done"
