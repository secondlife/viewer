-- utility functions, in alpha order

local util = {}

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
