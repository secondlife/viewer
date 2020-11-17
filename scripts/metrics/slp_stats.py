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
    df.drop(labels=non_timers,axis=1,inplace=True)
    tkey = "Frame - Times"
    ekey = "elapsed"
    df[ekey] = df[tkey].cumsum();
    total_elapsed = df[ekey].max()
    if total_elapsed > discard_start:
        print "removing first", discard_start, "seconds"
        df = df[df[ekey] > discard_start]
    else:
        print "total_elapsed", total_elapsed, "is less than cutoff value", discard_start, "not pruned"
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
    fig = plt.figure(figsize=(6, 2*num_plots))
    fig.subplots_adjust(hspace=0.5)
    for i, col in enumerate(cols):
        for j, df in enumerate(dfs):
            print i, j, col, df[col].mean()
            ax = fig.add_subplot(num_plots,1,3*i+j+1)
            ax.hist(df[col],100,range=val_range,density=True)
            ax.set_title(col + "(" + df_names[j] + ")")
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
    print rows
    start = rows.index("Frame - Times")
    df_diff_cols = rows[start:start+10]
    print df_diff_cols

    make_parallel_histo([df_fast,df,df_slow], ["FAST", "ALL", "SLOW"], df_diff_cols, [0,very_slow_frame], "df_parallel_histo.jpg") 
    #make_histo(df_slow, df_diff_cols, [0,slow_frame], "df_slow_histo.jpg") 

    df_sleep_cols = ["Frame - Times"]
    df_sleep_cols.extend([col for col in list(df.columns) if "Sleep" in col])
    make_parallel_histo([df_fast,df,df_slow], ["FAST", "ALL", "SLOW"], df_sleep_cols, [0,slow_frame], "df_sleep_histo.jpg")

    for col in df_diff_cols:
        print "Stats: col", col, "count", len(df[col].values), "mean", df[col].mean(), \
            "5%", df[col].quantile(0.05), "median", df[col].quantile(0.5), "95%", df[col].quantile(0.95),"99.9%",df[col].quantile(0.999) 

parser = argparse.ArgumentParser(description="analyze viewer performance files")
parser.add_argument("infilename", help="name of performance file to read", nargs = "?", default="performance.csv")
parser.add_argument("--discard_start", help="number of seconds to drop at beginning", type=float, default=10.0)
args = parser.parse_args()

df = read_input(args.infilename)
df = cleanup_data(df,args.discard_start)
analyze_timers(df)
