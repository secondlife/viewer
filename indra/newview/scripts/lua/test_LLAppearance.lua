local LLAppearance = require 'LLAppearance'
local startup = require 'startup'
local inspect = require 'inspect'
local UI = require 'UI'

local SHOW_OUTFITS = true
local SELECTED_OUTFIT_ID = {}
local DATA_MAP = {}

local wearables_lbl = 'Show wearables'
local outfits_lbl = 'Show outfits'
local replace_cof_lbl = 'Replace COF'
local add_cof_lbl = 'Add to COF'
local outfits_title = 'Outfits'
local wear_lbl = 'Wear item'
local detach_lbl = 'Detach item'

local flt = UI.Floater(
    "luafloater_outfits_list.xml",
    {outfits_list = {"double_click"}})

function get_selected_id()
    return flt:request({action="get_selected_id", ctrl_name='outfits_list'}).value
end

function populate_list()
    if SHOW_OUTFITS then
      DATA_MAP = LLAppearance.getOutfitsList()
    else
      DATA_MAP = LLAppearance.getOutfitItems(SELECTED_OUTFIT_ID)
    end

    local action_data = {}
    action_data.action = "add_list_element"
    action_data.ctrl_name = "outfits_list"
    local outfits = {}
    for uuid, info in pairs(DATA_MAP) do
      name = {}
      if SHOW_OUTFITS then
        name = info
      else
        name = info.name
      end
      table.insert(outfits, {value = uuid, columns={column = "outfit_name", value = name}})
    end
    action_data.value = outfits
    flt:post(action_data)
end

function set_label(btn_name, value)
    flt:post({action="set_label", ctrl_name=btn_name, value=value})
end

function set_enabled(btn_name, value)
    flt:post({action="set_enabled", ctrl_name=btn_name, value=value})
end

function update_labels()
    if SHOW_OUTFITS then
      set_label('select_btn', wearables_lbl)
      set_label('replace_btn', replace_cof_lbl)
      set_label('add_btn', add_cof_lbl)

      set_enabled('select_btn', false)
      flt:post({action="set_title", value=outfits_title})
    else
      set_label('select_btn', outfits_lbl)
      set_label('replace_btn', wear_lbl)
      set_label('add_btn', detach_lbl)

      set_enabled('select_btn', true)
      flt:post({action="set_title", value=DATA_MAP[SELECTED_OUTFIT_ID]})
    end

    set_enabled('replace_btn', false)
    set_enabled('add_btn', false)
end

function flt:post_build(event_data)
    populate_list()
end

function flt:commit_replace_btn(event_data)
    if SHOW_OUTFITS then
      LLAppearance.wearOutfit(get_selected_id(), 'replace')
    else
      LLAppearance.wearItems(get_selected_id(), false)
    end
end

function flt:commit_add_btn(event_data)
    if SHOW_OUTFITS then
      LLAppearance.wearOutfit(get_selected_id(), 'add')
    else
      LLAppearance.detachItems(get_selected_id())
    end
end

function flt:commit_select_btn(event_data)
    SHOW_OUTFITS = not SHOW_OUTFITS
    SELECTED_OUTFIT_ID = get_selected_id()
    update_labels()
    self:post({action="clear_list", ctrl_name='outfits_list'})
    populate_list()
end

function flt:commit_outfits_list(event_data)
      set_enabled('replace_btn', true)
      set_enabled('add_btn', true)
      set_enabled('select_btn', true)
end

startup.wait('STATE_STARTED')
flt:show()
