-- Allow a calling function to be passed a mix of positional arguments with
-- keyword arguments. Reference them as fields of a table.
-- Don't use this for a function that can accept a single table argument.
-- mapargs() assumes that a single table argument means its caller was called
-- with f{table constructor} syntax, and maps that table to the specified names.
-- Usage:
-- function f(...)
--     local a = mapargs({'a1', 'a2', 'a3'}, ...)
--     ... a.a1 ... etc.
-- end
-- f(10, 20, 30)     -- a.a1 == 10,  a.a2 == 20,  a.a3 == 30
-- f{10, 20, 30}     -- a.a1 == 10,  a.a2 == 20,  a.a3 == 30
-- f{a3=300, a1=100} -- a.a1 == 100, a.a2 == nil, a.a3 == 300
-- f{1, a3=3}        -- a.a1 == 1,   a.a2 == nil, a.a3 == 3
-- f{a3=3, 1}        -- a.a1 == 1,   a.a2 == nil, a.a3 == 3
local function mapargs(names, ...)
    local args = table.pack(...)
    local posargs = {}
    local keyargs = {}
    -- For a mixed table, no Lua operation will reliably tell you how many
    -- array items it contains, if there are any holes. Track that by hand.
    -- We must be able to handle f(1, nil, 3) calls.
    local maxpos = 0

    -- For convenience, allow passing 'names' as a string 'n0,n1,...'
    if type(names) == 'string' then
        names = string.split(names, ',')
    end

    if not (args.n == 1 and type(args[1]) == 'table') then
        -- If caller passes more than one argument, or if the first argument
        -- is not a table, then it's classic positional function-call syntax:
        -- f(first, second, etc.). In that case we need not bother teasing
        -- apart positional from keyword arguments.
        posargs = args
        maxpos = args.n
    else
        -- Single table argument implies f{mixed} syntax.
        -- Tease apart positional arguments from keyword arguments.
        for k, v in pairs(args[1]) do
            if type(k) == 'number' then
                posargs[k] = v
                maxpos = math.max(maxpos, k)
            else
                if table.find(names, k) == nil then
                    error('unknown keyword argument ' .. tostring(k))
                end
                keyargs[k] = v
            end
        end
    end

    -- keyargs already has keyword arguments in place, just fill in positionals
    args = keyargs
    -- Don't exceed the number of parameter names. Loop explicitly over every
    -- index value instead of using ipairs() so we can support holes (nils) in
    -- posargs.
    for i = 1, math.min(#names, maxpos) do
        if posargs[i] ~= nil then
            -- As in Python, make it illegal to pass an argument both positionally
            -- and by keyword. This implementation permits func(17, first=nil), a
            -- corner case about which I don't particularly care.
            if args[names[i]] ~= nil then
                error(string.format('parameter %s passed both positionally and by keyword',
                                    tostring(names[i])))
            end
            args[names[i]] = posargs[i]
        end
    end
    return args
end

return mapargs
