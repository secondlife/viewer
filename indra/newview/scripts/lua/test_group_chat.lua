LLChat = require 'LLChat'
LLAgent = require 'LLAgent'
UI = require 'UI'

local GROUPS = LLAgent.getGroups()

-- Choose one of the groups randomly and send group message
math.randomseed(os.time())
group_info = GROUPS[math.random(#GROUPS)]
LLChat.startGroupChat(group_info.id)
response = UI.popup:alertYesCancel('Started group chat with ' .. group_info.name .. ' group. Send greetings?')
if response == 'OK' then
  LLChat.sendGroupIM('Greetings', group_info.id)
end
