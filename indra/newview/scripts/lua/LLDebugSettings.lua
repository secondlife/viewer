leap = require 'leap'
util = require 'util'

local LLDebugSettings = {}

function LLDebugSettings.set(name, value)
    util.check_response(leap.request('LLViewerControl', {op='set', group='Global', key=name, value=value}))
end

function LLDebugSettings.toggle(name)
    util.check_response(leap.request('LLViewerControl', {op='toggle', group='Global', key=name}))
end

function LLDebugSettings.get(name)
    return util.check_response(leap.request('LLViewerControl', {op='get', group='Global', key=name}))['value']
end

return LLDebugSettings
