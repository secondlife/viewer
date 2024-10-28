local leap = require 'leap'
local startup = require 'startup'
local UI = require 'UI'

local flt = UI.Floater(
    'luafloater_demo.xml',
    {show_time_lbl = {"right_mouse_down", "double_click"}})

-- override base-class handleEvents() to report the event data in the floater's display field
function flt:handleEvents(event_data)
    self:post({action="add_text", ctrl_name="events_editor", value = event_data})
    -- forward the call to base-class handleEvents()
    return UI.Floater.handleEvents(self, event_data)
end

function flt:commit_disable_ctrl(event_data)
    self:post({action="set_enabled", ctrl_name="open_btn", value = not event_data.value})
end

function flt:commit_title_cmb(event_data)
    self:post({action="set_title", value=event_data.value})
end

function flt:commit_open_btn(event_data)
    floater_name = self:request({action="get_value", ctrl_name='openfloater_cmd'}).value
    leap.send("LLFloaterReg", {name = floater_name, op = "showInstance"})
end

local function getCurrentTime()
    local currentTime = os.date("*t")
    return string.format("%02d:%02d:%02d", currentTime.hour, currentTime.min, currentTime.sec)
end

function flt:double_click_show_time_lbl(event_data)
    self:post({action="set_value", ctrl_name="time_lbl", value=getCurrentTime()})
end

startup.wait('STATE_LOGIN_WAIT')
flt:show()
