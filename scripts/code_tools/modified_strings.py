"""\

This module contains tools for scanning the SL codebase for translation-related strings.

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

from __future__ import print_function

# packages required include: gitpython, pandas

import xml.etree.ElementTree as ET
import argparse
import os
import sys
from git import Repo, Git # requires the gitpython package
import pandas as pd

translate_attribs = [
    "title",
    "short_title",
    "value",
    "label",
    "label_selected",
    "tool_tip",
    "ignoretext",
    "yestext",
    "notext",
    "canceltext",
    "description",
    "longdescription"
]

def codify(val):
    if isinstance(val, unicode):
        return val.encode("utf-8")
    else:
        return unicode(val, 'utf-8').encode("utf-8")

def failure(*msg):
    print(*msg)
    sys.exit(1)
    
if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="analyze viewer xui files")
    parser.add_argument("--verbose", action="store_true", help="verbose flag")
    parser.add_argument("--rev", help="revision with modified strings, default HEAD", default="HEAD")
    parser.add_argument("--rev_base", help="previous revision to compare against, default master", default="master")
    parser.add_argument("--base_lang", help="base language, default en (useful only for testing)", default="en")
    parser.add_argument("--lang", help="target language, default fr", default="fr")
    #parser.add_argument("infilename", help="name of input file", nargs="?")
    args = parser.parse_args()

    #root = ET.parse(args.infilename)

    #for child in root.iter("string"):
    #    print child.attrib["name"], "\t", unicode(child.text, 'utf-8').encode("utf-8")
    #    #print unicode(child.text, 'utf-8')
    #    #print u'\u0420\u043e\u0441\u0441\u0438\u044f'.encode("utf-8")

    if args.rev == args.rev_base:
        failure("Revs are the same, nothing to compare")

    print("Finding changes in", args.rev, "not present in", args.rev_base)

    cwd = os.getcwd()
    rootdir = Git(cwd).rev_parse("--show-toplevel")
    repo = Repo(rootdir)
    try:
        mod_commit = repo.commit(args.rev)
    except:
        failure(args.rev,"is not a valid commit")
    try:
        base_commit = repo.commit(args.rev_base)
    except:
        failure(args.rev_base,"is not a valid commit")

    mod_tree = mod_commit.tree
    base_tree = base_commit.tree

    all_attrib = set()

    try:
        mod_xui_tree = mod_tree["indra/newview/skins/default/xui/{}".format(args.base_lang)]
    except:
        print("xui tree not found for language", args.base_lang)
        sys.exit(1)

    data = []
    # For all files to be checked for translations
    for mod_blob in mod_xui_tree.traverse():
        print(mod_blob.path)
        filename = mod_blob.path
        if mod_blob.type == "tree": # directory, skip
            continue

        mod_contents = mod_blob.data_stream.read()
        try:
            base_blob = base_tree[filename]
            base_contents = base_blob.data_stream.read()
        except:
            print("No matching base file found for", filename)
            base_contents = '<?xml version="1.0" encoding="utf-8" standalone="yes" ?><strings></strings>'

        mod_xml = ET.fromstring(mod_contents)
        base_xml = ET.fromstring(base_contents)
        
        mod_dict = {}
        for child in mod_xml.iter():
            if "name" in child.attrib:
                name = child.attrib['name']
                mod_dict[name] = child
        base_dict = {}
        for child in base_xml.iter():
            if "name" in child.attrib:
                name = child.attrib['name']
                base_dict[name] = child
        for name in mod_dict.keys():
            if not name in base_dict or mod_dict[name].text != base_dict[name].text:
                data.append([filename, name, "text", mod_dict[name].text,""])
                #print("   ", name, "text", codify(mod_dict[name].text))
            all_attrib = all_attrib.union(set(mod_dict[name].attrib.keys()))
            for attr in translate_attribs:
                if attr in mod_dict[name].attrib:
                    if name not in base_dict or attr not in base_dict[name] or mod_dict[name].attrib[attr] != base_dict[name].attrib[attr]:
                        val = mod_dict[name].attrib[attr]
                        data.append([filename, name, attr, mod_dict[name].attrib[attr],""])
                        #print("   ", name, attr, codify(val))

    cols = ["File", "Element", "Field", "EN", "Translation ({})".format(args.lang)]
    df = pd.DataFrame(data, columns=cols)
    df.to_excel("SL_Translations_{}.xlsx".format(args.lang.upper()), index=False)

    #print "all_attrib", all_attrib
                            
            
        
    
