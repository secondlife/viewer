local leap = require 'leap'
local mapargs = require 'mapargs'

-- notification is any name defined in notifications.xml as
-- <notification name=>
-- vars is a table providing values for [VAR] substitution keys in the
-- notification body
-- payload prepopulates the response table
-- wait=false means fire and forget, otherwise wait for user response
local popup_meta = {
    -- setting this function as getmetatable(popup).__call() means this gets
    -- called when a consumer calls popup(notification, vars, payload)
    __call = function(self, ...)
        local args = mapargs('notification,vars,payload,wait', ...)
        -- we use convenience argument names different from 'LLNotifications'
        -- listener
        args.name = args.notification
        args.notification = nil
        args.substitutions = args.vars
        args.vars = nil
        local wait = args.wait
        args.wait = nil
        args.op = 'requestAdd'
        -- Specifically test (wait == false), NOT (not wait), because we treat
        -- nil (omitted, default true) differently than false (explicitly
        -- DON'T wait).
        if wait == false then
            leap.send('LLNotifications', args)
        else
            return leap.request('LLNotifications', args).response
        end
    end
}

local popup = setmetatable({}, popup_meta)

function popup:alert(message)
    return self('GenericAlert', {MESSAGE=message})
end

function popup:alertOK(message)
    return self('GenericAlertOK', {MESSAGE=message})
end

function popup:alertYesCancel(message)
    return self('GenericAlertYesCancel', {MESSAGE=message})
end

function popup:tip(message)
    self{'SystemMessageTip', {MESSAGE=message}, wait=false}
end

return popup
