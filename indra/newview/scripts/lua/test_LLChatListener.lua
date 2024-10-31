local LLAgent = require 'LLAgent'
local LLChat = require 'LLChat'
local LLListener = require 'LLListener'
local UI = require 'UI'

-- Chat listener script allows to use the following commands in Nearby chat:
-- open inventory   -- open defined floater by name
-- close inventory  -- close defined floater by name
-- closeall         -- close all floaters
-- stop             -- close the script
-- any other messages will be echoed.
function openOrEcho(message)
    local open_floater_name = string.match(message, "^open%s+(%w+)")
    local close_floater_name = string.match(message, "^close%s+(%w+)")
    if open_floater_name then
      UI.showFloater(open_floater_name)
    elseif close_floater_name then
      UI.hideFloater(close_floater_name)
    elseif message == 'closeall' then
      UI.closeAllFloaters()
    else
      LLChat.sendNearby('Echo: ' .. message)
    end
end

local listener = LLListener(LLChat.nearbyChatPump)
local agent_id = LLAgent.getID()

function listener:handleMessages(event_data)
    -- ignore messages and commands from other avatars
    if event_data.from_id ~= agent_id then
      return true
    elseif string.find(event_data.message, '[LUA]') then
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
