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
import statsmodels.formula.api as smf

#=========================================================================================================
# for these, we will sum all counts found in all attachments
sum_graphic_properties = ["media_faces"]

ref_props = ["Avatars.Self.ARCCalculated",
             "Avatars.Self.ARCCalculated2",
             "active_triangle_count",
]

# for these, we will compute the fraction of triangles affected by the setting
face_count_props = ["alpha", "glow", "shiny", "bumpmap", "produces_light", "materials"]
tri_frac_props = [prop + "_frac" for prop in face_count_props]
#print "tri_frac_props", tri_frac_props

tri_count_props = [prop + "_count" for prop in face_count_props]
computed_props = ["raw_triangle_count"]

texture_pixels = ["tex_diffuse_kp", "tex_sculpt_kp", "tex_specular_kp", "tex_normal_kp"]
texture_counts = ["tex_diffuse_count", "tex_sculpt_count", "tex_specular_count", "tex_normal_count"]
texture_incomplete = ["tex_diffuse_incomplete", "tex_sculpt_incomplete", "tex_specular_incomplete", "tex_normal_incomplete"]
texture_props = texture_pixels + texture_counts + texture_incomplete


triangle_lod_keys = ["triangle_count_lowest", "triangle_count_low", "triangle_count_mid", "triangle_count_high"]

model_props = tri_frac_props
#=========================================================================================================

def xstr(x):
    if x is None:
        return ''
    return str(x)

def get_default_export_name(pd_data,prefix="performance"):
    res = None
    session_id = pd_data["Session.UniqueSessionUUID"][0]
    name = prefix + "_" + str(session_id) + ".csv"

    res = name.replace(":",".").replace("Z","").replace("T","-")
    print "session_id",session_id,"name",res
    return res
    
def export(filename, timers):
    print "export",filename
    if filename=="auto":
        filename = get_default_export_name(timers)
        print "saving to",filename
    if filename:
        if re.match(".*\.csv$", filename):
            timers.to_csv(filename)
            return filename
        else:
            print "unknown extension for export",filename
    return None

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

class AttachmentsDerivedField:
    def __init__(self,sd,**kwargs):
        self.sd = sd
        self.kwargs = kwargs
    def __getitem__(self, key):
        attachments = sd_extract_field(self.sd,"Avatars.Self.Attachments") 
        if attachments:
            if key=="attachment_count":
                return len(attachments)
            elif key=="mesh_attachment_count":
                count = 0
                for att in attachments:
                    for prim in att["prims"]:
                        if prim["m_mesh_vols"]>0:
                            count += 1
                            continue
                return count
            elif key=="rigged_mesh_attachment_count":
                count = 0
                for att in attachments:
                    for prim in att["prims"]:
                        if prim["m_weighted_mesh_vols"]>0:
                            count += 1
                            continue
                return count
            elif key in triangle_lod_keys:
                lod_tris = sum([prim["m_" + key] for att in attachments for prim in att["prims"]])
                return lod_tris
            elif key=="active_triangle_count":
                total = sum([prim["m_triangle_count"] for att in attachments for prim in att["prims"]])
                return total
            elif key == "produces_light_frac":
                prim_vol_prop = "m_" + key.replace("_frac","_vols")
                prop_tris = sum([prim["m_triangle_count"] for att in attachments for prim in att["prims"]
                                 if prim[prim_vol_prop]>0])
                all_tris = sum([prim["m_triangle_count"] for att in attachments for prim in att["prims"]])
                return 1.0 * prop_tris/all_tris
            elif key in tri_frac_props:
                prim_face_prop = "m_" + key.replace("_frac","_faces")
                all_tris = sum([prim["m_triangle_count"] for att in attachments for prim in att["prims"]])
                prop_tris = sum([prim["m_triangle_count"] for att in attachments for prim in att["prims"]
                                 if prim[prim_face_prop]>0])
                return 1.0 * prop_tris/all_tris
            elif key=="produces_light_count":
                prim_vol_prop = "m_" + key.replace("_count","_vols")
                prop_tris = sum([prim["m_triangle_count"] for att in attachments for prim in att["prims"]
                                 if prim[prim_vol_prop]>0])
                return prop_tris
            elif key in tri_count_props:
                prim_face_prop = "m_" + key.replace("_count","_faces")
                prop_tris = sum([prim["m_triangle_count"] for att in attachments for prim in att["prims"]
                                 if prim[prim_face_prop]>0])
                return prop_tris
            elif key in texture_pixels:
                tex_type = key.replace("tex_","").replace("_kp","")
                pixels = 0
                for att in attachments:
                    if tex_type in att["textures"]:
                        tex_array = att["textures"][tex_type]
                        for tex in tex_array:
                            if "width" in tex and "height" in tex:
                                im_pixels = tex["width"] * tex["height"]
                                #print "pixels", im_pixels, ":", tex["width"], "x", tex["height"]
                                # Deflate this from raw pixel count so we don't overflow the counter
                                pixels += im_pixels/(32*32)
                            else:
                                print "bad tex", tex
                return pixels
            elif key in texture_incomplete:
                tex_type = key.replace("tex_","").replace("_incomplete","")
                incomplete = 0
                for att in attachments:
                    if tex_type in att["textures"]:
                        tex_array = att["textures"][tex_type]
                        for tex in tex_array:
                            if "discard" not in tex or tex["discard"] != 0:
                                incomplete += 1
                return incomplete
            elif key in texture_counts:
                tex_type = key.replace("tex_","").replace("_count","")
                count = 0
                for att in attachments:
                    if tex_type in att["textures"]:
                        tex_array = att["textures"][tex_type]
                        count += len(tex_array)
                return count
            else:
                print "Unknown key", key
                raise IndexError()

