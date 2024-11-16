local leap = require 'leap'

local teleport_util = {}

local teleport_pump = 'LLTeleport'
local waitfor = leap.WaitFor(0, teleport_pump)
function waitfor:filter(pump, data)
    if pump == self.name then
        return data
    end
end

function waitfor:process(data)
    teleport_util._success = data.success
    leap.WaitFor.process(self, data)
end

leap.request(leap.cmdpump(),
             {op='listen', source=teleport_pump, listener='teleport.lua', tweak=true})

function teleport_util.wait()
    while teleport_util._success == nil do
      local item = waitfor:wait()
    end
    return teleport_util._success
end

return teleport_util
