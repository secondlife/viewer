-- from https://create.roblox.com/docs/luau/queues#implementing-queues,
-- amended per https://www.lua.org/pil/16.1.html

-- While coding some scripting in Lua
-- I found that I needed a queua
-- I thought of linked list
-- But had to resist
-- For fear it might be too obscua. 

local util = require 'util'

local Queue = {}

function Queue:new()
    local obj = setmetatable({}, self)
    self.__index = self

    obj._first = 0
    obj._last = -1
    obj._queue = {}

    return obj
end

util.classctor(Queue)

-- Check if the queue is empty
function Queue:IsEmpty()
    return self._first > self._last
end

-- Add a value to the queue
function Queue:Enqueue(value)
    local last = self._last + 1
    self._last = last
    self._queue[last] = value
end

-- Remove a value from the queue
function Queue:Dequeue()
    if self:IsEmpty() then
        return nil
    end
    local first = self._first
    local value = self._queue[first]
    self._queue[first] = nil
    self._first = first + 1
    return value
end

return Queue
