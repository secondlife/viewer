local leap = require 'leap'

local function logout()
    leap.send('LLAppViewer', {op='userQuit'});
end

return logout
