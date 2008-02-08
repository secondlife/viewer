"""\
@file llsubprocess.py
@author Phoenix
@date 2008-01-18
@brief The simplest possible wrapper for a common sub-process paradigm.

$LicenseInfo:firstyear=2007&license=mit$

Copyright (c) 2007, Linden Research, Inc.

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

import os
import popen2
import time
import select

class Timeout(RuntimeError):
    "Exception raised when a subprocess times out."
    pass

def run(command, args=None, data=None, timeout=None):
    """\
@brief Run command with arguments

This is it. This is the function I want to run all the time when doing
subprocces, but end up copying the code everywhere. none of the
standard commands are secure and provide a way to specify input, get
all the output, and get the result.
@param command A string specifying a process to launch.
@param args Arguments to be passed to command. Must be list, tuple or None.
@param data input to feed to the command.
@param timeout Maximum number of many seconds to run.
@return Returns (result, stdout, stderr) from process.
"""
    cmd = [command]
    if args:
        cmd.extend([str(arg) for arg in args])
    #print  "cmd: ","' '".join(cmd)
    child = popen2.Popen3(cmd, True)
    #print child.pid
    out = []
    err = []
    result = -1
    time_left = timeout
    tochild = [child.tochild.fileno()]
    while True:
        time_start = time.time()
        #print "time:",time_left
        p_in, p_out, p_err = select.select(
            [child.fromchild.fileno(), child.childerr.fileno()],
            tochild,
            [],
            time_left)
        if p_in:
            new_line = os.read(child.fromchild.fileno(), 32 * 1024)
            if new_line:
                #print "line:",new_line
                out.append(new_line)
            new_line = os.read(child.childerr.fileno(), 32 * 1024)
            if new_line:
                #print "error:", new_line
                err.append(new_line)
        if p_out:
            if data:
                #print "p_out"
                bytes = os.write(child.tochild.fileno(), data)
                data = data[bytes:]
                if len(data) == 0:
                    data = None
                    tochild = []
                    child.tochild.close()
        result = child.poll()
        if result != -1:
            child.tochild.close()
            child.fromchild.close()
            child.childerr.close()
            break
        if time_left is not None:
            time_left -= (time.time() - time_start)
            if time_left < 0:
                raise Timeout
    #print "result:",result
    out = ''.join(out)
    #print "stdout:", out
    err = ''.join(err)
    #print "stderr:", err
    return result, out, err
