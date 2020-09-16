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
import re
from datetime import datetime

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
project. You can find the last commit with translation work using "git log --grep INTL- | head"

The --missing argument can be used to find all text with missing
translations, regardless of when it was added. If translations are being kept
reasonably current, you will normally not need this argument.
"""

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

# return True iff any element of lis is "in" thing
def has_any(thing,lis):
    for l in lis:
        if l in thing:
            return True
    return False

def should_translate(filename, elt, field, val):
    if val is None:
        return False
    # Should translate apply recursively?
    if "translate" in elt.attrib and elt.attrib["translate"] == "false":
        return False
    if has_any(filename,["floater_test","floater_aaa","floater_ui_preview"]):
        return False
    if "TestString PleaseIgnore" in val:
        return False
    val = re.sub(r"\[.*?\]","",val)
    if len(val) == 0:
        return False
    if val.isspace():
        return False
    val = val.strip()
    if val.isdigit():
        return False
    if not re.search('\w+', val):
        return False
    if re.match(r"^\s*\d*\s*x\s*\d*\s*$", val):
        #print(val, "matches resolution string, will ignore")
        return False
    # "value" attribute is a hairball, mostly used to encode non-display info but a few exceptions
    if field == "value":
        if elt.text is not None and len(elt.text) > 0:
            #print("value has text, ignoring", ET.tostring(elt))
            return False
        if has_any(elt.attrib,["label"]):
            return False
        if elt.tag in ["string","text"]:
            return True
        #print("including value attribute", val, "tag", elt.tag,"in", ET.tostring(elt))
        return True
    return True

def make_translation_table(mod_tree, base_tree, lang, args):

    xui_path = "{}/{}".format(xui_base, args.base_lang)
    try:
        mod_xui_tree = mod_tree[xui_path]
    except:
        failure("xui tree not found for base language", args.base_lang)

    if args.rev == args.rev_base:
        failure("Revs are the same, nothing to compare")


    data = []
    # For all files to be checked for translations
    all_en_strings = set()
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
            transl_filename = filename.replace("/xui/{}/".format(args.base_lang), "/xui/{}/".format(lang))
            transl_blob = mod_tree[transl_filename]
        except:
            if args.verbose:
                print("No matching translation file found at", transl_filename)
            transl_blob = None

        mod_dict = read_xml_elements(mod_blob)
        base_dict = read_xml_elements(base_blob)
        transl_dict = read_xml_elements(transl_blob)

        rows = 0
        for name in mod_dict.keys():
            if not name in base_dict or mod_dict[name].text != base_dict[name].text or (args.missing and not name in transl_dict):
                elt = mod_dict[name]
                val = elt.text
                field = "text"
                if should_translate(filename, elt, field, val):
                    transl_val = "--"
                    if name in transl_dict:
                        transl_val = transl_dict[name].text
                    if val in all_en_strings:
                        new_val = "(DUPLICATE)"
                    else:
                        new_val = ""
                    data.append([val, transl_val, new_val, "", "", filename, name, field])
                    all_en_strings.add(val)
                    rows += 1
            for attr in translate_attribs:
                if attr in mod_dict[name].attrib:
                    if name not in base_dict \
                    or attr not in base_dict[name].attrib \
                    or mod_dict[name].attrib[attr] != base_dict[name].attrib[attr] \
                    or (args.missing and (not name in transl_dict or not attr in transl_dict[name].attrib)):
                        elt = mod_dict[name]
                        val = elt.attrib[attr]
                        if should_translate(filename, elt, attr, val):
                            transl_val = "--"
                            if name in transl_dict and attr in transl_dict[name].attrib:
                                transl_val = transl_dict[name].attrib[attr]
                            if val in all_en_strings:
                                new_val = "(DUPLICATE)"
                            else:
                                new_val = ""
                            #attr = attr + ":" + ET.tostring(elt)
                            data.append([val, transl_val, new_val, "", "", filename, name, attr])
                            all_en_strings.add(val)
                            rows += 1
    return data

def save_translation_file(per_lang_data, aux_data, outfile):

    langs = sorted(per_lang_data.keys())
    print("Saving languages", ",".join(langs),"as",outfile)

    writer = pd.ExcelWriter(outfile, engine='xlsxwriter')

    workbook = writer.book
    wrap_format = workbook.add_format({'text_wrap': True})
    bold_wrap_format = workbook.add_format({'text_wrap': True, 'bold': True})
    wrap_unlocked_format = workbook.add_format({'text_wrap': True, 'locked': False})

    for lang in langs:
        data = per_lang_data[lang]
        num_translations = len(data)
        cols = ["EN", "Previous Translation ({})".format(lang.upper()), "ENTER NEW TRANSLATION ({})".format(lang.upper()), "Translator Questions", "Notes", "File", "Element", "Field"]
        df = pd.DataFrame(data, columns=cols)
        df.to_excel(writer, index=False, sheet_name = lang.upper())

        worksheet = writer.sheets[lang.upper()]

        # Translators primarily care about columns A-C, and should write
        # only in column C. Hide the others. Set widths.
        worksheet.protect()
        worksheet.set_column('A:B', 60, wrap_format)
        worksheet.set_column('C:C', 60, wrap_unlocked_format)
        worksheet.set_column('D:E', 40, wrap_unlocked_format)
        worksheet.set_column('F:F', 50, wrap_format, {'hidden': True})
        worksheet.set_column('G:H', 30, wrap_format, {'hidden': True})

        # Lock the top row (column headers) in place while scrolling
        worksheet.freeze_panes(1, 0)
        print("Added", num_translations, "rows for language", lang)

    # Reference info, not for translation
    for aux, data in aux_data.items():
        df = pd.DataFrame(data, columns = ["Key", "Value"]) 
        df.to_excel(writer, index=False, sheet_name=aux)
        worksheet = writer.sheets[aux]
        worksheet.set_column('A:A', 50, bold_wrap_format)
        worksheet.set_column('B:B', 80, wrap_format)
        
    print("Writing", outfile)
    writer.save()

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="analyze viewer xui files for needed translations", usage=usage_msg)
    parser.add_argument("-v","--verbose", action="store_true", help="verbose flag")
    parser.add_argument("--missing", action="store_true", default = False, help="include all fields for which a translation does not exist")
    parser.add_argument("--rev", help="revision with modified strings, default HEAD", default="HEAD")
    parser.add_argument("--rev_base", help="previous revision to compare against, default master", default="master")
    parser.add_argument("--base_lang", help="base language, default en (normally leave unchanged - other values are only useful for testing)", default="en")
    parser.add_argument("--lang", help="target languages, or all", nargs="+", default = ["all"])
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

    print("Will identify changes in", args.rev, "not present in", args.rev_base)
    if args.missing:
        print("Will also include any text for which no corresponding translation exists, regardless of when it was added")
    sys.stdout.flush()

    mod_tree = mod_commit.tree
    base_tree = base_commit.tree

    xui_base = "indra/newview/skins/default/xui"
    xui_base_tree = mod_tree[xui_base]

    # Find target languages
    valid_langs = [tree.name.lower() for tree in xui_base_tree if tree.name.lower() != args.base_lang.lower()]
    supported_langs = ["fr", "es", "it", "pt", "ja", "de"]
    langs = [l.lower() for l in args.lang]
    if "all" in args.lang:
        langs = supported_langs
    langs = sorted(langs)
    for lang in langs:
          if not lang in valid_langs:
              failure("Unknown target language {}. Valid values are {} or all".format(lang,",".join(sorted(valid_langs))))
    print("Target language(s) are", ",".join(sorted(langs)))
    sys.stdout.flush()

    outfile = "SL_Translations.xlsx"
    try:
        f = open(outfile,"a+")
        f.close()
    except:
        failure("Can't write to output file",outfile,". Is it already open?")

    aux_data = { "REFERENCE": [["Command", " ".join(sys.argv)],
                               ["Date", str(datetime.now())],
                               ["Mod Commit", mod_commit.hexsha],
                               ["Base Commit", base_commit.hexsha],
                              ] }
    per_lang_data = {}
    for lang in langs:
        print("Creating spreadsheet for language", lang)
        sys.stdout.flush()
    
        per_lang_data[lang] = make_translation_table(mod_tree, base_tree, lang, args)

    print("Saving output file", outfile)
    save_translation_file(per_lang_data, aux_data, outfile)

