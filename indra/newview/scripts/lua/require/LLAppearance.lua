local leap = require 'leap'

local LLAppearance = {}

function LLAppearance.wearOutfit(folder, action)
    action = action or 'add'
    leap.request('LLAppearance', {op='wearOutfit', append = (action == 'add'), folder_id=folder})
end

function LLAppearance.wearOutfitByName(folder, action)
    action = action or 'add'
    leap.request('LLAppearance', {op='wearOutfit', append = (action == 'add'), folder_name=folder})
end

function LLAppearance.wearItems(items_id, replace)
    leap.send('LLAppearance', {op='wearItems', replace = replace, items_id=items_id})
end

function LLAppearance.detachItems(items_id)
    leap.send('LLAppearance', {op='detachItems', items_id=items_id})
end

function LLAppearance.getOutfitsList()
    return leap.request('LLAppearance', {op='getOutfitsList'})['outfits']
end

function LLAppearance.getOutfitItems(id)
    return leap.request('LLAppearance', {op='getOutfitItems', outfit_id = id})['items']
end

return LLAppearance
