startup=require 'startup'
UI=require 'UI'
startup.wait('STATE_LOGIN_WAIT')
for _, cbl in pairs(UI.callables()) do
    print(`{cbl.name} ({cbl.access})`)
end
