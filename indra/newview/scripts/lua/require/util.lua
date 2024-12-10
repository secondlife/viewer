-- utility functions, in alpha order

local util = {}

-- bind(func, args...) returns a function f such that f(moreargs) calls
-- func(args..., moreargs).
function util.bind(func, ...)
    local bound = table.pack(...)
    return function(...)
        -- What we want here is simply
        -- return func(table.unpack(bound), ...)
        -- But Lua only forwards a multi-valued expression as additional
        -- arguments if that expression is LAST in the function call argument
        -- list. Have to engage in more elaborate table manipulation.
        local args = table.pack(...)
        -- Don't modify the original bound arguments! Otherwise we'll impact
        -- all subsequent calls!
        local boundplus = table.clone(bound)
        -- Append 'args' to 'boundplus'
        table.move(args, 1, #args, #boundplus+1, boundplus)
        -- Now we can expand boundplus as the full argument list.
        return func(table.unpack(boundplus))
    end
end

-- Lua defines the special syntax table:attribute(args...) as shorthand for
-- table.attribute(table, args...). But what if we want to refer to
-- table.attribute as a free function? That is, what if we want:
-- f = table:attribute
-- ... some code ...
-- f(args) => calls table.attribute(table, args)
-- Sadly, the special syntax table:attribute doesn't return a callable that
-- implicitly binds 'table' as the first argument. table:attribute is defined
-- only for function definitions and function calls, not for expressions in
-- general.
-- bindclass(table, 'attribute') returns a function f such that f(args) calls
-- table.attribute(table, args). Note that 'attribute' must be passed as a
-- string. Otherwise we'd have to require bindclass(table, table.attribute).
-- Yes this is equivalent to bind(table.attribute, table), but much simpler.
function util.bindclass(t, attribute)
    local method = assert(t[attribute], `bindclass() class has no method '{attribute}'`)
    return function(...)
        return method(t, ...)
    end
end

-- A Lua object can be callable in either of a couple fairly verbose ways.
function util.callable(maybefunc)
    return (type(maybefunc) == 'function' or
            (type(maybefunc) == 'table' and
             getmetatable(maybefunc) and
             type(getmetatable(maybefunc).__call) == 'function'))
end

-- Check errors from pcall(), xpcall() or coroutine.resume().
-- pcall() et al. are defined to return either:
-- * true followed by all return values of the function or coroutine
-- * false with an error object (i.e. message).
-- Callers are therefore encouraged to check the initial bool. If it's false
-- they must handle the error; if true they can process the remaining return
-- values. This function encapsulates that responsibility, e.g.:
-- local a, b, c = util.callok(pcall(func, 'some args'))
function util.callok(ok, err, ...)
    if not ok then
        -- associate the error with our caller
        error(err, 2)
    end
    -- return everything except the initial bool 'ok'
    return err, ...
end

-- internal to util.class(), see usage below
local function wrapnew(ctor)
    -- Per https://www.lua.org/pil/16.1.html, ensure that a class constructor
    -- function returns an object with the class table set as the new object's
    -- metatable.
    return function(self, ...)
        -- Both new() and __call() expect initial 'self' argument, which in a
        -- MyClass:new(...) or MyClass(...) call is the class table. It's
        -- important to bind that 'self' parameter as the metatable for this
        -- class, not the current class table. If a subclass inherits its
        -- base-class constructor, calling the constructor on the subclass
        -- should bind the subclass as the metatable, not the base class.
        return setmetatable(ctor(self, ...), self)
    end
end

-- Define a "class" table.
-- Pass the class name, the class constructor (a function accepting whatever
-- arguments are appropriate) and any additional class attributes, e.g.
-- additional methods or __tostring(self).
-- Example: MyClass = util.class{
--     'MyClass',
--     function(self, _name)
--         -- In a constructor function, 'self' is the *class table*.
--         -- The constructor must create and return a new instance table.
--         return {name=_name}
--     end,
--     __tostring = function(self)
--         return `{self.classname}({self.name})`
--     end}
-- The class name is stored as classname, the constructor as new() -- and also
-- as the __call() metafunction.
-- MyClass = util.class('MyClass', function(self)...end) works too (i.e.
-- positional arguments), though in that case any additional class attributes
-- must be assigned explicitly.
-- With table-argument syntax util.class{}, the class name may be passed
-- either as the first positional argument or as classname=. Similarly, the
-- constructor may be passed either as the second positional argument or as
-- new=.
-- If base=BaseClass is passed, the new class will be a subclass of BaseClass.
-- If you omit the constructor without base=, the new object will be empty.
-- If you omit the constructor with base=, the new class inherits the base-
-- class constructor.
function util.class(classname, new)
    local nm = classname
    local ctor = new
    local args = {}
    if type(classname) == 'table' then
        assert(new == nil, 'Call util.class{...} or util.class(classname, new), not both')
        -- clone the passed table because we're going to modify it
        args = table.clone(classname)
        assert(not (args[1] and args.classname),
               "Call util.class{'Name', ...} or util.class{classname='Name', ...}, not both")
        nm = args[1] or args.classname
        args[1] = nil
        assert(not (args[2] and args.new),
               'Call util.class{name, ctor, ...} or util.class{name, new=ctor, ...}, not both')
        ctor = args[2] or args.new
        args[2] = nil
        assert((not args.base) or (type(args.base) == 'table'),
            'util.class{base=} must be a class table')
    end
    assert(type(nm) == 'string' and #nm ~= 0, 'util.class(classname) must be non-empty string')
    if ctor then
        assert(util.callable(ctor), 'util.class() constructor must be callable')
        ctor = wrapnew(ctor)
    else
        if args.base and args.base.new then
            -- Omitting the constructor inherits the base-class constructor.
            ctor = args.base.new
        else
            -- If there's no base-class constructor, just return empty table.
            ctor = wrapnew(function(self) return {} end)
        end
    end
    -- Use 'args' as our new class table, to capture any additional class
    -- attributes we were passed.
    args.classname = nm
    args.new = ctor
    -- Are we deriving this new class from an existing base class?
    if args.base then
        -- The metatable __index assignment below takes care of inheriting
        -- most class attributes. But remembering that the class table is the
        -- metatable for each instance, our base class may have __metafunctions
        -- in the class table. We expect a subclass object to share the base-
        -- class metafunctions; but Lua only looks up a __metafunction name in
        -- the immediate metatable. Copy those to the new subclass, avoiding
        -- any explicitly set by our caller.
        -- This must happen *before* we set args's metatable's __index,
        -- otherwise we think args already has every attribute set in
        -- args.base!
        for attribute, value in pairs(args.base) do
            if (string.sub(attribute, 1, 2) == '__' and #attribute > 2
                and not args[attribute]) then
                args[attribute] = value
            end
        end
    end
    -- Set ctor() as the __call() metafunction too.
    -- Also, if we're creating a subclass, set its metatable's __index to the
    -- base class so any class attributes not found in this class will be
    -- looked up in the base class.
    setmetatable(args, {__call=ctor, __index=args.base})
    -- Since 'args' is the metatable for each new instance, set its __index to
    -- the class table: if a referenced attribute isn't found on the object
    -- itself, seek it in the class.
    args.__index = args
    return args
end

-- Allow MyClass(ctor args...) equivalent to MyClass:new(ctor args...)
-- Usage:
-- local MyClass = {}
-- function MyClass:new(...)
--     ...
-- end
-- ...
-- util.classctor(MyClass)
-- or if your constructor is named something other than MyClass:new(), e.g.
-- MyClass:construct():
-- util.classctor(MyClass, MyClass.construct)
-- return MyClass
function util.classctor(class, ctor)
    -- set class's __call metamethod to the specified constructor function
    -- (class.new if not specified)
    util.setmetamethods{class, __call=(ctor or class.new)}
end

-- check if array-like table contains certain value
function util.contains(t, v)
    return table.find(t, v) ~= nil
end

-- reliable count of the number of entries in table t
-- (since #t is unreliable)
function util.count(t)
    local count = 0
    for _ in pairs(t) do
        count += 1
    end
    return count
end

-- cheap test whether table t is empty
function util.empty(t)
    return not next(t)
end

-- recursive table equality
function util.equal(t1, t2)
    if not (type(t1) == 'table' and type(t2) == 'table') then
        return t1 == t2
    end
    -- both t1 and t2 are tables: get modifiable copy of t2
    local temp = table.clone(t2)
    for k, v in pairs(t1) do
        -- if any key in t1 doesn't have same value in t2, not equal
        if not util.equal(v, temp[k]) then
            return false
        end
        -- temp[k] == t1[k], delete temp[k]
        temp[k] = nil
    end
    -- All keys in t1 have equal values in t2; t2 == t1 if there are no extra keys in t2
    return util.empty(temp)
end

-- Return an {x, y, z} table from a vector. Vectors don't implicitly convert.
function util.fromvector(v)
    if type(v) == 'table' then
        -- already a table
        return v
    elseif type(v) == 'vector' then
        -- Empirically, the only way we've found so far to extract the values of a
        -- vector is to name them individually.
        return {v.x, v.y, v.z}
    else
        error(`fromvector({type(v)}) not supported`)
    end
end

-- notfunc(function) returns a function that calls the specified function,
-- then returns the boolean inverse of its return value. But since some
-- modules distinguish nil versus non-nil callback returns, make our returned
-- function return nil for any truthy value, otherwise true. For any caller
-- that simply tests boolean truth, this is the same.
function util.notfunc(func)
    return function(...)
        if func(...) then
            return nil
        else
            return true
        end
    end
end

-- Find or create the metatable for a specified table (a new empty table if
-- omitted), and to that metatable assign the specified keys.
-- Setting multiple keys at once is more efficient than a function to set only
-- one at a time, e.g. setametamethod().
-- t = util.setmetamethods{__index=readfunc, __len=lenfunc}
-- returns a new table with specified metamethods __index, __len
-- util.setmetamethods{t, __call=action}
-- finds or creates the metatable for existing table t and sets __call
-- util.setmetamethods{table=t, __call=action}
-- same as util.setmetamethods{t, __call=action}
function util.setmetamethods(specs)
    -- first determine the target table
    assert(not (specs.table and specs[1]),
           "Pass setmetamethods table either as positional or table=, not both")
    local t = specs.table or specs[1] or {}
    -- remove both ways of specifying table, leaving only the metamethods
    specs.table = nil
    specs[1] = nil
    local mt = getmetatable(t)
    if not mt then
        -- t doesn't already have a metatable: just set specs
        setmetatable(t, specs)
    else
        -- t already has a metatable: copy specs into it
        local key, value
        for key, value in pairs(specs) do
            mt[key] = value
        end
    end
    -- having set or enriched t's metatable, return t
    return t
end

-- On the passed module (i.e. table), set an __index metamethod such that
-- referencing module.submodule lazily requires(path/submodule).
-- The loaded submodule is cached in the module table so it need not be passed
-- to require() again.
-- 'path', like any require() string, can be relative to LuaRequirePath.
-- Returns the enriched module, permitting e.g.
-- mymod = util.submoduledir({}, 'mymod')
function util.submoduledir(module, path)
    return util.setmetamethods{
        module,
        __index=function(t, key)
            local mod = require(`{path}/{key}`)
            -- cache the submodule
            t[key] = mod
            return mod
        end
    }
end

-- Return a Luau vector from (x, y, z) coordinates or an {x, y, z} table.
function util.tovector(x, y, z)
    -- you can pass an {x, y, z} table too
    if type(x) == 'table' then
        return vector.create(table.unpack(x))
    elseif type(x) == 'vector' then
        -- already a vector
        return x
    else
        return vector.create(x, y, z)
    end
end

return util
