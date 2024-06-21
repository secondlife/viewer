-- Lua implementation of LEAP (LLSD Event API Plugin) protocol
--
-- This module supports Lua scripts run by the Second Life viewer.
--
-- LEAP protocol passes LLSD objects, converted to/from Lua tables, in both
-- directions. A typical LLSD object is a map containing keys 'pump' and
-- 'data'.
--
-- The viewer's Lua post_to(pump, data) function posts 'data' to the
-- LLEventPump 'pump'. This is typically used to engage an LLEventAPI method.
--
-- Similarly, the viewer gives each Lua script its own LLEventPump with a
-- unique name. That name is returned by get_event_pumps(). Every event
-- received on that LLEventPump is queued for retrieval by get_event_next(),
-- which returns (pump, data): the name of the LLEventPump on which the event
-- was received and the received event data. When the queue is empty,
-- get_event_next() blocks the calling Lua script until the next event is
-- received.
--
-- Usage:
-- 1. Launch some number of Lua coroutines. The code in each coroutine may
--    call leap.send(), leap.request() or leap.generate(). leap.send() returns
--    immediately ("fire and forget"). leap.request() blocks the calling
--    coroutine until it receives and returns the viewer's response to its
--    request. leap.generate() expects an arbitrary number of responses to the
--    original request.
-- 2. To handle events from the viewer other than direct responses to
--    requests, instantiate a leap.WaitFor object with a filter(pump, data)
--    override method that returns non-nil for desired events. A coroutine may
--    call wait() on any such WaitFor.
-- 3. Once the coroutines have been launched, call leap.process() on the main
--    coroutine. process() retrieves incoming events from the viewer and
--    dispatches them to waiting request() or generate() calls, or to
--    appropriate WaitFor instances. process() returns when either
--    get_event_next() raises an error or the viewer posts nil to the script's
--    reply pump to indicate it's done.
-- 4. Alternatively, a running coroutine may call leap.done() to break out of
--    leap.process(). process() won't notice until the next event from the
--    viewer, though.

local fiber = require('fiber')
local ErrorQueue = require('ErrorQueue')
local inspect = require('inspect')
local function dbg(...) end 
-- local dbg = require('printf')
local util = require('util')

local leap = {}

-- reply: string name of reply LLEventPump. Any events the viewer posts to
-- this pump will be queued for get_event_next(). We usually specify it as the
-- reply pump for requests to internal viewer services.
-- command: string name of command LLEventPump. post_to(command, ...)
-- engages LLLeapListener operations such as listening on a specified other
-- LLEventPump, etc.
local reply, command = LL.get_event_pumps()
-- Dict of features added to the LEAP protocol since baseline implementation.
-- Before engaging a new feature that might break an older viewer, we can
-- check for the presence of that feature key. This table is solely about the
-- LEAP protocol itself, the way we communicate with the viewer. To discover
-- whether a given listener exists, or supports a particular operation, use
-- command's "getAPI" operation.
-- For Lua, command's "getFeatures" operation suffices?
-- leap._features = {}

-- Each outstanding request() or generate() call has a corresponding
-- WaitForReqid object (later in this module) to handle the
-- response(s). If an incoming event contains an echoed ["reqid"] key,
-- we can look up the appropriate WaitForReqid object more efficiently
-- in a dict than by tossing such objects into the usual waitfors list.
-- Note: the ["reqid"] must be unique, otherwise we could end up
-- replacing an earlier WaitForReqid object in pending with a
-- later one. That means that no incoming event will ever be given to
-- the old WaitForReqid object. Any coroutine waiting on the discarded
-- WaitForReqid object would therefore wait forever.
-- pending is NOT a weak table because the caller of request() or generate()
-- never sees the WaitForReqid object. pending holds the only reference, so
-- it should NOT be garbage-collected.
local pending = {}
-- Our consumer will instantiate some number of WaitFor subclass objects.
-- As these are traversed in descending priority order, we must keep
-- them in a list.
-- Anyone who instantiates a WaitFor subclass object should retain a reference
-- to it. Once the consuming script drops the reference, allow Lua to
-- garbage-collect the WaitFor despite its entry in waitfors.
local weak_values = {__mode='v'}
local waitfors = setmetatable({}, weak_values)
-- It has been suggested that we should use UUIDs as ["reqid"] values,
-- since UUIDs are guaranteed unique. However, as the "namespace" for
-- ["reqid"] values is our very own reply pump, we can get away with
-- an integer.
leap._reqid = 0
-- break leap.process() loop
leap._done = false

