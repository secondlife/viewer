-- Trigger Develop -> Render Tests -> Frame Profile

LLAgent = require 'LLAgent'
startup = require 'startup'
sleep = (require 'timers').sleep
UI = require 'UI'

startup.wait('STATE_STARTED')

-- teleport to http://maps.secondlife.com/secondlife/Bug%20Island/220/224/27
print(LLAgent.teleport{regionname='Bug Island', x=220, y=224, z=27})
sleep(10)
LLAgent.setCamera{camera_pos={220, 224, 26}, camera_locked=true,
                  focus_pos ={228, 232, 26}, focus_locked=true}
sleep(1)
-- This freezes the viewer for perceptible realtime
UI.popup:tip('starting Render Tests -> Frame Profile')
UI.call("Advanced.ClickRenderProfile")
sleep(1)
LLAgent.removeCamParams()
LLAgent.setFollowCamActive(false)

-- Home, James!
print(LLAgent.teleport('home'))
