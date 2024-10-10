LLInventory = require 'LLInventory'
LLAgent = require 'LLAgent'

-- Get 'Animations' folder id (you can see all folder types via LLInventory.getFolderTypeNames())
animations_id = LLInventory.getBasicFolderID('animatn')
-- Get animations from the 'Animation' folder (you can see all folder types via LLInventory.getAssetTypeNames())
anims = LLInventory.collectDescendentsIf{folder_id=animations_id, type="animatn"}.items

local anim_ids = {}
for _, key in pairs(anims) do
    table.insert(anim_ids, {id = key.id, name = key.name})
end

if #anim_ids == 0 then
    print("No animations found")
else
    -- Start playing a random animation
    math.randomseed(os.time())
    local random_anim = anim_ids[math.random(#anim_ids)]
    local anim_info = LLAgent.getAnimationInfo(random_anim.id)

    print("Starting animation locally: " .. random_anim.name)
    print("Loop: " .. tostring(anim_info.is_loop) .. " Joints: " .. anim_info.num_joints .. " Duration " .. tonumber(string.format("%.2f", anim_info.duration)))
    LLAgent.playAnimation{item_id=random_anim.id}

    -- Stop animation after 3 sec if it's looped or longer than 3 sec
    if anim_info.is_loop or anim_info.duration > 3 then
        LL.sleep(3)
        print("Stop animation.")
        LLAgent.stopAnimation(random_anim.id)
    end
end
