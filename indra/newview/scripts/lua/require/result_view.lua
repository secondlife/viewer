local leap = require 'leap'

-- metatable for every result_view() table
local mt = {
    __len = function(self)
        return self.length
    end,
    __index = function(self, i)
        -- right away, convert to 0-relative indexing
        i -= 1
        -- can we find this index within the current slice?
        local reli = i - self.start
        if 0 <= reli and reli < #self.slice then
            -- Lua 1-relative indexing
            return self.slice[reli + 1]
        end
        -- is this index outside the overall result set?
        if not (0 <= i and i < self.length) then
            return nil
        end
        -- fetch a new slice starting at i, using provided fetch()
        local start
        self.slice, start = self.fetch(self.key, i)
        -- It's possible that caller-provided fetch() function forgot
        -- to return the adjusted start index of the new slice. In
        -- Lua, 0 tests as true, so if fetch() returned (slice, 0),
        -- we'll duly reset self.start to 0. Otherwise, assume the
        -- requested index was not adjusted: that the returned slice
        -- really does start at i.
        self.start = start or i
        -- Hopefully this slice contains the desired i.
        -- Back to 1-relative indexing.
        return self.slice[i - self.start + 1]
    end,
    -- We purposely avoid putting any array entries (int keys) into
    -- our table so that access to any int key will always call our
    -- __index() metamethod. Moreover, we want any table iteration to
    -- call __index(table, i) however many times; we do NOT want it to
    -- retrieve key, length, start, slice.
    -- So turn 'for k, v in result' into 'for k, v in ipairs(result)'.
    __iter = ipairs,
    -- This result set provides read-only access.
    -- We do not support pushing updates to individual items back to
    -- C++; for the intended use cases, that makes no sense.
    __newindex = function(self, i, value)
        error("result_view is a read-only data structure", 2)
    end
}

-- result_view(key_length, fetch) returns a table which stores only a slice
-- of a result set plus some control values, yet presents read-only virtual
-- access to the entire result set.
-- key_length: {result set key, total result set length}
-- fetch:      function(key, start) that returns (slice, adjusted start) 
local result_view = setmetatable(
    {
        -- generic fetch() function
        fetch = function(key, start)
            local fetched = leap.request(
                'LLInventory',
                {op='getSlice', result=key, index=start})
            return fetched.slice, fetched.start
        end,
        -- generic close() function accepting variadic result-set keys
        close = function(...)
            local keys = table.pack(...)
            -- table.pack() produces a table with an array entry for every
            -- parameter, PLUS an 'n' key with the count. Unfortunately that
            -- 'n' key bollixes our conversion to LLSD, which requires either
            -- all int keys (for an array) or all string keys (for a map).
            keys.n = nil
            leap.send('LLInventory', {op='closeResult', result=keys})
        end
    },
    {
        -- result_view(key_length, fetch) calls this
        __call = function(class, key_length, fetch)
            return setmetatable(
                {
                    key=key_length[1],
                    length=key_length[2],
                    -- C++ result sets use 0-based indexing, so internally we do too
                    start=0,
                    -- start with a dummy array with length 0
                    slice={},
                    -- if caller didn't pass fetch() function, use generic
                    fetch=fetch or class.fetch,
                    -- returned view:close() will close result set with passed key
                    close=function(self) class.close(key_length[1]) end
                },
                -- use our special metatable
                mt
            )
        end
    }
)

return result_view
