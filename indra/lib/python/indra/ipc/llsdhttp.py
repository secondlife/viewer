"""\
@file llsdhttp.py
@brief Functions to ease moving llsd over http
 
Copyright (c) 2006-2007, Linden Research, Inc.
$License$
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
    llsd_body = llsd.parse(bodY)
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
