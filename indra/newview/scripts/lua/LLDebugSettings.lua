leap = require 'leap'

local LLDebugSettings = {}

function LLDebugSettings.set(name, value)
    return leap.request('LLViewerControl', {op='set', group='Global', key=name, value=value})
end

function LLDebugSettings.toggle(name)
    return leap.request('LLViewerControl', {op='toggle', group='Global', key=name})
end

function LLDebugSettings.get(name)
    return leap.request('LLViewerControl', {op='get', group='Global', key=name})['value']
end

return LLDebugSettings
