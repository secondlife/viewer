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

    local calls = 0
    if iterate then
        -- With iterative timers, beware of running a timer callback which
        -- performs async actions lasting longer than the timer interval. The
        -- lengthy callback suspends, allowing leap to retrieve the next
        -- event, which is a timer tick. leap calls a new instance of the
        -- callback, even though the previous callback call is still
        -- suspended... etc. 'in_callback' defends against that recursive
        -- case. Rather than re-enter the suspended callback, drop the
        -- too-soon timer event. (We could count the too-soon timer events and
        -- iterate calling the callback, but it's a bathtub problem: the
        -- callback could end up getting farther and farther behind.)
        local in_callback = false
        obj.id = leap.eventstream(
            'Timers',
            {op='scheduleEvery', every=delay},
            function (event)
                local reqid = event.reqid
                calls += 1
                if calls == 1 then
                    dbg('timer(%s) first callback', reqid)
                    -- discard the first (immediate) response: don't call callback
                    return nil
                else
                    if in_callback then
                        dbg('dropping timer(%s) callback %d', reqid, calls)
                    else
                        dbg('timer(%s) callback %d', reqid, calls)
                        in_callback = true
                        local ret = callback(event)
                        in_callback = false
                        return ret
                    end
                end
            end
        ).reqid
    else                        -- (not iterate)
        obj.id = leap.eventstream(
            'Timers',
            {op='scheduleAfter', after=delay},
            function (event)
                calls += 1
                -- Arrange to return nil the first time, true the second. This
                -- callback is called immediately with the response to
                -- 'scheduleAfter', and if we immediately returned true, we'd
                -- be done, and the subsequent timer event would be discarded.
                if calls == 1 then
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
