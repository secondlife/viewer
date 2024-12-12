-- Teleport to a target region, visit certain viewpoints and take snapshots.

LLAgent = require 'LLAgent'
LLDebugSettings = require 'LLDebugSettings'
LLFloaterAbout = require 'LLFloaterAbout'
LLListener = require 'LLListener'
teleport_util = require 'teleport_util'
timers = require 'timers'
UI = require 'UI'
util = require 'util'
inspect = require 'inspect'

-- 'http://maps.secondlife.com/secondlife/TextureTest/137/112/23'
REGION = {regionname='TextureTest', x=137, y=112, z=23}

SNAPDIR = UI.getTempDir()

-- save the user's initial graphics quality
quality = LLDebugSettings.getGraphicsQuality()
print(`Original graphics quality: {quality}`)

STOPFLAG = false

snapshot_list={}

floater = UI.Floater("luafloater_preview_demo.xml")

function floater:floater_close(event)
    STOPFLAG = true
    return false                -- terminate the floater's listen loop
end

function floater:commit_location_cmb(event_data)
     self:child_set_value("preview_icon", snapshot_list[event_data.value][self:child_get_value('quality_cmb1') + 1])
     self:child_set_value("preview_icon2", snapshot_list[event_data.value][self:child_get_value('quality_cmb2') + 1])
end

function floater:commit_quality_cmb1(event_data)
    self:child_set_value("preview_icon", snapshot_list[self:child_get_value('location_cmb')][event_data.value + 1])
end

function floater:commit_quality_cmb2(event_data)
    self:child_set_value("preview_icon2", snapshot_list[self:child_get_value('location_cmb')][event_data.value + 1])
end

-- A few interesting things in TextureTest region.
-- 'lookat' is the point, 'from' is the offset from which to regard it.
lookats = {
    {lookat={123, 110, 23}, from={0,  0, 4.5}},
    {lookat={112, 108, 24}, from={0,  0, 4}},
--  {lookat={242,  25, 38}, from={2, 10, 0}},
--  {lookat={162,  10, 23}, from={0,  4, 0}},
}

-- Teleport to the target region
if LLFloaterAbout.getInfo().REGION ~= REGION.regionname then
    LLAgent.teleport(REGION)
    teleport_util.wait()
    -- wait some more: teleport_util sometimes returns too soon
    timers.sleep(3)
else
    -- we're already on this region; but strangely, it matters for camera
    -- placement specifically where we are
    local localspot = vector.create(REGION.x, REGION.y, REGION.z)
    if vector.magnitude(util.tovector(LLAgent.getRegionPosition()) - localspot) > 10 then
        LLListener(LLAgent.autoPilotPump):next_event(
            LLAgent.startAutoPilot,
            {target_global=LLAgent.localToGlobal(localspot), allow_flying=true})
    end
end

-- put up the empty preview floater
floater:show()


-- base filename for the snapshots we'll take this run
local testbase = `{REGION.regionname}_{os.date('%Y-%m-%d_%H-%M-%S')}_`
for _, view in lookats do

    lookat_str="{" .. table.concat(view.lookat, ", ") .. "}"
    snapshot_list[lookat_str]={}
    floater:post({action="add_item", ctrl_name="location_cmb", name=lookat_str, value=lookat_str})
    floater:post({action="select_by_value", ctrl_name="location_cmb", value=lookat_str})

    -- Hop the camera to the next interesting lookat point
    lookat_txt=`Looking at \{{view.lookat[1]}, {view.lookat[2]}, {view.lookat[3]}}`
    print(lookat_txt)
    LLAgent.setCamera{
        camera_pos=util.fromvector(util.tovector(view.lookat) + util.tovector(view.from)),
        focus_pos=view.lookat,
        camera_locked=true,
        focus_locked=true}
    -- cycle through various levels of graphics quality
    for q = 0, 6 do
        if STOPFLAG then
            print('Aborting snapshot sequence')
            break
        end
        quality_txt=`quality {q}`
        print('    '..quality_txt)
        -- set the quality level
        LLDebugSettings.setGraphicsQuality(q)
        -- workaround for weird bug (?) in which the new RenderFarClip setting
        -- changed by the graphics quality affects camera zoom
        LLDebugSettings.set('RenderFarClip', 128)
        -- let the rendering settle
        timers.sleep(2)
        -- take a snapshot
        basename = `{testbase}{view.lookat[1]}-{view.lookat[2]}-{view.lookat[3]}-q{q}.png`
        fullname = `{SNAPDIR}/{basename}`
        if UI.snapshot{fullname, showui = false, showhud = false} then
            -- display it in our floater
            tex_id = UI.uploadLocalTexture(fullname)
            snapshot_list[lookat_str][q+1]=tex_id
            if q == 0 then
              floater:child_set_value("preview_icon", tex_id)
              floater:post({action="select_by_value", ctrl_name="quality_cmb1", value=q})
            end
            floater:child_set_value("preview_icon2", tex_id)
            floater:post({action="select_by_value", ctrl_name="quality_cmb2", value=q})
        end
    end
    if STOPFLAG then
        break
    end
end

print(`restoring quality {quality}`)
-- Restore original quality setting
LLDebugSettings.setGraphicsQuality(quality)

-- Unlock the camera so user can manipulate
LLAgent.removeCamParams()
LLAgent.setFollowCamActive(false)
print(inspect(snapshot_list))
