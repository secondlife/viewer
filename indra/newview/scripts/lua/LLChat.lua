local leap = require 'leap'

local LLChat = {}

function LLChat.sendNearby(msg)
    leap.send('LLChatBar', {op='sendChat', message=msg})
end

function LLChat.sendWhisper(msg)
    leap.send('LLChatBar', {op='sendChat', type='whisper', message=msg})
end

function LLChat.sendShout(msg)
    leap.send('LLChatBar', {op='sendChat', type='shout', message=msg})
end

return LLChat
