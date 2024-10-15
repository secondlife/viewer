inspect = require 'inspect'
LLInventory = require 'LLInventory'

-- Get 'My Landmarks' folder id (you can see all folder types via LLInventory.getFolderTypeNames())
my_landmarks_id = LLInventory.getBasicFolderID('landmark')
-- Get 3 landmarks from the 'My Landmarks' folder (you can see all folder types via LLInventory.getAssetTypeNames())
landmarks = LLInventory.collectDescendentsIf{folder_id=my_landmarks_id, type="landmark", limit=3}
for _, landmark in pairs(landmarks.items) do
    print(landmark.name)
end

-- Get 'Calling Cards' folder id
calling_cards_id = LLInventory.getBasicFolderID('callcard')
-- Get all items located directly in 'Calling Cards' folder
calling_cards = LLInventory.getDirectDescendents(calling_cards_id).items

-- Print a random calling card name from 'Calling Cards' folder
-- (because getDirectDescendents().items is a Lua result set, selecting
-- a random entry only fetches one slice containing that entry)
math.randomseed(os.time())
for i = 1, 5 do
    pick = math.random(#calling_cards)
    print(`Random calling card (#{pick} of {#calling_cards}): {calling_cards[pick].name}`)
end
