"""\
@file compatibility.py
@brief Classes that manage compatibility states.

Copyright (c) 2007, Linden Research, Inc.
$License$
"""


"""Compatibility combination table:

    I   M   O   N   S
    --  --  --  --  --
I:  I   I   I   I   I
M:  I   M   M   M   M   
O:  I   M   O   M   O
N:  I   M   M   N   N
S:  I   M   O   N   S

"""

class _Compatibility(object):
    def __init__(self, reason):
        self.reasons = [ ]
        if reason:
            self.reasons.append(reason)
        
    def combine(self, other):
        if self._level() <= other._level():
            return self._buildclone(other)
        else:
            return other._buildclone(self)
    
    def prefix(self, leadin):
        self.reasons = [ leadin + r for r in self.reasons ] 
    
    def same(self):         return self._level() >=  1
    def deployable(self):   return self._level() >   0
    def resolved(self):     return self._level() >  -1
    def compatible(self):   return self._level() >  -2
    
    def explain(self):
        return self.__class__.__name__ + "\n" + "\n".join(self.reasons) + "\n"
        
    def _buildclone(self, other=None):
        c = self._buildinstance()
        c.reasons = self.reasons
        if other:
            c.reasons = c.reasons + other.reasons
        return c
        
    def _buildinstance(self):
        return self.__class__(None)

#    def _level(self):
#        raise RuntimeError('implement in subclass')


class Incompatible(_Compatibility):
    def _level(self):
        return -2

class Mixed(_Compatibility):
    def __init__(self, *inputs):
        _Compatibility.__init__(self, None)
        for i in inputs:
            self.reasons += i.reasons
                    
    def _buildinstance(self):
        return self.__class__()
        
    def _level(self):
        return -1

class _Aged(_Compatibility):
    def combine(self, other):
        if self._level() == other._level():
            return self._buildclone(other)
        if int(self._level()) == int(other._level()):
            return Mixed(self, other)
        return _Compatibility.combine(self, other)

class Older(_Aged):
    def _level(self):
        return -0.25
    
class Newer(_Aged):
    def _level(self):
        return 0.25

class Same(_Compatibility):
    def __init__(self):
        _Compatibility.__init__(self, None)
    
    def _buildinstance(self):
        return self.__class__()
        
    def _level(self):
        return 1




