local timers = require 'timers'

print('t0:new(10)')
start = os.clock()
t0 = timers.Timer:new(10, function() print('t0 fired at', os.clock() - start) end)
print('t0:isRunning(): ', t0:isRunning())
print('t0:timeUntilCall(): ', t0:timeUntilCall())
print('t0:cancel(): ', t0:cancel())
print('t0:isRunning(): ', t0:isRunning())
print('t0:timeUntilCall(): ', t0:timeUntilCall())
print('t0:cancel(): ', t0:cancel())

print('t1:new(5)')
start = os.clock()
t1 = timers.Timer:new(5, function() print('t1 fired at', os.clock() - start) end)

print('t2:new(2)')
start = os.clock()
t2 = timers.Timer:new(2)
function t2:tick()
    print('t2 fired at', os.clock() - start)
end

start = os.clock()
timers.Timer:new(5, 'wait')
print(string.format('Timer(5) waited %f seconds', os.clock() - start))

start = os.clock()
timers.Timer:new(
    2,
    coroutine.wrap(function()
            for i = 1,5 do
                print('repeat(2) timer fired at ', os.clock() - start)
                coroutine.yield(nil) -- keep running
            end
            print('repeat(2) timer fired last at ', os.clock() - start)
            return true -- stop
    end),
    true) -- iterate