-- get the name of the reply pump
function leap.replypump()
    return reply
end

-- get the name of the command pump
function leap.cmdpump()
    return command
end

-- Fire and forget. Send the specified request LLSD, expecting no reply.
-- In fact, should the request produce an eventual reply, it will be
-- treated as an unsolicited event.
-- 
-- See also request(), generate().
function leap.send(pump, data, reqid)
    local data = data
    if type(data) == 'table' then
        data = table.clone(data)
        data['reply'] = reply
        if reqid ~= nil then
            data['reqid'] = reqid
        end
    end
    dbg('leap.send(%s, %s) calling post_on()', pump, data)
    LL.post_on(pump, data)
end

-- common setup code shared by request() and generate()
local function requestSetup(pump, data)
    -- invent a new, unique reqid
    leap._reqid += 1
    local reqid = leap._reqid
    -- Instantiate a new WaitForReqid object. The priority is irrelevant
    -- because, unlike the WaitFor base class, WaitForReqid does not
    -- self-register on our waitfors list. Instead, capture the new
    -- WaitForReqid object in pending so dispatch() can find it.
    local waitfor = leap.WaitForReqid(reqid)
    pending[reqid] = waitfor
    -- Pass reqid to send() to stamp it into (a copy of) the request data.
    dbg('requestSetup(%s, %s) storing %s', pump, data, waitfor.name)
    leap.send(pump, data, reqid)
    return reqid, waitfor
end

-- Send the specified request LLSD, expecting exactly one reply. Block
-- the calling coroutine until we receive that reply.
-- 
-- Every request() (or generate()) LLSD block we send will get stamped
-- with a distinct ["reqid"] value. The requested event API must echo the
-- same ["reqid"] field in each reply associated with that request. This way
-- we can correctly dispatch interleaved replies from different requests.
-- 
-- If the desired event API doesn't support the ["reqid"] echo convention,
-- you should use send() instead -- since request() or generate() would
-- wait forever for a reply stamped with that ["reqid"] -- and intercept
-- any replies using WaitFor.
-- 
-- Unless the request data already contains a ["reply"] key, we insert
-- reply=self.replypump to try to ensure that the expected reply will be
-- returned over the socket.
--
-- See also send(), generate().
function leap.request(pump, data)
    local reqid, waitfor = requestSetup(pump, data)
    dbg('leap.request(%s, %s) about to wait on %s', pump, data, tostring(waitfor))
    local ok, response = pcall(waitfor.wait, waitfor)
    dbg('leap.request(%s, %s) got %s: %s', pump, data, ok, response)
    -- kill off temporary WaitForReqid object, even if error
    pending[reqid] = nil
    if not ok then
        error(response)
    elseif response.error then
        error(response.error)
    else
        return response
    end
end

