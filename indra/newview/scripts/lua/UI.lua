-- Engage the UI LLEventAPI

leap = require 'leap'

local UI = {}

function UI.call(func, parameter)
    -- 'call' is fire-and-forget
    leap.send('UI', {op='call', ['function']=func, parameter=parameter})
end

function UI.getValue(path)
    return leap.request('UI', {op='getValue', path=path})['value']
end

return UI
