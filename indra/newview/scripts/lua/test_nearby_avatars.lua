inspect = require 'inspect'
LLAgent = require 'LLAgent'

-- Get the list of nearby avatars and print the info
local nearby_avatars = LLAgent.getNearbyAvatarsList()
if nearby_avatars.result.length == 0 then
    print("No avatars nearby")
else
    print("Nearby avatars:")
    for _, av in pairs(nearby_avatars.result) do
        print(av.name ..' ID: ' .. av.id ..' Global pos:' .. inspect(av.global_pos))
    end
end

-- Get the list of nearby objects in 3m range and print the info
local nearby_objects = LLAgent.getNearbyObjectsList(3)
if nearby_objects.result.length == 0 then
    print("No objects nearby")
else
    print("Nearby objects:")
    local pos={}
    for _, obj in pairs(nearby_objects.result) do
      print('ID: ' .. obj.id ..' Global pos:' .. inspect(obj.global_pos))
    end
end
