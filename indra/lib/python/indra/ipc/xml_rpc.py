"""\
@file xml_rpc.py
@brief An implementation of a parser/generator for the XML-RPC xml format.

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


from greenlet import greenlet

from mulib import mu

from xml.sax import handler
from xml.sax import parseString


# States
class Expected(object):
    def __init__(self, tag):
        self.tag = tag

    def __getattr__(self, name):
        return type(self)(name)

    def __repr__(self):
        return '%s(%r)' % (
            type(self).__name__, self.tag)


class START(Expected):
    pass


class END(Expected):
    pass


class STR(object):
    tag = ''


START = START('')
END = END('')


class Malformed(Exception):
    pass


class XMLParser(handler.ContentHandler):
    def __init__(self, state_machine, next_states):
        handler.ContentHandler.__init__(self)
        self.state_machine = state_machine
        if not isinstance(next_states, tuple):
            next_states = (next_states, )
        self.next_states = next_states
        self._character_buffer = ''

    def assertState(self, state, name, *rest):
        if not isinstance(self.next_states, tuple):
            self.next_states = (self.next_states, )
        for next in self.next_states:
            if type(state) == type(next):
                if next.tag and next.tag != name:
                    raise Malformed(
                        "Expected %s, got %s %s %s" % (
                            next, state, name, rest))
                break
        else:
            raise Malformed(
                "Expected %s, got %s %s %s" % (
                    self.next_states, state, name, rest))

    def startElement(self, name, attrs):
        self.assertState(START, name.lower(), attrs)
        self.next_states = self.state_machine.switch(START, (name.lower(), dict(attrs)))

    def endElement(self, name):
        if self._character_buffer.strip():
            characters = self._character_buffer.strip()
            self._character_buffer = ''
            self.assertState(STR, characters)
            self.next_states = self.state_machine.switch(characters)
        self.assertState(END, name.lower())
        self.next_states = self.state_machine.switch(END, name.lower())

    def error(self, exc):
        self.bozo = 1
        self.exc = exc

    def fatalError(self, exc):
        self.error(exc)
        raise exc

    def characters(self, characters):
        self._character_buffer += characters


def parse(what):
    child = greenlet(xml_rpc)
    me = greenlet.getcurrent()
    startup_states = child.switch(me)
    parser = XMLParser(child, startup_states)
    try:
        parseString(what, parser)
    except Malformed:
        print what
        raise
    return child.switch()


def xml_rpc(yielder):
    yielder.switch(START.methodcall)
    yielder.switch(START.methodname)
    methodName = yielder.switch(STR)
    yielder.switch(END.methodname)

    yielder.switch(START.params)

    root = None
    params = []
    while True:
        state, _ = yielder.switch(START.param, END.params)
        if state == END:
            break

        yielder.switch(START.value)
        
        params.append(
            handle(yielder))

        yielder.switch(END.value)
        yielder.switch(END.param)

    yielder.switch(END.methodcall)
    ## Resume parse
    yielder.switch()
    ## Return result to parse
    return methodName.strip(), params


def handle(yielder):
    _, (tag, attrs) = yielder.switch(START)
    if tag in ['int', 'i4']:
        result = int(yielder.switch(STR))
    elif tag == 'boolean':
        result = bool(int(yielder.switch(STR)))
    elif tag == 'string':
        result = yielder.switch(STR)
    elif tag == 'double':
        result = float(yielder.switch(STR))
    elif tag == 'datetime.iso8601':
        result = yielder.switch(STR)
    elif tag == 'base64':
        result = base64.b64decode(yielder.switch(STR))
    elif tag == 'struct':
        result = {}
        while True:
            state, _ = yielder.switch(START.member, END.struct)
            if state == END:
                break

            yielder.switch(START.name)
            key = yielder.switch(STR)
            yielder.switch(END.name)

            yielder.switch(START.value)
            result[key] = handle(yielder)
            yielder.switch(END.value)

            yielder.switch(END.member)
        ## We already handled </struct> above, don't want to handle it below
        return result
    elif tag == 'array':
        result = []
        yielder.switch(START.data)
        while True:
            state, _ = yielder.switch(START.value, END.data)
            if state == END:
                break

            result.append(handle(yielder))

            yielder.switch(END.value)

    yielder.switch(getattr(END, tag))

    return result


VALUE = mu.tag_factory('value')
BOOLEAN = mu.tag_factory('boolean')
INT = mu.tag_factory('int')
STRUCT = mu.tag_factory('struct')
MEMBER = mu.tag_factory('member')
NAME = mu.tag_factory('name')
ARRAY = mu.tag_factory('array')
DATA = mu.tag_factory('data')
STRING = mu.tag_factory('string')
DOUBLE = mu.tag_factory('double')
METHODRESPONSE = mu.tag_factory('methodResponse')
PARAMS = mu.tag_factory('params')
PARAM = mu.tag_factory('param')

mu.inline_elements['string'] = True
mu.inline_elements['boolean'] = True
mu.inline_elements['name'] = True


def _generate(something):
    if isinstance(something, dict):
        result = STRUCT()
        for key, value in something.items():
            result[
                MEMBER[
                    NAME[key], _generate(value)]]
        return VALUE[result]
    elif isinstance(something, list):
        result = DATA()
        for item in something:
            result[_generate(item)]
        return VALUE[ARRAY[[result]]]
    elif isinstance(something, basestring):
        return VALUE[STRING[something]]
    elif isinstance(something, bool):
        if something:
            return VALUE[BOOLEAN['1']]
        return VALUE[BOOLEAN['0']]
    elif isinstance(something, int):
        return VALUE[INT[something]]
    elif isinstance(something, float):
        return VALUE[DOUBLE[something]]

def generate(*args):
    params = PARAMS()
    for arg in args:
        params[PARAM[_generate(arg)]]
    return METHODRESPONSE[params]


if __name__ == '__main__':
    print parse("""<?xml version="1.0"?> <methodCall>  <methodName>examples.getStateName</methodName>  <params>  <param>  <value><i4>41</i4></value>  </param>  </params>  </methodCall>
""")
    
        
        
        
        
        
        
        
        
