#!/usr/bin/env python
"""\

This script scans the SL codebase for translation-related strings.

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

def codify_for_print(val):
    if isinstance(val, unicode):
        return val.encode("utf-8")
    else:
        return unicode(val, 'utf-8').encode("utf-8")

# Returns a dict of { name => xml_node }
def read_xml_elements(blob):
    try:
        contents = blob.data_stream.read()
    except:
        # default - pretend we read a file with no elements of interest.
        # Parser will complain if it gets no elements at all.
        contents = '<?xml version="1.0" encoding="utf-8" standalone="yes" ?><strings></strings>'
    xml = ET.fromstring(contents)
    elts = {}
    for child in xml.iter():
        if "name" in child.attrib:
            name = child.attrib['name']
            elts[name] = child
    return elts

def failure(*msg):
    print(*msg)
    sys.exit(1)

def can_translate(val):
    if val is None:
        return False
    if val.isspace():
        return False
    val = val.strip()
    if val.isdigit():
        return False
    return True

usage_msg="""%(prog)s [options]

Analyze the XUI configuration files to find text that may need to
be translated. Works by comparing two specified revisions, one
specified by --rev (default HEAD) and one specified by --rev_base
(default master). The script works by comparing xui contents of the
two revisions, and outputs a spreadsheet listing any areas of
difference. The target language must be specified using the --lang
option. Output is an excel file, which can be used as-is or imported
into google sheets.

If the --rev revision already contains a translation for the text, it
will be included in the spreadsheet for reference.
    
Normally you would want --rev_base to be the last revision to have
translations added, and --rev to be the tip of the current
project.

"""

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="analyze viewer xui files for needed translations", usage=usage_msg)
    parser.add_argument("-v","--verbose", action="store_true", help="verbose flag")
    parser.add_argument("--rev", help="revision with modified strings, default HEAD", default="HEAD")
    parser.add_argument("--rev_base", help="previous revision to compare against, default master", default="master")
    parser.add_argument("--base_lang", help="base language, default en (useful only for testing)", default="en")
    parser.add_argument("--lang", help="target language")
    args = parser.parse_args()

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

    xui_base = "indra/newview/skins/default/xui"
    xui_base_tree = mod_tree[xui_base]
    valid_langs = [tree.name.lower() for tree in xui_base_tree if tree.name.lower() != args.base_lang.lower()]
    if not args.lang or not args.lang.lower() in valid_langs:
        failure("Please specify a target language using --lang. Valid values are", ",".join(sorted(valid_langs)))

    xui_path = "{}/{}".format(xui_base, args.base_lang)
    try:
        mod_xui_tree = mod_tree[xui_path]
    except:
        failure("xui tree not found for base language", args.base_lang)

    if args.rev == args.rev_base:
        failure("Revs are the same, nothing to compare")

    print("Finding changes in", args.rev, "not present in", args.rev_base)
    sys.stdout.flush()

    data = []
    # For all files to be checked for translations
    for mod_blob in mod_xui_tree.traverse():
        filename = mod_blob.path
        if mod_blob.type == "tree": # directory, skip
            continue

        if args.verbose:
            print(filename)

        try:
            base_blob = base_tree[filename]
        except:
            if args.verbose:
                print("No matching base file found for", filename)
            base_blob = None

        try:
            transl_filename = filename.replace("/xui/{}/".format(args.base_lang), "/xui/{}/".format(args.lang))
            transl_blob = mod_tree[transl_filename]
        except:
            if args.verbose:
                failure("No matching translation file found at", transl_filename)
            transl_blob = None

        mod_dict = read_xml_elements(mod_blob)
        base_dict = read_xml_elements(base_blob)
        transl_dict = read_xml_elements(transl_blob)

        rows = 0
        for name in mod_dict.keys():
            if not name in base_dict or mod_dict[name].text != base_dict[name].text:
                val = mod_dict[name].text
                if can_translate(val):
                    transl_val = "--"
                    if name in transl_dict:
                        transl_val = transl_dict[name].text
                    data.append([filename, name, "text", val, transl_val, ""])
                    rows += 1
            for attr in translate_attribs:
                if attr in mod_dict[name].attrib:
                    if name not in base_dict or attr not in base_dict[name].attrib or mod_dict[name].attrib[attr] != base_dict[name].attrib[attr]:
                        val = mod_dict[name].attrib[attr]
                        if can_translate(val):
                            transl_val = "--"
                            if name in transl_dict and attr in transl_dict[name].attrib:
                                transl_val = transl_dict[name].attrib[attr]
                            data.append([filename, name, attr, val, transl_val, ""])
                            rows += 1
        if args.verbose and rows>0:
            print("    ",rows,"rows added")

    outfile = "SL_Translations_{}.xlsx".format(args.lang.upper())
    cols = ["File", "Element", "Field", "EN", "Previous Translation ({})".format(args.lang.upper()), "ENTER NEW TRANSLATION ({})".format(args.lang.upper())]
    num_translations = len(data)
    df = pd.DataFrame(data, columns=cols)
    df.to_excel(outfile, index=False)
    if num_translations>0:
        print("Wrote", num_translations, "rows to file", outfile)
    else:
        print("Nothing to translate,", outfile, "is empty")
