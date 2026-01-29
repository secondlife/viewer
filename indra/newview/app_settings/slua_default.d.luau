
----------------------------------
---------- LSL LUAU DEFS ---------
----------------------------------

declare extern type quaternion with
  x: number
  y: number
  z: number
  s: number
  function __add(self, other: quaternion): quaternion
  function __sub(self, other: quaternion): quaternion
  function __mul(self, other: quaternion): quaternion
  function __div(self, other: quaternion): quaternion
  function __unm(self): quaternion
  function __eq(self, other: quaternion): boolean
  function __tostring(self): string
end

declare extern type uuid with
  istruthy: boolean
  bytes: string?
  function __tostring(self): string
end

declare extern type vector with
  x: number
  y: number
  z: number
  function __add(self, other: vector): vector
  function __sub(self, other: vector): vector
  function __unm(self): vector
  function __mul(self, other: number | vector | quaternion): vector
  function __div(self, other: number | vector | quaternion): vector
  function __mod(self, other: vector): vector
  function __tostring(self): string
end


export type rotation = quaternion
export type list = {string | number | vector | uuid | quaternion | boolean}
type LLDetectedEventName = "collision" | "collision_end" | "collision_start" | "final_damage" | "on_damage" | "sensor" | "touch" | "touch_end" | "touch_start"
type LLNonDetectedEventName = "at_rot_target" | "at_target" | "attach" | "changed" | "control" | "dataserver" | "email" | "experience_permissions" | "experience_permissions_denied" | "game_control" | "http_request" | "http_response" | "land_collision" | "land_collision_end" | "land_collision_start" | "link_message" | "linkset_data" | "listen" | "money" | "moving_end" | "moving_start" | "no_sensor" | "not_at_rot_target" | "not_at_target" | "object_rez" | "on_death" | "on_rez" | "path_update" | "remote_data" | "run_time_permissions" | "timer" | "transaction_result"
type LLEventName = LLDetectedEventName | LLNonDetectedEventName
type LLEventHandler = (...any) -> ()
type LLDetectedEventHandler = (detected: {DetectedEvent}) -> ()
type LLTimerEveryCallback = (scheduled: number, interval: number) -> ()
type LLTimerOnceCallback = (scheduled: number) -> ()
type LLTimerCallback = LLTimerEveryCallback | LLTimerOnceCallback
export type OsDateTime = {
  year: number,
  month: number,
  day: number,
  hour: number?,
  min: number?,
  sec: number?,
  wday: number?,
  yday: number?,
  isdst: boolean?,
}

declare extern type DetectedEvent with
  index: number
  valid: boolean
  canAdjustDamage: boolean
  function adjustDamage(self, Damage: number): ()
  function getDamage(self): list
  function getGrab(self): vector
  function getGroup(self): boolean
  function getKey(self): uuid
  function getLinkNumber(self): number
  function getName(self): string
  function getOwner(self): uuid
  function getPos(self): vector
  function getRezzer(self): uuid
  function getRot(self): quaternion
  function getTouchBinormal(self): vector
  function getTouchFace(self): number
  function getTouchNormal(self): vector
  function getTouchPos(self): vector
  function getTouchST(self): vector
  function getTouchUV(self): vector
  function getType(self): number
  function getVel(self): vector
end

declare extern type LLEvents with
  at_rot_target: ((TargetNumber: number, TargetRotation: quaternion, CurrentRotation: quaternion) -> ())?
  at_target: ((TargetNumber: number, TargetPosition: vector, CurrentPosition: vector) -> ())?
  attach: ((AvatarID: uuid) -> ())?
  changed: ((Changed: number) -> ())?
  collision: (LLDetectedEventHandler)?
  collision_end: (LLDetectedEventHandler)?
  collision_start: (LLDetectedEventHandler)?
  control: ((AvatarID: uuid, Levels: number, Edges: number) -> ())?
  dataserver: ((RequestID: uuid, Data: string) -> ())?
  email: ((Time: string, Address: string, Subject: string, Body: string, NumberRemaining: number) -> ())?
  experience_permissions: ((agent_id: uuid) -> ())?
  experience_permissions_denied: ((agent_id: uuid, Reason: number) -> ())?
  final_damage: (LLDetectedEventHandler)?
  game_control: ((id: uuid, buttons: number, axes: list) -> ())?
  http_request: ((HTTPRequestID: uuid, HTTPMethod: string, Body: string) -> ())?
  http_response: ((HTTPRequestID: uuid, Status: number, Metadata: list, Body: string) -> ())?
  land_collision: ((Position: vector) -> ())?
  land_collision_end: ((Position: vector) -> ())?
  land_collision_start: ((Position: vector) -> ())?
  link_message: ((SendersLink: number, Value: number, Text: string, ID: string) -> ())?
  linkset_data: ((action: number, name: string, value: string) -> ())?
  listen: ((Channel: number, Name: string, ID: uuid, Text: string) -> ())?
  money: ((Payer: uuid, Amount: number) -> ())?
  moving_end: (() -> ())?
  moving_start: (() -> ())?
  no_sensor: (() -> ())?
  not_at_rot_target: (() -> ())?
  not_at_target: (() -> ())?
  object_rez: ((RezzedObjectsID: uuid) -> ())?
  on_damage: (LLDetectedEventHandler)?
  on_death: (() -> ())?
  on_rez: ((StartParameter: number) -> ())?
  path_update: ((Type: number, Reserved: list) -> ())?
  remote_data: ((EventType: number, ChannelID: uuid, MessageID: uuid, Sender: string, IData: number, SData: string) -> ())?
  run_time_permissions: ((PermissionFlags: number) -> ())?
  sensor: (LLDetectedEventHandler)?
  timer: (() -> ())?
  touch: (LLDetectedEventHandler)?
  touch_end: (LLDetectedEventHandler)?
  touch_start: (LLDetectedEventHandler)?
  transaction_result: ((RequestID: uuid, Success: number, Message: string) -> ())?
  on: ((self: LLEvents, event: LLDetectedEventName, callback: LLDetectedEventHandler) -> LLDetectedEventHandler)
    & ((self: LLEvents, event: "at_rot_target", callback: (TargetNumber: number, TargetRotation: quaternion, CurrentRotation: quaternion) -> ()) -> (TargetNumber: number, TargetRotation: quaternion, CurrentRotation: quaternion) -> ())
    & ((self: LLEvents, event: "at_target", callback: (TargetNumber: number, TargetPosition: vector, CurrentPosition: vector) -> ()) -> (TargetNumber: number, TargetPosition: vector, CurrentPosition: vector) -> ())
    & ((self: LLEvents, event: "attach", callback: (AvatarID: uuid) -> ()) -> (AvatarID: uuid) -> ())
    & ((self: LLEvents, event: "changed", callback: (Changed: number) -> ()) -> (Changed: number) -> ())
    & ((self: LLEvents, event: "control", callback: (AvatarID: uuid, Levels: number, Edges: number) -> ()) -> (AvatarID: uuid, Levels: number, Edges: number) -> ())
    & ((self: LLEvents, event: "dataserver", callback: (RequestID: uuid, Data: string) -> ()) -> (RequestID: uuid, Data: string) -> ())
    & ((self: LLEvents, event: "email", callback: (Time: string, Address: string, Subject: string, Body: string, NumberRemaining: number) -> ()) -> (Time: string, Address: string, Subject: string, Body: string, NumberRemaining: number) -> ())
    & ((self: LLEvents, event: "experience_permissions", callback: (agent_id: uuid) -> ()) -> (agent_id: uuid) -> ())
    & ((self: LLEvents, event: "experience_permissions_denied", callback: (agent_id: uuid, Reason: number) -> ()) -> (agent_id: uuid, Reason: number) -> ())
    & ((self: LLEvents, event: "game_control", callback: (id: uuid, buttons: number, axes: list) -> ()) -> (id: uuid, buttons: number, axes: list) -> ())
    & ((self: LLEvents, event: "http_request", callback: (HTTPRequestID: uuid, HTTPMethod: string, Body: string) -> ()) -> (HTTPRequestID: uuid, HTTPMethod: string, Body: string) -> ())
    & ((self: LLEvents, event: "http_response", callback: (HTTPRequestID: uuid, Status: number, Metadata: list, Body: string) -> ()) -> (HTTPRequestID: uuid, Status: number, Metadata: list, Body: string) -> ())
    & ((self: LLEvents, event: "land_collision", callback: (Position: vector) -> ()) -> (Position: vector) -> ())
    & ((self: LLEvents, event: "land_collision_end", callback: (Position: vector) -> ()) -> (Position: vector) -> ())
    & ((self: LLEvents, event: "land_collision_start", callback: (Position: vector) -> ()) -> (Position: vector) -> ())
    & ((self: LLEvents, event: "link_message", callback: (SendersLink: number, Value: number, Text: string, ID: string) -> ()) -> (SendersLink: number, Value: number, Text: string, ID: string) -> ())
    & ((self: LLEvents, event: "linkset_data", callback: (action: number, name: string, value: string) -> ()) -> (action: number, name: string, value: string) -> ())
    & ((self: LLEvents, event: "listen", callback: (Channel: number, Name: string, ID: uuid, Text: string) -> ()) -> (Channel: number, Name: string, ID: uuid, Text: string) -> ())
    & ((self: LLEvents, event: "money", callback: (Payer: uuid, Amount: number) -> ()) -> (Payer: uuid, Amount: number) -> ())
    & ((self: LLEvents, event: "moving_end", callback: () -> ()) -> () -> ())
    & ((self: LLEvents, event: "moving_start", callback: () -> ()) -> () -> ())
    & ((self: LLEvents, event: "no_sensor", callback: () -> ()) -> () -> ())
    & ((self: LLEvents, event: "not_at_rot_target", callback: () -> ()) -> () -> ())
    & ((self: LLEvents, event: "not_at_target", callback: () -> ()) -> () -> ())
    & ((self: LLEvents, event: "object_rez", callback: (RezzedObjectsID: uuid) -> ()) -> (RezzedObjectsID: uuid) -> ())
    & ((self: LLEvents, event: "on_death", callback: () -> ()) -> () -> ())
    & ((self: LLEvents, event: "on_rez", callback: (StartParameter: number) -> ()) -> (StartParameter: number) -> ())
    & ((self: LLEvents, event: "path_update", callback: (Type: number, Reserved: list) -> ()) -> (Type: number, Reserved: list) -> ())
    & ((self: LLEvents, event: "remote_data", callback: (EventType: number, ChannelID: uuid, MessageID: uuid, Sender: string, IData: number, SData: string) -> ()) -> (EventType: number, ChannelID: uuid, MessageID: uuid, Sender: string, IData: number, SData: string) -> ())
    & ((self: LLEvents, event: "run_time_permissions", callback: (PermissionFlags: number) -> ()) -> (PermissionFlags: number) -> ())
    & ((self: LLEvents, event: "timer", callback: () -> ()) -> () -> ())
    & ((self: LLEvents, event: "transaction_result", callback: (RequestID: uuid, Success: number, Message: string) -> ()) -> (RequestID: uuid, Success: number, Message: string) -> ())
  off: ((self: LLEvents, event: LLDetectedEventName, callback: LLDetectedEventHandler) -> boolean)
    & ((self: LLEvents, event: "at_rot_target", callback: (TargetNumber: number, TargetRotation: quaternion, CurrentRotation: quaternion) -> ()) -> boolean)
    & ((self: LLEvents, event: "at_target", callback: (TargetNumber: number, TargetPosition: vector, CurrentPosition: vector) -> ()) -> boolean)
    & ((self: LLEvents, event: "attach", callback: (AvatarID: uuid) -> ()) -> boolean)
    & ((self: LLEvents, event: "changed", callback: (Changed: number) -> ()) -> boolean)
    & ((self: LLEvents, event: "control", callback: (AvatarID: uuid, Levels: number, Edges: number) -> ()) -> boolean)
    & ((self: LLEvents, event: "dataserver", callback: (RequestID: uuid, Data: string) -> ()) -> boolean)
    & ((self: LLEvents, event: "email", callback: (Time: string, Address: string, Subject: string, Body: string, NumberRemaining: number) -> ()) -> boolean)
    & ((self: LLEvents, event: "experience_permissions", callback: (agent_id: uuid) -> ()) -> boolean)
    & ((self: LLEvents, event: "experience_permissions_denied", callback: (agent_id: uuid, Reason: number) -> ()) -> boolean)
    & ((self: LLEvents, event: "game_control", callback: (id: uuid, buttons: number, axes: list) -> ()) -> boolean)
    & ((self: LLEvents, event: "http_request", callback: (HTTPRequestID: uuid, HTTPMethod: string, Body: string) -> ()) -> boolean)
    & ((self: LLEvents, event: "http_response", callback: (HTTPRequestID: uuid, Status: number, Metadata: list, Body: string) -> ()) -> boolean)
    & ((self: LLEvents, event: "land_collision", callback: (Position: vector) -> ()) -> boolean)
    & ((self: LLEvents, event: "land_collision_end", callback: (Position: vector) -> ()) -> boolean)
    & ((self: LLEvents, event: "land_collision_start", callback: (Position: vector) -> ()) -> boolean)
    & ((self: LLEvents, event: "link_message", callback: (SendersLink: number, Value: number, Text: string, ID: string) -> ()) -> boolean)
    & ((self: LLEvents, event: "linkset_data", callback: (action: number, name: string, value: string) -> ()) -> boolean)
    & ((self: LLEvents, event: "listen", callback: (Channel: number, Name: string, ID: uuid, Text: string) -> ()) -> boolean)
    & ((self: LLEvents, event: "money", callback: (Payer: uuid, Amount: number) -> ()) -> boolean)
    & ((self: LLEvents, event: "moving_end", callback: () -> ()) -> boolean)
    & ((self: LLEvents, event: "moving_start", callback: () -> ()) -> boolean)
    & ((self: LLEvents, event: "no_sensor", callback: () -> ()) -> boolean)
    & ((self: LLEvents, event: "not_at_rot_target", callback: () -> ()) -> boolean)
    & ((self: LLEvents, event: "not_at_target", callback: () -> ()) -> boolean)
    & ((self: LLEvents, event: "object_rez", callback: (RezzedObjectsID: uuid) -> ()) -> boolean)
    & ((self: LLEvents, event: "on_death", callback: () -> ()) -> boolean)
    & ((self: LLEvents, event: "on_rez", callback: (StartParameter: number) -> ()) -> boolean)
    & ((self: LLEvents, event: "path_update", callback: (Type: number, Reserved: list) -> ()) -> boolean)
    & ((self: LLEvents, event: "remote_data", callback: (EventType: number, ChannelID: uuid, MessageID: uuid, Sender: string, IData: number, SData: string) -> ()) -> boolean)
    & ((self: LLEvents, event: "run_time_permissions", callback: (PermissionFlags: number) -> ()) -> boolean)
    & ((self: LLEvents, event: "timer", callback: () -> ()) -> boolean)
    & ((self: LLEvents, event: "transaction_result", callback: (RequestID: uuid, Success: number, Message: string) -> ()) -> boolean)
  once: ((self: LLEvents, event: LLDetectedEventName, callback: LLDetectedEventHandler) -> LLDetectedEventHandler)
    & ((self: LLEvents, event: "at_rot_target", callback: (TargetNumber: number, TargetRotation: quaternion, CurrentRotation: quaternion) -> ()) -> (TargetNumber: number, TargetRotation: quaternion, CurrentRotation: quaternion) -> ())
    & ((self: LLEvents, event: "at_target", callback: (TargetNumber: number, TargetPosition: vector, CurrentPosition: vector) -> ()) -> (TargetNumber: number, TargetPosition: vector, CurrentPosition: vector) -> ())
    & ((self: LLEvents, event: "attach", callback: (AvatarID: uuid) -> ()) -> (AvatarID: uuid) -> ())
    & ((self: LLEvents, event: "changed", callback: (Changed: number) -> ()) -> (Changed: number) -> ())
    & ((self: LLEvents, event: "control", callback: (AvatarID: uuid, Levels: number, Edges: number) -> ()) -> (AvatarID: uuid, Levels: number, Edges: number) -> ())
    & ((self: LLEvents, event: "dataserver", callback: (RequestID: uuid, Data: string) -> ()) -> (RequestID: uuid, Data: string) -> ())
    & ((self: LLEvents, event: "email", callback: (Time: string, Address: string, Subject: string, Body: string, NumberRemaining: number) -> ()) -> (Time: string, Address: string, Subject: string, Body: string, NumberRemaining: number) -> ())
    & ((self: LLEvents, event: "experience_permissions", callback: (agent_id: uuid) -> ()) -> (agent_id: uuid) -> ())
    & ((self: LLEvents, event: "experience_permissions_denied", callback: (agent_id: uuid, Reason: number) -> ()) -> (agent_id: uuid, Reason: number) -> ())
    & ((self: LLEvents, event: "game_control", callback: (id: uuid, buttons: number, axes: list) -> ()) -> (id: uuid, buttons: number, axes: list) -> ())
    & ((self: LLEvents, event: "http_request", callback: (HTTPRequestID: uuid, HTTPMethod: string, Body: string) -> ()) -> (HTTPRequestID: uuid, HTTPMethod: string, Body: string) -> ())
    & ((self: LLEvents, event: "http_response", callback: (HTTPRequestID: uuid, Status: number, Metadata: list, Body: string) -> ()) -> (HTTPRequestID: uuid, Status: number, Metadata: list, Body: string) -> ())
    & ((self: LLEvents, event: "land_collision", callback: (Position: vector) -> ()) -> (Position: vector) -> ())
    & ((self: LLEvents, event: "land_collision_end", callback: (Position: vector) -> ()) -> (Position: vector) -> ())
    & ((self: LLEvents, event: "land_collision_start", callback: (Position: vector) -> ()) -> (Position: vector) -> ())
    & ((self: LLEvents, event: "link_message", callback: (SendersLink: number, Value: number, Text: string, ID: string) -> ()) -> (SendersLink: number, Value: number, Text: string, ID: string) -> ())
    & ((self: LLEvents, event: "linkset_data", callback: (action: number, name: string, value: string) -> ()) -> (action: number, name: string, value: string) -> ())
    & ((self: LLEvents, event: "listen", callback: (Channel: number, Name: string, ID: uuid, Text: string) -> ()) -> (Channel: number, Name: string, ID: uuid, Text: string) -> ())
    & ((self: LLEvents, event: "money", callback: (Payer: uuid, Amount: number) -> ()) -> (Payer: uuid, Amount: number) -> ())
    & ((self: LLEvents, event: "moving_end", callback: () -> ()) -> () -> ())
    & ((self: LLEvents, event: "moving_start", callback: () -> ()) -> () -> ())
    & ((self: LLEvents, event: "no_sensor", callback: () -> ()) -> () -> ())
    & ((self: LLEvents, event: "not_at_rot_target", callback: () -> ()) -> () -> ())
    & ((self: LLEvents, event: "not_at_target", callback: () -> ()) -> () -> ())
    & ((self: LLEvents, event: "object_rez", callback: (RezzedObjectsID: uuid) -> ()) -> (RezzedObjectsID: uuid) -> ())
    & ((self: LLEvents, event: "on_death", callback: () -> ()) -> () -> ())
    & ((self: LLEvents, event: "on_rez", callback: (StartParameter: number) -> ()) -> (StartParameter: number) -> ())
    & ((self: LLEvents, event: "path_update", callback: (Type: number, Reserved: list) -> ()) -> (Type: number, Reserved: list) -> ())
    & ((self: LLEvents, event: "remote_data", callback: (EventType: number, ChannelID: uuid, MessageID: uuid, Sender: string, IData: number, SData: string) -> ()) -> (EventType: number, ChannelID: uuid, MessageID: uuid, Sender: string, IData: number, SData: string) -> ())
    & ((self: LLEvents, event: "run_time_permissions", callback: (PermissionFlags: number) -> ()) -> (PermissionFlags: number) -> ())
    & ((self: LLEvents, event: "timer", callback: () -> ()) -> () -> ())
    & ((self: LLEvents, event: "transaction_result", callback: (RequestID: uuid, Success: number, Message: string) -> ()) -> (RequestID: uuid, Success: number, Message: string) -> ())
  function listeners(self, event: LLEventName): {LLEventHandler}
  function eventNames(self): {string}
