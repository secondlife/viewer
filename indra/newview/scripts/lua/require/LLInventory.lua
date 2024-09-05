local leap = require 'leap'
local mapargs = require 'mapargs'
local result_view = require 'result_view'

local function result(keys)
    -- capture result_view() instances for both categories and items
    local result_table = {
        categories=result_view(keys.categories),
        items=result_view(keys.items),
        -- call result_table:close() to release result sets before garbage
        -- collection or script completion
        close = function(self)
            result_view.close(keys.categories[1], keys.items[1])
        end
    }
    -- When the result_table is destroyed, close its result_views.
    return LL.setdtor('LLInventory result', result_table, result_table.close)
end

local LLInventory = {}

-- Get the items/folders info by provided IDs,
-- reply will contain "items" and "categories" tables accordingly
function LLInventory.getItemsInfo(item_ids)
    return result(leap.request('LLInventory', {op = 'getItemsInfo', item_ids=item_ids}))
end

-- Get the table of folder type names, which can be later used to get the ID of the basic folders
function LLInventory.getFolderTypeNames()
    return leap.request('LLInventory', {op = 'getFolderTypeNames'}).names
end

-- Get the UUID of the basic folder("Textures", "My outfits", "Sounds" etc.) by specified folder type name
function LLInventory.getBasicFolderID(ft_name)
    return leap.request('LLInventory', {op = 'getBasicFolderID', ft_name=ft_name}).id
end

-- Get the table of asset type names, which can be later used to get the specific items via LLInventory.collectDescendantsIf(...)
function LLInventory.getAssetTypeNames()
    return leap.request('LLInventory', {op = 'getAssetTypeNames'}).names
end

-- Get the direct descendants of the 'folder_id' provided,
-- reply will contain "items" and "categories" tables accordingly
function LLInventory.getDirectDescendants(folder_id)
    return result(leap.request('LLInventory', {op = 'getDirectDescendants', folder_id=folder_id}))
end
-- backwards compatibility
LLInventory.getDirectDescendents = LLInventory.getDirectDescendants

-- Get the descendants of the 'folder_id' provided, which pass specified filters
-- reply will contain "items" and "categories" tables accordingly
-- LLInventory.collectDescendantsIf{ folder_id   -- parent folder ID
-- [, name]              -- name (substring)
-- [, desc]              -- description (substring)
-- [, type]              -- asset type
-- [, limit]             -- item count limit in reply, maximum and default is 100
-- [, filter_links]}     -- EXCLUDE_LINKS - don't show links, ONLY_LINKS - only show links, INCLUDE_LINKS - show links too (default)
function LLInventory.collectDescendantsIf(...)
    local args = mapargs('folder_id,name,desc,type,filter_links,limit', ...)
    args.op = 'collectDescendantsIf'
    return result(leap.request('LLInventory', args))
end
-- backwards compatibility
LLInventory.collectDescendentsIf = LLInventory.collectDescendantsIf

return LLInventory
