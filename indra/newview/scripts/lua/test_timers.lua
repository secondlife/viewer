local timers = require 'timers'

-- This t0 is constructed for 10 seconds, but its purpose is to exercise the
-- query and cancel methods. It would print "t0 fired at..." if it fired, but
-- it doesn't, so you don't see that message. Instead you see that isRunning()
-- is true, that timeUntilCall() is (true, close to 10), that cancel() returns
-- true. After that, isRunning() is false, timeUntilCall() returns (false, 0),
-- and a second cancel() returns false.
print('t0(10)')
start = os.clock()
t0 = timers.Timer(10, function() print('t0 fired at', os.clock() - start) end)
print('t0:isRunning(): ', t0:isRunning())
print('t0:timeUntilCall(): ', t0:timeUntilCall())
print('t0:cancel(): ', t0:cancel())
print('t0:isRunning(): ', t0:isRunning())
print('t0:timeUntilCall(): ', t0:timeUntilCall())
print('t0:cancel(): ', t0:cancel())

-- t1 is supposed to fire after 5 seconds, but it doesn't wait, so you see the
-- t2 messages immediately after.
print('t1(5)')
start = os.clock()
t1 = timers.Timer(5, function() print('t1 fired at', os.clock() - start) end)

-- t2 illustrates that instead of passing a callback to new(), you can
-- override the timer instance's tick() method. But t2 doesn't wait either, so
-- you see the Timer(5) message immediately.
print('t2(2)')
start = os.clock()
t2 = timers.Timer(2)
function t2:tick()
    print('t2 fired at', os.clock() - start)
end

-- This anonymous timer blocks the calling fiber for 5 seconds. Other fibers
-- are free to run during that time, so you see the t2 callback message and
-- then the t1 callback message before the Timer(5) completion message.
print('Timer(5) waiting')
start = os.clock()
timers.Timer(5, 'wait')
print(string.format('Timer(5) waited %f seconds', os.clock() - start))

-- This test demonstrates a repeating timer. It also shows that you can (but
-- need not) use a coroutine as the timer's callback function: unlike Python,
-- Lua doesn't disinguish between yield() and return. A coroutine wrapped with
-- coroutine.wrap() looks to Lua just like any other function that you can
-- call repeatedly and get a result each time. We use that to count the
-- callback calls and stop after a certain number. Of course that could also
-- be arranged in a plain function by incrementing a script-scope counter, but
-- it's worth knowing that a coroutine timer callback can be used to manage
-- more complex control flows.
start = os.clock()
timers.Timer(
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
