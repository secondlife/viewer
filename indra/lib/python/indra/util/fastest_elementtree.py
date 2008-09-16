"""\
@file fastest_elementtree.py
@brief Concealing some gnarly import logic in here.  This should export the interface of elementtree.

$LicenseInfo:firstyear=2008&license=mit$

Copyright (c) 2008, Linden Research, Inc.

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

# The parsing exception raised by the underlying library depends
# on the ElementTree implementation we're using, so we provide an
# alias here.
#
# Use ElementTreeError as the exception type for catching parsing
# errors.


# Using cElementTree might cause some unforeseen problems, so here's a
# convenient off switch.

use_celementree = True

try:
    if not use_celementree:
        raise ImportError()
    # Python 2.3 and 2.4.
    from cElementTree import *
    ElementTreeError = SyntaxError
except ImportError:
    try:
        if not use_celementree:
            raise ImportError()
        # Python 2.5 and above.
        from xml.etree.cElementTree import *
        ElementTreeError = SyntaxError
    except ImportError:
        # Pure Python code.
        try:
            # Python 2.3 and 2.4.
            from elementtree.ElementTree import *
        except ImportError:
            # Python 2.5 and above.
            from xml.etree.ElementTree import *

        # The pure Python ElementTree module uses Expat for parsing.
        from xml.parsers.expat import ExpatError as ElementTreeError
