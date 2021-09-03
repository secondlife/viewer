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

# Given data from a "standard" (non-arctan) performance log, analyze the timer data in various ways.

import re
import argparse
import sys
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

def read_input(infilename):
    print "reading",infilename
    df = pd.read_csv(infilename)
    return df

def cleanup_data(df, discard_start):
    # drop non-timer columns
    non_timers = [col for col in df.columns if "Times" not in col]
    junk_timers = ["Total - Times", "root - Times"]
    df.drop(labels=non_timers + junk_timers,axis=1,inplace=True)
    tkey = "Frame - Times"
    ekey = "elapsed"
    df[ekey] = df[tkey].cumsum();
    total_elapsed = df[ekey].max()
    if total_elapsed > discard_start:
        print "removing first", discard_start, "seconds"
        df = df[df[ekey] > discard_start]
    else:
        print "total_elapsed", total_elapsed, "is less than cutoff value", discard_start, "not pruned"
    # convert all times to ms
    for col in df.columns:
        df[col] *= 1000
    return df

def make_histo(df, cols, val_range, image_name):
    tkey = "Frame - Times"
    num_plots = len(cols)
    fig = plt.figure(figsize=(6, 2*num_plots))
    fig.subplots_adjust(hspace=0.5)
    for i, col in enumerate(cols):
        print i, col, df[col].mean()
        ax = fig.add_subplot(num_plots,1,i+1)
        ax.hist(df[col],100,range=val_range)
        ax.set_title(col)
    fig.savefig(image_name)
    
def make_parallel_histo(dfs, df_names, cols, val_range, image_name):
    tkey = "Frame - Times"
    num_plots = len(cols) * len(dfs)
    num_subplots_per = len(dfs)
    fig = plt.figure(figsize=(6, 2*num_plots))
    fig.subplots_adjust(hspace=1.0)
    for i, col in enumerate(cols):
        for j, df in enumerate(dfs):
            print i, j, col, df[col].mean()
            mean_time = df[col].mean()
            std_time = df[col].std()
            ax = fig.add_subplot(num_plots,1,num_subplots_per*i+j+1)
            ax.hist(df[col],100,range=val_range,density=True)
            ax.set_title(col + "(" + df_names[j] + ")")
            ax.set_xlabel('time (ms)')
            ax.set_ylabel('freq')
            ax.axvline(mean_time,label="mean",color="red")
            #ax.axvspan(mean_time - std_time, mean_time + std_time, 0.3, 0.7, color="red", fill=False)
    fig.savefig(image_name)
    
def analyze_timers(df):
    print "analyze_timers"
    tkey = "Frame - Times"

    ekey = "elapsed"
    df[ekey] = df[tkey].cumsum()
    print df.mean().sort_values(ascending=False)[0:20]
    print "Total elapsed", df[ekey].max()

    fast_frame = df[tkey].quantile(0.05)
    slow_frame = df[tkey].quantile(0.95)
    very_slow_frame = df[tkey].quantile(0.99)
    df_fast = df[df[tkey]<=fast_frame]
    print "\nTop timers for fast frames:"
    print df_fast.mean().sort_values(ascending=False)[0:30]
    df_slow = df[df[tkey]>=slow_frame]
    df_very_slow = df[df[tkey]>=very_slow_frame]
    print "\nMedians for slow frames:"
    print df_fast.quantile(0.5).sort_values(ascending=False)[0:30]
    sys.exit(1)
    
    print "\nTop timers for slow frames:"
    print df_slow.mean().sort_values(ascending=False)[0:30]

    print "\nTop differences between fast and slow:"
    df_diff = (df_slow.mean()-df_fast.mean()).sort_values(ascending=False)[0:30]
    print (df_slow.mean()-df_fast.mean()).sort_values(ascending=False)[0:30]
    rows = list(df_diff.index.values)
    print rows
    start = rows.index("Frame - Times")
    df_diff_cols = rows[start:start+10]
    print df_diff_cols

    print "\nTop median differences between fast and slow:"
    df_diff = (df_slow.quantile(0.5)-df_fast.quantile(0.5)).sort_values(ascending=False)[0:30]
    print df_diff 
    rows = list(df_diff.index.values)
    start = rows.index("Frame - Times")
    df_diff_cols = rows[start:start+10]
    print "df_diff_cols", df_diff_cols
    speed_bucket_labels = ["fastest 5%", "all", "slowest 5%", "slowest 1%"] 
    make_parallel_histo([df_fast,df,df_slow,df_very_slow], speed_bucket_labels, df_diff_cols, [0,very_slow_frame], "df_high_diff_timers_histo.png") 

    slow_cols = list(df_slow.mean().sort_values(ascending=False)[0:20].index.values)
    start = slow_cols.index("Frame - Times")
    print "slow_cols", slow_cols
    make_parallel_histo([df_fast,df,df_slow,df_very_slow], speed_bucket_labels, slow_cols[start:start+10], [0,very_slow_frame], "df_slow_frames_histo.png") 
    #make_histo(df_slow, df_diff_cols, [0,slow_frame], "df_slow_histo.png") 

    df_sleep_cols = ["Frame - Times"]
    df_sleep_cols.extend([col for col in list(df.columns) if "Sleep" in col])
    start = df_sleep_cols.index("Frame - Times")
    make_parallel_histo([df_fast,df,df_slow,df_very_slow], speed_bucket_labels, df_sleep_cols, [0,slow_frame], "df_sleep_histo.png")

    for col in df_diff_cols:
        print "Stats: col", col, "count", len(df[col].values), "mean", df[col].mean(), "std", df[col].std(), \
            "5%", df[col].quantile(0.05), "median", df[col].quantile(0.5), "95%", df[col].quantile(0.95),"99.9%",df[col].quantile(0.999) 

parser = argparse.ArgumentParser(description="analyze viewer performance files")
parser.add_argument("infilename", help="name of performance file to read", nargs = "?", default="performance.csv")
parser.add_argument("--discard_start", help="number of seconds to drop at beginning", type=float, default=10.0)
args = parser.parse_args()

df = read_input(args.infilename)
df = cleanup_data(df,args.discard_start)
analyze_timers(df)
