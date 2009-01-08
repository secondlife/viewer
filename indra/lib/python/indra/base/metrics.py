"""\
@file metrics.py
@author Phoenix
@date 2007-11-27
@brief simple interface for logging metrics

$LicenseInfo:firstyear=2007&license=mit$

Copyright (c) 2007-2009, Linden Research, Inc.

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

import sys
from indra.base import llsd

_sequence_id = 0

def record_metrics(table, stats, dest=None):
    "Write a standard metrics log"
    _log("LLMETRICS", table, stats, dest)

def record_event(table, data, dest=None):
    "Write a standard logmessage log"
    _log("LLLOGMESSAGE", table, data, dest)

def _log(header, table, data, dest):
    if dest is None:
        # do this check here in case sys.stdout changes at some
        # point. as a default parameter, it will never be
        # re-evaluated.
        dest = sys.stdout
    global _sequence_id
    print >>dest, header, "(" + str(_sequence_id) + ")",
    print >>dest, table, llsd.format_notation(data)
    _sequence_id += 1
