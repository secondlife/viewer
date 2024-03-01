-- from https://create.roblox.com/docs/luau/queues#implementing-queues

local Queue = {}
Queue.__index = Queue

function Queue.new()
	local self = setmetatable({}, Queue)

	self._first = 0
	self._last = -1
	self._queue = {}

	return self
end

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
	local first = self._first
	if self:IsEmpty() then
		return nil
	end
	local value = self._queue[first]
	self._queue[first] = nil
	self._first = first + 1
	return value
end

return Queue