class DerivedAvatarField:
    def __init__(self,sd,**kwargs):
        self.sd = sd
        self.kwargs = kwargs
    def __getitem__(self, key):
        if key=="AttachmentCount":
            attachments = sd_extract_field(self.sd,"Avatars.Self.Attachments") 
            if attachments:
                return len(attachments)
            return None
        if key=="Attachments":
            return AttachmentsDerivedField(self.sd,**self.kwargs)
        else:
            raise IndexError()

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
        elif key=="Avatar":
            return DerivedAvatarField(self.sd, **self.kwargs)
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
    try:
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
    except etree.XMLSyntaxError:
        print "Fell off end of document"

    print "Read",frame_count,"frame records"
    f.close()

def sd_extract_field(sd,key,default_val=None):
    chain = key.split(".")
    t = sd
    try:
        for subkey in chain:
            t = t[subkey]
        return t
    except KeyError:
        return default_val

def add_computed_columns(frame_data):
    frame_data['raw_triangle_count'] = frame_data['active_triangle_count']
    for prop in tri_count_props:
        frame_data['raw_triangle_count'] -= frame_data[prop]
    frame_data['elapsed_time'] = frame_data['frame_time'].cumsum()

def filter_extreme_times(fd, pct):
    times = fd['frame_time']
    extreme = np.percentile(times, pct)
    return fd[fd.frame_time<extreme]

def collect_overview(filename, **kwargs):
    #print "collect_overview", filename
    result = dict()
    if not re.match(".*\.atp", filename):
        print "filename", filename, "is not a .atp file"
        return result
    iter_rec = iter(get_frame_record(filename,**kwargs))
    first_rec = next(iter_rec)
    #print "got first_rec, keys", first_rec.keys()
    return first_rec
    
def collect_pandas_frame_data(filename, fields, max_records, **kwargs):
    # previously generated csv file?
    if re.match(".*\.csv$", filename):
        if args.filter_csv:
            return pd.read_csv(filename, index_col=0)[fields]
        else:
            return pd.read_csv(filename, index_col=0)

    # otherwise assume we're processing a .atp file
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

def get_nested_timer_info(filename):
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

