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
    return pd.read_csv(infilename)

def analyze_timers(df):
    print "analyze_timers"
    timers = []
    for col_name in df.columns:
        col = df[col_name]
        if col.values.dtype == "float64":
            #print "col", col_name, "mean", col.values.mean(), \
            #      "pctile 5/50/95", col.quantile(0.05), col.quantile(0.5), col.quantile(0.95)
            timers.append(col_name)
    df = df[timers]
    tkey = "Frame - Times"
    ekey = "elapsed"
    df[ekey] = df[tkey].cumsum()
    print df.mean().sort_values(ascending=False)[0:20]
    print "Total elapsed", df[ekey].max()

    # remove early frames when UI getting initialized
    #print "rows before prune", df[tkey].count()
    #df = df[df[ekey] > 20.0]
    #print "rows after prune", df[tkey].count()

    fast_frame = df[tkey].quantile(0.05)
    slow_frame = df[tkey].quantile(0.95)
    df_fast = df[df[tkey]<=fast_frame]
    print "Top timers for fast frames:", df_fast.mean().sort_values(ascending=False)[0:30]
    df_slow = df[df[tkey]>=slow_frame]
    print "Top timers for slow frames:", df_slow.mean().sort_values(ascending=False)[0:30]

    print "Frame stats: count", len(df[tkey].values), "mean", df[tkey].mean(), "median", df[tkey].quantile(0.5), "5%", fast_frame, "95%", slow_frame
    

    
        
    
parser = argparse.ArgumentParser(description="analyze viewer performance files")
parser.add_argument("infilename", help="name of performance file to read", nargs = "?", default="performance.csv")
args = parser.parse_args()

pd_data = read_input(args.infilename)
analyze_timers(pd_data)


