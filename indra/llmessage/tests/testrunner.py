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
import re
import errno
import socket
import subprocess

VERBOSE = os.environ.get("INTEGRATION_TEST_VERBOSE", "0") # default to quiet
# Support usage such as INTEGRATION_TEST_VERBOSE=off -- distressing to user if
# that construct actually turns on verbosity...
VERBOSE = not re.match(r"(0|off|false|quiet)$", VERBOSE, re.IGNORECASE)

if VERBOSE:
    def debug(fmt, *args):
        print fmt % args
        sys.stdout.flush()
else:
    debug = lambda *args: None

class Error(Exception):
    pass

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

    class Server(HTTPServer):
        # If you use BaseHTTPServer.HTTPServer, turning off this flag is
        # essential for proper operation of freeport()!
        allow_reuse_address = False
    # ...
    server, port = freeport(xrange(8000, 8010),
                            lambda port: Server(("localhost", port),
                                                MyRequestHandler))
    # pass 'port' to client code
    # call server.serve_forever()
    """
    try:
        # If portlist is completely empty, let StopIteration propagate: that's an
        # error because we can't return meaningful values. We have no 'port',
        # therefore no 'expr(port)'.
        portiter = iter(portlist)
        port = portiter.next()

        while True:
            try:
                # If this value of port works, return as promised.
                value = expr(port)

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

            else:
                debug("freeport() returning %s on port %s", value, port)
                return value, port

            # Recap of the control flow above:
            # If expr(port) doesn't raise, return as promised.
            # If expr(port) raises anything but EADDRINUSE, propagate that
            # exception.
            # If portiter.next() raises StopIteration -- that is, if the port
            # value we just passed to expr(port) was the last available -- reraise
            # the EADDRINUSE exception.
            # If we've actually arrived at this point, portiter.next() delivered a
            # new port value. Loop back to pass that to expr(port).

    except Exception, err:
        debug("*** freeport() raising %s: %s", err.__class__.__name__, err)
        raise

def run(*args, **kwds):
    """
    Run a specified command as a synchronous child process, optionally
    launching a server Thread during the run.

    All positional arguments collectively form a command line. The first
    positional argument names the program file to execute.

    Returns the termination code of the child process.

    In addition, you may pass keyword-only arguments:

    use_path=True: allow a simple filename as command and search PATH for that
    filename. (This argument is retained for backwards compatibility but is
    now the default behavior.)

    server_inst: an instance of a subclass of SocketServer.BaseServer.

    When you pass server_inst, run() calls its handle_request() method in a
    loop until the child process terminates.
    """
    # server= keyword arg is discontinued
    try:
        thread = kwds.pop("server")
    except KeyError:
        pass
    else:
        raise Error("Obsolete call to testrunner.run(): pass server_inst=, not server=")

    debug("Running %s...", " ".join(args))

    try:
        server_inst = kwds.pop("server_inst")
    except KeyError:
        # Without server_inst, this is very simple: just run child process.
        rc = subprocess.call(args)
    else:
        # We're being asked to run a local server while the child process
        # runs. We used to launch a daemon thread calling
        # server_inst.serve_forever(), then eventually call sys.exit() with
        # the daemon thread still running -- but in recent versions of Python
        # 2, even when you call sys.exit(0), apparently killing the thread
        # causes the Python runtime to force the process termination code
        # nonzero. So now we avoid the extra thread altogether.

        # SocketServer.BaseServer.handle_request() honors a 'timeout'
        # attribute, if it's set to something other than None.
        # We pick 0.5 seconds because that's the default poll timeout for
        # BaseServer.serve_forever(), which is what we used to use.
        server_inst.timeout = 0.5

        child = subprocess.Popen(args)
        while child.poll() is None:
            # Setting server_inst.timeout is what keeps this handle_request()
            # call from blocking "forever." Interestingly, looping over
            # handle_request() with a timeout is very like the implementation
            # of serve_forever(). We just check a different flag to break out.
            # It might be interesting if handle_request() returned an
            # indication of whether it in fact handled a request or timed out.
            # Oddly, it doesn't. We could discover that by overriding
            # handle_timeout(), whose default implementation does nothing --
            # but in fact we really don't care. All that matters is that we
            # regularly poll both the child process and the server socket.
            server_inst.handle_request()
        # We don't bother to capture the rc returned by child.poll() because
        # poll() is already defined to capture that in its returncode attr.
        rc = child.returncode

    debug("%s returned %s", args[0], rc)
    return rc

# ****************************************************************************
#   test code -- manual at this point, see SWAT-564
# ****************************************************************************
def test_freeport():
    # ------------------------------- Helpers --------------------------------
    from contextlib import contextmanager
    # helper Context Manager for expecting an exception
    # with exc(SomeError):
    #     raise SomeError()
    # raises AssertionError otherwise.
    @contextmanager
    def exc(exception_class, *args):
        try:
            yield
        except exception_class, err:
            for i, expected_arg in enumerate(args):
                assert expected_arg == err.args[i], \
                       "Raised %s, but args[%s] is %r instead of %r" % \
                       (err.__class__.__name__, i, err.args[i], expected_arg)
            print "Caught expected exception %s(%s)" % \
                  (err.__class__.__name__, ', '.join(repr(arg) for arg in err.args))
        else:
            assert False, "Failed to raise " + exception_class.__class__.__name__

    # helper to raise specified exception
    def raiser(exception):
        raise exception

    # the usual
    def assert_equals(a, b):
        assert a == b, "%r != %r" % (a, b)

    # ------------------------ Sanity check the above ------------------------
    class SomeError(Exception): pass
    # Without extra args, accept any err.args value
    with exc(SomeError):
        raiser(SomeError("abc"))
    # With extra args, accept only the specified value
    with exc(SomeError, "abc"):
        raiser(SomeError("abc"))
    with exc(AssertionError):
        with exc(SomeError, "abc"):
            raiser(SomeError("def"))
    with exc(AssertionError):
        with exc(socket.error, errno.EADDRINUSE):
            raiser(socket.error(errno.ECONNREFUSED, 'Connection refused'))

    # ----------- freeport() without engaging socket functionality -----------
    # If portlist is empty, freeport() raises StopIteration.
    with exc(StopIteration):
        freeport([], None)

    assert_equals(freeport([17], str), ("17", 17))

    # This is the magic exception that should prompt us to retry
    inuse = socket.error(errno.EADDRINUSE, 'Address already in use')
    # Get the iterator to our ports list so we can check later if we've used all
    ports = iter(xrange(5))
    with exc(socket.error, errno.EADDRINUSE):
        freeport(ports, lambda port: raiser(inuse))
    # did we entirely exhaust 'ports'?
    with exc(StopIteration):
        ports.next()

    ports = iter(xrange(2))
    # Any exception but EADDRINUSE should quit immediately
    with exc(SomeError):
        freeport(ports, lambda port: raiser(SomeError()))
    assert_equals(ports.next(), 1)

    # ----------- freeport() with platform-dependent socket stuff ------------
    # This is what we should've had unit tests to begin with (see CHOP-661).
    def newbind(port):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.bind(('127.0.0.1', port))
        return sock

    bound0, port0 = freeport(xrange(7777, 7780), newbind)
    assert_equals(port0, 7777)
    bound1, port1 = freeport(xrange(7777, 7780), newbind)
    assert_equals(port1, 7778)
    bound2, port2 = freeport(xrange(7777, 7780), newbind)
    assert_equals(port2, 7779)
    with exc(socket.error, errno.EADDRINUSE):
        bound3, port3 = freeport(xrange(7777, 7780), newbind)

if __name__ == "__main__":
    test_freeport()
