-- WaitQueue isa Queue with the added feature that when the queue is empty,
-- the Dequeue() operation blocks the calling coroutine until some other
-- coroutine Enqueue()s a new value.

local Queue = require('Queue')

WaitQueue = Queue:new()

function WaitQueue:new()
    local obj = setmetatable(Queue:new(), self)
    self.__index = self
    obj._waiters = {}
    obj._closed = false
    return obj
end

function WaitQueue:Enqueue(value)
    if self._closed then
        error("can't Enqueue() on closed Queue")
    end
    Queue:Enqueue(value)
    -- WaitQueue is designed to support multi-producer, multi-consumer use
    -- cases. With multiple consumers, if more than one is trying to
    -- Dequeue() from an empty WaitQueue, we'll have multiple waiters.
    -- Unlike OS threads, with cooperative concurrency it doesn't make sense
    -- to "notify all": we need resume only one of the waiting Dequeue()
    -- callers. But since resuming that caller might entail either Enqueue()
    -- or Dequeue() calls, recheck every time around to see if we must resume
    -- another waiting coroutine.
    while not self:IsEmpty() and #self._waiters do
        -- pop the oldest waiting coroutine instead of the most recent, for
        -- more-or-less round robin fairness
        local waiter = table.remove(self._waiters, 1)
        -- don't pass the head item: let the resumed coroutine retrieve it
        local ok, message = coroutine.resume(waiter)
        -- if resuming that waiter encountered an error, don't swallow it
        if not ok then
            error(message)
        end
    end
end

function WaitQueue:Dequeue()
    while self:IsEmpty() do
        -- Don't check for closed until the queue is empty: producer can close
        -- the queue while there are still items left, and we want the
        -- consumer(s) to retrieve those last few items.
        if self._closed then
            return nil
        end
        local coro = coroutine.running()
        if coro == nil then
            error("WaitQueue:Dequeue() trying to suspend main coroutine")
        end
        -- add the running coroutine to the list of waiters
        table.insert(self._waiters, coro)
        -- then let somebody else run
        coroutine.yield()
    end
    -- here we're sure this queue isn't empty
    return Queue:Dequeue()
end

function WaitQueue:close()
    self._closed = true
end

return WaitQueue
