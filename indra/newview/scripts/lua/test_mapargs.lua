local mapargs = require 'mapargs'
local inspect = require 'inspect'

function tabfunc(...)
    local a = mapargs({'a1', 'a2', 'a3'}, ...)
    print(inspect(a))
end

print('----------')
print('f(10, 20, 30)')
tabfunc(10, 20, 30)
print('f(10, nil, 30)')
tabfunc(10, nil, 30)
print('f{10, 20, 30}')
tabfunc{10, 20, 30}
print('f{10, nil, 30}')
tabfunc{10, nil, 30}
print('f{a3=300, a1=100}')
tabfunc{a3=300, a1=100}
print('f{1, a3=3}')
tabfunc{1, a3=3}
print('f{a3=3, 1}')
tabfunc{a3=3, 1}
print('----------')

if false then
    -- the code below was used to explore ideas that became mapargs()
    mixed = { '[1]', nil, '[3]', abc='[abc]', '[3]', def='[def]' }
    local function showtable(desc, t)
        print(string.format('%s (len %s)\n%s', desc, #t, inspect(t)))
    end
    showtable('mixed', mixed)

    print('ipairs(mixed)')
    for k, v in ipairs(mixed) do
        print(string.format('[%s] = %s', k, tostring(v)))
    end

    print('table.pack(mixed)')
    print(inspect(table.pack(mixed)))

    local function nilarg(desc, a, b, c)
        print(desc)
        print('a = ' .. tostring(a))
        print('b = ' .. tostring(b))
        print('c = ' .. tostring(c))
    end

    nilarg('nilarg(1)', 1)
    nilarg('nilarg(1, nil, 3)', 1, nil, 3)

    local function nilargs(desc, ...)
        args = table.pack(...)
        showtable(desc, args)
    end

    nilargs('nilargs{a=1, b=2, c=3}', {a=1, b=2, c=3})
    nilargs('nilargs(1, 2, 3)', 1, 2, 3)
    nilargs('nilargs(1, nil, 3)', 1, nil, 3)
    nilargs('nilargs{1, 2, 3}', {1, 2, 3})
    nilargs('nilargs{1, nil, 3}', {1, nil, 3})

    print('table.unpack({1, nil, 3})')
    a, b, c = table.unpack({1, nil, 3})
    print('a = ' .. tostring(a))
    print('b = ' .. tostring(b))
    print('c = ' .. tostring(c))
end