-- Send the specified request LLSD, expecting an arbitrary number of replies.
-- Each one is returned on request.
--
-- Usage:
-- sequence = leap.generate(pump, data)
-- repeat
--     response = sequence.next()
-- until last(response)
-- (last() means whatever test the caller wants to perform on response)
-- sequence.done()
-- 
-- See request() remarks about ["reqid"].
--
-- Note: this seems like a prime use case for Lua coroutines. But in a script
-- using fibers.lua, a "wild" coroutine confuses the fiber scheduler. If
-- generate() were itself a coroutine, it would call WaitForReqid:wait(),
-- which would yield -- thereby resuming generate() WITHOUT waiting.
function leap.generate(pump, data, checklast)
    -- Invent a new, unique reqid. Arrange to handle incoming events
    -- bearing that reqid. Stamp the outbound request with that reqid, and
    -- send it.
    local reqid, waitfor = requestSetup(pump, data)
    return {
        next = function()
            dbg('leap.generate(%s).next() about to wait on %s', reqid, tostring(waitfor))
            local ok, response = pcall(waitfor.wait, waitfor)
            dbg('leap.generate(%s).next() got %s: %s', reqid, ok, response)
            if not ok then
                error(response)
            elseif response.error then
                error(response.error)
            else
                return response
            end
        end,
        done = function()
            -- cleanup consists of removing our WaitForReqid from pending
            pending[reqid] = nil
        end
    }
end

-- Send the specified request LLSD, expecting an immediate reply followed by
-- an arbitrary number of subsequent replies with the same reqid. Block the
-- calling coroutine until the first (immediate) reply, but launch a separate
-- fiber on which to call the passed callback with later replies.
--
-- Once the callback returns true, the background fiber terminates.
function leap.eventstream(pump, data, callback)
    local reqid, waitfor = requestSetup(pump, data)
    local response = waitfor:wait()
    if response.error then
        -- clean up our WaitForReqid
        waitfor:close()
        error(response.error)
    end
    -- No error, so far so good:
    -- call the callback with the first response just in case
    dbg('leap.eventstream(%s): first callback', reqid)
    local ok, done = pcall(callback, response)
    dbg('leap.eventstream(%s) got %s, %s', reqid, ok, done)
    if not ok then
        -- clean up our WaitForReqid
        waitfor:close()
        error(done)
    end
    if done then
        return response
    end
    -- callback didn't throw an error, and didn't say stop,
    -- so set up to handle subsequent events
    -- TODO: distinguish "daemon" fibers that can be terminated even if waiting
    fiber.launch(
        pump,
        function ()
            local ok, done
            local nth = 1
            repeat
                event = waitfor:wait()
                if not event then
                    -- wait() returns nil once the queue is closed (e.g. cancelreq())
                    ok, done = true, true
                else
                    nth += 1
                    dbg('leap.eventstream(%s): callback %d', reqid, nth)
                    ok, done = pcall(callback, event)
                    dbg('leap.eventstream(%s) got %s, %s', reqid, ok, done)
                end
                -- not ok means callback threw an error (caught as 'done')
                -- done means callback succeeded but wants to stop
            until (not ok) or done
            -- once we break this loop, clean up our WaitForReqid
            waitfor:close()
            if not ok then
                -- can't reflect the error back to our caller
                LL.print_warning(fiber.get_name() .. ': ' .. done)
            end
        end)
    return response
end

-- we might want to clean up after leap.eventstream() even if the callback has
-- not yet returned true
function leap.cancelreq(reqid)
    dbg('cancelreq(%s)', reqid)
    local waitfor = pending[reqid]
    if waitfor ~= nil then
        -- close() removes the pending entry and also closes the queue,
        -- breaking the background fiber's wait loop.
        dbg('cancelreq(%s) canceling %s', reqid, waitfor.name)
        waitfor:close()
    end
end

local function cleanup(message)
    -- We're done: clean up all pending coroutines.
    -- Iterate over copies of the pending and waitfors tables, since the
    -- close() operation modifies the real tables.
    for i, waitfor in pairs(table.clone(pending)) do
        waitfor:close()
    end
    for i, waitfor in pairs(table.clone(waitfors)) do
        waitfor:close()
    end
end

-- Handle an incoming (pump, data) event with no recognizable ['reqid']
local function unsolicited(pump, data)
    -- we maintain waitfors in descending priority order, so the first waitfor
    -- to claim this event is the one with the highest priority
    for i, waitfor in pairs(waitfors) do
        dbg('unsolicited() checking %s', waitfor.name)
        if waitfor:handle(pump, data) then
            return
        end
    end
    LL.print_debug(string.format('unsolicited(%s, %s) discarding unclaimed event',
                                 pump, inspect(data)))
