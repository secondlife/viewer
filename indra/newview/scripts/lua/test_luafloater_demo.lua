XML_FILE_PATH = "luafloater_demo.xml"

leap = require 'leap'
fiber = require 'fiber'

--event pump for sending actions to the floater
COMMAND_PUMP_NAME = ""
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

function getCurrentTime()
    local currentTime = os.date("*t")
    return string.format("%02d:%02d:%02d", currentTime.hour, currentTime.min, currentTime.sec)
end

function handleEvents(event_data)
  post({action="add_text", ctrl_name="events_editor", value = event_data})
  if event_data.event == _event("commit") then
    if event_data.ctrl_name == "disable_ctrl" then
      post({action="set_enabled", ctrl_name="open_btn", value = (1 - event_data.value)})
    elseif event_data.ctrl_name == "title_cmb" then
      post({action="set_title", value= event_data.value})
    elseif event_data.ctrl_name == "open_btn" then
      floater_name = leap.request(COMMAND_PUMP_NAME, {action="get_value", ctrl_name='openfloater_cmd'})['value']
      leap.send("LLFloaterReg", {name = floater_name, op = "showInstance"})
    end
  elseif event_data.event == _event("double_click") then
    if event_data.ctrl_name == "show_time_lbl" then
      post({action="set_value", ctrl_name="time_lbl", value= getCurrentTime()})
    end
  elseif event_data.event == _event("floater_close") then
    LL.print_warning("Floater was closed")
    leap.done()
  end
end

local key = {xml_path = XML_FILE_PATH, op = "showLuaFloater"}
--sign for additional events for defined control {<control_name>= {action1, action2, ...}}
key.extra_events={show_time_lbl = {_event("right_mouse_down"), _event("double_click")}}
COMMAND_PUMP_NAME = leap.request("LLFloaterReg", key).command_name

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

fiber.launch("catch_events", process_events, catch_events)