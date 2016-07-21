"""\
@file llmessage.py
@brief Message template parsing and compatiblity

$LicenseInfo:firstyear=2007&license=mit$

Copyright (c) 2007-2009, Linden Research, Inc.

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

from compatibility import Incompatible, Older, Newer, Same
from tokenstream import TokenStream

###
### Message Template
###

class Template:
    def __init__(self):
        self.messages = { }
    
    def addMessage(self, m):
        self.messages[m.name] = m
    
    def compatibleWithBase(self, base):
        messagenames = (
              frozenset(self.messages.keys())
            | frozenset(base.messages.keys())
            )
            
        compatibility = Same()
        for name in messagenames:
            selfmessage = self.messages.get(name, None)
            basemessage = base.messages.get(name, None)
            
            if not selfmessage:
                c = Older("missing message %s, did you mean to deprecate?" % name)
            elif not basemessage:
                c = Newer("added message %s" % name)
            else:
                c = selfmessage.compatibleWithBase(basemessage)
                c.prefix("in message %s: " % name)
            
            compatibility = compatibility.combine(c)
        
        return compatibility



class Message:
    HIGH = "High"
    MEDIUM = "Medium"
    LOW = "Low"
    FIXED = "Fixed"
    priorities = [ HIGH, MEDIUM, LOW, FIXED ]
    prioritieswithnumber = [ FIXED ]
    
    TRUSTED = "Trusted"
    NOTTRUSTED = "NotTrusted"
    trusts = [ TRUSTED, NOTTRUSTED ]
    
    UNENCODED = "Unencoded"
    ZEROCODED = "Zerocoded"
    encodings = [ UNENCODED, ZEROCODED ]

    NOTDEPRECATED = "NotDeprecated"
    DEPRECATED = "Deprecated"
    UDPDEPRECATED = "UDPDeprecated"
    UDPBLACKLISTED = "UDPBlackListed"
    deprecations = [ NOTDEPRECATED, UDPDEPRECATED, UDPBLACKLISTED, DEPRECATED ]
    # in order of increasing deprecation
    
    def __init__(self, name, number, priority, trust, coding):
        self.name = name
        self.number = number
        self.priority = priority
        self.trust = trust
        self.coding = coding
        self.deprecateLevel = 0
        self.blocks = [ ]

    def deprecated(self):
        return self.deprecateLevel != 0

    def deprecate(self, deprecation): 
        self.deprecateLevel = self.deprecations.index(deprecation)

    def addBlock(self, block):
        self.blocks.append(block)
        
    def compatibleWithBase(self, base):
        if self.name != base.name:
            # this should never happen in real life because of the
            # way Template matches up messages by name
            return Incompatible("has different name: %s vs. %s in base"
                               % (self.name, base.name)) 
        if self.priority != base.priority:
            return Incompatible("has different priority: %s vs. %s in base"
                                % (self.priority, base.priority)) 
        if self.trust != base.trust:
            return Incompatible("has different trust: %s vs. %s in base"
                                % (self.trust, base.trust)) 
        if self.coding != base.coding:
            return Incompatible("has different coding: %s vs. %s in base"
                                % (self.coding, base.coding)) 
        if self.number != base.number:
            return Incompatible("has different number: %s vs. %s in base"
                                % (self.number, base.number))
        
        compatibility = Same()

        if self.deprecateLevel != base.deprecateLevel:
            if self.deprecateLevel < base.deprecateLevel:
                c = Older("is less deprecated: %s vs. %s in base" % (
                    self.deprecations[self.deprecateLevel],
                    self.deprecations[base.deprecateLevel]))
            else:
                c = Newer("is more deprecated: %s vs. %s in base" % (
                    self.deprecations[self.deprecateLevel],
                    self.deprecations[base.deprecateLevel]))
            compatibility = compatibility.combine(c)
        
        selflen = len(self.blocks)
        baselen = len(base.blocks)
        samelen = min(selflen, baselen)
            
        for i in xrange(0, samelen):
            selfblock = self.blocks[i]
            baseblock = base.blocks[i]
            
            c = selfblock.compatibleWithBase(baseblock)
            if not c.same():
                c = Incompatible("block %d isn't identical" % i)
            compatibility = compatibility.combine(c)
        
        if selflen > baselen:
            c = Newer("has %d extra blocks" % (selflen - baselen))
        elif selflen < baselen:
            c = Older("missing %d extra blocks" % (baselen - selflen))
        else:
            c = Same()
        
        compatibility = compatibility.combine(c)
        return compatibility



class Block(object):
    SINGLE = "Single"
    MULTIPLE = "Multiple"
    VARIABLE = "Variable"
    repeats = [ SINGLE, MULTIPLE, VARIABLE ]
    repeatswithcount = [ MULTIPLE ]
    
    def __init__(self, name, repeat, count=None):
        self.name = name
        self.repeat = repeat
        self.count = count
        self.variables = [ ]

    def addVariable(self, variable):
        self.variables.append(variable)
        
    def compatibleWithBase(self, base):
        if self.name != base.name:
            return Incompatible("has different name: %s vs. %s in base"
                                % (self.name, base.name))
        if self.repeat != base.repeat:
            return Incompatible("has different repeat: %s vs. %s in base"
                                % (self.repeat, base.repeat))
        if self.repeat in Block.repeatswithcount:
            if self.count != base.count:
                return Incompatible("has different count: %s vs. %s in base"
                                    % (self.count, base.count)) 

        compatibility = Same()
        
        selflen = len(self.variables)
        baselen = len(base.variables)
        
        for i in xrange(0, min(selflen, baselen)):
            selfvar = self.variables[i]
            basevar = base.variables[i]
            
            c = selfvar.compatibleWithBase(basevar)
            if not c.same():
                c = Incompatible("variable %d isn't identical" % i)
            compatibility = compatibility.combine(c)

        if selflen > baselen:
            c = Newer("has %d extra variables" % (selflen - baselen))
        elif selflen < baselen:
            c = Older("missing %d extra variables" % (baselen - selflen))
        else:
            c = Same()

        compatibility = compatibility.combine(c)
        return compatibility



class Variable:
    U8 = "U8"; U16 = "U16"; U32 = "U32"; U64 = "U64"
    S8 = "S8"; S16 = "S16"; S32 = "S32"; S64 = "S64"
    F32 = "F32"; F64 = "F64"
    LLVECTOR3 = "LLVector3"; LLVECTOR3D = "LLVector3d"; LLVECTOR4 = "LLVector4"
    LLQUATERNION = "LLQuaternion"
    LLUUID = "LLUUID"
    BOOL = "BOOL"
    IPADDR = "IPADDR"; IPPORT = "IPPORT"
    FIXED = "Fixed"
    VARIABLE = "Variable"
    types = [ U8, U16, U32, U64, S8, S16, S32, S64, F32, F64,
                LLVECTOR3, LLVECTOR3D, LLVECTOR4, LLQUATERNION,
                LLUUID, BOOL, IPADDR, IPPORT, FIXED, VARIABLE ]
    typeswithsize = [ FIXED, VARIABLE ]
    
    def __init__(self, name, type, size):
        self.name = name
        self.type = type
        self.size = size
        
    def compatibleWithBase(self, base):
        if self.name != base.name:
            return Incompatible("has different name: %s vs. %s in base"
                                % (self.name, base.name))
        if self.type != base.type:
            return Incompatible("has different type: %s vs. %s in base"
                                % (self.type, base.type))
        if self.type in Variable.typeswithsize:
            if self.size != base.size:
                return Incompatible("has different size: %s vs. %s in base"
                                    % (self.size, base.size)) 
        return Same()



###
### Parsing Message Templates
###

class TemplateParser:
    def __init__(self, tokens):
        self._tokens = tokens
        self._version = 0
        self._numbers = { }
        for p in Message.priorities:
            self._numbers[p] = 0

    def parseTemplate(self):
        tokens = self._tokens
        t = Template()
        while True:
            if tokens.want("version"):
                v = float(tokens.require(tokens.wantFloat()))
                self._version = v
                t.version = v
                continue
    
            m = self.parseMessage()
            if m:
                t.addMessage(m)
                continue
            
            if self._version >= 2.0:
                tokens.require(tokens.wantEOF())
                break
            else:
                if tokens.wantEOF():
                    break
            
                tokens.consume()
                    # just assume (gulp) that this is a comment
                    # line 468: "sim -> dataserver"
        return t                


    def parseMessage(self):
        tokens = self._tokens
        if not tokens.want("{"):
            return None
        
        name     = tokens.require(tokens.wantSymbol())
        priority = tokens.require(tokens.wantOneOf(Message.priorities))
        
        if self._version >= 2.0  or  priority in Message.prioritieswithnumber:
            number = int("+" + tokens.require(tokens.wantInteger()), 0)
        else:
            self._numbers[priority] += 1
            number = self._numbers[priority]
        
        trust    = tokens.require(tokens.wantOneOf(Message.trusts))
        coding   = tokens.require(tokens.wantOneOf(Message.encodings))
    
        m = Message(name, number, priority, trust, coding)
        
        if self._version >= 2.0:
            d = tokens.wantOneOf(Message.deprecations)
            if d:
                m.deprecate(d)
                
        while True:
            b = self.parseBlock()
            if not b:
                break
            m.addBlock(b)
            
        tokens.require(tokens.want("}"))
        
        return m
    
    
    def parseBlock(self):
        tokens = self._tokens
        if not tokens.want("{"):
            return None
        name = tokens.require(tokens.wantSymbol())
        repeat = tokens.require(tokens.wantOneOf(Block.repeats))
        if repeat in Block.repeatswithcount:
            count = int(tokens.require(tokens.wantInteger()))
        else:
            count = None
    
        b = Block(name, repeat, count)
    
        while True:
            v = self.parseVariable()
            if not v:
                break
            b.addVariable(v)
            
        tokens.require(tokens.want("}"))
        return b
            
    
    def parseVariable(self):
        tokens = self._tokens
        if not tokens.want("{"):
            return None
        name = tokens.require(tokens.wantSymbol())
        type = tokens.require(tokens.wantOneOf(Variable.types))
        if type in Variable.typeswithsize:
            size = tokens.require(tokens.wantInteger())
        else:
            tokens.wantInteger() # in LandStatRequest: "{ ParcelLocalID S32 1 }" 
            size = None
        tokens.require(tokens.want("}"))
        return Variable(name, type, size)
        
def parseTemplateString(s):
    return TemplateParser(TokenStream().fromString(s)).parseTemplate()

def parseTemplateFile(f):
    return TemplateParser(TokenStream().fromFile(f)).parseTemplate()