end

-- Route incoming (pump, data) event to the appropriate waiting coroutine.
local function dispatch(pump, data)
    local reqid = data['reqid']
    -- if the response has no 'reqid', it's not from request() or generate()
    if reqid == nil then
--      dbg('dispatch() found no reqid; calling unsolicited(%s, %s)', pump, data)
        return unsolicited(pump, data)
    end
    -- have reqid; do we have a WaitForReqid?
    local waitfor = pending[reqid]
    if waitfor == nil then
--      dbg('dispatch() found no WaitForReqid(%s); calling unsolicited(%s, %s)', reqid, pump, data)
        return unsolicited(pump, data)
    end
    -- found the right WaitForReqid object, let it handle the event
--  dbg('dispatch() calling %s.handle(%s, %s)', waitfor.name, pump, data)
    waitfor:handle(pump, data)
end

-- We configure fiber.set_idle() function. fiber.yield() calls the configured
-- idle callback whenever there are waiting fibers but no ready fibers. In
-- our case, that means it's time to fetch another incoming viewer event.
fiber.set_idle(function ()
    -- If someone has called leap.done(), then tell fiber.yield() to break loop.
    if leap._done then
        cleanup('done')
        return 'done'
    end
    dbg('leap.idle() calling get_event_next()')
    local ok, pump, data = pcall(LL.get_event_next)
    dbg('leap.idle() got %s: %s, %s', ok, pump, data)
    -- ok false means get_event_next() raised a Lua error, pump is message
    if not ok then
        cleanup(pump)
        error(pump)
    end
    -- data nil means get_event_next() returned (pump, LLSD()) to indicate done
    if not data then
        cleanup('end')
        return 'end'
    end
    -- got a real pump, data pair 
    dispatch(pump, data)
    -- return to fiber.yield(): any incoming message might result in one or
    -- more fibers becoming ready
end)

function leap.done()
    leap._done = true
end

-- called by WaitFor.enable()
local function registerWaitFor(waitfor)
    table.insert(waitfors, waitfor)
    -- keep waitfors sorted in descending order of specified priority
    table.sort(waitfors,
               function (lhs, rhs) return lhs.priority > rhs.priority end)
end

-- called by WaitFor.disable()
local function unregisterWaitFor(waitfor)
    local i = table.find(waitfors, waitfor)
    if i ~= nil then
        waitfors[i] = nil
    end
end

-- ******************************************************************************
-- WaitFor and friends
-- ******************************************************************************

-- An unsolicited event is handled by the highest-priority WaitFor subclass
-- object willing to accept it. If no such object is found, the unsolicited
-- event is discarded.
-- 
-- * First, instantiate a WaitFor subclass object to register its interest in
--   some incoming event(s). WaitFor instances are self-registering; merely
--   instantiating the object suffices.
-- * Any coroutine may call a given WaitFor object's wait() method. This blocks
--   the calling coroutine until a suitable event arrives.
-- * WaitFor's constructor accepts a float priority. Every incoming event
--   (other than those claimed by request() or generate()) is passed to each
--   extant WaitFor.filter() method in descending priority order. The first
--   such filter() to return nontrivial data claims that event.
-- * At that point, the blocked wait() call on that WaitFor object returns the
--   item returned by filter().
-- * WaitFor contains a queue. Multiple arriving events claimed by that WaitFor
--   object's filter() method are added to the queue. Naturally, until the
--   queue is empty, calling wait() immediately returns the front entry.
-- 
-- It's reasonable to instantiate a WaitFor subclass whose filter() method
-- unconditionally returns the incoming event, and whose priority places it
-- last in the list. This object will enqueue every unsolicited event left
-- unclaimed by other WaitFor subclass objects.
-- 
-- It's not strictly necessary to associate a WaitFor object with exactly one
-- coroutine. You might have multiple "worker" coroutines drawing from the same
-- WaitFor object, useful if the work being done per event might itself involve
-- "blocking" operations. Or a given coroutine might sample a number of WaitFor
-- objects in round-robin fashion... etc. etc. Nonetheless, it's
-- straightforward to designate one coroutine for each WaitFor object.

