#!/usr/bin/python
## 
## $LicenseInfo:firstyear=2011&license=viewerlgpl$
## Second Life Viewer Source Code
## Copyright (C) 2011, Linden Research, Inc.
## 
## This library is free software; you can redistribute it and/or
## modify it under the terms of the GNU Lesser General Public
## License as published by the Free Software Foundation;
## version 2.1 of the License only.
## 
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## Lesser General Public License for more details.
## 
## You should have received a copy of the GNU Lesser General Public
## License along with this library; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
## 
## Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
## $/LicenseInfo$
from indra.base import llsd, lluuid
from datetime import datetime
import cllsd
import time, sys

class myint(int):
    pass

values = (
    '&<>',
    u'\u81acj',
    llsd.uri('http://foo<'),
    lluuid.UUID(),
    llsd.LLSD(['thing']),
    1,
    myint(31337),
    sys.maxint + 10,
    llsd.binary('foo'),
    [],
    {},
    {u'f&\u1212': 3},
    3.1,
    True,
    None,
    datetime.fromtimestamp(time.time()),
    )

def valuator(values):
    for v in values:
        yield v

longvalues = () # (values, list(values), iter(values), valuator(values))

for v in values + longvalues:
    print '%r => %r' % (v, cllsd.llsd_to_xml(v))

a = [[{'a':3}]] * 1000000

s = time.time()
print hash(cllsd.llsd_to_xml(a))
e = time.time()
t1 = e - s
print t1

s = time.time()
print hash(llsd.LLSDXMLFormatter()._format(a))
e = time.time()
t2 = e - s
print t2

print 'Speedup:', t2 / t1
