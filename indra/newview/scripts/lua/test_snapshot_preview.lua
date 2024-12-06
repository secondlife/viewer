local UI = require 'UI'
local flt = UI.Floater("luafloater_preview.xml")

local PATH = 'E:\\'
local EXT = '.png'


local snapshotList = {
  data = {},
  current_idx = 0
}

function snapshotList:add(str)
  table.insert(self.data, str)
  self.current_idx = #self.data
end

function snapshotList:get_prev()
  if self.current_idx > 1 then
    self.current_idx = self.current_idx - 1
    return self.data[self.current_idx]
  else
    return nil
  end
end

function snapshotList:get_next()
  if self.current_idx < #self.data then
    self.current_idx = self.current_idx + 1
    return self.data[self.current_idx]
  else
    return nil
  end
end

function set_preview_id(tex_id)
  flt:post({action="set_value", ctrl_name="preview_icon", value=tex_id})
end

function new_snapshot()
  NAME_SIMPLE = 'Snapshot_' .. os.date("%Y-%m-%d_%H-%M-%S")
  fullname = PATH .. NAME_SIMPLE .. EXT
  if UI.snapshot{fullname, showui = false, showhud = false} then
    tex_id = UI.uploadLocalTexture(fullname)
    set_preview_id(tex_id)
    snapshotList:add(tex_id)
  end
end

function flt:commit_snapshot_btn(event_data)
    new_snapshot()
end

function flt:commit_next_btn(event_data)
  local previous = snapshotList:get_next()
  if previous then
    set_preview_id(previous)
  end
end

function flt:commit_prev_btn(event_data)
  local previous = snapshotList:get_prev()
  if previous then
    set_preview_id(previous)
  end
end

flt:show()
new_snapshot()
