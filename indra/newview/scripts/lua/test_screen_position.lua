LLAgent = require 'LLAgent'

local screen_pos = LLAgent.getAgentScreenPos()
if screen_pos.onscreen then
  print("Avatar screen coordinates X: " .. screen_pos.x .. " Y: " .. screen_pos.y)
else
  print("Avatar is not on the screen")
end
