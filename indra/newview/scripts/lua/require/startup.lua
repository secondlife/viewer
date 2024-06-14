-- query, wait for or mandate a particular viewer startup state

-- During startup, the viewer steps through a sequence of numbered (and named)
-- states. This can be used to detect when, for instance, the login screen is
-- displayed, or when the viewer has finished logging in and is fully
-- in-world.

local fiber = require 'fiber'
local leap = require 'leap'
local inspect = require 'inspect'
local function dbg(...) end
-- local dbg = require 'printf'

-- ---------------------------------------------------------------------------
local startup = {}

-- Get the list of startup states from the viewer.
local bynum = leap.request('LLStartUp', {op='getStateTable'})['table']

local byname = setmetatable(
    {},
    -- set metatable to throw an error if you look up invalid state name
    {__index=function(t, k)
         local v = rawget(t, k)
         if v then
             return v
         end
         error(string.format('startup module passed invalid state %q', k), 2)
    end})

-- derive byname as a lookup table to find the 0-based index for a given name
for i, name in pairs(bynum) do
    -- the viewer's states are 0-based, not 1-based like Lua indexes
    byname[name] = i - 1
end
-- dbg('startup states: %s', inspect(byname))

-- specialize a WaitFor to track the viewer's startup state
local startup_pump = 'StartupState'
local waitfor = leap.WaitFor(0, startup_pump)
function waitfor:filter(pump, data)
    if pump == self.name then
        return data
    end
end

function waitfor:process(data)
    -- keep updating startup._state for interested parties
    startup._state = data.str
    dbg('startup updating state to %q', data.str)
    -- now pass data along to base-class method to queue
    leap.WaitFor.process(self, data)
end

-- listen for StartupState events
leap.request(leap.cmdpump(),
             {op='listen', source=startup_pump, listener='startup.lua', tweak=true})
-- poke LLStartUp to make sure we get an event
leap.send('LLStartUp', {op='postStartupState'})

-- ---------------------------------------------------------------------------
-- wait for response from postStartupState
while not startup._state do
    dbg('startup.state() waiting for first StartupState event')
    waitfor:wait()
end

-- return a list of all known startup states
function startup.list()
    return bynum
end

-- report whether state with string name 'left' is before string name 'right'
function startup.before(left, right)
    return byname[left] < byname[right]
end

-- report the viewer's current startup state
function startup.state()
    return startup._state
end

-- error if script is called before specified state string name
function startup.ensure(state)
    if startup.before(startup.state(), state) then
        -- tell error() to pretend this error was thrown by our caller
        error('must not be called before startup state ' .. state, 2)
    end
end

-- block calling fiber until viewer has reached state with specified string name
function startup.wait(state)
    dbg('startup.wait(%q)', state)
    while startup.before(startup.state(), state) do
        local item = waitfor:wait()
        dbg('startup.wait(%q) sees %s', state, item)
    end
end

return startup
