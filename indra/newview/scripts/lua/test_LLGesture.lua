-- exercise LLGesture API

LLGesture = require 'LLGesture'
inspect = require 'inspect'


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
        LL.sleep(1)
    until not LLGesture.isGesturePlaying(uuid)
end
print('Done.')
