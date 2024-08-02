local leap = require 'leap'
local startup = require 'startup'
local mapargs = require 'mapargs'

local function login(...)
    startup.wait('STATE_LOGIN_WAIT')
    local args = mapargs('username,grid,slurl', ...)
    args.op = 'login'
    return leap.request('LLPanelLogin', args)
end

return login
