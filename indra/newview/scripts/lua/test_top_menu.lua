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

--Add 'Demo scripts...' branch to the 'LUA Menu'
local DEMO_BRANCH = "demo_scripts"
UI.addMenuBranch{name=DEMO_BRANCH,label="Demo scripts...",parent_menu=MENU_NAME}

--Add menu items to the 'Demo scripts...' branch, which will invoke specified script on click
UI.addMenuItem{name="speedometer",label="Speedometer",
                param="test_luafloater_speedometer.lua",
                func="Lua.RunScript",
                parent_menu=DEMO_BRANCH}

UI.addMenuItem{name="gesture_list",label="Gesture list",
                param="test_luafloater_gesture_list.lua",
                func="Lua.RunScript",
                parent_menu=DEMO_BRANCH}

--Add one more menu separator
UI.addMenuSeparator{parent_menu=MENU_NAME}

--Add 'About...' branch to the 'LUA Menu'
local ABOUT_BRANCH = "about_branch"
UI.addMenuBranch{name=ABOUT_BRANCH,label="About...",parent_menu=MENU_NAME}

--Add two new menu items to the 'About...' branch
UI.addMenuItem{name="lua_info",label="Lua...",
                param="https://www.lua.org/about.html",
                func="Advanced.ShowURL",
                parent_menu=ABOUT_BRANCH}

UI.addMenuItem{name="lua_info",label="Luau...",
                param="https://luau-lang.org/",
                func="Advanced.ShowURL",
                parent_menu=ABOUT_BRANCH}
