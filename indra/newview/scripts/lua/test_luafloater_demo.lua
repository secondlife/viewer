XML_FILE_PATH = "luafloater_demo.xml"

leap = require 'leap'
coro = require 'coro'

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

function getCurrentTime()
    local currentTime = os.date("*t")
    return string.format("%02d:%02d:%02d", currentTime.hour, currentTime.min, currentTime.sec)
end

function handleEvents(event_data)
  if event_data.event == e.COMMIT_EVENT then
    if event_data.ctrl_name == "disable_ctrl" then
      post({action="set_enabled", ctrl_name="open_btn", value = (1 - event_data.value)})
    elseif event_data.ctrl_name == "title_cmb" then
      post({action="set_title", value= event_data.value})
    elseif event_data.ctrl_name == "open_btn" then
      floater_name = leap.request(COMMAND_PUMP_NAME, {action="get_value", ctrl_name='openfloater_cmd'})['value']
      leap.send("LLFloaterReg", {name = floater_name, op = "showInstance"})
    end
  elseif event_data.event == e.DOUBLE_CLICK_EVENT then
    if event_data.ctrl_name == "show_time_lbl" then
      post({action="set_value", ctrl_name="time_lbl", value= getCurrentTime()})
    end
  elseif event_data.event == e.CLOSE_EVENT then
    print_warning("Floater was closed")
    leap.done()
  --script received event pump name, after floater was built
  elseif event_data.event == e.POST_BUILD_EVENT then
    COMMAND_PUMP_NAME = event_data.command_name
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

--sign for additional events for defined control {<control_name>= {action1, action2, ...}}
key.extra_events={show_time_lbl = {e.RIGHT_MOUSE_DOWN_EVENT, e.DOUBLE_CLICK_EVENT}}
leap.send("LLFloaterReg", key, "floater1")

coro.launch(process_events, catch_events)
leap.process()
print_warning("End of the script")
