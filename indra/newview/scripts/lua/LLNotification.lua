-- Engage the LLNotificationsListener LLEventAPI

leap = require 'leap'

local LLNotification = {}

function LLNotification.add(name, substitutions)
    leap.send('LLNotifications', {op='requestAdd', name=name, substitutions=substitutions})
end

function LLNotification.requestAdd(name, substitutions)
    return leap.request('LLNotifications', {op='requestAdd', name=name, substitutions=substitutions})['response']
end

return LLNotification
