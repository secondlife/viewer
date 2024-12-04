local LLDebugSettings = require 'LLDebugSettings'
local SETTING_NAME = '360CaptureCameraFOV'
LL.atexit(function() LL.print_info('Finished') end)

LL.print_info('Updating ' .. SETTING_NAME)
LLDebugSettings.set(SETTING_NAME, 100)
-- 'inadvertently' using Luau library as variable name to hide it
debug = LLDebugSettings.get(SETTING_NAME)
LL.print_info(debug)
