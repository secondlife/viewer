-- ErrorQueue isa WaitQueue with the added feature that a producer can push an
-- error through the queue. When that error is dequeued, the consumer will
-- throw that error.

local WaitQueue = require('WaitQueue')

ErrorQueue = WaitQueue:new()

function ErrorQueue:Enqueue(value)
    -- normal value, not error
    WaitQueue:Enqueue({ false, value })
end

function ErrorQueue:Error(message)
    -- flag this entry as an error message
    WaitQueue:Enqueue({ true, message })
end

function ErrorQueue:Dequeue()
    local errflag, value = table.unpack(WaitQueue:Dequeue())
    if errflag == nil then
        -- queue has been closed, tell caller
        return nil
    end
    if errflag then
        -- 'value' is a message pushed by Error()
        error(value)
    end
    return value
end

return ErrorQueue
