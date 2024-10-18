-- Engage the viewer's UI

local leap = require 'leap'
local mapargs = require 'mapargs'
local result_view = require 'result_view'
local Timer = (require 'timers').Timer
local util = require 'util'

-- Allow lazily accessing UI submodules on demand, e.g. a reference to
-- UI.Floater lazily loads the UI/Floater module.
local UI = util.submoduledir({}, 'UI')

-- ***************************************************************************
--  registered menu actions
-- ***************************************************************************
function UI.call(func, parameter)
    -- 'call' is fire-and-forget
    leap.request('UI', {op='call', ['function']=func, parameter=parameter})
end

function UI.callables()
    return leap.request('UI', {op='callables'}).callables
end

function UI.getValue(path)
    return leap.request('UI', {op='getValue', path=path})['value']
end

-- ***************************************************************************
--  UI views
-- ***************************************************************************
-- Either:
-- wreq{op='Something', a=1, b=2, ...}
-- or:
-- (args should be local, as this wreq() call modifies it)
-- local args = {a=1, b=2, ...}
-- wreq('Something', args)
local function wreq(op_or_data, data_if_op)
    if data_if_op ~= nil then
        -- this is the wreq(op, data) form
        data_if_op.op = op_or_data
        op_or_data = data_if_op
    end
    return leap.request('LLWindow', op_or_data)
end

-- omit 'parent' to list all view paths
function UI.listviews(parent)
    return wreq{op='getPaths', under=parent}
end

function UI.viewinfo(path)
    return wreq{op='getInfo', path=path}
end

-- ***************************************************************************
--  mouse actions
-- ***************************************************************************
-- pass a table:
-- UI.click{path=path
--          [, button='LEFT' | 'CENTER' | 'RIGHT']
--          [, x=x, y=y]
--          [, hold=duration]}
function UI.click(...)
    local args = mapargs('path,button,x,y,hold', ...)
    args.button = args.button or 'LEFT'
    local hold = args.hold or 1.0
    wreq('mouseMove', args)
    wreq('mouseDown', args)
    Timer(hold, 'wait')
    wreq('mouseUp', args)
end

-- pass a table as for UI.click()
function UI.doubleclick(...)
    local args = mapargs('path,button,x,y', ...)
    args.button = args.button or 'LEFT'
    wreq('mouseDown', args)
    wreq('mouseUp',   args)
    wreq('mouseDown', args)
    wreq('mouseUp',   args)
end

-- UI.drag{path=, xoff=, yoff=}
function UI.drag(...)
    local args = mapargs('path,xoff,yoff', ...)
    -- query the specified path
    local rect = UI.viewinfo(args.path).rect
    local centerx = math.floor(rect.left   + (rect.right - rect.left)/2)
    local centery = math.floor(rect.bottom + (rect.top   - rect.bottom)/2)
    wreq{op='mouseMove', path=args.path, x=centerx, y=centery}
    wreq{op='mouseDown', path=args.path, button='LEFT'}
    wreq{op='mouseMove', path=args.path, x=centerx + args.xoff, y=centery + args.yoff}
    wreq{op='mouseUp',   path=args.path, button='LEFT'}
end

-- ***************************************************************************
--  keyboard actions
-- ***************************************************************************
-- pass a table:
-- UI.keypress{
--     [path=path]                      -- if omitted, default input field
--     [, char='x']                     -- requires one of char, keycode, keysym
--     [, keycode=120]
--     keysym per https://github.com/secondlife/viewer/blob/main/indra/llwindow/llkeyboard.cpp#L68-L124
--     [, keysym='Enter']
--     [, mask={'SHIFT', 'CTL', 'ALT', 'MAC_CONTROL'}]  -- some subset of these
--     }
function UI.keypress(...)
    local args = mapargs('path,char,keycode,keysym,mask', ...)
    if args.char == '\n' then
        args.char = nil
        args.keysym = 'Enter'
    end
    return wreq('keyDown', args)
end

