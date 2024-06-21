leap = require 'leap'

local LLAppearance = {}

function LLAppearance.addOutfit(folder)
    leap.request('LLAppearance', {op='wearOutfit', append = true, folder_id=folder})
end

function LLAppearance.replaceOutfit(folder)
    leap.request('LLAppearance', {op='wearOutfit', append = false, folder_id=folder})
end

function LLAppearance.addOutfitByName(folder)
    leap.request('LLAppearance', {op='wearOutfitByName', append = true, folder_name=folder})
end

function LLAppearance.replaceOutfitByName(folder)
    leap.request('LLAppearance', {op='wearOutfitByName', append = false, folder_name=folder})
end

function LLAppearance.getOutfitsList()
    return leap.request('LLAppearance', {op='getOutfitsList'})['outfits']
end

return LLAppearance
