from indra.base import llsd
from webob import exc
import webob
import re, socket

try:
    from cStringIO import StringIO
except ImportError:
    from StringIO import StringIO

try:
    import cjson
    json_decode = cjson.decode
    json_encode = cjson.encode
    JsonDecodeError = cjson.DecodeError
    JsonEncodeError = cjson.EncodeError
except ImportError:
    import simplejson
    json_decode = simplejson.loads
    json_encode = simplejson.dumps
    JsonDecodeError = ValueError
    JsonEncodeError = TypeError


llsd_parsers = {
    'application/json': json_decode,
    'application/llsd+binary': llsd.parse_binary,
    'application/llsd+notation': llsd.parse_notation,
    'application/llsd+xml': llsd.parse_xml,
    'application/xml': llsd.parse_xml,
    }


def mime_type(content_type):
    '''Given a Content-Type header, return only the MIME type.'''

    return content_type.split(';', 1)[0].strip().lower()
    
class BodyLLSD(object):
    '''Give a webob Request or Response an llsd property.

    Getting the llsd property parses the body, and caches the result.

    Setting the llsd property formats a payload, and the body property
    is set.'''

    def _llsd__get(self):
        '''Get, set, or delete the LLSD value stored in this object.'''

        try:
            return self._llsd
        except AttributeError:
            if not self.body:
                raise AttributeError('No llsd attribute has been set')
            else:
                mtype = mime_type(self.content_type)
                try:
                    parser = llsd_parsers[mtype]
                except KeyError:
                    raise exc.HTTPUnsupportedMediaType(
                        'Content type %s not supported' % mtype).exception
                try:
                    self._llsd = parser(self.body)
                except (llsd.LLSDParseError, JsonDecodeError, TypeError), err:
                    raise exc.HTTPBadRequest(
                        'Could not parse body: %r' % err.args).exception
            return self._llsd

    def _llsd__set(self, val):
        req = getattr(self, 'request', None)
        if req is not None:
            formatter, ctype = formatter_for_request(req)
            self.content_type = ctype
        else:
            formatter, ctype = formatter_for_mime_type(
                mime_type(self.content_type))
        self.body = formatter(val)

    def _llsd__del(self):
        if hasattr(self, '_llsd'):
            del self._llsd

    llsd = property(_llsd__get, _llsd__set, _llsd__del)


class Response(webob.Response, BodyLLSD):
    '''Response class with LLSD support.

    A sensible default content type is used.

    Setting the llsd property also sets the body.  Getting the llsd
    property parses the body if necessary.

    If you set the body property directly, the llsd property will be
    deleted.'''

    default_content_type = 'application/llsd+xml'

    def _body__set(self, body):
        if hasattr(self, '_llsd'):
            del self._llsd
        super(Response, self)._body__set(body)

    def cache_forever(self):
        self.cache_expires(86400 * 365)

    body = property(webob.Response._body__get, _body__set,
                    webob.Response._body__del,
                    webob.Response._body__get.__doc__)