def fill_blanks(df):
    print "fill_blanks"
    for col in df:
        if col.startswith("Timers."):
            df[col].fillna(0.0, inplace=True)
        else:
            # Intermittently recorded, can just fill in
            df[col].fillna(method="ffill", inplace=True)
            # catch anything before the first recorded value
            df[col].fillna(value=0, inplace=True)

# Shorten 33,412 to 33K, etc., for simplified display.
# Input: float
# Output: string
def abbrev_number(f):
    if f <= 0.0:
        return "0"
    if f <= 1e3:
        return str(int(f))
    if f <= 1e6:
        return str(int(f/1e3))+"K"
    if f <= 1e9:
        return str(int(f/1e6))+"M"
    if f <= 1e12:
        return str(int(f/1e9))+"G"
    else:
        return str(int(f/1e12))+"T"

def estimate_cost_coeffs(outfit_spans, props):
    tckey = "Derived.Avatar.Attachments.active_triangle_count"
    modal_tri_count = outfit_spans[tckey].mode()[0]
    print "modal_tris", modal_tri_count
    is_modal = outfit_spans[tckey]==modal_tri_count
    modal_spans = outfit_spans[is_modal]
    span_clusters = modal_spans.groupby(props)

    canonical = dict()
    for name, group in span_clusters:
        #print "span_cluster", name, type(name)
        highest_duration = group.loc[group["duration"].idxmax()]
        canonical[name] = highest_duration
        #print "-- highest duration row", highest_duration
    base_prop_key = tuple([0.0 for field in props])
    print "base_prop", base_prop_key
    base_span = canonical[base_prop_key]
    print "base_span:", base_span
    for prop_key, span in canonical.iteritems():
        if prop_key != base_prop_key:
            diff_keys = []
            for key in props:
                if base_span[key] != span[key]:
                    diff_keys.append(key)
            print "Comparing test span, starting", span["start_frame"], "to base span, starting", base_span["start_frame"]
            coeff = span["avg"]/base_span["avg"]
            print "est coeff:", diff_keys, coeff

def print_arc_coeffs(lm_params):
    for key in lm_params.index:
        coeff = lm_params[key]/lm_params['raw_triangle_count']
        print "ARC coeff", key, coeff

def linear_regression_model_outfits(outfit_spans, props):
    print "Linear regression model based on outfit_spans:"
    driver_vars = tri_count_props
    driver_vars.append("raw_triangle_count")
    formula_str = "avg ~ " + " + ".join(driver_vars)
    print formula_str
    lm = smf.ols(formula = formula_str, data = outfit_spans).fit()
    print lm.params
    #print lm.summary()
    print_arc_coeffs(lm.params)

def linear_regression_frame_times(pd_data, props, time_key="frame_time", **kwargs):
    window = kwargs["window"] or 1
    print "window", window
    pd_data['smooth_time'] = pd_data['frame_time'].rolling(window, center=True, min_periods=1).mean()
    print "Linear regression model based on frame times, hopefully filtered into periods with fixed content"
    driver_vars = tri_count_props
    driver_vars.append("raw_triangle_count")
    formula_str = time_key + " ~ " + " + ".join(driver_vars)
    print formula_str
    lm = smf.ols(formula = formula_str, data = pd_data).fit()
    print lm.params
    #print lm.summary()
    print_arc_coeffs(lm.params)