end

declare extern type LLTimers with
  function every(self, seconds: number, callback: LLTimerEveryCallback): LLTimerCallback
  function once(self, seconds: number, callback: LLTimerOnceCallback): LLTimerCallback
  function off(self, callback: LLTimerCallback): boolean
end


declare function dangerouslyexecuterequiredmodule(f: (...any) -> ...any): ...any
declare function touuid(val: string | buffer | uuid): uuid?
declare function tovector(val: string | vector): vector?
declare function toquaternion(val: string | quaternion): quaternion?
declare function torotation(val: string | quaternion): quaternion?

---------------------------
-- Global Table: bit32
---------------------------

declare bit32: {
  arshift: (n: number, i: number) -> number,
  band: (...number) -> number,
  bnot: (n: number) -> number,
  bor: (...number) -> number,
  bxor: (...number) -> number,
  btest: (...number) -> boolean,
  extract: (n: number, field: number, width: number?) -> number,
  lrotate: (n: number, i: number) -> number,
  lshift: (n: number, i: number) -> number,
  replace: (n: number, v: number, field: number, width: number?) -> number,
  rrotate: (n: number, i: number) -> number,
  rshift: (n: number, i: number) -> number,
  countlz: (n: number) -> number,
  countrz: (n: number) -> number,
  byteswap: (n: number) -> number,
}


---------------------------
-- Global Table: buffer
---------------------------

declare buffer: {
  create: (size: number) -> buffer,
  fromstring: (str: string) -> buffer,
  tostring: (b: buffer) -> string,
  readi8: (b: buffer, offset: number) -> number,
  readu8: (b: buffer, offset: number) -> number,
  readi16: (b: buffer, offset: number) -> number,
  readu16: (b: buffer, offset: number) -> number,
  readi32: (b: buffer, offset: number) -> number,
  readu32: (b: buffer, offset: number) -> number,
  readf32: (b: buffer, offset: number) -> number,
  readf64: (b: buffer, offset: number) -> number,
  writei8: (b: buffer, offset: number, value: number) -> (),
  writeu8: (b: buffer, offset: number, value: number) -> (),
  writei16: (b: buffer, offset: number, value: number) -> (),
  writeu16: (b: buffer, offset: number, value: number) -> (),
  writei32: (b: buffer, offset: number, value: number) -> (),
  writeu32: (b: buffer, offset: number, value: number) -> (),
  writef32: (b: buffer, offset: number, value: number) -> (),
  writef64: (b: buffer, offset: number, value: number) -> (),
  readstring: (b: buffer, offset: number, count: number) -> string,
  writestring: (b: buffer, offset: number, value: string, count: number?) -> (),
  len: (b: buffer) -> number,
  copy: (target: buffer, targetOffset: number, source: buffer, sourceOffset: number?, count: number?) -> (),
  fill: (b: buffer, offset: number, value: number, count: number?) -> (),
  readbits: (b: buffer, bitOffset: number, bitCount: number) -> number,
  writebits: (b: buffer, bitOffset: number, bitCount: number, value: number) -> (),
}


---------------------------
-- Global Table: coroutine
---------------------------

declare coroutine: {
  create: (f: (...any) -> ...any) -> thread,
  resume: (co: thread, ...any) -> (boolean, ...any),
  running: () -> thread?,
  status: (co: thread) -> "running" | "suspended" | "normal" | "dead",
  wrap: (f: (...any) -> ...any) -> (...any) -> ...any,
  yield: (...any) -> ...any,
  isyieldable: () -> boolean,
  close: (co: thread) -> (boolean, string?),
}


---------------------------
-- Global Table: debug
---------------------------

declare debug: {
  info: ((co: thread | ((...any) -> ...any) | number, level: number, s: string) -> ...any)
    & ((level: number, s: string) -> ...any)
    & ((func: (...any) -> ...any, s: string) -> ...any),
  traceback: ((co: thread, msg: string?, level: number?) -> string)
    & ((message: string?, level: number?) -> string),
}


---------------------------
-- Global Table: llbase64
---------------------------

declare llbase64: {
  encode: (data: string | buffer) -> string,
  decode: ((data: string) -> string)
    & ((data: string, asBuffer: boolean?) -> string | buffer),
}


---------------------------
-- Global Table: lljson
---------------------------

declare lljson: {
  null: any,
  empty_array_mt: { [any]: any },
  array_mt: { [any]: any },
  empty_array: any,
  _NAME: string,
  _VERSION: string,
  encode: (value: any) -> string,
  decode: (json: string) -> any,
  slencode: (value: any, tight: boolean?) -> string,
  sldecode: (json: string) -> any,
}


---------------------------
-- Global Table: math
---------------------------

declare math: {
  pi: number,
  huge: number,
  abs: (n: number) -> number,
  acos: (n: number) -> number,
  asin: (n: number) -> number,
  atan: (n: number) -> number,
  atan2: (y: number, x: number) -> number,
  ceil: (n: number) -> number,
  clamp: (n: number, min: number, max: number) -> number,
  cos: (n: number) -> number,
  cosh: (n: number) -> number,
  deg: (n: number) -> number,
  exp: (n: number) -> number,
  floor: (n: number) -> number,
  fmod: (x: number, y: number) -> number,
  frexp: (n: number) -> (number, number),
  ldexp: (s: number, e: number) -> number,
  lerp: (a: number, b: number, t: number) -> number,
  log: (n: number, base: number?) -> number,
  log10: (n: number) -> number,
  map: (n: number, inMin: number, inMax: number, outMin: number, outMax: number) -> number,
  max: (n: number, ...number) -> number,
  min: (n: number, ...number) -> number,
  modf: (n: number) -> (number, number),
  noise: (x: number, y: number?, z: number?) -> number,
  pow: (base: number, exponent: number) -> number,
  rad: (n: number) -> number,
  random: (min: number?, max: number?) -> number,
  randomseed: (seed: number) -> (),
  round: (n: number) -> number,
  sign: (n: number) -> number,
  sin: (n: number) -> number,
  sinh: (n: number) -> number,
  sqrt: (n: number) -> number,
  tan: (n: number) -> number,
  tanh: (n: number) -> number,
  isnan: (n: number) -> boolean,
  isinf: (n: number) -> boolean,
  isfinite: (n: number) -> boolean,
}


---------------------------
-- Global Table: os
---------------------------

declare os: {
  clock: () -> number,
  date: (s: string?, t: number?) -> (string | OsDateTime)?,
  difftime: (a: number, b: number?) -> number,
  time: (t: OsDateTime?) -> number?,
}


---------------------------
-- Global Table: quaternion
---------------------------

declare quaternion: ((x: number, y: number, z: number, s: number) -> quaternion) & {
  identity: quaternion,
  create: (x: number, y: number, z: number, s: number) -> quaternion,
  normalize: (q: quaternion) -> quaternion,
  magnitude: (q: quaternion) -> number,
  dot: (a: quaternion, b: quaternion) -> number,
  slerp: (a: quaternion, b: quaternion, t: number) -> quaternion,
  conjugate: (q: quaternion) -> quaternion,
  tofwd: (q: quaternion) -> vector,
  toleft: (q: quaternion) -> vector,
  toup: (q: quaternion) -> vector,
}


---------------------------
-- Global Table: string
---------------------------

declare string: {
  byte: (s: string, i: number?, j: number?) -> ...number,
  char: (...number) -> string,
  find: (s: string, pattern: string, init: number?, plain: boolean?) -> (number?, number?, ...string),
  format: (formatstring: string, ...any) -> string,
  gmatch: (s: string, pattern: string) -> () -> ...string,
  gsub: (s: string, pattern: string, repl: string | { [string]: string } | (...string) -> string, maxn: number?) -> (string, number),
  len: (s: string) -> number,
  lower: (s: string) -> string,
  match: (s: string, pattern: string, init: number?) -> ...string,
  pack: (fmt: string, ...any) -> string,
  packsize: (fmt: string) -> number,
  rep: (s: string, n: number) -> string,
  reverse: (s: string) -> string,
  split: (s: string, separator: string?) -> {string},
  sub: (s: string, i: number, j: number?) -> string,
  unpack: (fmt: string, s: string, init: number?) -> ...any,
  upper: (s: string) -> string,
}


---------------------------
-- Global Table: table
---------------------------

declare table: {
  concat: (a: {string | number}, sep: string?, i: number?, j: number?) -> string,
  foreach: @deprecated <K, V, R>(t: {[K]: V}, f: (key: K, value: V) -> R?) -> R?,
  foreachi: @deprecated <V, R>(a: {V}, f: (index: number, value: V) -> R?) -> R?,
  getn: @deprecated (a: {any}) -> number,
  maxn: (t: {any}) -> number,
  insert: (<V>(a: {V}, i: number, v: V) -> ())
    & (<V>(a: {V}, value: V) -> ()),
  remove: <V>(a: {V}, i: number?) -> V?,
  sort: <V>(a: {V}, f: ((a: V, b: V) -> boolean)?) -> (),
  pack: <V>(...V) -> { n: number, [number]: V },
  unpack: <V>(a: {V}, i: number?, j: number?) -> ...V,
  move: <V>(src: {V}, i: number, j: number, d: number, dest: {V}?) -> {V},
  create: <V>(n: number, v: V?) -> {V},
  find: <V>(t: {V}, v: V, i: number?) -> number?,
  clear: (t: {[any]: any}) -> (),
  shrink: <K, V>(t: {[K]: V}, reorder: boolean?) -> {[K]: V},
  freeze: <K, V>(t: {[K]: V}) -> {[K]: V},
  isfrozen: (t: {[any]: any}) -> boolean,
  clone: <K, V>(t: {[K]: V}) -> {[K]: V},
}


---------------------------
-- Global Table: utf8
---------------------------

declare utf8: {
  charpattern: string,
  char: (...number) -> string,
  codes: (s: string) -> ((string, number) -> (number, number), string, number),
  codepoint: (s: string, i: number?, j: number?) -> ...number,
  len: (s: string, i: number?, j: number?) -> (number?, number?),
  offset: (s: string, n: number, i: number?) -> number?,
}


---------------------------
-- Global Table: uuid
---------------------------

declare uuid: ((value: string | buffer | uuid) -> uuid?) & {
  create: (value: string | buffer | uuid) -> uuid?,
}


---------------------------
-- Global Table: vector
---------------------------

declare vector: ((x: number, y: number, z: number?) -> vector) & {
  zero: vector,
  one: vector,
  create: (x: number, y: number, z: number?) -> vector,
  magnitude: (v: vector) -> number,
  normalize: (v: vector) -> vector,
  cross: (a: vector, b: vector) -> vector,
  dot: (a: vector, b: vector) -> number,
  angle: (a: vector, b: vector, axis: vector?) -> number,
  floor: (v: vector) -> vector,
  ceil: (v: vector) -> vector,
  abs: (v: vector) -> vector,
  sign: (v: vector) -> vector,
  clamp: (v: vector, min: vector, max: vector) -> vector,
  max: (v: vector, ...vector) -> vector,
  min: (v: vector, ...vector) -> vector,
  lerp: (a: vector, b: vector, t: number) -> vector,
}

declare rotation: typeof(quaternion)
declare LLEvents: LLEvents
declare LLTimers: LLTimers
declare loadstring: nil
declare getfenv: nil
declare setfenv: nil

---------------------------
-- Global Table: ll
---------------------------

