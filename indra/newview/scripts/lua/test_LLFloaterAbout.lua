-- test LLFloaterAbout

LLFloaterAbout = require('LLFloaterAbout')
leap = require('leap')
coro = require('coro')
inspect = require('inspect')

coro.launch(function ()
    print(inspect(LLFloaterAbout.getInfo()))
    leap.done()
end)

leap.process()

