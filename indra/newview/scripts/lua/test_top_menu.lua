UI = require 'UI'

--Add new drop-down 'LUA Menu' to the Top menu.
local MENU_NAME = "lua_menu"
UI.addMenu{name=MENU_NAME,label="LUA Menu"}

--Add two new menu items to the 'LUA Menu': 'Debug console' and 'Scripts'
UI.addMenuItem{name="lua_debug",label="Debug console",
                param="lua_debug",
                func="Floater.ToggleOrBringToFront",
                parent_menu=MENU_NAME}

UI.addMenuItem{name="lua_scripts",label="Scripts",
                param="lua_scripts",
                func="Floater.ToggleOrBringToFront",
                parent_menu=MENU_NAME}

--Add menu separator to the 'LUA Menu' under added menu items
UI.addMenuSeparator{parent_menu=MENU_NAME}

--Add two new menu branch 'About...' to the 'LUA Menu'
local BRANCH_NAME = "about_branch"
UI.addMenuBranch{name="about_branch",label="About...",parent_menu=MENU_NAME}

--Add two new menu items to the 'About...' branch
UI.addMenuItem{name="lua_info",label="Lua...",
                param="https://www.lua.org/about.html",
                func="Advanced.ShowURL",
                parent_menu=BRANCH_NAME}

UI.addMenuItem{name="lua_info",label="Luau...",
                param="https://luau-lang.org/",
                func="Advanced.ShowURL",
                parent_menu=BRANCH_NAME}