class Request(webob.Request, BodyLLSD):
    '''Request class with LLSD support.

    Sensible content type and accept headers are used by default.

    Setting the llsd property also sets the body.  Getting the llsd
    property parses the body if necessary.

    If you set the body property directly, the llsd property will be
    deleted.'''
    
    default_content_type = 'application/llsd+xml'
    default_accept = ('application/llsd+xml; q=0.5, '
                      'application/llsd+notation; q=0.3, '
                      'application/llsd+binary; q=0.2, '
                      'application/xml; q=0.1, '
                      'application/json; q=0.0')

    def __init__(self, environ=None, *args, **kwargs):
        if environ is None:
            environ = {}
        else:
            environ = environ.copy()
        if 'CONTENT_TYPE' not in environ:
            environ['CONTENT_TYPE'] = self.default_content_type
        if 'HTTP_ACCEPT' not in environ:
            environ['HTTP_ACCEPT'] = self.default_accept
        super(Request, self).__init__(environ, *args, **kwargs)

    def _body__set(self, body):
        if hasattr(self, '_llsd'):
            del self._llsd
        super(Request, self)._body__set(body)

    def path_urljoin(self, *parts):
        return '/'.join([path_url.rstrip('/')] + list(parts))

    body = property(webob.Request._body__get, _body__set,
                    webob.Request._body__del, webob.Request._body__get.__doc__)

    def create_response(self, llsd=None, status='200 OK',
                        conditional_response=webob.NoDefault):
        resp = self.ResponseClass(status=status, request=self,
                                  conditional_response=conditional_response)
        resp.llsd = llsd
        return resp

    def curl(self):
        '''Create and fill out a pycurl easy object from this request.'''
 
        import pycurl
        c = pycurl.Curl()
        c.setopt(pycurl.URL, self.url())
        if self.headers:
            c.setopt(pycurl.HTTPHEADER,
                     ['%s: %s' % (k, self.headers[k]) for k in self.headers])
        c.setopt(pycurl.FOLLOWLOCATION, True)
        c.setopt(pycurl.AUTOREFERER, True)
        c.setopt(pycurl.MAXREDIRS, 16)
        c.setopt(pycurl.NOSIGNAL, True)
        c.setopt(pycurl.READFUNCTION, self.body_file.read)
        c.setopt(pycurl.SSL_VERIFYHOST, 2)
        
        if self.method == 'POST':
            c.setopt(pycurl.POST, True)
            post301 = getattr(pycurl, 'POST301', None)
            if post301 is not None:
                # Added in libcurl 7.17.1.
                c.setopt(post301, True)
        elif self.method == 'PUT':
            c.setopt(pycurl.PUT, True)
        elif self.method != 'GET':
            c.setopt(pycurl.CUSTOMREQUEST, self.method)
        return c

Request.ResponseClass = Response
Response.RequestClass = Request


llsd_formatters = {
    'application/json': json_encode,
    'application/llsd+binary': llsd.format_binary,
    'application/llsd+notation': llsd.format_notation,
    'application/llsd+xml': llsd.format_xml,
    'application/xml': llsd.format_xml,
    }


def formatter_for_mime_type(mime_type):
    '''Return a formatter that encodes to the given MIME type.

    The result is a pair of function and MIME type.'''

    try:
        return llsd_formatters[mime_type], mime_type
    except KeyError:
        raise exc.HTTPInternalServerError(
            'Could not use MIME type %r to format response' %
            mime_type).exception


def formatter_for_request(req):
    '''Return a formatter that encodes to the preferred type of the client.

    The result is a pair of function and actual MIME type.'''

    for ctype in req.accept.best_matches('application/llsd+xml'):
        try:
            return llsd_formatters[ctype], ctype
        except KeyError:
            pass
    else:
        raise exc.HTTPNotAcceptable().exception


def wsgi_adapter(func, environ, start_response):
    '''Adapt a Siesta callable to act as a WSGI application.'''

    try:
        req = Request(environ)
        resp = func(req, **req.urlvars)
        if not isinstance(resp, webob.Response):
            try:
                formatter, ctype = formatter_for_request(req)
                resp = req.ResponseClass(formatter(resp), content_type=ctype)
                resp._llsd = resp
            except (JsonEncodeError, TypeError), err:
                resp = exc.HTTPInternalServerError(
                    detail='Could not format response')
    except exc.HTTPException, e:
        resp = e
    except socket.error, e:
        resp = exc.HTTPInternalServerError(detail=e.args[1])
    return resp(environ, start_response)


def llsd_callable(func):
    '''Turn a callable into a Siesta application.'''

    def replacement(environ, start_response):
        return wsgi_adapter(func, environ, start_response)

    return replacement


def llsd_method(http_method, func):
    def replacement(environ, start_response):
        if environ['REQUEST_METHOD'] == http_method:
            return wsgi_adapter(func, environ, start_response)
        return exc.HTTPMethodNotAllowed()(environ, start_response)

    return replacement


http11_methods = 'OPTIONS GET HEAD POST PUT DELETE TRACE CONNECT'.split()
http11_methods.sort()

