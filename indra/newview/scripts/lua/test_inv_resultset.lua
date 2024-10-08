local LLInventory = require 'LLInventory'
local inspect = require 'inspect'

print('basic folders:')
print(inspect(LLInventory.getFolderTypeNames()))

local folder = LLInventory.getBasicFolderID('my_otfts')
print(`folder = {folder}`)
local result = LLInventory.getDirectDescendants(folder)
print(`type(result) = {type(result)}`)
print(#result.categories, 'categories:')
for i, cat in pairs(result.categories) do
    print(`{i}: {cat.name}`)
end
print(#result.items, 'items')
for i, item in pairs(result.items) do
    print(`{i}: {item.name}`)
end
