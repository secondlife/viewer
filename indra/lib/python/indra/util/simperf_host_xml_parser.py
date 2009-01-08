#!/usr/bin/env python
"""\
@file simperf_host_xml_parser.py
@brief Digest collector's XML dump and convert to simple dict/list structure

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

import sys, os, getopt, time
import simplejson
from xml import sax


def usage():
    print "Usage:"
    print sys.argv[0] + " [options]"
    print "  Convert RRD's XML dump to JSON.  Script to convert the simperf_host_collector-"
    print "  generated RRD dump into JSON.  Steps include converting selected named"
    print "  fields from GAUGE type to COUNTER type by computing delta with preceding"
    print "  values.  Top-level named fields are:"
    print 
    print "     lastupdate      Time (javascript timestamp) of last data sample"
    print "     step            Time in seconds between samples"
    print "     ds              Data specification (name/type) for each column"
    print "     database        Table of data samples, one time step per row"
    print 
    print "Options:"
    print "  -i, --in      Input settings filename.  (Default:  stdin)"
    print "  -o, --out     Output settings filename.  (Default:  stdout)"
    print "  -h, --help    Print this message and exit."
    print
    print "Example: %s -i rrddump.xml -o rrddump.json" % sys.argv[0]
    print
    print "Interfaces:"
    print "   class SimPerfHostXMLParser()         # SAX content handler"
    print "   def simperf_host_xml_fixup(parser)   # post-parse value fixup"

class SimPerfHostXMLParser(sax.handler.ContentHandler):

    def __init__(self):
        pass
        
    def startDocument(self):
        self.rrd_last_update = 0         # public
        self.rrd_step = 0                # public
        self.rrd_ds = []                 # public
        self.rrd_records = []            # public
        self._rrd_level = 0
        self._rrd_parse_state = 0
        self._rrd_chars = ""
        self._rrd_capture = False
        self._rrd_ds_val = {}
        self._rrd_data_row = []
        self._rrd_data_row_has_nan = False
        
    def endDocument(self):
        pass

    # Nasty little ad-hoc state machine to extract the elements that are
    # necessary from the 'rrdtool dump' XML output.  The same element
    # name '<ds>' is used for two different data sets so we need to pay
    # some attention to the actual structure to get the ones we want
    # and ignore the ones we don't.
    
    def startElement(self, name, attrs):
        self._rrd_level = self._rrd_level + 1
        self._rrd_capture = False
        if self._rrd_level == 1:
            if name == "rrd" and self._rrd_parse_state == 0:
                self._rrd_parse_state = 1     # In <rrd>
                self._rrd_capture = True
                self._rrd_chars = ""
        elif self._rrd_level == 2:
            if self._rrd_parse_state == 1:
                if name == "lastupdate":
                    self._rrd_parse_state = 2         # In <rrd><lastupdate>
                    self._rrd_capture = True
                    self._rrd_chars = ""
                elif name == "step":
                    self._rrd_parse_state = 3         # In <rrd><step>
                    self._rrd_capture = True
                    self._rrd_chars = ""
                elif name == "ds":
                    self._rrd_parse_state = 4         # In <rrd><ds>
                    self._rrd_ds_val = {}
                    self._rrd_chars = ""
                elif name == "rra":
                    self._rrd_parse_state = 5         # In <rrd><rra>
        elif self._rrd_level == 3:
            if self._rrd_parse_state == 4:
                if name == "name":
                    self._rrd_parse_state = 6         # In <rrd><ds><name>
                    self._rrd_capture = True
                    self._rrd_chars = ""
                elif name == "type":
                    self._rrd_parse_state = 7         # In <rrd><ds><type>
                    self._rrd_capture = True
                    self._rrd_chars = ""
            elif self._rrd_parse_state == 5:
                if name == "database":
                    self._rrd_parse_state = 8         # In <rrd><rra><database>
        elif self._rrd_level == 4:
            if self._rrd_parse_state == 8:
                if name == "row":
                    self._rrd_parse_state = 9         # In <rrd><rra><database><row>
                    self._rrd_data_row = []
                    self._rrd_data_row_has_nan = False
        elif self._rrd_level == 5:
            if self._rrd_parse_state == 9:
                if name == "v":
                    self._rrd_parse_state = 10        # In <rrd><rra><database><row><v>
                    self._rrd_capture = True
                    self._rrd_chars = ""

    def endElement(self, name):
        self._rrd_capture = False
        if self._rrd_parse_state == 10:
            self._rrd_capture = self._rrd_level == 6
            if self._rrd_level == 5:
                if self._rrd_chars == "NaN":
                    self._rrd_data_row_has_nan = True
                else:
                    self._rrd_data_row.append(self._rrd_chars)
                self._rrd_parse_state = 9              # In <rrd><rra><database><row>
        elif self._rrd_parse_state == 9:
            if self._rrd_level == 4:
                if not self._rrd_data_row_has_nan:
                    self.rrd_records.append(self._rrd_data_row)
                self._rrd_parse_state = 8              # In <rrd><rra><database>
        elif self._rrd_parse_state == 8:
            if self._rrd_level == 3:
                self._rrd_parse_state = 5              # In <rrd><rra>
        elif self._rrd_parse_state == 7:
            if self._rrd_level == 3:
                self._rrd_ds_val["type"] = self._rrd_chars
                self._rrd_parse_state = 4              # In <rrd><ds>
        elif self._rrd_parse_state == 6:
            if self._rrd_level == 3:
                self._rrd_ds_val["name"] = self._rrd_chars
                self._rrd_parse_state = 4              # In <rrd><ds>
        elif self._rrd_parse_state == 5:
            if self._rrd_level == 2:
                self._rrd_parse_state = 1              # In <rrd>
        elif self._rrd_parse_state == 4:
            if self._rrd_level == 2:
                self.rrd_ds.append(self._rrd_ds_val)
                self._rrd_parse_state = 1              # In <rrd>
        elif self._rrd_parse_state == 3:
            if self._rrd_level == 2:
                self.rrd_step = long(self._rrd_chars)
                self._rrd_parse_state = 1              # In <rrd>
        elif self._rrd_parse_state == 2:
            if self._rrd_level == 2:
                self.rrd_last_update = long(self._rrd_chars)
                self._rrd_parse_state = 1              # In <rrd>
        elif self._rrd_parse_state == 1:
            if self._rrd_level == 1:
                self._rrd_parse_state = 0              # At top
                
        if self._rrd_level:
            self._rrd_level = self._rrd_level - 1

    def characters(self, content):
        if self._rrd_capture:
            self._rrd_chars = self._rrd_chars + content.strip()

def _make_numeric(value):
    try:
        value = float(value)
    except:
        value = ""
    return value

def simperf_host_xml_fixup(parser, filter_start_time = None, filter_end_time = None):
    # Fixup for GAUGE fields that are really COUNTS.  They
    # were forced to GAUGE to try to disable rrdtool's
    # data interpolation/extrapolation for non-uniform time
    # samples.
    fixup_tags = [ "cpu_user",
                   "cpu_nice",
                   "cpu_sys",
                   "cpu_idle",
                   "cpu_waitio",
                   "cpu_intr",
                   # "file_active",
                   # "file_free",
                   # "inode_active",
                   # "inode_free",
                   "netif_in_kb",
                   "netif_in_pkts",
                   "netif_in_errs",
                   "netif_in_drop",
                   "netif_out_kb",
                   "netif_out_pkts",
                   "netif_out_errs",
                   "netif_out_drop",
                   "vm_page_in",
                   "vm_page_out",
                   "vm_swap_in",
                   "vm_swap_out",
                   #"vm_mem_total",
                   #"vm_mem_used",
                   #"vm_mem_active",
                   #"vm_mem_inactive",
                   #"vm_mem_free",
                   #"vm_mem_buffer",
                   #"vm_swap_cache",
                   #"vm_swap_total",
                   #"vm_swap_used",
                   #"vm_swap_free",
                   "cpu_interrupts",
                   "cpu_switches",
                   "cpu_forks" ]

    col_count = len(parser.rrd_ds)
    row_count = len(parser.rrd_records)

    # Process the last row separately, just to make all values numeric.
    for j in range(col_count):
        parser.rrd_records[row_count - 1][j] = _make_numeric(parser.rrd_records[row_count - 1][j])

    # Process all other row/columns.
    last_different_row = row_count - 1
    current_row = row_count - 2
    while current_row >= 0:
        # Check for a different value than the previous row.  If everything is the same
        # then this is probably just a filler/bogus entry.
        is_different = False
        for j in range(col_count):
            parser.rrd_records[current_row][j] = _make_numeric(parser.rrd_records[current_row][j])
            if parser.rrd_records[current_row][j] != parser.rrd_records[last_different_row][j]:
                # We're good.  This is a different row.
                is_different = True

        if not is_different:
            # This is a filler/bogus entry.  Just ignore it.
            for j in range(col_count):
                parser.rrd_records[current_row][j] = float('nan')
        else:
            # Some tags need to be converted into deltas.
            for j in range(col_count):
                if parser.rrd_ds[j]["name"] in fixup_tags:
                    parser.rrd_records[last_different_row][j] = \
                        parser.rrd_records[last_different_row][j] - parser.rrd_records[current_row][j]
            last_different_row = current_row

        current_row -= 1

    # Set fixup_tags in the first row to 'nan' since they aren't useful anymore.
    for j in range(col_count):
        if parser.rrd_ds[j]["name"] in fixup_tags:
            parser.rrd_records[0][j] = float('nan')

    # Add a timestamp to each row and to the catalog.  Format and name
    # chosen to match other simulator logging (hopefully).
    start_time = parser.rrd_last_update - (parser.rrd_step * (row_count - 1))
    # Build a filtered list of rrd_records if we are limited to a time range.
    filter_records = False
    if filter_start_time is not None or filter_end_time is not None:
        filter_records = True
        filtered_rrd_records = []
        if filter_start_time is None:
            filter_start_time = start_time * 1000
        if filter_end_time is None:
            filter_end_time = parser.rrd_last_update * 1000
        
    for i in range(row_count):
        record_timestamp = (start_time + (i * parser.rrd_step)) * 1000
        parser.rrd_records[i].insert(0, record_timestamp)
        if filter_records:
            if filter_start_time <= record_timestamp and record_timestamp <= filter_end_time:
                filtered_rrd_records.append(parser.rrd_records[i])

    if filter_records:
        parser.rrd_records = filtered_rrd_records

    parser.rrd_ds.insert(0, {"type": "GAUGE", "name": "javascript_timestamp"})


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

    # Using the SAX parser as it is at least 4X faster and far, far
    # smaller on this dataset than the DOM-based interface in xml.dom.minidom.
    # With SAX and a 5.4MB xml file, this requires about seven seconds of
    # wall-clock time and 32MB VSZ.  With the DOM interface, about 22 seconds
    # and over 270MB VSZ.

    handler = SimPerfHostXMLParser()
    sax.parse(input_file, handler)
    if input_file != sys.stdin:
        input_file.close()

    # Various format fixups:  string-to-num, gauge-to-counts, add
    # a time stamp, etc.
    simperf_host_xml_fixup(handler)
    
    # Create JSONable dict with interesting data and format/print it
    print >>output_file, simplejson.dumps({ "step" : handler.rrd_step,
                                            "lastupdate": handler.rrd_last_update * 1000,
                                            "ds" : handler.rrd_ds,
                                            "database" : handler.rrd_records })

    return 0

if __name__ == "__main__":
    sys.exit(main())
