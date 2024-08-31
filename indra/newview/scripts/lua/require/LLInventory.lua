local leap = require 'leap'
local mapargs = require 'mapargs'

local function result(keys)
    return LL.setdtor(
        'LLInventory result',
        setmetatable(
            -- the basic table wrapped by setmetatable just captures the int
            -- result-set keys from 'keys', but with underscore prefixes
            {
                _categories=keys.categories,
                _items=keys.items,
                -- call result:close() to release result sets before garbage
                -- collection or script completion
                close = function(self)
                    leap.send('LLInventory',
                              {op='closeResult',
                               result={self._categories, self._items}})
                end
            },
            -- The caller of one of our methods that returns a result set
            -- isn't necessarily interested in both categories and items, so
            -- don't proactively populate both. Instead, when caller references
            -- either 'categories' or 'items', the __index() metamethod
            -- populates that field.
            {
                __index = function(t, key)
                    -- we really don't care about references to any other field
                    if not table.find({'categories', 'items'}, key) then
                        return nil
                    end
                    -- We cleverly saved the int result set key in a field
                    -- with the same name but an underscore prefix.
                    local resultkey = t['_' .. key]
                    -- TODO: This only ever fetches the FIRST slice. What we
                    -- really want is to return a table with metamethods that
                    -- manage indexed access and table iteration.
                    -- Remember our C++ entry point uses 0-relative indexing.
                    local slice = leap.request(
                        'LLInventory',
                        {op='getSlice', result=resultkey, index=0}).slice
                    print(`getSlice({resultkey}, 0) => {slice} ({#slice} entries)`)
                    -- cache this slice for future reference
                    t[key] = slice
                    return slice
                end
            }
        ),
        -- When the table-with-metatable above is destroyed, tell LLInventory
        -- we're done with its result sets -- whether or not we ever fetched
        -- either of them.
        function(keys)
            keys:close()
        end
    )
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
