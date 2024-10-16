local leap = require 'leap'
local mapargs = require 'mapargs'
local result_view = require 'result_view'

local function result(keys)
    local result_table = {
        result=result_view(keys.result),
        -- call result_table:close() to release result sets before garbage
        -- collection or script completion
        close = function(self)
            result_view.close(keys.result[1])
        end
    }
    -- When the result_table is destroyed, close its result_views.
    return LL.setdtor('LLAgent result', result_table, result_table.close)
end

local LLAgent = {}

function LLAgent.getID()
    return leap.request('LLAgent', {op = 'getID'}).id
end

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

-- Teleport to specified "regionname" at specified region-relative "x", "y", "z".
-- If "regionname" is "home", ignore "x", "y", "z" and teleport home.
-- If "regionname" omitted, teleport to GLOBAL coordinates "x", "y", "z".
function LLAgent.teleport(...)
    local args = mapargs('regionname,x,y,z', ...)
    args.op = 'teleport'
    return leap.request('LLTeleportHandler', args).message
end

-- Call with no arguments to sit on the ground.
-- Otherwise specify "obj_uuid" to sit on,
-- or region "position" {x, y, z} where to find closest object to sit on.
-- For example: LLAgent.requestSit{position=LLAgent.getRegionPosition()}
-- Your avatar should be close enough to the object you want to sit on
function LLAgent.requestSit(...)
    local args = mapargs('obj_uuid,position', ...)
    args.op = 'requestSit'
    return leap.request('LLAgent', args)
end

function LLAgent.requestStand()
    leap.send('LLAgent', {op = 'requestStand'})
end

-- Get the nearby avatars in a range of provided "dist",
-- if "dist" is not specified, "RenderFarClip" setting is used
-- reply will contain "result" table with following fields:
-- "id", "global_pos", "region_pos",  "name", "region_id"
function LLAgent.getNearbyAvatarsList(...)
    local args = mapargs('dist', ...)
    args.op = 'getNearbyAvatarsList'
    return result(leap.request('LLAgent', args))
end

-- reply will contain "result" table with following fields:
-- "id", "global_pos", "region_pos", "region_id"
function LLAgent.getNearbyObjectsList(...)
    local args = mapargs('dist', ...)
    args.op = 'getNearbyObjectsList'
    return result(leap.request('LLAgent', args))
end

-- Get screen position of your own avatar or any other (if "avatar_id" is specified)
-- reply contains "x", "y" coordinates and "onscreen" flag to indicate if it's actually in within the current window
-- avatar render position is used as the point
function LLAgent.getAgentScreenPos(...)
    local args = mapargs('avatar_id', ...)
    args.op = 'getAgentScreenPos'
    return leap.request('LLAgent', args)
end

-- ***************************************************************************
--  Autopilot
-- ***************************************************************************
LLAgent.autoPilotPump = "LLAutopilot"

-- Start the autopilot to move to "target_global" location using specified parameters
-- LLAgent.startAutoPilot{ target_global       array of target global {x, y, z} position
--                      [, allow_flying]       allow flying during autopilot [default: true]
--                      [, stop_distance]      target maximum distance from target [default: autopilot guess]
--                      [, behavior_name]      name of the autopilot behavior [default: (script name)]
--                      [, target_rotation]    array of [x, y, z, w] quaternion values [default: no target]
--                      [, rotation_threshold] target maximum angle from target facing rotation [default: 0.03 radians]
-- an event with "success" flag is sent to "LLAutopilot" event pump, when auto pilot is terminated
function LLAgent.startAutoPilot(...)
    local args = mapargs('target_global,allow_flying,stop_distance,behavior_name,target_rotation,rotation_threshold', ...)
    args.op = 'startAutoPilot'
    leap.send('LLAgent', args)
end

-- Update target location for currently running autopilot
function LLAgent.setAutoPilotTarget(target_global)
    leap.send('LLAgent', {op = 'setAutoPilotTarget', target_global=target_global})
end

-- Start the autopilot to move to the specified target location
-- either "leader_id" (uuid of target) or "avatar_name" (avatar full name: use just first name for 'Resident') should be specified
-- "allow_flying" [default: true], "stop_distance" [default: autopilot guess]
function LLAgent.startFollowPilot(...)
    local args = mapargs('leader_id,avatar_name,allow_flying,stop_distance', ...)
    args.op = 'startFollowPilot'
    return leap.request('LLAgent', args)
end

-- Stop the autopilot system: "user_cancel" indicates whether or not to act as though user canceled autopilot [default: false]
function LLAgent.stopAutoPilot(...)
    local args = mapargs('user_cancel', ...)
    args.op = 'stopAutoPilot'
    leap.send('LLAgent', args)
end

-- Get information about current state of the autopilot
function LLAgent.getAutoPilot()
    return leap.request('LLAgent', {op = 'getAutoPilot'})
end

return LLAgent