declare ll: {
  Abs: (Value: number) -> number,
  Acos: (Value: number) -> number,
  AddToLandBanList: (ID: uuid, Hours: number) -> (),
  AddToLandPassList: (ID: uuid, Hours: number) -> (),
  AdjustDamage: @deprecated (Number: number, Damage: number) -> (),
  AdjustSoundVolume: (Volume: number) -> (),
  AgentInExperience: (AgentID: uuid) -> boolean,
  AllowInventoryDrop: (Flag: boolean | number) -> (),
  AngleBetween: (Rot1: quaternion, Rot2: quaternion) -> number,
  ApplyImpulse: (Force: vector, Local: boolean | number) -> (),
  ApplyRotationalImpulse: (Force: vector, Local: boolean | number) -> (),
  Asin: (Value: number) -> number,
  Atan2: (y: number, x: number) -> number,
  AttachToAvatar: (AttachmentPoint: number) -> (),
  AttachToAvatarTemp: (AttachPoint: number) -> (),
  AvatarOnLinkSitTarget: (LinkNumber: number) -> uuid,
  AvatarOnSitTarget: () -> uuid,
  Axes2Rot: (Forward: vector, Left: vector, Up: vector) -> quaternion,
  AxisAngle2Rot: (Axis: vector, Angle: number) -> quaternion,
  Base64ToInteger: (Text: string) -> number,
  Base64ToString: (Text: string) -> string,
  BreakAllLinks: () -> (),
  BreakLink: (LinkNumber: number) -> (),
  CSV2List: (Text: string) -> {string},
  CastRay: (Start: vector, End: vector, Options: list) -> list,
  Ceil: (Value: number) -> number,
  Char: (value: number) -> string,
  ClearCameraParams: () -> (),
  ClearLinkMedia: (Link: number, Face: number) -> number,
  ClearPrimMedia: (Face: number) -> number,
  CloseRemoteDataChannel: @deprecated (ChannelID: uuid) -> (),
  Cloud: @deprecated (Offset: vector) -> number,
  CollisionFilter: (ObjectName: string, ObjectID: uuid, Accept: boolean | number) -> (),
  CollisionSound: (ImpactSound: string, ImpactVolume: number) -> (),
  CollisionSprite: @deprecated (ImpactSprite: string) -> (),
  ComputeHash: (Message: string, Algorithm: string) -> string,
  Cos: (Theta: number) -> number,
  CreateCharacter: (Options: list) -> (),
  CreateKeyValue: (Key: string, Value: string) -> uuid,
  CreateLink: (TargetPrim: uuid, Parent: boolean | number) -> (),
  Damage: (target: uuid, damage: number, type: number) -> (),
  DataSizeKeyValue: () -> uuid,
  DeleteCharacter: () -> (),
  DeleteKeyValue: (Key: string) -> uuid,
  DeleteSubList: <T>(Source: {T}, Start: number, End: number) -> {T},
  DeleteSubString: (Source: string, Start: number, End: number) -> string,
  DerezObject: (ID: uuid, flags: number) -> boolean,
  DetachFromAvatar: () -> (),
  DetectedDamage: @deprecated (Number: number) -> list,
  DetectedGrab: @deprecated (Number: number) -> vector,
  DetectedGroup: @deprecated (Number: number) -> boolean,
  DetectedKey: @deprecated (Number: number) -> uuid,
  DetectedLinkNumber: @deprecated (Number: number) -> number,
  DetectedName: @deprecated (Number: number) -> string,
  DetectedOwner: @deprecated (Number: number) -> uuid,
  DetectedPos: @deprecated (Number: number) -> vector,
  DetectedRezzer: @deprecated (Number: number) -> uuid,
  DetectedRot: @deprecated (Number: number) -> quaternion,
  DetectedTouchBinormal: @deprecated (Index: number) -> vector,
  DetectedTouchFace: @deprecated (Index: number) -> number,
  DetectedTouchNormal: @deprecated (Index: number) -> vector,
  DetectedTouchPos: @deprecated (Index: number) -> vector,
  DetectedTouchST: @deprecated (Index: number) -> vector,
  DetectedTouchUV: @deprecated (Index: number) -> vector,
  DetectedType: @deprecated (Number: number) -> number,
  DetectedVel: @deprecated (Number: number) -> vector,
  Dialog: (AvatarID: uuid, Text: string, Buttons: list, Channel: number) -> (),
  Die: () -> (),
  DumpList2String: (Source: list, Separator: string) -> string,
  EdgeOfWorld: (Position: vector, Direction: vector) -> boolean,
  EjectFromLand: (AvatarID: uuid) -> (),
  Email: (Address: string, Subject: string, Text: string) -> (),
  EscapeURL: (URL: string) -> string,
  Euler2Rot: (Vector: vector) -> quaternion,
  Evade: (TargetID: uuid, Options: list) -> (),
  ExecCharacterCmd: (Command: number, Options: list) -> (),
  Fabs: (Value: number) -> number,
  FindNotecardTextCount: (NotecardName: string, Pattern: string, Options: list) -> uuid,
  FindNotecardTextSync: (NotecardName: string, Pattern: string, StartMatch: number, Count: number, Options: list) -> list,
  FleeFrom: (Source: vector, Distance: number, Options: list) -> (),
  Floor: (Value: number) -> number,
  ForceMouselook: (Enable: boolean | number) -> (),
  Frand: (Magnitude: number) -> number,
  GenerateKey: () -> uuid,
  GetAccel: () -> vector,
  GetAgentInfo: (AvatarID: uuid) -> number,
  GetAgentLanguage: (AvatarID: uuid) -> string,
  GetAgentList: (Scope: number, Options: list) -> list,
  GetAgentSize: (AvatarID: uuid) -> vector,
  GetAlpha: (Face: number) -> number,
  GetAnimation: (AvatarID: uuid) -> string,
  GetAnimationList: (AvatarID: uuid) -> list,
  GetAnimationOverride: (AnimationState: string) -> string,
  GetAttached: () -> number,
  GetAttachedList: (ID: uuid) -> {uuid},
  GetAttachedListFiltered: (AgentID: uuid, Options: list) -> {uuid},
  GetBoundingBox: (ID: uuid) -> {vector},
  GetCameraAspect: () -> number,
  GetCameraFOV: () -> number,
  GetCameraPos: () -> vector,
  GetCameraRot: () -> quaternion,
  GetCenterOfMass: () -> vector,
  GetClosestNavPoint: (Point: vector, Options: list) -> list,
  GetColor: (Face: number) -> vector,
  GetCreator: () -> uuid,
  GetDate: () -> string,
  GetDayLength: () -> number,
  GetDayOffset: () -> number,
  GetDisplayName: (AvatarID: uuid) -> string,
  GetEnergy: () -> number,
  GetEnv: (DataRequest: string) -> string,
  GetEnvironment: (Position: vector, EnvParams: list) -> list,
  GetExperienceDetails: (ExperienceID: uuid) -> list,
  GetExperienceErrorMessage: (Error: number) -> string,
  GetForce: () -> vector,
  GetFreeMemory: () -> number,
  GetFreeURLs: () -> number,
  GetGMTclock: () -> number,
  GetGeometricCenter: () -> vector,
  GetHTTPHeader: (HTTPRequestID: uuid, Header: string) -> string,
  GetHealth: (ID: uuid) -> number,
  GetInventoryAcquireTime: (InventoryItem: string) -> string,
  GetInventoryCreator: (InventoryItem: string) -> uuid,
  GetInventoryDesc: (InventoryItem: string) -> string,
  GetInventoryKey: (InventoryItem: string) -> uuid,
  GetInventoryName: (InventoryType: number, Index: number) -> string,
  GetInventoryNumber: (InventoryType: number) -> number,
  GetInventoryPermMask: (InventoryItem: string, BitMask: number) -> number,
  GetInventoryType: (InventoryItem: string) -> number,
  GetKey: () -> uuid,
  GetLandOwnerAt: (Position: vector) -> uuid,
  GetLinkKey: (LinkNumber: number) -> uuid,
  GetLinkMedia: (LinkNumber: number, Face: number, Parameters: list) -> list,
  GetLinkName: (LinkNumber: number) -> string,
  GetLinkNumber: () -> number,
  GetLinkNumberOfSides: (LinkNumber: number) -> number,
  GetLinkPrimitiveParams: (LinkNumber: number, Parameters: list) -> list,
  GetLinkSitFlags: (LinkNumber: number) -> number,
  GetListEntryType: (ListVariable: list, Index: number) -> number,
  GetListLength: (ListVariable: list) -> number,
  GetLocalPos: () -> vector,
  GetLocalRot: () -> quaternion,
  GetMass: () -> number,
  GetMassMKS: () -> number,
  GetMaxScaleFactor: () -> number,
  GetMemoryLimit: () -> number,
  GetMinScaleFactor: () -> number,
  GetMoonDirection: () -> vector,
  GetMoonRotation: () -> quaternion,
  GetNextEmail: (Address: string, Subject: string) -> (),
  GetNotecardLine: (NotecardName: string, LineNumber: number) -> uuid,
  GetNotecardLineSync: (NotecardName: string, LineNumber: number) -> string,
  GetNumberOfNotecardLines: (NotecardName: string) -> uuid,
  GetNumberOfPrims: () -> number,
  GetNumberOfSides: () -> number,
  GetObjectAnimationNames: () -> list,
  GetObjectDesc: () -> string,
  GetObjectDetails: (ID: uuid, Parameters: list) -> list,
  GetObjectLinkKey: (id: uuid, link_no: number) -> uuid,
  GetObjectMass: (ID: uuid) -> number,
  GetObjectName: () -> string,
  GetObjectPermMask: (Category: number) -> number,
  GetObjectPrimCount: (ObjectID: uuid) -> number,
  GetOmega: () -> vector,
  GetOwner: () -> uuid,
  GetOwnerKey: (ObjectID: uuid) -> uuid,
  GetParcelDetails: (Position: vector, ParcelDetails: list) -> list,
  GetParcelFlags: (Position: vector) -> number,
  GetParcelMaxPrims: (Position: vector, SimWide: boolean | number) -> number,
  GetParcelMusicURL: () -> string,
  GetParcelPrimCount: (Position: vector, Category: number, SimWide: boolean | number) -> number,
  GetParcelPrimOwners: (Position: vector) -> list,
  GetPermissions: () -> number,
  GetPermissionsKey: () -> uuid,
  GetPhysicsMaterial: () -> list,
  GetPos: () -> vector,
  GetPrimMediaParams: (Face: number, Parameters: list) -> list,
  GetPrimitiveParams: (Parameters: list) -> list,
  GetRegionAgentCount: () -> number,
  GetRegionCorner: () -> vector,
  GetRegionDayLength: () -> number,
  GetRegionDayOffset: () -> number,
  GetRegionFPS: () -> number,
  GetRegionFlags: () -> number,
  GetRegionMoonDirection: () -> vector,
  GetRegionMoonRotation: () -> quaternion,
  GetRegionName: () -> string,
  GetRegionSunDirection: () -> vector,
  GetRegionSunRotation: () -> quaternion,
  GetRegionTimeDilation: () -> number,
  GetRegionTimeOfDay: () -> number,
  GetRenderMaterial: (Face: number) -> string,
  GetRootPosition: () -> vector,
  GetRootRotation: () -> quaternion,
  GetRot: () -> quaternion,
  GetSPMaxMemory: () -> number,
  GetScale: () -> vector,
  GetScriptName: () -> string,
  GetScriptState: (ScriptName: string) -> boolean,
  GetSimStats: (StatType: number) -> number,
  GetSimulatorHostname: () -> string,
  GetStartParameter: () -> number,
  GetStartString: () -> string,
  GetStaticPath: (Start: vector, End: vector, Radius: number, Parameters: list) -> list,
  GetStatus: (StatusFlag: number) -> boolean,
  GetSubString: (String: string, Start: number, End: number) -> string,
  GetSunDirection: () -> vector,
  GetSunRotation: () -> quaternion,
  GetTexture: (Face: number) -> string,
  GetTextureOffset: (Face: number) -> vector,
  GetTextureRot: (Face: number) -> number,
  GetTextureScale: (Face: number) -> vector,
  GetTime: () -> number,
  GetTimeOfDay: () -> number,
  GetTimestamp: () -> string,
  GetTorque: () -> vector,
  GetUnixTime: () -> number,
  GetUsedMemory: () -> number,
  GetUsername: (AvatarID: uuid) -> string,
  GetVel: () -> vector,
  GetVisualParams: (ID: uuid, Parameters: list) -> list,
  GetWallclock: () -> number,
  GiveAgentInventory: (AgentID: uuid, FolderName: string, InventoryItems: list, Options: list) -> number,
  GiveInventory: (TargetID: uuid, InventoryItem: string) -> (),
  GiveInventoryList: (TargetID: uuid, FolderName: string, InventoryItems: list) -> (),
  GiveMoney: (AvatarID: uuid, Amount: number) -> number,
  GodLikeRezObject: (InventoryItemID: uuid, Position: vector) -> (),
  Ground: (Offset: vector) -> number,
  GroundContour: (Offset: vector) -> vector,
  GroundNormal: (Offset: vector) -> vector,
  GroundRepel: (Height: number, Water: boolean | number, Tau: number) -> (),
  GroundSlope: (Offset: vector) -> vector,
  HMAC: (Key: string, Message: string, Algorithm: string) -> string,
  HTTPRequest: (URL: string, Parameters: list, Body: string) -> uuid,
  HTTPResponse: (HTTPRequestID: uuid, Status: number, Body: string) -> (),
  Hash: (value: string) -> number,
  InsertString: (TargetVariable: string, Position: number, SourceVariable: string) -> string,
  InstantMessage: (AvatarID: uuid, Text: string) -> (),
  IntegerToBase64: (Value: number) -> string,
  IsFriend: (agent_id: uuid) -> boolean,
  IsLinkGLTFMaterial: (link: number, face: number) -> boolean,
  Json2List: (JSON: string) -> list,
  JsonGetValue: (JSON: string, Specifiers: list) -> string,
  JsonSetValue: (JSON: string, Specifiers: list, Value: string) -> string,
  JsonValueType: (JSON: string, Specifiers: list) -> string,
  Key2Name: (ID: uuid) -> string,
  KeyCountKeyValue: () -> uuid,
  KeysKeyValue: (First: number, Count: number) -> uuid,
  Linear2sRGB: (color: vector) -> vector,
  LinkAdjustSoundVolume: (LinkNumber: number, Volume: number) -> (),
  LinkParticleSystem: (LinkNumber: number, Rules: list) -> (),
  LinkPlaySound: (LinkNumber: number, Sound: string, Volume: number, Flags: number) -> (),
  LinkSetSoundQueueing: (LinkNumber: number, QueueEnable: boolean | number) -> (),
  LinkSetSoundRadius: (LinkNumber: number, radius: number) -> (),
  LinkSitTarget: (LinkNumber: number, Offset: vector, Rotation: quaternion) -> (),
  LinkStopSound: (LinkNumber: number) -> (),
  LinksetDataAvailable: () -> number,
  LinksetDataCountFound: (search: string) -> number,
  LinksetDataCountKeys: () -> number,
  LinksetDataDelete: (name: string) -> number,
  LinksetDataDeleteFound: (search: string, pass: string) -> list,
  LinksetDataDeleteProtected: (name: string, pass: string) -> number,
  LinksetDataFindKeys: (search: string, start: number, count: number) -> list,
  LinksetDataListKeys: (start: number, count: number) -> list,
  LinksetDataRead: (name: string) -> string,
  LinksetDataReadProtected: (name: string, pass: string) -> string,
  LinksetDataReset: () -> (),
  LinksetDataWrite: (name: string, value: string) -> number,
  LinksetDataWriteProtected: (name: string, value: string, pass: string) -> number,
  List2CSV: (ListVariable: list) -> string,
  List2Float: (ListVariable: list, Index: number) -> number,
  List2Integer: (ListVariable: list, Index: number) -> number,
  List2Json: (JsonType: string, Values: list) -> string,
  List2Key: (ListVariable: list, Index: number) -> uuid,
  List2List: <T>(ListVariable: {T}, Start: number, End: number) -> {T},
  List2ListSlice: <T>(ListVariable: {T}, Start: number, End: number, Stride: number, slice_index: number) -> {T},
  List2ListStrided: <T>(ListVariable: {T}, Start: number, End: number, Stride: number) -> {T},
  List2Rot: (ListVariable: list, Index: number) -> quaternion,
  List2String: (ListVariable: list, Index: number) -> string,
  List2Vector: (ListVariable: list, Index: number) -> vector,
  ListFindList: (ListVariable: list, Find: list) -> number,
  ListFindListNext: (ListVariable: list, Find: list, Instance: number) -> number,
  ListFindStrided: (ListVariable: list, Find: list, Start: number, End: number, Stride: number) -> number,
  ListInsertList: <T>(Target: {T}, ListVariable: {T}, Position: number) -> {T},
  ListRandomize: <T>(ListVariable: {T}, Stride: number) -> {T},
  ListReplaceList: <T>(Target: {T}, ListVariable: {T}, Start: number, End: number) -> {T},
  ListSort: <T>(ListVariable: {T}, Stride: number, Ascending: boolean | number) -> {T},
  ListSortStrided: <T>(ListVariable: {T}, Stride: number, Sortkey: number, Ascending: boolean | number) -> {T},
  ListStatistics: (Operation: number, ListVariable: list) -> number,
  Listen: (Channel: number, SpeakersName: string, SpeakersID: uuid, Text: string) -> number,
  ListenControl: (ChannelHandle: number, Active: boolean | number) -> (),
  ListenRemove: (ChannelHandle: number) -> (),
  LoadURL: (AvatarID: uuid, Text: string, URL: string) -> (),
  Log: (Value: number) -> number,
  Log10: (Value: number) -> number,
  LookAt: (Target: vector, Strength: number, Damping: number) -> (),
  LoopSound: (Sound: string, Volume: number) -> (),
  LoopSoundMaster: (Sound: string, Volume: number) -> (),
  LoopSoundSlave: (Sound: string, Volume: number) -> (),
  MD5String: (Text: string, Nonce: number) -> string,
  MakeExplosion: @deprecated (Particles: number, Scale: number, Velocity: number, Lifetime: number, Arc: number, Texture: string, Offset: vector) -> (),
  MakeFire: @deprecated (Particles: number, Scale: number, Velocity: number, Lifetime: number, Arc: number, Texture: string, Offset: vector) -> (),
  MakeFountain: @deprecated (Particles: number, Scale: number, Velocity: number, Lifetime: number, Arc: number, Bounce: number, Texture: string, Offset: vector, Bounce_Offset: number) -> (),
  MakeSmoke: @deprecated (Particles: number, Scale: number, Velocity: number, Lifetime: number, Arc: number, Texture: string, Offset: vector) -> (),
  ManageEstateAccess: (Action: number, AvatarID: uuid) -> boolean,
  MapBeacon: (RegionName: string, Position: vector, Options: list) -> (),
  MapDestination: (RegionName: string, Position: vector, Direction: vector) -> (),
  MessageLinked: (LinkNumber: number, Number: number, Text: string | uuid, ID: string | uuid) -> (),
  MinEventDelay: (Delay: number) -> (),
  ModPow: (Value: number, Power: number, Modulus: number) -> number,
  ModifyLand: (Action: number, Area: number) -> (),
  MoveToTarget: (Target: vector, Tau: number) -> (),
  Name2Key: (Name: string) -> uuid,
  NavigateTo: (Location: vector, Options: list) -> (),
  OffsetTexture: (OffsetS: number, OffsetT: number, Face: number) -> (),
  OpenFloater: (floater_name: string, url: string, params: list) -> number,
  OpenRemoteDataChannel: @deprecated () -> (),
  Ord: (value: string, index: number) -> number,
  OverMyLand: (ID: uuid) -> boolean,
  OwnerSay: (Text: string) -> (),
  ParcelMediaCommandList: (CommandList: list) -> (),
  ParcelMediaQuery: (QueryList: list) -> list,
  ParseString2List: (Text: string, Separators: {string}, Spacers: {string}) -> {string},
  ParseStringKeepNulls: (Text: string, Separators: {string}, Spacers: {string}) -> {string},
  ParticleSystem: (Parameters: list) -> (),
  PassCollisions: (Pass: boolean | number) -> (),
  PassTouches: (Pass: boolean | number) -> (),
  PatrolPoints: (Points: list, Options: list) -> (),
  PlaySound: (Sound: string, Volume: number) -> (),
  PlaySoundSlave: (Sound: string, Volume: number) -> (),
  Pow: (Value: number, Exponent: number) -> number,
  PreloadSound: (Sound: string) -> (),
  Pursue: (TargetID: uuid, Options: list) -> (),
  PushObject: (ObjectID: uuid, Impulse: vector, AngularImpulse: vector, Local: boolean | number) -> (),
  ReadKeyValue: (Key: string) -> uuid,
  RefreshPrimURL: @deprecated () -> (),
  RegionSay: (Channel: number, Text: string) -> (),
  RegionSayTo: (TargetID: uuid, Channel: number, Text: string) -> (),
  ReleaseCamera: @deprecated (AvatarID: uuid) -> (),
  ReleaseControls: () -> (),
  ReleaseURL: (URL: string) -> (),
  RemoteDataReply: @deprecated (ChannelID: uuid, MessageID: uuid, sData: string, iData: number) -> (),
  RemoteDataSetRegion: @deprecated () -> (),
  RemoteLoadScriptPin: (ObjectID: uuid, ScriptName: string, PIN: number, Running: boolean | number, StartParameter: number) -> (),
  RemoveFromLandBanList: (AvatarID: uuid) -> (),
  RemoveFromLandPassList: (AvatarID: uuid) -> (),
  RemoveInventory: (InventoryItem: string) -> (),
  RemoveVehicleFlags: (Vehiclelags: number) -> (),
  ReplaceAgentEnvironment: (agent_id: uuid, transition: number, environment: string) -> number,
  ReplaceEnvironment: (position: vector, environment: string, track_no: number, day_length: number, day_offset: number) -> number,
  ReplaceSubString: (InitialString: string, SubString: string, NewSubString: string, Count: number) -> string,
  RequestAgentData: (AvatarID: uuid, Data: number) -> uuid,
  RequestDisplayName: (AvatarID: uuid) -> uuid,
  RequestExperiencePermissions: (AgentID: uuid, unused: string) -> (),
  RequestInventoryData: (InventoryItem: string) -> uuid,
  RequestPermissions: (AvatarID: uuid, PermissionMask: number) -> (),
  RequestSecureURL: () -> uuid,
  RequestSimulatorData: (RegionName: string, Data: number) -> uuid,
  RequestURL: () -> uuid,
  RequestUserKey: (Name: string) -> uuid,
  RequestUsername: (AvatarID: uuid) -> uuid,
  ResetAnimationOverride: (AnimationState: string) -> (),
  ResetLandBanList: () -> (),
  ResetLandPassList: () -> (),
  ResetOtherScript: (ScriptName: string) -> (),
  ResetScript: () -> (),
  ReturnObjectsByID: (ObjectIDs: list) -> number,
  ReturnObjectsByOwner: (ID: uuid, Scope: number) -> number,
  RezAtRoot: (InventoryItem: string, Position: vector, Velocity: vector, Rotation: quaternion, StartParameter: number) -> (),
  RezObject: (InventoryItem: string, Position: vector, Velocity: vector, Rotation: quaternion, StartParameter: number) -> (),
  RezObjectWithParams: (InventoryItem: string, Params: list) -> uuid,
  Rot2Angle: (Rotation: quaternion) -> number,
  Rot2Axis: (Rotation: quaternion) -> vector,
  Rot2Euler: (Rotation: quaternion) -> vector,
  Rot2Fwd: (Rotation: quaternion) -> vector,
  Rot2Left: (Rotation: quaternion) -> vector,
  Rot2Up: (Rotation: quaternion) -> vector,
  RotBetween: (Vector1: vector, Vector2: vector) -> quaternion,
  RotLookAt: (Rotation: quaternion, Strength: number, Damping: number) -> (),
  RotTarget: (Rotation: quaternion, LeeWay: number) -> number,
  RotTargetRemove: (Handle: number) -> (),
  RotateTexture: (Radians: number, Face: number) -> (),
  Round: (Value: number) -> number,
  SHA1String: (Text: string) -> string,
  SHA256String: (text: string) -> string,
  SameGroup: (ID: uuid) -> boolean,
  Say: (Channel: number, Text: string) -> (),
  ScaleByFactor: (ScalingFactor: number) -> boolean,
  ScaleTexture: (Horizontal: number, Vertical: number, Face: number) -> (),
  ScriptDanger: (Position: vector) -> boolean,
  ScriptProfiler: (State: number) -> (),
  SendRemoteData: @deprecated (ChannelID: uuid, Destination: string, Value: number, Text: string) -> uuid,
  Sensor: (Name: string, ID: uuid, Type: number, Range: number, Arc: number) -> (),
  SensorRemove: () -> (),
  SensorRepeat: (Name: string, ID: uuid, Type: number, Range: number, Arc: number, Rate: number) -> (),
  SetAgentEnvironment: (agent_id: uuid, transition: number, Settings: list) -> number,
  SetAgentRot: (rot: quaternion, flags: number) -> (),
  SetAlpha: (Opacity: number, Face: number) -> (),
  SetAngularVelocity: (AngVel: vector, Local: boolean | number) -> (),
  SetAnimationOverride: (AnimationState: string, AnimationName: string) -> (),
  SetBuoyancy: (Buoyancy: number) -> (),
  SetCameraAtOffset: (Offset: vector) -> (),
  SetCameraEyeOffset: (Offset: vector) -> (),
  SetCameraParams: (Parameters: list) -> (),
  SetClickAction: (Action: number) -> (),
  SetColor: (Color: vector, Face: number) -> (),
  SetContentType: (HTTPRequestID: uuid, ContentType: number) -> (),
  SetDamage: (Damage: number) -> (),
  SetEnvironment: (Position: vector, EnvParams: list) -> number,
  SetForce: (Force: vector, Local: boolean | number) -> (),
  SetForceAndTorque: (Force: vector, Torque: vector, Local: boolean | number) -> (),
  SetGroundTexture: (Changes: list) -> number,
  SetHoverHeight: (Height: number, Water: boolean | number, Tau: number) -> (),
  SetInventoryPermMask: (InventoryItem: string, PermissionFlag: number, PermissionMask: number) -> (),
  SetKeyframedMotion: (Keyframes: list, Options: list) -> (),
  SetLinkAlpha: (LinkNumber: number, Opacity: number, Face: number) -> (),
  SetLinkCamera: (LinkNumber: number, EyeOffset: vector, LookOffset: vector) -> (),
  SetLinkColor: (LinkNumber: number, Color: vector, Face: number) -> (),
  SetLinkGLTFOverrides: (link: number, face: number, options: list) -> (),
  SetLinkMedia: (Link: number, Face: number, Parameters: list) -> number,
  SetLinkPrimitiveParams: @deprecated (LinkNumber: number, Parameters: list) -> (),
  SetLinkPrimitiveParamsFast: (LinkNumber: number, Parameters: list) -> (),
  SetLinkRenderMaterial: (LinkNumber: number, RenderMaterial: string, Face: number) -> (),
  SetLinkSitFlags: (LinkNumber: number, Flags: number) -> (),
  SetLinkTexture: (LinkNumber: number, Texture: string, Face: number) -> (),
  SetLinkTextureAnim: (LinkNumber: number, Mode: number, Face: number, SizeX: number, SizeY: number, Start: number, Length: number, Rate: number) -> (),
  SetLocalRot: (Rotation: quaternion) -> (),
  SetObjectDesc: (Description: string) -> (),
  SetObjectName: (Name: string) -> (),
  SetObjectPermMask: (PermissionFlag: number, PermissionMask: number) -> (),
  SetParcelForSale: (ForSale: boolean | number, Options: list) -> number,
  SetParcelMusicURL: (URL: string) -> (),
  SetPayPrice: (Price: number, QuickButtons: list) -> (),
  SetPhysicsMaterial: (MaterialBits: number, GravityMultiplier: number, Restitution: number, Friction: number, Density: number) -> (),
  SetPos: (Position: vector) -> (),
  SetPrimMediaParams: (Face: number, MediaParameters: list) -> number,
  SetPrimURL: @deprecated (URL: string) -> (),
  SetPrimitiveParams: @deprecated (Parameters: list) -> (),
  SetRegionPos: (Position: vector) -> boolean,
  SetRemoteScriptAccessPin: (PIN: number) -> (),
  SetRenderMaterial: (Material: string, Face: number) -> (),
  SetRot: (Rotation: quaternion) -> (),
  SetScale: (Scale: vector) -> (),
  SetScriptState: (ScriptName: string, Running: boolean | number) -> (),
  SetSitText: (Text: string) -> (),
  SetSoundQueueing: (QueueEnable: boolean | number) -> (),
  SetSoundRadius: (Radius: number) -> (),
  SetStatus: (Status: number, Value: boolean | number) -> (),
  SetText: (Text: string, Color: vector, Opacity: number) -> (),
  SetTexture: (Texture: string, Face: number) -> (),
  SetTextureAnim: (Mode: number, Face: number, SizeX: number, SizeY: number, Start: number, Length: number, Rate: number) -> (),
  SetTorque: (Torque: vector, Local: boolean | number) -> (),
  SetTouchText: (Text: string) -> (),
  SetVehicleFlags: (Flags: number) -> (),
  SetVehicleFloatParam: (ParameterName: number, ParameterValue: number) -> (),
  SetVehicleRotationParam: (ParameterName: number, ParameterValue: quaternion) -> (),
  SetVehicleType: (Type: number) -> (),
  SetVehicleVectorParam: (ParameterName: number, ParameterValue: vector) -> (),
  SetVelocity: (Velocity: vector, Local: number) -> (),
  Shout: (Channel: number, Text: string) -> (),
  SignRSA: (PrivateKey: string, Message: string, Algorithm: string) -> string,
  Sin: (Theta: number) -> number,
  SitOnLink: (AvatarID: uuid, LinkID: number) -> number,
  SitTarget: (Offset: vector, Rotation: quaternion) -> (),
  Sleep: (Time: number) -> (),
  Sound: @deprecated (Sound: string, Volume: number, Queue: boolean | number, Loop: boolean | number) -> (),
  SoundPreload: @deprecated (Sound: string) -> (),
  Sqrt: (Value: number) -> number,
  StartAnimation: (Animation: string) -> (),
  StartObjectAnimation: (Animation: string) -> (),
  StopAnimation: (Animation: string) -> (),
  StopHover: () -> (),
  StopLookAt: () -> (),
  StopMoveToTarget: () -> (),
  StopObjectAnimation: (Animation: string) -> (),
  StopSound: () -> (),
  StringLength: (Text: string) -> number,
  StringToBase64: (Text: string) -> string,
  StringTrim: (Text: string, TrimType: number) -> string,
  SubStringIndex: (Text: string, Sequence: string) -> number,
  TakeCamera: @deprecated (AvatarID: uuid) -> (),
  TakeControls: (Controls: number, Accept: boolean | number, PassOn: boolean | number) -> (),
  Tan: (Theta: number) -> number,
  Target: (Position: vector, Range: number) -> number,
  TargetOmega: (Axis: vector, SpinRate: number, Gain: number) -> (),
  TargetRemove: (Target: number) -> (),
  TargetedEmail: (Target: number, Subject: string, Text: string) -> (),
  TeleportAgent: (AvatarID: uuid, LandmarkName: string, Position: vector, LookAtPoint: vector) -> (),
  TeleportAgentGlobalCoords: (AvatarID: uuid, GlobalPosition: vector, RegionPosition: vector, LookAtPoint: vector) -> (),
  TeleportAgentHome: (AvatarID: uuid) -> (),
  TextBox: (AvatarID: uuid, Text: string, Channel: number) -> (),
  ToLower: (Text: string) -> string,
  ToUpper: (Text: string) -> string,
  TransferLindenDollars: (AvatarID: uuid, Amount: number) -> uuid,
  TransferOwnership: (AgentID: uuid, Flags: number, Params: list) -> number,
  TriggerSound: (Sound: string, Volume: number) -> (),
  TriggerSoundLimited: (Sound: string, Volume: number, TNE: vector, BSW: vector) -> (),
  UnSit: (AvatarID: uuid) -> (),
  UnescapeURL: (URL: string) -> string,
  UpdateCharacter: (Options: list) -> (),
  UpdateKeyValue: (Key: string, Value: string, Checked: boolean | number, OriginalValue: string) -> uuid,
  VecDist: (Location1: vector, Location2: vector) -> number,
  VecMag: (Vector: vector) -> number,
  VecNorm: (Vector: vector) -> vector,
  VerifyRSA: (PublicKey: string, Message: string, Signature: string, Algorithm: string) -> boolean,
  VolumeDetect: (DetectEnabled: boolean | number) -> (),
  WanderWithin: (Origin: vector, Area: vector, Options: list) -> (),
  Water: (Offset: vector) -> number,
  Whisper: (Channel: number, Text: string) -> (),
  Wind: (Offset: vector) -> vector,
  WorldPosToHUD: (world_pos: vector) -> vector,
  XorBase64: (Text1: string, Text2: string) -> string,
  XorBase64Strings: @deprecated (Text1: string, Text2: string) -> string,
  XorBase64StringsCorrect: @deprecated (Text1: string, Text2: string) -> string,
  sRGB2Linear: (srgb: vector) -> vector,
}


