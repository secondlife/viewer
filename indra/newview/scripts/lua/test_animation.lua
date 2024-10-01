LLInventory = require 'LLInventory'
LLAgent = require 'LLAgent'
Timer = (require 'timers').Timer

-- Get 'Animations' folder id (you can see all folder types via LLInventory.getFolderTypeNames())
animations_id = LLInventory.getBasicFolderID('animatn')
-- Get animations from the 'Animation' folder (you can see all folder types via LLInventory.getAssetTypeNames())
anims = LLInventory.collectDescendentsIf{folder_id=animations_id, type="animatn"}.items

local anim_ids = {}
for key in pairs(anims) do
    table.insert(anim_ids, key)
end

if #anim_ids == 0 then
    print("No animations found")
else
    -- Start playing a random animation
    math.randomseed(os.time())
    local random_id = anim_ids[math.random(#anim_ids)]
    local anim_info = LLAgent.getAnimationInfo(random_id)

    print("Starting animation locally: " .. anims[random_id].name)
    print("Loop: " .. anim_info.is_loop .. " Joints: " .. anim_info.num_joints .. " Duration " .. tonumber(string.format("%.2f", anim_info.duration)))
    LLAgent.playAnimation{item_id=random_id}

    -- Stop animation after 3 sec if it's looped or longer than 3 sec
    if anim_info.is_loop == 1 or anim_info.duration > 3 then
        Timer(3, 'wait')
        print("Stop animation.")
        LLAgent.stopAnimation(random_id)
    end
end
