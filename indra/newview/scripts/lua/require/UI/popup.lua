local leap = require 'leap'
local mapargs = require 'mapargs'
local util = require 'util'

-- notification is any name defined in notifications.xml as
-- <notification name=>
-- vars is a table providing values for [VAR] substitution keys in the
-- notification body
-- payload prepopulates the response table
-- wait=false means fire and forget, returning nil
-- wait=true waits for user response:
-- * If the viewer returns a table containing exactly one key=true pair,
--   popup() returns just that key. If the key is a string containing an
--   underscore, e.g. 'OK_okcancelbuttons', it's truncated at the first
--   underscore, e.g. 'OK'.
-- * Otherwise the viewer's response is returned unchanged. To suppress the
--   above transformations, pass a non-empty payload table; this will cause
--   the viewer to return a table with at least two keys.
local popup = util.setmetamethods{
    -- this gets called when a consumer calls popup(notification, vars, payload)
    __call = function(self, ...)
        local args = mapargs('notification,vars,payload,wait', ...)
        -- we use convenience argument names different from 'LLNotifications'
        -- listener
        newargs = {op='requestAdd',
                   name=args.notification,
                   substitutions=args.vars,
                   payload=args.payload}
        -- Specifically test (wait == false), NOT (not wait), because we treat
        -- nil (omitted, default true) differently than false (explicitly
        -- DON'T wait).
        if args.wait == false then
            leap.send('LLNotifications', newargs)
        else
            local response = leap.request('LLNotifications', newargs).response
            -- response is typically a table. It might have multiple keys,
            -- e.g. if caller passed non-empty payload. In that case, just
            -- return the whole thing.
            if type(response) ~= 'table' then
                return response
            end
            -- get first key=value pair, if any
            local key, value = next(response)
            if (not key) or next(response, key) then
                -- key == nil means response is empty
                -- next(response, non-nil first key) ~= nil means at least two keys
                return response
            end
            -- Here response is a table containing exactly one key. The
            -- notifications system typically returns a table of the form
            -- {OK_okcancelbuttons=true}, which is tricky to test for because it
            -- varies with each set of buttons.
            if value == true then
                -- change {key=true} to plain key
                response = key
                if type(response) == 'string' then
                    -- change 'OK_okcancelbuttons' to plain 'OK'
                    response = string.split(response, '_')[1]
                end
            end
            return response
        end
    end
}

function popup:alert(message, payload)
    return self('GenericAlert', {MESSAGE=message, payload=payload})
end

function popup:alertOK(message, payload)
    return self('GenericAlertOK', {MESSAGE=message, payload=payload})
end

function popup:alertYesCancel(message, payload)
    return self('GenericAlertYesCancel', {MESSAGE=message, payload=payload})
end

function popup:tip(message, payload)
    self{'SystemMessageTip', {MESSAGE=message, payload=payload}, wait=false}
end

return popup
