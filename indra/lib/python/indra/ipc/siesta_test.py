from indra.base import llsd, lluuid
from indra.ipc import siesta
import datetime, math, unittest
from webob import exc


class ClassApp(object):
    def handle_get(self, req):
        pass

    def handle_post(self, req):
        return req.llsd
    

def callable_app(req):
    if req.method == 'UNDERPANTS':
        raise exc.HTTPMethodNotAllowed()
    elif req.method == 'GET':
        return None
    return req.llsd


class TestBase:
    def test_basic_get(self):
        req = siesta.Request.blank('/')
        self.assertEquals(req.get_response(self.server).body,
                          llsd.format_xml(None))
        
    def test_bad_method(self):
        req = siesta.Request.blank('/')
        req.environ['REQUEST_METHOD'] = 'UNDERPANTS'
        self.assertEquals(req.get_response(self.server).status_int,
                          exc.HTTPMethodNotAllowed.code)
        
    json_safe = {
        'none': None,
        'bool_true': True,
        'bool_false': False,
        'int_zero': 0,
        'int_max': 2147483647,
        'int_min': -2147483648,
        'long_zero': 0,
        'long_max': 2147483647L,
        'long_min': -2147483648L,
        'float_zero': 0,
        'float': math.pi,
        'float_huge': 3.14159265358979323846e299,
        'str_empty': '',
        'str': 'foo',
        u'unic\u1e51de_empty': u'',
        u'unic\u1e51de': u'\u1e4exx\u10480',
        }
    json_safe['array'] = json_safe.values()
    json_safe['tuple'] = tuple(json_safe.values())
    json_safe['dict'] = json_safe.copy()

    json_unsafe = {
        'uuid_empty': lluuid.UUID(),
        'uuid_full': lluuid.UUID('dc61ab0530200d7554d23510559102c1a98aab1b'),
        'binary_empty': llsd.binary(),
        'binary': llsd.binary('f\0\xff'),
        'uri_empty': llsd.uri(),
        'uri': llsd.uri('http://www.secondlife.com/'),
        'datetime_empty': datetime.datetime(1970,1,1),
        'datetime': datetime.datetime(1999,9,9,9,9,9),
        }
    json_unsafe.update(json_safe)
    json_unsafe['array'] = json_unsafe.values()
    json_unsafe['tuple'] = tuple(json_unsafe.values())
    json_unsafe['dict'] = json_unsafe.copy()
    json_unsafe['iter'] = iter(json_unsafe.values())

    def _test_client_content_type_good(self, content_type, ll):
        def run(ll):
            req = siesta.Request.blank('/')
            req.environ['REQUEST_METHOD'] = 'POST'
            req.content_type = content_type
            req.llsd = ll
            req.accept = content_type
            resp = req.get_response(self.server)
            self.assertEquals(resp.status_int, 200)
            return req, resp
        
        if False and isinstance(ll, dict):
            def fixup(v):
                if isinstance(v, float):
                    return '%.5f' % v
                if isinstance(v, long):
                    return int(v)
                if isinstance(v, (llsd.binary, llsd.uri)):
                    return v
                if isinstance(v, (tuple, list)):
                    return [fixup(i) for i in v]
                if isinstance(v, dict):
                    return dict([(k, fixup(i)) for k, i in v.iteritems()])
                return v
            for k, v in ll.iteritems():
                l = [k, v]
                req, resp = run(l)
                self.assertEquals(fixup(resp.llsd), fixup(l))

        run(ll)

    def test_client_content_type_json_good(self):
        self._test_client_content_type_good('application/json', self.json_safe)

    def test_client_content_type_llsd_xml_good(self):
        self._test_client_content_type_good('application/llsd+xml',
                                            self.json_unsafe)

    def test_client_content_type_llsd_notation_good(self):
        self._test_client_content_type_good('application/llsd+notation',
                                            self.json_unsafe)

    def test_client_content_type_llsd_binary_good(self):
        self._test_client_content_type_good('application/llsd+binary',
                                            self.json_unsafe)

    def test_client_content_type_xml_good(self):
        self._test_client_content_type_good('application/xml',
                                            self.json_unsafe)

    def _test_client_content_type_bad(self, content_type):
        req = siesta.Request.blank('/')
        req.environ['REQUEST_METHOD'] = 'POST'
        req.body = '\0invalid nonsense under all encodings'
        req.content_type = content_type
        self.assertEquals(req.get_response(self.server).status_int,
                          exc.HTTPBadRequest.code)
        
    def test_client_content_type_json_bad(self):
        self._test_client_content_type_bad('application/json')

    def test_client_content_type_llsd_xml_bad(self):
        self._test_client_content_type_bad('application/llsd+xml')

    def test_client_content_type_llsd_notation_bad(self):
        self._test_client_content_type_bad('application/llsd+notation')

    def test_client_content_type_llsd_binary_bad(self):
        self._test_client_content_type_bad('application/llsd+binary')

    def test_client_content_type_xml_bad(self):
        self._test_client_content_type_bad('application/xml')

    def test_client_content_type_bad(self):
        req = siesta.Request.blank('/')
        req.environ['REQUEST_METHOD'] = 'POST'
        req.body = 'XXX'
        req.content_type = 'application/nonsense'
        self.assertEquals(req.get_response(self.server).status_int,
                          exc.HTTPUnsupportedMediaType.code)

    def test_request_default_content_type(self):
        req = siesta.Request.blank('/')
        self.assertEquals(req.content_type, req.default_content_type)

    def test_request_default_accept(self):
        req = siesta.Request.blank('/')
        from webob import acceptparse
        self.assertEquals(str(req.accept).replace(' ', ''),
                          req.default_accept.replace(' ', ''))

    def test_request_llsd_auto_body(self):
        req = siesta.Request.blank('/')
        req.llsd = {'a': 2}
        self.assertEquals(req.body, '<?xml version="1.0" ?><llsd><map>'
                          '<key>a</key><integer>2</integer></map></llsd>')

    def test_request_llsd_mod_body_changes_llsd(self):
        req = siesta.Request.blank('/')
        req.llsd = {'a': 2}
        req.body = '<?xml version="1.0" ?><llsd><integer>1337</integer></llsd>'
        self.assertEquals(req.llsd, 1337)

    def test_request_bad_llsd_fails(self):
        def crashme(ctype):
            def boom():
                class foo(object): pass
                req = siesta.Request.blank('/')
                req.content_type = ctype
                req.llsd = foo()
        for mime_type in siesta.llsd_parsers:
            self.assertRaises(TypeError, crashme(mime_type))


class ClassServer(TestBase, unittest.TestCase):
    def __init__(self, *args, **kwargs):
        unittest.TestCase.__init__(self, *args, **kwargs)
        self.server = siesta.llsd_class(ClassApp)


class CallableServer(TestBase, unittest.TestCase):
    def __init__(self, *args, **kwargs):
        unittest.TestCase.__init__(self, *args, **kwargs)
        self.server = siesta.llsd_callable(callable_app)


class RouterServer(unittest.TestCase):
    def test_router(self):
        def foo(req, quux):
            print quux

        r = siesta.Router()
        r.add('/foo/{quux:int}', siesta.llsd_callable(foo), methods=['GET'])
        req = siesta.Request.blank('/foo/33')
        req.get_response(r)

        req = siesta.Request.blank('/foo/bar')
        self.assertEquals(req.get_response(r).status_int,
                          exc.HTTPNotFound.code)
    
if __name__ == '__main__':
    unittest.main()
