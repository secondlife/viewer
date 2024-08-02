local leap = require 'leap'
local mapargs = require 'mapargs'

local LLAgent = {}

function LLAgent.getRegionPosition()
    return leap.request('LLAgent', {op = 'getPosition'}).region
end

function LLAgent.getGlobalPosition()
    return leap.request('LLAgent', {op = 'getPosition'}).global
end

-- Use LL.leaphelp('LLAgent') and see 'setCameraParams' to get more info about params
--                                          -- TYPE               -- DEFAULT      -- RANGE
-- LLAgent.setCamera{ [, camera_pos]        -- vector3
--                    [, focus_pos]         -- vector3
--                    [, focus_offset]      -- vector3            -- {1,0,0}      -- {-10,-10,-10} to {10,10,10}
--                    [, distance]          -- float (meters)     -- 3            -- 0.5 to 50
--                    [, focus_threshold]   -- float (meters)     -- 1            -- 0 to 4
--                    [, camera_threshold]  -- float (meters)     -- 1            -- 0 to 4
--                    [, focus_lag]         -- float (seconds)    -- 0.1          -- 0 to 3
--                    [, camera_lag]        -- float (seconds)    -- 0.1          -- 0 to 3
--                    [, camera_pitch]      -- float (degrees)    -- 0            -- -45 to 80
--                    [, behindness_angle]  -- float (degrees)    -- 10           -- 0 to 180
--                    [, behindness_lag]    -- float (seconds)    -- 0            -- 0 to 3
--                    [, camera_locked]     -- bool               -- false
--                    [, focus_locked]}     -- bool               -- false
function LLAgent.setCamera(...)
    local args = mapargs('camera_pos,focus_pos,focus_offset,focus_lag,camera_lag,' ..
                         'distance,focus_threshold,camera_threshold,camera_pitch,' ..
                         'camera_locked,focus_locked,behindness_angle,behindness_lag', ...)
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
