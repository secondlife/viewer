-- Engage the UI LLEventAPI

leap = require 'leap'
util = require 'util'

local UI = {}

function UI.call(func, parameter)
    -- 'call' is fire-and-forget
    util.check_response(leap.request('UI', {op='call', ['function']=func, parameter=parameter}))
end

function UI.getValue(path)
    return util.check_response(leap.request('UI', {op='getValue', path=path}))['value']
end

return UI
