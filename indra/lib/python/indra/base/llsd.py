"""\
@file llsd.py
@brief Types as well as parsing and formatting functions for handling LLSD.

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

import datetime
import base64
import struct
import time
import types
import re

from indra.util.fastest_elementtree import fromstring
from indra.base import lluuid

int_regex = re.compile("[-+]?\d+")
real_regex = re.compile("[-+]?(\d+(\.\d*)?|\d*\.\d+)([eE][-+]?\d+)?")
alpha_regex = re.compile("[a-zA-Z]+")
date_regex = re.compile("(?P<year>\d{4})-(?P<month>\d{2})-(?P<day>\d{2})T(?P<hour>\d{2}):(?P<minute>\d{2}):(?P<second>\d{2})(?P<second_float>\.\d{2})?Z")
#date: d"YYYY-MM-DDTHH:MM:SS.FFZ"

class LLSDParseError(Exception):
    pass

class LLSDSerializationError(Exception):
    pass


class binary(str):
    pass

class uri(str):
    pass


BOOL_TRUE = ('1', '1.0', 'true')
BOOL_FALSE = ('0', '0.0', 'false', '')


def format_datestr(v):
    """ Formats a datetime object into the string format shared by xml and notation serializations."""
    second_str = ""
    if v.microsecond > 0:
        seconds = v.second + float(v.microsecond) / 1000000
        second_str = "%05.2f" % seconds
    else:
        second_str = "%02d" % v.second
    return '%s%sZ' % (v.strftime('%Y-%m-%dT%H:%M:'), second_str)


def parse_datestr(datestr):
    """Parses a datetime object from the string format shared by xml and notation serializations."""
    if datestr == "":
        return datetime.datetime(1970, 1, 1)
    
    match = re.match(date_regex, datestr)
    if not match:
        raise LLSDParseError("invalid date string '%s'." % datestr)
    
    year = int(match.group('year'))
    month = int(match.group('month'))
    day = int(match.group('day'))
    hour = int(match.group('hour'))
    minute = int(match.group('minute'))
    second = int(match.group('second'))
    seconds_float = match.group('second_float')
    microsecond = 0
    if seconds_float:
        microsecond = int(seconds_float[1:]) * 10000
    return datetime.datetime(year, month, day, hour, minute, second, microsecond)


def bool_to_python(node):
    val = node.text or ''
    if val in BOOL_TRUE:
        return True
    else:
        return False

def int_to_python(node):
    val = node.text or ''
    if not val.strip():
        return 0
    return int(val)

def real_to_python(node):
    val = node.text or ''
    if not val.strip():
        return 0.0
    return float(val)

def uuid_to_python(node):
    return lluuid.UUID(node.text)

def str_to_python(node):
    return unicode(node.text or '').encode('utf8', 'replace')

def bin_to_python(node):
    return binary(base64.decodestring(node.text or ''))

def date_to_python(node):
    val = node.text or ''
    if not val:
        val = "1970-01-01T00:00:00Z"
    return parse_datestr(val)

def uri_to_python(node):
    val = node.text or ''
    if not val:
        return None
    return uri(val)

def map_to_python(node):
    result = {}
    for index in range(len(node))[::2]:
        result[node[index].text] = to_python(node[index+1])
    return result

def array_to_python(node):
    return [to_python(child) for child in node]


NODE_HANDLERS = dict(
    undef=lambda x: None,
    boolean=bool_to_python,
    integer=int_to_python,
    real=real_to_python,
    uuid=uuid_to_python,
    string=str_to_python,
    binary=bin_to_python,
    date=date_to_python,
    uri=uri_to_python,
    map=map_to_python,
    array=array_to_python,
    )

def to_python(node):
    return NODE_HANDLERS[node.tag](node)

class Nothing(object):
    pass


class LLSDXMLFormatter(object):
    def __init__(self):
        self.type_map = {
            type(None) : self.UNDEF,
            bool : self.BOOLEAN,
            int : self.INTEGER,
            long : self.INTEGER,
            float : self.REAL,
            lluuid.UUID : self.UUID,
            binary : self.BINARY,
            str : self.STRING,
            unicode : self.STRING,
            uri : self.URI,
            datetime.datetime : self.DATE,
            list : self.ARRAY,
            tuple : self.ARRAY,
            types.GeneratorType : self.ARRAY,
            dict : self.MAP,
            LLSD : self.LLSD
        }

    def elt(self, name, contents=None):
        if(contents is None or contents is ''):
            return "<%s />" % (name,)
        else:
            return "<%s>%s</%s>" % (name, contents, name)

    def xml_esc(self, v):
        return v.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')

    def LLSD(self, v):
        return self.generate(v.thing)
    def UNDEF(self, v):
        return self.elt('undef')
    def BOOLEAN(self, v):
        if v:
            return self.elt('boolean', 'true')
        else:
            return self.elt('boolean', 'false')
    def INTEGER(self, v):
        return self.elt('integer', v)
    def REAL(self, v):
        return self.elt('real', v)
    def UUID(self, v):
        if(v.isNull()):
            return self.elt('uuid')
        else:
            return self.elt('uuid', v)
    def BINARY(self, v):
        return self.elt('binary', base64.encodestring(v))
    def STRING(self, v):
        return self.elt('string', self.xml_esc(v))
    def URI(self, v):
        return self.elt('uri', self.xml_esc(str(v)))
    def DATE(self, v):
        return self.elt('date', format_datestr(v))
    def ARRAY(self, v):
        return self.elt('array', ''.join([self.generate(item) for item in v]))
    def MAP(self, v):
        return self.elt(
            'map',
            ''.join(["%s%s" % (self.elt('key', key), self.generate(value))
             for key, value in v.items()]))

    typeof = type
    def generate(self, something):
        t = self.typeof(something)
        if self.type_map.has_key(t):
            return self.type_map[t](something)
        else:
            raise LLSDSerializationError("Cannot serialize unknown type: %s (%s)" % (
                t, something))

    def format(self, something):
        return '<?xml version="1.0" ?>' + self.elt("llsd", self.generate(something))

_g_xml_formatter = None
def format_xml(something):
    global _g_xml_formatter
    if _g_xml_formatter is None:
        _g_xml_formatter = LLSDXMLFormatter()
    return _g_xml_formatter.format(something)

class LLSDXMLPrettyFormatter(LLSDXMLFormatter):
    def __init__(self, indent_atom = None):
        # Call the super class constructor so that we have the type map
        super(LLSDXMLPrettyFormatter, self).__init__()

        # Override the type map to use our specialized formatters to
        # emit the pretty output.
        self.type_map[list] = self.PRETTY_ARRAY
        self.type_map[tuple] = self.PRETTY_ARRAY
        self.type_map[types.GeneratorType] = self.PRETTY_ARRAY,
        self.type_map[dict] = self.PRETTY_MAP

        # Private data used for indentation.
        self._indent_level = 1
        if indent_atom is None:
            self._indent_atom = '  '
        else:
            self._indent_atom = indent_atom

    def _indent(self):
        "Return an indentation based on the atom and indentation level."
        return self._indent_atom * self._indent_level

    def PRETTY_ARRAY(self, v):
        rv = []
        rv.append('<array>\n')
        self._indent_level = self._indent_level + 1
        rv.extend(["%s%s\n" %
                   (self._indent(),
                    self.generate(item))
                   for item in v])
        self._indent_level = self._indent_level - 1
        rv.append(self._indent())
        rv.append('</array>')
        return ''.join(rv)

    def PRETTY_MAP(self, v):
        rv = []
        rv.append('<map>\n')
        self._indent_level = self._indent_level + 1
        keys = v.keys()
        keys.sort()
        rv.extend(["%s%s\n%s%s\n" %
                   (self._indent(),
                    self.elt('key', key),
                    self._indent(),
                    self.generate(v[key]))
                   for key in keys])
        self._indent_level = self._indent_level - 1
        rv.append(self._indent())
        rv.append('</map>')
        return ''.join(rv)

    def format(self, something):
        data = []
        data.append('<?xml version="1.0" ?>\n<llsd>')
        data.append(self.generate(something))
        data.append('</llsd>\n')
        return '\n'.join(data)

def format_pretty_xml(something):
    """@brief Serialize a python object as 'pretty' llsd xml.

    The output conforms to the LLSD DTD, unlike the output from the
    standard python xml.dom DOM::toprettyxml() method which does not
    preserve significant whitespace. 
    This function is not necessarily suited for serializing very large
    objects. It is not optimized by the cllsd module, and sorts on
    dict (llsd map) keys alphabetically to ease human reading.
    """
    return LLSDXMLPrettyFormatter().format(something)

class LLSDNotationFormatter(object):
    def __init__(self):
        self.type_map = {
            type(None) : self.UNDEF,
            bool : self.BOOLEAN,
            int : self.INTEGER,
            long : self.INTEGER,
            float : self.REAL,
            lluuid.UUID : self.UUID,
            binary : self.BINARY,
            str : self.STRING,
            unicode : self.STRING,
            uri : self.URI,
            datetime.datetime : self.DATE,
            list : self.ARRAY,
            tuple : self.ARRAY,
            types.GeneratorType : self.ARRAY,
            dict : self.MAP,
            LLSD : self.LLSD
        }

    def LLSD(self, v):
        return self.generate(v.thing)
    def UNDEF(self, v):
        return '!'
    def BOOLEAN(self, v):
        if v:
            return 'true'
        else:
            return 'false'
    def INTEGER(self, v):
        return "i%s" % v
    def REAL(self, v):
        return "r%s" % v
    def UUID(self, v):
        return "u%s" % v
    def BINARY(self, v):
        raise LLSDSerializationError("binary notation not yet supported")
    def STRING(self, v):
        return "'%s'" % v.replace("\\", "\\\\").replace("'", "\\'")
    def URI(self, v):
        return 'l"%s"' % str(v).replace("\\", "\\\\").replace('"', '\\"')
    def DATE(self, v):
        return 'd"%s"' % format_datestr(v)
    def ARRAY(self, v):
        return "[%s]" % ','.join([self.generate(item) for item in v])
    def MAP(self, v):
        return "{%s}" % ','.join(["'%s':%s" % (key.replace("\\", "\\\\").replace("'", "\\'"), self.generate(value))
             for key, value in v.items()])

    def generate(self, something):
        t = type(something)
        if self.type_map.has_key(t):
            return self.type_map[t](something)
        else:
            raise LLSDSerializationError("Cannot serialize unknown type: %s (%s)" % (
                t, something))

    def format(self, something):
        return self.generate(something)

def format_notation(something):
    return LLSDNotationFormatter().format(something)

def _hex_as_nybble(hex):
    if (hex >= '0') and (hex <= '9'):
        return ord(hex) - ord('0')
    elif (hex >= 'a') and (hex <='f'):
        return 10 + ord(hex) - ord('a')
    elif (hex >= 'A') and (hex <='F'):
        return 10 + ord(hex) - ord('A');

class LLSDBinaryParser(object):
    def __init__(self):
        pass

    def parse(self, buffer, ignore_binary = False):
        """
        This is the basic public interface for parsing.

        @param buffer the binary data to parse in an indexable sequence.
        @param ignore_binary parser throws away data in llsd binary nodes.
        @return returns a python object.
        """
        self._buffer = buffer
        self._index = 0
        self._keep_binary = not ignore_binary
        return self._parse()

    def _parse(self):
        cc = self._buffer[self._index]
        self._index += 1
        if cc == '{':
            return self._parse_map()
        elif cc == '[':
            return self._parse_array()
        elif cc == '!':
            return None
        elif cc == '0':
            return False
        elif cc == '1':
            return True
        elif cc == 'i':
            # 'i' = integer
            idx = self._index
            self._index += 4
            return struct.unpack("!i", self._buffer[idx:idx+4])[0]
        elif cc == ('r'):
            # 'r' = real number
            idx = self._index
            self._index += 8
            return struct.unpack("!d", self._buffer[idx:idx+8])[0]
        elif cc == 'u':
            # 'u' = uuid
            idx = self._index
            self._index += 16
            return lluuid.uuid_bits_to_uuid(self._buffer[idx:idx+16])
        elif cc == 's':
            # 's' = string
            return self._parse_string()
        elif cc in ("'", '"'):
            # delimited/escaped string
            return self._parse_string_delim(cc)
        elif cc == 'l':
            # 'l' = uri
            return uri(self._parse_string())
        elif cc == ('d'):
            # 'd' = date in seconds since epoch
            idx = self._index
            self._index += 8
            seconds = struct.unpack("!d", self._buffer[idx:idx+8])[0]
            return datetime.datetime.fromtimestamp(seconds)
        elif cc == 'b':
            binary = self._parse_string()
            if self._keep_binary:
                return binary
            # *NOTE: maybe have a binary placeholder which has the
            # length.
            return None
        else:
            raise LLSDParseError("invalid binary token at byte %d: %d" % (
                self._index - 1, ord(cc)))

    def _parse_map(self):
        rv = {}
        size = struct.unpack("!i", self._buffer[self._index:self._index+4])[0]
        self._index += 4
        count = 0
        cc = self._buffer[self._index]
        self._index += 1
        key = ''
        while (cc != '}') and (count < size):
            if cc == 'k':
                key = self._parse_string()
            elif cc in ("'", '"'):
                key = self._parse_string_delim(cc)
            else:
                raise LLSDParseError("invalid map key at byte %d." % (
                    self._index - 1,))
            value = self._parse()
            #print "kv:",key,value
            rv[key] = value
            count += 1
            cc = self._buffer[self._index]
            self._index += 1
        if cc != '}':
            raise LLSDParseError("invalid map close token at byte %d." % (
                self._index,))
        return rv

    def _parse_array(self):
        rv = []
        size = struct.unpack("!i", self._buffer[self._index:self._index+4])[0]
        self._index += 4
        count = 0
        cc = self._buffer[self._index]
        while (cc != ']') and (count < size):
            rv.append(self._parse())
            count += 1
            cc = self._buffer[self._index]
        if cc != ']':
            raise LLSDParseError("invalid array close token at byte %d." % (
                self._index,))
        self._index += 1
        return rv

    def _parse_string(self):
        size = struct.unpack("!i", self._buffer[self._index:self._index+4])[0]
        self._index += 4
        rv = self._buffer[self._index:self._index+size]
        self._index += size
        return rv

    def _parse_string_delim(self, delim):
        list = []
        found_escape = False
        found_hex = False
        found_digit = False
        byte = 0
        while True:
            cc = self._buffer[self._index]
            self._index += 1
            if found_escape:
                if found_hex:
                    if found_digit:
                        found_escape = False
                        found_hex = False
                        found_digit = False
                        byte <<= 4
                        byte |= _hex_as_nybble(cc)
                        list.append(chr(byte))
                        byte = 0
                    else:
                        found_digit = True
                        byte = _hex_as_nybble(cc)
                elif cc == 'x':
                    found_hex = True
                else:
                    if cc == 'a':
                        list.append('\a')
                    elif cc == 'b':
                        list.append('\b')
                    elif cc == 'f':
                        list.append('\f')
                    elif cc == 'n':
                        list.append('\n')
                    elif cc == 'r':
                        list.append('\r')
                    elif cc == 't':
                        list.append('\t')
                    elif cc == 'v':
                        list.append('\v')
                    else:
                        list.append(cc)
                    found_escape = False
            elif cc == '\\':
                found_escape = True
            elif cc == delim:
                break
            else:
                list.append(cc)
        return ''.join(list)

class LLSDNotationParser(object):
    """ Parse LLSD notation:
    map: { string:object, string:object }
    array: [ object, object, object ]
    undef: !
    boolean: true | false | 1 | 0 | T | F | t | f | TRUE | FALSE
    integer: i####
    real: r####
    uuid: u####
    string: "g\'day" | 'have a "nice" day' | s(size)"raw data"
    uri: l"escaped"
    date: d"YYYY-MM-DDTHH:MM:SS.FFZ"
    binary: b##"ff3120ab1" | b(size)"raw data"
    """
    def __init__(self):
        pass

    def parse(self, buffer, ignore_binary = False):
        """
        This is the basic public interface for parsing.

        @param buffer the notation string to parse.
        @param ignore_binary parser throws away data in llsd binary nodes.
        @return returns a python object.
        """
        if buffer == "":
            return False

        self._buffer = buffer
        self._index = 0
        return self._parse()

    def _parse(self):
        cc = self._buffer[self._index]
        self._index += 1
        if cc == '{':
            return self._parse_map()
        elif cc == '[':
            return self._parse_array()
        elif cc == '!':
            return None
        elif cc == '0':
            return False
        elif cc == '1':
            return True
        elif cc in ('F', 'f'):
            self._skip_alpha()
            return False
        elif cc in ('T', 't'):
            self._skip_alpha()
            return True
        elif cc == 'i':
            # 'i' = integer
            return self._parse_integer()
        elif cc == ('r'):
            # 'r' = real number
            return self._parse_real()
        elif cc == 'u':
            # 'u' = uuid
            return self._parse_uuid()
        elif cc in ("'", '"', 's'):
            return self._parse_string(cc)
        elif cc == 'l':
            # 'l' = uri
            delim = self._buffer[self._index]
            self._index += 1
            val = uri(self._parse_string(delim))
            if len(val) == 0:
                return None
            return val
        elif cc == ('d'):
            # 'd' = date in seconds since epoch
            return self._parse_date()
        elif cc == 'b':
            raise LLSDParseError("binary notation not yet supported")
        else:
            raise LLSDParseError("invalid token at index %d: %d" % (
                self._index - 1, ord(cc)))

    def _parse_map(self):
        """ map: { string:object, string:object } """
        rv = {}
        cc = self._buffer[self._index]
        self._index += 1
        key = ''
        found_key = False
        while (cc != '}'):
            if not found_key:
                if cc in ("'", '"', 's'):
                    key = self._parse_string(cc)
                    found_key = True
                    #print "key:",key
                elif cc.isspace() or cc == ',':
                    cc = self._buffer[self._index]
                    self._index += 1
                else:
                    raise LLSDParseError("invalid map key at byte %d." % (
                                        self._index - 1,))
            else:
                if cc.isspace() or cc == ':':
                    #print "skipping whitespace '%s'" % cc
                    cc = self._buffer[self._index]
                    self._index += 1
                    continue
                self._index += 1
                value = self._parse()
                #print "kv:",key,value
                rv[key] = value
                found_key = False
                cc = self._buffer[self._index]
                self._index += 1
                #if cc == '}':
                #    break
                #cc = self._buffer[self._index]
                #self._index += 1

        return rv

    def _parse_array(self):
        """ array: [ object, object, object ] """
        rv = []
        cc = self._buffer[self._index]
        while (cc != ']'):
            if cc.isspace() or cc == ',':
                self._index += 1
                cc = self._buffer[self._index]
                continue
            rv.append(self._parse())
            cc = self._buffer[self._index]

        if cc != ']':
            raise LLSDParseError("invalid array close token at index %d." % (
                self._index,))
        self._index += 1
        return rv

    def _parse_uuid(self):
        match = re.match(lluuid.UUID.uuid_regex, self._buffer[self._index:])
        if not match:
            raise LLSDParseError("invalid uuid token at index %d." % self._index)

        (start, end) = match.span()
        start += self._index
        end += self._index
        self._index = end
        return lluuid.UUID(self._buffer[start:end])

    def _skip_alpha(self):
        match = re.match(alpha_regex, self._buffer[self._index:])
        if match:
            self._index += match.end()
            
    def _parse_date(self):
        delim = self._buffer[self._index]
        self._index += 1
        datestr = self._parse_string(delim)
        return parse_datestr(datestr)

    def _parse_real(self):
        match = re.match(real_regex, self._buffer[self._index:])
        if not match:
            raise LLSDParseError("invalid real token at index %d." % self._index)

        (start, end) = match.span()
        start += self._index
        end += self._index
        self._index = end
        return float( self._buffer[start:end] )

    def _parse_integer(self):
        match = re.match(int_regex, self._buffer[self._index:])
        if not match:
            raise LLSDParseError("invalid integer token at index %d." % self._index)

        (start, end) = match.span()
        start += self._index
        end += self._index
        self._index = end
        return int( self._buffer[start:end] )

    def _parse_string(self, delim):
        """ string: "g\'day" | 'have a "nice" day' | s(size)"raw data" """
        rv = ""

        if delim in ("'", '"'):
            rv = self._parse_string_delim(delim)
        elif delim == 's':
            rv = self._parse_string_raw()
        else:
            raise LLSDParseError("invalid string token at index %d." % self._index)

        return rv


    def _parse_string_delim(self, delim):
        """ string: "g'day 'un" | 'have a "nice" day' """
        list = []
        found_escape = False
        found_hex = False
        found_digit = False
        byte = 0
        while True:
            cc = self._buffer[self._index]
            self._index += 1
            if found_escape:
                if found_hex:
                    if found_digit:
                        found_escape = False
                        found_hex = False
                        found_digit = False
                        byte <<= 4
                        byte |= _hex_as_nybble(cc)
                        list.append(chr(byte))
                        byte = 0
                    else:
                        found_digit = True
                        byte = _hex_as_nybble(cc)
                elif cc == 'x':
                    found_hex = True
                else:
                    if cc == 'a':
                        list.append('\a')
                    elif cc == 'b':
                        list.append('\b')
                    elif cc == 'f':
                        list.append('\f')
                    elif cc == 'n':
                        list.append('\n')
                    elif cc == 'r':
                        list.append('\r')
                    elif cc == 't':
                        list.append('\t')
                    elif cc == 'v':
                        list.append('\v')
                    else:
                        list.append(cc)
                    found_escape = False
            elif cc == '\\':
                found_escape = True
            elif cc == delim:
                break
            else:
                list.append(cc)
        return ''.join(list)

    def _parse_string_raw(self):
        """ string: s(size)"raw data" """
        # Read the (size) portion.
        cc = self._buffer[self._index]
        self._index += 1
        if cc != '(':
            raise LLSDParseError("invalid string token at index %d." % self._index)

        rparen = self._buffer.find(')', self._index)
        if rparen == -1:
            raise LLSDParseError("invalid string token at index %d." % self._index)

        size = int(self._buffer[self._index:rparen])

        self._index = rparen + 1
        delim = self._buffer[self._index]
        self._index += 1
        if delim not in ("'", '"'):
            raise LLSDParseError("invalid string token at index %d." % self._index)

        rv = self._buffer[self._index:(self._index + size)]
        self._index += size
        cc = self._buffer[self._index]
        self._index += 1
        if cc != delim:
            raise LLSDParseError("invalid string token at index %d." % self._index)

        return rv
        
def format_binary(something):
    return '<?llsd/binary?>\n' + _format_binary_recurse(something)

def _format_binary_recurse(something):
    if something is None:
        return '!'
    elif isinstance(something, LLSD):
        return _format_binary_recurse(something.thing)
    elif isinstance(something, bool):
        if something:
            return '1'
        else:
            return '0'
    elif isinstance(something, (int, long)):
        return 'i' + struct.pack('!i', something)
    elif isinstance(something, float):
        return 'r' + struct.pack('!d', something)
    elif isinstance(something, lluuid.UUID):
        return 'u' + something._bits
    elif isinstance(something, binary):
        return 'b' + struct.pack('!i', len(something)) + something
    elif isinstance(something, (str, unicode)):
        return 's' + struct.pack('!i', len(something)) + something
    elif isinstance(something, uri):
        return 'l' + struct.pack('!i', len(something)) + something
    elif isinstance(something, datetime.datetime):
        seconds_since_epoch = time.mktime(something.timetuple())
        return 'd' + struct.pack('!d', seconds_since_epoch)
    elif isinstance(something, (list, tuple)):
        array_builder = []
        array_builder.append('[' + struct.pack('!i', len(something)))
        for item in something:
            array_builder.append(_format_binary_recurse(item))
        array_builder.append(']')
        return ''.join(array_builder)
    elif isinstance(something, dict):
        map_builder = []
        map_builder.append('{' + struct.pack('!i', len(something)))
        for key, value in something.items():
            map_builder.append('k' + struct.pack('!i', len(key)) + key)
            map_builder.append(_format_binary_recurse(value))
        map_builder.append('}')
        return ''.join(map_builder)
    else:
        raise LLSDSerializationError("Cannot serialize unknown type: %s (%s)" % (
            type(something), something))


def parse(something):
    try:
        if something.startswith('<?llsd/binary?>'):
            just_binary = something.split('\n', 1)[1]
            return LLSDBinaryParser().parse(just_binary)
        # This should be better.
        elif something.startswith('<'):
            return to_python(fromstring(something)[0])
        else:
            return LLSDNotationParser().parse(something)
    except KeyError, e:
        raise Exception('LLSD could not be parsed: %s' % (e,))

class LLSD(object):
    def __init__(self, thing=None):
        self.thing = thing

    def __str__(self):
        return self.toXML(self.thing)

    parse = staticmethod(parse)
    toXML = staticmethod(format_xml)
    toPrettyXML = staticmethod(format_pretty_xml)
    toBinary = staticmethod(format_binary)
    toNotation = staticmethod(format_notation)


undef = LLSD(None)

# register converters for llsd in mulib, if it is available
try:
    from mulib import stacked, mu
    stacked.NoProducer()  # just to exercise stacked
    mu.safe_load(None)    # just to exercise mu
except:
    # mulib not available, don't print an error message since this is normal
    pass
else:
    mu.add_parser(parse, 'application/llsd+xml')
    mu.add_parser(parse, 'application/llsd+binary')

    def llsd_convert_xml(llsd_stuff, request):
        request.write(format_xml(llsd_stuff))

    def llsd_convert_binary(llsd_stuff, request):
        request.write(format_binary(llsd_stuff))

    for typ in [LLSD, dict, list, tuple, str, int, float, bool, unicode, type(None)]:
        stacked.add_producer(typ, llsd_convert_xml, 'application/llsd+xml')
        stacked.add_producer(typ, llsd_convert_xml, 'application/xml')
        stacked.add_producer(typ, llsd_convert_xml, 'text/xml')

        stacked.add_producer(typ, llsd_convert_binary, 'application/llsd+binary')

    stacked.add_producer(LLSD, llsd_convert_xml, '*/*')
