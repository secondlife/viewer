local fiber = require 'fiber'
local inspect = require 'inspect'
local leap = require 'leap'
local util = require 'util'

local LLListener = {}

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

function LLListener:debugHandleMessages(event_data)
    local ret = self:handleMessages(event_data)
    print(`LLListener({self.name}): got {ret} from {inspect(event_data)}`)
    return ret
end

function LLListener:start()
    local _pump = self._pump
    self.waitfor = leap.WaitFor(-1, self.name)
    function self.waitfor:filter(pump, data)
        if _pump == pump then
          return data
        end
    end

    fiber.launch(self.name, function()
        local event = self.waitfor:wait()
        while event and self:handleMessages(event) do
          event = self.waitfor:wait()
        end
    end)

    self.listener_name = leap.request(
        leap.cmdpump(),
        {op='listen', source=_pump, listener="LLListener", tweak=true}).listener
end

function LLListener:stop()
    if self.listener_name then
        leap.send(leap.cmdpump(),
                  {op='stoplistening', source=self._pump, listener=self.listener_name})
    end
    self.waitfor:close()
end

return LLListener