def get_outfit_spans(pd_data, cost_key="Avatars.Self.ARCCalculated"): 
    results = []
    results = []
    outfit_key = "Avatars.Self.OutfitName"
    time_key="frame_time"
    pd_data['span_num'] = (pd_data[cost_key].shift(1) != pd_data[cost_key]).astype(int).cumsum()

    std_props = ref_props + model_props + texture_props + tri_count_props + computed_props
    print "outfit std props", std_props
    group_keys = [outfit_key, cost_key, 'span_num'] + std_props
    grouped = pd_data.groupby(group_keys)
    pd_filtered = grouped.filter(lambda x: x[time_key].sum() > 10.0 and len(x)>200)
    if args.verbose:
        print "Grouped has",len(grouped),"groups"
        print "orig data lines", len(pd_data), "filtered", len(pd_filtered)
        print "columns are:", pd_data.columns
        print "std_props is", std_props
    for name, group in grouped:
        #print name, len(group) 
        #print "group:", group
        num_frames = len(group)
        times = group[time_key]
        duration = sum(times)
        if (num_frames) > 200 and (duration > 10.0):
            kv = dict(zip(group_keys, name))
            #print kv
            group_label = kv[outfit_key]
            differs = []
            for k in tri_frac_props:
                if kv[k] != group[k].iloc[0]:
                    print "name ordering issue for", k
                if group[k].iloc[0] != 0:
                #print k
                    differs.append(k.replace("_frac",""))
            if len(differs):
                group_label += "(" + ",".join(differs) + ")"
            print "group_label", group_label
        
            outfit_rec = {"outfit": name[0], 
                          "group_label": group_label,
                          "cost": name[1],
                          "duration": duration,
                          "start_time": group['elapsed_time'].iloc[0],
                          "end_time": group['elapsed_time'].iloc[-1],
                          "num_frames": num_frames,
                          "group": group,
                          "start_frame": min(group.index),
                          "avg": np.percentile(times, 50.0), #np.average(timespan.clip(low,high)), 
                          "std": np.std(times), 
                          "times": times}
            for f in std_props:
                outfit_rec[f] = group.iloc[0][f] # all values are equal after groupby, can use any index
            results.append(outfit_rec)
    if args.verbose:
        print len(results),"groups found of sufficient duration and frame count"
    df =  pd.DataFrame(results)
    return (df, pd_filtered)

def process_by_outfit(pd_data, cost_key="Avatars.Self.ARCCalculated", **kwargs):
    time_key = "frame_time"
    (outfit_spans, pd_filtered) = get_outfit_spans(pd_data, cost_key)
    outfits_csv_filename = get_default_export_name(pd_data,"outfits")
    outfit_spans = outfit_spans.sort_values("start_frame")
    outfit_spans.to_csv(outfits_csv_filename, columns=outfit_spans.columns.difference(['group','times']))

#    estimate_cost_coeffs(outfit_spans, model_props)
    linear_regression_model_outfits(outfit_spans, model_props)
    linear_regression_frame_times(pd_filtered, model_props, time_key="smooth_time", **kwargs)

    #outfit_spans.to_csv("linreg_" + outfits_csv_filename, columns=outfit_spans.columns.difference(['group','times']))

    all_times = []
    num_rows = outfit_spans.shape[0]
    errorbars_low = []
    errorbars_high = []
    labels = []
    timeses = []

    for index, outfit_span in outfit_spans.iterrows():
        outfit = outfit_span["outfit"]
        group_label = outfit_span["group_label"]
        avg = outfit_span["avg"]
        arc = outfit_span["cost"]
        times = outfit_span["times"]
        num_frames = outfit_span["num_frames"]
        timeses.append(times)
        all_times.extend(times)
        errorbar_low = (avg-np.percentile(times,25.0))
        errorbar_high = (np.percentile(times,75.0)-avg)
        errorbars_low.append(errorbar_low)
        errorbars_high.append(errorbar_high)
        label = group_label + " frames " + str(num_frames)
        labels.append(label)
        per_outfit_csv_filename = label.replace(" ","_").replace("*","") + ".csv"
        outfit_span["group"].to_csv(per_outfit_csv_filename)

    avgs = outfit_spans["avg"]
    costs = outfit_spans[cost_key]
    arcs = outfit_spans["cost"]

    #if num_rows>1:
    #    try:
    #        corr = np.corrcoef([costs,avgs,arcs])
    #        print corr
    #        print "CORRELATION between",cost_key,"and",time_key,"is:",corr[0][1]
    #        columns = outfit_spans.columns.difference(['group','times','outfit'])
    #        subspans = outfit_spans[columns].T
    #        #print subspans.describe()
    #        print subspans.values
    #        allcorr = np.corrcoef(subspans.values)
    #        print "columns",len(columns),columns
    #        print allcorr
    #    except:
    #        print "CORCOEFF failed"
    #        raise

    fig = plt.figure()
    plt.errorbar(costs, avgs, yerr=(errorbars_low, errorbars_high), fmt='o')
    for label, x, y in zip(labels,costs,avgs):
        plt.annotate(label, xy=(x,y))
    plt.gca().set_xlabel(cost_key)
    plt.gca().set_ylabel(time_key)
    plt.gcf().savefig("costs_vs_times.jpg", bbox_inches="tight")
    plt.clf()

    all_low, all_high = np.percentile(all_times,0), np.percentile(all_times,98.0)
    fig = plt.figure(figsize=(6, 2*num_rows))
    fig.subplots_adjust(wspace=1.0)
    for i, (times, label, avg_val) in enumerate(zip(timeses,labels, avgs)):
        #fig = plt.figure()
        ax = fig.add_subplot(num_rows,1,i+1)
        low, high = np.percentile(times,2.0), np.percentile(times,98.0)
        ax.hist(times, 100, label=label, range=(all_low,all_high), alpha=0.3)
        plt.title(label)
        plt.axvline(x=avg_val)
    plt.tight_layout()
    fig.savefig("times_histo_outfits.jpg")
        
