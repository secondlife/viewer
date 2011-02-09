#!/usr/bin/env python
"""\
@file   update_version_files.py
@brief  Update all of the various files in the repository to a new version number,
instead of having to figure it out by hand

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

import sys
import os.path

# Look for indra/lib/python in all possible parent directories ...
# This is an improvement over the setup-path.py method used previously:
#  * the script may blocated anywhere inside the source tree
#  * it doesn't depend on the current directory
#  * it doesn't depend on another file being present.

def add_indra_lib_path():
    root = os.path.realpath(__file__)
    # always insert the directory of the script in the search path
    dir = os.path.dirname(root)
    if dir not in sys.path:
        sys.path.insert(0, dir)

    # Now go look for indra/lib/python in the parent dies
    while root != os.path.sep:
        root = os.path.dirname(root)
        dir = os.path.join(root, 'indra', 'lib', 'python')
        if os.path.isdir(dir):
            if dir not in sys.path:
                sys.path.insert(0, dir)
            break
    else:
        print >>sys.stderr, "This script is not inside a valid installation."
        sys.exit(1)

add_indra_lib_path()

import getopt, os, re, commands
from indra.util import llversion

svn = os.path.expandvars("${SVN}")
if not svn or svn == "${SVN}": svn = "svn"

def usage():
    print "Usage:"
    print sys.argv[0] + """ [options]

Options:
  --version
   Specify the version string to replace current version.
  --revision
   Specify the revision to replace the last digit of the version.
   By default, revision is computed from the version control system.
  --skip-on-branch
   Specify a regular expression against which the current branch
   is matched. If it matches, then leave version strings alone.
   Use this to avoid changing version strings on release candidate
   builds.
  --server
   Update llversionserver.h only with new version
  --viewer
   Update llversionviewer.h only with new version
  --channel
   Specify the viewer channel string to replace current channel.
  --server_channel
   Specify the server channel string to replace current channel.
  --verbose
  --help
   Print this message and exit.

Common Uses:
   # Update server and viewer build numbers to the current SVN revision:
   update_version_files.py

   # Update build numbers unless we are on a release branch:
   update_version_files.py --skip-on-branch='^Branch_'

   # Update server and viewer version numbers explicitly:
   update_version_files.py --version=1.18.1.6     
                               
   # Update just the viewer version number explicitly:
   update_version_files.py --viewer --version=1.18.1.6     

   # Update just the server build number to the current SVN revision:
   update_version_files.py --server
                               
   # Update the viewer channel
   update_version_files.py --channel="First Look Puppeteering"
                               
   # Update the server channel
   update_version_files.py --server_channel="Het Grid"
   
"""
def _getstatusoutput(cmd):
    """Return Win32 (status, output) of executing cmd
