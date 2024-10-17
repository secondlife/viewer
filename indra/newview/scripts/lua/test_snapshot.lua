local UI = require 'UI'

PATH = 'E:\\'
-- 'png', 'jpeg' or 'bmp'
EXT = '.png'

NAME_SIMPLE = 'Snapshot_simple' .. '_' .. os.date("%Y-%m-%d_%H-%M-%S")
UI.snapshot(PATH .. NAME_SIMPLE .. EXT)

NAME_ARGS = 'Snapshot_args' .. '_' .. os.date("%Y-%m-%d_%H-%M-%S")

-- 'COLOR' or 'DEPTH'
TYPE = 'COLOR'
UI.snapshot{PATH .. NAME_ARGS .. EXT, width = 700, height = 400,
            type = TYPE, showui = false, showhud = false}
