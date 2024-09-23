-- Make camera fly around the subject avatar for a few seconds.

local LLAgent = require 'LLAgent'
local startup = require 'startup'
local timers = require 'timers'

local height = 2.0 -- meters
local radius = 4.0 -- meters
local speed = 1.0 -- meters/second along circle
local start = os.clock()
local stop  = os.clock() + 30 -- seconds

local function cameraPos(t)
    local agent = LLAgent.getRegionPosition()
    local radians = speed * t
    return {
        agent[1] + radius * math.cos(radians),
        agent[2] + radius * math.sin(radians),
        agent[3] + height
    }
end

local function moveCamera()
    if os.clock() < stop then
        -- usual case
        LLAgent.setCamera{ camera_pos=cameraPos(os.clock() - start), camera_locked=true }
        return nil
    else
        -- last time
        LLAgent.removeCamParams()
        LLAgent.setFollowCamActive(false)
        return true
    end
end

startup.wait('STATE_STARTED')
-- call moveCamera() repeatedly until it returns true
local timer = timers.Timer(0.1, moveCamera, true)
