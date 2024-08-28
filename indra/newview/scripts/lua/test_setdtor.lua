inspect = require 'inspect'

print('initial setdtor')
bye = LL.setdtor('initial setdtor', 'Goodbye world!', print)

print('arithmetic')
n = LL.setdtor('arithmetic', 11, print)
print("n =", n)
print("n._target =", n._target)
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
for i = 2, 3 do
    print(`n + {i} =`, n + i)
    print(`{i} + n =`, i + n)
    print(`n - {i} =`, n - i)
    print(`{i} - n =`, i - n)
    print(`n * {i} =`, n * i)
    print(`{i} * n =`, i * n)
    print(`n / {i} =`, n / i)
    print(`{i} / n =`, i / n)
    print(`n % {i} =`, n % i)
    print(`{i} % n =`, i % n)
    print(`n ^ {i} =`, n ^ i)
    print(`{i} ^ n =`, i ^ n)
end

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
print('t[2] =', t[2])
print('t.def =', t.def)
t[1] = 'new [1]'
print('t[1] =', t[1])

print('function')
f = LL.setdtor('function', function(a, b) return (a .. b) end, print)
print('f =', f)
print('f._target =', f._target)
print('f("Hello", " world") =', f("Hello", " world"))

print('cleanup')
