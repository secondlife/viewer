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
try:
    import syslog
except ImportError:
    # Windows
    import sys
    class syslog(object):
        # wrap to a lame syslog for windows
        _logfp = sys.stderr
        def syslog(msg):
            _logfp.write(msg)
            if not msg.endswith('\n'):
                _logfp.write('\n')
        syslog = staticmethod(syslog)

from indra.base.llsd import format_notation

def record_metrics(table, stats):
    "Write a standard metrics log"
    _log("LLMETRICS", table, stats)

def record_event(table, data):
    "Write a standard logmessage log"
    _log("LLLOGMESSAGE", table, data)

def set_destination(dest):
    """Set the destination of metrics logs for this process.

    If you do not call this function prior to calling a logging
    method, that function will open sys.stdout as a destination.
    Attempts to set dest to None will throw a RuntimeError.
    @param dest a file-like object which will be the destination for logs."""
    if dest is None:
        raise RuntimeError("Attempt to unset metrics destination.")
    global _destination
    _destination = dest

def destination():
    """Get the destination of the metrics logs for this process.
    Returns None if no destination is set"""
    global _destination
    return _destination

class SysLogger(object):
    "A file-like object which writes to syslog."
    def __init__(self, ident='indra', logopt = None, facility = None):
        try:
            if logopt is None:
                logopt = syslog.LOG_CONS | syslog.LOG_PID
            if facility is None:
                facility = syslog.LOG_LOCAL0
            syslog.openlog(ident, logopt, facility)
            import atexit
            atexit.register(syslog.closelog)
        except AttributeError:
            # No syslog module on Windows
            pass

    def write(str):
        syslog.syslog(str)
    write = staticmethod(write)

    def flush():
        pass
    flush = staticmethod(flush)

#
# internal API
#
_sequence_id = 0
_destination = None

def _next_id():
    global _sequence_id
    next = _sequence_id
    _sequence_id += 1
    return next

def _dest():
    global _destination
    if _destination is None:
        # this default behavior is documented in the metrics functions above.
        _destination = sys.stdout
    return _destination
    
def _log(header, table, data):
    log_line = "%s (%d) %s %s" \
               % (header, _next_id(), table, format_notation(data))
    dest = _dest()
    dest.write(log_line)
    dest.flush()
