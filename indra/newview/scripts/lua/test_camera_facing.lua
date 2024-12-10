local LLAgent = require 'LLAgent'
local timers = require 'timers'

-- set follow camera in front of avatar's face

local distance = 2.0
local height = 1.0
local yaw_rad = LLAgent.getYaw()

local agent_pos = LLAgent.getRegionPosition()
agent_pos[1] += distance * math.cos(yaw_rad)
agent_pos[2] += distance * math.sin(yaw_rad)
agent_pos[3] += height
LLAgent.setCamera{ camera_pos=agent_pos, camera_locked=true }

-- disable follow cam after 5 seconds
timers.Timer(5, function() LLAgent.setFollowCamActive(false) end)
