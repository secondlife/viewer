-- Trigger Develop -> Render Tests -> Frame Profile and quit

LLAgent = require 'LLAgent'
logout = require 'logout'
startup = require 'startup'
Timer = (require 'timers').Timer
UI = require 'UI'

startup.wait('STATE_STARTED')

-- Assume we logged into http://maps.secondlife.com/secondlife/Bug%20Island/220/224/27
-- (see frame_profile bash script)
Timer(10, 'wait')
LLAgent.setCamera{camera_pos={220, 224, 26}, camera_locked=true,
                  focus_pos ={228, 232, 26}, focus_locked=true}
Timer(1, 'wait')
-- This freezes the viewer for perceptible realtime
UI.popup:tip('starting Render Tests -> Frame Profile')
UI.call("Advanced.ClickRenderProfile")
Timer(1, 'wait')
LLAgent.removeCamParams()
LLAgent.setFollowCamActive(false)

-- done
logout()
