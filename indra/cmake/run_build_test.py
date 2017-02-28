#!/usr/bin/env python
"""\
@file   run_build_test.py
@author Nat Goodspeed
@date   2009-09-03
@brief  Helper script to allow CMake to run some command after setting
        environment variables.

CMake has commands to run an external program. But remember that each CMake
command must be backed by multiple build-system implementations. Unfortunately
it seems CMake can't promise that every target build system can set specified
environment variables before running the external program of interest.

This helper script is a workaround. It simply sets the requested environment
variables and then executes the program specified on the rest of its command
line.

Example:

python run_build_test.py -DFOO=bar myprog somearg otherarg

sets environment variable FOO=bar, then runs:
myprog somearg otherarg

$LicenseInfo:firstyear=2009&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2009-2010, Linden Research, Inc.

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
import HTMLParser
import re
import signal
import subprocess
import logging

def main(command, arguments=[], libpath=[], vars={}):
    """Pass:
    command is the command to be executed

    argument is a sequence (e.g. a list) of strings to be passed to command

    libpath is a sequence of directory pathnames. These will be appended to
    the platform-specific dynamic library search path environment variable.

    vars is a dict of arbitrary (var, value) pairs to be added to the
    environment before running 'command'.

    This function runs the specified command, waits for it to terminate and
    returns its return code. This will be negative if the command terminated
    with a signal, else it will be the process's specified exit code.
    """
    # Handle platform-dependent libpath first.
    if sys.platform == "win32":
        lpvars = ["PATH"]
    elif sys.platform == "darwin":
        lpvars = ["LD_LIBRARY_PATH", "DYLD_LIBRARY_PATH"]
    elif sys.platform.startswith("linux"):
        lpvars = ["LD_LIBRARY_PATH"]
    else:
        # No idea what the right pathname might be! But only crump if this
        # feature is requested.
        if libpath:
            raise RuntimeError("run_build_test: unknown platform %s" % sys.platform)
        lpvars = []
    for var in lpvars:
        # Split the existing path. Bear in mind that the variable in question
        # might not exist; instead of KeyError, just use an empty string.
        dirs = os.environ.get(var, "").split(os.pathsep)
        # Append the sequence in libpath
        log.info("%s += %r" % (var, libpath))
        for dir in libpath:
            # append system paths at the end
            if dir in ('/lib', '/usr/lib'):
                dirs.append(dir)
            # prepend non-system paths
            else:
                dirs.insert(0, dir)

        # Filter out some useless pieces
        clean_dirs = []
        for dir in dirs:
            if dir and dir not in ('', '.'):
                clean_dirs.append(dir)

        # Now rebuild the path string. This way we use a minimum of separators
        # -- and we avoid adding a pointless separator when libpath is empty.
        os.environ[var] = os.pathsep.join(clean_dirs)
        log.info("%s = %r" % (var, os.environ[var]))
    # Now handle arbitrary environment variables. The tricky part is ensuring
    # that all the keys and values we try to pass are actually strings.
    if vars:
         log.info("Setting: %s" % ("\n".join(["%s=%s" % (key, value) for key, value in vars.iteritems()])))
    os.environ.update(dict([(str(key), str(value)) for key, value in vars.iteritems()]))
    # Run the child process.
    command_list = [command]
    command_list.extend(arguments)
    log.info("Running: %s" % " ".join(command_list))
    # Make sure we see all relevant output *before* child-process output.
    sys.stdout.flush()
    try:
        return subprocess.call(command_list)
    except OSError as err:
        # If the caller is trying to execute a test program that doesn't
        # exist, we want to produce a reasonable error message rather than a
        # traceback. This happens when the build is halted by errors, but
        # CMake tries to proceed with testing anyway <eyeroll/>. However, do
        # NOT attempt to handle any error but "doesn't exist."
        if err.errno != errno.ENOENT:
            raise
        # In practice, the pathnames into CMake's build tree are so long as to
        # obscure the name of the test program. Just log its basename.
        log.warn("No such program %s; check for preceding build errors" % \
                 os.path.basename(command[0]))
        # What rc should we simulate for missing executable? Windows produces
        # 9009.
        return 9009

# swiped from vita, sigh, seems like a Bad Idea to introduce dependency
def translate_rc(rc):
    """
    Accept an rc encoded as for subprocess.Popen.returncode:
    None means still running
    int >= 0 means terminated voluntarily with specified rc
    int <  0 means terminated by signal (-rc)

    Return a string explaining the outcome. In case of a signal, try to
    name the corresponding symbol from the 'signal' module.
    """
    if rc is None:
        return "still running"
    
    if rc >= 0:
        return "terminated with rc %s" % rc

    if sys.platform.startswith("win"):
        # From http://stackoverflow.com/questions/20629027/process-finished-with-exit-code-1073741571
        # [-1073741571] is the signed integer representation of Microsoft's
        # "stack overflow/stack exhaustion" error code 0xC00000FD.
        # Anytime you see strange, large negative exit codes in windows, convert
        # them to hex and then look them up in the ntstatus error codes
        # http://msdn.microsoft.com/en-us/library/cc704588.aspx

        # Python bends over backwards to give you all the integer precision
        # you need, avoiding truncation. But only with 32-bit signed ints is
        # -1073741571 equivalent to 0xC00000FD! Explicitly truncate before
        # converting.
        hexrc = "0x%X" % (rc & 0xFFFFFFFF)
        # At this point, we're only trying to format the rc to make it easier
        # for a human being to understand. Any exception here -- file doesn't
        # exist, HTML parsing error, unrecognized table structure, unknown key
        # -- should NOT kill the script! It should only cause us to shrug and
        # present our caller with the best information available.
        try:
            table = get_windows_table()
            symbol, desc = table[hexrc]
        except Exception, err:
            log.error("(%s -- carrying on)" % err)
            log.error("terminated with rc %s (%s)" % (rc, hexrc))
        else:
            log.info("terminated with rc %s: %s: %s" % (hexrc, symbol, desc))

    else:
        # On Posix, negative rc means the child was terminated by signal -rc.
        rc = -rc
        for attr in dir(signal):
            if attr.startswith('SIG') and getattr(signal, attr) == rc:
                strc = attr
                break
        else:
            strc = str(rc)
        return "terminated by signal %s" % strc

class TableParser(HTMLParser.HTMLParser):
    """
    This HTMLParser subclass is designed to parse the table we know exists
    in windows-rcs.html, hopefully without building in too much knowledge of
    the specific way that table is currently formatted.
    """
    # regular expression matching any string containing only whitespace
    whitespace = re.compile(r'\s*$')

    def __init__(self):
        # Because Python 2.x's HTMLParser is an old-style class, we must use
        # old-style syntax to forward the __init__() call -- not super().
        HTMLParser.HTMLParser.__init__(self)
        # this will collect all the data, eventually
        self.table = []
        # Stack whose top (last item) indicates where to append current
        # element data. When empty, don't collect data at all.
        self.dest = []

    def handle_starttag(self, tag, attrs):
        if tag == "table":
            # This is the outermost tag we recognize. Collect nested elements
            # within self.table.
            self.dest.append(self.table)
        elif tag in ("tr", "td"):
            # Nested elements whose contents we want to capture as sublists.
            # To the list currently designated by the top of the dest stack,
            # append a new empty sublist.
            self.dest[-1].append([])
            # Now push THAT new, empty list as the new top of the dest stack.
            self.dest.append(self.dest[-1][-1])
        elif tag == "p":
            # We could handle <p> ... </p> just like <tr> or <td>, but that
            # introduces an unnecessary extra level of nesting. Just skip.
            pass
        else:
            # For any tag we don't recognize (notably <th>), push a new, empty
            # list to the top of the dest stack. This new list is NOT
            # referenced by anything in self.table; thus, when we pop it, any
            # data we've collected inside that list will be discarded.
            self.dest.append([])

    def handle_endtag(self, tag):
        # Because we avoid pushing self.dest for <p> in handle_starttag(), we
        # must refrain from popping it for </p> here.
        if tag != "p":
            # For everything else, including unrecognized tags, pop the dest
            # stack, reverting to outer collection.
            self.dest.pop()

    def handle_startendtag(self, tag, attrs):
        # The table of interest contains <td> entries of the form:
        # <p>0x00000000<br />STATUS_SUCCESS</p>
        # The <br/> is very useful -- we definitely want two different data
        # items for "0x00000000" and "STATUS_SUCCESS" -- but we don't need or
        # want it to push, then discard, an empty list as it would if we let
        # the default HTMLParser.handle_startendtag() call handle_starttag()
        # followed by handle_endtag(). Just ignore <br/> or any other
        # singleton tag.
        pass

    def handle_data(self, data):
        # Outside the <table> of interest, self.dest is empty. Do not bother
        # collecting data when self.dest is empty.
        # HTMLParser calls handle_data() with every chunk of whitespace
        # between tags. That would be lovely if our eventual goal was to
        # reconstitute the original input stream with its existing formatting,
        # but for us, whitespace only clutters the table. Ignore it.
        if self.dest and not self.whitespace.match(data):
            # Here we're within our <table> and we have non-whitespace data.
            # Append it to the list designated by the top of the dest stack.
            self.dest[-1].append(data)

# cache for get_windows_table()
_windows_table = None

def get_windows_table():
    global _windows_table
    # If we already loaded _windows_table, no need to load it all over again.
    if _windows_table:
        return _windows_table

    # windows-rcs.html was fetched on 2015-03-24 with the following command:
    # curl -o windows-rcs.html \
    #         https://msdn.microsoft.com/en-us/library/cc704588.aspx
    parser = TableParser()
    with open(os.path.join(os.path.dirname(__file__), "windows-rcs.html")) as hf:
        # We tried feeding the file data to TableParser in chunks, to avoid
        # buffering the entire file as a single string. Unfortunately its
        # handle_data() cannot tell the difference between distinct calls
        # separated by HTML tags, and distinct calls necessitated by a chunk
        # boundary. Sigh! Read in the whole file. At the time this was
        # written, it was only 500KB anyway.
        parser.feed(hf.read())
    parser.close()
    table = parser.table

    # With our parser, any <tr><th>...</th></tr> row leaves a table entry
    # consisting only of an empty list. Remove any such.
    while table and not table[0]:
        table.pop(0)

    # We expect rows of the form:
    # [['0x00000000', 'STATUS_SUCCESS'],
    #  ['The operation completed successfully.']]
    # The latter list will have multiple entries if Microsoft embedded <br/>
    # or <p> ... </p> in the text, in which case joining with '\n' is
    # appropriate.
    # Turn that into a dict whose key is the hex string, and whose value is
    # the pair (symbol, desc).
    _windows_table = dict((key, (symbol, '\n'.join(desc)))
                          for (key, symbol), desc in table)

    return _windows_table

log=logging.getLogger(__name__)
logging.basicConfig()

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("-d", "--debug", dest="loglevel", action="store_const",
                        const=logging.DEBUG, default=logging.WARNING)
    parser.add_argument("-D", "--define", dest="vars", default=[], action="append",
                        metavar="VAR=value",
                        help="Add VAR=value to the env variables defined")
    parser.add_argument("-l", "--libpath", dest="libpath", default=[], action="append",
                        metavar="DIR",
                        help="Add DIR to the platform-dependent DLL search path")
    parser.add_argument("command")
    parser.add_argument('args', nargs=argparse.REMAINDER)
    args = parser.parse_args()

    log.setLevel(args.loglevel)

    # What we have in opts.vars is a list of strings of the form "VAR=value"
    # or possibly just "VAR". What we want is a dict. We can build that dict by
    # constructing a list of ["VAR", "value"] pairs -- so split each
    # "VAR=value" string on the '=' sign (but only once, in case we have
    # "VAR=some=user=string"). To handle the case of just "VAR", append "" to
    # the list returned by split(), then slice off anything after the pair we
    # want.
    rc = main(command=args.command, arguments=args.args, libpath=args.libpath,
              vars=dict([(pair.split('=', 1) + [""])[:2] for pair in args.vars]))
    if rc not in (None, 0):
        log.error("Failure running: %s" % " ".join([args.command] + args.args))
        log.error("Error %s: %s" % (rc, translate_rc(rc)))
    sys.exit((rc < 0) and 255 or rc)
