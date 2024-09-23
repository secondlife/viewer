local leap = require 'leap'
local startup = require 'startup'
local mapargs = require 'mapargs'

local login = {}

local function ensure_login_state(op)
    -- no point trying to login until the viewer is ready
    startup.wait('STATE_LOGIN_WAIT')
    -- Once we've actually started login, LLPanelLogin is destroyed, and so is
    -- its "LLPanelLogin" listener. At that point,
    -- leap.request("LLPanelLogin", ...) will hang indefinitely because no one
    -- is listening on that LLEventPump any more. Intercept that case and
    -- produce a sensible error.
    local state = startup.state()
    if startup.before('STATE_LOGIN_WAIT', state) then
        error(`Can't engage login operation {op} once we've reached state {state}`, 2)
    end
end

local function fullgrid(grid)
    if string.find(grid, '.', 1, true) then
        return grid
    else
        return `util.{grid}.secondlife.com`
    end
end

function login.login(...)
    ensure_login_state('login')
    local args = mapargs('username,grid,slurl', ...)
    args.op = 'login'
    args.grid = fullgrid(args.grid)
    return leap.request('LLPanelLogin', args)
end

function login.savedLogins(grid)
    ensure_login_state('savedLogins')
    return leap.request('LLPanelLogin', {op='savedLogins', grid=fullgrid(grid)})['logins']
end

return login
