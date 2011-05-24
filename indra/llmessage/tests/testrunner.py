#!/usr/bin/env python
"""\
@file   testrunner.py
@author Nat Goodspeed
@date   2009-03-20
@brief  Utilities for writing wrapper scripts for ADD_COMM_BUILD_TEST unit tests

$LicenseInfo:firstyear=2009&license=viewerlgpl$
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

import os
import sys
import errno
import socket

def debug(*args):
    sys.stdout.writelines(args)
    sys.stdout.flush()
# comment out the line below to enable debug output
debug = lambda *args: None

def freeport(portlist, expr):
    """
    Find a free server port to use. Specifically, evaluate 'expr' (a
    callable(port)) until it stops raising EADDRINUSE exception.

    Pass:

    portlist: an iterable (e.g. xrange()) of ports to try. If you exhaust the
    range, freeport() lets the socket.error exception propagate. If you want
    unbounded, you could pass itertools.count(baseport), though of course in
    practice the ceiling is 2^16-1 anyway. But it seems prudent to constrain
    the range much more sharply: if we're iterating an absurd number of times,
    probably something else is wrong.

    expr: a callable accepting a port number, specifically one of the items
    from portlist. If calling that callable raises socket.error with
    EADDRINUSE, freeport() retrieves the next item from portlist and retries.

    Returns: (expr(port), port)

    port: the value from portlist for which expr(port) succeeded

    Raises:

    Any exception raised by expr(port) other than EADDRINUSE.

    socket.error if, for every item from portlist, expr(port) raises
    socket.error. The exception you see is the one from the last item in
    portlist.

    StopIteration if portlist is completely empty.

    Example:

    server, port = freeport(xrange(8000, 8010),
                            lambda port: HTTPServer(("localhost", port),
                                                    MyRequestHandler))
    # pass 'port' to client code
    # call server.serve_forever()
    """
    # If portlist is completely empty, let StopIteration propagate: that's an
    # error because we can't return meaningful values. We have no 'port',
    # therefore no 'expr(port)'.
    portiter = iter(portlist)
    port = portiter.next()

    while True:
        try:
            # If this value of port works, return as promised.
            return expr(port), port

        except socket.error, err:
            # Anything other than 'Address already in use', propagate
            if err.args[0] != errno.EADDRINUSE:
                raise

            # Here we want the next port from portiter. But on StopIteration,
            # we want to raise the original exception rather than
            # StopIteration. So save the original exc_info().
            type, value, tb = sys.exc_info()
            try:
                try:
                    port = portiter.next()
                except StopIteration:
                    raise type, value, tb
            finally:
                # Clean up local traceback, see docs for sys.exc_info()
                del tb

        # Recap of the control flow above:
        # If expr(port) doesn't raise, return as promised.
        # If expr(port) raises anything but EADDRINUSE, propagate that
        # exception.
        # If portiter.next() raises StopIteration -- that is, if the port
        # value we just passed to expr(port) was the last available -- reraise
        # the EADDRINUSE exception.
        # If we've actually arrived at this point, portiter.next() delivered a
        # new port value. Loop back to pass that to expr(port).

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
