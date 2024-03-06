-- Exercise the Queue, WaitQueue, ErrorQueue family

Queue = require('Queue')
WaitQueue = require('WaitQueue')
ErrorQueue = require('ErrorQueue')

q1 = Queue:new()
q2 = Queue:new()

q1:Enqueue(17)

assert(not q1:IsEmpty())
assert(q2:IsEmpty())
assert(q1:Dequeue() == 17)
assert(q1:Dequeue() == nil)
assert(q2:Dequeue() == nil)

q1 = WaitQueue:new()

inspect = require('inspect')
print(inspect(q1))

q2 = WaitQueue:new()
result = {}

values = { 1, 1, 2, 3, 5, 8, 13, 21 }
for i, value in pairs(values) do
    q1:Enqueue(value)
end

function consumer(desc, q)
    print('consumer(', desc, ') start')
    local value = q:Dequeue()
    while value ~= nil do
        table.insert(result, value)
        value = q:Dequeue()
    end
    print('consumer(', desc, ') done')
end

coa = coroutine.create(consumer)
cob = coroutine.create(consumer)
coroutine.resume(coa, 'a', q1)
coroutine.resume(cob, 'b', q1)

assert(result == values)

