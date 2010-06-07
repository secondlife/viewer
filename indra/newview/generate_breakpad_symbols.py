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
import operator
import os
import sys
import shlex
import subprocess
import tarfile
import StringIO

def usage():
    print >>sys.stderr, "usage: %s viewer_dir viewer_exes libs_suffix dump_syms_tool viewer_symbol_file" % sys.argv[0]

class MissingModuleError(Exception):
    def __init__(self, modules):
        Exception.__init__(self, "Failed to find required modules: %r" % modules)
        self.modules = modules

def main(viewer_dir, viewer_exes, libs_suffix, dump_syms_tool, viewer_symbol_file):
    print "generate_breakpad_symbols run with args: %s" % str((viewer_dir, viewer_exes, libs_suffix, dump_syms_tool, viewer_symbol_file))

    # split up list of viewer_exes
    # "'Second Life' SLPlugin" becomes ['Second Life', 'SLPlugin']
    viewer_exes = shlex.split(viewer_exes)

    found_required = dict([(module, False) for module in viewer_exes])

    def matches(f):
        if f in viewer_exes:
            found_required[f] = True
            return True
        return fnmatch.fnmatch(f, libs_suffix)

    def list_files():
        for (dirname, subdirs, filenames) in os.walk(viewer_dir):
            #print "scanning '%s' for modules..." % dirname
            for f in itertools.ifilter(matches, filenames):
                yield os.path.join(dirname, f)

    def dump_module(m):
        print "dumping module '%s' with '%s'..." % (m, dump_syms_tool)
        child = subprocess.Popen([dump_syms_tool, m] , stdout=subprocess.PIPE)
        out, err = child.communicate()
        return (m,child.returncode, out, err)

    out = tarfile.open(viewer_symbol_file, 'w:bz2')

    for (filename,status,symbols,err) in itertools.imap(dump_module, list_files()):
        if status == 0:
            module_line = symbols[:symbols.index('\n')]
            module_line = module_line.split()
            hash_id = module_line[3]
            module = ' '.join(module_line[4:])
            if sys.platform in ['win32', 'cygwin']:
                mod_name = module[:module.rindex('.pdb')]
            else:
                mod_name = module
            symbolfile = StringIO.StringIO(symbols)
            info = tarfile.TarInfo("%(module)s/%(hash_id)s/%(mod_name)s.sym" % dict(module=module, hash_id=hash_id, mod_name=mod_name))
            info.size = symbolfile.len
            out.addfile(info, symbolfile)
        else:
            print >>sys.stderr, "warning: failed to dump symbols for '%s': %s" % (filename, err)

    out.close()

    missing_modules = [m for (m,_) in
        itertools.ifilter(lambda (k,v): not v, found_required.iteritems())
    ]
    if missing_modules:
        print >> sys.stderr, "failed to generate %s" % viewer_symbol_file
        os.remove(viewer_symbol_file)
        raise MissingModuleError(missing_modules)

    symbols = tarfile.open(viewer_symbol_file, 'r:bz2')
    tarfile_members = symbols.getnames()
    def match_module_basename(m):
        return os.path.splitext(required_module)[0] == os.path.splitext(os.path.basename(m))[0]
    for required_module in viewer_exes:
        # there must be at least one .sym file in tarfile_members that matches each required module (ignoring file extensions)
        if not reduce(operator.or_, itertools.imap(match_module_basename, tarfile_members)):
            print >> sys.stderr, "failed to find required %s in generated %s" % (required_module, viewer_symbol_file)
            os.remove(viewer_symbol_file)
            raise MissingModuleError([required_module])

    print "successfully generated %s including required modules '%s'" % (viewer_symbol_file, viewer_exes)

    return 0

if __name__ == "__main__":
    if len(sys.argv) != 6:
        usage()
        sys.exit(1)
    sys.exit(main(*sys.argv[1:]))

