#!runpy.sh

"""\

This module contains tools for analyzing viewer asset metrics logs produced by the viewer.

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
from llbase import llsd

def get_metrics_record(infiles):
    for filename in args.infiles:
        f = open(filename)
        # get an iterable
        context = etree.iterparse(f, events=("start", "end"))

        # turn it into an iterator
        context = iter(context)

        # get the root element
        event, root = context.next()
        try:
            for event, elem in context:
                if event == "end" and elem.tag == "llsd":
                    xmlstr = etree.tostring(elem, encoding="utf8", method="xml")
                    sd = llsd.parse_xml(xmlstr)
                    yield sd
        except etree.XMLSyntaxError:
            print "Fell off end of document"

        f.close()

def update_stats(stats,rec):
    for region in rec["regions"]:
        region_key = (region["grid_x"],region["grid_y"])
        #print "region",region_key
        for field, val in region.iteritems():
            if field in ["duration","grid_x","grid_y"]:
                continue
            if field == "fps":
                # handle fps record as special case
                pass
            else:
                #print "field",field 
                stats.setdefault(field,{})
                type_stats = stats.get(field)
                newcount = val["resp_count"]
                #print "field",field,"add count",newcount
                type_stats["count"] = type_stats.get("count",0) + val["resp_count"]
                #print "field",field,"count",type_stats["count"]
                if (newcount>0):
                    type_stats["sum"] = type_stats.get("sum",0) + val["resp_count"] * val["resp_mean"]
                    type_stats["sum_bytes"] = type_stats.get("sum_bytes",0) + val["resp_count"] * val.get("resp_mean_bytes",0)
                type_stats["enqueued"] = type_stats.get("enqueued",0) + val["enqueued"]
                type_stats["dequeued"] = type_stats.get("dequeued",0) + val["dequeued"]
                
            
    
if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="process metric xml files for viewer asset fetching")
    parser.add_argument("--verbose", action="store_true",help="verbose flag")
    parser.add_argument("infiles", nargs="+", help="name of .xml files to process")
    args = parser.parse_args()

    #print "process files:",args.infiles

    stats = {}
    for rec in get_metrics_record(args.infiles):
        #print "record",rec

        update_stats(stats,rec)

    for key in sorted(stats.keys()):
        val = stats[key]
        if val["count"] > 0:
            print key,"count",val["count"],"mean_time",val["sum"]/val["count"],"mean_bytes",val["sum_bytes"]/val["count"],"net bytes/sec",val["sum_bytes"]/val["sum"],"enqueued",val["enqueued"],"dequeued",val["dequeued"]
        else:
            print key,"count",val["count"],"enqueued",val["enqueued"],"dequeued",val["dequeued"]

