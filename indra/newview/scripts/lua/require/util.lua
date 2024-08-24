-- utility functions, in alpha order

local util = {}

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

return util
