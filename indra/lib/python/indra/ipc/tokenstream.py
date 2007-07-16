"""\
@file tokenstream.py
@brief Message template parsing utility class

Copyright (c) 2007, Linden Research, Inc.
$License$
"""

import re

class _EOF(object):
    pass

EOF = _EOF()

class _LineMarker(int):
    pass
    
_commentRE = re.compile(r'//.*')
_symbolRE = re.compile(r'[a-zA-Z_][a-zA-Z_0-9]*')
_integerRE = re.compile(r'(0x[0-9A-Fa-f]+|0\d*|[1-9]\d*)')
_floatRE = re.compile(r'\d+(\.\d*)?')


class ParseError(Exception):
    def __init__(self, stream, reason):
        self.line = stream.line
        self.context = stream._context()
        self.reason = reason

    def _contextString(self):    
        c = [ ]
        for t in self.context:
            if isinstance(t, _LineMarker):
                break
            c.append(t)
        return " ".join(c)

    def __str__(self):
        return "line %d: %s @ ... %s" % (
            self.line, self.reason, self._contextString())

    def __nonzero__(self):
        return False


def _optionText(options):
    n = len(options)
    if n == 1:
        return '"%s"' % options[0]
    return '"' + '", "'.join(options[0:(n-1)]) + '" or "' + options[-1] + '"'


class TokenStream(object):
    def __init__(self):
        self.line = 0
        self.tokens = [ ]
    
    def fromString(self, string):
        return self.fromLines(string.split('\n'))
        
    def fromFile(self, file):
        return self.fromLines(file)

    def fromLines(self, lines):
        i = 0
        for line in lines:
            i += 1
            self.tokens.append(_LineMarker(i))
            self.tokens.extend(_commentRE.sub(" ", line).split())
        self._consumeLines()
        return self
    
    def consume(self):
        if not self.tokens:
            return EOF
        t = self.tokens.pop(0)
        self._consumeLines()
        return t
    
    def _consumeLines(self):
        while self.tokens and isinstance(self.tokens[0], _LineMarker):
            self.line = self.tokens.pop(0)
    
    def peek(self):
        if not self.tokens:
            return EOF
        return self.tokens[0]
            
    def want(self, t):
        if t == self.peek():
            return self.consume()
        return ParseError(self, 'expected "%s"' % t)

    def wantOneOf(self, options):
        assert len(options)
        if self.peek() in options:
            return self.consume()
        return ParseError(self, 'expected one of %s' % _optionText(options))

    def wantEOF(self):
        return self.want(EOF)
        
    def wantRE(self, re, message=None):
        t = self.peek()
        if t != EOF:
            m = re.match(t)
            if m and m.end() == len(t):
                return self.consume()
        if not message:
            message = "expected match for r'%s'" % re.pattern
        return ParseError(self, message)
    
    def wantSymbol(self):
        return self.wantRE(_symbolRE, "expected symbol")
    
    def wantInteger(self):
        return self.wantRE(_integerRE, "expected integer")
    
    def wantFloat(self):
        return self.wantRE(_floatRE, "expected float")
    
    def _context(self):
        n = min(5, len(self.tokens))
        return self.tokens[0:n]

    def require(self, t):
        if t:
            return t
        if isinstance(t, ParseError):
            raise t
        else:
            raise ParseError(self, "unmet requirement")

