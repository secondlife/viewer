local Floater = require 'Floater'
local LLAppearance = require 'LLAppearance'
local startup = require 'startup'

local flt = Floater:new(
    "luafloater_outfits_list.xml",
    {outfits_list = {"double_click"}})

function flt:post_build(event_data)
    local outfits_map = LLAppearance.getOutfitsList()
    local action_data = {}
    action_data.action = "add_list_element"
    action_data.ctrl_name = "outfits_list"
    local outfits = {}
    for uuid, name in pairs(outfits_map) do
      table.insert(outfits, {value = uuid, columns={column = "outfit_name", value = name}})
    end
    action_data.value = outfits
    self:post(action_data)
end

function flt:double_click_outfits_list(event_data)
    LLAppearance.replaceOutfit(event_data.value)
end

startup.wait('STATE_STARTED')
flt:show()
