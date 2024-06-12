startup = require 'startup'
login = require 'login'

startup.wait('STATE_LOGIN_WAIT')
login()
-- WIP: not working as of 2024-06-11
-- login('My Username', 'password')
