#!/usr/bin/python
"""\
@file   test_llxmlrpc_peer.py
@author Nat Goodspeed
@date   2008-10-09
@brief  This script asynchronously runs the executable (with args) specified on
        the command line, returning its result code. While that executable is
        running, we provide dummy local services for use by C++ tests.

$LicenseInfo:firstyear=2008&license=viewergpl$
Copyright (c) 2008, Linden Research, Inc.
$/LicenseInfo$
"""

import os
import sys
from threading import Thread
from SimpleXMLRPCServer import SimpleXMLRPCServer

mydir = os.path.dirname(__file__)       # expected to be .../indra/newview/tests/
sys.path.insert(0, os.path.join(mydir, os.pardir, os.pardir, "lib", "python"))
sys.path.insert(1, os.path.join(mydir, os.pardir, os.pardir, "llmessage", "tests"))
from testrunner import run, debug

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

class ServerRunner(Thread):
    def run(self):
        server = TestServer(('127.0.0.1', 8000))
        debug("Starting XMLRPC server...\n")
        server.serve_forever()

if __name__ == "__main__":
    sys.exit(run(server=ServerRunner(name="xmlrpc"), *sys.argv[1:]))
