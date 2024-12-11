local fiber = require 'fiber'
local inspect = require 'inspect'
local leap = require 'leap'
local util = require 'util'

local LLListener = util.class(
    'LLListener',
    function(self, pump_name)
        return {
            name = 'Listener:' .. pump_name,
            _pump = pump_name
        }
    end)

function LLListener:handleMessages(event_data)
    print(inspect(event_data))
    return true
end

function LLListener:debugHandleMessages(event_data)
    local ret = self:handleMessages(event_data)
    print(`LLListener({self.name}): got {ret} from {inspect(event_data)}`)
    return ret
end

function LLListener:start()
    -- It's important to instantiate the WaitForPump *before* telling the
    -- viewer we want to listen on that pump. Otherwise, there's a small time
    -- window between the 'listen' command and instantiating the WaitForPump
    -- when events could arrive but be discarded.
    self.waitfor = leap.WaitForPump(self._pump)

    self.listener_name = leap.request(
        leap.cmdpump(),
        {op='listen', source=self._pump, listener="LLListener", tweak=true}).listener

    fiber.launch(
        self.name,
        -- start() expects handleMessages() to keep returning true until it's
        -- done, then return false. consumeWaitFor() expects its handler to
        -- keep returning nil until it's done, then return non-nil.
        leap.consumeWaitFor, self.waitfor, util.notfunc(util.bindclass(self, 'handleMessages')))
end

function LLListener:stop()
    if self.listener_name then
        leap.send(leap.cmdpump(),
                  {op='stoplistening', source=self._pump, listener=self.listener_name})
    end
    self.waitfor:close()
end

-- inline(function(event) ... end) calls the passed handler function for each
-- incoming message on the pump_name specified to the constructor. As long as
-- the handler function returns nil, inline() blocks the calling fiber. When
-- the handler returns any non-nil value, inline() cleans up and returns that
-- value to its caller.
-- If kickoff() is specified, it's a function to be called once we set up
-- listening on the specified pump_name. Any additional arguments are passed
-- to kickoff(). Any errors raised by kickoff() are propagated to the caller.
function LLListener:inline(handler, kickoff, ...)
--    print(`LLListener({self._pump}):inline()`)
    local waitfor = leap.WaitForPump(self._pump)
    local listener_name = leap.request(
        leap.cmdpump(),
        {op='listen', source=self._pump, listener="LLListener", tweak=true}).listener

    local ok=true, ret
    if kickoff then
        -- We don't care what kickoff() returns voluntarily, but if it raises
        -- an error, we need to capture it for the callok() call below.
        ok, ret = pcall(kickoff, ...)
    end
    -- don't call consumeWaitFor() if kickoff() raise an error
    if ok then
        ok, ret = pcall(leap.consumeWaitFor, waitfor, handler)
    end
    -- clean up regardless of errors raised by either kickoff() or consumeWaitFor()
    leap.send(leap.cmdpump(),
              {op='stoplistening', source=self._pump, listener=listener_name})
    return util.callok(ok, ret)
end

return LLListener
