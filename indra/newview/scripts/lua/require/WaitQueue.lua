-- WaitQueue isa Queue with the added feature that when the queue is empty,
-- the Dequeue() operation blocks the calling coroutine until some other
-- coroutine Enqueue()s a new value.

local fiber = require('fiber')
local Queue = require('Queue')
local util  = require('util')

local function dbg(...) end
-- local dbg = require('printf')

local WaitQueue = Queue()

function WaitQueue:new()
    local obj = Queue()
    setmetatable(obj, self)
    self.__index = self

    obj._waiters = {}
    obj._closed = false
    return obj
end

util.classctor(WaitQueue)

function WaitQueue:Enqueue(value)
    if self._closed then
        error("can't Enqueue() on closed Queue")
    end
    -- can't simply call Queue:Enqueue(value)! That calls the method on the
    -- Queue class definition, instead of calling Queue:Enqueue() on self.
    -- Hand-expand the Queue:Enqueue() syntactic sugar.
    Queue.Enqueue(self, value)
    self:_wake_waiters()
end

function WaitQueue:_wake_waiters()
    -- WaitQueue is designed to support multi-producer, multi-consumer use
    -- cases. With multiple consumers, if more than one is trying to
    -- Dequeue() from an empty WaitQueue, we'll have multiple waiters.
    -- Unlike OS threads, with cooperative concurrency it doesn't make sense
    -- to "notify all": we need wake only one of the waiting Dequeue()
    -- callers.
    if ((not self:IsEmpty()) or self._closed) and next(self._waiters) then
        -- Pop the oldest waiting coroutine instead of the most recent, for
        -- more-or-less round robin fairness. But skip any coroutines that
        -- have gone dead in the meantime.
        local waiter = table.remove(self._waiters, 1)
        while waiter and fiber.status(waiter) == "dead" do
            waiter = table.remove(self._waiters, 1)
        end
        -- do we still have at least one waiting coroutine?
        if waiter then
            -- don't pass the head item: let the resumed coroutine retrieve it
            fiber.wake(waiter)
        end
    end
end

function WaitQueue:Dequeue()
    while self:IsEmpty() do
        -- Don't check for closed until the queue is empty: producer can close
        -- the queue while there are still items left, and we want the
        -- consumer(s) to retrieve those last few items.
        if self._closed then
            dbg('WaitQueue:Dequeue(): closed')
            return nil
        end
        dbg('WaitQueue:Dequeue(): waiting')
        -- add the running coroutine to the list of waiters
        dbg('WaitQueue:Dequeue() running %s', tostring(coroutine.running() or 'main'))
        table.insert(self._waiters, fiber.running())
        -- then let somebody else run
        fiber.wait()
    end
    -- here we're sure this queue isn't empty
    dbg('WaitQueue:Dequeue() calling Queue.Dequeue()')
    return Queue.Dequeue(self)
end

function WaitQueue:close()
    self._closed = true
    -- close() is like Enqueueing an end marker. If there are waiting
    -- consumers, give them a chance to see we're closed.
    self:_wake_waiters()
end

return WaitQueue
