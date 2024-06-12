leap = require 'leap'

-- notification is any name defined in notifications.xml as
-- <notification name=>
-- vars is a table providing values for [VAR] substitution keys in the
-- notification body.
local popup_meta = {
    -- setting this function as getmetatable(popup).__call() means this gets
    -- called when a consumer calls popup(notification, vars, payload)
    __call = function(self, notification, vars, payload)
        return leap.request('LLNotifications',
                            {op='requestAdd', name=notification,
                             substitutions=vars,
                             payload=payload})
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

return popup
