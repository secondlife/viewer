#!/usr/bin/python
"""\
@file   testrunner.py
@author Nat Goodspeed
@date   2009-03-20
@brief  Utilities for writing wrapper scripts for ADD_COMM_BUILD_TEST unit tests

$LicenseInfo:firstyear=2009&license=viewergpl$
Copyright (c) 2009, Linden Research, Inc.
$/LicenseInfo$
"""

import os
import sys

def debug(*args):
    sys.stdout.writelines(args)
    sys.stdout.flush()
# comment out the line below to enable debug output
debug = lambda *args: None

def run(*args, **kwds):
    """All positional arguments collectively form a command line, executed as
    a synchronous child process.
    In addition, pass server=new_thread_instance as an explicit keyword (to
    differentiate it from an additional command-line argument).
    new_thread_instance should be an instantiated but not yet started Thread
    subclass instance, e.g.:
    run("python", "-c", 'print "Hello, world!"', server=TestHTTPServer(name="httpd"))
    """
    # If there's no server= keyword arg, don't start a server thread: simply
    # run a child process.
    try:
        thread = kwds.pop("server")
    except KeyError:
        pass
    else:
        # Start server thread. Note that this and all other comm server
        # threads should be daemon threads: we'll let them run "forever,"
        # confident that the whole process will terminate when the main thread
        # terminates, which will be when the child process terminates.
        thread.setDaemon(True)
        thread.start()
    # choice of os.spawnv():
    # - [v vs. l] pass a list of args vs. individual arguments,
    # - [no p] don't use the PATH because we specifically want to invoke the
    #   executable passed as our first arg,
    # - [no e] child should inherit this process's environment.
    debug("Running %s...\n" % (" ".join(args)))
    sys.stdout.flush()
    rc = os.spawnv(os.P_WAIT, args[0], args)
    debug("%s returned %s\n" % (args[0], rc))
    return rc