def plot_time_series(pd_data,fields):
    print "plot_time_series",fields
    (outfit_spans, ignored) = get_outfit_spans(pd_data)
    #pd_data = filter_extreme_times(pd_data, 95.0)
    maxes = pd_data[fields].max(axis=1)
    ax = pd_data.plot(x='elapsed_time', y=fields, alpha=0.3)#, kind='scatter')
    ylim = 1.2 * np.percentile(maxes,95.0)
    print "ylim",ylim
    ax.set_ylim(0.0, ylim)
    for f in fields:
        #pd_data[f] = pd_data[f].clip(0,0.05)
        for index, outfit_span in outfit_spans.iterrows():
            index = outfit_span["group"].index
            x0 = outfit_span["start_time"]
            x1 = outfit_span["end_time"]
            y = outfit_span["avg"]
            cost = outfit_span["cost"]
            outfit = outfit_span["outfit"]
            print "annotate",outfit,(x0,x1),y
            plt.plot((x0,x1),(y,y),"b-")
            plt.text((x0+x1)/2, y, outfit + " cost " + abbrev_number(cost) , ha='center', va='bottom', rotation='vertical')
    fig = ax.get_figure()
    fig.savefig("time_series_" + "+".join(fields) + ".jpg")

def compare_frames(a,b):
    fill_blanks(a)
    fill_blanks(b)
    print "Comparing two data frames"
    a_numeric = [col for col in a.columns.values if a[col].dtype=="float64"]
    b_numeric = [col for col in b.columns.values if b[col].dtype=="float64"]
    shared_columns = sorted(set(a_numeric).intersection(set(b_numeric)))
    print "compare_frames found",len(shared_columns),"shared columns"
    names = [col for col in shared_columns]
    mean_a = [a[col].mean() for col in shared_columns]
    mean_b = [b[col].mean() for col in shared_columns]
    abs_diff_mean = [abs(a[col].mean()-b[col].mean()) for col in shared_columns]
    diff_mean_pct = [(100.0*(b[col].mean()-a[col].mean())/a[col].mean() if a[col].mean()!=0.0 else 0.0) for col in shared_columns]
    compare_df = pd.DataFrame({"names": names, 
                               "mean_a": mean_a, 
                               "mean_b": mean_b, 
                               "abs_diff_mean": abs_diff_mean,
                               "diff_mean_pct": diff_mean_pct,
    })
    print compare_df.describe()
    compare_df.to_csv("compare.csv")
    return compare_df

