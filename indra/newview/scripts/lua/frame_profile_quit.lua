-- Trigger Develop -> Render Tests -> Frame Profile and quit

assert(arg.n == 3, 'Usage: frame_profile_quit.lua x y z (for camera focus)')
-- Try coercing string arguments to numbers, using Lua's implicit conversion.
-- If the args passed as x, y, z won't convert nicely to numbers, better find
-- out now.
focus = {arg[1]+0, arg[2]+0, arg[3]+0}

LLAgent = require 'LLAgent'
logout = require 'logout'
startup = require 'startup'
Timer = (require 'timers').Timer
UI = require 'UI'

startup.wait('STATE_STARTED')

-- Figure out where we are
camera = LLAgent.getRegionPosition()
-- assume z is the agent's feet, add 2 meters
camera[2] += 2

Timer(10, 'wait')
LLAgent.setCamera{camera_pos=camera, camera_locked=true,
                  focus_pos=focus, focus_locked=true}
Timer(1, 'wait')
-- This freezes the viewer for perceptible realtime
UI.popup:tip('starting Render Tests -> Frame Profile')
UI.call("Advanced.ClickRenderProfile")
Timer(1, 'wait')
LLAgent.removeCamParams()
LLAgent.setFollowCamActive(false)

-- done
logout()
