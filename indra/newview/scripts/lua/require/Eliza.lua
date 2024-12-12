-- from https://devforum.roblox.com/t/eliza-chatbot-ported-to-luau-open-source/2652873,
-- with gratitude to Magus_ArtStudios https://devforum.roblox.com/u/Magus_ArtStudios

function Eliza(text)
    local response = ""
    local user = string.lower(text)
    local userOrig = user

    -- randomly selected replies if no keywords
    local randReplies = {
        "What does that suggest to you?",
        "I see...",
        "I'm not sure I understand you fully.",
        "Can you elaborate on that?",
        "That is quite interesting!",
        "That's so... Please continue...",
        "I understand...", "Well, well... Do go on", 
        "Why are you saying that?",
        "Please explain the background to that remark...", 
        "Could you say that again, in a different way?",
    }
    
    local replies = {
        [" can you"] = {"Perhaps you would like to be able to"},
        [" do you"] = {"Yes, I"},
        [" can i"] = {"Perhaps you don't want to be able to"},
        [" you are"] = {"What makes you think I am"},
        [" you're"] = {"What is your reaction to me being"},
        [" i don't"] = {"Why don't you"},
        [" i feel"] = {"Tell me more about feeling"},
        [" why don't you"] = {"Why would you want me to"},
        [" why can't i"] = {"What makes you think you should be able to"},
        [" are you"] = {"Why are you interested in whether or not I am"},
        [" i can't"] = {"How do you know you can't"},
        [" i am"] = {"How long have you been"},
        [" i'm"] = {"Why are you telling me you're"},
        [" i want"] = {"Why do you want"},
        [" what"] = {"What do you think?"},
        [" how"] = {"What answer would please you the most?"},
        [" who"] = {"How often do you think of such questions?"},
        [" where"] = {"Why did you think of that?"},
        [" when"] = {"What would your best friend say to that question?"},
        [" why"] = {"What is it that you really want to know?"},
        [" perhaps"] = {"You're not very firm on that!"},
        [" drink"] = {"Moderation in all things should be the rule."},
        [" sorry"] = {"Why are you apologizing?",  "Please don't apologize",
                      "Apologies are not necessary",
                      "What feelings do you have when you apologize",},
        [" dreams"] = {"Why did you bring up the subject of dreams?"},
        [" i like"] = {"Is it good that you like"},
        [" maybe"] = {"Aren't you being a bit tentative?"},
        [" no"] = {"Why are you being negative?"},
        [" your"] = {"Why are you concerned about my"},
        [" always"] = {"Can you think of a specific example?"},
        [" think"] = {"Do you doubt"},
        [" yes"] = {"You seem quite certain. Why is this so?"},
        [" friend"] = {"Why do you bring up the subject of friends?"},
        [" am i"] = {"You are"},
        [" i remember"]= {
            "Do you often think of",
            "What else do you remember?",
            "Why do you recall",
            "What in the present situation reminds you of",
            "What is the connection between me and"
        },    
        
    }

    -- keywords, replies
    

    -- conjugate
    local conjugate = {
        [" i "] = "you",
        [" are "] = "am",
        [" were "] = "was",
        [" you "] = "me",
        [" your "] = "my",
        [" i've "] = "you've",
        [" i'm "] = "you're",
        [" me "] = "you",
        [" am i "] = "you are",
        [" am "] = "are",
    }

    local function createSentences(str)
        local sentences = {} -- create an empty table to store the sentences
        local start = 1 -- initialize the start index of the current sentence
        for i = 1, #str do -- loop through each character in the input string
            local c = str:sub(i, i) -- get the current character
            if c == "!" or c == "?" or c == "." or i==#str then -- check if the current character is a punctuation mark
                local sentence = str:sub(start, i) -- get the current sentence from the start index to the current index
                table.insert(sentences, sentence) -- insert the current sentence into the table
                start = i + 1 -- update the start index to the next character after the punctuation mark
            end
        end
        if sentences[1]==nil then
            return {str}
        end
        -- random replies, no keyword
        
        return sentences -- return the table of sentences
    end
    
    
    local function processSentences(user,response)
        -- find keyword, phrase
        local function replyRandomly()
            response = randReplies[math.random(#randReplies)]..""
        end
        local function processInput()
            
            for keyword, reply in pairs(replies) do
                local d, e = string.find(user, keyword, 1, 1)
                if d then
                    -- process keywords
                    local chr=reply[math.random(1,#reply)]
                    response = response..chr.." "
                    if string.byte(string.sub(chr, -1)) < 65 then -- "A"
                        response = response..""; return
                    end
                    local h = string.len(user) - (d + string.len(keyword))
                    if h > 0 then
                        user = string.sub(user, -h)
                    end
                    for cFrom, cTo in pairs(conjugate) do
                        local f, g = string.find(user, cFrom, 1, 1)
                        if f then
                            local j = string.sub(user, 1, f - 1).." "..cTo
                            local z = string.len(user) - (f - 1) - string.len(cTo)
                            response = response..j..""
                            if z > 2 then
                                local l = string.sub(user, -(z - 2))
                                if not string.find(userOrig, l) then return end
                            end
                            if z > 2 then response = response..string.sub(user, -(z - 2)).."" end
                            if z < 2 then response = response.."" end
                            return 
                        end--if f
                    end--for
                    response = response..user..""
                    return response
                end--if d
            end--for
            replyRandomly()
            return response
        end

        -- main()
        -- accept user input
        if string.sub(user, 1, 3) == "bye" then
            response = "Bye, bye for now.See you again some time."
            return response
        end
        if string.sub(user, 1, 7) == "because" then
            user = string.sub(user, 8)
        end
        user = " "..user.." "
        -- process input, print reply
        processInput()
        response = response..""
        return response
    end
    local responses={}
    for i,v in pairs(createSentences(user)) do
        local answ=processSentences(v," ")
        table.insert(responses, answ)
    end
    return table.concat(responses, ' ')
end

return Eliza
