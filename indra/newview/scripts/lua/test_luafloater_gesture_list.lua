XML_FILE_PATH = LL.abspath("luafloater_gesture_list.xml")

scriptparts = string.split(LL.source_path(), '/')
scriptname = scriptparts[#scriptparts]
print('Running ' .. scriptname)

leap = require 'leap'
fiber = require 'fiber'
LLGesture = require 'LLGesture'
startup = require 'startup'

--event pump for sending actions to the floater
local COMMAND_PUMP_NAME = ""
local reqid
--table of floater UI events
event_list=leap.request("LLFloaterReg", {op="getFloaterEvents"}).events

local function _event(event_name)
  if not table.find(event_list, event_name) then
    LL.print_warning("Incorrect event name: " .. event_name)
  end
  return event_name
end

function post(action)
  leap.send(COMMAND_PUMP_NAME, action)
end

function handleEvents(event_data)
  if event_data.event == _event("floater_close") then
    return false
  end

  if event_data.event == _event("post_build") then
    COMMAND_PUMP_NAME = event_data.command_name
    reqid = event_data.reqid
    gestures_uuid = LLGesture.getActiveGestures()
    local action_data = {}
    action_data.action = "add_list_element"
    action_data.ctrl_name = "gesture_list"
    gestures = {}
    for uuid, info in pairs(gestures_uuid) do
      table.insert(gestures, {value = uuid, columns ={column = "gesture_name", value = info.name}})
    end
    action_data.value = gestures
    post(action_data)
  elseif event_data.event == _event("double_click") then
    if event_data.ctrl_name == "gesture_list" then
      LLGesture.startGesture(event_data.value)
    end
  end
  return true
end

startup.wait('STATE_STARTED')
local key = {xml_path = XML_FILE_PATH, op = "showLuaFloater"}
--receive additional events for defined control {<control_name>= {action1, action2, ...}}
key.extra_events={gesture_list = {_event("double_click")}}
handleEvents(leap.request("LLFloaterReg", key))

catch_events = leap.WaitFor:new(-1, "all_events")
function catch_events:filter(pump, data)
  if data.reqid == reqid then
    return data
  end
end

function process_events(waitfor)
  event_data = waitfor:wait()
  while event_data and handleEvents(event_data) do
    event_data = waitfor:wait()
  end
end

fiber.launch("catch_events", process_events, catch_events)
