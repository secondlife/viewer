-- Inject Lua-related menus into the top menu structure. Run this as a Lua
-- script so that turning off the Lua feature also disables these menus.

-- Under Develop -> Consoles, want to present the equivalent of:
-- <menu_item_separator/>
-- <menu_item_check
--     label="LUA Debug Console"
--     name="LUA Debug Console">
--   <menu_item_check.on_check
--       function="Floater.Visible"
--       parameter="lua_debug" />
--   <menu_item_check.on_click
--       function="Floater.Toggle"
--       parameter="lua_debug" />
-- </menu_item_check>
-- <menu_item_check
--     label="LUA Scripts Info"
--     name="LUA Scripts">
--   <menu_item_check.on_check
--       function="Floater.Visible"
--       parameter="lua_scripts" />
--   <menu_item_check.on_click
--       function="Floater.Toggle"
--       parameter="lua_scripts" />
-- </menu_item_check>

local startup = require 'startup'
local UI = require 'UI'

-- Don't mess with the viewer's menu structure until we've logged in.
startup.wait('STATE_STARTED')

-- Add LUA Debug Console to Develop->Consoles
local pos = 9
UI.addMenuSeparator{
    parent_menu='Consoles', pos=pos,
}
pos += 1
UI.addMenuItem{
    parent_menu='Consoles', pos=pos,
    name='lua_debug', label='LUA Debug Console',
    func='Floater.ToggleOrBringToFront', param='lua_debug',
}
pos += 1

-- Add LUA Scripts Info to Develop->Consoles
UI.addMenuItem{
    parent_menu='Consoles', pos=pos,
    name='lua_scripts', label='LUA Scripts Info',
    func='Floater.ToggleOrBringToFront', param='lua_scripts',
}
