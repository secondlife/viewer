local UI = require 'UI'
local leap = require 'leap'

local function login(username, password)
    if username and password then
        local userpath = '//username_combo/Combo Text Entry'
        local passpath = '//password_edit'
        -- first clear anything presently in those text fields
        for _, path in pairs({userpath, passpath}) do
            UI.click(path)
            UI.keypress{keysym='Backsp', path=path}
        end
        UI.type{path=userpath, text=username}
        UI.type{path=passpath, text=password}
    end
    leap.send('LLPanelLogin', {op='onClickConnect'})
end

return login
