#!/usr/bin/env python
"""\
@file   test_llxmlrpc_peer.py
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
from SimpleXMLRPCServer import SimpleXMLRPCServer

mydir = os.path.dirname(__file__)       # expected to be .../indra/newview/tests/
sys.path.insert(0, os.path.join(mydir, os.pardir, os.pardir, "lib", "python"))
sys.path.insert(1, os.path.join(mydir, os.pardir, os.pardir, "llmessage", "tests"))
from testrunner import freeport, run, debug

class TestServer(SimpleXMLRPCServer):
    def _dispatch(self, method, params):
        try:
            func = getattr(self, method)
        except AttributeError:
            raise Exception('method "%s" is not supported' % method)
        else:
            # LLXMLRPCListener constructs XMLRPC parameters that arrive as a
            # 1-tuple containing a dict.
            return func(**(params[0]))

    def hello(self, who):
        # LLXMLRPCListener expects a dict return.
        return {"hi_there": "Hello, %s!" % who}

    def getdict(self):
        return dict(nested_dict=dict(a=17, b=5))

    def log_request(self, code, size=None):
        # For present purposes, we don't want the request splattered onto
        # stderr, as it would upset devs watching the test run
        pass

    def log_error(self, format, *args):
        # Suppress error output as well
        pass

if __name__ == "__main__":
    # Instantiate a TestServer on the first free port in the specified port
    # range. Doing this inline is better than in a daemon thread: if it blows
    # up here, we'll get a traceback. If it blew up in some other thread, the
    # traceback would get eaten and we'd run the subject test program anyway.
    xmlrpcd, port = freeport(xrange(8000, 8020),
                             lambda port: TestServer(('127.0.0.1', port)))
    # Pass the selected port number to the subject test program via the
    # environment. We don't want to impose requirements on the test program's
    # command-line parsing -- and anyway, for C++ integration tests, that's
    # performed in TUT code rather than our own.
    os.environ["PORT"] = str(port)
    sys.exit(run(server=Thread(name="xmlrpc", target=xmlrpcd.serve_forever), *sys.argv[1:]))
