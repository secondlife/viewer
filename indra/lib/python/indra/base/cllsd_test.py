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
