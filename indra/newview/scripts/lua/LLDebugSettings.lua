leap = require 'leap'

local LLDebugSettings = {}

function LLDebugSettings.set(name, value)
    leap.send('LLAppViewer', {op='setDebugSetting', setting=name, value=value})
end

function LLDebugSettings.get(name)
    return leap.request('LLAppViewer', {op='getDebugSetting', setting=name})['value']
end

return LLDebugSettings
