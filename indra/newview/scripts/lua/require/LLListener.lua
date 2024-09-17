local fiber = require 'fiber'
local inspect = require 'inspect'
local leap = require 'leap'
local util = require 'util'

local LLListener = {}
local waitfor = {}
local listener_name = {}
local pump = {}

function LLListener:new()
    local obj = setmetatable({}, self)
    self.__index = self
    obj.name = 'Listener'

    return obj
end

util.classctor(LLListener)

function LLListener:handleMessages(event_data)
    print(inspect(event_data))
    return true
end

function LLListener:start(pump_name)
    pump = pump_name
    waitfor = leap.WaitFor(-1, self.name)
    function waitfor:filter(pump_, data)
        if pump == pump_ then
          return data
        end
    end

    fiber.launch(self.name, function()
        event = waitfor:wait()
        while event and self:handleMessages(event) do
          event = waitfor:wait()
        end
    end)

    listener_name = leap.request(leap.cmdpump(), {op='listen', source=pump, listener="LLListener", tweak=true}).listener
end

function LLListener:stop()
    leap.send(leap.cmdpump(), {op='stoplistening', source=pump, listener=listener_name})
    waitfor:close()
end

return LLListener