in a shell."""
    if os.path.sep != "/":
        # stupid #%#$$ windows
        cmd = 'cmd.exe /c "'+cmd+'"'
    pipe = os.popen(cmd, 'r')
    text = pipe.read()
    sts = pipe.close()
    if sts is None: sts = 0
    if text[-1:] == '\n': text = text[:-1]
    return sts, text

re_map = {}

#re_map['filename'] = (('pattern', 'replacement'),
#                      ('pattern', 'replacement')
re_map['indra/llcommon/llversionviewer.h'] = \
    (('const S32 LL_VERSION_MAJOR = (\d+);',
      'const S32 LL_VERSION_MAJOR = %(VER_MAJOR)s;'),
     ('const S32 LL_VERSION_MINOR = (\d+);',
      'const S32 LL_VERSION_MINOR = %(VER_MINOR)s;'),
     ('const S32 LL_VERSION_PATCH = (\d+);',
      'const S32 LL_VERSION_PATCH = %(VER_PATCH)s;'),
     ('const S32 LL_VERSION_BUILD = (\d+);',
      'const S32 LL_VERSION_BUILD = %(VER_BUILD)s;'),
     ('const char \* const LL_CHANNEL = "(.+)";',
      'const char * const LL_CHANNEL = "%(VIEWER_CHANNEL)s";'))
re_map['indra/llcommon/llversionserver.h'] = \
    (('const S32 LL_VERSION_MAJOR = (\d+);',
      'const S32 LL_VERSION_MAJOR = %(SERVER_VER_MAJOR)s;'),
     ('const S32 LL_VERSION_MINOR = (\d+);',
      'const S32 LL_VERSION_MINOR = %(SERVER_VER_MINOR)s;'),
     ('const S32 LL_VERSION_PATCH = (\d+);',
      'const S32 LL_VERSION_PATCH = %(SERVER_VER_PATCH)s;'),
     ('const S32 LL_VERSION_BUILD = (\d+);',
      'const S32 LL_VERSION_BUILD = %(SERVER_VER_BUILD)s;'),
     ('const char \* const LL_CHANNEL = "(.+)";',
      'const char * const LL_CHANNEL = "%(SERVER_CHANNEL)s";'))
re_map['indra/newview/res/viewerRes.rc'] = \
    (('FILEVERSION [0-9,]+',
      'FILEVERSION %(VER_MAJOR)s,%(VER_MINOR)s,%(VER_PATCH)s,%(VER_BUILD)s'),
     ('PRODUCTVERSION [0-9,]+',
      'PRODUCTVERSION %(VER_MAJOR)s,%(VER_MINOR)s,%(VER_PATCH)s,%(VER_BUILD)s'),
     ('VALUE "FileVersion", "[0-9.]+"',
      'VALUE "FileVersion", "%(VER_MAJOR)s.%(VER_MINOR)s.%(VER_PATCH)s.%(VER_BUILD)s"'),
     ('VALUE "ProductVersion", "[0-9.]+"',
      'VALUE "ProductVersion", "%(VER_MAJOR)s.%(VER_MINOR)s.%(VER_PATCH)s.%(VER_BUILD)s"'))

# Trailing ',' in top level tuple is special form to avoid parsing issues with one element tuple
re_map['indra/newview/Info-SecondLife.plist'] = \
    (('<key>CFBundleVersion</key>\n\t<string>[0-9.]+</string>',
      '<key>CFBundleVersion</key>\n\t<string>%(VER_MAJOR)s.%(VER_MINOR)s.%(VER_PATCH)s.%(VER_BUILD)s</string>'),)

# This will probably only work as long as InfoPlist.strings is NOT UTF16, which is should be...
re_map['indra/newview/English.lproj/InfoPlist.strings'] = \
    (('CFBundleShortVersionString = "Second Life version [0-9.]+";',
      'CFBundleShortVersionString = "Second Life version %(VER_MAJOR)s.%(VER_MINOR)s.%(VER_PATCH)s.%(VER_BUILD)s";'),
     ('CFBundleGetInfoString = "Second Life version [0-9.]+',
      'CFBundleGetInfoString = "Second Life version %(VER_MAJOR)s.%(VER_MINOR)s.%(VER_PATCH)s.%(VER_BUILD)s'))


version_re      = re.compile('(\d+).(\d+).(\d+).(\d+)')
svn_branch_re   = re.compile('^URL:\s+\S+/([^/\s]+)$', re.MULTILINE)
svn_revision_re = re.compile('^Last Changed Rev: (\d+)$', re.MULTILINE)

def main():
    script_path = os.path.dirname(__file__)
    src_root = script_path + "/../"
    verbose = False

    opts, args = getopt.getopt(sys.argv[1:],
                               "",
                               ['version=',
                                'revision=',
                                'channel=',
                                'server_channel=',
                                'skip-on-branch=',
                                'verbose',
                                'server',
                                'viewer',
                                'help'])
    update_server = False
    update_viewer = False
    new_version = None
    new_revision = None
    new_viewer_channel = None
    new_server_channel = None
    skip_on_branch_re = None
    for o,a in opts:
        if o in ('--version'):
            new_version = a
        if o in ('--revision'):
            new_revision = a
        if o in ('--skip-on-branch'):
            skip_on_branch_re = re.compile(a)
        if o in ('--channel'):
            new_viewer_channel = a
        if o in ('--server_channel'):
            new_server_channel = a
        if o in ('--verbose'):
            verbose = True
        if o in ('--server'):
            update_server = True
        if o in ('--viewer'):
            update_viewer = True
        if o in ('--help'):
            usage()
            return 0

    if not(update_server or update_viewer):
        update_server = True
        update_viewer = True

    # Get current channel/version from llversion*.h
    try:
        viewer_channel = llversion.get_viewer_channel()
        viewer_version = llversion.get_viewer_version()
    except IOError:
        print "Viewer version file not present, skipping..."
        viewer_channel = None
        viewer_version = None
        update_viewer = False

    try:
        server_channel = llversion.get_server_channel()
        server_version = llversion.get_server_version()
    except IOError:
        print "Server version file not present, skipping..."
        server_channel = None
        server_version = None
        update_server = False

    if verbose:
        print "Source Path:", src_root
        if viewer_channel != None:
            print "Current viewer channel/version: '%(viewer_channel)s' / '%(viewer_version)s'" % locals()
        if server_channel != None:          
            print "Current server channel/version: '%(server_channel)s' / '%(server_version)s'" % locals()
        print

    # Determine new channel(s)
    if new_viewer_channel != None and len(new_viewer_channel) > 0:
        viewer_channel = new_viewer_channel
    if new_server_channel != None and len(new_server_channel) > 0:
        server_channel = new_server_channel

    # Determine new version(s)
    if new_version:
        m = version_re.match(new_version)
        if not m:
            print "Invalid version string specified!"
            return -1
        if update_viewer:
            viewer_version = new_version
        if update_server:
            server_version = new_version
    else:

        if llversion.using_svn():
            if new_revision:
                revision = new_revision
            else:
                revision = llversion.get_svn_revision()
            branch = llversion.get_svn_branch()
        elif llversion.using_hg():
            if new_revision:
                revision = new_revision
            else:
                revision = llversion.get_hg_changeset()
            branch = llversion.get_hg_repo()
        elif new_revision:
            revision = new_revision
            branch = "unknown"
        else:
            print >>sys.stderr, "ERROR: could not determine revision and branch"
            return -1
        
        if skip_on_branch_re and skip_on_branch_re.match(branch):
            print "Release Candidate Build, leaving version files untouched."
            return 0
        if update_viewer:
            m = version_re.match(viewer_version)
            viewer_version = m.group(1)+"."+m.group(2)+"."+m.group(3)+"."+revision
        if update_server:
            m = version_re.match(server_version)
            server_version = m.group(1)+"."+m.group(2)+"."+m.group(3)+"."+revision

    if verbose:
        if update_viewer:
            print "Setting viewer channel/version: '%(viewer_channel)s' / '%(viewer_version)s'" % locals()
        if update_server:
            print "Setting server channel/version: '%(server_channel)s' / '%(server_version)s'" % locals()
        print

    # split out version parts
    if viewer_version != None:
        m = version_re.match(viewer_version)
        VER_MAJOR = m.group(1)
        VER_MINOR = m.group(2)
        VER_PATCH = m.group(3)
        VER_BUILD = m.group(4)

    if server_version != None:
        m = version_re.match(server_version)
        SERVER_VER_MAJOR = m.group(1)
        SERVER_VER_MINOR = m.group(2)
        SERVER_VER_PATCH = m.group(3)
        SERVER_VER_BUILD = m.group(4)

    # For readability and symmetry with version strings:
    VIEWER_CHANNEL = viewer_channel
    SERVER_CHANNEL = server_channel

    # Iterate through all of the files in the map, and apply the
    # substitution filters
    for filename in re_map.keys():
        try:
            # Read the entire file into a string
            full_fn = src_root + '/' + filename
            file = open(full_fn,"r")
            file_str = file.read()
            file.close()

            if verbose:
                print "Processing file:",filename
            for rule in re_map[filename]:
                repl = rule[1] % locals()
                file_str = re.sub(rule[0], repl, file_str)

            file = open(full_fn,"w")
            file.write(file_str)
            file.close()
        except IOError:
            print "File %(filename)s not present, skipping..." % locals()
    return 0

main()

