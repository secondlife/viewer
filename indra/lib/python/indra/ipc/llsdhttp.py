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


get, put, delete, post = httpc.make_suite(
    llsd.format_xml, llsd.parse, 'application/xml+llsd')


for x in (httpc.ConnectionError, httpc.NotFound, httpc.Forbidden):
    globals()[x.__name__] = x


def postFile(url, filename, verbose=False):
    f = open(filename)
    body = f.read()
    f.close()
    llsd_body = llsd.parse(body)
    return post(url, llsd_body, verbose=verbose)


def getStatus(url, use_proxy=False):
    status, _headers, _body = get(url, use_proxy=use_proxy, verbose=True)
    return status


def putStatus(url, data):
    status, _headers, _body = put(url, data, verbose=True)
    return status


def deleteStatus(url):
    status, _headers, _body = delete(url, verbose=True)
    return status


def postStatus(url, data):
    status, _headers, _body = post(url, data, verbose=True)
    return status


def postFileStatus(url, filename):
    status, _headers, body = postFile(url, filename, verbose=True)
    return status, body


def getFromSimulator(path, use_proxy=False):
    return get('http://' + simulatorHostAndPort + path, use_proxy=use_proxy)


def postToSimulator(path, data=None):
    return post('http://' + simulatorHostAndPort + path, data)
