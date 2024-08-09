LLChat = require 'LLChat'
inspect = require 'inspect'
LLAgent = require 'LLAgent'
popup = require 'popup'

local OK = 'OK_okcancelbuttons'
local GROUPS = LLAgent.getGroups()

-- Choose one of the groups randomly and send group message
math.randomseed(os.time())
group_info = GROUPS[math.random(#GROUPS)]
LLChat.startGroupChat(group_info.id)
response = popup:alertYesCancel('Started group chat with ' .. group_info.name .. ' group. Send greetings?')
if next(response) == OK then
  LLChat.sendGroupIM('Greetings', group_info.id)
end
