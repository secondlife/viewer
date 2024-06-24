startup = require 'startup'
login = require 'login'

startup.wait('STATE_LOGIN_WAIT')
login()
-- Fill in valid credentials as they would be entered on the login screen
-- login('My Username', 'password')
