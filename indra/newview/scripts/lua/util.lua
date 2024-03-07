-- utility functions, in alpha order

local util = {}

-- cheap test whether table t is empty
function util.empty(t)
    for _ in pairs(t) do
        return false
    end
    return true
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

-- Concatentate the strings in the passed list, return the composite string.
-- For iterative string building, the theory is that building a list with
-- table.insert() and then using join() to allocate the full-size result
-- string once should be more efficient than reallocating an intermediate
-- string for every partial concatenation.
function util.join(list, sep)
    -- This succinct implementation assumes that string.format() precomputes
    -- the required size of its output buffer before populating it. We don't
    -- know that. Moreover, this implementation predates our sep argument.
--  return string.format(string.rep('%s', #list), table.unpack(list))

    -- this implementation makes it explicit
    local sep = sep or ''
    local size = if util.empty(list) then 0 else -#sep
    for _, s in pairs(list) do
        size += #sep + #s
    end
    local result = buffer.create(size)
    size = 0
    for i, s in pairs(list) do
        if i > 1 then
            buffer.writestring(result, size, sep)
            size += #sep
        end
        buffer.writestring(result, size, s)
        size += #s
    end
    return buffer.tostring(result)
end

return util