-- UI.type{text=, path=}
function UI.type(...)
    local args = mapargs('text,path', ...)
    if #args.text > 0 then
        -- The caller's path may be specified in a way that requires recursively
        -- searching parts of the LLView tree. No point in doing that more than
        -- once. Capture the actual path found by that first call and use that for
        -- subsequent calls.
        local path = UI.keypress{path=args.path, char=string.sub(args.text, 1, 1)}.path
        for i = 2, #args.text do
            UI.keypress{path=path, char=string.sub(args.text, i, i)}
        end
    end
end

-- ***************************************************************************
--  Snapshot
-- ***************************************************************************
-- UI.snapshot{filename=filename            -- extension may be specified: bmp, jpeg, png
--          [, type='COLOR' | 'DEPTH']
--          [, width=width][, height=height]  -- uses current window size if not specified
--          [, showui=true][, showhud=true]
--          [, rebuild=false]}
function UI.snapshot(...)
    local args = mapargs('filename,width,height,showui,showhud,rebuild,type', ...)
    args.op = 'saveSnapshot'
    return leap.request('LLViewerWindow', args).result
end

-- ***************************************************************************
--  Top menu
-- ***************************************************************************

function UI.getTopMenus()
    return leap.request('UI', {op='getTopMenus'}).menus
end

function UI.addMenu(...)
    local args = mapargs('name,label', ...)
    args.op = 'addMenu'
    return leap.request('UI', args)
end

function UI.setMenuVisible(name, visible)
    return leap.request('UI', {op='setMenuVisible', name=name, visible=visible})
end

function UI.addMenuBranch(...)
    local args = mapargs('name,label,parent_menu', ...)
    args.op = 'addMenuBranch'
    return leap.request('UI', args)
end

-- see UI.callables() for valid values of 'func'
function UI.addMenuItem(...)
    local args = mapargs('name,label,parent_menu,func,param,pos', ...)
    args.op = 'addMenuItem'
    return leap.request('UI', args)
end

function UI.addMenuSeparator(...)
    local args = mapargs('parent_menu,pos', ...)
    args.op = 'addMenuSeparator'
    return leap.request('UI', args)
end

-- ***************************************************************************
--  Toolbar buttons
-- ***************************************************************************
-- Clears all buttons off the toolbars
function UI.clearAllToolbars()
    leap.send('UI', {op='clearAllToolbars'})
end

function UI.defaultToolbars()
    leap.send('UI', {op='defaultToolbars'})
end

-- UI.addToolbarBtn{btn_name=btn_name
--          [, toolbar= bottom] -- left, right, bottom -- default is bottom
--          [, rank=1]} -- position on the toolbar, starts at 0 (0 - first position, 1 - second position etc.)
function UI.addToolbarBtn(...)
    local args = mapargs('btn_name,toolbar,rank', ...)
    args.op = 'addToolbarBtn'
    return leap.request('UI', args)
end

-- Returns the rank(position) of the command in the original list
function UI.removeToolbarBtn(btn_name)
    return leap.request('UI', {op = 'removeToolbarBtn', btn_name=btn_name}).rank
end

function UI.getToolbarBtnNames()
    return leap.request('UI', {op = 'getToolbarBtnNames'}).cmd_names
end

-- ***************************************************************************
--  Floaters
-- ***************************************************************************
function UI.showFloater(floater_name)
    leap.send("LLFloaterReg", {op = "showInstance", name = floater_name})
end

function UI.hideFloater(floater_name)
    leap.send("LLFloaterReg", {op = "hideInstance", name = floater_name})
end

function UI.toggleFloater(...)
    local args = mapargs('name,key', ...)
    args.op = 'toggleInstance'
    leap.send("LLFloaterReg", args)
end

function UI.isFloaterVisible(floater_name)
    return leap.request("LLFloaterReg", {op = "instanceVisible", name = floater_name}).visible
end

function UI.closeAllFloaters()
    return leap.send("UI", {op = "closeAllFloaters"})
end

function UI.getFloaterNames()
    local key_length = leap.request("LLFloaterReg", {op = "getFloaterNames"}).floaters
    local view = result_view(key_length)
    return LL.setdtor('registered floater names', view, view.close)
end

return UI
