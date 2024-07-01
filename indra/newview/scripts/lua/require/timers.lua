-- Access to the viewer's time-delay facilities

local leap = require 'leap'
local util = require 'util'

local timers = {}

local function dbg(...) end
-- local dbg = require 'printf'

timers.Timer = {}

-- delay:    time in seconds until callback
-- callback: 'wait', or function to call when timer fires (self:tick if nil)
-- iterate:  if non-nil, call callback repeatedly until it returns non-nil
--           (ignored if 'wait')
function timers.Timer:new(delay, callback, iterate)
    local obj = setmetatable({}, self)
    self.__index = self

    if callback == 'wait' then
        dbg('scheduleAfter(%d):', delay)
        sequence = leap.generate('Timers', {op='scheduleAfter', after=delay})
        -- ignore the immediate return
        dbg('scheduleAfter(%d) -> %s', delay,
        sequence.next())
        -- this call is where we wait for real
        dbg('next():')
        dbg('next() -> %s',
        sequence.next())
        sequence.done()
        return
    end

    callback = callback or function() obj:tick() end

    local first = true
    if iterate then
        obj.id = leap.eventstream(
            'Timers',
            {op='scheduleEvery', every=delay},
            function (event)
                local reqid = event.reqid
                if first then
                    first = false
                    dbg('timer(%s) first callback', reqid)
                    -- discard the first (immediate) response: don't call callback
                    return nil
                else
                    dbg('timer(%s) nth callback', reqid)
                    return callback(event)
                end
            end
        ).reqid
    else
        obj.id = leap.eventstream(
            'Timers',
            {op='scheduleAfter', after=delay},
            function (event)
                -- Arrange to return nil the first time, true the second. This
                -- callback is called immediately with the response to
                -- 'scheduleAfter', and if we immediately returned true, we'd
                -- be done, and the subsequent timer event would be discarded.
                if first then
                    first = false
                    -- Caller doesn't expect an immediate callback.
                    return nil
                else
                    callback(event)
                    -- Since caller doesn't want to iterate, the value
                    -- returned by the callback is irrelevant: just stop after
                    -- this one and only call.
                    return true
                end
            end
        ).reqid
    end

    return obj
end

util.classctor(timers.Timer)

function timers.Timer:tick()
    error('Pass a callback to Timer:new(), or override Timer:tick()')
end

function timers.Timer:cancel()
    local ok = leap.request('Timers', {op='cancel', id=self.id}).ok
    leap.cancelreq(self.id)
    return ok
end

function timers.Timer:isRunning()
    return leap.request('Timers', {op='isRunning', id=self.id}).running
end

-- returns (true, seconds left) for a live timer, else (false, 0)
function timers.Timer:timeUntilCall()
    local result = leap.request('Timers', {op='timeUntilCall', id=self.id})
    return result.ok, result.remaining
end

return timers
