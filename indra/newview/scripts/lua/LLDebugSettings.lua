leap = require 'leap'

local function check_response(res)
  if res.error then
    error(res.error)
  end
  return res
end

local LLDebugSettings = {}

function LLDebugSettings.set(name, value)
    check_response(leap.request('LLViewerControl', {op='set', group='Global', key=name, value=value}))
end

function LLDebugSettings.toggle(name)
    check_response(leap.request('LLViewerControl', {op='toggle', group='Global', key=name}))
end

function LLDebugSettings.get(name)
    return check_response(leap.request('LLViewerControl', {op='get', group='Global', key=name}))['value']
end

return LLDebugSettings
