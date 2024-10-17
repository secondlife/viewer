-- Organize Lua coroutines into fibers.

-- In this usage, the difference between coroutines and fibers is that fibers
-- have a scheduler. Yielding a fiber means allowing other fibers, plural, to
-- run: it's more than just returning control to the specific Lua thread that
-- resumed the running coroutine.

-- fiber.launch() creates a new fiber ready to run.
-- fiber.status() reports (augmented) status of the passed fiber: instead of
-- 'suspended', it returns either 'ready' or 'waiting'
-- fiber.yield() allows other fibers to run, but leaves the calling fiber
-- ready to run.
-- fiber.wait() marks the running fiber not ready, and resumes other fibers.
-- fiber.wake() marks the designated suspended fiber ready to run, but does
-- not yet resume it.
-- fiber.run() runs all current fibers until all have terminated (successfully
-- or with an error).

local printf = require 'printf'
local function dbg(...) end
-- local dbg = printf
local coro = require 'coro'

local fiber = {}

-- The tables in which we track fibers must have weak keys so dead fibers
-- can be garbage-collected.
local weak_values = {__mode='v'}
local weak_keys   = {__mode='k'}

-- Track each current fiber as being either ready to run or not ready
-- (waiting). wait() moves the running fiber from ready to waiting; wake()
-- moves the designated fiber from waiting back to ready.
-- The ready table is used as a list so yield() can go round robin.
local ready   = setmetatable({'main'}, weak_keys)
-- The waiting table is used as a set because order doesn't matter.
local waiting = setmetatable({}, weak_keys)

-- Every fiber has a name, for diagnostic purposes. Names must be unique.
-- A colliding name will be suffixed with an integer.
-- Predefine 'main' with our marker so nobody else claims that name.
local names  = setmetatable({main='main'}, weak_keys)
local byname = setmetatable({main='main'}, weak_values)
-- each colliding name has its own distinct suffix counter
local suffix = {}

-- Specify a nullary idle() callback to be called whenever there are no ready
-- fibers but there are waiting fibers. The idle() callback is responsible for
-- changing zero or more waiting fibers to ready fibers by calling
-- fiber.wake(), although a given call may leave them all still waiting.
-- When there are no ready fibers, it's a good idea for the idle() function to
-- return control to a higher-level execution agent. Simply returning without
-- changing any fiber's status will spin the CPU.
-- The idle() callback can return non-nil to exit fiber.run() with that value.
function fiber._idle()
    error('fiber.yield(): you must first call set_idle(nullary idle() function)')
end

function fiber.set_idle(func)
    fiber._idle = func
end

-- Launch a new Lua fiber, ready to run.
function fiber.launch(name, func, ...)
    local args = table.pack(...)
    local co = coroutine.create(function() func(table.unpack(args)) end)
    -- a new fiber is ready to run
    table.insert(ready, co)
    local namekey = name
    while byname[namekey] do
        if not suffix[name] then
            suffix[name] = 1
        end
        suffix[name] += 1
        namekey = name .. tostring(suffix[name])
    end
    -- found a namekey not yet in byname: set it
    byname[namekey] = co
    -- and remember it as this fiber's name
    names[co] = namekey
