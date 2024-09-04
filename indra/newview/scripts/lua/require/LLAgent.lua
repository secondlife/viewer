local leap = require 'leap'
local mapargs = require 'mapargs'

local LLAgent = {}

function LLAgent.getRegionPosition()
    return leap.request('LLAgent', {op = 'getPosition'}).region
end

function LLAgent.getGlobalPosition()
    return leap.request('LLAgent', {op = 'getPosition'}).global
end

-- Return array information about the agent's groups
-- id: group id\n"
-- name: group name\n"
-- insignia: group insignia texture id
-- notices: bool indicating if this user accepts notices from this group
-- display: bool indicating if this group is listed in the user's profile
-- contrib: user's land contribution to this group
function LLAgent.getGroups()
    return leap.request('LLAgent', {op = 'getGroups'}).groups
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

-- Play specified animation by "item_id" locally
-- if "inworld" is specified as true, animation will be played inworld instead
function LLAgent.playAnimation(...)
    local args = mapargs('item_id,inworld', ...)
    args.op = 'playAnimation'
    return leap.request('LLAgent', args)
end

function LLAgent.stopAnimation(item_id)
    return leap.request('LLAgent', {op = 'stopAnimation', item_id=item_id})
end

-- Get animation info by "item_id"
-- reply contains "duration", "is_loop", "num_joints", "asset_id", "priority"
function LLAgent.getAnimationInfo(item_id)
    return leap.request('LLAgent', {op = 'getAnimationInfo', item_id=item_id}).anim_info
end

return LLAgent
