"""\
@file llsdhttp.py
@brief Functions to ease moving llsd over http
 
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
 
import os.path
import os
import urlparse

from indra.base import llsd

from eventlet import httpc

suite = httpc.HttpSuite(llsd.format_xml, llsd.parse, 'application/llsd+xml')
delete = suite.delete
delete_ = suite.delete_
get = suite.get
get_ = suite.get_
head = suite.head
head_ = suite.head_
post = suite.post
post_ = suite.post_
put = suite.put
put_ = suite.put_
request = suite.request
request_ = suite.request_

# import every httpc error exception into our namespace for convenience
for x in httpc.status_to_error_map.itervalues():
    globals()[x.__name__] = x

for x in (httpc.ConnectionError,):
    globals()[x.__name__] = x


def postFile(url, filename):
    f = open(filename)
    body = f.read()
    f.close()
    llsd_body = llsd.parse(body)
    return post_(url, llsd_body)


# deprecated in favor of get_
def getStatus(url, use_proxy=False):
    status, _headers, _body = get_(url, use_proxy=use_proxy)
    return status

# deprecated in favor of put_
def putStatus(url, data):
    status, _headers, _body = put_(url, data)
    return status

# deprecated in favor of delete_
def deleteStatus(url):
    status, _headers, _body = delete_(url)
    return status

# deprecated in favor of post_
def postStatus(url, data):
    status, _headers, _body = post_(url, data)
    return status


def postFileStatus(url, filename):
    status, _headers, body = postFile(url, filename)
    return status, body


def getFromSimulator(path, use_proxy=False):
    return get('http://' + simulatorHostAndPort + path, use_proxy=use_proxy)


def postToSimulator(path, data=None):
    return post('http://' + simulatorHostAndPort + path, data)
