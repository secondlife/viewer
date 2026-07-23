# Viewer to External Editor JSON-RPC<br>Message Interfaces Documentation

This document describes all the message interfaces defined for WebSocket communication between the Second Life viewer and an external editor such as a VSCode extension.

## Table of Contents

- [Usage Flow](#usage-flow)
- [VS Code Launch URI](#vs-code-launch-uri)
- [JSON-RPC Method Summary](#json-rpc-method-summary)
- [Session Management Interfaces](#session-management-interfaces)
  - [SessionHandshake](#sessionhandshake)
  - [SessionHandshakeResponse](#sessionhandshakeresponse)
  - [Session OK](#session-ok)
  - [SessionDisconnect](#sessiondisconnect)
  - [SessionPing](#sessionping)
- [Language and Syntax Interfaces](#language-and-syntax-interfaces)
  - [SyntaxChange](#syntaxchange)
  - [Language Syntax ID Request](#language-syntax-id-request)
  - [Language Syntax Request](#language-syntax-request)
  - [Language Syntax Cache List](#language-syntax-cache-list)
  - [Language Syntax Cache Get](#language-syntax-cache-get)
- [Script Subscription Interfaces](#script-subscription-interfaces)
  - [ScriptSubscribe](#scriptsubscribe)
  - [ScriptSubscribeResponse](#scriptsubscriberesponse)
  - [ScriptUnsubscribe](#scriptunsubscribe)
  - [ScriptList](#scriptlist)
- [Compilation Interfaces](#compilation-interfaces)
  - [CompilationError](#compilationerror)
  - [CompilationResult](#compilationresult)
- [Runtime Event Interfaces](#runtime-event-interfaces)
  - [RuntimeDebug](#runtimedebug)
  - [RuntimeError](#runtimeerror)
- [Handler and Configuration Interfaces](#handler-and-configuration-interfaces)
  - [WebSocketHandlers](#websockethandlers)
  - [ClientInfo](#clientinfo)
- [Object Content Interfaces](#object-content-interfaces)
  - [Core Data Types](#core-data-types)
  - [ObjectPublish](#objectpublish)
  - [ObjectUnpublish](#objectunpublish)
  - [ObjectUpdate](#objectupdate)
  - [ObjectContentGet](#objectcontentget)
  - [ObjectContentSave](#objectcontentsave)
  - [ObjectItemCreate](#objectitemcreate)
  - [ObjectItemDelete](#objectitemdelete)
  - [ObjectScriptSetRunning](#objectscriptsetrunning)
  - [ObjectRequest](#objectrequest)
  - [ObjectModify](#objectmodify)
  - [ObjectItemModify](#objectitemmodify)

## Usage Flow

1. **Connection Establishment:**

   - Viewer sends `session.handshake` call with `SessionHandshake` data
   - Extension responds with `SessionHandshakeResponse`
   - Viewer confirms with `session.ok` notification

2. **Language Information Exchange:**

   - Extension makes `language.syntax.id` call to get current syntax version
   - Extension makes `language.syntax` calls with different `kind` parameters to get specific language data
   - Viewer responds with a `LanguageInfo` object containing the requested definitions

3. **Script Subscription Management:**

   - Extension makes `script.subscribe` call with `ScriptSubscribe` data to request live synchronization for a script
   - Viewer responds with `ScriptSubscribeResponse` indicating success or failure
   - When subscription needs to be terminated, viewer sends `script.unsubscribe` notification with `ScriptUnsubscribe` data
   - Extension handles unsubscription by cleaning up local script tracking

4. **Object Content Publishing:**

   - Viewer sends `object.publish` notification when an in-world object's contents are made available for editing
   - Viewer sends `object.unpublish` notification when an object is removed or the owner stops publishing
   - Viewer sends `object.update` notification when object inventory changes (full replacement or delta)
   - Extension calls `object.content.get` to fetch an item's content on demand
   - Extension calls `object.content.save` to write modified content back to the viewer
   - Extension calls `object.item.create` / `object.item.delete` to manage inventory items
   - Extension calls `object.script.set_running` to start or stop a script

5. **Runtime Events:**

   - Viewer sends `language.syntax.change` notification with `SyntaxChange` when language changes
   - Viewer sends `script.compiled` notification with `CompilationResult` after script compilation
   - Viewer sends `runtime.debug` notification with `RuntimeDebug` for debug messages during script execution
   - Viewer sends `runtime.error` notification with `RuntimeError` when runtime errors occur

6. **Connection Termination:**
   - Either side can send `session.disconnect` notification with `SessionDisconnect` data
   - Connection is closed gracefully

## VS Code Launch URI

The viewer can launch VS Code and trigger an automatic WebSocket connection by opening a `vscode://` URI via the operating system's default URI handler. The extension registers a URI handler for this scheme; VS Code will launch itself if not already running and deliver the URI to the extension.

### URI Format

```
vscode://lindenlab.sl-vscode-plugin/connect[?port=<port>][&object=<uuid>][&script=<uuid>]
```

### Parameters

| Parameter | Required | Description |
| --------- | -------- | ----------- |
| `port`    | No       | Port number the viewer's WebSocket server is listening on. Overrides the user's configured port for this session. Defaults to the configured `slVscodeEdit.network.websocketPort` (default `9020`) if absent. Must be in range 1024-65535. |
| `object`  | No       | UUID of a root prim. After the handshake completes the extension calls `object.request` to ask the viewer to publish this object. The viewer then sends an `object.publish` notification and the object appears as a workspace folder in the Explorer. |
| `script`  | No       | UUID of a script. After the handshake completes the extension locates the corresponding temp file via `script.list` and opens it, triggering the normal `script.subscribe` + live-sync flow. |

`object` and `script` are mutually exclusive in typical use but both may be supplied; the extension will process both.

### Examples

```
# Open VS Code and connect on default port
vscode://lindenlab.sl-vscode-plugin/connect

# Connect on a custom port
vscode://lindenlab.sl-vscode-plugin/connect?port=9021

# Connect and immediately publish a specific object
vscode://lindenlab.sl-vscode-plugin/connect?port=9020&object=550e8400-e29b-41d4-a716-446655440000

# Connect and open a specific script for editing
vscode://lindenlab.sl-vscode-plugin/connect?port=9020&script=6ba7b810-9dad-11d1-80b4-00c04fd430c8
```

### Post-connection sequence

When the URI contains an `object` or `script` parameter the extension acts only **after** the handshake is fully complete (`session.ok` received):

```
URI received by extension
  |
  v
WebSocket connects -> session.handshake -> session.ok
  |
  |- object=<uuid> -> object.request({ object_id }) call
  |                       |
  |                       v  (async, when viewer is ready)
  |                  object.publish notification
  |
  \- script=<uuid> -> script.list call -> open temp file
                |
                v
                                          script.subscribe + live-sync
```

## JSON-RPC Method Summary

| Method                          | Direction          | Type         | Interface/Parameters       |
| ------------------------------- | ------------------ | ------------ | -------------------------- |
| `session.handshake`             | Viewer -> Extension | Call         | `SessionHandshake`         |
| `session.handshake` (response)  | Extension -> Viewer | Response     | `SessionHandshakeResponse` |
| `session.ok`                    | Viewer -> Extension | Notification | _(no interface)_           |
| `session.disconnect`            | Bidirectional      | Notification | `SessionDisconnect`        |
| `session.ping`                  | Bidirectional      | Call         | `SessionPing`              |
| `session.ping` (response)       | Bidirectional      | Response     | `SessionPingResponse`      |
| `script.subscribe`              | Extension -> Viewer | Call         | `ScriptSubscribe`          |
| `script.subscribe` (response)   | Viewer -> Extension | Response     | `ScriptSubscribeResponse`  |
| `script.unsubscribe`            | Viewer -> Extension | Notification | `ScriptUnsubscribe`        |
| `script.list`                   | Extension -> Viewer | Call         | _(no parameters)_          |
| `script.list` (response)        | Viewer -> Extension | Response     | `ScriptList`               |
| `language.syntax.id`            | Extension -> Viewer | Call         | _(no parameters)_          |
| `language.syntax.id` (response) | Viewer -> Extension | Response     | `{ id: string }`           |
| `language.syntax`               | Extension -> Viewer | Call         | `{ kind: string }`         |
| `language.syntax` (response)    | Viewer -> Extension | Response     | `LanguageInfo`             |
| `language.syntax.cache`            | Extension -> Viewer | Call         | _(no parameters)_                    |
| `language.syntax.cache` (response) | Viewer -> Extension | Response     | `SyntaxCacheList`                    |
| `language.syntax.get`              | Extension -> Viewer | Call         | `{ filename: string, as_json?: boolean }` |
| `language.syntax.get` (response)   | Viewer -> Extension | Response     | `SyntaxCacheFile`                    |
| `language.syntax.change`        | Viewer -> Extension | Notification | `SyntaxChange`             |
| `script.compiled`               | Viewer -> Extension | Notification | `CompilationResult`        |
| `runtime.debug`                 | Viewer -> Extension | Notification | `RuntimeDebug`             |
| `runtime.error`                 | Viewer -> Extension | Notification | `RuntimeError`             |
| `object.publish`                | Viewer -> Extension | Notification | `ObjectPublishMessage`     |
| `object.unpublish`              | Viewer -> Extension | Notification | `ObjectUnpublishMessage`   || `object.unpublish`              | Extension → Viewer | Call         | `ObjectUnpublishParams`    |
| `object.unpublish` (response)   | Viewer → Extension | Response     | `ObjectUnpublishResponse`  || `object.update`                 | Viewer -> Extension | Notification | `ObjectUpdateMessage`      |
| `object.content.get`            | Extension -> Viewer | Call         | `ObjectContentGetParams`   |
| `object.content.get` (response) | Viewer -> Extension | Response     | `ObjectContentGetResponse` |
| `object.content.save`           | Extension -> Viewer | Call         | `ObjectContentSaveParams`  |
| `object.content.save` (response)| Viewer -> Extension | Response     | `ObjectContentSaveResponse`|
| `object.item.create`            | Extension -> Viewer | Call         | `ObjectItemCreateParams`   |
| `object.item.create` (response) | Viewer -> Extension | Response     | `ObjectItemCreateResponse` |
| `object.item.delete`            | Extension -> Viewer | Call         | `ObjectItemDeleteParams`   |
| `object.item.delete` (response) | Viewer -> Extension | Response     | `ObjectItemDeleteResponse` |
| `object.script.set_running`     | Extension -> Viewer | Call         | `ObjectScriptSetRunningParams` |
| `object.script.set_running` (response) | Viewer -> Extension | Response | `ObjectScriptSetRunningResponse` |
| `object.script.reset`           | Extension -> Viewer | Call         | `ObjectScriptResetParams`        |
| `object.script.reset` (response)| Viewer -> Extension | Response     | `ObjectScriptResetResponse`      |
| `object.request`                | Extension -> Viewer | Call         | `ObjectRequestParams`          |
| `object.request` (response)     | Viewer -> Extension | Response     | `ObjectRequestResponse`        |
| `object.list`                   | Extension -> Viewer | Call         | `{}` (no params)               |
| `object.list` (response)        | Viewer -> Extension | Response     | `ObjectListResponse`           |
| `object.modify`                 | Extension -> Viewer | Call         | `ObjectModifyParams`           |
| `object.modify` (response)      | Viewer -> Extension | Response     | `ObjectModifyResponse`         |
| `object.item.modify`            | Extension -> Viewer | Call         | `ObjectItemModifyParams`       |
| `object.item.modify` (response) | Viewer -> Extension | Response     | `ObjectItemModifyResponse`     |

## Session Management Interfaces

### SessionHandshake

**JSON-RPC Method:** `session.handshake` (call from viewer)

The initial handshake call sent by the viewer to establish a session.

```typescript
interface SessionHandshake {
  server_version: "1.0.0";
  protocol_version: "1.0";
  viewer_name: string;
  viewer_version: string;
  agent_id: string;
  agent_name: string;
  challenge?: string;
  languages: string[];
  syntax_id: string;
  features: { [feature: string]: boolean };
}
```

**Fields:**

- `server_version`: Fixed version "1.0.0" indicating the server API version
- `protocol_version`: Fixed version "1.0" for the communication protocol
- `viewer_name`: Name of the Second Life viewer application
- `viewer_version`: Version string of the viewer
- `agent_id`: Unique identifier for the user/agent
- `agent_name`: Human-readable name of the agent
- `challenge` (optional): Path to a temporary file on the local filesystem containing a UUID. The client must read this file and return the UUID as `challenge_response` to authenticate the connection.
- `languages`: Array of supported scripting languages (e.g., `["lsl", "luau"]`)
- `syntax_id`: Current active syntax identifier as a UUID string
- `features`: Dictionary of feature flags indicating viewer capabilities. Known flags:
  - `live_sync`: Viewer supports live script synchronisation with the external editor
  - `compilation`: Viewer will forward compilation results via `script.compiled`
  - `syntax_cache`: Viewer supports `language.syntax.cache` and `language.syntax.get` for retrieving syntax definition files

### SessionHandshakeResponse

**JSON-RPC Method:** Response to `session.handshake`

The response sent by the VS Code extension to complete the handshake.

```typescript
interface SessionHandshakeResponse {
  client_name: string;
  client_version: "1.0";
  protocol_version: string;
  challenge_response?: string;
  languages: string[];
  features: { [feature: string]: boolean };
  script_name?: string;
  script_language?: string;
}
```

**Fields:**

- `client_name`: Name of the client (VS Code extension)
- `client_version`: Fixed version "1.0" of the client
- `protocol_version`: Protocol version the client supports
- `challenge_response` (optional): The UUID read from the temporary file identified by the `challenge` field in the handshake. Must be provided if `challenge` was present, otherwise the connection will be closed.
- `languages`: Array of languages supported by the client
- `features`: Dictionary of features supported by the client
- `script_name` (optional): Name of the script currently open in the editor
- `script_language` (optional): Language of the script currently open in the editor (e.g. `"lsl"`, `"luau"`)

### Session OK

**JSON-RPC Method:** `session.ok` (notification from viewer)

Confirmation notification sent by the viewer after successful handshake completion. No parameters are sent with this notification.

### SessionDisconnect

**JSON-RPC Method:** `session.disconnect` (notification, bidirectional)

Message sent when terminating the connection.

```typescript
interface SessionDisconnect {
  reason: number;
  message: string;
}
```

**Fields:**

- `reason`: Numeric code indicating the reason for disconnection:
  - `0`: Normal closure
  - `1`: Editor closed
  - `2`: Protocol error
  - `3`: Connection timeout
  - `4`: Internal server error
- `message`: Human-readable description of the disconnect reason

### SessionPing

**JSON-RPC Method:** `session.ping` (call, bidirectional)

Heartbeat call used to verify the connection is alive and measure latency. Either side can initiate a ping; the recipient responds with the original timestamp plus its own server time.

```typescript
interface SessionPing {
  timestamp: number;
}
```

**Fields:**

- `timestamp`: Unix timestamp in milliseconds when the ping was sent

**Response:**

```typescript
interface SessionPingResponse {
  timestamp: number;
  server_time: number;
}
```

**Response Fields:**

- `timestamp`: The original timestamp from the request (echoed back)
- `server_time`: Unix timestamp in milliseconds when the response was generated

**Example Request:**

```json
{
  "jsonrpc": "2.0",
  "method": "session.ping",
  "id": 42,
  "params": {
    "timestamp": 1721145600000
  }
}
```

**Example Response:**

```json
{
  "jsonrpc": "2.0",
  "id": 42,
  "result": {
    "timestamp": 1721145600000,
    "server_time": 1721145600015
  }
}
```

## Language and Syntax Interfaces

### SyntaxChange

**JSON-RPC Method:** `language.syntax.change` (notification from viewer)

Notification sent when the active language syntax changes in the viewer.

```typescript
interface SyntaxChange {
  id: string;
}
```

**Fields:**

- `id`: UUID string identifying the new syntax version

### Language Syntax ID Request

**JSON-RPC Method:** `language.syntax.id` (call from extension to viewer)

Requests the current active language syntax identifier from the viewer. This method takes no parameters.

**Response:** Returns `{ id: string }` where `id` is the current syntax version as a UUID string.

### Language Syntax Request

**JSON-RPC Method:** `language.syntax` (call from extension to viewer)

Requests the in-memory keyword definitions for a specific language. These definitions are the deserialized, viewer-processed form of the syntax data for the current region.

**Parameters:**

```typescript
{
  kind: string; // The language whose definitions to retrieve
}
```

**Valid `kind` values:**

| Value | Description |
| ----------- | ----------------------------------------- |
| `"defs.lsl"` | Returns the LSL keyword definitions |
| `"defs.lua"` | Returns the Luau keyword definitions |

**Response:**

```typescript
interface LanguageInfo {
  id: string;
  defs?: object;    // Present only on success
  success: boolean;
  error?: string;   // Present only on failure
}
```

**Response Fields:**

- `id`: The current syntax version identifier
- `defs` (optional): The keyword definitions object. Only present when `success` is `true`. Structure varies by language.
- `success`: Whether the definitions were found and returned successfully
- `error` (optional): Human-readable error description. Only present when `success` is `false`

**Error cases:**

- No `kind` parameter supplied: `success: false`, `error: "No syntax category specified"`
- Unknown `kind` value: `success: false`, `error: "Unknown syntax category requested"`

### Language Syntax Cache List

**JSON-RPC Method:** `language.syntax.cache` (call from extension to viewer)

Requests the list of file names currently held in the `LLSyntaxDefCache`. This provides the extension with the available syntax definition file names that can subsequently be retrieved with `language.syntax.get`. This method takes no parameters.

**Response:**

```typescript
interface SyntaxCacheList {
  files: string[];  // Array of file names (e.g. ["lsl_keywords.xml", "slua_definitions.yaml"])
  success: boolean;
}
```

**Response Fields:**

- `files`: Array of file name strings, each of which can be passed as the `filename` parameter to `language.syntax.get`
- `success`: Whether the request was handled successfully

**Known cache files:**

| File name | Description |
| -------------------------------- | ---------------------------------------------------- |
| `builtins.txt`                   | LSL built-in keyword list in plain text format |
| `lsl_definitions.yaml`           | LSL language definitions in YAML format |
| `lsl_keywords.xml`               | LSL keyword definitions in LLSD XML format. Used by the viewer's script editor |
| `lsl_keywords_pretty.xml`        | LSL keyword definitions in formatted LLSD XML format |
| `secondlife.d.luau`              | Luau type definition file. Used by luau-lsp |
| `secondlife.docs.json`           | Luau documentation data in JSON format. Used by luau-lsp |
| `slua_definitions.yaml`          | Luau language definitions in YAML format |
| `lua_keywords.xml`               | Luau keyword definitions in LLSD XML format. Used by the viewer's script editor |
| `lua_keywords_pretty.xml`        | Luau keyword definitions in formatted LLSD XML format |
| `secondlife_selene.yml`          | Luau Selene linter configuration in YAML format |

Not all files may be present in every cache - the actual list returned by `language.syntax.cache` reflects only what is available on the viewer's local filesystem at the time of the request.

### Language Syntax Cache Get

**JSON-RPC Method:** `language.syntax.get` (call from extension to viewer)

Requests the content of a specific file from the syntax definition cache. The file name must be one of the names returned by a prior `language.syntax.cache` call. Content is returned either as a raw text string or as a parsed JSON/LLSD object depending on the `as_json` parameter.

**Parameters:**

```typescript
{
  filename: string;   // The file name to retrieve, as returned by language.syntax.cache
  as_json?: boolean;  // Optional. If true, content is returned as a parsed object rather than raw text
}
```

**Fields:**

- `filename`: The file name to retrieve (e.g. `"lsl_keywords.xml"`, `"slua_definitions.yaml"`)
- `as_json` (optional): When `true`, the file is deserialized and returned as a structured object in `content`. When omitted or `false`, `content` is the raw text of the file.

**Response:**

```typescript
interface SyntaxCacheFile {
  content?: string | object;  // Present only on success. String if as_json is false/omitted, object if as_json is true
  success: boolean;
  error?: string;             // Present only on failure
}
```

**Response Fields:**

- `content`: The file content. Only present when `success` is `true`. Is a raw text string when `as_json` is omitted or `false`; is a parsed object when `as_json` is `true`.
- `success`: Whether the file was found and read successfully
- `error` (optional): Human-readable error description. Only present when `success` is `false`

**Error cases:**

- No `filename` parameter supplied: `success: false`, `error: "No filename specified"`
- Name not found in cache: `success: false`, `error: "Requested syntax cache file not found"`
- File could not be loaded: `success: false`, `error: "Failed to load syntax cache file"` (or `"Failed to load and format syntax cache file."` when `as_json` is `true`)

## Script Subscription Interfaces

### ScriptSubscribe

**JSON-RPC Method:** `script.subscribe` (call from extension to viewer)

Requests subscription to a script for live synchronization between the editor and viewer.

```typescript
interface ScriptSubscribe {
  script_id: string;
  script_name: string;
  script_language: string;
}
```

**Fields:**

- `script_id`: Unique identifier for the script to subscribe to
- `script_name`: Display name of the script file
- `script_language`: Programming language of the script (e.g., "lsl", "luau")

### ScriptSubscribeResponse

**JSON-RPC Method:** Response to `script.subscribe`

Response from the viewer indicating whether script subscription was successful.

```typescript
interface ScriptSubscribeResponse {
  script_id: string;
  success: boolean;
  status: number;
  object_id?: string;
  item_id?: string;
  message?: string;
}
```

**Fields:**

- `script_id`: The script identifier that was subscribed to
- `success`: Whether the subscription was successful
- `status`: Numeric status code indicating the result:
  - `0`: Success
  - `1`: Invalid editor - the script editor panel is no longer open
  - `2`: Invalid subscription - no subscription found for the given `script_id`
  - `3`: Already subscribed - another connection is already subscribed to this script
  - `4`: Internal server error
- `object_id` (optional): The in-world UUID of the object containing the script
- `item_id` (optional): The inventory item UUID of the script within the object
- `message` (optional): Additional information about the subscription result

### ScriptUnsubscribe

**JSON-RPC Method:** `script.unsubscribe` (notification from viewer)

Notification sent by the viewer when a script subscription should be terminated.

```typescript
interface ScriptUnsubscribe {
  script_id: string;
}
```

**Fields:**

- `script_id`: Unique identifier for the script to unsubscribe from

### ScriptList

**JSON-RPC Method:** `script.list` (call from extension to viewer)

Requests the list of all scripts currently open and tracked by the viewer, along with the viewer's temp directory. This is intended for use by a file watcher tool that needs to discover which script temp files are active without going through the full `script.subscribe` flow. This method takes no parameters.

**Response:**

```typescript
interface ScriptList {
  temp_dir: string;
  script_ids: string[];
  success: boolean;
}
```

**Response Fields:**

- `temp_dir`: The absolute path to the viewer's temp directory where live-sync script files are written. Combined with a `script_id`, the caller can locate the corresponding temp file on disk.
- `script_ids`: Array of script ID strings for all currently subscribed scripts, across all active connections.
- `success`: Always `true`.

## Compilation Interfaces

### CompilationError

Individual compilation error record.

```typescript
interface CompilationError {
  row: number;
  column: number;
  level: string;
  message: string;
  format?: "lsl";  // Present only for LSL compilation errors
}
```

**Fields:**

- `row`: Line number where the error occurred (1-based for both LSL and Luau)
- `column`: Column position of the error (1-based for LSL; always `0` for Luau as the compiler does not provide column information)
- `level`: Compiler severity string (e.g. `"ERROR"`, `"WARNING"`)
- `message`: Error description
- `format` (optional): Present and set to `"lsl"` for LSL compilation errors; absent for Luau errors

### CompilationResult

**JSON-RPC Method:** `script.compiled` (notification from viewer)

Result of a compilation operation in the viewer.

```typescript
interface CompilationResult {
  script_id: string;
  success: boolean;
  running: boolean;
  errors?: CompilationError[];
}
```

**Fields:**

- `script_id`: Unique identifier for the script that was compiled
- `success`: Whether the compilation was successful
- `running`: Whether the compiled script is currently running
- `errors` (optional): Array of compilation errors if any occurred

## Runtime Event Interfaces

### RuntimeDebug

**JSON-RPC Method:** `runtime.debug` (notification from viewer)

Debug message notification sent by the viewer during script execution.

```typescript
interface RuntimeDebug {
  script_id: string;
  object_id: string;
  object_name: string;
  message: string;
}
```

**Fields:**

- `script_id`: Unique identifier for the script generating the debug message
- `object_id`: Unique identifier for the object containing the script
- `object_name`: Human-readable name of the object
- `message`: The debug message content

### RuntimeError

**JSON-RPC Method:** `runtime.error` (notification from viewer)

Runtime error notification sent by the viewer when a script encounters an error during execution.

```typescript
interface RuntimeError {
  script_id: string;
  object_id: string;
  object_name: string;
  message: string;
  error: string;
  line: number;
  stack?: string[];
}
```

**Fields:**

- `script_id`: Unique identifier for the script that encountered the error
- `object_id`: Unique identifier for the object containing the script
- `object_name`: Human-readable name of the object
- `message`: The full raw chat text of the runtime error message as received from the simulator
- `error`: Extracted error description. Currently always an empty string - runtime error extraction from the simulator's multi-message format is not yet fully implemented.
- `line`: Line number where the error occurred. Currently always `0` for the same reason.
- `stack` (optional): Stack trace lines if they could be extracted from the error message

## Handler and Configuration Interfaces

### WebSocketHandlers

Event handler interface for WebSocket events.

```typescript
interface WebSocketHandlers {
  onHandshake?: (message: SessionHandshake) => SessionHandshakeResponse;
  onHandshakeOk?: () => void;
  onDisconnect?: (message: SessionDisconnect) => void;
  onSubscribe?: (message: ScriptSubscribe) => ScriptSubscribeResponse;
  onUnsubscribe?: (message: ScriptUnsubscribe) => void;
  onSyntaxChange?: (message: SyntaxChange) => void;
  onConnectionClosed?: () => void;
  onCompilationResult?: (message: CompilationResult) => void;
  onRuntimeDebug?: (message: RuntimeDebug) => void;
  onRuntimeError?: (message: RuntimeError) => void;
}
```

**Methods:**

- `onHandshake`: Handler for initial handshake message, returns handshake response
- `onHandshakeOk`: Handler called when handshake is successfully completed
- `onDisconnect`: Handler for disconnect notifications
- `onSubscribe`: Handler called when the extension sends a `script.subscribe` request, returns subscription response
- `onUnsubscribe`: Handler for script unsubscription notifications from viewer
- `onSyntaxChange`: Handler for syntax change notifications
- `onConnectionClosed`: Handler called when connection is closed
- `onCompilationResult`: Handler for compilation result notifications
- `onRuntimeDebug`: Handler for runtime debug message notifications
- `onRuntimeError`: Handler for runtime error notifications

### ClientInfo

Client information used in handshake responses.

```typescript
interface ClientInfo {
  scriptName: string;
  scriptId: string;
  extension: string;
}
```

**Fields:**

- `scriptName`: Name of the script being edited
- `scriptId`: Unique identifier for the script
- `extension`: File extension or script type

---

## Object Content Interfaces

These interfaces support publishing in-world object inventories (scripts and notecards) to the external editor as a browseable virtual filesystem. The extension exposes published objects under the `sl://objects/` URI scheme.

### Core Data Types

```typescript
type InventoryItemType = "script" | "notecard";

type ScriptVM = "lsl2" | "mono" | "luau";

/** Permission mask fields. Only owner and next_owner are transmitted. */
interface ItemPermissions {
  owner: number;       // e.g. PERM_MODIFY=0x4000, PERM_COPY=0x8000, PERM_TRANSFER=0x2000
  next_owner: number;
}

/**
 * Inventory item within an object or linked prim.
 * asset_id is intentionally never transmitted.
 */
interface ObjectInventoryItem {
  item_id: string;         // Inventory item UUID
  name: string;            // Display name (no file extension)
  description?: string;
  type: InventoryItemType;
  subtype?: number;        // Scripts only: language from II_FLAGS_SUBTYPE_MASK (0=LSL, 1=Luau)
  vm?: ScriptVM;           // Scripts only: which VM the script targets
  running?: boolean;       // Scripts only: whether the script is running
  faulted?: boolean;       // Scripts only: whether the script has a runtime fault
  permissions?: ItemPermissions;
  creator_id?: string;
}

/** A linked (child) prim within a linkset */
interface LinkedObject {
  link_id: string;          // UUID of the linked prim
  link_number: number;      // Link number (root=1, children>=2)
  link_name: string;
  link_description?: string;
  inventory: ObjectInventoryItem[];
}

interface ObjectPermissions {
  owner: number;
  next_owner: number;
}

/** Root of a linkset, as published to the extension */
interface PublishedObject {
  object_id: string;          // UUID of the root prim
  object_name: string;
  object_description?: string;
  region?: string;
  owner_id?: string;
  permissions?: ObjectPermissions;
  inventory: ObjectInventoryItem[];      // Root prim's scripts and notecards
  linked_objects?: LinkedObject[];       // Child prims
}
```

**Script display extensions** (synthetic, derived from `subtype`):

| `subtype` | Extension |
| --------- | --------- |
| `0` (LSL) | `.lsl`    |
| `1` (Luau)| `.luau`   |
| notecard  | `.txt`    |

---

### ObjectPublish

**JSON-RPC Method:** `object.publish` (notification from viewer)

Sent when the viewer publishes an in-world object's inventory for external editing. Triggers creation of a virtual filesystem workspace folder in the extension.

```typescript
interface ObjectPublishMessage {
  object: PublishedObject;
}
```

**Fields:**

- `object`: The full published object tree, including root prim inventory and all linked prim inventories.

---

### ObjectUnpublish

**JSON-RPC Method:** `object.unpublish` (notification from viewer)

Sent when the viewer removes a previously published object - for example when the owner deselects it, moves away, or the object is deleted.

```typescript
interface ObjectUnpublishMessage {
  object_id: string;
  reason?: string;
}
```

**Fields:**

- `object_id`: UUID of the root prim that is being unpublished
- `reason` (optional): Human-readable explanation (e.g. `"object deleted"`, `"out of range"`)

**JSON-RPC Method:** `object.unpublish` (call from extension to viewer)

The extension may also call `object.unpublish` to manually stop tracking an object. The viewer will stop publishing it and send a corresponding `object.unpublish` notification back to the caller.

```typescript
interface ObjectUnpublishParams {
  object_id: string;  // UUID of the root prim to unpublish
}

interface ObjectUnpublishResponse {
  success: boolean;
  object_id?: string;
}
```

**Fields:**

- `object_id`: UUID of the root prim to unpublish.
- `success`: `true` if the object was published and has been removed.

**Note:** The viewer also sends an `object.unpublish` notification to the caller immediately after responding. Extensions should handle that notification idempotently.

---

### ObjectUpdate

**JSON-RPC Method:** `object.update` (notification from viewer)

Sent when the inventory of a published object changes. Supports two modes:
- **Full replacement**: `inventory` and/or `linked_objects` fields replace the entire prior state.
- **Delta update**: `changes` field describes only what changed. Takes precedence over full replacement fields when present.

```typescript
interface InventoryChanges {
  added?: ObjectInventoryItem[];
  removed?: string[];                                         // item_ids removed
  modified?: ObjectInventoryItem[];                           // metadata-only changes
  content_changed?: string[];                                 // item_ids whose content changed (invalidates cache)
  running_changed?: { item_id: string; running: boolean }[];  // running state toggled
}

interface LinkedObjectChanges {
  added?: LinkedObject[];
  removed?: string[];         // link_ids removed
  modified?: {
    link_id: string;
    link_name?: string;
    inventory?: InventoryChanges;
  }[];
}

interface ObjectUpdateMessage {
  object_id: string;
  object_name?: string;
  // Full replacement (used when changes is absent)
  inventory?: ObjectInventoryItem[];
  linked_objects?: LinkedObject[];
  // Delta (takes precedence when present)
  changes?: {
    inventory?: InventoryChanges;
    linked_objects?: LinkedObjectChanges;
  };
}
```

---

### ObjectContentGet

**JSON-RPC Method:** `object.content.get` (call from extension to viewer)

Requests the text content of a script or notecard. The extension calls this lazily when the user opens a file in the virtual filesystem.

```typescript
interface ObjectContentGetParams {
  prim_id: string;  // UUID of any prim (root or child) - no object_id + link_id needed
  item_id: string;
}

interface ObjectContentGetResponse {
  success: boolean;
  prim_id: string;
  item_id: string;
  content: string;  // Raw text content (UTF-8). Notecard envelope is unwrapped automatically.
}
```

**Fields:**

- `prim_id`: UUID of the prim that owns the item. Child prims are addressable directly by UUID without knowing the root object_id.
- `item_id`: Inventory item UUID.
- `success`: `true` on success.
- `content`: The raw text content of the item. For notecards, the `Linden text version 2` envelope is stripped - only the body text is returned.

---

### ObjectContentSave

**JSON-RPC Method:** `object.content.save` (call from extension to viewer)

Writes modified content back to the viewer. For scripts, the viewer will attempt to compile the updated source.

```typescript
interface ObjectContentSaveParams {
  prim_id: string;
  item_id: string;
  content: string;
  vm?: "mono" | "lsl2" | "luau";
}

interface ObjectContentSaveResponse {
  success: boolean;
  prim_id?: string;
  item_id?: string;
  compiled?: boolean;
  errors?: string[];
  message?: string;
}
```

**Fields:**

- `prim_id`: UUID of the prim that owns the saved item.
- `item_id`: UUID of the saved inventory item.
- `content`: Raw script/notecard source text to store.
- `vm` (optional): Scripts only compile target. Accepted values are `"mono"`, `"lsl2"`, `"luau"`. When `"luau"` is specified for an LSL script (as opposed to a native Luau script), the viewer automatically selects the correct LSL-on-Luau compile path. If omitted, inferred from item metadata or content analysis.
- `success`: Whether the upload/save operation succeeded.
- `compiled` (optional): Scripts only. `true` when compilation succeeded, `false` when source saved but compile failed.
- `errors` (optional): Scripts only. Compiler diagnostics when `compiled` is `false`.
- `message` (optional): Error description on failure.

---

### ObjectItemCreate

**JSON-RPC Method:** `object.item.create` (call from extension to viewer)

Creates a new script in a prim's inventory. The call is asynchronous - the viewer sends
`RezScript` to the simulator and waits for the inventory-changed callback before returning
the created item's details. The simulator may rename the item if a duplicate name exists.

Notecard creation is not yet supported and will return an error.

```typescript
interface ObjectItemCreateParams {
  prim_id: string;           // UUID of the prim to create the item in
  name: string;              // Pure SL inventory name - no file extension
  type: InventoryItemType;   // "script" ("notecard" reserved for future)
  vm: ScriptVM;              // Required for scripts: "luau" | "mono" | "lsl2"
}

// On success, returns an ObjectInventoryItem with prim_id:
interface ObjectItemCreateResponse extends ObjectInventoryItem {
  prim_id: string;           // Echoed prim UUID
}
```

**Notes:**
- The response matches the `ObjectInventoryItem` structure (same fields as items in
  `object.publish` and `object.update` notifications).
- The `name` in the response may differ from the request if the simulator renamed it.
- An `object.update` notification will also fire for the prim (since inventory changed).
- Timeout: 30 seconds. Returns a JSON-RPC internal error if the simulator does not respond.

---

### ObjectItemDelete

**JSON-RPC Method:** `object.item.delete` (call from extension to viewer)

Deletes a script or notecard from a prim's inventory. Requires `PERM_MODIFY` on the item.

```typescript
interface ObjectItemDeleteParams {
  prim_id: string;
  item_id: string;
}

interface ObjectItemDeleteResponse {
  success: boolean;
  prim_id: string;   // Echoed back from request
  item_id: string;   // Echoed back from request
}
```

---

### ObjectScriptSetRunning

**JSON-RPC Method:** `object.script.set_running` (call from extension to viewer)

Starts or stops a script within a prim.

```typescript
interface ObjectScriptSetRunningParams {
  prim_id: string;
  item_id: string;
  running: boolean;  // true = start, false = stop
}

interface ObjectScriptSetRunningResponse {
  success: boolean;
  message?: string;
}
```

---

### ObjectScriptReset

**JSON-RPC Method:** `object.script.reset` (call from extension to viewer)

Resets a script within a prim, clearing its state and restarting from the default state entry.

```typescript
interface ObjectScriptResetParams {
  prim_id: string;
  item_id: string;
}

interface ObjectScriptResetResponse {
  success: boolean;
  message?: string;
}
```

---

### ObjectRequest

**JSON-RPC Method:** `object.request` (call from extension to viewer)

Requests the viewer to publish a specific in-world object. The viewer responds synchronously to confirm the request was accepted, then asynchronously sends an `object.publish` notification with the full object tree.

This is typically called immediately after the handshake completes when the extension was launched by the viewer with an `object=<uuid>` URI parameter.

```typescript
interface ObjectRequestParams {
  object_id: string;  // UUID of the root prim to request publishing for
}

interface ObjectRequestResponse {
  success: boolean;
  message?: string;  // reason on failure (e.g. "object not found", "permission denied")
}
```

**Fields:**

- `object_id`: UUID of the root prim of the linkset to publish.
- `success`: Whether the viewer accepted the request. A `true` response does not mean `object.publish` has been sent yet - it means the viewer will send it.
- `message` (optional): Human-readable failure reason. Only present when `success` is `false`.

**Sequence:**
1. Extension calls `object.request`
2. Viewer responds with `{ success: true }` (or error)
3. Viewer sends `object.publish` notification (asynchronously, when ready)

---

### ObjectList

**JSON-RPC Method:** `object.list` (call from extension to viewer)

Requests the complete list of currently published objects. Called by the extension immediately after the handshake completes (`session.ok`) to restore state for any objects the viewer already has published.

The viewer responds synchronously with all published objects in the same format as `object.publish` notifications. No follow-up notifications are sent.

```typescript
// No request parameters

interface ObjectListResponse {
  objects: PublishedObject[];  // All currently published objects; empty array if none
}
```

**Fields:**

- `objects`: Array of `PublishedObject` records (same shape as the `object` field in `object.publish`). Empty array when no objects are currently published.

**Sequence:**
1. Viewer sends `session.ok`
2. Extension calls `object.list` (no params)
3. Viewer responds with `{ objects: [...] }` synchronously

---

### ObjectModify

**JSON-RPC Method:** `object.modify` (call from extension to viewer)

Modifies properties of a prim (root or linked) such as name, description, or permissions. Only specified fields are modified; omitted fields remain unchanged. Requires `PERM_MODIFY` on the object.

```typescript
interface ObjectModifyParams {
  prim_id: string;           // UUID of any prim (root or child)
  name?: string;             // New display name
  description?: string;      // New description
  permissions?: {
    next_owner?: number;     // Permission mask applied on transfer
  };
}

interface ObjectModifyResponse {
  success: boolean;
  prim_id: string;           // Echoed back from request
  message?: string;          // Error description on failure
}
```

**Fields:**

- `prim_id`: UUID of the prim to modify. Child prims are addressable directly by UUID.
- `name` (optional): New display name for the prim. If omitted, name remains unchanged.
- `description` (optional): New description for the prim. If omitted, description remains unchanged.
- `permissions` (optional): Permission changes.
  - `next_owner`: Permission mask applied when the object is transferred. Uses same bit flags as `ItemPermissions` (e.g., `PERM_MODIFY=0x4000`, `PERM_COPY=0x8000`, `PERM_TRANSFER=0x2000`).
- `success`: Whether the update operation succeeded.
- `message` (optional): Error description. Only present when `success` is `false`.

**Notes:**
- At least one property field (`name`, `description`, or `permissions`) must be specified.
- An `object.update` notification will fire after successful modification.
- Owner permissions cannot be modified directly — only `next_owner` can be changed.

---

### ObjectItemModify

**JSON-RPC Method:** `object.item.modify` (call from extension to viewer)

Modifies properties of an inventory item such as name, description, or permissions. Only specified fields are modified; omitted fields remain unchanged. Requires `PERM_MODIFY` on the item.

```typescript
interface ObjectItemModifyParams {
  prim_id: string;           // UUID of any prim (root or child)
  item_id: string;           // Inventory item UUID
  name?: string;             // New display name (no file extension)
  description?: string;      // New description
  permissions?: {
    next_owner?: number;     // Permission mask applied on transfer
  };
}

interface ObjectItemModifyResponse {
  success: boolean;
  prim_id: string;           // Echoed back from request
  item_id: string;           // Echoed back from request
  message?: string;          // Error description on failure
}
```

**Fields:**

- `prim_id`: UUID of the prim that owns the item. Child prims are addressable directly by UUID.
- `item_id`: Inventory item UUID.
- `name` (optional): New display name for the item. Should not include file extension (e.g., `.lsl`, `.luau`). If omitted, name remains unchanged.
- `description` (optional): New description for the item. If omitted, description remains unchanged.
- `permissions` (optional): Permission changes.
  - `next_owner`: Permission mask applied when the item is transferred. Uses same bit flags as `ItemPermissions` (e.g., `PERM_MODIFY=0x4000`, `PERM_COPY=0x8000`, `PERM_TRANSFER=0x2000`).
- `success`: Whether the update operation succeeded.
- `message` (optional): Error description. Only present when `success` is `false`.

**Notes:**
- At least one property field (`name`, `description`, or `permissions`) must be specified.
- An `object.update` notification will fire after successful modification.
- Owner permissions cannot be modified directly — only `next_owner` can be changed.
- If the item is renamed, the virtual filesystem path will change and the extension must handle the rename appropriately.
