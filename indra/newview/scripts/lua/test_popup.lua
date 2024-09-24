UI = require 'UI'
popup = UI.popup
startup = require 'startup'

startup.wait('STATE_STARTED')

response = popup:alert('This just has a Close button')
response = popup:alertOK(string.format('You said "%s", is that OK?', response))
response = popup:alertYesCancel(string.format('You said "%s"', response))
popup:tip(string.format('You said "%s"', response))