--  dbg('launch(%s)', namekey)
--  dbg('byname[%s] = %s', namekey, tostring(byname[namekey]))
--  dbg('names[%s] = %s', tostring(co), names[co])
--  dbg('ready[-1] = %s', tostring(ready[#ready]))
end

-- for debugging
function format_all()
    output = {}
    table.insert(output, 'Ready fibers:' .. if next(ready) then '' else ' none')
    for _, co in pairs(ready) do
        table.insert(output, string.format('  %s: %s', fiber.get_name(co), fiber.status(co)))
    end
    table.insert(output, 'Waiting fibers:' .. if next(waiting) then '' else ' none')
    for co in pairs(waiting) do
        table.insert(output, string.format('  %s: %s', fiber.get_name(co), fiber.status(co)))
    end
    return table.concat(output, '\n')
end

function fiber.print_all()
    print(format_all())
end

-- return either the running coroutine or, if called from the main thread,
-- 'main'
function fiber.running()
    return coroutine.running() or 'main'
end

-- Query a fiber's name (nil for the running fiber)
function fiber.get_name(co)
    return names[co or fiber.running()] or 'unknown'
end

-- Query status of the passed fiber
function fiber.status(co)
    local running = coroutine.running()
    if (not co) or co == running then
        -- silly to ask the status of the running fiber: it's 'running'
        return 'running'
    end
    if co ~= 'main' then
        -- for any coroutine but main, consult coroutine.status()
        local status = coroutine.status(co)
        if status ~= 'suspended' then
            return status
        end
        -- here co is suspended, answer needs further refinement
    else
        -- co == 'main'
        if not running then
            -- asking about 'main' from the main fiber
            return 'running'
        end
        -- asking about 'main' from some other fiber, so presumably main is suspended
    end
    -- here we know co is suspended -- but is it ready to run?
    if waiting[co] then
        return 'waiting'
    end
    -- not waiting should imply ready: sanity check
    if table.find(ready, co) then
        return 'ready'
    end
    -- Calls within yield() between popping the next ready fiber and
    -- re-appending it to the list are in this state. Once we're done
    -- debugging yield(), we could reinstate either of the below.
--  error(string.format('fiber.status(%s) is stumped', fiber.get_name(co)))
--  print(string.format('*** fiber.status(%s) is stumped', fiber.get_name(co)))
    return '(unknown)'
end

-- change the running fiber's status to waiting
local function set_waiting()
    -- if called from the main fiber, inject a 'main' marker into the list
    co = fiber.running()
    -- delete from ready list
    local i = table.find(ready, co)
    if i then
        table.remove(ready, i)
    end
    -- add to waiting list
    waiting[co] = true
end

-- Suspend the current fiber until some other fiber calls fiber.wake() on it
function fiber.wait()
    dbg('Fiber %q waiting', fiber.get_name())
    set_waiting()
    -- now yield to other fibers
    fiber.yield()
end

-- Mark a suspended fiber as being ready to run
function fiber.wake(co)
    if not waiting[co] then
        error(string.format('fiber.wake(%s) but status=%s, ready=%s, waiting=%s',
                            names[co], fiber.status(co), ready[co], waiting[co]))
    end
    -- delete from waiting list
    waiting[co] = nil
    -- add to end of ready list
    table.insert(ready, co)
    dbg('Fiber %q ready', fiber.get_name(co))
    -- but don't yet resume it: that happens next time we reach yield()
end

-- pop and return the next not-dead fiber in the ready list, or nil if none remain
local function live_ready_iter()
    -- don't write:
    -- for co in table.remove, ready, 1
    -- because it would keep passing a new second parameter!
    for co in function() return table.remove(ready, 1) end do
        dbg('%s live_ready_iter() sees %s, status %s',
              fiber.get_name(), fiber.get_name(co), fiber.status(co))
        -- keep removing the head entry until we find one that's not dead,
        -- discarding any dead coroutines along the way
        if co == 'main' or coroutine.status(co) ~= 'dead' then
            dbg('%s live_ready_iter() returning %s',
                  fiber.get_name(), fiber.get_name(co))
            return co
        end
    end
    dbg('%s live_ready_iter() returning nil', fiber.get_name())
    return nil
end

-- prune the set of waiting fibers
local function prune_waiting()
    for waiter in pairs(waiting) do
        if waiter ~= 'main' and coroutine.status(waiter) == 'dead' then
            waiting[waiter] = nil
        end
    end
end

-- Run other ready fibers, leaving this one ready, returning after a cycle.
-- Returns:
-- * true, nil if there remain other live fibers, whether ready or waiting,
--   but it's our turn to run
-- * false, nil if this is the only remaining fiber
-- * nil,   x   if configured idle() callback returns non-nil x
local function scheduler()
    dbg('scheduler():\n%s', format_all())
    -- scheduler() is asymmetric because Lua distinguishes the main thread
    -- from other coroutines. The main thread can't yield; it can only resume
    -- other coroutines. So although an arbitrary coroutine could resume still
    -- other arbitrary coroutines, it could NOT resume the main thread because
    -- the main thread can't yield. Therefore, scheduler() delegates its real
    -- processing to the main thread. If called from a coroutine, pass control
    -- back to the main thread.
    if coroutine.running() then
        -- this is a real coroutine, yield normally to main thread
        coroutine.yield()
        -- main certainly still exists
        return true
    end

    -- This is the main fiber: coroutine.yield() doesn't work.
    -- Instead, resume each of the ready fibers.
    -- Prune the set of waiting fibers after every time fiber business logic
    -- runs (i.e. other fibers might have terminated or hit error), such as
    -- here on entry.
    prune_waiting()
    local others, idle_stop
    repeat
        for co in live_ready_iter do
            -- seize the opportunity to make sure the viewer isn't shutting down
            LL.check_stop()
            -- before we re-append co, is it the only remaining entry?
            others = next(ready)
            -- co is live, re-append it to the ready list
            table.insert(ready, co)
            if co == 'main' then
                -- Since we know the caller is the main fiber, it's our turn.
                -- Tell caller if there are other ready or waiting fibers.
                return others or next(waiting)
            end
            -- not main, but some other ready coroutine:
            -- use coro.resume() so we'll propagate any error encountered
            coro.resume(co)
            prune_waiting()
        end
        -- Here there are no ready fibers. Are there any waiting fibers?
        if not next(waiting) then
            return false
        end
        -- there are waiting fibers: call consumer's configured idle() function
        idle_stop = fiber._idle()
        if idle_stop ~= nil then
            return nil, idle_stop
        end
        prune_waiting()
        -- loop "forever", that is, until:
        -- * main is ready, or
        -- * there are neither ready fibers nor waiting fibers, or
        -- * fiber._idle() returned non-nil
    until false
end

-- Let other fibers run. This is useful in either of two cases:
-- * fiber.wait() calls this to run other fibers while this one is waiting.
--   fiber.yield() (and therefore fiber.wait()) works from the main thread as
--   well as from explicitly-launched fibers, without the caller having to
--   care.
-- * A long-running fiber that doesn't often call fiber.wait() should sprinkle
--   in fiber.yield() calls to interleave processing on other fibers.
function fiber.yield()
    -- The difference between this and fiber.run() is that fiber.yield()
    -- assumes its caller has work to do. yield() returns to its caller as
    -- soon as scheduler() pops this fiber from the ready list. fiber.run()
    -- continues looping until all other fibers have terminated, or the
    -- set_idle() callback tells it to stop.
    local others, idle_done = scheduler()
    -- scheduler() returns either if we're ready, or if idle_done ~= nil.
    if idle_done ~= nil then
        -- Returning normally from yield() means the caller can carry on with
        -- its pending work. But in this case scheduler() returned because the
        -- configured set_idle() function interrupted it -- not because we're
        -- actually ready. Don't return normally.
        error('fiber.set_idle() interrupted yield() with: ' .. tostring(idle_done))
    end
    -- We're ready! Just return to caller. In this situation we don't care
    -- whether there are other ready fibers.
    dbg('fiber.yield() returning to %s (%sothers are ready)',
        fiber.get_name(), ((not others) and "no " or ""))
end

-- Run fibers until all but main have terminated: return nil.
-- Or until configured idle() callback returns x ~= nil: return x.
function fiber.run()
    -- A fiber calling run() is not also doing other useful work. Remove the
    -- calling fiber from the ready list. Otherwise yield() would keep seeing
    -- that our caller is ready and return to us, instead of realizing that
    -- all coroutines are waiting and call idle(). But don't say we're
    -- waiting, either, because then when all other fibers have terminated
    -- we'd call idle() forever waiting for something to make us ready again.
    local i = table.find(ready, fiber.running())
    if i then
        table.remove(ready, i)
    end
    local others, idle_done
    repeat
        dbg('%s calling fiber.run() calling scheduler()', fiber.get_name())
        others, idle_done = scheduler()
        dbg("%s fiber.run()'s scheduler() returned %s, %s", fiber.get_name(),
              tostring(others), tostring(idle_done))
    until (not others)
    dbg('%s fiber.run() done', fiber.get_name())
    -- For whatever it's worth, put our own fiber back in the ready list.
    table.insert(ready, fiber.running())
    -- Once there are no more waiting fibers, and the only ready fiber is
    -- us, return to caller. All previously-launched fibers are done. Possibly
    -- the chunk is done, or the chunk may decide to launch a new batch of
    -- fibers.
    return idle_done
end

-- Make sure we finish up with a call to run(). That allows a consuming script
-- to kick off some number of fibers, do some work on the main thread and then
-- fall off the end of the script without explicitly calling fiber.run().
-- run() ensures the rest of the fibers run to completion (or error).
LL.atexit(fiber.run)

return fiber