-- --------------------------------- WaitFor ---------------------------------
leap.WaitFor = { _id=0 }

function leap.WaitFor.tostring(self)
    -- Lua (sub)classes have no name; can't prefix with that
    return self.name
end

function leap.WaitFor:new(priority, name)
    local obj = setmetatable({__tostring=leap.WaitFor.tostring}, self)
    self.__index = self

    obj.priority = priority
    if name then
        obj.name = name
    else
        self._id += 1
        obj.name = 'WaitFor' .. self._id
    end
    obj._queue = ErrorQueue()
    obj._registered = false
    -- if no priority, then don't enable() - remember 0 is truthy
    if priority then
        obj:enable()
    end

    return obj
end

util.classctor(leap.WaitFor)

-- Re-enable a disable()d WaitFor object. New WaitFor objects are
-- enable()d by default.
function leap.WaitFor:enable()
    if not self._registered then
        registerWaitFor(self)
        self._registered = true
    end
end

-- Disable an enable()d WaitFor object.
function leap.WaitFor:disable()
    if self._registered then
        unregisterWaitFor(self)
        self._registered = false
    end
end

-- Block the calling coroutine until a suitable unsolicited event (one
-- for which filter() returns the event) arrives.
function leap.WaitFor:wait()
    dbg('%s about to wait', self.name)
    local item = self._queue:Dequeue()
    dbg('%s got %s', self.name, item)
    return item
end

-- Override filter() to examine the incoming event in whatever way
-- makes sense.
-- 
-- Return nil to ignore this event.
-- 
-- To claim the event, return the item you want placed in the queue.
-- Typically you'd write:
-- return data
-- or perhaps
-- return {pump=pump, data=data}
-- or some variation.
function leap.WaitFor:filter(pump, data)
    error('You must override the WaitFor.filter() method')
end

-- called by unsolicited() for each WaitFor in waitfors
function leap.WaitFor:handle(pump, data)
    local item = self:filter(pump, data)
    dbg('%s.filter() returned %s', self.name, item)
    -- if this item doesn't pass the filter, we're not interested
    if not item then
        return false
    end
    -- okay, filter() claims this event
    self:process(item)
    return true
end

-- called by WaitFor:handle() for an accepted event
function leap.WaitFor:process(item)
    self._queue:Enqueue(item)
end

-- called by cleanup() at end
function leap.WaitFor:close()
    self:disable()
    self._queue:close()
end

-- called by leap.process() when get_event_next() raises an error
function leap.WaitFor:exception(message)
    LL.print_warning(self.name .. ' error: ' .. message)
    self._queue:Error(message)
end

-- ------------------------------ WaitForReqid -------------------------------
leap.WaitForReqid = leap.WaitFor()

function leap.WaitForReqid:new(reqid)
    -- priority is meaningless, since this object won't be added to the
    -- priority-sorted waitfors list. Use the reqid as the debugging name
    -- string.
    local obj = leap.WaitFor(nil, 'WaitForReqid(' .. reqid .. ')')
    setmetatable(obj, self)
    self.__index = self

    obj.reqid = reqid

    return obj
end

util.classctor(leap.WaitForReqid)

function leap.WaitForReqid:filter(pump, data)
    -- Because we expect to directly look up the WaitForReqid object of
    -- interest based on the incoming ["reqid"] value, it's not necessary
    -- to test the event again. Accept every such event.
    return data
end

function leap.WaitForReqid:close()
    -- remove this entry from pending table
    pending[self.reqid] = nil
    self._queue:close()
end

return leap
