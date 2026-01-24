
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


type rotation = quaternion
type numeric = boolean | number
type list = {string | number | vector | uuid | quaternion | boolean}
type LLDetectedEventName = "collision" | "collision_end" | "collision_start" | "final_damage" | "on_damage" | "sensor" | "touch" | "touch_end" | "touch_start"
type LLNonDetectedEventName = "at_rot_target" | "at_target" | "attach" | "changed" | "control" | "dataserver" | "email" | "experience_permissions" | "experience_permissions_denied" | "game_control" | "http_request" | "http_response" | "land_collision" | "land_collision_end" | "land_collision_start" | "link_message" | "linkset_data" | "listen" | "money" | "moving_end" | "moving_start" | "no_sensor" | "not_at_rot_target" | "not_at_target" | "object_rez" | "on_death" | "on_rez" | "path_update" | "remote_data" | "run_time_permissions" | "state_entry" | "state_exit" | "timer" | "transaction_result"
type LLEventName = LLDetectedEventName | LLNonDetectedEventName
type LLEventHandler = (...any) -> ()
type LLDetectedEventHandler = (detected: {LLDetectedEvent}) -> ()
type LLTimerEveryCallback = (scheduled: number, interval: number) -> ()
type LLTimerOnceCallback = (scheduled: number) -> ()
type LLTimerCallback = LLTimerEveryCallback | LLTimerOnceCallback | (...any) -> ...any
type OsDateTime = {
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

declare extern type LLDetectedEvent with
  index: number
  valid: boolean
  canAdjustDamage: boolean
  function AdjustDamage(self, Damage: number): nil
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
  at_rot_target: (TargetNumber: number, TargetRotation: quaternion, CurrentRotation: quaternion) -> () | nil
  at_target: (TargetNumber: number, TargetPosition: vector, CurrentPosition: vector) -> () | nil
  attach: (AvatarID: uuid) -> () | nil
  changed: (Changed: number) -> () | nil
  collision: (LLDetectedEventHandler)?
  collision_end: (LLDetectedEventHandler)?
  collision_start: (LLDetectedEventHandler)?
  control: (AvatarID: uuid, Levels: number, Edges: number) -> () | nil
  dataserver: (RequestID: uuid, Data: string) -> () | nil
  email: (Time: string, Address: string, Subject: string, Body: string, NumberRemaining: number) -> () | nil
  experience_permissions: (agent_id: uuid) -> () | nil
  experience_permissions_denied: (agent_id: uuid, Reason: number) -> () | nil
  final_damage: (LLDetectedEventHandler)?
  game_control: (id: uuid, buttons: number, axes: list) -> () | nil
  http_request: (HTTPRequestID: uuid, HTTPMethod: string, Body: string) -> () | nil
  http_response: (HTTPRequestID: uuid, Status: number, Metadata: list, Body: string) -> () | nil
  land_collision: (Position: vector) -> () | nil
  land_collision_end: (Position: vector) -> () | nil
  land_collision_start: (Position: vector) -> () | nil
  link_message: (SendersLink: number, Value: number, Text: string, ID: string) -> () | nil
  linkset_data: (action: number, name: string, value: string) -> () | nil
  listen: (Channel: number, Name: string, ID: uuid, Text: string) -> () | nil
  money: (Payer: uuid, Amount: number) -> () | nil
  moving_end: () -> () | nil
  moving_start: () -> () | nil
  no_sensor: () -> () | nil
  not_at_rot_target: () -> () | nil
  not_at_target: () -> () | nil
  object_rez: (RezzedObjectsID: uuid) -> () | nil
  on_damage: (LLDetectedEventHandler)?
  on_death: () -> () | nil
  on_rez: (StartParameter: number) -> () | nil
  path_update: (Type: number, Reserved: list) -> () | nil
  remote_data: (EventType: number, ChannelID: uuid, MessageID: uuid, Sender: string, IData: number, SData: string) -> () | nil
  run_time_permissions: (PermissionFlags: number) -> () | nil
  sensor: (LLDetectedEventHandler)?
  state_entry: () -> () | nil
  state_exit: () -> () | nil
  timer: () -> () | nil
  touch: (LLDetectedEventHandler)?
  touch_end: (LLDetectedEventHandler)?
  touch_start: (LLDetectedEventHandler)?
  transaction_result: (RequestID: uuid, Success: number, Message: string) -> () | nil
  function on(self, event: LLEventName, callback: LLEventHandler): LLEventHandler
  function off(self, event: LLEventName, callback: LLEventHandler): boolean
  function once(self, event: LLEventName, callback: LLEventHandler): LLEventHandler
  function listeners(self, event: LLEventName): {LLEventHandler}
  function eventNames(self): {string}
end

declare extern type LLTimers with
  function every(self, seconds: number, callback: LLTimerEveryCallback): LLTimerCallback
  function once(self, seconds: number, callback: LLTimerOnceCallback): LLTimerCallback
  function off(self, callback: LLTimerCallback): boolean
end


declare quaternion: ((x: number, y: number, z: number, s: number) -> quaternion) & {
  create: (x: number, y: number, z: number, s: number) -> quaternion,
  identity: quaternion,
  normalize: (q: quaternion) -> quaternion,
  magnitude: (q: quaternion) -> number,
  dot: (a: quaternion, b: quaternion) -> number,
  slerp: (a: quaternion, b: quaternion, t: number) -> quaternion,
  conjugate: (q: quaternion) -> quaternion,
  tofwd: (q: quaternion) -> vector,
  toleft: (q: quaternion) -> vector,
  toup: (q: quaternion) -> vector,
}
declare rotation: typeof(quaternion)
declare uuid: ((value: string | buffer | uuid) -> uuid?) & {
  create: (value: string | buffer | uuid) -> uuid?,
}
declare vector: ((x: number, y: number, z: number?) -> vector) & {
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
  zero: vector,
  one: vector,
}
declare LLEvents: LLEvents
declare LLTimers: LLTimers
declare loadstring: nil
declare getfenv: nil
declare setfenv: nil
declare function dangerouslyexecuterequiredmodule(f: (...any) -> ...any): ...any
declare function touuid(val: string | buffer | uuid): uuid?
declare function tovector(val: any): vector?
declare function toquaternion(val: any): quaternion?
declare function torotation(val: any): quaternion?

---------------------------
-- Global Table: bit32
---------------------------

declare bit32: {
  arshift: (x: number, disp: number) -> number,
  band: (...number) -> number,
  bnot: (x: number) -> number,
  bor: (...number) -> number,
  bxor: (...number) -> number,
  btest: (...number) -> boolean,
  extract: (n: number, field: number, width: number?) -> number,
  lrotate: (x: number, disp: number) -> number,
  lshift: (x: number, disp: number) -> number,
  replace: (n: number, v: number, field: number, width: number?) -> number,
  rrotate: (x: number, disp: number) -> number,
  rshift: (x: number, disp: number) -> number,
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
  info: ((thread: thread, level: number, options: string) -> ...any) & ((level: number, options: string) -> ...any) & ((func: (...any) -> ...any, options: string) -> ...any),
  traceback: ((thread: thread, message: string?, level: number?) -> string) & ((message: string?, level: number?) -> string),
}


---------------------------
-- Global Table: llbase64
---------------------------

declare llbase64: {
  encode: (data: string | buffer) -> string,
  decode: ((data: string) -> string) & ((data: string, asBuffer: boolean?) -> string | buffer),
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
  abs: (x: number) -> number,
  acos: (x: number) -> number,
  asin: (x: number) -> number,
  atan: (x: number) -> number,
  atan2: (y: number, x: number) -> number,
  ceil: (x: number) -> number,
  clamp: (n: number, min: number, max: number) -> number,
  cos: (x: number) -> number,
  cosh: (x: number) -> number,
  deg: (x: number) -> number,
  exp: (x: number) -> number,
  floor: (x: number) -> number,
  fmod: (x: number, y: number) -> number,
  frexp: (x: number) -> (number, number),
  ldexp: (m: number, e: number) -> number,
  lerp: (a: number, b: number, t: number) -> number,
  log: (x: number, base: number?) -> number,
  log10: (x: number) -> number,
  map: (x: number, inMin: number, inMax: number, outMin: number, outMax: number) -> number,
  max: (x: number, ...number) -> number,
  min: (x: number, ...number) -> number,
  modf: (x: number) -> (number, number),
  noise: (x: number, y: number?, z: number?) -> number,
  pow: (base: number, exponent: number) -> number,
  rad: (x: number) -> number,
  random: (m: number?, n: number?) -> number,
  randomseed: (seed: number) -> (),
  round: (x: number) -> number,
  sign: (x: number) -> number,
  sin: (x: number) -> number,
  sinh: (x: number) -> number,
  sqrt: (x: number) -> number,
  tan: (x: number) -> number,
  tanh: (x: number) -> number,
  isnan: (x: number) -> boolean,
  isinf: (x: number) -> boolean,
  isfinite: (x: number) -> boolean,
}


---------------------------
-- Global Table: os
---------------------------

declare os: {
  clock: () -> number,
  date: (format: string?, time: number?) -> string | OsDateTime | nil,
  difftime: (t2: number, t1: number?) -> number,
  time: (time: OsDateTime?) -> number?,
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
  gsub: (s: string, pattern: string, repl: string | { [string]: string } | (...string) -> string, n: number?) -> (string, number),
  len: (s: string) -> number,
  lower: (s: string) -> string,
  match: (s: string, pattern: string, init: number?) -> ...string,
  pack: (fmt: string, ...any) -> string,
  packsize: (fmt: string) -> number,
  rep: (s: string, n: number) -> string,
  reverse: (s: string) -> string,
  split: (s: string, separator: string?) -> {string},
  sub: (s: string, i: number, j: number?) -> string,
  unpack: (fmt: string, s: string, pos: number?) -> ...any,
  upper: (s: string) -> string,
}


---------------------------
-- Global Table: table
---------------------------

declare table: {
  concat: (list: {string}, sep: string?, i: number?, j: number?) -> string,
  foreach: <T, U>(t: {[T]: U}, f: (key: T, value: T) -> any) -> any?,
  foreachi: <T>(t: {T}, f: (index: number, value: T) -> any) -> any?,
  getn: (t: {any}) -> number,
  maxn: (t: {any}) -> number,
  insert: (<T>(list: {T}, value: T) -> ()) & (<T>(list: {T}, pos: number, value: T) -> ()),
  remove: <T>(list: {T}, pos: number?) -> T?,
  sort: <T>(list: {T}, comp: ((a: T, b: T) -> boolean)?) -> (),
  pack: <T>(...T) -> { n: number, [number]: T },
  unpack: <T>(list: {T}, i: number?, j: number?) -> ...T,
  move: <T>(a1: {T}, f: number, e: number, t: number, a2: {T}?) -> {T},
  create: <T>(count: number, value: T?) -> {T},
  find: <T>(t: {T}, value: T, init: number?) -> number?,
  clear: (t: {[any]: any}) -> (),
  freeze: <T, U>(t: {[T]: U}) -> {[T]: U},
  isfrozen: (t: {[any]: any}) -> boolean,
  clone: <T, U>(t: {[T]: U}) -> {[T]: U},
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
-- Global Table: ll
---------------------------

declare ll: {
  Abs: (Value: number) -> number,
  Acos: (Value: number) -> number,
  AddToLandBanList: (ID: uuid, Hours: number) -> nil,
  AddToLandPassList: (ID: uuid, Hours: number) -> nil,
  AdjustSoundVolume: (Volume: number) -> nil,
  AgentInExperience: (AgentID: uuid) -> boolean,
  AllowInventoryDrop: (Flag: numeric) -> nil,
  AngleBetween: (Rot1: quaternion, Rot2: quaternion) -> number,
  ApplyImpulse: (Force: vector, Local: numeric) -> nil,
  ApplyRotationalImpulse: (Force: vector, Local: numeric) -> nil,
  Asin: (Value: number) -> number,
  Atan2: (y: number, x: number) -> number,
  AttachToAvatar: (AttachmentPoint: number) -> nil,
  AttachToAvatarTemp: (AttachPoint: number) -> nil,
  AvatarOnLinkSitTarget: (LinkNumber: number) -> uuid,
  AvatarOnSitTarget: () -> uuid,
  Axes2Rot: (Forward: vector, Left: vector, Up: vector) -> quaternion,
  AxisAngle2Rot: (Axis: vector, Angle: number) -> quaternion,
  Base64ToInteger: (Text: string) -> number,
  Base64ToString: (Text: string) -> string,
  BreakAllLinks: () -> nil,
  BreakLink: (LinkNumber: number) -> nil,
  CSV2List: (Text: string) -> {string},
  CastRay: (Start: vector, End: vector, Options: list) -> list,
  Ceil: (Value: number) -> number,
  Char: (value: number) -> string,
  ClearCameraParams: () -> nil,
  ClearLinkMedia: (Link: number, Face: number) -> number,
  ClearPrimMedia: (Face: number) -> number,
  CloseRemoteDataChannel: (ChannelID: uuid) -> nil,
  Cloud: (Offset: vector) -> number,
  CollisionFilter: (ObjectName: string, ObjectID: uuid, Accept: number) -> nil,
  CollisionSound: (ImpactSound: string, ImpactVolume: number) -> nil,
  CollisionSprite: (ImpactSprite: string) -> nil,
  ComputeHash: (Message: string, Algorithm: string) -> string,
  Cos: (Theta: number) -> number,
  CreateCharacter: (Options: list) -> nil,
  CreateKeyValue: (Key: string, Value: string) -> uuid,
  CreateLink: (TargetPrim: uuid, Parent: number) -> nil,
  Damage: (target: uuid, damage: number, type: number) -> nil,
  DataSizeKeyValue: () -> uuid,
  DeleteCharacter: () -> nil,
  DeleteKeyValue: (Key: string) -> uuid,
  DeleteSubList: <T>(Source: {T}, Start: number, End: number) -> {T},
  DeleteSubString: (Source: string, Start: number, End: number) -> string,
  DerezObject: (ID: uuid, flags: number) -> boolean,
  DetachFromAvatar: () -> nil,
  Dialog: (AvatarID: uuid, Text: string, Buttons: list, Channel: number) -> nil,
  Die: () -> nil,
  DumpList2String: (Source: list, Separator: string) -> string,
  EdgeOfWorld: (Position: vector, Direction: vector) -> boolean,
  EjectFromLand: (AvatarID: uuid) -> nil,
  Email: (Address: string, Subject: string, Text: string) -> nil,
  EscapeURL: (URL: string) -> string,
  Euler2Rot: (Vector: vector) -> quaternion,
  Evade: (TargetID: uuid, Options: list) -> nil,
  ExecCharacterCmd: (Command: number, Options: list) -> nil,
  Fabs: (Value: number) -> number,
  FindNotecardTextCount: (NotecardName: string, Pattern: string, Options: list) -> uuid,
  FindNotecardTextSync: (NotecardName: string, Pattern: string, StartMatch: number, Count: number, Options: list) -> list,
  FleeFrom: (Source: vector, Distance: number, Options: list) -> nil,
  Floor: (Value: number) -> number,
  ForceMouselook: (Enable: numeric) -> nil,
  Frand: (Magnitude: number) -> number,
  GenerateKey: () -> uuid,
  GetAccel: () -> vector,
  GetAgentInfo: (AvatarID: uuid) -> number,
  GetAgentLanguage: (AvatarID: uuid) -> string,
  GetAgentList: (Scope: number, Options: list) -> list,
  GetAgentSize: (AvatarID: uuid) -> vector,
  GetAlpha: (Face: number) -> number,
  GetAndResetTime: () -> number,
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
  GetNextEmail: (Address: string, Subject: string) -> nil,
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
  GetParcelMaxPrims: (Position: vector, SimWide: numeric) -> number,
  GetParcelMusicURL: () -> string,
  GetParcelPrimCount: (Position: vector, Category: number, SimWide: numeric) -> number,
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
  GiveInventory: (TargetID: uuid, InventoryItem: string) -> nil,
  GiveInventoryList: (TargetID: uuid, FolderName: string, InventoryItems: list) -> nil,
  GiveMoney: (AvatarID: uuid, Amount: number) -> number,
  GodLikeRezObject: (InventoryItemID: uuid, Position: vector) -> nil,
  Ground: (Offset: vector) -> number,
  GroundContour: (Offset: vector) -> vector,
  GroundNormal: (Offset: vector) -> vector,
  GroundRepel: (Height: number, Water: numeric, Tau: number) -> nil,
  GroundSlope: (Offset: vector) -> vector,
  HMAC: (Key: string, Message: string, Algorithm: string) -> string,
  HTTPRequest: (URL: string, Parameters: list, Body: string) -> uuid,
  HTTPResponse: (HTTPRequestID: uuid, Status: number, Body: string) -> nil,
  Hash: (value: string) -> number,
  InsertString: (TargetVariable: string, Position: number, SourceVariable: string) -> string,
  InstantMessage: (AvatarID: uuid, Text: string) -> nil,
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
  LinkAdjustSoundVolume: (LinkNumber: number, Volume: number) -> nil,
  LinkParticleSystem: (LinkNumber: number, Rules: list) -> nil,
  LinkPlaySound: (LinkNumber: number, Sound: string, Volume: number, Flags: number) -> nil,
  LinkSetSoundQueueing: (LinkNumber: number, QueueEnable: numeric) -> nil,
  LinkSetSoundRadius: (LinkNumber: number, radius: number) -> nil,
  LinkSitTarget: (LinkNumber: number, Offset: vector, Rotation: quaternion) -> nil,
  LinkStopSound: (LinkNumber: number) -> nil,
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
  LinksetDataReset: () -> nil,
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
  ListSort: <T>(ListVariable: {T}, Stride: number, Ascending: numeric) -> {T},
  ListSortStrided: <T>(ListVariable: {T}, Stride: number, Sortkey: number, Ascending: numeric) -> {T},
  ListStatistics: (Operation: number, ListVariable: list) -> number,
  Listen: (Channel: number, SpeakersName: string, SpeakersID: uuid, Text: string) -> number,
  ListenControl: (ChannelHandle: number, Active: number) -> nil,
  ListenRemove: (ChannelHandle: number) -> nil,
  LoadURL: (AvatarID: uuid, Text: string, URL: string) -> nil,
  Log: (Value: number) -> number,
  Log10: (Value: number) -> number,
  LookAt: (Target: vector, Strength: number, Damping: number) -> nil,
  LoopSound: (Sound: string, Volume: number) -> nil,
  LoopSoundMaster: (Sound: string, Volume: number) -> nil,
  LoopSoundSlave: (Sound: string, Volume: number) -> nil,
  MD5String: (Text: string, Nonce: number) -> string,
  MakeExplosion: (Particles: number, Scale: number, Velocity: number, Lifetime: number, Arc: number, Texture: string, Offset: vector) -> nil,
  MakeFire: (Particles: number, Scale: number, Velocity: number, Lifetime: number, Arc: number, Texture: string, Offset: vector) -> nil,
  MakeFountain: (Particles: number, Scale: number, Velocity: number, Lifetime: number, Arc: number, Bounce: number, Texture: string, Offset: vector, Bounce_Offset: number) -> nil,
  MakeSmoke: (Particles: number, Scale: number, Velocity: number, Lifetime: number, Arc: number, Texture: string, Offset: vector) -> nil,
  ManageEstateAccess: (Action: number, AvatarID: uuid) -> boolean,
  MapBeacon: (RegionName: string, Position: vector, Options: list) -> nil,
  MapDestination: (RegionName: string, Position: vector, Direction: vector) -> nil,
  MessageLinked: (LinkNumber: number, Number: number, Text: string | uuid, ID: string | uuid) -> nil,
  MinEventDelay: (Delay: number) -> nil,
  ModPow: (Value: number, Power: number, Modulus: number) -> number,
  ModifyLand: (Action: number, Area: number) -> nil,
  MoveToTarget: (Target: vector, Tau: number) -> nil,
  Name2Key: (Name: string) -> uuid,
  NavigateTo: (Location: vector, Options: list) -> nil,
  OffsetTexture: (OffsetS: number, OffsetT: number, Face: number) -> nil,
  OpenFloater: (floater_name: string, url: string, params: list) -> number,
  OpenRemoteDataChannel: () -> nil,
  Ord: (value: string, index: number) -> number,
  OverMyLand: (ID: uuid) -> boolean,
  OwnerSay: (Text: string) -> nil,
  ParcelMediaCommandList: (CommandList: list) -> nil,
  ParcelMediaQuery: (QueryList: list) -> list,
  ParseString2List: (Text: string, Separators: {string}, Spacers: {string}) -> {string},
  ParseStringKeepNulls: (Text: string, Separators: {string}, Spacers: {string}) -> {string},
  ParticleSystem: (Parameters: list) -> nil,
  PassCollisions: (Pass: numeric) -> nil,
  PassTouches: (Pass: numeric) -> nil,
  PatrolPoints: (Points: list, Options: list) -> nil,
  PlaySound: (Sound: string, Volume: number) -> nil,
  PlaySoundSlave: (Sound: string, Volume: number) -> nil,
  Pow: (Value: number, Exponent: number) -> number,
  PreloadSound: (Sound: string) -> nil,
  Pursue: (TargetID: uuid, Options: list) -> nil,
  PushObject: (ObjectID: uuid, Impulse: vector, AngularImpulse: vector, Local: number) -> nil,
  ReadKeyValue: (Key: string) -> uuid,
  RefreshPrimURL: () -> nil,
  RegionSay: (Channel: number, Text: string) -> nil,
  RegionSayTo: (TargetID: uuid, Channel: number, Text: string) -> nil,
  ReleaseCamera: (AvatarID: uuid) -> nil,
  ReleaseControls: () -> nil,
  ReleaseURL: (URL: string) -> nil,
  RemoteDataReply: (ChannelID: uuid, MessageID: uuid, sData: string, iData: number) -> nil,
  RemoteDataSetRegion: () -> nil,
  RemoteLoadScriptPin: (ObjectID: uuid, ScriptName: string, PIN: number, Running: numeric, StartParameter: number) -> nil,
  RemoveFromLandBanList: (AvatarID: uuid) -> nil,
  RemoveFromLandPassList: (AvatarID: uuid) -> nil,
  RemoveInventory: (InventoryItem: string) -> nil,
  RemoveVehicleFlags: (Vehiclelags: number) -> nil,
  ReplaceAgentEnvironment: (agent_id: uuid, transition: number, environment: string) -> number,
  ReplaceEnvironment: (position: vector, environment: string, track_no: number, day_length: number, day_offset: number) -> number,
  ReplaceSubString: (InitialString: string, SubString: string, NewSubString: string, Count: number) -> string,
  RequestAgentData: (AvatarID: uuid, Data: number) -> uuid,
  RequestDisplayName: (AvatarID: uuid) -> uuid,
  RequestExperiencePermissions: (AgentID: uuid, unused: string) -> nil,
  RequestInventoryData: (InventoryItem: string) -> uuid,
  RequestPermissions: (AvatarID: uuid, PermissionMask: number) -> nil,
  RequestSecureURL: () -> uuid,
  RequestSimulatorData: (RegionName: string, Data: number) -> uuid,
  RequestURL: () -> uuid,
  RequestUserKey: (Name: string) -> uuid,
  RequestUsername: (AvatarID: uuid) -> uuid,
  ResetAnimationOverride: (AnimationState: string) -> nil,
  ResetLandBanList: () -> nil,
  ResetLandPassList: () -> nil,
  ResetOtherScript: (ScriptName: string) -> nil,
  ResetScript: () -> nil,
  ResetTime: () -> nil,
  ReturnObjectsByID: (ObjectIDs: list) -> number,
  ReturnObjectsByOwner: (ID: uuid, Scope: number) -> number,
  RezAtRoot: (InventoryItem: string, Position: vector, Velocity: vector, Rotation: quaternion, StartParameter: number) -> nil,
  RezObject: (InventoryItem: string, Position: vector, Velocity: vector, Rotation: quaternion, StartParameter: number) -> nil,
  RezObjectWithParams: (InventoryItem: string, Parms: list) -> uuid,
  Rot2Angle: (Rotation: quaternion) -> number,
  Rot2Axis: (Rotation: quaternion) -> vector,
  Rot2Euler: (Rotation: quaternion) -> vector,
  Rot2Fwd: (Rotation: quaternion) -> vector,
  Rot2Left: (Rotation: quaternion) -> vector,
  Rot2Up: (Rotation: quaternion) -> vector,
  RotBetween: (Vector1: vector, Vector2: vector) -> quaternion,
  RotLookAt: (Rotation: quaternion, Strength: number, Damping: number) -> nil,
  RotTarget: (Rotation: quaternion, LeeWay: number) -> number,
  RotTargetRemove: (Handle: number) -> nil,
  RotateTexture: (Radians: number, Face: number) -> nil,
  Round: (Value: number) -> number,
  SHA1String: (Text: string) -> string,
  SHA256String: (text: string) -> string,
  SameGroup: (ID: uuid) -> boolean,
  Say: (Channel: number, Text: string) -> nil,
  ScaleByFactor: (ScalingFactor: number) -> boolean,
  ScaleTexture: (Horizontal: number, Vertical: number, Face: number) -> nil,
  ScriptDanger: (Position: vector) -> boolean,
  ScriptProfiler: (State: number) -> nil,
  SendRemoteData: (ChannelID: uuid, Destination: string, Value: number, Text: string) -> uuid,
  Sensor: (Name: string, ID: uuid, Type: number, Range: number, Arc: number) -> nil,
  SensorRemove: () -> nil,
  SensorRepeat: (Name: string, ID: uuid, Type: number, Range: number, Arc: number, Rate: number) -> nil,
  SetAgentEnvironment: (agent_id: uuid, transition: number, Settings: list) -> number,
  SetAgentRot: (rot: quaternion, flags: number) -> nil,
  SetAlpha: (Opacity: number, Face: number) -> nil,
  SetAngularVelocity: (AngVel: vector, Local: number) -> nil,
  SetAnimationOverride: (AnimationState: string, AnimationName: string) -> nil,
  SetBuoyancy: (Buoyancy: number) -> nil,
  SetCameraAtOffset: (Offset: vector) -> nil,
  SetCameraEyeOffset: (Offset: vector) -> nil,
  SetCameraParams: (Parameters: list) -> nil,
  SetClickAction: (Action: number) -> nil,
  SetColor: (Color: vector, Face: number) -> nil,
  SetContentType: (HTTPRequestID: uuid, ContentType: number) -> nil,
  SetDamage: (Damage: number) -> nil,
  SetEnvironment: (Position: vector, EnvParams: list) -> number,
  SetForce: (Force: vector, Local: numeric) -> nil,
  SetForceAndTorque: (Force: vector, Torque: vector, Local: numeric) -> nil,
  SetGroundTexture: (Changes: list) -> number,
  SetHoverHeight: (Height: number, Water: numeric, Tau: number) -> nil,
  SetInventoryPermMask: (InventoryItem: string, PermissionFlag: number, PermissionMask: number) -> nil,
  SetKeyframedMotion: (Keyframes: list, Options: list) -> nil,
  SetLinkAlpha: (LinkNumber: number, Opacity: number, Face: number) -> nil,
  SetLinkCamera: (LinkNumber: number, EyeOffset: vector, LookOffset: vector) -> nil,
  SetLinkColor: (LinkNumber: number, Color: vector, Face: number) -> nil,
  SetLinkGLTFOverrides: (link: number, face: number, options: list) -> nil,
  SetLinkMedia: (Link: number, Face: number, Parameters: list) -> number,
  SetLinkPrimitiveParams: (LinkNumber: number, Parameters: list) -> nil,
  SetLinkPrimitiveParamsFast: (LinkNumber: number, Parameters: list) -> nil,
  SetLinkRenderMaterial: (LinkNumber: number, RenderMaterial: string, Face: number) -> nil,
  SetLinkSitFlags: (LinkNumber: number, Flags: number) -> nil,
  SetLinkTexture: (LinkNumber: number, Texture: string, Face: number) -> nil,
  SetLinkTextureAnim: (LinkNumber: number, Mode: number, Face: number, SizeX: number, SizeY: number, Start: number, Length: number, Rate: number) -> nil,
  SetLocalRot: (Rotation: quaternion) -> nil,
  SetMemoryLimit: (Limit: number) -> boolean,
  SetObjectDesc: (Description: string) -> nil,
  SetObjectName: (Name: string) -> nil,
  SetObjectPermMask: (PermissionFlag: number, PermissionMask: number) -> nil,
  SetParcelForSale: (ForSale: numeric, Options: list) -> boolean,
  SetParcelMusicURL: (URL: string) -> nil,
  SetPayPrice: (Price: number, QuickButtons: list) -> nil,
  SetPhysicsMaterial: (MaterialBits: number, GravityMultiplier: number, Restitution: number, Friction: number, Density: number) -> nil,
  SetPos: (Position: vector) -> nil,
  SetPrimMediaParams: (Face: number, MediaParameters: list) -> number,
  SetPrimURL: (URL: string) -> nil,
  SetPrimitiveParams: (Parameters: list) -> nil,
  SetRegionPos: (Position: vector) -> boolean,
  SetRemoteScriptAccessPin: (PIN: number) -> nil,
  SetRenderMaterial: (Material: string, Face: number) -> nil,
  SetRot: (Rotation: quaternion) -> nil,
  SetScale: (Scale: vector) -> nil,
  SetScriptState: (ScriptName: string, Running: numeric) -> nil,
  SetSitText: (Text: string) -> nil,
  SetSoundQueueing: (QueueEnable: numeric) -> nil,
  SetSoundRadius: (Radius: number) -> nil,
  SetStatus: (Status: number, Value: numeric) -> nil,
  SetText: (Text: string, Color: vector, Opacity: number) -> nil,
  SetTexture: (Texture: string, Face: number) -> nil,
  SetTextureAnim: (Mode: number, Face: number, SizeX: number, SizeY: number, Start: number, Length: number, Rate: number) -> nil,
  SetTimerEvent: (Rate: number) -> nil,
  SetTorque: (Torque: vector, Local: numeric) -> nil,
  SetTouchText: (Text: string) -> nil,
  SetVehicleFlags: (Flags: number) -> nil,
  SetVehicleFloatParam: (ParameterName: number, ParameterValue: number) -> nil,
  SetVehicleRotationParam: (ParameterName: number, ParameterValue: quaternion) -> nil,
  SetVehicleType: (Type: number) -> nil,
  SetVehicleVectorParam: (ParameterName: number, ParameterValue: vector) -> nil,
  SetVelocity: (Velocity: vector, Local: number) -> nil,
  Shout: (Channel: number, Text: string) -> nil,
  SignRSA: (PrivateKey: string, Message: string, Algorithm: string) -> string,
  Sin: (Theta: number) -> number,
  SitOnLink: (AvatarID: uuid, LinkID: number) -> number,
  SitTarget: (Offset: vector, Rotation: quaternion) -> nil,
  Sleep: (Time: number) -> nil,
  Sound: (Sound: string, Volume: number, Queue: number, Loop: number) -> nil,
  SoundPreload: (Sound: string) -> nil,
  Sqrt: (Value: number) -> number,
  StartAnimation: (Animation: string) -> nil,
  StartObjectAnimation: (Animation: string) -> nil,
  StopAnimation: (Animation: string) -> nil,
  StopHover: () -> nil,
  StopLookAt: () -> nil,
  StopMoveToTarget: () -> nil,
  StopObjectAnimation: (Animation: string) -> nil,
  StopSound: () -> nil,
  StringLength: (Text: string) -> number,
  StringToBase64: (Text: string) -> string,
  StringTrim: (Text: string, TrimType: number) -> string,
  SubStringIndex: (Text: string, Sequence: string) -> number,
  TakeCamera: (AvatarID: uuid) -> nil,
  TakeControls: (Controls: number, Accept: numeric, PassOn: numeric) -> nil,
  Tan: (Theta: number) -> number,
  Target: (Position: vector, Range: number) -> number,
  TargetOmega: (Axis: vector, SpinRate: number, Gain: number) -> nil,
  TargetRemove: (Target: number) -> nil,
  TargetedEmail: (Target: number, Subject: string, Text: string) -> nil,
  TeleportAgent: (AvatarID: uuid, LandmarkName: string, Position: vector, LookAtPoint: vector) -> nil,
  TeleportAgentGlobalCoords: (AvatarID: uuid, GlobalPosition: vector, RegionPosition: vector, LookAtPoint: vector) -> nil,
  TeleportAgentHome: (AvatarID: uuid) -> nil,
  TextBox: (AvatarID: uuid, Text: string, Channel: number) -> nil,
  ToLower: (Text: string) -> string,
  ToUpper: (Text: string) -> string,
  TransferLindenDollars: (AvatarID: uuid, Amount: number) -> uuid,
  TransferOwnership: (AgentID: uuid, Flags: number, Params: list) -> number,
  TriggerSound: (Sound: string, Volume: number) -> nil,
  TriggerSoundLimited: (Sound: string, Volume: number, TNE: vector, BSW: vector) -> nil,
  UnSit: (AvatarID: uuid) -> nil,
  UnescapeURL: (URL: string) -> string,
  UpdateCharacter: (Options: list) -> nil,
  UpdateKeyValue: (Key: string, Value: string, Checked: number, OriginalValue: string) -> uuid,
  VecDist: (Location1: vector, Location2: vector) -> number,
  VecMag: (Vector: vector) -> number,
  VecNorm: (Vector: vector) -> vector,
  VerifyRSA: (PublicKey: string, Message: string, Signature: string, Algorithm: string) -> boolean,
  VolumeDetect: (DetectEnabled: numeric) -> nil,
  WanderWithin: (Origin: vector, Area: vector, Options: list) -> nil,
  Water: (Offset: vector) -> number,
  Whisper: (Channel: number, Text: string) -> nil,
  Wind: (Offset: vector) -> vector,
  WorldPosToHUD: (world_pos: vector) -> vector,
  XorBase64: (Text1: string, Text2: string) -> string,
  XorBase64Strings: (Text1: string, Text2: string) -> string,
  XorBase64StringsCorrect: (Text1: string, Text2: string) -> string,
  sRGB2Linear: (srgb: vector) -> vector,
}


---------------------------
-- Global Table: llcompat
---------------------------

declare llcompat: {
  AdjustDamage: (Number: number, Damage: number) -> nil,
  DetectedDamage: (Number: number) -> list,
  DetectedGrab: (Number: number) -> vector,
  DetectedGroup: (Number: number) -> boolean,
  DetectedKey: (Number: number) -> uuid,
  DetectedLinkNumber: (Number: number) -> number,
  DetectedName: (Number: number) -> string,
  DetectedOwner: (Number: number) -> uuid,
  DetectedPos: (Number: number) -> vector,
  DetectedRezzer: (Number: number) -> uuid,
  DetectedRot: (Number: number) -> quaternion,
  DetectedTouchBinormal: (Index: number) -> vector,
  DetectedTouchFace: (Index: number) -> number,
  DetectedTouchNormal: (Index: number) -> vector,
  DetectedTouchPos: (Index: number) -> vector,
  DetectedTouchST: (Index: number) -> vector,
  DetectedTouchUV: (Index: number) -> vector,
  DetectedType: (Number: number) -> number,
  DetectedVel: (Number: number) -> vector,
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
declare default: any