def llsd_class(cls):
    '''Turn a class into a Siesta application.

    A new instance is created for each request.  A HTTP method FOO is
    turned into a call to the handle_foo method of the instance.'''

    def foo(req, **kwargs):
        instance = cls()
        method = req.method.lower()
        try:
            handler = getattr(instance, 'handle_' + method)
        except AttributeError:
            allowed = [m for m in http11_methods
                       if hasattr(instance, 'handle_' + m.lower())]
            raise exc.HTTPMethodNotAllowed(
                headers={'Allowed': ', '.join(allowed)}).exception
        return handler(req, **kwargs)

    def replacement(environ, start_response):
        return wsgi_adapter(foo, environ, start_response)

    return replacement


def curl(reqs):
    import pycurl

    m = pycurl.CurlMulti()
    curls = [r.curl() for r in reqs]
    io = {}
    for c in curls:
        fp = StringIO()
        hdr = StringIO()
        c.setopt(pycurl.WRITEFUNCTION, fp.write)
        c.setopt(pycurl.HEADERFUNCTION, hdr.write)
        io[id(c)] = fp, hdr
    m.handles = curls
    try:
        while True:
            ret, num_handles = m.perform()
            if ret != pycurl.E_CALL_MULTI_PERFORM:
                break
    finally:
        m.close()

    for req, c in zip(reqs, curls):
        fp, hdr = io[id(c)]
        hdr.seek(0)
        status = hdr.readline().rstrip()
        headers = []
        name, values = None, None

        # XXX We don't currently handle bogus header data.

        for line in hdr.readlines():
            if not line[0].isspace():
                if name:
                    headers.append((name, ' '.join(values)))
                name, value = line.strip().split(':', 1)
                value = [value]
            else:
                values.append(line.strip())
        if name:
            headers.append((name, ' '.join(values)))

        resp = c.ResponseClass(fp.getvalue(), status, headers, request=req)


route_re = re.compile(r'''
    \{                 # exact character "{"
    (\w+)              # variable name (restricted to a-z, 0-9, _)
    (?:([:~])([^}]+))? # optional :type or ~regex part
    \}                 # exact character "}"
    ''', re.VERBOSE)

predefined_regexps = {
    'uuid': r'[a-f0-9][a-f0-9-]{31,35}',
    'int': r'\d+',
    }

def compile_route(route):
    fp = StringIO()
    last_pos = 0
    for match in route_re.finditer(route):
        fp.write(re.escape(route[last_pos:match.start()]))
        var_name = match.group(1)
        sep = match.group(2)
        expr = match.group(3)
        if expr:
            if sep == ':':
                expr = predefined_regexps[expr]
            # otherwise, treat what follows '~' as a regexp
        else:
            expr = '[^/]+'
        expr = '(?P<%s>%s)' % (var_name, expr)
        fp.write(expr)
        last_pos = match.end()
    fp.write(re.escape(route[last_pos:]))
    return '^%s$' % fp.getvalue()

class Router(object):
    '''WSGI routing class.  Parses a URL and hands off a request to
    some other WSGI application.  If no suitable application is found,
    responds with a 404.'''

    def __init__(self):
        self.routes = []
        self.paths = []

    def add(self, route, app, methods=None):
        self.paths.append(route)
        self.routes.append((re.compile(compile_route(route)), app,
                            methods and dict.fromkeys(methods)))

    def __call__(self, environ, start_response):
        path_info = environ['PATH_INFO']
        request_method = environ['REQUEST_METHOD']
        allowed = []
        for regex, app, methods in self.routes:
            m = regex.match(path_info)
            if m:
                if not methods or request_method in methods:
                    environ['paste.urlvars'] = m.groupdict()
                    return app(environ, start_response)
                else:
                    allowed += methods
        if allowed:
            allowed = dict.fromkeys(allows).keys()
            allowed.sort()
            resp = exc.HTTPMethodNotAllowed(
                headers={'Allowed': ', '.join(allowed)})
        else:
            resp = exc.HTTPNotFound()
        return resp(environ, start_response)
