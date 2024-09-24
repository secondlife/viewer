inspect = require 'inspect'

print('initial setdtor')
bye = LL.setdtor('initial setdtor', 'Goodbye world!', print)

print('arithmetic')
n = LL.setdtor('arithmetic', 11, print)
print("n =", n)
print("n._target =", n._target)
print(pcall(function() n._target = 12 end))
print("getmetatable(n) =", inspect(getmetatable(n)))
print("-n =", -n)
for i = 10, 12 do
    -- Comparison metamethods are only called if both operands have the same
    -- metamethod.
    tempi = LL.setdtor('tempi', i, function(n) print('temp', i) end)
    print(`n <  {i}`, n <  tempi)
    print(`n <= {i}`, n <= tempi)
    print(`n == {i}`, n == tempi)
    print(`n ~= {i}`, n ~= tempi)
    print(`n >= {i}`, n >= tempi)
    print(`n >  {i}`, n >  tempi)
end
i = 2
print(`n + {i} =`, n + i)
print(`{i} + n =`, i + n)
print(`n - {i} =`, n - i)
print(`{i} - n =`, i - n)
print(`n * {i} =`, n * i)
print(`{i} * n =`, i * n)
print(`n / {i} =`, n / i)
print(`{i} / n =`, i / n)
print(`n // {i} =`, n // i)
print(`{i} // n =`, i // n)
print(`n % {i} =`, n % i)
print(`{i} % n =`, i % n)
print(`n ^ {i} =`, n ^ i)
print(`{i} ^ n =`, i ^ n)

print('string')
s = LL.setdtor('string', 'hello', print)
print('s =', s)
print('#s =', #s)
print('s .. " world" =', s .. " world")
print('"world " .. s =', "world " .. s)

print('table')
t = LL.setdtor('table', {'[1]', '[2]', abc='.abc', def='.def'},
               function(t) print(inspect(t)) end)
print('t =', inspect(t))
print('t._target =', inspect(t._target))
print('#t =', #t)
print('next(t) =', next(t))
print('next(t, 1) =', next(t, 1))
print('t[2] =', t[2])
print('t.def =', t.def)
t[1] = 'new [1]'
print('t[1] =', t[1])
print('for k, v in pairs(t) do')
for k, v in pairs(t) do
    print(`{k}: {v}`)
end
print('for k, v in ipairs(t) do')
for k, v in ipairs(t) do
    print(`{k}: {v}`)
end
print('for k, v in t do')
for k, v in t do
    print(`{k}: {v}`)
end
-- and now for something completely different
setmetatable(
    t._target,
    {
        __iter = function(arg)
            return next, {'alternate', '__iter'}
        end
    }
)
print('for k, v in t with __iter() metamethod do')
for k, v in t do
    print(`{k}: {v}`)
end

print('function')
f = LL.setdtor('function', function(a, b) return (a .. b) end, print)
print('f =', f)
print('f._target =', f._target)
print('f("Hello", " world") =', f("Hello", " world"))

print('cleanup')
