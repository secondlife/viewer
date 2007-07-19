#!/usr/bin/python
# @file template_verifier.py
# @brief Message template compatibility verifier.
#
# Copyright (c) 2007-$CurrentYear$, Linden Research, Inc.
# $License$

"""template_verifier is a script which will compare the
current repository message template with the "master" message template, accessible
via http://secondlife.com/app/message_template/master_message_template.msg
If [FILE] is specified, it will be checked against the master template.
If [FILE] [FILE] is specified, two local files will be checked against
each other.
"""

from os.path import realpath, dirname, join, exists
setup_path = join(dirname(realpath(__file__)), "setup-path.py")
if exists(setup_path):
    execfile(setup_path)
import optparse
import os
import sys
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


def compare(base, current, mode):
    """Compare the current template against the base template using the given
    'mode' strictness:

    development: Allows Same, Newer, Older, and Mixed
    production: Allows only Same or Newer

    Print out information about whether the current template is compatible
    with the base template.

    Returns a tuple of (bool, Compatibility)
    Return True if they are compatible in this mode, False if not.
    """
    try:
        base = llmessage.parseTemplateString(base)
    except tokenstream.ParseError, e:
        print "Error parsing master message template -- this might be a network problem, try again"
        raise e

    try:
        current = llmessage.parseTemplateString(current)
    except tokenstream.ParseError, e:
        print "Error parsing local message template"
        raise e
    
    compat = current.compatibleWithBase(base)
    if mode == 'production':
        acceptable = PRODUCTION_ACCEPTABLE
    else:
        acceptable = DEVELOPMENT_ACCEPTABLE

    if type(compat) in acceptable:
        return True, compat
    return False, compat

def local_template_filename():
    """Returns the message template's default location relative to template_verifier.py:
    ./messages/message_template.msg."""
    d = os.path.dirname(os.path.realpath(__file__))
    return os.path.join(d, 'messages', MESSAGE_TEMPLATE)

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

    options, args = parser.parse_args(sysargs)

    # both current and master supplied in positional params
    if len(args) == 2:
        master_filename, current_filename = args
        print "base:", master_filename
        print "current:", current_filename
        master = file(master_filename).read()
        current = file(current_filename).read()
    # only current supplied in positional param
    elif len(args) == 1:
        master = None
        current_filename = args[0]
        print "base: <master template from repository>"
        print "current:", current_filename
        current = file(current_filename).read()
    # nothing specified, use defaults for everything
    elif len(args) == 0:
        master = None
        current = None
    else:
        die("Too many arguments")

    # fetch the master from the url (default or supplied)
    if master is None:
        master = ''.join(urllib.urlopen(options.master_url).readlines())

    # fetch the template for this build
    if current is None:
        current_filename = local_template_filename()
        print "base: <master template from repository>"
        print "current:", current_filename
        current = file(current_filename).read()

    acceptable, compat = compare(
        master, current, options.mode)

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


