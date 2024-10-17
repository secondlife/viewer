local LLGesture = require 'LLGesture'
local startup = require 'startup'
local UI = require 'UI'

local flt = UI.Floater(
    "luafloater_gesture_list.xml",
    {gesture_list = {"double_click"}})

function flt:post_build(event_data)
    local gestures_uuid = LLGesture.getActiveGestures()
    local action_data = {}
    action_data.action = "add_list_element"
    action_data.ctrl_name = "gesture_list"
    local gestures = {}
    for uuid, info in pairs(gestures_uuid) do
      table.insert(gestures, {value = uuid, columns={column = "gesture_name", value = info.name}})
    end
    action_data.value = gestures
    self:post(action_data)
end

function flt:double_click_gesture_list(event_data)
    LLGesture.startGesture(event_data.value)
end

startup.wait('STATE_STARTED')
flt:show()
