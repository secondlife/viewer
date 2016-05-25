#!runpy.sh

"""\

This module contains tools for comparing files output by LLVOAvatar::dumpArchetypeXML

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
from itertools import chain

def node_key(e):
    if e.tag == "param":
        return e.tag + " " + e.get("id")
    if e.tag == "texture":
        return e.tag + " " + e.get("te")
    if e.get("name"):
        return e.tag + " " + e.get("name")
    return None

def compare_matched_nodes(key,items,summary):
    tags = list(set([e.tag for e in items]))
    if len(tags) != 1:
        print "different tag types for key",key
        summary.setdefault("tag_mismatch",0)
        summary["tag_mismatch"] += 1
        return
    all_attrib = list(set(chain.from_iterable([e.attrib.keys() for e in items])))
    #print key,"all_attrib",all_attrib
    for attr in all_attrib:
        vals = [e.get(attr) for e in items]
        #print "key",key,"attr",attr,"vals",vals
        if len(set(vals)) != 1:
            print key,"- attr",attr,"multiple values",vals
            summary.setdefault("attr",{})
            summary["attr"].setdefault(attr,0)
            summary["attr"][attr] += 1

def compare_trees(file_trees):
    print "compare_trees"
    summary = {}
    all_keys = list(set([node_key(e) for tree in file_trees for e in tree.getroot().iter() if node_key(e)]))
    #print "keys",all_keys
    tree_nodes = []
    for i,tree in enumerate(file_trees):
        nodes = dict((node_key(e),e) for e in tree.getroot().iter() if node_key(e))
        tree_nodes.append(nodes)
    for key in sorted(all_keys):
        items = []
        for nodes in tree_nodes:
            if not key in nodes:
                print "file",i,"missing item for key",key
                summary.setdefault("missing",0)
                summary["missing"] += 1
            else:
                items.append(nodes[key])
        compare_matched_nodes(key,items,summary)
    print "Summary:"
    print summary
                

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="compare avatar XML archetype files")
    parser.add_argument("--verbose", help="verbose flag", action="store_true")
    parser.add_argument("files", nargs="+", help="name of one or more archtype files")
    args = parser.parse_args()


    print "files",args.files
    file_trees = [etree.parse(filename) for filename in args.files]
    compare_trees(file_trees)
