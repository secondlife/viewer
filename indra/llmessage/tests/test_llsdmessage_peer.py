#!/usr/bin/env python
"""\
@file   test_llsdmessage_peer.py
@author Nat Goodspeed
@date   2008-10-09
@brief  This script asynchronously runs the executable (with args) specified on
        the command line, returning its result code. While that executable is
        running, we provide dummy local services for use by C++ tests.

$LicenseInfo:firstyear=2008&license=viewerlgpl$
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
from threading import Thread
from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler

mydir = os.path.dirname(__file__)       # expected to be .../indra/llmessage/tests/
sys.path.insert(0, os.path.join(mydir, os.pardir, os.pardir, "lib", "python"))
from indra.util.fastest_elementtree import parse as xml_parse
from indra.base import llsd
from testrunner import freeport, run, debug, VERBOSE
import time

_storage=None

class TestHTTPRequestHandler(BaseHTTPRequestHandler):
    """This subclass of BaseHTTPRequestHandler is to receive and echo
    LLSD-flavored messages sent by the C++ LLHTTPClient.
    """
    def read(self):
        # The following logic is adapted from the library module
        # SimpleXMLRPCServer.py.
        # Get arguments by reading body of request.
        # We read this in chunks to avoid straining
        # socket.read(); around the 10 or 15Mb mark, some platforms
        # begin to have problems (bug #792570).
        try:
            size_remaining = int(self.headers["content-length"])
        except (KeyError, ValueError):
            return ""
        max_chunk_size = 10*1024*1024
        L = []
        while size_remaining:
            chunk_size = min(size_remaining, max_chunk_size)
            chunk = self.rfile.read(chunk_size)
            L.append(chunk)
            size_remaining -= len(chunk)
        return ''.join(L)
        # end of swiped read() logic

    def read_xml(self):
        # This approach reads the entire POST data into memory first
        return llsd.parse(self.read())
##         # This approach attempts to stream in the LLSD XML from self.rfile,
##         # assuming that the underlying XML parser reads its input file
##         # incrementally. Unfortunately I haven't been able to make it work.
##         tree = xml_parse(self.rfile)
##         debug("Finished raw parse")
##         debug("parsed XML tree %s", tree)
##         debug("parsed root node %s", tree.getroot())
##         debug("root node tag %s", tree.getroot().tag)
##         return llsd.to_python(tree.getroot())

    def do_HEAD(self):
        self.do_GET(withdata=False)

    def do_GET(self, withdata=True):
        # Of course, don't attempt to read data.
        data = dict(reply="success", body="avatar", random=17)
        self.answer(data, withdata=withdata)

    def do_POST(self):
        # Read the provided POST data.
        self.answer(self.read_xml())

    def do_PUT(self):
        # Read the provided PUT data.
        self.answer(self.read_xml())

    def answer(self, data, withdata=True):
        global _storage
        debug("%s.answer(%s): self.path = %r", self.__class__.__name__, data, self.path)
        if "fail" in self.path or "test/error" in self.path: # fail requested
            status = data.get("status", 500)
            # self.responses maps an int status to a (short, long) pair of
            # strings. We want the longer string. That's why we pass a string
            # pair to get(): the [1] will select the second string, whether it
            # came from self.responses or from our default pair.
            reason = data.get("reason",
                               self.responses.get(status,
                                                  ("fail requested",
                                                   "Your request specified failure status %s "
                                                   "without providing a reason" % status))[1])
            debug("fail requested: %s: %r", status, reason)
            self.send_error(status, reason)
        else:
            if "web/echo" in self.path:
                pass
            elif "test/timeout" in self.path:
                time.sleep(5.0)
                return
            elif "test/storage" in self.path:
                if "GET" == self.command:
                    data = _storage
                else:
                    _storage = data
                    data = "ok"
            else:
                data = data.copy()          # we're going to modify
                # Ensure there's a "reply" key in data, even if there wasn't before
                data["reply"] = data.get("reply", llsd.LLSD("success"))
            response = llsd.format_xml(data)
            debug("success: %s", response)
            self.send_response(200)
            self.send_header("Content-type", "application/llsd+xml")
            self.send_header("Content-Length", str(len(response)))
            self.end_headers()
            if withdata:
                self.wfile.write(response)

    if not VERBOSE:
        # When VERBOSE is set, skip both these overrides because they exist to
        # suppress output.

        def log_request(self, code, size=None):
            # For present purposes, we don't want the request splattered onto
            # stderr, as it would upset devs watching the test run
            pass

        def log_error(self, format, *args):
            # Suppress error output as well
            pass

class Server(HTTPServer):
    # This pernicious flag is on by default in HTTPServer. But proper
    # operation of freeport() absolutely depends on it being off.
    allow_reuse_address = False

if __name__ == "__main__":
    # Instantiate a Server(TestHTTPRequestHandler) on the first free port
    # in the specified port range. Doing this inline is better than in a
    # daemon thread: if it blows up here, we'll get a traceback. If it blew up
    # in some other thread, the traceback would get eaten and we'd run the
    # subject test program anyway.
    httpd, port = freeport(xrange(8000, 8020),
                           lambda port: Server(('127.0.0.1', port), TestHTTPRequestHandler))
    # Pass the selected port number to the subject test program via the
    # environment. We don't want to impose requirements on the test program's
    # command-line parsing -- and anyway, for C++ integration tests, that's
    # performed in TUT code rather than our own.
    os.environ["PORT"] = str(port)
    debug("$PORT = %s", port)
    sys.exit(run(server=Thread(name="httpd", target=httpd.serve_forever), *sys.argv[1:]))
