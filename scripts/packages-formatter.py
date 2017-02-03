#!/usr/bin/env python
"""\
This module formats the package version and copyright information for the
viewer and its dependent packages.

$LicenseInfo:firstyear=2014&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2014, Linden Research, Inc.

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
import os
import sys
import errno
import re
import subprocess
import argparse

parser = argparse.ArgumentParser(description='Format dependency version and copyright information for the viewer About box content')
parser.add_argument('channel', help='viewer channel name')
parser.add_argument('version', help='viewer version number')
args = parser.parse_args()

_autobuild=os.getenv('AUTOBUILD', 'autobuild')

pkg_line=re.compile('^([\w-]+):\s+(.*)$')

def autobuild(*args):
    """
    Launch autobuild with specified command-line arguments.
    Return its stdout pipe from which the caller can read.
    """
    # subprocess wants a list, not a tuple
    command = [_autobuild] + list(args)
    try:
        child = subprocess.Popen(command,
                                 stdin=None, stdout=subprocess.PIPE,
                                 universal_newlines=True)
    except OSError as err:
        if err.errno != errno.ENOENT:
            # Don't attempt to interpret anything but ENOENT
            raise
        # Here it's ENOENT: subprocess can't find the autobuild executable.
        sys.exit("packages-formatter on %s: can't run autobuild:\n%s\n%s" % \
                 (sys.platform, ' '.join(command), err))

    # no exceptions yet, let caller read stdout
    return child.stdout

version={}
versions=autobuild('install', '--versions')
for line in versions:
    pkg_info = pkg_line.match(line)
    if pkg_info:
        pkg = pkg_info.group(1)
        if pkg not in version:
            version[pkg] = pkg_info.group(2).strip()
        else:
            sys.exit("Duplicate version for %s" % pkg)
    else:
        sys.exit("Unrecognized --versions output: %s" % line)

copyright={}
copyrights=autobuild('install', '--copyrights')
viewer_copyright = copyrights.readline() # first line is the copyright for the viewer itself
for line in copyrights:
    pkg_info = pkg_line.match(line)
    if pkg_info:
        pkg = pkg_info.group(1)
        if pkg not in copyright:
            copyright[pkg] = pkg_info.group(2).strip()
        else:
            sys.exit("Duplicate copyright for %s" % pkg)
    else:
        sys.exit("Unrecognized --copyrights output: %s" % line)

print "%s %s" % (args.channel, args.version)
print viewer_copyright
for pkg in sorted(version):
    print ': '.join([pkg, version[pkg]])
    if pkg in copyright:
        print copyright[pkg]
    else:
        sys.exit("No copyright for %s" % pkg)
    print ''
