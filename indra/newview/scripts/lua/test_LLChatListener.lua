local LLAgent = require 'LLAgent'
local LLAppearance = require 'LLAppearance'
local LLChat = require 'LLChat'
local LLListener = require 'LLListener'
local UI = require 'UI'

-- Chat listener script allows to use the following commands in Nearby chat:
-- /open inventory   -- open defined floater by name
-- /close inventory  -- close defined floater by name
-- /closeall         -- close all floaters
-- /outfit           -- wear specified outfit
-- /help             -- show help
-- /stop             -- close the script
function processCommands(message)
    print("handling message " .. message)
    local cmd, arg = string.match(message, '^/(%w+)%s+(.*)$')
    if arg then
      -- remove quotes from arg
      arg = string.gsub(arg, '"', '')
    else
      cmd = string.match(message, '^/(%w+)')
    end
    print("cmd: " .. (cmd or '') .. " arg: " .. (arg or ''))
    if cmd == 'open' then
      if arg then
        LLChat.sendNearby('Opening floater ' .. arg)
        UI.showFloater(arg)
      else
        LLChat.sendNearby('Usage: /open <floater_name>')
      end
    elseif cmd == 'close' then
      if arg then
        UI.hideFloater(arg)
      else
        LLChat.sendNearby('Usage: /close <floater_name>')
      end
    elseif cmd == 'outfit' then
      if arg then
        LLAppearance.wearOutfitByName(arg, 'replace')
      else
        LLChat.sendNearby('Usage: /outfit <outfit_name>')
      end
    elseif cmd == 'closeall' then
      UI.closeAllFloaters()
    elseif cmd == 'help' then
      LLChat.sendNearby('Commands: /open <floater_name>, /close <floater_name>, /outfit <outfit_name>, /closeall, /stop')
    end
end

local listener = LLListener(LLChat.nearbyChatPump)
local agent_id = LLAgent.getID()

function listener:handleMessages(event_data)
    -- ignore messages and commands from other avatars
    if event_data.from_id ~= agent_id then
      return true
    elseif string.sub(event_data.message, 1, 5) == '[LUA]' then
      return true
    elseif event_data.message == '/stop' then
      LLChat.sendNearby('Closing echo script.')
      return false
    else
      ok, ret = pcall(processCommands, event_data.message)
      if not ok then
        LLChat.sendNearby('Error: ' .. ret)
      end
    end
    return true
end

listener:start()
