"""\
@file iterators.py
@brief Useful general-purpose iterators.

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

from __future__ import nested_scopes

def iter_chunks(rows, aggregate_size=100):
    """
    Given an iterable set of items (@p rows), produces lists of up to @p
    aggregate_size items at a time, for example:
    
    iter_chunks([1,2,3,4,5,6,7,8,9,10], 3)

    Values for @p aggregate_size < 1 will raise ValueError.

    Will return a generator that produces, in the following order:
    - [1, 2, 3]
    - [4, 5, 6]
    - [7, 8, 9]
    - [10]
    """
    if aggregate_size < 1:
        raise ValueError()

    def iter_chunks_inner():
        row_iter = iter(rows)
        done = False
        agg = []
        while not done:
            try:
                row = row_iter.next()
                agg.append(row)
            except StopIteration:
                done = True
            if agg and (len(agg) >= aggregate_size or done):
                yield agg
                agg = []
    
    return iter_chunks_inner()
