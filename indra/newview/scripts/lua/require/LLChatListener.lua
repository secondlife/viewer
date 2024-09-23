local fiber = require 'fiber'
local inspect = require 'inspect'
local leap = require 'leap'
local util = require 'util'

local LLChatListener = {}
local waitfor = {}
local listener_name = {}

function LLChatListener:new()
    local obj = setmetatable({}, self)
    self.__index = self
    obj.name = 'Chat_listener'

    return obj
end

util.classctor(LLChatListener)

function LLChatListener:handleMessages(event_data)
    print(inspect(event_data))
    return true
end

function LLChatListener:start()
    waitfor = leap.WaitFor(-1, self.name)
    function waitfor:filter(pump, data)
        if pump == "LLNearbyChat" then
          return data
        end
    end

    fiber.launch(self.name, function()
        event = waitfor:wait()
        while event and self:handleMessages(event) do
          event = waitfor:wait()
        end
    end)

    listener_name = leap.request(leap.cmdpump(), {op='listen', source='LLNearbyChat', listener="ChatListener", tweak=true}).listener
end

function LLChatListener:stop()
    leap.send(leap.cmdpump(), {op='stoplistening', source='LLNearbyChat', listener=listener_name})
    waitfor:close()
end

return LLChatListener
