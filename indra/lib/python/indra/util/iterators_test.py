"""\
@file iterators_test.py
@brief Test cases for iterators module.

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

import unittest

from indra.util.iterators import iter_chunks

class TestIterChunks(unittest.TestCase):
    """Unittests for iter_chunks"""
    def test_bad_agg_size(self):
        rows = [1,2,3,4]
        self.assertRaises(ValueError, iter_chunks, rows, 0)
        self.assertRaises(ValueError, iter_chunks, rows, -1)

        try:
            for i in iter_chunks(rows, 0):
                pass
        except ValueError:
            pass
        else:
            self.fail()
        
        try:
            result = list(iter_chunks(rows, 0))
        except ValueError:
            pass
        else:
            self.fail()
    def test_empty(self):
        rows = []
        result = list(iter_chunks(rows))
        self.assertEqual(result, [])
    def test_small(self):
        rows = [[1]]
        result = list(iter_chunks(rows, 2))
        self.assertEqual(result, [[[1]]])
    def test_size(self):
        rows = [[1],[2]]
        result = list(iter_chunks(rows, 2))
        self.assertEqual(result, [[[1],[2]]])
    def test_multi_agg(self):
        rows = [[1],[2],[3],[4],[5]]
        result = list(iter_chunks(rows, 2))
        self.assertEqual(result, [[[1],[2]],[[3],[4]],[[5]]])

if __name__ == "__main__":
    unittest.main()
