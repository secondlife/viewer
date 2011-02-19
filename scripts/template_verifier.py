#!/usr/bin/env python
"""\
@file template_verifier.py
@brief Message template compatibility verifier.

$LicenseInfo:firstyear=2007&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2010, Linden Research, Inc.

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

"""template_verifier is a script which will compare the
current repository message template with the "master" message template, accessible
via http://secondlife.com/app/message_template/master_message_template.msg
If [FILE] is specified, it will be checked against the master template.
If [FILE] [FILE] is specified, two local files will be checked against
each other.
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

import optparse
import os
import urllib

from indra.ipc import compatibility
from indra.ipc import tokenstream
from indra.ipc import llmessage

def getstatusall(command):
    """ Like commands.getstatusoutput, but returns stdout and 
    stderr separately(to get around "killed by signal 15" getting 
    included as part of the file).  Also, works on Windows."""
    (input, out, err) = os.popen3(command, 't')
    status = input.close() # send no input to the command
    output = out.read()
    error = err.read()
    status = out.close()
    status = err.close() # the status comes from the *last* pipe that is closed
    return status, output, error

def getstatusoutput(command):
    status, output, error = getstatusall(command)
    return status, output


def die(msg):
    print >>sys.stderr, msg
    sys.exit(1)

MESSAGE_TEMPLATE = 'message_template.msg'

PRODUCTION_ACCEPTABLE = (compatibility.Same, compatibility.Newer)
DEVELOPMENT_ACCEPTABLE = (
    compatibility.Same, compatibility.Newer,
    compatibility.Older, compatibility.Mixed)

MAX_MASTER_AGE = 60 * 60 * 4   # refresh master cache every 4 hours

def retry(times, function, *args, **kwargs):
    for i in range(times):
        try:
            return function(*args, **kwargs)
        except Exception, e:
            if i == times - 1:
                raise e  # we retried all the times we could

def compare(base_parsed, current_parsed, mode):
    """Compare the current template against the base template using the given
    'mode' strictness:

    development: Allows Same, Newer, Older, and Mixed
    production: Allows only Same or Newer

    Print out information about whether the current template is compatible
    with the base template.

    Returns a tuple of (bool, Compatibility)
    Return True if they are compatible in this mode, False if not.
    """

    compat = current_parsed.compatibleWithBase(base_parsed)
    if mode == 'production':
        acceptable = PRODUCTION_ACCEPTABLE
    else:
        acceptable = DEVELOPMENT_ACCEPTABLE

    if type(compat) in acceptable:
        return True, compat
    return False, compat

def fetch(url):
    if url.startswith('file://'):
        # just open the file directly because urllib is dumb about these things
        file_name = url[len('file://'):]
        return open(file_name).read()
    else:
        # *FIX: this doesn't throw an exception for a 404, and oddly enough the sl.com 404 page actually gets parsed successfully
        return ''.join(urllib.urlopen(url).readlines())   

def cache_master(master_url):
    """Using the url for the master, updates the local cache, and returns an url to the local cache."""
    master_cache = local_master_cache_filename()
    master_cache_url = 'file://' + master_cache
    # decide whether to refresh the master cache based on its age
    import time
    if (os.path.exists(master_cache)
        and time.time() - os.path.getmtime(master_cache) < MAX_MASTER_AGE):
        return master_cache_url  # our cache is fresh
    # new master doesn't exist or isn't fresh
    print "Refreshing master cache from %s" % master_url
    def get_and_test_master():
        new_master_contents = fetch(master_url)
        llmessage.parseTemplateString(new_master_contents)
        return new_master_contents
    try:
        new_master_contents = retry(3, get_and_test_master)
    except IOError, e:
        # the refresh failed, so we should just soldier on
        print "WARNING: unable to download new master, probably due to network error.  Your message template compatibility may be suspect."
        print "Cause: %s" % e
        return master_cache_url
    try:
        tmpname = '%s.%d' % (master_cache, os.getpid())
        mc = open(tmpname, 'wb')
        mc.write(new_master_contents)
        mc.close()
        try:
            os.rename(tmpname, master_cache)
        except OSError:
            # We can't rename atomically on top of an existing file on
            # Windows.  Unlinking the existing file will fail if the
            # file is being held open by a process, but there's only
            # so much working around a lame I/O API one can take in
            # a single day.
            os.unlink(master_cache)
            os.rename(tmpname, master_cache)
    except IOError, e:
        print "WARNING: Unable to write master message template to %s, proceeding without cache." % master_cache
        print "Cause: %s" % e
        return master_url
    return master_cache_url

