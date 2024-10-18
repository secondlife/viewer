LLFloaterAbout = require 'LLFloaterAbout'

local Region = {}

function Region.getInfo()
    info = LLFloaterAbout.getInfo()
    return {
        HOSTNAME=info.HOSTNAME,
        POSITION=info.POSITION,
        POSITION_LOCAL=info.POSITION_LOCAL,
        REGION=info.REGION,
        SERVER_VERSION=info.SERVER_VERSION,
        SLURL=info.SLURL,
    }
end

return Region
