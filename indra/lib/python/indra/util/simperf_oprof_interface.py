#!/usr/bin/python
"""\
@file simperf_oprof_interface.py
@brief Manage OProfile data collection on a host

$LicenseInfo:firstyear=2008&license=mit$

Copyright (c) 2008-2009, Linden Research, Inc.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
$/LicenseInfo$
"""

import sys, os, getopt
import simplejson


def usage():
    print "Usage:"
    print sys.argv[0] + " [options]"
    print "  Digest the OProfile report forms that come out of the"
    print "  simperf_oprof_ctl program's -r/--report command.  The result"
    print "  is an array of dictionaires with the following keys:"
    print 
    print "     symbol        Name of sampled, calling, or called procedure"
    print "     file          Executable or library where symbol resides"
    print "     percentage    Percentage contribution to profile, calls or called"
    print "     samples       Sample count"
    print "     calls         Methods called by the method in question (full only)"
    print "     called_by     Methods calling the method (full only)"
    print 
    print "  For 'full' reports the two keys 'calls' and 'called_by' are"
    print "  themselves arrays of dictionaries based on the first four keys."
    print
    print "Return Codes:"
    print "  None.  Aggressively digests everything.  Will likely mung results"
    print "  if a program or library has whitespace in its name."
    print
    print "Options:"
    print "  -i, --in      Input settings filename.  (Default:  stdin)"
    print "  -o, --out     Output settings filename.  (Default:  stdout)"
    print "  -h, --help    Print this message and exit."
    print
    print "Interfaces:"
    print "   class SimPerfOProfileInterface()"
    
class SimPerfOProfileInterface:
    def __init__(self):
        self.isBrief = True             # public
        self.isValid = False            # public
        self.result = []                # public

    def parse(self, input):
        in_samples = False
        for line in input:
            if in_samples:
                if line[0:6] == "------":
                    self.isBrief = False
                    self._parseFull(input)
                else:
                    self._parseBrief(input, line)
                self.isValid = True
                return
            try:
                hd1, remain = line.split(None, 1)
                if hd1 == "samples":
                    in_samples = True
            except ValueError:
                pass

    def _parseBrief(self, input, line1):
        try:
            fld1, fld2, fld3, fld4 = line1.split(None, 3)
            self.result.append({"samples" : fld1,
                                "percentage" : fld2,
                                "file" : fld3,
                                "symbol" : fld4.strip("\n")})
        except ValueError:
            pass
        for line in input:
            try:
                fld1, fld2, fld3, fld4 = line.split(None, 3)
                self.result.append({"samples" : fld1,
                                    "percentage" : fld2,
                                    "file" : fld3,
                                    "symbol" : fld4.strip("\n")})
            except ValueError:
                pass

    def _parseFull(self, input):
        state = 0       # In 'called_by' section
        calls = []
        called_by = []
        current = {}
        for line in input:
            if line[0:6] == "------":
                if len(current):
                    current["calls"] = calls
                    current["called_by"] = called_by
                    self.result.append(current)
                state = 0
                calls = []
                called_by = []
                current = {}
            else:
                try:
                    fld1, fld2, fld3, fld4 = line.split(None, 3)
                    tmp = {"samples" : fld1,
                           "percentage" : fld2,
                           "file" : fld3,
                           "symbol" : fld4.strip("\n")}
                except ValueError:
                    continue
                if line[0] != " ":
                    current = tmp
                    state = 1       # In 'calls' section
                elif state == 0:
                    called_by.append(tmp)
                else:
                    calls.append(tmp)
        if len(current):
            current["calls"] = calls
            current["called_by"] = called_by
            self.result.append(current)


def main(argv=None):
    opts, args = getopt.getopt(sys.argv[1:], "i:o:h", ["in=", "out=", "help"])
    input_file = sys.stdin
    output_file = sys.stdout
    for o, a in opts:
        if o in ("-i", "--in"):
            input_file = open(a, 'r')
        if o in ("-o", "--out"):
            output_file = open(a, 'w')
        if o in ("-h", "--help"):
            usage()
            sys.exit(0)

    oprof = SimPerfOProfileInterface()
    oprof.parse(input_file)
    if input_file != sys.stdin:
        input_file.close()

    # Create JSONable dict with interesting data and format/print it
    print >>output_file, simplejson.dumps(oprof.result)

    return 0

if __name__ == "__main__":
    sys.exit(main())
