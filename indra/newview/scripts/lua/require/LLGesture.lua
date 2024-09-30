-- Engage the LLGesture LLEventAPI

local leap = require 'leap'

local LLGesture = {}

function LLGesture.getActiveGestures()
    return leap.request('LLGesture', {op='getActiveGestures'})['gestures']
end

function LLGesture.isGesturePlaying(id)
    return leap.request('LLGesture', {op='isGesturePlaying', id=id})['playing']
end

function LLGesture.startGesture(id)
    leap.send('LLGesture', {op='startGesture', id=id})
end

function LLGesture.stopGesture(id)
    leap.send('LLGesture', {op='stopGesture', id=id})
end

return LLGesture
