local fiber = require 'fiber'
local inspect = require 'inspect'
local leap = require 'leap'
local util = require 'util'

local LLListener = {}
local waitfor = {}
local listener_name = {}

function LLListener:new(pump_name)
    local obj = setmetatable({}, self)
    self.__index = self
    obj.name = 'Listener:' .. pump_name
    obj._pump = pump_name

    return obj
end

util.classctor(LLListener)

function LLListener:handleMessages(event_data)
    print(inspect(event_data))
    return true
end

function LLListener:start()
    _pump = self._pump
    waitfor = leap.WaitFor(-1, self.name)
    function waitfor:filter(pump, data)
        if _pump == pump then
          return data
        end
    end

    fiber.launch(self.name, function()
        event = waitfor:wait()
        while event and self:handleMessages(event) do
          event = waitfor:wait()
        end
    end)

    listener_name = leap.request(leap.cmdpump(), {op='listen', source=_pump, listener="LLListener", tweak=true}).listener
end

function LLListener:stop()
    leap.send(leap.cmdpump(), {op='stoplistening', source=self._pump, listener=listener_name})
    waitfor:close()
end

return LLListener
