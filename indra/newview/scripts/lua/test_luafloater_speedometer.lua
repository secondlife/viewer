local Floater = require 'Floater'
local startup = require 'startup'
inspect = require 'inspect'
leap = require 'leap'
LLNotification = require 'LLNotification'
local max_speed = 0
local flt = Floater:new("luafloater_speedometer.xml")
startup.wait('STATE_STARTED')

function flt:floater_close(event_data)
    msg = "Registered max speed: " .. string.format("%.2f", max_speed) .. " m/s";
    LLNotification.add('SystemMessageTip', {MESSAGE = msg})
end

function flt:idle(event_data)
    local speed = leap.request('LLVOAvatar', {op='getSpeed'})['value']
    flt:post({action="set_value", ctrl_name="speed_lbl", value = string.format("%.2f", speed)})
    max_speed=math.max(max_speed, speed)
end

msg = 'Are you sure you want to run this "speedometer" script?'
response = LLNotification.requestAdd('GenericAlertYesCancel', {MESSAGE = msg})

if response.OK_okcancelbuttons then
  flt:show()
end
