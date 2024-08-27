local leap = require 'leap'
local mapargs = require 'mapargs'

local LLInventory = {}

-- Get the items/folders info by provided IDs,
-- reply will contain "items" and "categories" tables accordingly
function LLInventory.getItemsInfo(item_ids)
    return leap.request('LLInventory', {op = 'getItemsInfo', item_ids=item_ids})
end

-- Get the table of folder type names, which can be later used to get the ID of the basic folders
function LLInventory.getFolderTypeNames()
    return leap.request('LLInventory', {op = 'getFolderTypeNames'}).names
end

-- Get the UUID of the basic folder("Textures", "My outfits", "Sounds" etc.) by specified folder type name
function LLInventory.getBasicFolderID(ft_name)
    return leap.request('LLInventory', {op = 'getBasicFolderID', ft_name=ft_name}).id
end

-- Get the table of asset type names, which can be later used to get the specific items via LLInventory.collectDescendentsIf(...)
function LLInventory.getAssetTypeNames()
    return leap.request('LLInventory', {op = 'getAssetTypeNames'}).names
end

-- Get the direct descendents of the 'folder_id' provided,
-- reply will contain "items" and "categories" tables accordingly
function LLInventory.getDirectDescendents(folder_id)
    return leap.request('LLInventory', {op = 'getDirectDescendents', folder_id=folder_id})
end

-- Get the descendents of the 'folder_id' provided, which pass specified filters
-- reply will contain "items" and "categories" tables accordingly
-- LLInventory.collectDescendentsIf{ folder_id   -- parent folder ID
-- [, name]              -- name (substring)
-- [, desc]              -- description (substring)
-- [, type]              -- asset type
-- [, limit]             -- item count limit in reply, maximum and default is 100
-- [, filter_links]}     -- EXCLUDE_LINKS - don't show links, ONLY_LINKS - only show links, INCLUDE_LINKS - show links too (default)
function LLInventory.collectDescendentsIf(...)
    local args = mapargs('folder_id,name,desc,type,filter_links,limit', ...)
    args.op = 'collectDescendentsIf'
    return leap.request('LLInventory', args)
end


return LLInventory
