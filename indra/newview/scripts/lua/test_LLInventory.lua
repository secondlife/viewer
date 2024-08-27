inspect = require 'inspect'
LLInventory = require 'LLInventory'

-- Get 'My Landmarks' folder id (you can see all folder types via LLInventory.getFolderTypeNames())
my_landmarks_id = LLInventory.getBasicFolderID('landmark')
-- Get 3 landmarks from the 'My Landmarks' folder (you can see all folder types via LLInventory.getAssetTypeNames())
landmarks = LLInventory.collectDescendentsIf{folder_id=my_landmarks_id, type="landmark", limit=3}
print(inspect(landmarks))

-- Get 'Calling Cards' folder id
calling_cards_id = LLInventory.getBasicFolderID('callcard')
-- Get all items located directly in 'Calling Cards' folder
calling_cards = LLInventory.getDirectDescendents(calling_cards_id).items

-- Print a random calling card name from 'Calling Cards' folder
local card_names = {}
for _, value in pairs(calling_cards) do
    table.insert(card_names, value.name)
end
math.randomseed(os.time())
print("Random calling card: " .. inspect(card_names[math.random(#card_names)]))
