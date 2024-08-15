local leap = require 'leap'
local mapargs = require 'mapargs'

local LLInventory = {}

-- Get the items/folders info by provided IDs,
-- reply will contain "items" and "categories" tables accordingly
function LLInventory.getItemsInfo(items_id)
    return leap.request('LLInventory', {op = 'getItemsInfo', items_id=items_id})
end

-- Get the table of folder type names, which can be later used to get the ID of the basic folders
function LLInventory.getFolderTypeNames()
    return leap.request('LLInventory', {op = 'getFolderTypeNames'}).type_names
end

-- Get the UUID of the basic folder("Textures", "My outfits", "Sounds" etc.) by specified folder type name
function LLInventory.getBasicFolderID(ft_name)
    return leap.request('LLInventory', {op = 'getBasicFolderID', ft_name=ft_name}).id
end

-- Get the direct descendents of the 'folder_id' provided,
-- reply will contain "items" and "categories" tables accordingly
function LLInventory.getDirectDescendents(folder_id)
    return leap.request('LLInventory', {op = 'getDirectDescendents', folder_id=folder_id})
end

return LLInventory
