local leap = require 'leap'

local LLChat = {}

-- ***************************************************************************
--  Nearby chat
-- ***************************************************************************
LLChat.nearbyChatPump = "LLNearbyChat"

-- 0 is public nearby channel, other channels are used to communicate with LSL scripts
function LLChat.sendNearby(msg, channel)
    leap.send('LLChatBar', {op='sendChat', message=msg, channel=channel})
end

function LLChat.sendWhisper(msg)
    leap.send('LLChatBar', {op='sendChat', type='whisper', message=msg})
end

function LLChat.sendShout(msg)
    leap.send('LLChatBar', {op='sendChat', type='shout', message=msg})
end

-- ***************************************************************************
--  Group chat
-- ***************************************************************************

function LLChat.startGroupChat(group_id)
    return leap.request('GroupChat', {op='startGroupChat', group_id=group_id})
end

function LLChat.leaveGroupChat(group_id)
    leap.send('GroupChat', {op='leaveGroupChat', group_id=group_id})
end

function LLChat.sendGroupIM(msg, group_id)
    leap.send('GroupChat', {op='sendGroupIM', message=msg, group_id=group_id})
end

return LLChat
