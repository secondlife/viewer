local leap = require 'leap'
local mapargs = require 'mapargs'

local LLAgent = {}

function LLAgent.getRegionPosition()
    return leap.request('LLAgent', {op = 'getPosition'}).region
end

function LLAgent.getGlobalPosition()
    return leap.request('LLAgent', {op = 'getPosition'}).global
end

function LLAgent.setCamera(...)
    local args = mapargs('camera_pos,focus_pos,focus_offset,camera_locked,focus_locked', ...)
    args.op = 'setCameraParams'
    leap.send('LLAgent', args)
end

function LLAgent.setFollowCamActive(active)
    leap.send('LLAgent', {op = 'setFollowCamActive', active = active})
end

function LLAgent.removeCamParams()
    leap.send('LLAgent', {op = 'removeCameraParams'})
end

return LLAgent