def local_template_filename():
    """Returns the message template's default location relative to template_verifier.py:
    ./messages/message_template.msg."""
    d = os.path.dirname(os.path.realpath(__file__))
    return os.path.join(d, 'messages', MESSAGE_TEMPLATE)

def getuser():
    try:
        # Unix-only.
        import getpass
        return getpass.getuser()
    except ImportError:
        import ctypes
        MAX_PATH = 260                  # according to a recent WinDef.h
        name = ctypes.create_unicode_buffer(MAX_PATH)
        namelen = ctypes.c_int(len(name)) # len in chars, NOT bytes
        if not ctypes.windll.advapi32.GetUserNameW(name, ctypes.byref(namelen)):
            raise ctypes.WinError()
        return name.value

def local_master_cache_filename():
    """Returns the location of the master template cache (which is in the system tempdir)
    <temp_dir>/master_message_template_cache.msg"""
    import tempfile
    d = tempfile.gettempdir()
    user = getuser()
    return os.path.join(d, 'master_message_template_cache.%s.msg' % user)


def run(sysargs):
    parser = optparse.OptionParser(
        usage="usage: %prog [FILE] [FILE]",
        description=__doc__)
    parser.add_option(
        '-m', '--mode', type='string', dest='mode',
        default='development',
        help="""[development|production] The strictness mode to use
while checking the template; see the wiki page for details about
what is allowed and disallowed by each mode:
http://wiki.secondlife.com/wiki/Template_verifier.py
""")
    parser.add_option(
        '-u', '--master_url', type='string', dest='master_url',
        default='http://secondlife.com/app/message_template/master_message_template.msg',
        help="""The url of the master message template.""")
    parser.add_option(
        '-c', '--cache_master', action='store_true', dest='cache_master',
        default=False,  help="""Set to true to attempt use local cached copy of the master template.""")

    options, args = parser.parse_args(sysargs)

    if options.mode == 'production':
        options.cache_master = False

    # both current and master supplied in positional params
    if len(args) == 2:
        master_filename, current_filename = args
        print "master:", master_filename
        print "current:", current_filename
        master_url = 'file://%s' % master_filename
        current_url = 'file://%s' % current_filename
    # only current supplied in positional param
    elif len(args) == 1:
        master_url = None
        current_filename = args[0]
        print "master:", options.master_url 
        print "current:", current_filename
        current_url = 'file://%s' % current_filename
    # nothing specified, use defaults for everything
    elif len(args) == 0:
        master_url  = None
        current_url = None
    else:
        die("Too many arguments")

    if master_url is None:
        master_url = options.master_url
        
    if current_url is None:
        current_filename = local_template_filename()
        print "master:", options.master_url
        print "current:", current_filename
        current_url = 'file://%s' % current_filename

    # retrieve the contents of the local template and check for syntax
    current = fetch(current_url)
    current_parsed = llmessage.parseTemplateString(current)

    if options.cache_master:
        # optionally return a url to a locally-cached master so we don't hit the network all the time
        master_url = cache_master(master_url)

    def parse_master_url():
        master = fetch(master_url)
        return llmessage.parseTemplateString(master)
    try:
        master_parsed = retry(3, parse_master_url)
    except (IOError, tokenstream.ParseError), e:
        if options.mode == 'production':
            raise e
        else:
            print "WARNING: problems retrieving the master from %s."  % master_url
            print "Syntax-checking the local template ONLY, no compatibility check is being run."
            print "Cause: %s\n\n" % e
            return 0
        
    acceptable, compat = compare(
        master_parsed, current_parsed, options.mode)

    def explain(header, compat):
        print header
        # indent compatibility explanation
        print '\n\t'.join(compat.explain().split('\n'))

    if acceptable:
        explain("--- PASS ---", compat)
    else:
        explain("*** FAIL ***", compat)
        return 1

if __name__ == '__main__':
    sys.exit(run(sys.argv[1:]))


