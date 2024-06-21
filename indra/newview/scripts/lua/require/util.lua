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
    -- get the metatable for the passed class
    local mt = getmetatable(class)
    if mt == nil then
        -- if it doesn't already have a metatable, then create one
        mt = {}
        setmetatable(class, mt)
    end
    -- now that class has a metatable, set its __call method to the specified
    -- constructor method (class.new if not specified)
    mt.__call = ctor or class.new
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

return util
