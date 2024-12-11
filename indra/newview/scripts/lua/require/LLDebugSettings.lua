local leap = require 'leap'

local LLDebugSettings = {}

function LLDebugSettings.set(name, value)
    leap.request('LLViewerControl', {op='set', group='Global', key=name, value=value})
end

function LLDebugSettings.toggle(name)
    leap.request('LLViewerControl', {op='toggle', group='Global', key=name})
end

function LLDebugSettings.get(name)
    return leap.request('LLViewerControl', {op='get', group='Global', key=name})['value']
end

function LLDebugSettings.getGraphicsQuality()
    return leap.request('UI', {op='getGraphicsQuality'}).level
end

-- from 0 (Low) to 6 (Ultra)
function LLDebugSettings.setGraphicsQuality(level)
    return leap.request('UI', {op='setGraphicsQuality', level=level})
end

return LLDebugSettings
