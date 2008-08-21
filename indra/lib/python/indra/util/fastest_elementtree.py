"""\
@file fastest_elementtree.py
@brief Concealing some gnarly import logic in here.  This should export the interface of elementtree.

$LicenseInfo:firstyear=2006&license=mit$

Copyright (c) 2006-2007, Linden Research, Inc.

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

# Using celementree might cause some unforeseen problems so here's a
# convenient off switch.
use_celementree = True

try:
    if not use_celementree:
        raise ImportError()
    from cElementTree import * ## This does not work under Windows
except ImportError:
    try:
        if not use_celementree:
            raise ImportError()
        ## This is the name of cElementTree under python 2.5
        from xml.etree.cElementTree import *
    except ImportError:
        try:
            ## This is the old name of elementtree, for use with 2.3
            from elementtree.ElementTree import *
        except ImportError:
            ## This is the name of elementtree under python 2.5
            from xml.etree.ElementTree import *

