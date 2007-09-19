"""\
@file llsdhttp.py
@brief Functions to ease moving llsd over http
 
Copyright (c) 2006-2007, Linden Research, Inc.
$License$
"""
 
import os.path
import os
import urlparse


from eventlet import httpc as httputil



from indra.base import llsd
LLSD = llsd.LLSD()


class ConnectionError(Exception):
    def __init__(self, method, host, port, path, status, reason, body):
        self.method = method
        self.host = host
        self.port = port
        self.path = path
        self.status = status
        self.reason = reason
        self.body = body
        Exception.__init__(self)

    def __str__(self):
        return "%s(%r, %r, %r, %r, %r, %r, %r)" % (
            self.__class__.__name__,
            self.method, self.host, self.port,
            self.path, self.status, self.reason, self.body)


class NotFound(ConnectionError):
    pass

class Forbidden(ConnectionError):
    pass

class MalformedResponse(ConnectionError):
    pass

class NotImplemented(ConnectionError):
    pass

def runConnection(connection, raise_parse_errors=True):
    response = connection.getresponse()
    if (response.status not in [200, 201, 204]):
        klass = {404:NotFound,
                 403:Forbidden,
                 501:NotImplemented}.get(response.status, ConnectionError)
        raise klass(
            connection.method, connection.host, connection.port,
            connection.path, response.status, response.reason, response.read())
    content_length = response.getheader('content-length')

    if content_length: # Check to see whether it is not None
        content_length = int(content_length)

    if content_length: # Check to see whether the length is not 0
        body = response.read(content_length)
    else:
        body = ''

    if not body.strip():
        #print "No body:", `body`
        return None
    try:
        return LLSD.parse(body)
    except Exception, e:
        if raise_parse_errors:
            print "Exception: %s, Could not parse LLSD: " % (e), body
            raise MalformedResponse(
            connection.method, connection.host, connection.port,
            connection.path, response.status, response.reason, body)
        else:
            return None

class FileScheme(object):
    """Retarded scheme to local file wrapper."""
    host = '<file>'
    port = '<file>'
    reason = '<none>'

    def __init__(self, location):
        pass

    def request(self, method, fullpath, body='', headers=None):
        self.status = 200
        self.path = fullpath.split('?')[0]
        self.method = method = method.lower()
        assert method in ('get', 'put', 'delete')
        if method == 'delete':
            try:
                os.remove(self.path)
            except OSError:
                pass  # don't complain if already deleted
        elif method == 'put':
            try:
                f = file(self.path, 'w')
                f.write(body)
                f.close()
            except IOError, e:
                self.status = 500
                self.raise_connection_error()
        elif method == 'get':
            if not os.path.exists(self.path):
                self.status = 404
                self.raise_connection_error(NotFound)

    def connect(self):
        pass

    def getresponse(self):
        return self

    def getheader(self, header):
        if header == 'content-length':
            try:
                return os.path.getsize(self.path)
            except OSError:
                return 0

    def read(self, howmuch=None):
        if self.method == 'get':
            try:
                return file(self.path, 'r').read(howmuch)
            except IOError:
                self.status = 500
                self.raise_connection_error()
        return ''

    def raise_connection_error(self, klass=ConnectionError):
        raise klass(
            self.method, self.host, self.port,
            self.path, self.status, self.reason, '')

scheme_to_factory_map = {
    'http': httputil.HttpClient,
    'https': httputil.HttpsClient,
    'file': FileScheme,
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


def get(url, headers=None, use_proxy=False):
    if headers is None:
        headers = {}
    scheme, location, path, params, query, id = urlparse.urlparse(url)
    connection = makeConnection(scheme, location, use_proxy=use_proxy)
    fullpath = path
    if query:
        fullpath += "?"+query
    connection.connect()
    if headers is None:
        headers=dict()
    if use_proxy:
        headers.update({ "Host" : location })
        connection.request("GET", url, headers=headers)
    else:
        connection.request("GET", fullpath, headers=headers)

    return runConnection(connection)

def put(url, data, headers=None):
    body = LLSD.toXML(data)
    scheme, location, path, params, query, id = urlparse.urlparse(url)
    connection = makeConnection(scheme, location, use_proxy=False)
    if headers is None:
        headers = {}
    headers.update({'Content-Type': 'application/xml'})
    fullpath = path
    if query:
        fullpath += "?"+query
    connection.request("PUT", fullpath, body, headers)
    return runConnection(connection, raise_parse_errors=False)

def delete(url):
    scheme, location, path, params, query, id = urlparse.urlparse(url)
    connection = makeConnection(scheme, location, use_proxy=False)
    connection.request("DELETE", path+"?"+query)
    return runConnection(connection, raise_parse_errors=False)

def post(url, data=None):
    body = LLSD.toXML(data)
    scheme, location, path, params, query, id = urlparse.urlparse(url)
    connection = makeConnection(scheme, location, use_proxy=False)
    connection.request("POST", path+"?"+query, body, {'Content-Type': 'application/xml'})
    return runConnection(connection)

def postFile(url, filename):
    f = open(filename)
    body = f.read()
    f.close()
    scheme, location, path, params, query, id = urlparse.urlparse(url)
    connection = makeConnection(scheme, location, use_proxy=False)
    connection.request("POST", path+"?"+query, body, {'Content-Type': 'application/octet-stream'})
    return runConnection(connection)

def getStatus(url, use_proxy=False):
    scheme, location, path, params, query, id = urlparse.urlparse(url)
    connection = makeConnection(scheme, location, use_proxy=use_proxy)
    if use_proxy:
        connection.request("GET", url, headers={ "Host" : location })
    else:
        connection.request("GET", path+"?"+query)
    return connection.getresponse().status

def putStatus(url, data):
    body = LLSD.toXML(data)
    scheme, location, path, params, query, id = urlparse.urlparse(url)
    connection = makeConnection(scheme, location, use_proxy=False)
    connection.request("PUT", path+"?"+query, body, {'Content-Type': 'application/xml'})
    return connection.getresponse().status

def deleteStatus(url):
    scheme, location, path, params, query, id = urlparse.urlparse(url)
    connection = makeConnection(scheme, location, use_proxy=False)
    connection.request("DELETE", path+"?"+query)
    return connection.getresponse().status

def postStatus(url, data):
    body = LLSD.toXML(data)
    scheme, location, path, params, query, id = urlparse.urlparse(url)
    connection = makeConnection(scheme, location, use_proxy=False)
    connection.request("POST", path+"?"+query, body)
    return connection.getresponse().status

def postFileStatus(url, filename):
    f = open(filename)
    body = f.read()
    f.close()
    scheme, location, path, params, query, id = urlparse.urlparse(url)
    connection = makeConnection(scheme, location, use_proxy=False)
    connection.request("POST", path+"?"+query, body, {'Content-Type': 'application/octet-stream'})
    response = connection.getresponse()
    return response.status, response.read()

def getFromSimulator(path, use_proxy=False):
    return get('http://' + simulatorHostAndPort + path, use_proxy=use_proxy)

def postToSimulator(path, data=None):
    return post('http://' + simulatorHostAndPort + path, data)
