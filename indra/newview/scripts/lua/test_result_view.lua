-- Verify the functionality of result_view.
result_view = require 'result_view'

print('alphabet')
alphabet = "abcdefghijklmnopqrstuvwxyz"
assert(#alphabet == 26)
alphabits = string.split(alphabet, '')

print('function slice()')
function slice(t, index, count)
    return table.move(t, index, index + count - 1, 1, {})
end

print('verify slice()')
-- verify that slice() does what we expect
assert(table.concat(slice(alphabits, 4, 3))  == "def")
assert(table.concat(slice(alphabits, 14, 3)) == "nop")
assert(table.concat(slice(alphabits, 25, 3)) == "yz")

print('function fetch()')
function fetch(key, index)
    -- fetch function is defined to be 0-relative: fix for Lua data
    -- constrain view of alphabits to slices of at most 3 elements
    return slice(alphabits, index+1, 3), index
end

print('result_view()')
-- for test purposes, key is irrelevant, so just 'key'
view = result_view({'key', #alphabits}, fetch)

print('function check_iter()')
function check_iter(...)
    result = {}
    for k, v in ... do
        table.insert(result, v)
    end
    assert(table.concat(result) == alphabet)
end

print('check_iter(pairs(view))')
check_iter(pairs(view))
print('check_iter(ipairs(view))')
check_iter(ipairs(view))
print('check_iter(view)')
check_iter(view)

print('raw index access')
assert(view[5]  == 'e')
assert(view[10] == 'j')
assert(view[15] == 'o')
assert(view[20] == 't')
assert(view[25] == 'y')

print('Success!')

