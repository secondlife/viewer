local LLAgent = require 'LLAgent'
local LLListener = require 'LLListener'

local pos = LLAgent.getGlobalPosition()
pos[1]+=10 -- delta x
pos[2]+=5  -- delta y
LLAgent.requestStand()
LLAgent.startAutoPilot{target_global=pos,allow_flying=false,stop_distance=1}

local listener = LLListener(LLAgent.autoPilotPump)

function listener:handleMessages(event_data)
    if event_data.success then
      print('Destination is reached')
      LLAgent.requestSit()
    else
      print('Failed to reach destination')
    end
    return false
end

listener:start()
