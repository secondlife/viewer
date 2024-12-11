-- Teleport to a target region, visit certain viewpoints and take snapshots.

LLAgent = require 'LLAgent'
LLDebugSettings = require 'LLDebugSettings'
LLFloaterAbout = require 'LLFloaterAbout'
LLListener = require 'LLListener'
teleport_util = require 'teleport_util'
timers = require 'timers'
UI = require 'UI'
util = require 'util'

-- 'http://maps.secondlife.com/secondlife/TextureTest/137/112/23'
REGION = {regionname='TextureTest', x=137, y=112, z=23}
-- TODO: This should be a temp directory, not the directory in which this
-- script lives.
SNAPDIR = LL.source_dir()
floater = UI.Floater("luafloater_preview.xml")
-- save the user's initial graphics quality
quality = LLDebugSettings.getGraphicsQuality()
print(`Original graphics quality: {quality}`)

-- A few interesting things in TextureTest region.
-- 'lookat' is the point, 'from' is the offset from which to regard it.
lookats = {
    {lookat={123, 110, 23}, from={0,  0, 4.5}},
    {lookat={112, 108, 24}, from={0,  0, 4}},
    {lookat={242,  25, 38}, from={2, 10, 0}},
    {lookat={162,  10, 23}, from={0,  4, 0}},
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
    -- Hop the camera to the next interesting lookat point
    print(`Looking at \{{view.lookat[1]}, {view.lookat[2]}, {view.lookat[3]}}`)
    LLAgent.setCamera{
        camera_pos=util.fromvector(util.tovector(view.lookat) + util.tovector(view.from)),
        focus_pos=view.lookat,
        camera_locked=true,
        focus_locked=true}
    -- cycle through various levels of graphics quality
    for q = 0, 6 do
        print(`    quality {q}`)
        -- set the quality level
        LLDebugSettings.setGraphicsQuality(q)
        timers.sleep(3)
        -- take a snapshot
        basename = `{testbase}{view.lookat[1]}-{view.lookat[2]}-{view.lookat[3]}-q{q}.png`
        fullname = `{SNAPDIR}/{basename}`
        if UI.snapshot{fullname, showui = false, showhud = false} then
            -- display it in our floater
            tex_id = UI.uploadLocalTexture(fullname)
            floater:post({action="set_value", ctrl_name="preview_icon", value=tex_id})
        end
    end
end

print(`restoring quality {quality}`)
-- Restore original quality setting
LLDebugSettings.setGraphicsQuality(quality)
    
-- Unlock the camera so user can manipulate
LLAgent.removeCamParams()
LLAgent.setFollowCamActive(false)
