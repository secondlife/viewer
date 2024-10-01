-- exercise LLGesture API

LLGesture = require 'LLGesture'
inspect = require 'inspect'
Timer = (require 'timers').Timer

-- getActiveGestures() returns {<UUID>: {name, playing, trigger}}
gestures_uuid = LLGesture.getActiveGestures()
-- convert to {<name>: <uuid>}
gestures = {}
for uuid, info in pairs(gestures_uuid) do
    gestures[info.name] = uuid
end
-- now run through the list
for name, uuid in pairs(gestures) do
    if name == 'afk' then
    -- afk has a long timeout, and isn't interesting to look at
        continue
    end
    print(name)
    LLGesture.startGesture(uuid)
    repeat
        Timer(1, 'wait')
    until not LLGesture.isGesturePlaying(uuid)
end
print('Done.')
