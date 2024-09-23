-- printf(...) is short for print(string.format(...))

local inspect = require 'inspect'

local function printf(format, ...)
    -- string.format() only handles numbers and strings.
    -- Convert anything else to string using the inspect module.
    local args = {}
    for _, arg in pairs(table.pack(...)) do
        if type(arg) == 'number' or type(arg) == 'string' then
            table.insert(args, arg)
        else
            table.insert(args, inspect(arg))
        end
    end
    print(string.format(format, table.unpack(args)))
end

return printf
