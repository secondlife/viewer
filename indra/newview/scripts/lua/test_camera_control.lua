local LLAgent = require 'LLAgent'
local startup = require 'startup'
local UI = require 'UI'

local flt = UI.Floater('luafloater_camera_control.xml')

function getValue(ctrl_name)
    return flt:request({action="get_value", ctrl_name=ctrl_name}).value
end

function setValue(ctrl_name, value)
    flt:post({action="set_value", ctrl_name=ctrl_name, value=value})
end

function flt:commit_update_btn(event_data)
    lock_focus = getValue('lock_focus_ctrl')
    lock_camera = getValue('lock_camera_ctrl')
    local camera_pos = {getValue('cam_x'), getValue('cam_y'), getValue('cam_z')}
    local focus_pos = {getValue('focus_x'), getValue('focus_y'), getValue('focus_z')}

    LLAgent.setCamera{camera_pos=camera_pos, focus_pos=focus_pos,
                      focus_locked=lock_focus, camera_locked=lock_camera}

    self:post({action="add_text", ctrl_name="events_editor",
                value = {'Updating FollowCam params', 'camera_pos:', camera_pos, 'focus_pos:', focus_pos,
                          'lock_focus:', lock_focus, 'lock_camera:', lock_camera}})
end

function flt:commit_agent_cam_btn(event_data)
    agent_pos = LLAgent.getRegionPosition()
    setValue('cam_x', math.floor(agent_pos[1]))
    setValue('cam_y', math.floor(agent_pos[2]))
    setValue('cam_z', math.floor(agent_pos[3]))
end

function flt:commit_agent_focus_btn(event_data)
    agent_pos = LLAgent.getRegionPosition()
    setValue('focus_x', math.floor(agent_pos[1]))
    setValue('focus_y', math.floor(agent_pos[2]))
    setValue('focus_z', math.floor(agent_pos[3]))
end

function flt:commit_reset_btn(event_data)
    LLAgent.removeCamParams()
    LLAgent.setFollowCamActive(false)
end

startup.wait('STATE_STARTED')
flt:show()
