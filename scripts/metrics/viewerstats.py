#!runpy.sh

"""\

This module contains code for analyzing ViewerStats data as uploaded by the viewer.

$LicenseInfo:firstyear=2021&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2021, Linden Research, Inc.

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
import numpy as np
import pandas as pd
import json
from collections import Counter, defaultdict
from llbase import llsd

def show_stats_by_key(recs,indices,settings_sd = None):
    cnt = Counter()
    per_key_cnt = defaultdict(Counter)
    for r in recs:
        try:
            d = r
            for idx in indices:
                d = d[idx]
            for k,v in d.items():
                if isinstance(v,dict):
                    continue
                cnt[k] += 1
                if isinstance(v,list):
                    v = tuple(v)
                per_key_cnt[k][v] += 1
        except Exception as e:
            print "err", e
            print "d", d, "k", k, "v", v
            raise
    mc = cnt.most_common()
    print "========================="
    keyprefix = ""
    if len(indices)>0:
        keyprefix = ".".join(indices) + "."
    for i,m in enumerate(mc):
        k = m[0]
        bigc = m[1]
        unset_cnt = len(recs) - bigc
        kmc = per_key_cnt[k].most_common(5)
        print i, keyprefix+str(k), bigc
        if settings_sd is not None and k in settings_sd and "Value" in settings_sd[k]:
            print "    ", "default",settings_sd[k]["Value"],"count",unset_cnt
        for v in kmc:
            print "    ", "value",v[0],"count",v[1]
    if settings_sd is not None:
        print "Total keys in settings", len(settings_sd.keys())
        unused_keys = list(set(settings_sd.keys()) - set(cnt.keys()))
        unused_keys_non_str = [k for k in unused_keys if settings_sd[k]["Type"] != "String"]
        unused_keys_str = [k for k in unused_keys if settings_sd[k]["Type"] == "String"]

        # Things that no one in the sample has set to a non-default value. Possible candidates for removal.
        print "\nUnused_keys_non_str", len(unused_keys_non_str)
        print   "======================"
        print "\n".join(sorted(unused_keys_non_str))

        # Strings are not currently logged, so we have no info on usage.
        print "\nString keys (usage unknown)", len(unused_keys_str)
        print   "======================"
        print "\n".join(sorted(unused_keys_str))

        # Things that someone has set but that aren't recognized settings.
        unrec_keys = list(set(cnt.keys()) - set(settings_sd.keys()))
        print "\nUnrecognized keys", len(unrec_keys)
        print   "======================"
        print "\n".join(sorted(unrec_keys))

def parse_settings_xml(fname):
    # assume we're in scripts/metrics
    fname = "../../indra/newview/app_settings/" + fname
    with open(fname,"r") as f:
        contents = f.read()
        return llsd.parse_xml(contents)

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="process tab-separated table containing viewerstats logs")
    parser.add_argument("--verbose", action="store_true",help="verbose flag")
    parser.add_argument("--preferences", action="store_true", help="analyze preference info")
    parser.add_argument("--column", help="name of column containing viewerstats info")
    parser.add_argument("infiles", nargs="+", help="name of .tsv files to process")
    args = parser.parse_args()

    for fname in args.infiles:
        print "process", fname
        df = pd.read_csv(fname,sep='\t')
        #print "DF", df.describe()
        jstrs = df['RAW_LOG:BODY']
        #print "JSTRS", jstrs.describe()
        recs = []
        for i,jstr in enumerate(jstrs):
            recs.append(json.loads(jstr))
        show_stats_by_key(recs,[])
        show_stats_by_key(recs,["agent"])
        if args.preferences:
            print "\nSETTINGS.XML"
            settings_sd = parse_settings_xml("settings.xml")
            #for skey,svals in settings_sd.items(): 
            #    print skey, "=>", svals
            show_stats_by_key(recs,["preferences","settings"],settings_sd)
            print

            print "\nSETTINGS_PER_ACCOUNT.XML"
            settings_pa_sd = parse_settings_xml("settings_per_account.xml")
            show_stats_by_key(recs,["preferences","settings_per_account"],settings_pa_sd)

        
    
