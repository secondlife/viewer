#!/usr/bin/python
#
# Update all of the various files in the repository to a new version number,
# instead of having to figure it out by hand
#

from os.path import realpath, dirname, join, exists
setup_path = join(dirname(realpath(__file__)), "setup-path.py")
if exists(setup_path):
    execfile(setup_path)
import getopt, sys, os, re, commands
from indra.util import llversion

def usage():
    print "Usage:"
    print sys.argv[0] + """ [options]

Options:
  --version
   Specify the version string to replace current version.
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
      'const char * const LL_CHANNEL = "%(CHANNEL)s";'))
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
re_map['indra/newview/res/newViewRes.rc'] = \
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


version_re = re.compile('(\d+).(\d+).(\d+).(\d+)')
svn_re = re.compile('Last Changed Rev: (\d+)')

def main():
    script_path = os.path.dirname(__file__)
    src_root = script_path + "/../"
    verbose = False

    opts, args = getopt.getopt(sys.argv[1:],
                               "",
                               ['version=', 'channel=', 'server_channel=', 'verbose', 'server', 'viewer', 'help'])
    update_server = False
    update_viewer = False
    version_string = None
    channel_string = None
    server_channel_string = None
    for o,a in opts:
        if o in ('--version'):
            version_string = a
        if o in ('--channel'):
            channel_string = a
        if o in ('--server_channel'):
            server_channel_string = a
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

    # Get channel from llversion*.h and update channel
    CHANNEL = llversion.get_viewer_channel()
    SERVER_CHANNEL = llversion.get_server_channel()
    if channel_string != None:
        CHANNEL = channel_string
    if server_channel_string != None:
        SERVER_CHANNEL = server_channel_string

    # Get version number from llversion*.h
    viewer_version = llversion.get_viewer_version()
    server_version = llversion.get_server_version()
    if verbose:
        print "Source Path:", src_root
        print "Current viewer version: '%(viewer_version)s'" % locals()
        print "Current server version: '%(server_version)s'" % locals()
    
    if version_string:
        m = version_re.match(version_string)
        if not m:
            print "Invalid version string specified!"
            return -1
        if update_viewer:
            viewer_version = version_string
        if update_server:
            server_version = version_string
    else:
        # Assume we're updating just the build number
        cl = 'svn info "%s"' % src_root
        status, output = _getstatusoutput(cl)
        #print
        #print "svn info output:"
        #print "----------------"
        #print output
        m = svn_re.search(output)
        if not m:
            print "Failed to execute svn info, output follows:"
            print output
            return -1
        revision = m.group(1)
        if update_viewer:
            m = version_re.match(viewer_version)
            viewer_version = m.group(1)+"."+m.group(2)+"."+m.group(3)+"."+revision
        if update_server:
            m = version_re.match(server_version)
            server_version = m.group(1)+"."+m.group(2)+"."+m.group(3)+"."+revision

    if verbose:
        print "Setting viewer version: '%(viewer_version)s'" % locals()
        print "Setting server version: '%(server_version)s'" % locals()
        print

    # split out version parts
    m = version_re.match(viewer_version)
    VER_MAJOR = m.group(1)
    VER_MINOR = m.group(2)
    VER_PATCH = m.group(3)
    VER_BUILD = m.group(4)

    m = version_re.match(server_version)
    SERVER_VER_MAJOR = m.group(1)
    SERVER_VER_MINOR = m.group(2)
    SERVER_VER_PATCH = m.group(3)
    SERVER_VER_BUILD = m.group(4)

    # Iterate through all of the files in the map, and apply the
    # substitution filters
    for filename in re_map.keys():
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
    return 0

main()

