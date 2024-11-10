-- Trigger Develop -> Render Tests -> Frame Profile

LLAgent = require 'LLAgent'
startup = require 'startup'
Timer = (require 'timers').Timer
UI = require 'UI'
teleport_util = require('teleport_util')

startup.wait('STATE_STARTED')

-- teleport to http://maps.secondlife.com/secondlife/Bug%20Island/220/224/27
print(LLAgent.teleport{regionname='Bug Island', x=220, y=224, z=27})
--teleport_util.wait()
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

-- Home, James!
print(LLAgent.teleport('home'))
