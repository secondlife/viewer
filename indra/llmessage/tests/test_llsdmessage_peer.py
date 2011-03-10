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
from testrunner import run, debug

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
##         debug("Finished raw parse\n")
##         debug("parsed XML tree %s\n" % tree)
##         debug("parsed root node %s\n" % tree.getroot())
##         debug("root node tag %s\n" % tree.getroot().tag)
##         return llsd.to_python(tree.getroot())

    def do_GET(self):
        # Of course, don't attempt to read data.
        self.answer(dict(reply="success", status=500,
                         reason="Your GET operation requested failure"))

    def do_POST(self):
        # Read the provided POST data.
        self.answer(self.read_xml())

    def answer(self, data):
        if "fail" not in self.path:
            response = llsd.format_xml(data.get("reply", llsd.LLSD("success")))
            self.send_response(200)
            self.send_header("Content-type", "application/llsd+xml")
            self.send_header("Content-Length", str(len(response)))
            self.end_headers()
            self.wfile.write(response)
        else:                           # fail requested
            status = data.get("status", 500)
            reason = data.get("reason",
                               self.responses.get(status,
                                                  ("fail requested",
                                                   "Your request specified failure status %s "
                                                   "without providing a reason" % status))[1])
            self.send_error(status, reason)

    def log_request(self, code, size=None):
        # For present purposes, we don't want the request splattered onto
        # stderr, as it would upset devs watching the test run
        pass

    def log_error(self, format, *args):
        # Suppress error output as well
        pass

class TestHTTPServer(Thread):
    def run(self):
        httpd = HTTPServer(('127.0.0.1', 8000), TestHTTPRequestHandler)
        debug("Starting HTTP server...\n")
        httpd.serve_forever()

if __name__ == "__main__":
    sys.exit(run(server=TestHTTPServer(name="httpd"), *sys.argv[1:]))
