-- Floater base class

local leap = require 'leap'
local fiber = require 'fiber'
local util = require 'util'

-- list of all the events that a LLLuaFloater might send
local event_list = leap.request("LLFloaterReg", {op="getFloaterEvents"}).events
local event_set = {}
for _, event in pairs(event_list) do
    event_set[event] = true
end

local function _event(event_name)
    if not event_set[event_name] then
        error("Incorrect event name: " .. event_name, 3)
    end
    return event_name
end

-- ---------------------------------------------------------------------------
local Floater = {}

-- Pass:
-- relative file path to floater's XUI definition file
-- optional: sign up for additional events for defined control
-- {<control_name>={action1, action2, ...}}
function Floater:new(path, extra)
    local obj = setmetatable({}, self)
    self.__index = self

    local path_parts = string.split(path, '/')
    obj.name = 'Floater ' .. path_parts[#path_parts]

    obj._command = {op="showLuaFloater", xml_path=LL.abspath(path)}
    if extra then
        -- validate each of the actions for each specified control
        for control, actions in pairs(extra) do
            for _, action in pairs(actions) do
                _event(action)
            end
        end
        obj._command.extra_events = extra
    end

    return obj
end

util.classctor(Floater)

function Floater:show()
    -- leap.eventstream() returns the first response, and launches a
    -- background fiber to call the passed callback with all subsequent
    -- responses.
    local event = leap.eventstream(
        'LLFloaterReg',
        self._command,
        -- handleEvents() returns false when done.
        -- eventstream() expects a true return when done.
        function(event) return not self:handleEvents(event) end)
    self._pump = event.command_name
    -- we might need the returned reqid to cancel the eventstream() fiber
    self.reqid = event.reqid

    -- The response to 'showLuaFloater' *is* the 'post_build' event. Check if
    -- subclass has a post_build() method. Honor the convention that if
    -- handleEvents() returns false, we're done.
    if not self:handleEvents(event) then
        return
    end
end

function Floater:post(action)
    leap.send(self._pump, action)
end

function Floater:request(action)
    return leap.request(self._pump, action)
end

-- local inspect = require 'inspect'

function Floater:handleEvents(event_data)
    local event = event_data.event
    if event_set[event] == nil then
        LL.print_warning(string.format('%s received unknown event %q', self.name, event))
    end

    -- Before checking for a general (e.g.) commit() method, first look for
    -- commit_ctrl_name(): in other words, concatenate the event name with the
    -- ctrl_name, with an underscore between. If there exists such a specific
    -- method, call that.
    local handler, ret
    if event_data.ctrl_name then
        local specific = event .. '_' .. event_data.ctrl_name
        handler = self[specific]
        if handler then
            ret = handler(self, event_data)
            -- Avoid 'return ret or true' because we explicitly want to allow
            -- the handler to return false.
            if ret ~= nil then
                return ret
            else
                return true
            end
        end
    end

    -- No specific "event_on_ctrl()" method found; try just "event()"
    handler = self[event]
    if handler then
        ret = handler(self, event_data)
        if ret ~= nil then
            return ret
        end
--  else
--      print(string.format('%s ignoring event %s', self.name, inspect(event_data)))
    end

    -- We check for event() method before recognizing floater_close in case
    -- the consumer needs to react specially to closing the floater. Now that
    -- we've checked, recognize it ourselves. Returning false terminates the
    -- anonymous fiber function launched by leap.eventstream().
    if event == _event('floater_close') then
        LL.print_warning(self.name .. ' closed')
        return false
    end
    return true
end

-- onCtrl() permits a different dispatch style in which the general event()
-- method explicitly calls (e.g.)
-- self:onCtrl(event_data, {
--     ctrl_name=function()
--         self:post(...)
--     end,
--     ...
-- })
function Floater:onCtrl(event_data, ctrl_map)
    local handler = ctrl_map[event_data.ctrl_name]
    if handler then
        handler()
    end
end

return Floater
