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

info=dict(versions={},    copyrights={})
dups=dict(versions=set(), copyrights=set())

def add_info(key, pkg, lines):
    if pkg not in info[key]:
        info[key][pkg] = '\n'.join(lines)
    else:
        dups[key].add(pkg)

versions=autobuild('install', '--versions')
copyrights=autobuild('install', '--copyrights')
viewer_copyright = copyrights.readline() # first line is the copyright for the viewer itself

# Two different autobuild outputs, but we treat them essentially the same way:
# populating each into a dict; each a subdict of 'info'.
for key, rawdata in ("versions", versions), ("copyrights", copyrights):
    lines = iter(rawdata)
    try:
        line = next(lines)
    except StopIteration:
        # rawdata is completely empty? okay...
        pass
    else:
        pkg_info = pkg_line.match(line)
        # The first line for each package must match pkg_line.
        if not pkg_info:
            sys.exit("Unrecognized --%s output: %r" % (key, line))
        # Only the very first line in rawdata MUST match; for the rest of
        # rawdata, matching the regexp is how we recognize the start of the
        # next package.
        while True:                     # iterate over packages in rawdata
            pkg = pkg_info.group(1)
            pkg_lines = [pkg_info.group(2).strip()]
            for line in lines:
                pkg_info = pkg_line.match(line)
                if pkg_info:
                    # we hit the start of the next package data
                    add_info(key, pkg, pkg_lines)
                    break
                else:
                    # no package prefix: additional line for same package
                    pkg_lines.append(line.rstrip())
            else:
                # last package in the output -- finished 'lines'
                add_info(key, pkg, pkg_lines)
                break

# Now that we've run through all of both outputs -- are there duplicates?
if any(pkgs for pkgs in dups.values()):
    for key, pkgs in dups.items():
        if pkgs:
            print >>sys.stderr, "Duplicate %s for %s" % (key, ", ".join(pkgs))
    sys.exit(1)

print "%s %s" % (args.channel, args.version)
print viewer_copyright
version = list(info['versions'].items())
version.sort()
for pkg, pkg_version in version:
    print ': '.join([pkg, pkg_version])
    try:
        print info['copyrights'][pkg]
    except KeyError:
        sys.exit("No copyright for %s" % pkg)
    print