---------------------------
-- Global Table: llcompat
---------------------------

declare llcompat: {
  Abs: @deprecated (Value: number) -> number,
  Acos: @deprecated (Value: number) -> number,
  AddToLandBanList: @deprecated (ID: uuid, Hours: number) -> (),
  AddToLandPassList: @deprecated (ID: uuid, Hours: number) -> (),
  AdjustDamage: @deprecated (Number: number, Damage: number) -> (),
  AdjustSoundVolume: @deprecated (Volume: number) -> (),
  AgentInExperience: @deprecated (AgentID: uuid) -> number,
  AllowInventoryDrop: @deprecated (Flag: boolean | number) -> (),
  AngleBetween: @deprecated (Rot1: quaternion, Rot2: quaternion) -> number,
  ApplyImpulse: @deprecated (Force: vector, Local: boolean | number) -> (),
  ApplyRotationalImpulse: @deprecated (Force: vector, Local: boolean | number) -> (),
  Asin: @deprecated (Value: number) -> number,
  Atan2: @deprecated (y: number, x: number) -> number,
  AttachToAvatar: @deprecated (AttachmentPoint: number) -> (),
  AttachToAvatarTemp: @deprecated (AttachPoint: number) -> (),
  AvatarOnLinkSitTarget: @deprecated (LinkNumber: number) -> uuid,
  AvatarOnSitTarget: @deprecated () -> uuid,
  Axes2Rot: @deprecated (Forward: vector, Left: vector, Up: vector) -> quaternion,
  AxisAngle2Rot: @deprecated (Axis: vector, Angle: number) -> quaternion,
  Base64ToInteger: @deprecated (Text: string) -> number,
  Base64ToString: @deprecated (Text: string) -> string,
  BreakAllLinks: @deprecated () -> (),
  BreakLink: @deprecated (LinkNumber: number) -> (),
  CSV2List: @deprecated (Text: string) -> {string},
  CastRay: @deprecated (Start: vector, End: vector, Options: list) -> list,
  Ceil: @deprecated (Value: number) -> number,
  Char: @deprecated (value: number) -> string,
  ClearCameraParams: @deprecated () -> (),
  ClearLinkMedia: @deprecated (Link: number, Face: number) -> number,
  ClearPrimMedia: @deprecated (Face: number) -> number,
  CloseRemoteDataChannel: @deprecated (ChannelID: uuid) -> (),
  Cloud: @deprecated (Offset: vector) -> number,
  CollisionFilter: @deprecated (ObjectName: string, ObjectID: uuid, Accept: boolean | number) -> (),
  CollisionSound: @deprecated (ImpactSound: string, ImpactVolume: number) -> (),
  CollisionSprite: @deprecated (ImpactSprite: string) -> (),
  ComputeHash: @deprecated (Message: string, Algorithm: string) -> string,
  Cos: @deprecated (Theta: number) -> number,
  CreateCharacter: @deprecated (Options: list) -> (),
  CreateKeyValue: @deprecated (Key: string, Value: string) -> uuid,
  CreateLink: @deprecated (TargetPrim: uuid, Parent: boolean | number) -> (),
  Damage: @deprecated (target: uuid, damage: number, type: number) -> (),
  DataSizeKeyValue: @deprecated () -> uuid,
  DeleteCharacter: @deprecated () -> (),
  DeleteKeyValue: @deprecated (Key: string) -> uuid,
  DeleteSubList: @deprecated <T>(Source: {T}, Start: number, End: number) -> {T},
  DeleteSubString: @deprecated (Source: string, Start: number, End: number) -> string,
  DerezObject: @deprecated (ID: uuid, flags: number) -> number,
  DetachFromAvatar: @deprecated () -> (),
  DetectedDamage: @deprecated (Number: number) -> list,
  DetectedGrab: @deprecated (Number: number) -> vector,
  DetectedGroup: @deprecated (Number: number) -> number,
  DetectedKey: @deprecated (Number: number) -> uuid,
  DetectedLinkNumber: @deprecated (Number: number) -> number,
  DetectedName: @deprecated (Number: number) -> string,
  DetectedOwner: @deprecated (Number: number) -> uuid,
  DetectedPos: @deprecated (Number: number) -> vector,
  DetectedRezzer: @deprecated (Number: number) -> uuid,
  DetectedRot: @deprecated (Number: number) -> quaternion,
  DetectedTouchBinormal: @deprecated (Index: number) -> vector,
  DetectedTouchFace: @deprecated (Index: number) -> number,
  DetectedTouchNormal: @deprecated (Index: number) -> vector,
  DetectedTouchPos: @deprecated (Index: number) -> vector,
  DetectedTouchST: @deprecated (Index: number) -> vector,
  DetectedTouchUV: @deprecated (Index: number) -> vector,
  DetectedType: @deprecated (Number: number) -> number,
  DetectedVel: @deprecated (Number: number) -> vector,
  Dialog: @deprecated (AvatarID: uuid, Text: string, Buttons: list, Channel: number) -> (),
  Die: @deprecated () -> (),
  DumpList2String: @deprecated (Source: list, Separator: string) -> string,
  EdgeOfWorld: @deprecated (Position: vector, Direction: vector) -> number,
  EjectFromLand: @deprecated (AvatarID: uuid) -> (),
  Email: @deprecated (Address: string, Subject: string, Text: string) -> (),
  EscapeURL: @deprecated (URL: string) -> string,
  Euler2Rot: @deprecated (Vector: vector) -> quaternion,
  Evade: @deprecated (TargetID: uuid, Options: list) -> (),
  ExecCharacterCmd: @deprecated (Command: number, Options: list) -> (),
  Fabs: @deprecated (Value: number) -> number,
  FindNotecardTextCount: @deprecated (NotecardName: string, Pattern: string, Options: list) -> uuid,
  FindNotecardTextSync: @deprecated (NotecardName: string, Pattern: string, StartMatch: number, Count: number, Options: list) -> list,
  FleeFrom: @deprecated (Source: vector, Distance: number, Options: list) -> (),
  Floor: @deprecated (Value: number) -> number,
  ForceMouselook: @deprecated (Enable: boolean | number) -> (),
  Frand: @deprecated (Magnitude: number) -> number,
  GenerateKey: @deprecated () -> uuid,
  GetAccel: @deprecated () -> vector,
  GetAgentInfo: @deprecated (AvatarID: uuid) -> number,
  GetAgentLanguage: @deprecated (AvatarID: uuid) -> string,
  GetAgentList: @deprecated (Scope: number, Options: list) -> list,
  GetAgentSize: @deprecated (AvatarID: uuid) -> vector,
  GetAlpha: @deprecated (Face: number) -> number,
  GetAndResetTime: @deprecated () -> number,
  GetAnimation: @deprecated (AvatarID: uuid) -> string,
  GetAnimationList: @deprecated (AvatarID: uuid) -> list,
  GetAnimationOverride: @deprecated (AnimationState: string) -> string,
  GetAttached: @deprecated () -> number,
  GetAttachedList: @deprecated (ID: uuid) -> {uuid},
  GetAttachedListFiltered: @deprecated (AgentID: uuid, Options: list) -> {uuid},
  GetBoundingBox: @deprecated (ID: uuid) -> {vector},
  GetCameraAspect: @deprecated () -> number,
  GetCameraFOV: @deprecated () -> number,
  GetCameraPos: @deprecated () -> vector,
  GetCameraRot: @deprecated () -> quaternion,
  GetCenterOfMass: @deprecated () -> vector,
  GetClosestNavPoint: @deprecated (Point: vector, Options: list) -> list,
  GetColor: @deprecated (Face: number) -> vector,
  GetCreator: @deprecated () -> uuid,
  GetDate: @deprecated () -> string,
  GetDayLength: @deprecated () -> number,
  GetDayOffset: @deprecated () -> number,
  GetDisplayName: @deprecated (AvatarID: uuid) -> string,
  GetEnergy: @deprecated () -> number,
  GetEnv: @deprecated (DataRequest: string) -> string,
  GetEnvironment: @deprecated (Position: vector, EnvParams: list) -> list,
  GetExperienceDetails: @deprecated (ExperienceID: uuid) -> list,
  GetExperienceErrorMessage: @deprecated (Error: number) -> string,
  GetForce: @deprecated () -> vector,
  GetFreeMemory: @deprecated () -> number,
  GetFreeURLs: @deprecated () -> number,
  GetGMTclock: @deprecated () -> number,
  GetGeometricCenter: @deprecated () -> vector,
  GetHTTPHeader: @deprecated (HTTPRequestID: uuid, Header: string) -> string,
  GetHealth: @deprecated (ID: uuid) -> number,
  GetInventoryAcquireTime: @deprecated (InventoryItem: string) -> string,
  GetInventoryCreator: @deprecated (InventoryItem: string) -> uuid,
  GetInventoryDesc: @deprecated (InventoryItem: string) -> string,
  GetInventoryKey: @deprecated (InventoryItem: string) -> uuid,
  GetInventoryName: @deprecated (InventoryType: number, Index: number) -> string,
  GetInventoryNumber: @deprecated (InventoryType: number) -> number,
  GetInventoryPermMask: @deprecated (InventoryItem: string, BitMask: number) -> number,
  GetInventoryType: @deprecated (InventoryItem: string) -> number,
  GetKey: @deprecated () -> uuid,
  GetLandOwnerAt: @deprecated (Position: vector) -> uuid,
  GetLinkKey: @deprecated (LinkNumber: number) -> uuid,
  GetLinkMedia: @deprecated (LinkNumber: number, Face: number, Parameters: list) -> list,
  GetLinkName: @deprecated (LinkNumber: number) -> string,
  GetLinkNumber: @deprecated () -> number,
  GetLinkNumberOfSides: @deprecated (LinkNumber: number) -> number,
  GetLinkPrimitiveParams: @deprecated (LinkNumber: number, Parameters: list) -> list,
  GetLinkSitFlags: @deprecated (LinkNumber: number) -> number,
  GetListEntryType: @deprecated (ListVariable: list, Index: number) -> number,
  GetListLength: @deprecated (ListVariable: list) -> number,
  GetLocalPos: @deprecated () -> vector,
  GetLocalRot: @deprecated () -> quaternion,
  GetMass: @deprecated () -> number,
  GetMassMKS: @deprecated () -> number,
  GetMaxScaleFactor: @deprecated () -> number,
  GetMemoryLimit: @deprecated () -> number,
  GetMinScaleFactor: @deprecated () -> number,
  GetMoonDirection: @deprecated () -> vector,
  GetMoonRotation: @deprecated () -> quaternion,
  GetNextEmail: @deprecated (Address: string, Subject: string) -> (),
  GetNotecardLine: @deprecated (NotecardName: string, LineNumber: number) -> uuid,
  GetNotecardLineSync: @deprecated (NotecardName: string, LineNumber: number) -> string,
  GetNumberOfNotecardLines: @deprecated (NotecardName: string) -> uuid,
  GetNumberOfPrims: @deprecated () -> number,
  GetNumberOfSides: @deprecated () -> number,
  GetObjectAnimationNames: @deprecated () -> list,
  GetObjectDesc: @deprecated () -> string,
  GetObjectDetails: @deprecated (ID: uuid, Parameters: list) -> list,
  GetObjectLinkKey: @deprecated (id: uuid, link_no: number) -> uuid,
  GetObjectMass: @deprecated (ID: uuid) -> number,
  GetObjectName: @deprecated () -> string,
  GetObjectPermMask: @deprecated (Category: number) -> number,
  GetObjectPrimCount: @deprecated (ObjectID: uuid) -> number,
  GetOmega: @deprecated () -> vector,
  GetOwner: @deprecated () -> uuid,
  GetOwnerKey: @deprecated (ObjectID: uuid) -> uuid,
  GetParcelDetails: @deprecated (Position: vector, ParcelDetails: list) -> list,
  GetParcelFlags: @deprecated (Position: vector) -> number,
  GetParcelMaxPrims: @deprecated (Position: vector, SimWide: boolean | number) -> number,
  GetParcelMusicURL: @deprecated () -> string,
  GetParcelPrimCount: @deprecated (Position: vector, Category: number, SimWide: boolean | number) -> number,
  GetParcelPrimOwners: @deprecated (Position: vector) -> list,
  GetPermissions: @deprecated () -> number,
  GetPermissionsKey: @deprecated () -> uuid,
  GetPhysicsMaterial: @deprecated () -> list,
  GetPos: @deprecated () -> vector,
  GetPrimMediaParams: @deprecated (Face: number, Parameters: list) -> list,
  GetPrimitiveParams: @deprecated (Parameters: list) -> list,
  GetRegionAgentCount: @deprecated () -> number,
  GetRegionCorner: @deprecated () -> vector,
  GetRegionDayLength: @deprecated () -> number,
  GetRegionDayOffset: @deprecated () -> number,
  GetRegionFPS: @deprecated () -> number,
  GetRegionFlags: @deprecated () -> number,
  GetRegionMoonDirection: @deprecated () -> vector,
  GetRegionMoonRotation: @deprecated () -> quaternion,
  GetRegionName: @deprecated () -> string,
  GetRegionSunDirection: @deprecated () -> vector,
  GetRegionSunRotation: @deprecated () -> quaternion,
  GetRegionTimeDilation: @deprecated () -> number,
  GetRegionTimeOfDay: @deprecated () -> number,
  GetRenderMaterial: @deprecated (Face: number) -> string,
  GetRootPosition: @deprecated () -> vector,
  GetRootRotation: @deprecated () -> quaternion,
  GetRot: @deprecated () -> quaternion,
  GetSPMaxMemory: @deprecated () -> number,
  GetScale: @deprecated () -> vector,
  GetScriptName: @deprecated () -> string,
  GetScriptState: @deprecated (ScriptName: string) -> number,
  GetSimStats: @deprecated (StatType: number) -> number,
  GetSimulatorHostname: @deprecated () -> string,
  GetStartParameter: @deprecated () -> number,
  GetStartString: @deprecated () -> string,
  GetStaticPath: @deprecated (Start: vector, End: vector, Radius: number, Parameters: list) -> list,
  GetStatus: @deprecated (StatusFlag: number) -> number,
  GetSubString: @deprecated (String: string, Start: number, End: number) -> string,
  GetSunDirection: @deprecated () -> vector,
  GetSunRotation: @deprecated () -> quaternion,
  GetTexture: @deprecated (Face: number) -> string,
  GetTextureOffset: @deprecated (Face: number) -> vector,
  GetTextureRot: @deprecated (Face: number) -> number,
  GetTextureScale: @deprecated (Face: number) -> vector,
  GetTime: @deprecated () -> number,
  GetTimeOfDay: @deprecated () -> number,
  GetTimestamp: @deprecated () -> string,
  GetTorque: @deprecated () -> vector,
  GetUnixTime: @deprecated () -> number,
  GetUsedMemory: @deprecated () -> number,
  GetUsername: @deprecated (AvatarID: uuid) -> string,
  GetVel: @deprecated () -> vector,
  GetVisualParams: @deprecated (ID: uuid, Parameters: list) -> list,
  GetWallclock: @deprecated () -> number,
  GiveAgentInventory: @deprecated (AgentID: uuid, FolderName: string, InventoryItems: list, Options: list) -> number,
  GiveInventory: @deprecated (TargetID: uuid, InventoryItem: string) -> (),
  GiveInventoryList: @deprecated (TargetID: uuid, FolderName: string, InventoryItems: list) -> (),
  GiveMoney: @deprecated (AvatarID: uuid, Amount: number) -> number,
  GodLikeRezObject: @deprecated (InventoryItemID: uuid, Position: vector) -> (),
  Ground: @deprecated (Offset: vector) -> number,
  GroundContour: @deprecated (Offset: vector) -> vector,
  GroundNormal: @deprecated (Offset: vector) -> vector,
  GroundRepel: @deprecated (Height: number, Water: boolean | number, Tau: number) -> (),
  GroundSlope: @deprecated (Offset: vector) -> vector,
  HMAC: @deprecated (Key: string, Message: string, Algorithm: string) -> string,
  HTTPRequest: @deprecated (URL: string, Parameters: list, Body: string) -> uuid,
  HTTPResponse: @deprecated (HTTPRequestID: uuid, Status: number, Body: string) -> (),
  Hash: @deprecated (value: string) -> number,
  InsertString: @deprecated (TargetVariable: string, Position: number, SourceVariable: string) -> string,
  InstantMessage: @deprecated (AvatarID: uuid, Text: string) -> (),
  IntegerToBase64: @deprecated (Value: number) -> string,
  IsFriend: @deprecated (agent_id: uuid) -> number,
  IsLinkGLTFMaterial: @deprecated (link: number, face: number) -> number,
  Json2List: @deprecated (JSON: string) -> list,
  JsonGetValue: @deprecated (JSON: string, Specifiers: list) -> string,
  JsonSetValue: @deprecated (JSON: string, Specifiers: list, Value: string) -> string,
  JsonValueType: @deprecated (JSON: string, Specifiers: list) -> string,
  Key2Name: @deprecated (ID: uuid) -> string,
  KeyCountKeyValue: @deprecated () -> uuid,
  KeysKeyValue: @deprecated (First: number, Count: number) -> uuid,
  Linear2sRGB: @deprecated (color: vector) -> vector,
  LinkAdjustSoundVolume: @deprecated (LinkNumber: number, Volume: number) -> (),
  LinkParticleSystem: @deprecated (LinkNumber: number, Rules: list) -> (),
  LinkPlaySound: @deprecated (LinkNumber: number, Sound: string, Volume: number, Flags: number) -> (),
  LinkSetSoundQueueing: @deprecated (LinkNumber: number, QueueEnable: boolean | number) -> (),
  LinkSetSoundRadius: @deprecated (LinkNumber: number, radius: number) -> (),
  LinkSitTarget: @deprecated (LinkNumber: number, Offset: vector, Rotation: quaternion) -> (),
  LinkStopSound: @deprecated (LinkNumber: number) -> (),
  LinksetDataAvailable: @deprecated () -> number,
  LinksetDataCountFound: @deprecated (search: string) -> number,
  LinksetDataCountKeys: @deprecated () -> number,
  LinksetDataDelete: @deprecated (name: string) -> number,
  LinksetDataDeleteFound: @deprecated (search: string, pass: string) -> list,
  LinksetDataDeleteProtected: @deprecated (name: string, pass: string) -> number,
  LinksetDataFindKeys: @deprecated (search: string, start: number, count: number) -> list,
  LinksetDataListKeys: @deprecated (start: number, count: number) -> list,
  LinksetDataRead: @deprecated (name: string) -> string,
  LinksetDataReadProtected: @deprecated (name: string, pass: string) -> string,
  LinksetDataReset: @deprecated () -> (),
  LinksetDataWrite: @deprecated (name: string, value: string) -> number,
  LinksetDataWriteProtected: @deprecated (name: string, value: string, pass: string) -> number,
  List2CSV: @deprecated (ListVariable: list) -> string,
  List2Float: @deprecated (ListVariable: list, Index: number) -> number,
  List2Integer: @deprecated (ListVariable: list, Index: number) -> number,
  List2Json: @deprecated (JsonType: string, Values: list) -> string,
  List2Key: @deprecated (ListVariable: list, Index: number) -> uuid,
  List2List: @deprecated <T>(ListVariable: {T}, Start: number, End: number) -> {T},
  List2ListSlice: @deprecated <T>(ListVariable: {T}, Start: number, End: number, Stride: number, slice_index: number) -> {T},
  List2ListStrided: @deprecated <T>(ListVariable: {T}, Start: number, End: number, Stride: number) -> {T},
  List2Rot: @deprecated (ListVariable: list, Index: number) -> quaternion,
  List2String: @deprecated (ListVariable: list, Index: number) -> string,
  List2Vector: @deprecated (ListVariable: list, Index: number) -> vector,
  ListFindList: @deprecated (ListVariable: list, Find: list) -> number,
  ListFindListNext: @deprecated (ListVariable: list, Find: list, Instance: number) -> number,
  ListFindStrided: @deprecated (ListVariable: list, Find: list, Start: number, End: number, Stride: number) -> number,
  ListInsertList: @deprecated <T>(Target: {T}, ListVariable: {T}, Position: number) -> {T},
  ListRandomize: @deprecated <T>(ListVariable: {T}, Stride: number) -> {T},
  ListReplaceList: @deprecated <T>(Target: {T}, ListVariable: {T}, Start: number, End: number) -> {T},
  ListSort: @deprecated <T>(ListVariable: {T}, Stride: number, Ascending: boolean | number) -> {T},
  ListSortStrided: @deprecated <T>(ListVariable: {T}, Stride: number, Sortkey: number, Ascending: boolean | number) -> {T},
  ListStatistics: @deprecated (Operation: number, ListVariable: list) -> number,
  Listen: @deprecated (Channel: number, SpeakersName: string, SpeakersID: uuid, Text: string) -> number,
  ListenControl: @deprecated (ChannelHandle: number, Active: boolean | number) -> (),
  ListenRemove: @deprecated (ChannelHandle: number) -> (),
  LoadURL: @deprecated (AvatarID: uuid, Text: string, URL: string) -> (),
  Log: @deprecated (Value: number) -> number,
  Log10: @deprecated (Value: number) -> number,
  LookAt: @deprecated (Target: vector, Strength: number, Damping: number) -> (),
  LoopSound: @deprecated (Sound: string, Volume: number) -> (),
  LoopSoundMaster: @deprecated (Sound: string, Volume: number) -> (),
  LoopSoundSlave: @deprecated (Sound: string, Volume: number) -> (),
  MD5String: @deprecated (Text: string, Nonce: number) -> string,
  MakeExplosion: @deprecated (Particles: number, Scale: number, Velocity: number, Lifetime: number, Arc: number, Texture: string, Offset: vector) -> (),
  MakeFire: @deprecated (Particles: number, Scale: number, Velocity: number, Lifetime: number, Arc: number, Texture: string, Offset: vector) -> (),
  MakeFountain: @deprecated (Particles: number, Scale: number, Velocity: number, Lifetime: number, Arc: number, Bounce: number, Texture: string, Offset: vector, Bounce_Offset: number) -> (),
  MakeSmoke: @deprecated (Particles: number, Scale: number, Velocity: number, Lifetime: number, Arc: number, Texture: string, Offset: vector) -> (),
  ManageEstateAccess: @deprecated (Action: number, AvatarID: uuid) -> number,
  MapBeacon: @deprecated (RegionName: string, Position: vector, Options: list) -> (),
  MapDestination: @deprecated (RegionName: string, Position: vector, Direction: vector) -> (),
  MessageLinked: @deprecated (LinkNumber: number, Number: number, Text: string | uuid, ID: string | uuid) -> (),
  MinEventDelay: @deprecated (Delay: number) -> (),
  ModPow: @deprecated (Value: number, Power: number, Modulus: number) -> number,
  ModifyLand: @deprecated (Action: number, Area: number) -> (),
  MoveToTarget: @deprecated (Target: vector, Tau: number) -> (),
  Name2Key: @deprecated (Name: string) -> uuid,
  NavigateTo: @deprecated (Location: vector, Options: list) -> (),
  OffsetTexture: @deprecated (OffsetS: number, OffsetT: number, Face: number) -> (),
  OpenFloater: @deprecated (floater_name: string, url: string, params: list) -> number,
  OpenRemoteDataChannel: @deprecated () -> (),
  Ord: @deprecated (value: string, index: number) -> number,
  OverMyLand: @deprecated (ID: uuid) -> number,
  OwnerSay: @deprecated (Text: string) -> (),
  ParcelMediaCommandList: @deprecated (CommandList: list) -> (),
  ParcelMediaQuery: @deprecated (QueryList: list) -> list,
  ParseString2List: @deprecated (Text: string, Separators: {string}, Spacers: {string}) -> {string},
  ParseStringKeepNulls: @deprecated (Text: string, Separators: {string}, Spacers: {string}) -> {string},
  ParticleSystem: @deprecated (Parameters: list) -> (),
  PassCollisions: @deprecated (Pass: boolean | number) -> (),
  PassTouches: @deprecated (Pass: boolean | number) -> (),
  PatrolPoints: @deprecated (Points: list, Options: list) -> (),
  PlaySound: @deprecated (Sound: string, Volume: number) -> (),
  PlaySoundSlave: @deprecated (Sound: string, Volume: number) -> (),
  Pow: @deprecated (Value: number, Exponent: number) -> number,
  PreloadSound: @deprecated (Sound: string) -> (),
  Pursue: @deprecated (TargetID: uuid, Options: list) -> (),
  PushObject: @deprecated (ObjectID: uuid, Impulse: vector, AngularImpulse: vector, Local: boolean | number) -> (),
  ReadKeyValue: @deprecated (Key: string) -> uuid,
  RefreshPrimURL: @deprecated () -> (),
  RegionSay: @deprecated (Channel: number, Text: string) -> (),
  RegionSayTo: @deprecated (TargetID: uuid, Channel: number, Text: string) -> (),
  ReleaseCamera: @deprecated (AvatarID: uuid) -> (),
  ReleaseControls: @deprecated () -> (),
  ReleaseURL: @deprecated (URL: string) -> (),
  RemoteDataReply: @deprecated (ChannelID: uuid, MessageID: uuid, sData: string, iData: number) -> (),
  RemoteDataSetRegion: @deprecated () -> (),
  RemoteLoadScriptPin: @deprecated (ObjectID: uuid, ScriptName: string, PIN: number, Running: boolean | number, StartParameter: number) -> (),
  RemoveFromLandBanList: @deprecated (AvatarID: uuid) -> (),
  RemoveFromLandPassList: @deprecated (AvatarID: uuid) -> (),
  RemoveInventory: @deprecated (InventoryItem: string) -> (),
  RemoveVehicleFlags: @deprecated (Vehiclelags: number) -> (),
  ReplaceAgentEnvironment: @deprecated (agent_id: uuid, transition: number, environment: string) -> number,
  ReplaceEnvironment: @deprecated (position: vector, environment: string, track_no: number, day_length: number, day_offset: number) -> number,
  ReplaceSubString: @deprecated (InitialString: string, SubString: string, NewSubString: string, Count: number) -> string,
  RequestAgentData: @deprecated (AvatarID: uuid, Data: number) -> uuid,
  RequestDisplayName: @deprecated (AvatarID: uuid) -> uuid,
  RequestExperiencePermissions: @deprecated (AgentID: uuid, unused: string) -> (),
  RequestInventoryData: @deprecated (InventoryItem: string) -> uuid,
  RequestPermissions: @deprecated (AvatarID: uuid, PermissionMask: number) -> (),
  RequestSecureURL: @deprecated () -> uuid,
  RequestSimulatorData: @deprecated (RegionName: string, Data: number) -> uuid,
  RequestURL: @deprecated () -> uuid,
  RequestUserKey: @deprecated (Name: string) -> uuid,
  RequestUsername: @deprecated (AvatarID: uuid) -> uuid,
  ResetAnimationOverride: @deprecated (AnimationState: string) -> (),
  ResetLandBanList: @deprecated () -> (),
  ResetLandPassList: @deprecated () -> (),
  ResetOtherScript: @deprecated (ScriptName: string) -> (),
  ResetScript: @deprecated () -> (),
  ResetTime: @deprecated () -> (),
  ReturnObjectsByID: @deprecated (ObjectIDs: list) -> number,
  ReturnObjectsByOwner: @deprecated (ID: uuid, Scope: number) -> number,
  RezAtRoot: @deprecated (InventoryItem: string, Position: vector, Velocity: vector, Rotation: quaternion, StartParameter: number) -> (),
  RezObject: @deprecated (InventoryItem: string, Position: vector, Velocity: vector, Rotation: quaternion, StartParameter: number) -> (),
  RezObjectWithParams: @deprecated (InventoryItem: string, Params: list) -> uuid,
  Rot2Angle: @deprecated (Rotation: quaternion) -> number,
  Rot2Axis: @deprecated (Rotation: quaternion) -> vector,
  Rot2Euler: @deprecated (Rotation: quaternion) -> vector,
  Rot2Fwd: @deprecated (Rotation: quaternion) -> vector,
  Rot2Left: @deprecated (Rotation: quaternion) -> vector,
  Rot2Up: @deprecated (Rotation: quaternion) -> vector,
  RotBetween: @deprecated (Vector1: vector, Vector2: vector) -> quaternion,
  RotLookAt: @deprecated (Rotation: quaternion, Strength: number, Damping: number) -> (),
  RotTarget: @deprecated (Rotation: quaternion, LeeWay: number) -> number,
  RotTargetRemove: @deprecated (Handle: number) -> (),
  RotateTexture: @deprecated (Radians: number, Face: number) -> (),
  Round: @deprecated (Value: number) -> number,
  SHA1String: @deprecated (Text: string) -> string,
  SHA256String: @deprecated (text: string) -> string,
  SameGroup: @deprecated (ID: uuid) -> number,
  Say: @deprecated (Channel: number, Text: string) -> (),
  ScaleByFactor: @deprecated (ScalingFactor: number) -> number,
  ScaleTexture: @deprecated (Horizontal: number, Vertical: number, Face: number) -> (),
  ScriptDanger: @deprecated (Position: vector) -> number,
  ScriptProfiler: @deprecated (State: number) -> (),
  SendRemoteData: @deprecated (ChannelID: uuid, Destination: string, Value: number, Text: string) -> uuid,
  Sensor: @deprecated (Name: string, ID: uuid, Type: number, Range: number, Arc: number) -> (),
  SensorRemove: @deprecated () -> (),
  SensorRepeat: @deprecated (Name: string, ID: uuid, Type: number, Range: number, Arc: number, Rate: number) -> (),
  SetAgentEnvironment: @deprecated (agent_id: uuid, transition: number, Settings: list) -> number,
  SetAgentRot: @deprecated (rot: quaternion, flags: number) -> (),
  SetAlpha: @deprecated (Opacity: number, Face: number) -> (),
  SetAngularVelocity: @deprecated (AngVel: vector, Local: boolean | number) -> (),
  SetAnimationOverride: @deprecated (AnimationState: string, AnimationName: string) -> (),
  SetBuoyancy: @deprecated (Buoyancy: number) -> (),
  SetCameraAtOffset: @deprecated (Offset: vector) -> (),
  SetCameraEyeOffset: @deprecated (Offset: vector) -> (),
  SetCameraParams: @deprecated (Parameters: list) -> (),
  SetClickAction: @deprecated (Action: number) -> (),
  SetColor: @deprecated (Color: vector, Face: number) -> (),
  SetContentType: @deprecated (HTTPRequestID: uuid, ContentType: number) -> (),
  SetDamage: @deprecated (Damage: number) -> (),
  SetEnvironment: @deprecated (Position: vector, EnvParams: list) -> number,
  SetForce: @deprecated (Force: vector, Local: boolean | number) -> (),
  SetForceAndTorque: @deprecated (Force: vector, Torque: vector, Local: boolean | number) -> (),
  SetGroundTexture: @deprecated (Changes: list) -> number,
  SetHoverHeight: @deprecated (Height: number, Water: boolean | number, Tau: number) -> (),
  SetInventoryPermMask: @deprecated (InventoryItem: string, PermissionFlag: number, PermissionMask: number) -> (),
  SetKeyframedMotion: @deprecated (Keyframes: list, Options: list) -> (),
  SetLinkAlpha: @deprecated (LinkNumber: number, Opacity: number, Face: number) -> (),
  SetLinkCamera: @deprecated (LinkNumber: number, EyeOffset: vector, LookOffset: vector) -> (),
  SetLinkColor: @deprecated (LinkNumber: number, Color: vector, Face: number) -> (),
  SetLinkGLTFOverrides: @deprecated (link: number, face: number, options: list) -> (),
  SetLinkMedia: @deprecated (Link: number, Face: number, Parameters: list) -> number,
  SetLinkPrimitiveParams: @deprecated (LinkNumber: number, Parameters: list) -> (),
  SetLinkPrimitiveParamsFast: @deprecated (LinkNumber: number, Parameters: list) -> (),
  SetLinkRenderMaterial: @deprecated (LinkNumber: number, RenderMaterial: string, Face: number) -> (),
  SetLinkSitFlags: @deprecated (LinkNumber: number, Flags: number) -> (),
  SetLinkTexture: @deprecated (LinkNumber: number, Texture: string, Face: number) -> (),
  SetLinkTextureAnim: @deprecated (LinkNumber: number, Mode: number, Face: number, SizeX: number, SizeY: number, Start: number, Length: number, Rate: number) -> (),
  SetLocalRot: @deprecated (Rotation: quaternion) -> (),
  SetMemoryLimit: @deprecated (Limit: number) -> number,
  SetObjectDesc: @deprecated (Description: string) -> (),
  SetObjectName: @deprecated (Name: string) -> (),
  SetObjectPermMask: @deprecated (PermissionFlag: number, PermissionMask: number) -> (),
  SetParcelForSale: @deprecated (ForSale: boolean | number, Options: list) -> number,
  SetParcelMusicURL: @deprecated (URL: string) -> (),
  SetPayPrice: @deprecated (Price: number, QuickButtons: list) -> (),
  SetPhysicsMaterial: @deprecated (MaterialBits: number, GravityMultiplier: number, Restitution: number, Friction: number, Density: number) -> (),
  SetPos: @deprecated (Position: vector) -> (),
  SetPrimMediaParams: @deprecated (Face: number, MediaParameters: list) -> number,
  SetPrimURL: @deprecated (URL: string) -> (),
  SetPrimitiveParams: @deprecated (Parameters: list) -> (),
  SetRegionPos: @deprecated (Position: vector) -> number,
  SetRemoteScriptAccessPin: @deprecated (PIN: number) -> (),
  SetRenderMaterial: @deprecated (Material: string, Face: number) -> (),
  SetRot: @deprecated (Rotation: quaternion) -> (),
  SetScale: @deprecated (Scale: vector) -> (),
  SetScriptState: @deprecated (ScriptName: string, Running: boolean | number) -> (),
  SetSitText: @deprecated (Text: string) -> (),
  SetSoundQueueing: @deprecated (QueueEnable: boolean | number) -> (),
  SetSoundRadius: @deprecated (Radius: number) -> (),
  SetStatus: @deprecated (Status: number, Value: boolean | number) -> (),
  SetText: @deprecated (Text: string, Color: vector, Opacity: number) -> (),
  SetTexture: @deprecated (Texture: string, Face: number) -> (),
  SetTextureAnim: @deprecated (Mode: number, Face: number, SizeX: number, SizeY: number, Start: number, Length: number, Rate: number) -> (),
  SetTimerEvent: @deprecated (Rate: number) -> (),
  SetTorque: @deprecated (Torque: vector, Local: boolean | number) -> (),
  SetTouchText: @deprecated (Text: string) -> (),
  SetVehicleFlags: @deprecated (Flags: number) -> (),
  SetVehicleFloatParam: @deprecated (ParameterName: number, ParameterValue: number) -> (),
  SetVehicleRotationParam: @deprecated (ParameterName: number, ParameterValue: quaternion) -> (),
  SetVehicleType: @deprecated (Type: number) -> (),
  SetVehicleVectorParam: @deprecated (ParameterName: number, ParameterValue: vector) -> (),
  SetVelocity: @deprecated (Velocity: vector, Local: number) -> (),
  Shout: @deprecated (Channel: number, Text: string) -> (),
  SignRSA: @deprecated (PrivateKey: string, Message: string, Algorithm: string) -> string,
  Sin: @deprecated (Theta: number) -> number,
  SitOnLink: @deprecated (AvatarID: uuid, LinkID: number) -> number,
  SitTarget: @deprecated (Offset: vector, Rotation: quaternion) -> (),
  Sleep: @deprecated (Time: number) -> (),
  Sound: @deprecated (Sound: string, Volume: number, Queue: boolean | number, Loop: boolean | number) -> (),
  SoundPreload: @deprecated (Sound: string) -> (),
  Sqrt: @deprecated (Value: number) -> number,
  StartAnimation: @deprecated (Animation: string) -> (),
  StartObjectAnimation: @deprecated (Animation: string) -> (),
  StopAnimation: @deprecated (Animation: string) -> (),
  StopHover: @deprecated () -> (),
  StopLookAt: @deprecated () -> (),
  StopMoveToTarget: @deprecated () -> (),
  StopObjectAnimation: @deprecated (Animation: string) -> (),
  StopSound: @deprecated () -> (),
  StringLength: @deprecated (Text: string) -> number,
  StringToBase64: @deprecated (Text: string) -> string,
  StringTrim: @deprecated (Text: string, TrimType: number) -> string,
  SubStringIndex: @deprecated (Text: string, Sequence: string) -> number,
  TakeCamera: @deprecated (AvatarID: uuid) -> (),
  TakeControls: @deprecated (Controls: number, Accept: boolean | number, PassOn: boolean | number) -> (),
  Tan: @deprecated (Theta: number) -> number,
  Target: @deprecated (Position: vector, Range: number) -> number,
  TargetOmega: @deprecated (Axis: vector, SpinRate: number, Gain: number) -> (),
  TargetRemove: @deprecated (Target: number) -> (),
  TargetedEmail: @deprecated (Target: number, Subject: string, Text: string) -> (),
  TeleportAgent: @deprecated (AvatarID: uuid, LandmarkName: string, Position: vector, LookAtPoint: vector) -> (),
  TeleportAgentGlobalCoords: @deprecated (AvatarID: uuid, GlobalPosition: vector, RegionPosition: vector, LookAtPoint: vector) -> (),
  TeleportAgentHome: @deprecated (AvatarID: uuid) -> (),
  TextBox: @deprecated (AvatarID: uuid, Text: string, Channel: number) -> (),
  ToLower: @deprecated (Text: string) -> string,
  ToUpper: @deprecated (Text: string) -> string,
  TransferLindenDollars: @deprecated (AvatarID: uuid, Amount: number) -> uuid,
  TransferOwnership: @deprecated (AgentID: uuid, Flags: number, Params: list) -> number,
  TriggerSound: @deprecated (Sound: string, Volume: number) -> (),
  TriggerSoundLimited: @deprecated (Sound: string, Volume: number, TNE: vector, BSW: vector) -> (),
  UnSit: @deprecated (AvatarID: uuid) -> (),
  UnescapeURL: @deprecated (URL: string) -> string,
  UpdateCharacter: @deprecated (Options: list) -> (),
  UpdateKeyValue: @deprecated (Key: string, Value: string, Checked: boolean | number, OriginalValue: string) -> uuid,
  VecDist: @deprecated (Location1: vector, Location2: vector) -> number,
  VecMag: @deprecated (Vector: vector) -> number,
  VecNorm: @deprecated (Vector: vector) -> vector,
  VerifyRSA: @deprecated (PublicKey: string, Message: string, Signature: string, Algorithm: string) -> number,
  VolumeDetect: @deprecated (DetectEnabled: boolean | number) -> (),
  WanderWithin: @deprecated (Origin: vector, Area: vector, Options: list) -> (),
  Water: @deprecated (Offset: vector) -> number,
  Whisper: @deprecated (Channel: number, Text: string) -> (),
  Wind: @deprecated (Offset: vector) -> vector,
  WorldPosToHUD: @deprecated (world_pos: vector) -> vector,
  XorBase64: @deprecated (Text1: string, Text2: string) -> string,
  XorBase64Strings: @deprecated (Text1: string, Text2: string) -> string,
  XorBase64StringsCorrect: @deprecated (Text1: string, Text2: string) -> string,
  sRGB2Linear: @deprecated (srgb: vector) -> vector,
}

