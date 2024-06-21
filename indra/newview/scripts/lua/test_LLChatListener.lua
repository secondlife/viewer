local LLChatListener = require 'LLChatListener'
local LLChat = require 'LLChat'
local leap = require 'leap'

function openOrEcho(message)
    local floater_name = string.match(message, "^open%s+(%w+)")
    if floater_name then
      leap.send("LLFloaterReg", {name = floater_name, op = "showInstance"})
    else
      LLChat.sendNearby('Echo: ' .. message)
    end
end

local listener = LLChatListener:new()

function listener:handleMessages(event_data)
    if string.find(event_data.message, '[LUA]') then
      return true
    elseif event_data.message == 'stop' then
      LLChat.sendNearby('Closing echo script.')
      return false
    else
      openOrEcho(event_data.message)
    end
    return true
end

listener:start()
