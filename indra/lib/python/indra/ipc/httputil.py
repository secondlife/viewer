"""\
@file httputil.py
@brief HTTP utilities. HTTP date conversion and non-blocking HTTP
client support.

Copyright (c) 2006-2007, Linden Research, Inc.
$License$
"""


import os
import time
import urlparse

import httplib
try:
    from mx.DateTime import Parser

    parse_date = Parser.DateTimeFromString
except ImportError:
    from dateutil import parser

    parse_date = parser.parse


HTTP_TIME_FORMAT = '%a, %d %b %Y %H:%M:%S GMT'


to_http_time = lambda t: time.strftime(HTTP_TIME_FORMAT, time.gmtime(t))
from_http_time = lambda t: int(parse_date(t).gmticks())

def host_and_port_from_url(url):
    """@breif Simple function to get host and port from an http url.
    @return Returns host, port and port may be None.
    """
    host = None
    port = None
    parsed_url = urlparse.urlparse(url)
    try:
        host, port = parsed_url[1].split(':')
    except ValueError:
        host = parsed_url[1].split(':')
    return host, port


def better_putrequest(self, method, url, skip_host=0):
    self.method = method
    self.path = url
    self.old_putrequest(method, url, skip_host)


class HttpClient(httplib.HTTPConnection):
    """A subclass of httplib.HTTPConnection which works around a bug
    in the interaction between eventlet sockets and httplib. httplib relies
    on gc to close the socket, causing the socket to be closed too early.

    This is an awful hack and the bug should be fixed properly ASAP.
    """
    def __init__(self, host, port=None, strict=None):
       httplib.HTTPConnection.__init__(self, host, port, strict)

    def close(self):
        pass

    old_putrequest = httplib.HTTPConnection.putrequest
    putrequest = better_putrequest


class HttpsClient(httplib.HTTPSConnection):
    def close(self):
        pass
    old_putrequest = httplib.HTTPSConnection.putrequest
    putrequest = better_putrequest



scheme_to_factory_map = {
    'http': HttpClient,
    'https': HttpsClient,
}


def makeConnection(scheme, location, use_proxy):
    if use_proxy:
        if "http_proxy" in os.environ:
            location = os.environ["http_proxy"]
        elif "ALL_PROXY" in os.environ:
            location = os.environ["ALL_PROXY"]
        else:
            location = "localhost:3128" #default to local squid
        if location.startswith("http://"):
            location = location[len("http://"):]
    return scheme_to_factory_map[scheme](location)

