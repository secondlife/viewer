XML_FILE_PATH = "luafloater_gesture_list.xml"

leap = require 'leap'
coro = require 'coro'
util = require 'util'
LLGesture = require 'LLGesture'

--event pump for sending actions to the floater
COMMAND_PUMP_NAME = ""
--table of floater UI events
event_list={}
coro.launch(function ()
  event_list = leap.request("LLFloaterReg", {op="getFloaterEvents"})["events"]
  leap.done()
end)
leap.process()

local function _event(event_name)
  if not util.contains(event_list, event_name) then
    print_warning("Incorrect event name: " .. event_name)
  end
  return event_name
end

function post(action)
  leap.send(COMMAND_PUMP_NAME, action)
end

function handleEvents(event_data)
  if event_data.event == _event("floater_close") then
    leap.done()
  elseif event_data.event == _event("post_build") then
    COMMAND_PUMP_NAME = event_data.command_name
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
end

local key = {xml_path = XML_FILE_PATH, op = "showLuaFloater"}
--receive additional events for defined control {<control_name>= {action1, action2, ...}}
key.extra_events={gesture_list = {_event("double_click")}}
coro.launch(function ()
  handleEvents(leap.request("LLFloaterReg", key))
  leap.done()
end)
leap.process()

catch_events = leap.WaitFor:new(-1, "all_events")
function catch_events:filter(pump, data)
  return data
end

function process_events(waitfor)
  event_data = waitfor:wait()
  while event_data do
    handleEvents(event_data)
    event_data = waitfor:wait()
  end
end

coro.launch(process_events, catch_events)
leap.process()
