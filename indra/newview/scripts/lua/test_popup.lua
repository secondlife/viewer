UI = require 'UI'
popup = UI.popup

response = popup:alert('This just has a Close button')
response = popup:alertOK(string.format('You said "%s", is that OK?', next(response)))
response = popup:alertYesCancel(string.format('You said "%s"', next(response)))
popup:tip(string.format('You said "%s"', next(response)))
