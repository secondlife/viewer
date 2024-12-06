-- Maxim: I'm thinking about the following scenario for demo:
-- * teleport to the specified region
-- * 'autopilot-walking' to the prepared chair
-- * playing some sort of cheerful animation + camera orbits around the avatar
-- * sitting on the chair and sending a message to nearby chat
-- It's not necessary to use it like that, but I think we should show the
-- things that are new (camera controls, teleportation, sitting on the object
-- vs just sitting on the floor etc).

Eliza = require 'Eliza'
inspect = require 'inspect'
LLAgent = require 'LLAgent'
LLChat = require 'LLChat'
LLGesture = require 'LLGesture'
LLListener = require 'LLListener'
startup = require 'startup'
teleport_util = require 'teleport_util'
timers = require 'timers'
UI = require 'UI'
util = require 'util'
WaitQueue = require 'WaitQueue'

MYCHAIR = '0d92143d-f045-db11-3f06-695ca08f4893'

startup.wait('STATE_STARTED')

UI.popup:tip 'Heading to my office now'
-- Don't bother for preliminary testing
LLAgent.teleport{regionname='LindenWorld B', x=170, y=86, z=47}
teleport_util.wait()

where = LLAgent.getGlobalPosition()
print(`my pos: {inspect(where)}`)
local function distance2(p0, p1)
    return (p1[1] - p0[1])^2 + (p1[2] - p0[2])^2 + (p1[3] - p0[3])^2
end

local function distance2me(p1)
    return distance2(where, p1)
end

local orbit = {
    height = 2.0, -- meters
    radius = 4.0, -- meters
    speed = 1.0, -- meters/second along circle
    pos = function(self, t)
        local agent = LLAgent.getRegionPosition()
        local radians = self.speed * t
        return {
            agent[1] + self.radius * math.cos(radians),
            agent[2] + self.radius * math.sin(radians),
            agent[3] + self.height
        }
    end
}

local function moveCamera(start, stop)
    if os.clock() < stop then
        -- usual case
        LLAgent.setCamera{ camera_pos=orbit:pos(os.clock() - start), camera_locked=true }
        return nil
    else
        -- last time
        LLAgent.removeCamParams()
        LLAgent.setFollowCamActive(false)
        return true
    end
end

local function animateCamera(duration)
    local start = os.clock()
    local stop  = start + duration
    -- call moveCamera() repeatedly until it returns true
    local timer = timers.Timer(0.1, function() return moveCamera(start, stop) end, true)
end

-- -- we doubt table.sort() will work well on a result_view proxy object
-- near_copy = {}
-- for k, v in nearby do
--     table.insert(near_copy, v)
-- end
-- -- put the closest objects at the front
-- table.sort(
--     near_copy,
--     function (lhs, rhs)
--         return distance2me(lhs.global_pos) < distance2me(rhs.global_pos)
--     end)

-- Find objects within 10 meters
nearby = LLAgent.getNearbyObjectsList(6)
print(`Got {#nearby} near objects`)

-- Find MYCHAIR within that list
for _, obj in nearby do
    if obj.id == MYCHAIR then
        gchair = obj.global_pos
        lchair = obj.region_pos
        print(`Found my chair at {inspect(gchair)}`)
        break
    end
end
if not gchair then
    print("Can't find my chair, sorry")
    -- TODO: we should have an LL.exit() to terminate a script early
else
    local arrived = WaitQueue()
    local walk_listener = LLListener(LLAgent.autoPilotPump)

    function walk_listener:handleMessages(event_data)
        if event_data.success then
          print('Destination is reached')
        else
          print('Failed to reach destination')
        end
        -- wake up arrived:Dequeue()
        arrived:close()
        return false
    end

    print(`Going to {inspect(gchair)}`)
    animateCamera(8)
    LLAgent.startAutoPilot{target_global=gchair, allow_flying=false, stop_distance=1}
    walk_listener:start()
    -- wait for walk_listener:handleMessages()
    arrived:Dequeue()
    gestures = {}
    for uuid, info in LLGesture.getActiveGestures() do
        gestures[info.name] = uuid
    end
    if not gestures.dance2 then
        print("Can't find dance2 gesture")
    else
        LLGesture.startGesture(gestures.dance2)
        repeat
            timers.sleep(1)
        until not LLGesture.isGesturePlaying(dance2)
    end
    timers.sleep(1)
    -- LLAgent.requestSit{obj_uuid=MYCHAIR} -- doesn't work?
    -- LLAgent.requestSit() -- on the ground, then
    -- has to be region-local position, not global position!
    -- LLAgent.requestSit{position=lchair}
    LLAgent.requestSit{position=LLAgent.getRegionPosition()}
    agent_id = LLAgent.getID()

    done_talking = WaitQueue()
    chat_listener = LLListener(LLChat.nearbyChatPump)
    function chat_listener:handleMessages(event_data)
        -- ignore messages and commands from other avatars
        if event_data.from_id ~= agent_id then
            return true
        elseif string.sub(event_data.message, 1, 5) == '[LUA]' then
            -- Keep returning true until Eliza says "bye...", then return false
            if string.lower(string.sub(event_data.message, 6, 8)) == 'bye' then
                done_talking:close()
                return false
            else
                return true
            end
        else
            -- TODO: LLAgent query to get agent name
            print(`Me: {event_data.message}`)
            answer = Eliza(event_data.message)
            print(`Liz: {answer}`)
            LLChat.sendNearby(answer)
            return true
        end
    end

    chat_listener:start()
    LLChat.sendNearby('Hello, I am the doctor. What is your problem?')
    done_talking:Dequeue()
    LLAgent.requestStand()
    LLGesture.startGesture(gestures.afk)
end
