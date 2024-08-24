local leap = require 'leap'
local startup = require 'startup'
local Timer = (require 'timers').Timer
local UI = require 'UI'
local popup = UI.popup
local max_speed = 0
local flt = UI.Floater("luafloater_speedometer.xml")
startup.wait('STATE_STARTED')

local timer

function flt:floater_close(event_data)
    if timer then
        timer:cancel()
    end
    popup:tip(string.format("Registered max speed: %.2f m/s", max_speed))
end

local function idle(event_data)
    local speed = leap.request('LLVOAvatar', {op='getSpeed'})['value']
    flt:post({action="set_value", ctrl_name="speed_lbl", value = string.format("%.2f", speed)})
    max_speed=math.max(max_speed, speed)
end

msg = 'Are you sure you want to run this "speedometer" script?'
response = popup:alertYesCancel(msg)

if response == 'OK' then
    flt:show()
    timer = Timer(1, idle, true) -- iterate
end
