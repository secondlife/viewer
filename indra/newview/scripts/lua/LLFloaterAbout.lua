-- Engage the LLFloaterAbout LLEventAPI

leap = require 'leap'

local LLFloaterAbout = {}

function LLFloaterAbout.getInfo()
    return leap.request('LLFloaterAbout', {op='getInfo'})
end

return LLFloaterAbout
