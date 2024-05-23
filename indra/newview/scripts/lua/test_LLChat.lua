LLChat = require 'LLChat'

function generateRandomWord(length)
    local alphabet = "abcdefghijklmnopqrstuvwxyz"
    local word = ""
    for i = 1, length do
        local randomIndex = math.random(1, #alphabet)
        word = word .. alphabet:sub(randomIndex, randomIndex)
    end
    return word
end

local msg = ""
math.randomseed(os.time())
for i = 1, math.random(1, 10) do
    msg = msg .. " ".. generateRandomWord(math.random(1, 8))
end
LLChat.sendNearby(msg)
