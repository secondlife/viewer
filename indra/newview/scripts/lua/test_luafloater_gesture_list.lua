XML_FILE_PATH = "luafloater_gesture_list.xml"

leap = require 'leap'
coro = require 'coro'
LLGesture = require 'LLGesture'

--event pump for sending actions to the floater
COMMAND_PUMP_NAME = ""
--table of floater UI events
e={}
coro.launch(function ()
  e = leap.request("LLFloaterReg", {op="getFloaterEvents"})["events"]
  leap.done()
end)
leap.process()

function post(action)
  leap.send(COMMAND_PUMP_NAME, action)
end

function handleEvents(event_data)
  if event_data.event == e.CLOSE_EVENT then
    leap.done()
  elseif event_data.event == e.POST_BUILD_EVENT then
    COMMAND_PUMP_NAME = event_data.command_name
    gestures_uuid = LLGesture.getActiveGestures()
    local action_data = {}
    action_data.action = "add_list_element"
    action_data.ctrl_name = "gesture_list"
    gestures = {}
    for uuid, info in pairs(gestures_uuid) do
      element={value = uuid, columns ={column = "gesture_name", value = info.name}}
      action_data.value = element
      post(action_data)
    end
  elseif event_data.event == e.DOUBLE_CLICK_EVENT then
    if event_data.ctrl_name == "gesture_list" then
      LLGesture.startGesture(event_data.value)
    end
  end
end

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

local key = {xml_path = XML_FILE_PATH, op = "showLuaFloater"}
--receive additional events for defined control {<control_name>= {action1, action2, ...}}
key.extra_events={gesture_list = {e.DOUBLE_CLICK_EVENT}}
leap.send("LLFloaterReg", key, "floater1")

coro.launch(process_events, catch_events)
leap.process()
