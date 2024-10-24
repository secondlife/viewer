-- Access to the viewer's time-delay facilities

local leap = require 'leap'
local util = require 'util'

local timers = {}

local function dbg(...) end
-- local dbg = require 'printf'

function timers.sleep(delay)
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
end

timers.Timer = {}

-- delay:    time in seconds until callback
-- callback: function to call when timer fires (self:tick if nil)
-- iterate:  if non-nil, call callback repeatedly until it returns non-nil
function timers.Timer:new(delay, callback, iterate)
    local obj = setmetatable({}, self)
    self.__index = self

    callback = callback or function() obj:tick() end

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
            nil,                -- don't call callback with initial response
            function (event)
                local reqid = event.reqid
                if in_callback then
                    dbg('dropping timer(%s) callback', reqid)
                else
                    dbg('timer(%s) callback', reqid)
                    in_callback = true
                    local ret = callback(event)
                    in_callback = false
                    return ret
                end
            end
        ).reqid
    else                        -- (not iterate)
        obj.id = leap.eventstream(
            'Timers',
            {op='scheduleAfter', after=delay},
            nil,                -- don't call callback with initial response
            function (event)
                callback(event)
                -- Since caller doesn't want to iterate, the value returned by
                -- caller's callback is irrelevant: just stop after this one
                -- and only call.
                return true
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
