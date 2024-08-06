inspect = require 'inspect'
login = require 'login'

local grid = 'agni'
print(inspect(login.savedLogins(grid)))

print(inspect(login.login{
    username='Your Username',
    grid=grid
    }))
