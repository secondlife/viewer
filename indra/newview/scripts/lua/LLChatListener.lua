local fiber = require 'fiber'
local inspect = require 'inspect'

local LLChatListener = {}
local waitfor = {}

function LLChatListener:new()
    local obj = setmetatable({}, self)
    self.__index = self
    obj.name = 'Chat_listener'

    return obj
end

function LLChatListener:handleMessages(event_data)
    --print(inspect(event_data))
    return true
end

function LLChatListener:start()
    waitfor = leap.WaitFor:new(-1, self.name)
    function waitfor:filter(pump, data)
        return data
    end

    fiber.launch(self.name, function()
        event = waitfor:wait()
        while event and self:handleMessages(event) do
          event = waitfor:wait()
        end
    end)

    leap.send('LLChatBar', {op='listen'})
end

function LLChatListener:stop()
    leap.send('LLChatBar', {op='stopListening'})
    waitfor:close()
end

return LLChatListener
