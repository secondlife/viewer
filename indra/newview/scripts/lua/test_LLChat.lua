LLChat = require 'LLChat'

function generateRandomWord(length)
    local alphabet = "abcdefghijklmnopqrstuvwxyz"
    local word = {}
    for i = 1, length do
        local randomIndex = math.random(1, #alphabet)
        table.insert(word, alphabet:sub(randomIndex, randomIndex))
    end
    return table.concat(word)
end

local msg = {'AI says:'}
math.randomseed(os.time())
for i = 1, math.random(1, 10) do
    table.insert(msg, generateRandomWord(math.random(1, 8)))
end
LLChat.sendNearby(table.concat(msg, ' '))