declare ACTIVE: number
declare AGENT: number
declare AGENT_ALWAYS_RUN: number
declare AGENT_ATTACHMENTS: number
declare AGENT_AUTOMATED: number
declare AGENT_AUTOPILOT: number
declare AGENT_AWAY: number
declare AGENT_BUSY: number
declare AGENT_BY_LEGACY_NAME: number
declare AGENT_BY_USERNAME: number
declare AGENT_CROUCHING: number
declare AGENT_FLOATING_VIA_SCRIPTED_ATTACHMENT: number
declare AGENT_FLYING: number
declare AGENT_IN_AIR: number
declare AGENT_LIST_PARCEL: number
declare AGENT_LIST_PARCEL_OWNER: number
declare AGENT_LIST_REGION: number
declare AGENT_MOUSELOOK: number
declare AGENT_ON_OBJECT: number
declare AGENT_SCRIPTED: number
declare AGENT_SITTING: number
declare AGENT_TYPING: number
declare AGENT_WALKING: number
declare ALL_SIDES: number
declare ANIM_ON: number
declare ATTACH_ANY_HUD: number
declare ATTACH_AVATAR_CENTER: number
declare ATTACH_BACK: number
declare ATTACH_BELLY: number
declare ATTACH_CHEST: number
declare ATTACH_CHIN: number
declare ATTACH_FACE_JAW: number
declare ATTACH_FACE_LEAR: number
declare ATTACH_FACE_LEYE: number
declare ATTACH_FACE_REAR: number
declare ATTACH_FACE_REYE: number
declare ATTACH_FACE_TONGUE: number
declare ATTACH_GROIN: number
declare ATTACH_HEAD: number
declare ATTACH_HIND_LFOOT: number
declare ATTACH_HIND_RFOOT: number
declare ATTACH_HUD_BOTTOM: number
declare ATTACH_HUD_BOTTOM_LEFT: number
declare ATTACH_HUD_BOTTOM_RIGHT: number
declare ATTACH_HUD_CENTER_1: number
declare ATTACH_HUD_CENTER_2: number
declare ATTACH_HUD_TOP_CENTER: number
declare ATTACH_HUD_TOP_LEFT: number
declare ATTACH_HUD_TOP_RIGHT: number
declare ATTACH_LEAR: number
declare ATTACH_LEFT_PEC: number
declare ATTACH_LEYE: number
declare ATTACH_LFOOT: number
declare ATTACH_LHAND: number
declare ATTACH_LHAND_RING1: number
declare ATTACH_LHIP: number
declare ATTACH_LLARM: number
declare ATTACH_LLLEG: number
declare ATTACH_LPEC: number
declare ATTACH_LSHOULDER: number
declare ATTACH_LUARM: number
declare ATTACH_LULEG: number
declare ATTACH_LWING: number
declare ATTACH_MOUTH: number
declare ATTACH_NECK: number
declare ATTACH_NOSE: number
declare ATTACH_PELVIS: number
declare ATTACH_REAR: number
declare ATTACH_REYE: number
declare ATTACH_RFOOT: number
declare ATTACH_RHAND: number
declare ATTACH_RHAND_RING1: number
declare ATTACH_RHIP: number
declare ATTACH_RIGHT_PEC: number
declare ATTACH_RLARM: number
declare ATTACH_RLLEG: number
declare ATTACH_RPEC: number
declare ATTACH_RSHOULDER: number
declare ATTACH_RUARM: number
declare ATTACH_RULEG: number
declare ATTACH_RWING: number
declare ATTACH_TAIL_BASE: number
declare ATTACH_TAIL_TIP: number
declare AVOID_CHARACTERS: number
declare AVOID_DYNAMIC_OBSTACLES: number
declare AVOID_NONE: number
declare BEACON_MAP: number
declare CAMERA_ACTIVE: number
declare CAMERA_BEHINDNESS_ANGLE: number
declare CAMERA_BEHINDNESS_LAG: number
declare CAMERA_DISTANCE: number
declare CAMERA_FOCUS: number
declare CAMERA_FOCUS_LAG: number
declare CAMERA_FOCUS_LOCKED: number
declare CAMERA_FOCUS_OFFSET: number
declare CAMERA_FOCUS_THRESHOLD: number
declare CAMERA_PITCH: number
declare CAMERA_POSITION: number
declare CAMERA_POSITION_LAG: number
declare CAMERA_POSITION_LOCKED: number
declare CAMERA_POSITION_THRESHOLD: number
declare CHANGED_ALLOWED_DROP: number
declare CHANGED_COLOR: number
declare CHANGED_INVENTORY: number
declare CHANGED_LINK: number
declare CHANGED_MEDIA: number
declare CHANGED_OWNER: number
declare CHANGED_REGION: number
declare CHANGED_REGION_START: number
declare CHANGED_RENDER_MATERIAL: number
declare CHANGED_SCALE: number
declare CHANGED_SHAPE: number
declare CHANGED_TELEPORT: number
declare CHANGED_TEXTURE: number
declare CHARACTER_ACCOUNT_FOR_SKIPPED_FRAMES: number
declare CHARACTER_AVOIDANCE_MODE: number
declare CHARACTER_CMD_JUMP: number
declare CHARACTER_CMD_SMOOTH_STOP: number
declare CHARACTER_CMD_STOP: number
declare CHARACTER_DESIRED_SPEED: number
declare CHARACTER_DESIRED_TURN_SPEED: number
declare CHARACTER_LENGTH: number
declare CHARACTER_MAX_ACCEL: number
declare CHARACTER_MAX_DECEL: number
declare CHARACTER_MAX_SPEED: number
declare CHARACTER_MAX_TURN_RADIUS: number
declare CHARACTER_ORIENTATION: number
declare CHARACTER_RADIUS: number
declare CHARACTER_STAY_WITHIN_PARCEL: number
declare CHARACTER_TYPE: number
declare CHARACTER_TYPE_A: number
declare CHARACTER_TYPE_B: number
declare CHARACTER_TYPE_C: number
declare CHARACTER_TYPE_D: number
declare CHARACTER_TYPE_NONE: number
declare CLICK_ACTION_BUY: number
declare CLICK_ACTION_DISABLED: number
declare CLICK_ACTION_IGNORE: number
declare CLICK_ACTION_NONE: number
declare CLICK_ACTION_OPEN: number
declare CLICK_ACTION_OPEN_MEDIA: number
declare CLICK_ACTION_PAY: number
declare CLICK_ACTION_PLAY: number
declare CLICK_ACTION_SIT: number
declare CLICK_ACTION_TOUCH: number
declare CLICK_ACTION_ZOOM: number
declare COMBAT_CHANNEL: number
declare COMBAT_LOG_ID: uuid
declare CONTENT_TYPE_ATOM: number
declare CONTENT_TYPE_FORM: number
declare CONTENT_TYPE_HTML: number
declare CONTENT_TYPE_JSON: number
declare CONTENT_TYPE_LLSD: number
declare CONTENT_TYPE_RSS: number
declare CONTENT_TYPE_TEXT: number
declare CONTENT_TYPE_XHTML: number
declare CONTENT_TYPE_XML: number
declare CONTROL_BACK: number
declare CONTROL_DOWN: number
declare CONTROL_FWD: number
declare CONTROL_LBUTTON: number
declare CONTROL_LEFT: number
declare CONTROL_ML_LBUTTON: number
declare CONTROL_RIGHT: number
declare CONTROL_ROT_LEFT: number
declare CONTROL_ROT_RIGHT: number
declare CONTROL_UP: number
declare DAMAGEABLE: number
declare DAMAGE_TYPE_ACID: number
declare DAMAGE_TYPE_BLUDGEONING: number
declare DAMAGE_TYPE_COLD: number
declare DAMAGE_TYPE_ELECTRIC: number
declare DAMAGE_TYPE_EMOTIONAL: number
declare DAMAGE_TYPE_FIRE: number
declare DAMAGE_TYPE_FORCE: number
declare DAMAGE_TYPE_GENERIC: number
declare DAMAGE_TYPE_IMPACT: number
declare DAMAGE_TYPE_NECROTIC: number
declare DAMAGE_TYPE_PIERCING: number
declare DAMAGE_TYPE_POISON: number
declare DAMAGE_TYPE_PSYCHIC: number
declare DAMAGE_TYPE_RADIANT: number
declare DAMAGE_TYPE_SLASHING: number
declare DAMAGE_TYPE_SONIC: number
declare DATA_BORN: number
declare DATA_NAME: number
declare DATA_ONLINE: number
declare DATA_PAYINFO: number
declare DATA_RATING: number
declare DATA_SIM_POS: number
declare DATA_SIM_RATING: number
declare DATA_SIM_STATUS: number
declare DEBUG_CHANNEL: number
declare DEG_TO_RAD: number
declare DENSITY: number
declare DEREZ_DIE: number
declare DEREZ_MAKE_TEMP: number
declare DEREZ_TO_INVENTORY: number
declare ENVIRONMENT_DAYINFO: number
declare ENV_INVALID_AGENT: number
declare ENV_INVALID_RULE: number
declare ENV_NOT_EXPERIENCE: number
declare ENV_NO_ENVIRONMENT: number
declare ENV_NO_EXPERIENCE_LAND: number
declare ENV_NO_EXPERIENCE_PERMISSION: number
declare ENV_NO_PERMISSIONS: number
declare ENV_THROTTLE: number
declare ENV_VALIDATION_FAIL: number
declare EOF: string
declare ERR_GENERIC: number
declare ERR_MALFORMED_PARAMS: number
declare ERR_PARCEL_PERMISSIONS: number
declare ERR_RUNTIME_PERMISSIONS: number
declare ERR_THROTTLED: number
declare ESTATE_ACCESS_ALLOWED_AGENT_ADD: number
declare ESTATE_ACCESS_ALLOWED_AGENT_REMOVE: number
declare ESTATE_ACCESS_ALLOWED_GROUP_ADD: number
declare ESTATE_ACCESS_ALLOWED_GROUP_REMOVE: number
declare ESTATE_ACCESS_BANNED_AGENT_ADD: number
declare ESTATE_ACCESS_BANNED_AGENT_REMOVE: number
declare FILTER_FLAGS: number
declare FILTER_FLAG_HUDS: number
declare FILTER_INCLUDE: number
declare FORCE_DIRECT_PATH: number
declare FRICTION: number
declare GAME_CONTROL_AXIS_LEFTX: number
declare GAME_CONTROL_AXIS_LEFTY: number
declare GAME_CONTROL_AXIS_RIGHTX: number
declare GAME_CONTROL_AXIS_RIGHTY: number
declare GAME_CONTROL_AXIS_TRIGGERLEFT: number
declare GAME_CONTROL_AXIS_TRIGGERRIGHT: number
declare GAME_CONTROL_BUTTON_A: number
declare GAME_CONTROL_BUTTON_B: number
declare GAME_CONTROL_BUTTON_BACK: number
declare GAME_CONTROL_BUTTON_DPAD_DOWN: number
declare GAME_CONTROL_BUTTON_DPAD_LEFT: number
declare GAME_CONTROL_BUTTON_DPAD_RIGHT: number
declare GAME_CONTROL_BUTTON_DPAD_UP: number
declare GAME_CONTROL_BUTTON_GUIDE: number
declare GAME_CONTROL_BUTTON_LEFTSHOULDER: number
declare GAME_CONTROL_BUTTON_LEFTSTICK: number
declare GAME_CONTROL_BUTTON_MISC1: number
declare GAME_CONTROL_BUTTON_PADDLE1: number
declare GAME_CONTROL_BUTTON_PADDLE2: number
declare GAME_CONTROL_BUTTON_PADDLE3: number
declare GAME_CONTROL_BUTTON_PADDLE4: number
declare GAME_CONTROL_BUTTON_RIGHTSHOULDER: number
declare GAME_CONTROL_BUTTON_RIGHTSTICK: number
declare GAME_CONTROL_BUTTON_START: number
declare GAME_CONTROL_BUTTON_TOUCHPAD: number
declare GAME_CONTROL_BUTTON_X: number
declare GAME_CONTROL_BUTTON_Y: number
declare GCNP_RADIUS: number
declare GCNP_STATIC: number
declare GRAVITY_MULTIPLIER: number
declare HORIZONTAL: number
declare HTTP_ACCEPT: number
declare HTTP_BODY_MAXLENGTH: number
declare HTTP_BODY_TRUNCATED: number
declare HTTP_CUSTOM_HEADER: number
declare HTTP_EXTENDED_ERROR: number
declare HTTP_METHOD: number
declare HTTP_MIMETYPE: number
declare HTTP_PRAGMA_NO_CACHE: number
declare HTTP_USER_AGENT: number
declare HTTP_VERBOSE_THROTTLE: number
declare HTTP_VERIFY_CERT: number
declare IMG_USE_BAKED_AUX1: uuid
declare IMG_USE_BAKED_AUX2: uuid
declare IMG_USE_BAKED_AUX3: uuid
declare IMG_USE_BAKED_EYES: uuid
declare IMG_USE_BAKED_HAIR: uuid
declare IMG_USE_BAKED_HEAD: uuid
declare IMG_USE_BAKED_LEFTARM: uuid
declare IMG_USE_BAKED_LEFTLEG: uuid
declare IMG_USE_BAKED_LOWER: uuid
declare IMG_USE_BAKED_SKIRT: uuid
declare IMG_USE_BAKED_UPPER: uuid
declare INVENTORY_ALL: number
declare INVENTORY_ANIMATION: number
declare INVENTORY_BODYPART: number
declare INVENTORY_CLOTHING: number
declare INVENTORY_GESTURE: number
declare INVENTORY_LANDMARK: number
declare INVENTORY_MATERIAL: number
declare INVENTORY_NONE: number
declare INVENTORY_NOTECARD: number
declare INVENTORY_OBJECT: number
declare INVENTORY_SCRIPT: number
declare INVENTORY_SETTING: number
declare INVENTORY_SOUND: number
declare INVENTORY_TEXTURE: number
declare JSON_APPEND: number
declare JSON_ARRAY: string
declare JSON_DELETE: string
declare JSON_FALSE: string
declare JSON_INVALID: string
declare JSON_NULL: string
declare JSON_NUMBER: string
declare JSON_OBJECT: string
declare JSON_STRING: string
declare JSON_TRUE: string
declare KFM_CMD_PAUSE: number
declare KFM_CMD_PLAY: number
declare KFM_CMD_STOP: number
declare KFM_COMMAND: number
declare KFM_DATA: number
declare KFM_FORWARD: number
declare KFM_LOOP: number
declare KFM_MODE: number
declare KFM_PING_PONG: number
declare KFM_REVERSE: number
declare KFM_ROTATION: number
declare KFM_TRANSLATION: number
declare LAND_LARGE_BRUSH: number
declare LAND_LEVEL: number
declare LAND_LOWER: number
declare LAND_MEDIUM_BRUSH: number
declare LAND_NOISE: number
declare LAND_RAISE: number
declare LAND_REVERT: number
declare LAND_SMALL_BRUSH: number
declare LAND_SMOOTH: number
declare LINKSETDATA_DELETE: number
declare LINKSETDATA_EMEMORY: number
declare LINKSETDATA_ENOKEY: number
declare LINKSETDATA_EPROTECTED: number
declare LINKSETDATA_MULTIDELETE: number
declare LINKSETDATA_NOTFOUND: number
declare LINKSETDATA_NOUPDATE: number
declare LINKSETDATA_OK: number
declare LINKSETDATA_RESET: number
declare LINKSETDATA_UPDATE: number
declare LINK_ALL_CHILDREN: number
declare LINK_ALL_OTHERS: number
declare LINK_ROOT: number
declare LINK_SET: number
declare LINK_THIS: number
declare LIST_STAT_GEOMETRIC_MEAN: number
declare LIST_STAT_MAX: number
declare LIST_STAT_MEAN: number
declare LIST_STAT_MEDIAN: number
declare LIST_STAT_MIN: number
declare LIST_STAT_NUM_COUNT: number
declare LIST_STAT_RANGE: number
declare LIST_STAT_STD_DEV: number
declare LIST_STAT_SUM: number
declare LIST_STAT_SUM_SQUARES: number
declare LOOP: number
declare MASK_BASE: number
declare MASK_COMBINED: number
declare MASK_EVERYONE: number
declare MASK_GROUP: number
declare MASK_NEXT: number
declare MASK_OWNER: number
declare NAK: string
declare NULL_KEY: uuid
declare OBJECT_ACCOUNT_LEVEL: number
declare OBJECT_ANIMATED_COUNT: number
declare OBJECT_ANIMATED_SLOTS_AVAILABLE: number
declare OBJECT_ATTACHED_POINT: number
declare OBJECT_ATTACHED_SLOTS_AVAILABLE: number
declare OBJECT_BODY_SHAPE_TYPE: number
declare OBJECT_CHARACTER_TIME: number
declare OBJECT_CLICK_ACTION: number
declare OBJECT_CREATION_TIME: number
declare OBJECT_CREATOR: number
declare OBJECT_DAMAGE: number
declare OBJECT_DAMAGE_TYPE: number
declare OBJECT_DESC: number
declare OBJECT_GROUP: number
declare OBJECT_GROUP_TAG: number
declare OBJECT_HEALTH: number
declare OBJECT_HOVER_HEIGHT: number
declare OBJECT_LAST_OWNER_ID: number
declare OBJECT_LINK_NUMBER: number
declare OBJECT_MASS: number
declare OBJECT_MATERIAL: number
declare OBJECT_NAME: number
declare OBJECT_OMEGA: number
declare OBJECT_OWNER: number
declare OBJECT_PATHFINDING_TYPE: number
declare OBJECT_PERMS: number
declare OBJECT_PERMS_COMBINED: number
declare OBJECT_PHANTOM: number
declare OBJECT_PHYSICS: number
declare OBJECT_PHYSICS_COST: number
declare OBJECT_POS: number
declare OBJECT_PRIM_COUNT: number
declare OBJECT_PRIM_EQUIVALENCE: number
declare OBJECT_RENDER_WEIGHT: number
declare OBJECT_RETURN_PARCEL: number
declare OBJECT_RETURN_PARCEL_OWNER: number
declare OBJECT_RETURN_REGION: number
declare OBJECT_REZZER_KEY: number
declare OBJECT_REZ_TIME: number
declare OBJECT_ROOT: number
declare OBJECT_ROT: number
declare OBJECT_RUNNING_SCRIPT_COUNT: number
declare OBJECT_SCALE: number
declare OBJECT_SCRIPT_MEMORY: number
declare OBJECT_SCRIPT_TIME: number
declare OBJECT_SELECT_COUNT: number
declare OBJECT_SERVER_COST: number
declare OBJECT_SIT_COUNT: number
declare OBJECT_STREAMING_COST: number
declare OBJECT_TEMP_ATTACHED: number
declare OBJECT_TEMP_ON_REZ: number
declare OBJECT_TEXT: number
declare OBJECT_TEXT_ALPHA: number
declare OBJECT_TEXT_COLOR: number
declare OBJECT_TOTAL_INVENTORY_COUNT: number
declare OBJECT_TOTAL_SCRIPT_COUNT: number
declare OBJECT_UNKNOWN_DETAIL: number
declare OBJECT_VELOCITY: number
declare OPT_AVATAR: number
declare OPT_CHARACTER: number
declare OPT_EXCLUSION_VOLUME: number
declare OPT_LEGACY_LINKSET: number
declare OPT_MATERIAL_VOLUME: number
declare OPT_OTHER: number
declare OPT_STATIC_OBSTACLE: number
declare OPT_WALKABLE: number
declare OVERRIDE_GLTF_BASE_ALPHA: number
declare OVERRIDE_GLTF_BASE_ALPHA_MASK: number
declare OVERRIDE_GLTF_BASE_ALPHA_MODE: number
declare OVERRIDE_GLTF_BASE_COLOR_FACTOR: number
declare OVERRIDE_GLTF_BASE_DOUBLE_SIDED: number
declare OVERRIDE_GLTF_EMISSIVE_FACTOR: number
declare OVERRIDE_GLTF_METALLIC_FACTOR: number
declare OVERRIDE_GLTF_ROUGHNESS_FACTOR: number
declare PARCEL_COUNT_GROUP: number
declare PARCEL_COUNT_OTHER: number
declare PARCEL_COUNT_OWNER: number
declare PARCEL_COUNT_SELECTED: number
declare PARCEL_COUNT_TEMP: number
declare PARCEL_COUNT_TOTAL: number
declare PARCEL_DETAILS_AREA: number
declare PARCEL_DETAILS_DESC: number
declare PARCEL_DETAILS_FLAGS: number
declare PARCEL_DETAILS_GROUP: number
declare PARCEL_DETAILS_ID: number
declare PARCEL_DETAILS_LANDING_LOOKAT: number
declare PARCEL_DETAILS_LANDING_POINT: number
declare PARCEL_DETAILS_NAME: number
declare PARCEL_DETAILS_OWNER: number
declare PARCEL_DETAILS_PRIM_CAPACITY: number
declare PARCEL_DETAILS_PRIM_USED: number
declare PARCEL_DETAILS_SCRIPT_DANGER: number
declare PARCEL_DETAILS_SEE_AVATARS: number
declare PARCEL_DETAILS_TP_ROUTING: number
declare PARCEL_FLAG_ALLOW_ALL_OBJECT_ENTRY: number
declare PARCEL_FLAG_ALLOW_CREATE_GROUP_OBJECTS: number
declare PARCEL_FLAG_ALLOW_CREATE_OBJECTS: number
declare PARCEL_FLAG_ALLOW_DAMAGE: number
declare PARCEL_FLAG_ALLOW_FLY: number
declare PARCEL_FLAG_ALLOW_GROUP_OBJECT_ENTRY: number
declare PARCEL_FLAG_ALLOW_GROUP_SCRIPTS: number
declare PARCEL_FLAG_ALLOW_LANDMARK: number
declare PARCEL_FLAG_ALLOW_SCRIPTS: number
declare PARCEL_FLAG_ALLOW_TERRAFORM: number
declare PARCEL_FLAG_LOCAL_SOUND_ONLY: number
declare PARCEL_FLAG_RESTRICT_PUSHOBJECT: number
declare PARCEL_FLAG_USE_ACCESS_GROUP: number
declare PARCEL_FLAG_USE_ACCESS_LIST: number
declare PARCEL_FLAG_USE_BAN_LIST: number
declare PARCEL_FLAG_USE_LAND_PASS_LIST: number
declare PARCEL_MEDIA_COMMAND_AGENT: number
declare PARCEL_MEDIA_COMMAND_AUTO_ALIGN: number
declare PARCEL_MEDIA_COMMAND_DESC: number
declare PARCEL_MEDIA_COMMAND_LOOP: number
declare PARCEL_MEDIA_COMMAND_LOOP_SET: number
declare PARCEL_MEDIA_COMMAND_PAUSE: number
declare PARCEL_MEDIA_COMMAND_PLAY: number
declare PARCEL_MEDIA_COMMAND_SIZE: number
declare PARCEL_MEDIA_COMMAND_STOP: number
declare PARCEL_MEDIA_COMMAND_TEXTURE: number
declare PARCEL_MEDIA_COMMAND_TIME: number
declare PARCEL_MEDIA_COMMAND_TYPE: number
declare PARCEL_MEDIA_COMMAND_UNLOAD: number
declare PARCEL_MEDIA_COMMAND_URL: number
declare PARCEL_SALE_AGENT: number
declare PARCEL_SALE_ERROR_BAD_PARAMS: number
declare PARCEL_SALE_ERROR_INVALID_PRICE: number
declare PARCEL_SALE_ERROR_IN_ESCROW: number
declare PARCEL_SALE_ERROR_NO_PARCEL: number
declare PARCEL_SALE_ERROR_NO_PERMISSIONS: number
declare PARCEL_SALE_OBJECTS: number
declare PARCEL_SALE_OK: number
declare PARCEL_SALE_PRICE: number
declare PASSIVE: number
declare PASS_ALWAYS: number
declare PASS_IF_NOT_HANDLED: number
declare PASS_NEVER: number
declare PATROL_PAUSE_AT_WAYPOINTS: number
declare PAYMENT_INFO_ON_FILE: number
declare PAYMENT_INFO_USED: number
declare PAY_DEFAULT: number
declare PAY_HIDE: number
declare PERMISSION_ATTACH: number
declare PERMISSION_CHANGE_JOINTS: number
declare PERMISSION_CHANGE_LINKS: number
declare PERMISSION_CHANGE_PERMISSIONS: number
declare PERMISSION_CONTROL_CAMERA: number
declare PERMISSION_DEBIT: number
declare PERMISSION_OVERRIDE_ANIMATIONS: number
declare PERMISSION_PRIVILEGED_LAND_ACCESS: number
declare PERMISSION_RELEASE_OWNERSHIP: number
declare PERMISSION_REMAP_CONTROLS: number
declare PERMISSION_RETURN_OBJECTS: number
declare PERMISSION_SILENT_ESTATE_MANAGEMENT: number
declare PERMISSION_TAKE_CONTROLS: number
declare PERMISSION_TELEPORT: number
declare PERMISSION_TRACK_CAMERA: number
declare PERMISSION_TRIGGER_ANIMATION: number
declare PERM_ALL: number
declare PERM_COPY: number
declare PERM_MODIFY: number
declare PERM_MOVE: number
declare PERM_TRANSFER: number
declare PI: number
declare PING_PONG: number
declare PI_BY_TWO: number
declare PRIM_ALLOW_UNSIT: number
declare PRIM_ALPHA_MODE: number
declare PRIM_ALPHA_MODE_BLEND: number
declare PRIM_ALPHA_MODE_EMISSIVE: number
declare PRIM_ALPHA_MODE_MASK: number
declare PRIM_ALPHA_MODE_NONE: number
declare PRIM_BUMP_BARK: number
declare PRIM_BUMP_BLOBS: number
declare PRIM_BUMP_BRICKS: number
declare PRIM_BUMP_BRIGHT: number
declare PRIM_BUMP_CHECKER: number
declare PRIM_BUMP_CONCRETE: number
declare PRIM_BUMP_DARK: number
declare PRIM_BUMP_DISKS: number
declare PRIM_BUMP_GRAVEL: number
declare PRIM_BUMP_LARGETILE: number
declare PRIM_BUMP_NONE: number
declare PRIM_BUMP_SHINY: number
declare PRIM_BUMP_SIDING: number
declare PRIM_BUMP_STONE: number
declare PRIM_BUMP_STUCCO: number
declare PRIM_BUMP_SUCTION: number
declare PRIM_BUMP_TILE: number
declare PRIM_BUMP_WEAVE: number
declare PRIM_BUMP_WOOD: number
declare PRIM_CAST_SHADOWS: number
declare PRIM_CLICK_ACTION: number
declare PRIM_COLLISION_SOUND: number
declare PRIM_COLOR: number
declare PRIM_DAMAGE: number
declare PRIM_DESC: number
declare PRIM_FLEXIBLE: number
declare PRIM_FULLBRIGHT: number
declare PRIM_GLOW: number
declare PRIM_GLTF_ALPHA_MODE_BLEND: number
declare PRIM_GLTF_ALPHA_MODE_MASK: number
declare PRIM_GLTF_ALPHA_MODE_OPAQUE: number
declare PRIM_GLTF_BASE_COLOR: number
declare PRIM_GLTF_EMISSIVE: number
declare PRIM_GLTF_METALLIC_ROUGHNESS: number
declare PRIM_GLTF_NORMAL: number
declare PRIM_HEALTH: number
declare PRIM_HOLE_CIRCLE: number
declare PRIM_HOLE_DEFAULT: number
declare PRIM_HOLE_SQUARE: number
declare PRIM_HOLE_TRIANGLE: number
declare PRIM_LINK_TARGET: number
declare PRIM_MATERIAL: number
declare PRIM_MATERIAL_FLESH: number
declare PRIM_MATERIAL_GLASS: number
declare PRIM_MATERIAL_LIGHT: number
declare PRIM_MATERIAL_METAL: number
declare PRIM_MATERIAL_PLASTIC: number
declare PRIM_MATERIAL_RUBBER: number
declare PRIM_MATERIAL_STONE: number
declare PRIM_MATERIAL_WOOD: number
declare PRIM_MEDIA_ALT_IMAGE_ENABLE: number
declare PRIM_MEDIA_AUTO_LOOP: number
declare PRIM_MEDIA_AUTO_PLAY: number
declare PRIM_MEDIA_AUTO_SCALE: number
declare PRIM_MEDIA_AUTO_ZOOM: number
declare PRIM_MEDIA_CONTROLS: number
declare PRIM_MEDIA_CONTROLS_MINI: number
declare PRIM_MEDIA_CONTROLS_STANDARD: number
declare PRIM_MEDIA_CURRENT_URL: number
declare PRIM_MEDIA_FIRST_CLICK_INTERACT: number
declare PRIM_MEDIA_HEIGHT_PIXELS: number
declare PRIM_MEDIA_HOME_URL: number
declare PRIM_MEDIA_MAX_HEIGHT_PIXELS: number
declare PRIM_MEDIA_MAX_URL_LENGTH: number
declare PRIM_MEDIA_MAX_WHITELIST_COUNT: number
declare PRIM_MEDIA_MAX_WHITELIST_SIZE: number
declare PRIM_MEDIA_MAX_WIDTH_PIXELS: number
declare PRIM_MEDIA_PARAM_MAX: number
declare PRIM_MEDIA_PERMS_CONTROL: number
declare PRIM_MEDIA_PERMS_INTERACT: number
declare PRIM_MEDIA_PERM_ANYONE: number
declare PRIM_MEDIA_PERM_GROUP: number
declare PRIM_MEDIA_PERM_NONE: number
declare PRIM_MEDIA_PERM_OWNER: number
declare PRIM_MEDIA_WHITELIST: number
declare PRIM_MEDIA_WHITELIST_ENABLE: number
declare PRIM_MEDIA_WIDTH_PIXELS: number
declare PRIM_NAME: number
declare PRIM_NORMAL: number
declare PRIM_OMEGA: number
declare PRIM_PHANTOM: number
declare PRIM_PHYSICS: number
declare PRIM_PHYSICS_SHAPE_CONVEX: number
declare PRIM_PHYSICS_SHAPE_NONE: number
declare PRIM_PHYSICS_SHAPE_PRIM: number
declare PRIM_PHYSICS_SHAPE_TYPE: number
declare PRIM_POINT_LIGHT: number
declare PRIM_POSITION: number
declare PRIM_POS_LOCAL: number
declare PRIM_PROJECTOR: number
declare PRIM_REFLECTION_PROBE: number
declare PRIM_REFLECTION_PROBE_BOX: number
declare PRIM_REFLECTION_PROBE_DYNAMIC: number
declare PRIM_REFLECTION_PROBE_MIRROR: number
declare PRIM_RENDER_MATERIAL: number
declare PRIM_ROTATION: number
declare PRIM_ROT_LOCAL: number
declare PRIM_SCRIPTED_SIT_ONLY: number
declare PRIM_SCULPT_FLAG_ANIMESH: number
declare PRIM_SCULPT_FLAG_INVERT: number
declare PRIM_SCULPT_FLAG_MIRROR: number
declare PRIM_SCULPT_TYPE_CYLINDER: number
declare PRIM_SCULPT_TYPE_MASK: number
declare PRIM_SCULPT_TYPE_MESH: number
declare PRIM_SCULPT_TYPE_PLANE: number
declare PRIM_SCULPT_TYPE_SPHERE: number
declare PRIM_SCULPT_TYPE_TORUS: number
declare PRIM_SHINY_HIGH: number
declare PRIM_SHINY_LOW: number
declare PRIM_SHINY_MEDIUM: number
declare PRIM_SHINY_NONE: number
declare PRIM_SIT_FLAGS: number
declare PRIM_SIT_TARGET: number
declare PRIM_SIZE: number
declare PRIM_SLICE: number
declare PRIM_SPECULAR: number
declare PRIM_TEMP_ON_REZ: number
declare PRIM_TEXGEN: number
declare PRIM_TEXGEN_DEFAULT: number
declare PRIM_TEXGEN_PLANAR: number
declare PRIM_TEXT: number
declare PRIM_TEXTURE: number
declare PRIM_TYPE: number
declare PRIM_TYPE_BOX: number
declare PRIM_TYPE_CYLINDER: number
declare PRIM_TYPE_PRISM: number
declare PRIM_TYPE_RING: number
declare PRIM_TYPE_SCULPT: number
declare PRIM_TYPE_SPHERE: number
declare PRIM_TYPE_TORUS: number
declare PRIM_TYPE_TUBE: number
declare PROFILE_NONE: number
declare PROFILE_SCRIPT_MEMORY: number
declare PSYS_PART_BF_DEST_COLOR: number
declare PSYS_PART_BF_ONE: number
declare PSYS_PART_BF_ONE_MINUS_DEST_COLOR: number
declare PSYS_PART_BF_ONE_MINUS_SOURCE_ALPHA: number
declare PSYS_PART_BF_ONE_MINUS_SOURCE_COLOR: number
declare PSYS_PART_BF_SOURCE_ALPHA: number
declare PSYS_PART_BF_SOURCE_COLOR: number
declare PSYS_PART_BF_ZERO: number
declare PSYS_PART_BLEND_FUNC_DEST: number
declare PSYS_PART_BLEND_FUNC_SOURCE: number
declare PSYS_PART_BOUNCE_MASK: number
declare PSYS_PART_EMISSIVE_MASK: number
declare PSYS_PART_END_ALPHA: number
declare PSYS_PART_END_COLOR: number
declare PSYS_PART_END_GLOW: number
declare PSYS_PART_END_SCALE: number
declare PSYS_PART_FLAGS: number
declare PSYS_PART_FOLLOW_SRC_MASK: number
declare PSYS_PART_FOLLOW_VELOCITY_MASK: number
declare PSYS_PART_INTERP_COLOR_MASK: number
declare PSYS_PART_INTERP_SCALE_MASK: number
declare PSYS_PART_MAX_AGE: number
declare PSYS_PART_RIBBON_MASK: number
declare PSYS_PART_START_ALPHA: number
declare PSYS_PART_START_COLOR: number
declare PSYS_PART_START_GLOW: number
declare PSYS_PART_START_SCALE: number
declare PSYS_PART_TARGET_LINEAR_MASK: number
declare PSYS_PART_TARGET_POS_MASK: number
declare PSYS_PART_WIND_MASK: number
declare PSYS_SRC_ACCEL: number
declare PSYS_SRC_ANGLE_BEGIN: number
declare PSYS_SRC_ANGLE_END: number
declare PSYS_SRC_BURST_PART_COUNT: number
declare PSYS_SRC_BURST_RADIUS: number
declare PSYS_SRC_BURST_RATE: number
declare PSYS_SRC_BURST_SPEED_MAX: number
declare PSYS_SRC_BURST_SPEED_MIN: number
declare PSYS_SRC_INNERANGLE: number
declare PSYS_SRC_MAX_AGE: number
declare PSYS_SRC_OMEGA: number
declare PSYS_SRC_OUTERANGLE: number
declare PSYS_SRC_PATTERN: number
declare PSYS_SRC_PATTERN_ANGLE: number
declare PSYS_SRC_PATTERN_ANGLE_CONE: number
declare PSYS_SRC_PATTERN_ANGLE_CONE_EMPTY: number
declare PSYS_SRC_PATTERN_DROP: number
declare PSYS_SRC_PATTERN_EXPLODE: number
declare PSYS_SRC_TARGET_KEY: number
declare PSYS_SRC_TEXTURE: number
declare PUBLIC_CHANNEL: number
declare PURSUIT_FUZZ_FACTOR: number
declare PURSUIT_GOAL_TOLERANCE: number
declare PURSUIT_INTERCEPT: number
declare PURSUIT_OFFSET: number
declare PU_EVADE_HIDDEN: number
declare PU_EVADE_SPOTTED: number
declare PU_FAILURE_DYNAMIC_PATHFINDING_DISABLED: number
declare PU_FAILURE_INVALID_GOAL: number
declare PU_FAILURE_INVALID_START: number
declare PU_FAILURE_NO_NAVMESH: number
declare PU_FAILURE_NO_VALID_DESTINATION: number
declare PU_FAILURE_OTHER: number
declare PU_FAILURE_PARCEL_UNREACHABLE: number
declare PU_FAILURE_TARGET_GONE: number
declare PU_FAILURE_UNREACHABLE: number
declare PU_GOAL_REACHED: number
declare PU_SLOWDOWN_DISTANCE_REACHED: number
declare RAD_TO_DEG: number
declare RCERR_CAST_TIME_EXCEEDED: number
declare RCERR_SIM_PERF_LOW: number
declare RCERR_UNKNOWN: number
declare RC_DATA_FLAGS: number
declare RC_DETECT_PHANTOM: number
declare RC_GET_LINK_NUM: number
declare RC_GET_NORMAL: number
declare RC_GET_ROOT_KEY: number
declare RC_MAX_HITS: number
declare RC_REJECT_AGENTS: number
declare RC_REJECT_LAND: number
declare RC_REJECT_NONPHYSICAL: number
declare RC_REJECT_PHYSICAL: number
declare RC_REJECT_TYPES: number
declare REGION_FLAG_ALLOW_DAMAGE: number
declare REGION_FLAG_ALLOW_DIRECT_TELEPORT: number
declare REGION_FLAG_BLOCK_FLY: number
declare REGION_FLAG_BLOCK_FLYOVER: number
declare REGION_FLAG_BLOCK_TERRAFORM: number
declare REGION_FLAG_DISABLE_COLLISIONS: number
declare REGION_FLAG_DISABLE_PHYSICS: number
declare REGION_FLAG_FIXED_SUN: number
declare REGION_FLAG_RESTRICT_PUSHOBJECT: number
declare REGION_FLAG_SANDBOX: number
declare REMOTE_DATA_CHANNEL: number
declare REMOTE_DATA_REPLY: number
declare REMOTE_DATA_REQUEST: number
declare REQUIRE_LINE_OF_SIGHT: number
declare RESTITUTION: number
declare REVERSE: number
declare REZ_ACCEL: number
declare REZ_DAMAGE: number
declare REZ_DAMAGE_TYPE: number
declare REZ_FLAGS: number
declare REZ_FLAG_BLOCK_GRAB_OBJECT: number
declare REZ_FLAG_DIE_ON_COLLIDE: number
declare REZ_FLAG_DIE_ON_NOENTRY: number
declare REZ_FLAG_NO_COLLIDE_FAMILY: number
declare REZ_FLAG_NO_COLLIDE_OWNER: number
declare REZ_FLAG_PHANTOM: number
declare REZ_FLAG_PHYSICAL: number
declare REZ_FLAG_TEMP: number
declare REZ_LOCK_AXES: number
declare REZ_OMEGA: number
declare REZ_PARAM: number
declare REZ_PARAM_STRING: number
declare REZ_POS: number
declare REZ_ROT: number
declare REZ_SOUND: number
declare REZ_SOUND_COLLIDE: number
declare REZ_VEL: number
declare ROTATE: number
declare SCALE: number
declare SCRIPTED: number
declare SIM_STAT_ACTIVE_SCRIPT_COUNT: number
declare SIM_STAT_AGENT_COUNT: number
declare SIM_STAT_AGENT_MS: number
declare SIM_STAT_AGENT_UPDATES: number
declare SIM_STAT_AI_MS: number
declare SIM_STAT_ASSET_DOWNLOADS: number
declare SIM_STAT_ASSET_UPLOADS: number
declare SIM_STAT_CHILD_AGENT_COUNT: number
declare SIM_STAT_FRAME_MS: number
declare SIM_STAT_IMAGE_MS: number
declare SIM_STAT_IO_PUMP_MS: number
declare SIM_STAT_NET_MS: number
declare SIM_STAT_OTHER_MS: number
declare SIM_STAT_PACKETS_IN: number
declare SIM_STAT_PACKETS_OUT: number
declare SIM_STAT_PCT_CHARS_STEPPED: number
declare SIM_STAT_PHYSICS_FPS: number
declare SIM_STAT_PHYSICS_MS: number
declare SIM_STAT_PHYSICS_OTHER_MS: number
declare SIM_STAT_PHYSICS_SHAPE_MS: number
declare SIM_STAT_PHYSICS_STEP_MS: number
declare SIM_STAT_SCRIPT_EPS: number
declare SIM_STAT_SCRIPT_MS: number
declare SIM_STAT_SCRIPT_RUN_PCT: number
declare SIM_STAT_SLEEP_MS: number
declare SIM_STAT_SPARE_MS: number
declare SIM_STAT_UNACKED_BYTES: number
declare SIT_FLAG_ALLOW_UNSIT: number
declare SIT_FLAG_NO_COLLIDE: number
declare SIT_FLAG_NO_DAMAGE: number
declare SIT_FLAG_SCRIPTED_ONLY: number
declare SIT_FLAG_SIT_TARGET: number
declare SIT_INVALID_AGENT: number
declare SIT_INVALID_LINK: number
declare SIT_INVALID_OBJECT: number
declare SIT_NOT_EXPERIENCE: number
declare SIT_NO_ACCESS: number
declare SIT_NO_EXPERIENCE_PERMISSION: number
declare SIT_NO_SIT_TARGET: number
declare SKY_AMBIENT: number
declare SKY_BLUE: number
declare SKY_CLOUDS: number
declare SKY_CLOUD_TEXTURE: number
declare SKY_DOME: number
declare SKY_GAMMA: number
declare SKY_GLOW: number
declare SKY_HAZE: number
declare SKY_LIGHT: number
declare SKY_MOON: number
declare SKY_MOON_TEXTURE: number
declare SKY_PLANET: number
declare SKY_REFLECTION_PROBE_AMBIANCE: number
declare SKY_REFRACTION: number
declare SKY_STAR_BRIGHTNESS: number
declare SKY_SUN: number
declare SKY_SUN_TEXTURE: number
declare SKY_TEXTURE_DEFAULTS: number
declare SKY_TRACKS: number
declare SMOOTH: number
declare SOUND_LOOP: number
declare SOUND_PLAY: number
declare SOUND_SYNC: number
declare SOUND_TRIGGER: number
declare SQRT2: number
declare STATUS_BLOCK_GRAB: number
declare STATUS_BLOCK_GRAB_OBJECT: number
declare STATUS_BOUNDS_ERROR: number
declare STATUS_CAST_SHADOWS: number
declare STATUS_DIE_AT_EDGE: number
declare STATUS_DIE_AT_NO_ENTRY: number
declare STATUS_INTERNAL_ERROR: number
declare STATUS_MALFORMED_PARAMS: number
declare STATUS_NOT_FOUND: number
declare STATUS_NOT_SUPPORTED: number
declare STATUS_OK: number
declare STATUS_PHANTOM: number
declare STATUS_PHYSICS: number
declare STATUS_RETURN_AT_EDGE: number
declare STATUS_ROTATE_X: number
declare STATUS_ROTATE_Y: number
declare STATUS_ROTATE_Z: number
declare STATUS_SANDBOX: number
declare STATUS_TYPE_MISMATCH: number
declare STATUS_WHITELIST_FAILED: number
declare STRING_TRIM: number
declare STRING_TRIM_HEAD: number
declare STRING_TRIM_TAIL: number
declare TARGETED_EMAIL_OBJECT_OWNER: number
declare TARGETED_EMAIL_ROOT_CREATOR: number
declare TERRAIN_DETAIL_1: number
declare TERRAIN_DETAIL_2: number
declare TERRAIN_DETAIL_3: number
declare TERRAIN_DETAIL_4: number
declare TERRAIN_HEIGHT_RANGE_NE: number
declare TERRAIN_HEIGHT_RANGE_NW: number
declare TERRAIN_HEIGHT_RANGE_SE: number
declare TERRAIN_HEIGHT_RANGE_SW: number
declare TERRAIN_PBR_OFFSET_1: number
declare TERRAIN_PBR_OFFSET_2: number
declare TERRAIN_PBR_OFFSET_3: number
declare TERRAIN_PBR_OFFSET_4: number
declare TERRAIN_PBR_ROTATION_1: number
declare TERRAIN_PBR_ROTATION_2: number
declare TERRAIN_PBR_ROTATION_3: number
declare TERRAIN_PBR_ROTATION_4: number
declare TERRAIN_PBR_SCALE_1: number
declare TERRAIN_PBR_SCALE_2: number
declare TERRAIN_PBR_SCALE_3: number
declare TERRAIN_PBR_SCALE_4: number
declare TEXTURE_BLANK: uuid
declare TEXTURE_DEFAULT: uuid
declare TEXTURE_MEDIA: uuid
declare TEXTURE_PLYWOOD: uuid
declare TEXTURE_TRANSPARENT: uuid
declare TOUCH_INVALID_FACE: number
declare TOUCH_INVALID_TEXCOORD: vector
declare TOUCH_INVALID_VECTOR: vector
declare TP_ROUTING_BLOCKED: number
declare TP_ROUTING_FREE: number
declare TP_ROUTING_LANDINGP: number
declare TRANSFER_BAD_OPTS: number
declare TRANSFER_BAD_ROOT: number
declare TRANSFER_DEST: number
declare TRANSFER_FLAGS: number
declare TRANSFER_FLAG_COPY: number
declare TRANSFER_FLAG_RESERVED: number
declare TRANSFER_FLAG_TAKE: number
declare TRANSFER_NO_ATTACHMENT: number
declare TRANSFER_NO_ITEMS: number
declare TRANSFER_NO_PERMS: number
declare TRANSFER_NO_TARGET: number
declare TRANSFER_OK: number
declare TRANSFER_THROTTLE: number
declare TRAVERSAL_TYPE: number
declare TRAVERSAL_TYPE_FAST: number
declare TRAVERSAL_TYPE_NONE: number
declare TRAVERSAL_TYPE_SLOW: number
declare TWO_PI: number
declare TYPE_FLOAT: number
declare TYPE_INTEGER: number
declare TYPE_INVALID: number
declare TYPE_KEY: number
declare TYPE_ROTATION: number
declare TYPE_STRING: number
declare TYPE_VECTOR: number
declare URL_REQUEST_DENIED: string
declare URL_REQUEST_GRANTED: string
declare VEHICLE_ANGULAR_DEFLECTION_EFFICIENCY: number
declare VEHICLE_ANGULAR_DEFLECTION_TIMESCALE: number
declare VEHICLE_ANGULAR_FRICTION_TIMESCALE: number
declare VEHICLE_ANGULAR_MOTOR_DECAY_TIMESCALE: number
declare VEHICLE_ANGULAR_MOTOR_DIRECTION: number
declare VEHICLE_ANGULAR_MOTOR_TIMESCALE: number
declare VEHICLE_BANKING_EFFICIENCY: number
declare VEHICLE_BANKING_MIX: number
declare VEHICLE_BANKING_TIMESCALE: number
declare VEHICLE_BUOYANCY: number
declare VEHICLE_FLAG_BLOCK_INTERFERENCE: number
declare VEHICLE_FLAG_CAMERA_DECOUPLED: number
declare VEHICLE_FLAG_HOVER_GLOBAL_HEIGHT: number
declare VEHICLE_FLAG_HOVER_TERRAIN_ONLY: number
declare VEHICLE_FLAG_HOVER_UP_ONLY: number
declare VEHICLE_FLAG_HOVER_WATER_ONLY: number
declare VEHICLE_FLAG_LIMIT_MOTOR_UP: number
declare VEHICLE_FLAG_LIMIT_ROLL_ONLY: number
declare VEHICLE_FLAG_MOUSELOOK_BANK: number
declare VEHICLE_FLAG_MOUSELOOK_STEER: number
declare VEHICLE_FLAG_NO_DEFLECTION_UP: number
declare VEHICLE_FLAG_NO_FLY_UP: number
declare VEHICLE_HOVER_EFFICIENCY: number
declare VEHICLE_HOVER_HEIGHT: number
declare VEHICLE_HOVER_TIMESCALE: number
declare VEHICLE_LINEAR_DEFLECTION_EFFICIENCY: number
declare VEHICLE_LINEAR_DEFLECTION_TIMESCALE: number
declare VEHICLE_LINEAR_FRICTION_TIMESCALE: number
declare VEHICLE_LINEAR_MOTOR_DECAY_TIMESCALE: number
declare VEHICLE_LINEAR_MOTOR_DIRECTION: number
declare VEHICLE_LINEAR_MOTOR_OFFSET: number
declare VEHICLE_LINEAR_MOTOR_TIMESCALE: number
declare VEHICLE_REFERENCE_FRAME: number
declare VEHICLE_TYPE_AIRPLANE: number
declare VEHICLE_TYPE_BALLOON: number
declare VEHICLE_TYPE_BOAT: number
declare VEHICLE_TYPE_CAR: number
declare VEHICLE_TYPE_NONE: number
declare VEHICLE_TYPE_SLED: number
declare VEHICLE_VERTICAL_ATTRACTION_EFFICIENCY: number
declare VEHICLE_VERTICAL_ATTRACTION_TIMESCALE: number
declare VERTICAL: number
declare WANDER_PAUSE_AT_WAYPOINTS: number
declare WATER_BLUR_MULTIPLIER: number
declare WATER_FOG: number
declare WATER_FRESNEL: number
declare WATER_NORMAL_SCALE: number
declare WATER_NORMAL_TEXTURE: number
declare WATER_REFRACTION: number
declare WATER_TEXTURE_DEFAULTS: number
declare WATER_WAVE_DIRECTION: number
declare XP_ERROR_EXPERIENCES_DISABLED: number
declare XP_ERROR_EXPERIENCE_DISABLED: number
declare XP_ERROR_EXPERIENCE_SUSPENDED: number
declare XP_ERROR_INVALID_EXPERIENCE: number
declare XP_ERROR_INVALID_PARAMETERS: number
declare XP_ERROR_KEY_NOT_FOUND: number
declare XP_ERROR_MATURITY_EXCEEDED: number
declare XP_ERROR_NONE: number
declare XP_ERROR_NOT_FOUND: number
declare XP_ERROR_NOT_PERMITTED: number
declare XP_ERROR_NOT_PERMITTED_LAND: number
declare XP_ERROR_NO_EXPERIENCE: number
declare XP_ERROR_QUOTA_EXCEEDED: number
declare XP_ERROR_REQUEST_PERM_TIMEOUT: number
declare XP_ERROR_RETRY_UPDATE: number
declare XP_ERROR_STORAGE_EXCEPTION: number
declare XP_ERROR_STORE_DISABLED: number
declare XP_ERROR_THROTTLED: number
declare XP_ERROR_UNKNOWN_ERROR: number
declare ZERO_ROTATION: quaternion
declare ZERO_VECTOR: vector
