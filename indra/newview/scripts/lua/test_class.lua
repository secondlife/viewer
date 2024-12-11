-- Test util.class() functionality.

util = require 'util'
-- inspect = require 'inspect'

BaseClass = util.class{
    'BaseClass',
    function(self, n)
        return {name=n}
    end,
    __tostring=function(self)
        return `{self.classname}({self.name})`
    end}
function BaseClass:laugh()
    return `{self} says hee hee hee`
end

base1 = BaseClass('base1')
print('base1:', base1)
assert(tostring(base1) == 'BaseClass(base1)')

base2 = BaseClass('base2')
assert(tostring(base2) == 'BaseClass(base2)')
-- creating a second instance should not affect the first instance
assert(tostring(base1) == 'BaseClass(base1)')

ChildClass = util.class{'ChildClass', base=BaseClass}

child1 = ChildClass('oldest')
-- print(inspect{'BaseClass', BaseClass, 'ChildClass', ChildClass, 'child1', child1})
assert(tostring(child1) == 'ChildClass(oldest)')

SibClass = util.class{
    'SibClass',
    base=BaseClass,
    _id=0,
    new=function(self)
        self._id += 1
        return BaseClass(`sib{self._id}`)
    end,
    laugh=function(self)
        return `{self} says ho ho ho`
    end}

sib1 = SibClass()
sib2 = SibClass()
assert(tostring(sib1) == 'SibClass(sib1)')
assert(tostring(sib2) == 'SibClass(sib2)')

assert(base1:laugh() == 'BaseClass(base1) says hee hee hee')
assert(child1:laugh() == 'ChildClass(oldest) says hee hee hee')
assert(sib2:laugh() == 'SibClass(sib2) says ho ho ho')