def extract_percent(df, key="frame_time", low=0.0, high=100.0, filename="extract_percent.csv"):
    df.to_csv("percent_input.csv")
    result = df
    result["Avatars.Self.OutfitName"] = df["Avatars.Self.OutfitName"].fillna(method='ffill')
    result["Avatars.Self.ARCCalculated"] = df["Avatars.Self.ARCCalculated"].fillna(method='ffill')
    result = result[result[key] > result[key].quantile(low/100.0)]
    result = result[result[key] < result[key].quantile(high/100.0)]
    result.to_csv(filename)

def show_usage():
    print """
    % perfstats.py --export some_file.atp   # convert some_file.atp to some_file.csv
    % perfstats.py --by_outfit some_file.{atp,csv}    # show stats breakdown by time spans of constant outfit and ARC
    """
    
if __name__ == "__main__":

    default_fields = [ "Timers.Frame.Time", 
                       "Session.UniqueHostID", 
                       "Session.UniqueSessionUUID", 
                       "ViewerInfo.RENDER_QUALITY",
                       "Summary.Timestamp", 
                       "Avatars.Self.ARCCalculated", 
                       "Avatars.Self.ARCCalculated2", 
                       "Avatars.Self.OutfitName", 
                       "Avatars.Self.AttachmentSurfaceArea",
                       "Derived.Avatar.Attachments.attachment_count",
                       "Derived.Avatar.Attachments.mesh_attachment_count",
                       "Derived.Avatar.Attachments.rigged_mesh_attachment_count",
                       "Derived.Avatar.Attachments.active_triangle_count",
    ]
    default_fields.extend(["Derived.Avatar.Attachments." + key for key in triangle_lod_keys])
    default_fields.extend(["Derived.Avatar.Attachments." + key for key in tri_count_props])
    default_fields.extend(["Derived.Avatar.Attachments." + key for key in tri_frac_props])
    default_fields.extend(["Derived.Avatar.Attachments." + key for key in texture_props])

    parser = argparse.ArgumentParser(description="analyze viewer performance files, use --usage for details")
    parser.add_argument("--verbose", action="store_true", help="verbose flag")
    parser.add_argument("--summarize", action="store_true", help="show summary of results")
    parser.add_argument("--fields", help="specify fields to be extracted or calculated", nargs="+", default=[])
    parser.add_argument("--timers", help="specify timer keys to be added to fields", nargs="+", default=[])
    parser.add_argument("--filter_csv", action="store_true", help="restrict to requested fields/timers when reading csv files too")
    parser.add_argument("--child_timers", help="include children of specified timer keys in fields", nargs="+", default=[])
    parser.add_argument("--no_reparented", action="store_true", help="ignore timers that have been reparented directly or indirectly")
    parser.add_argument("--export", action="store_true", help="export results as csv")
    parser.add_argument("--max_records", type=int, help="limit to number of frames to process") 
    parser.add_argument("--by_outfit", action="store_true", help="break results down based on active outfit")
    parser.add_argument("--plot_time_series", nargs="+", default=[], help="show timers by frame")
    parser.add_argument("--extract_percent", nargs="+", metavar="blah", help="extract subset based on frame time")
    parser.add_argument("--compare", help="compare infilename to specified file")
    parser.add_argument("--overview", help="show one-line summary for each stats file", nargs="+")
    parser.add_argument("--usage", action="store_true", help="show typical usage examples")
    parser.add_argument("infilename", help="name of performance or csv file", nargs="?", default="performance.atp")
    args = parser.parse_args()

    if (args.usage):
        show_usage()
        sys.exit(0)
        
    if (args.overview):
        #print "getting overview for", args.overview
        for filename in args.overview:
            sd = collect_overview(filename)
            if sd:
                print filename,\
                    "User", sd_extract_field(sd,"Session.DebugInfo.LoginName"),\
                    "Region", '"' + sd_extract_field(sd,"Session.DebugInfo.CurrentRegion") + '"',\
                    "StartTime", sd_extract_field(sd,"Session.StartTime")
                    
            #except:
            #    print "Failed to process", filename
            #    pass
        sys.exit(0)
    
    all_timer_keys = ["Frame"]
    all_self_timers = None
    child_info = None

    if args.no_reparented:
        print len(reparented_timers),"are reparented, of which",len(directly_reparented),"reparented directly"
        print ", ".join(sorted(reparented_timers))
        print
        print ", ".join(sorted(directly_reparented))
        all_timer_keys = [key for key in all_timer_keys if key not in reparented_timers]

    all_timers = sorted(["Timers." + key + ".Time" for key in all_timer_keys])
    all_calls = sorted(["Timers." + key + ".Calls" for key in all_timer_keys])

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
    args.fields.extend(newargs)
    if "no_default" not in args.fields:
        args.fields.extend(default_fields)
    while "no_default" in args.fields:
        args.fields.remove("no_default")

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

    args.fields = list(set(args.fields))
    print "FIELDS",args.fields
        
    print "collecting frame data, infilename",args.infilename
    pd_data = collect_pandas_frame_data(args.infilename, args.fields, args.max_records, children=child_info, ignore=ignore_list)
    pd_data.rename(inplace=True, columns = lambda col: col.replace("Derived.Avatar.Attachments.",""))
    pd_data.rename(inplace=True, columns = lambda col: col.replace("Timers.Frame.Time","frame_time"))
    fill_blanks(pd_data)
    add_computed_columns(pd_data)
    print "driver data means:"
    for p in tri_frac_props:
        print p, pd_data[p].mean()
    for p in tri_count_props:
        print p, pd_data[p].mean()

    if args.compare:
        compare_data = collect_pandas_frame_data(args.compare, args.fields, args.max_records, children=child_info, ignore=ignore_list)

        compare_frames(pd_data, compare_data)

    if args.verbose:
        print "Timer Ancestry"
        for key in sorted(child_info.keys()):
            print key,child_info[key]

        
    print "args.export",args.export
    if args.export:
        print "Calling export",args.export
        export_file = export("auto", pd_data)
        if export_file is not None:
            print "reloading data from exported file",export_file
            pd_data = collect_pandas_frame_data(export_file, args.fields, args.max_records, children=child_info, ignore=ignore_list)
        else:
            raise Error() 
            

    if args.extract_percent:
        print "extract percent",args.extract_percent
        (low_pct,high_pct,filename) = args.extract_percent
        low_pct = float(low_pct)
        high_pct = float(high_pct)
        print "extract percent",low_pct,high_pct,filename
        extract_percent(pd_data,"frame_time",low_pct,high_pct,filename)
            
    if args.summarize:
        pd_data = pd_data.fillna(0.0)
        pd.set_option('max_rows', 500)
        pd.set_option('max_columns', 500)
        res = pd_data.describe(include='all')
        print res
        medians = {} 
        #print "columns are",pd_data.columns.values
        for f in pd_data.columns.values:
            if pd_data[f].dtype in ["float64"]:
                medians[f] = pd_data[f].quantile(0.5)
                print f, "median", medians[f]
        medl = sorted(medians.keys(), key=lambda x: medians[x])
        for f in medl:
            print "median", f, medians[f]
        
    if args.by_outfit:
        # how sensitive are the results to this?
        #process_by_outfit(pd_data,cost_key="Avatars.Self.ARCCalculated", window=1) #Derived.Avatar.Attachments.active_triangle_count")
        process_by_outfit(pd_data,cost_key="Avatars.Self.ARCCalculated", window=10) #Derived.Avatar.Attachments.active_triangle_count")
        #process_by_outfit(pd_data,cost_key="Avatars.Self.ARCCalculated", window=100) #Derived.Avatar.Attachments.active_triangle_count")

    if args.plot_time_series:
        plot_time_series(pd_data, args.plot_time_series)

    print "done"
