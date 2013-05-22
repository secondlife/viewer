#!/usr/bin/env python
"""\
@file generate_breakpad_symbols.py
@author Brad Kittenbrink <brad@lindenlab.com>
@brief Simple tool for generating google_breakpad symbol information
       for the crash reporter.

$LicenseInfo:firstyear=2010&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2010-2011, Linden Research, Inc.

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


import collections
import fnmatch
import itertools
import operator
import os
import re
import sys
import shlex
import subprocess
import tarfile
import StringIO
import pprint

DEBUG=False

def usage():
    print >>sys.stderr, "usage: %s search_dirs viewer_exes libs_suffix dump_syms_tool viewer_symbol_file" % sys.argv[0]

class MissingModuleError(Exception):
    def __init__(self, modules):
        Exception.__init__(self, "Failed to find required modules: %r" % modules)
        self.modules = modules

def main(configuration, search_dirs, viewer_exes, libs_suffix, dump_syms_tool, viewer_symbol_file):
    print "generate_breakpad_symbols run with args: %s" % str((configuration, search_dirs, viewer_exes, libs_suffix, dump_syms_tool, viewer_symbol_file))

    if not re.match("release", configuration, re.IGNORECASE):
        print "skipping breakpad symbol generation for non-release build."
        return 0

    # split up list of viewer_exes
    # "'Second Life' SLPlugin" becomes ['Second Life', 'SLPlugin']
    viewer_exes = shlex.split(viewer_exes)

    found_required = dict([(module, False) for module in viewer_exes])

    def matches(f):
        if f in viewer_exes:
            found_required[f] = True
            return True
        return fnmatch.fnmatch(f, libs_suffix)

    search_dirs = search_dirs.split(";")

    def list_files():
        for search_dir in search_dirs:
            for (dirname, subdirs, filenames) in os.walk(search_dir):
                if DEBUG:
                    print "scanning '%s' for modules..." % dirname
                for f in itertools.ifilter(matches, filenames):
                    yield os.path.join(dirname, f)

    def dump_module(m):
        print "dumping module '%s' with '%s'..." % (m, dump_syms_tool)
        dsym_full_path = m
        child = subprocess.Popen([dump_syms_tool, dsym_full_path] , stdout=subprocess.PIPE)
        out, err = child.communicate()
        return (m,child.returncode, out, err)

    
    modules = {}
        
    for m in list_files():
        if DEBUG:
            print "examining module '%s' ... " % m,
        filename=os.path.basename(m)
        if -1 != m.find("DWARF"):
            # Just use this module; it has the symbols we want.
            modules[filename] = m
            if DEBUG:
                print "found dSYM entry"
        elif filename not in modules:
            # Only use this if we don't already have a (possibly better) entry.
            modules[filename] = m
            if DEBUG:
                print "found new entry"
        elif DEBUG:
            print "ignoring entry"


    print "Found these following modules:"
    pprint.pprint( modules )

    out = tarfile.open(viewer_symbol_file, 'w:bz2')
    for (filename,status,symbols,err) in itertools.imap(dump_module, modules.values()):
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
    symbols.close()

    for required_module in viewer_exes:
        def match_module_basename(m):
            return os.path.splitext(required_module)[0].lower() \
                   == os.path.splitext(os.path.basename(m))[0].lower()
        # there must be at least one .sym file in tarfile_members that matches
        # each required module (ignoring file extensions)
        if not reduce(operator.or_, itertools.imap(match_module_basename, tarfile_members)):
            print >> sys.stderr, "failed to find required %s in generated %s" \
                    % (required_module, viewer_symbol_file)
            os.remove(viewer_symbol_file)
            raise MissingModuleError([required_module])

    print "successfully generated %s including required modules '%s'" % (viewer_symbol_file, viewer_exes)

    return 0

if __name__ == "__main__":
    if len(sys.argv) != 7:
        usage()
        sys.exit(1)
    sys.exit(main(*sys.argv[1:]))

