#!/usr/bin/env python
# @file generate_breakpad_symbols.py
# @author Brad Kittenbrink <brad@lindenlab.com>
# @brief Simple tool for generating google_breakpad symbol information
#        for the crash reporter.
#
# $LicenseInfo:firstyear=2010&license=viewergpl$
# 
# Copyright (c) 2010-2010, Linden Research, Inc.
# 
# Second Life Viewer Source Code
# The source code in this file ("Source Code") is provided by Linden Lab
# to you under the terms of the GNU General Public License, version 2.0
# ("GPL"), unless you have obtained a separate licensing agreement
# ("Other License"), formally executed by you and Linden Lab.  Terms of
# the GPL can be found in doc/GPL-license.txt in this distribution, or
# online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
# 
# There are special exceptions to the terms and conditions of the GPL as
# it is applied to this Source Code. View the full text of the exception
# in the file doc/FLOSS-exception.txt in this software distribution, or
# online at
# http://secondlifegrid.net/programs/open_source/licensing/flossexception
# 
# By copying, modifying or distributing this software, you acknowledge
# that you have read and understood your obligations described above,
# and agree to abide by those obligations.
# 
# ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
# WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
# COMPLETENESS OR PERFORMANCE.
# $/LicenseInfo$


import collections
import fnmatch
import itertools
import os
import sys
import subprocess
import bz2

def usage():
    print >>sys.stderr, "usage: %s viewer_dir viewer_exes libs_suffix dump_syms_tool viewer_symbol_file" % sys.argv[0]

def main(viewer_dir, viewer_exes, libs_suffix, dump_syms_tool, viewer_symbol_file):
    print "generate_breakpad_symbols: %s" % str((viewer_dir, viewer_exes, libs_suffix, dump_syms_tool, viewer_symbol_file))

    def matches(f):
        return f in viewer_exes or fnmatch.fnmatch(f, libs_suffix)

    def list_files():
        for (dirname, subdirs, filenames) in os.walk(viewer_dir):
            #print "scanning '%s' for modules..." % dirname
            for f in itertools.ifilter(matches, filenames):
                yield os.path.join(dirname, f)

    def dump_module(m):
        print "dumping module '%s' with '%s'..." % (m, dump_syms_tool)
        child = subprocess.Popen([dump_syms_tool, m] , stdout=subprocess.PIPE)
        out, err = child.communicate()
        return out

    out = bz2.BZ2File(viewer_symbol_file, 'w')

    for symbols in map(dump_module, list_files()):
        out.writelines(symbols)

    out.close()

    return 0

if __name__ == "__main__":
    if len(sys.argv) != 6:
        usage()
        sys.exit(1)
    sys.exit(main(*sys.argv[1:]))

