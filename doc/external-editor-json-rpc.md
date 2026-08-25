# Viewer to External Editor JSON-RPC<br>Message Interfaces Documentation

> **This file is a mirror.** The canonical source is `doc/Message_Interfaces.md` in the
> object_publish extension repository. Do not edit this copy — edit the canonical file and
> re-copy it here.

This document describes all the message interfaces defined for WebSocket communication between the Second Life viewer and an external editor such as a VSCode extension.

## Table of Contents

- [Usage Flow](#usage-flow)
- [VS Code Launch URI](#vs-code-launch-uri)
- [JSON-RPC Method Summary](#json-rpc-method-summary)
- [Error Handling](#error-handling)
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
  - [Diagnostic](#diagnostic)
  - [CompilationResult](#compilationresult)
- [Runtime Event Interfaces](#runtime-event-interfaces)
  - [RuntimeDebug](#runtimedebug)
  - [RuntimeError](#runtimeerror)
- [Handler and Configuration Interfaces](#handler-and-configuration-interfaces)
  - [WebSocketHandlers](#websockethandlers)
  - [ClientInfo](#clientinfo)
- [Object Explorer Interfaces](#object-explorer-interfaces)
  - [Core Data Types](#core-data-types)
  - [Common preconditions](#common-preconditions)
  - [ObjectPublish](#objectpublish)
  - [ObjectUnpublish](#objectunpublish)
  - [ObjectUpdate](#objectupdate)
  - [ObjectContentGet](#objectcontentget)
  - [ObjectContentSave](#objectcontentsave)
  - [ObjectItemCreate](#objectitemcreate)
  - [ObjectItemDelete](#objectitemdelete)
  - [ObjectScriptSetRunning](#objectscriptsetrunning)
  - [ObjectScriptReset](#objectscriptreset)
  - [ObjectRequest](#objectrequest)
  - [ObjectList](#objectlist)
  - [ObjectModify](#objectmodify)
  - [ObjectItemModify](#objectitemmodify)
- [Command Interfaces](#command-interfaces)
  - [CommandExecute](#commandexecute)
  - [CommandList](#commandlist)

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

4. **Object Explorer:**

   - Viewer sends `object.publish` notification when an in-world object's contents are made available for editing (user clicks "Explore in IDE")
   - Viewer sends `object.unpublish` notification when an object is removed or the user stops exploring
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
| `port`    | No       | Port number the viewer's WebSocket server is listening on. Overrides the user's configured port for this session. Defaults to the configured `slVscodeEdit.network.websocketPort` (default `9020`) if absent. Must be in range 1024–65535. |
| `object`  | No       | UUID of a root prim. After the handshake completes the extension calls `object.request` to ask the viewer to start exploring this object. The viewer then sends an `object.publish` notification and the object appears as a workspace folder in the Explorer. |
| `script`  | No       | UUID of a script. After the handshake completes the extension locates the corresponding temp file via `script.list` and opens it, triggering the normal `script.subscribe` + live-sync flow. |

`object` and `script` are mutually exclusive in typical use but both may be supplied; the extension will process both.

### Examples

```
# Open VS Code and connect on default port
vscode://lindenlab.sl-vscode-plugin/connect

# Connect on a custom port
vscode://lindenlab.sl-vscode-plugin/connect?port=9021

# Connect and immediately explore a specific object
vscode://lindenlab.sl-vscode-plugin/connect?port=9020&object=550e8400-e29b-41d4-a716-446655440000

# Connect and open a specific script for editing
vscode://lindenlab.sl-vscode-plugin/connect?port=9020&script=6ba7b810-9dad-11d1-80b4-00c04fd430c8
```

### Post-connection sequence

When the URI contains an `object` or `script` parameter the extension acts only **after** the handshake is fully complete (`session.ok` received):

```
URI received by extension
        │
        ▼
WebSocket connects → session.handshake → session.ok
        │
        ├─ object=<uuid> → object.request({ object_id }) call
        │                       │
        │                       ▼  (async, when viewer is ready)
        │                  object.publish notification
        │
        └─ script=<uuid> → script.list call → open temp file
                                                    │
                                                    ▼
                                          script.subscribe + live-sync
```

## JSON-RPC Method Summary

| Method                          | Direction          | Type         | Interface/Parameters       |
| ------------------------------- | ------------------ | ------------ | -------------------------- |
| `session.handshake`             | Viewer → Extension | Call         | `SessionHandshake`         |
| `session.handshake` (response)  | Extension → Viewer | Response     | `SessionHandshakeResponse` |
| `session.ok`                    | Viewer → Extension | Notification | _(no interface)_           |
| `session.disconnect`            | Bidirectional      | Notification | `SessionDisconnect`        |
| `session.ping`                  | Bidirectional      | Call         | `SessionPing`              |
| `session.ping` (response)       | Bidirectional      | Response     | `SessionPingResponse`      |
| `script.subscribe`              | Extension → Viewer | Call         | `ScriptSubscribe`          |
| `script.subscribe` (response)   | Viewer → Extension | Response     | `ScriptSubscribeResponse`  |
| `script.unsubscribe`            | Viewer → Extension | Notification | `ScriptUnsubscribe`        |
| `script.unsubscribe`            | Extension → Viewer | Call         | `ScriptUnsubscribeParams`  |
| `script.unsubscribe` (response) | Viewer → Extension | Response     | `null`                     |
| `script.list`                   | Extension → Viewer | Call         | _(no parameters)_          |
| `script.list` (response)        | Viewer → Extension | Response     | `ScriptList`               |
| `language.syntax.id`            | Extension → Viewer | Call         | _(no parameters)_          |
| `language.syntax.id` (response) | Viewer → Extension | Response     | `{ id: string }`           |
| `language.syntax`               | Extension → Viewer | Call         | `{ kind: string }`         |
| `language.syntax` (response)    | Viewer → Extension | Response     | `LanguageInfo`             |
| `language.syntax.cache`            | Extension → Viewer | Call         | _(no parameters)_                    |
| `language.syntax.cache` (response) | Viewer → Extension | Response     | `SyntaxCacheList`                    |
| `language.syntax.get`              | Extension → Viewer | Call         | `{ filename: string, as_json?: boolean }` |
| `language.syntax.get` (response)   | Viewer → Extension | Response     | `SyntaxCacheFile`                    |
| `language.syntax.change`        | Viewer → Extension | Notification | `SyntaxChange`             |
| `script.compiled`               | Viewer → Extension | Notification | `CompilationResult`        |
| `runtime.debug`                 | Viewer → Extension | Notification | `RuntimeDebug`             |
| `runtime.error`                 | Viewer → Extension | Notification | `RuntimeError`             |
| `object.publish`                | Viewer → Extension | Notification | `ObjectPublishMessage`     |
| `object.unpublish`              | Viewer → Extension | Notification | `ObjectUnpublishMessage`   |
| `object.unpublish`              | Extension → Viewer | Call         | `ObjectUnpublishParams`    |
| `object.unpublish` (response)   | Viewer → Extension | Response     | `ObjectUnpublishResponse`  |
| `object.update`                 | Viewer → Extension | Notification | `ObjectUpdateMessage`      |
| `object.content.get`            | Extension → Viewer | Call         | `ObjectContentGetParams`   |
| `object.content.get` (response) | Viewer → Extension | Response     | `ObjectContentGetResponse` |
| `object.content.save`           | Extension → Viewer | Call         | `ObjectContentSaveParams`  |
| `object.content.save` (response)| Viewer → Extension | Response     | `ObjectContentSaveResponse`|
| `object.item.create`            | Extension → Viewer | Call         | `ObjectItemCreateParams`   |
| `object.item.create` (response) | Viewer → Extension | Response     | `ObjectItemCreateResponse` |
| `object.item.delete`            | Extension → Viewer | Call         | `ObjectItemDeleteParams`   |
| `object.item.delete` (response) | Viewer → Extension | Response     | `ObjectItemDeleteResponse` |
| `object.script.set_running`     | Extension → Viewer | Call         | `ObjectScriptSetRunningParams` |
| `object.script.set_running` (response) | Viewer → Extension | Response | `ObjectScriptSetRunningResponse` |
| `object.script.reset`           | Extension → Viewer | Call         | `ObjectScriptResetParams`      |
| `object.script.reset` (response)| Viewer → Extension | Response     | `ObjectScriptResetResponse`    |
| `object.request`                | Extension → Viewer | Call         | `ObjectRequestParams`          |
| `object.request` (response)     | Viewer → Extension | Response     | `ObjectRequestResponse`        |
| `object.list`                   | Extension → Viewer | Call         | `{}` (no params)               |
| `object.list` (response)        | Viewer → Extension | Response     | `ObjectListResponse`           |
| `object.modify`                 | Extension → Viewer | Call         | `ObjectModifyParams`           |
| `object.modify` (response)      | Viewer → Extension | Response     | `ObjectModifyResponse`         |
| `object.item.modify`            | Extension → Viewer | Call         | `ObjectItemModifyParams`       |
| `object.item.modify` (response) | Viewer → Extension | Response     | `ObjectItemModifyResponse`     |
| `command.execute`               | Bidirectional      | Call         | `CommandExecuteParams`         |
| `command.execute` (response)    | Bidirectional      | Response     | `CommandExecuteResponse`       |
| `command.list`                  | Bidirectional      | Call         | _(no params)_                  |
| `command.list` (response)       | Bidirectional      | Response     | `CommandListResponse`          |

## Error Handling

A call that fails returns a JSON-RPC 2.0 `error` object rather than a result. There is no partial
success: a response carries either `result` or `error`, never both.

```json
{
  "jsonrpc": "2.0",
  "id": 12,
  "error": {
    "code": -32602,
    "message": "Invalid params: No syntax category specified"
  }
}
```

**Standard JSON-RPC codes:**

| Code | Meaning |
| ---- | ------- |
| `-32700` | Parse error — invalid JSON was received |
| `-32600` | Invalid Request — the JSON sent is not a valid Request object |
| `-32601` | Method not found |
| `-32602` | Invalid params |
| `-32603` | Internal error |

**Server-specific codes.** The range `-32000` to `-32099` is reserved for server errors. The
transport defines the following; `-32001` and `-32003` are the ones handlers commonly raise.

| Code | Meaning |
| ---- | ------- |
| `-32000` | Connection closed unexpectedly |
| `-32001` | Request timed out |
| `-32002` | Authentication required |
| `-32003` | Access denied |
| `-32004` | Too many requests |
| `-32005` | Service temporarily unavailable |
| `-32006` | Message exceeds maximum size |
| `-32007` | Session expired or invalid |

**Message format.** Standard errors prefix the handler's detail text with a fixed label, so
`error.message` reads `"Invalid params: <detail>"`, `"Internal error: <detail>"`,
`"Method not found: <method>"`, and so on. Server-specific errors carry the detail text alone
(e.g. `"Access denied"`). Clients should branch on `error.code`, not on `error.message`.

**`success` is not an error channel.** Several results carry a `success` field. Where a method
reports failure through a JSON-RPC error, that field is `true` on every response it ever
appears in, and a failed call produces no result to inspect. Do not treat a missing or false
`success` as the failure signal unless the method's own section documents it as one.

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
  - `commands`: Both sides support `command.execute` and `command.list`
  - `unified_diagnostics`: Viewer supports the unified diagnostic reporting format

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
- `features`: Dictionary of features supported by the client. Known flags:
  - `live_sync`: Client supports live script synchronisation
  - `error_reporting`: Client accepts `runtime.error` notifications
  - `unified_diagnostics`: Client supports the unified diagnostic reporting format
  - `object_publish`: Client supports the object explorer methods and notifications
  - `commands`: Both sides support `command.execute` and `command.list`
  - `debugging`: Advertised as `false`. Reserved; no debugging support.
  - `breakpoints`: Advertised as `false`. Reserved; no breakpoint support.
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

In practice the extension initiates and the viewer only answers — the viewer never sends
`session.ping` itself. The extension pings every 30 seconds and tears the connection down after
two consecutive failures.

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

- `timestamp`: The original timestamp from the request. Echoed back only when the request supplied one.
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
  defs: object;
  success: boolean;
}
```

**Response Fields:**

- `id`: The current syntax version identifier
- `defs`: The keyword definitions object. Structure varies by language.
- `success`: Always `true`. Failures are returned as JSON-RPC errors — see below.

**Errors:**

| Condition | Code | `error.message` |
| --------- | ---- | --------------- |
| No `kind` parameter supplied | `-32602` | `Invalid params: No syntax category specified` |
| Unknown `kind` value | `-32602` | `Invalid params: Unknown syntax category requested` |
| Definitions unavailable for a valid `kind` | `-32603` | `Internal error: Syntax definitions are unavailable` |

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

Not all files may be present in every cache — the actual list returned by `language.syntax.cache` reflects only what is available on the viewer's local filesystem at the time of the request.

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
  content: string | object;  // String if as_json is false/omitted, object if as_json is true
  success: boolean;
}
```

**Response Fields:**

- `content`: The file content. Is a raw text string when `as_json` is omitted or `false`; is a parsed object when `as_json` is `true`.
- `success`: Always `true`. Failures are returned as JSON-RPC errors — see below.

**Errors:**

| Condition | Code | `error.message` |
| --------- | ---- | --------------- |
| No `filename` parameter supplied | `-32602` | `Invalid params: No filename specified` |
| Name not found in cache | `-32602` | `Invalid params: Requested syntax cache file not found` |
| File could not be loaded (`as_json` omitted or `false`) | `-32603` | `Internal error: Failed to load syntax cache file` |
| File could not be loaded or parsed (`as_json` is `true`) | `-32603` | `Internal error: Failed to load and format syntax cache file.` |

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
  message: string;
  object_id?: string;   // Success only
  root_id?: string;     // Success only
  item_id?: string;     // Success only
}
```

**Fields:**

- `script_id`: The script identifier that was subscribed to
- `success`: Whether the subscription was successful
- `status`: Numeric status code indicating the result:
  - `0`: Success
  - `1`: Invalid editor — the script editor panel is no longer open
  - `2`: Invalid subscription — no subscription found for the given `script_id`
  - `3`: Already subscribed — another connection is already subscribed to this script
  - `4`: Internal server error
- `message`: Always present. Fixed text matching `status`:

| `status` | `message` |
| -------- | --------- |
| `0` | `OK` |
| `1` | `Invalid editor handle` |
| `2` | `No subscription found for script` |
| `3` | `Script already subscribed` |
| `4` | `Internal server error` |

- `object_id`: UUID of the **prim** that owns the script. For a script in a child prim this is the child's UUID, not the linkset root's. Present only when `success` is `true`.
- `root_id`: UUID of the root prim of the linkset containing the script. Present only when `success` is `true`. If the prim cannot be resolved, `root_id` is set equal to `object_id`, so equality does not by itself mean the script lives in the root prim.
- `item_id`: The inventory item UUID of the script within the prim. Present only when `success` is `true`.

### ScriptUnsubscribe

**JSON-RPC Method:** `script.unsubscribe` (notification from viewer)

Notification sent by the viewer when a script subscription should be terminated. It is delivered
only to the connection holding the subscription, not to all connections.

```typescript
interface ScriptUnsubscribe {
  script_id: string;
}
```

**Fields:**

- `script_id`: Unique identifier for the script to unsubscribe from

**JSON-RPC Method:** `script.unsubscribe` (call from extension to viewer)

The extension may also call `script.unsubscribe` to end a subscription it holds.

```typescript
interface ScriptUnsubscribeParams {
  script_id: string;  // Script whose subscription should be dropped
}
```

The result is `null`.

The viewer drops the subscription only when the calling connection owns it. The call is
idempotent: an unknown `script_id`, or one held by a different connection, is ignored and still
returns a successful `null` result. A client therefore cannot use the response to detect that it
targeted the wrong script.

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

### Diagnostic

Individual compilation error record.

```typescript
interface Diagnostic {
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
  diagnostics?: Diagnostic[];
}
```

**Fields:**

- `script_id`: Unique identifier for the script that was compiled
- `success`: Whether the compilation was successful
- `running`: Whether the compiled script is currently running
- `diagnostics` (optional): Array of `Diagnostic` records if any occurred

**Delivery:** routed only to the connection subscribed to that script. This differs from
`runtime.debug` and `runtime.error`, which are broadcast to every connection.

**Two compile-feedback paths.** Compilation results reach a client by one of two routes,
depending on how the save was made:

| Save route | Feedback |
| ---------- | -------- |
| `object.content.save` (object explorer) | `compiled` and `diagnostics` returned inline in the response |
| Live-sync editing of a subscribed script | `script.compiled` notification |

A client using only the object explorer never receives `script.compiled`; a client waiting for
`script.compiled` after an `object.content.save` will wait indefinitely. Consolidating these two
paths is tracked separately.

**Known limitation.** `script.compiled` is produced only while the viewer's script editor for that
script is open. If that editor has closed, compilation results are dropped without notice — no
result, no error, and not necessarily a preceding `script.unsubscribe`. Tracked separately.

## Runtime Event Interfaces

### RuntimeDebug

**JSON-RPC Method:** `runtime.debug` (notification from viewer)

Debug message notification sent by the viewer during script execution.

```typescript
interface RuntimeDebug {
  script_id: string;   // Not currently sent — see note below
  object_id: string;
  prim_id: string;
  item_id: string;
  object_name: string;
  message: string;
  channel: "debug" | "owner_say";
  item: ItemRef;
}
```

**Fields:**

- `script_id`: Identifier for the script generating the debug message
- `object_id`: UUID of the root prim of the object containing the script
- `prim_id`: UUID of the prim that owns the script
- `item_id`: Inventory item UUID of the script
- `object_name`: Human-readable name of the object
- `message`: The debug message content
- `channel`: Source of the text. `"debug"` for script debug output, `"owner_say"` for owner-directed chat.
- `item`: Reference identifying the originating script. See `ItemRef` under `RuntimeError`.

### RuntimeError

**JSON-RPC Method:** `runtime.error` (notification from viewer)

Runtime error notification sent by the viewer when a script encounters an error during execution.

```typescript
interface RuntimeError {
  script_id: string;   // Not currently sent — see note below
  object_id: string;
  prim_id: string;
  item_id: string;
  object_name: string;
  message: string;
  error: string;
  line: number;
  column: number;
  stack: string[];
  channel: "debug" | "owner_say";
  item: ItemRef;
}

interface ItemRef {
  root_id: string;
  prim_id: string;
  item_id: string;
  name: string;
  language: "lsl" | "luau";
}
```

**Fields:**

- `script_id`: Identifier for the script that encountered the error
- `object_id`: UUID of the root prim of the object containing the script
- `prim_id`: UUID of the prim that owns the script
- `item_id`: Inventory item UUID of the script
- `object_name`: Human-readable name of the object
- `message`: The full raw chat text of the runtime error message as received from the simulator
- `error`: Extracted runtime error description. This remains a top-level compatibility field while the protocol stays on version `1.0`.
- `line`: Line number where the error occurred when the runtime format can be parsed; otherwise `0`.
- `column`: Column position where the error occurred when the runtime format can be parsed; otherwise `0`.
- `stack`: Stack trace lines extracted from the error message. Always present; an empty array when no trace could be extracted, so test its length rather than its presence.
- `channel`: Source of the text. `"debug"` for script debug output, `"owner_say"` for owner-directed chat.
- `item`: Reference identifying the originating script.
  - `root_id`: UUID of the root prim of the linkset.
  - `prim_id`: UUID of the prim that owns the script.
  - `item_id`: Inventory item UUID of the script.
  - `name`: Script name as it appears in the prim's inventory.
  - `language`: The script's source language. Independent of the compile target; the VM is not carried in runtime messages.

**Note on `script_id`:** this field is part of the contract but is **not currently sent** by the
viewer for either `runtime.debug` or `runtime.error`. Implementation is tracked separately.

**Delivery:** `runtime.debug` and `runtime.error` are broadcast to all connections. An event is
emitted when the originating object is published or its script is subscribed.

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

## Object Explorer Interfaces

These interfaces support exploring in-world object inventories (scripts and notecards) from the external editor as a browseable virtual filesystem. The extension exposes explored objects under the `sl://objects/` URI scheme.

Explored objects are shared across all connections rather than owned by the connection that
requested them. `object.publish`, `object.update` and `object.unpublish` are broadcast to every
connected client, so a client will receive notifications for objects it never requested, and must
drop an object from its own state when an `object.unpublish` for it arrives.

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
  link_number: number;      // Link number (root=1, children≥2)
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
  can_save_back?: boolean;      // Whether Save Back to Contents is currently available for this object
  inventory: ObjectInventoryItem[];      // Root prim's scripts and notecards
  linked_objects?: LinkedObject[];       // Child prims
}
```

**Display names.** The extension synthesises a file extension for scripts when presenting items in
the virtual filesystem:

| Item | Extension |
| ---- | --------- |
| Script, `subtype` `0` (LSL) | `.lsl` |
| Script, `subtype` `1` (Luau) | `.luau` |
| Notecard | _(none)_ |

Notecards receive no synthetic extension; the inventory name is used verbatim, so an extension the
user gave the notecard is preserved and none is added.

These extensions exist only for display and are never part of the item's inventory name. Names sent
to and received from the viewer — including in `object.item.create` and `object.item.modify` — are
always the pure inventory name, without an extension.

### Common preconditions

Every method that addresses an item by `prim_id` + `item_id` — `object.content.get`,
`object.content.save`, `object.item.delete` and `object.item.modify` — runs the same validation
before doing any work, in this order:

1. Both `prim_id` and `item_id` are present.
2. The prim exists.
3. The object containing the prim is currently published.
4. The item exists in that prim's inventory.
5. The item is a script or a notecard.
6. The caller holds the permissions that method requires.

An object must be published before any of its items can be addressed. Learning an object's id from
`object.list` is not sufficient on its own — the object must be published, which `object.list`
reports and `object.request` initiates.

**Errors:**

| Condition | Code | `error.message` |
| --------- | ---- | --------------- |
| `prim_id` or `item_id` missing | `-32602` | `Invalid params: prim_id and item_id are required` |
| Prim not found | `-32602` | `Invalid params: Prim not found` |
| Object is not published | `-32003` | `Object is not published` |
| Item not in the prim's inventory | `-32602` | `Invalid params: Item not found in prim inventory` |
| Item is not a script or notecard | `-32602` | `Invalid params: Item is not a script or notecard` |
| Required item permission denied | `-32003` | `Insufficient permissions` |
| Modify denied on the containing prim | `-32003` | `No modify permission on object` |

Writes require modify permission on both the item **and** the prim that contains it. A no-modify
object can be published and read, but its contents cannot be changed.

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

- `object`: The full object tree being explored, including root prim inventory and all linked prim inventories.
  - `can_save_back` (optional): Capability hint for UI actions. When `true`, the object currently supports the `viewer.object.save_back_to_contents` command.

---

### ObjectUnpublish

**JSON-RPC Method:** `object.unpublish` (notification from viewer)

Sent when the viewer stops exploring a previously explored object — for example when the user stops exploring it from the viewer UI, the extension calls `object.unpublish`, or the object is deleted or linked into another object.

```typescript
interface ObjectUnpublishMessage {
  object_id: string;
  reason?: string;
}
```

**Fields:**

- `object_id`: UUID of the root prim that is no longer being explored
- `reason` (optional): Machine-readable token identifying why exploring stopped. One of:

| Value | Meaning |
| ----- | ------- |
| `manual` | The extension called `object.unpublish` for this object. |
| `republish` | The object is being re-published; a fresh `object.publish` follows. |
| `user` | The user stopped exploring the object from the viewer UI. |
| `deleted` | The object was deleted. |
| `linked` | The object became a child prim of a linkset and is no longer a root. |

The field is omitted only when no reason was supplied; in practice every unpublish carries one
of the values above.

**JSON-RPC Method:** `object.unpublish` (call from extension to viewer)

The extension may also call `object.unpublish` to manually stop exploring an object. The viewer will stop and broadcast a corresponding `object.unpublish` notification to all connections.

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

- `object_id`: UUID of the root prim to stop exploring.
- `success`: `true` if the object was being explored and has been removed.

**Note:** The viewer also broadcasts an `object.unpublish` notification immediately after responding. The caller receives it too, so extensions should handle that notification idempotently.

---

### ObjectUpdate

**JSON-RPC Method:** `object.update` (notification from viewer)

Sent when an explored object's inventory, properties, or linkset membership change.

**Shapes currently emitted.** The viewer sends exactly the following five payloads:

| Trigger | Payload |
| ------- | ------- |
| Root prim inventory changed | `object_id`, `inventory` (complete replacement array) |
| Child prim inventory changed | `object_id`, `changes.linked_objects.modified[]` with `link_id` and `inventory` (complete replacement array for that child) |
| Root prim name/description changed | `object_id`, `object_name` and/or `object_description` |
| Child prim name/description changed | `object_id`, `changes.linked_objects.modified[]` with `link_id` and `link_name` and/or `link_description` |
| Linkset membership changed | `object_id`, `linked_objects` (complete replacement array covering every child prim) |

`inventory` and `linked_objects` are always complete replacements of the prior state, never
increments. Linkset membership changes are coalesced behind a short flush delay, so several
rapid link or unlink operations may arrive as a single update.

```typescript
interface ObjectUpdateMessage {
  object_id: string;
  object_name?: string;
  object_description?: string;
  // Full replacement
  inventory?: ObjectInventoryItem[];
  linked_objects?: LinkedObject[];
  changes?: {
    inventory?: InventoryChanges;        // Not implemented — see below
    linked_objects?: LinkedObjectChanges;
  };
}

interface LinkedObjectChanges {
  added?: LinkedObject[];     // Not implemented — see below
  removed?: string[];         // Not implemented — see below
  modified?: {
    link_id: string;
    link_name?: string;
    link_description?: string;
    inventory?: ObjectInventoryItem[];   // Complete replacement array, not a delta
  }[];
}
```

**Note:** `changes.linked_objects.modified` is emitted, as shown in the table above.
`changes.linked_objects.added` and `changes.linked_objects.removed` are not.

#### Not implemented

The delta sub-protocol below is specified but is **not currently emitted by the viewer**.
Implementation is tracked separately. Clients must rely on the full-replacement shapes listed
above; code written to consume these types will never run against the current viewer.

```typescript
interface InventoryChanges {
  added?: ObjectInventoryItem[];
  removed?: string[];                                         // item_ids removed
  modified?: ObjectInventoryItem[];                           // metadata-only changes
  content_changed?: string[];                                 // item_ids whose content changed (invalidates cache)
  running_changed?: { item_id: string; running: boolean }[];  // running state toggled
}
```

Also not implemented: `LinkedObjectChanges.added` and `LinkedObjectChanges.removed`. Linkset
membership changes are sent as a complete `linked_objects` replacement array instead.

---

### ObjectContentGet

**JSON-RPC Method:** `object.content.get` (call from extension to viewer)

Requests the text content of a script or notecard. The extension calls this lazily when the user opens a file in the virtual filesystem.

```typescript
interface ObjectContentGetParams {
  prim_id: string;  // UUID of any prim (root or child) — no object_id + link_id needed
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
- `content`: The raw text content of the item. For notecards, the `Linden text version 2` envelope is stripped — only the body text is returned.

**Permissions.** Scripts require both `PERM_COPY` and `PERM_MODIFY` on the item: the source of a
no-copy or no-modify script is never exposed. Notecards require no permission at all, so that
no-modify notecards remain readable in the external editor. See
[Common preconditions](#common-preconditions) for the shared checks and errors.

**Timeout.** 30 seconds to fetch the asset, after which the call fails with `-32001`
(`Asset fetch timed out`).

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
  running?: boolean;  // Scripts only: run state applied after compilation when supplied.
}

interface ObjectContentSaveResponse {
  success: boolean;
  prim_id?: string;
  item_id?: string;
  compiled?: boolean;
  diagnostics?: Diagnostic[];
}
```

**Fields:**

- `prim_id`: UUID of the prim that owns the saved item.
- `item_id`: UUID of the saved inventory item.
- `content`: Raw script/notecard source text to store.
- `vm` (optional): Scripts only compile target. Accepted values are `"mono"`, `"lsl2"`, `"luau"`. When `"luau"` is specified for an LSL script (as opposed to a native Luau script), the viewer automatically selects the correct LSL-on-Luau compile path. If omitted, inferred from item metadata or content analysis.
- `running` (optional): Scripts only. When provided, the viewer applies that run state after upload and compilation. When omitted, the viewer preserves the script's current run state and does not force it off.
- `success`: Whether the upload/save operation succeeded.
- `compiled` (optional): Scripts only. `true` when compilation succeeded, `false` when source saved but compile failed.
- `diagnostics` (optional): Scripts only. Compiler diagnostics when `compiled` is `false`.

> **Note:** Omitting `running` leaves the script's existing run state unchanged. Only an explicit `true` or `false` changes the post-save state.

**Permissions.** Requires `PERM_MODIFY` on the item and modify permission on the containing prim.
See [Common preconditions](#common-preconditions) for the shared checks and errors.

**Timeouts.** Scripts allow 60 seconds for upload and compilation; notecards allow 30 seconds. A
client's own timeout must exceed the longer of the two. Exceeding either fails the call with
`-32001`.

---

### ObjectItemCreate

**JSON-RPC Method:** `object.item.create` (call from extension to viewer)

Creates a new script or notecard in a prim's inventory. The call is asynchronous and returns the
created item's details once the simulator confirms the item exists. The simulator may rename the
item if a duplicate name exists.

```typescript
interface ObjectItemCreateParams {
  prim_id: string;           // UUID of the prim to create the item in
  name: string;              // Pure SL inventory name — no file extension
  type: InventoryItemType;   // "script" | "notecard"
  vm?: ScriptVM;             // Scripts only (required): "luau" | "mono" | "lsl2"
  text?: string;             // Notecards only (optional): initial body text
}

// On success, returns an ObjectInventoryItem with prim_id:
interface ObjectItemCreateResponse extends ObjectInventoryItem {
  prim_id: string;           // Echoed prim UUID
}
```

**Fields:**

- `prim_id`: UUID of the prim to create the item in. Child prims are addressable directly by UUID.
- `name`: Pure SL inventory name, without a file extension.
- `type`: `"script"` or `"notecard"`.
- `vm`: Scripts only. Required when `type` is `"script"`; accepted values are `"luau"`, `"mono"` and `"lsl2"`. Ignored for notecards.
- `text` (optional): Notecards only. Initial body text for the new notecard. Ignored for scripts.

**Notes:**
- The response matches the `ObjectInventoryItem` structure (same fields as items in
  `object.publish` and `object.update` notifications).
- The `name` in the response may differ from the request if the simulator renamed it.
- An `object.update` notification will also fire for the prim (since inventory changed).

**Errors:**

| Condition | Code | `error.message` |
| --------- | ---- | --------------- |
| `type` is not `"script"` or `"notecard"` | `-32602` | `Invalid params: Unsupported item type: <type>` |
| `prim_id` missing | `-32602` | `Invalid params: prim_id is required` |
| Prim not found | `-32602` | `Invalid params: Prim not found` |
| `name` missing | `-32602` | `Invalid params: name is required` |
| `vm` missing or invalid for a script | `-32602` | `Invalid params: vm must be 'luau', 'mono', or 'lsl2'` |
| Object is not published | `-32003` | `Object is not published` |
| Another `object.item.create` is already in flight for this prim | `-32600` | `Invalid Request: An item.create is already in flight for this prim` |
| Simulator did not respond within 30 seconds | `-32001` | `Timed out waiting for item creation` |

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

**Permissions.** Requires `PERM_MODIFY` on the item and modify permission on the containing prim.
See [Common preconditions](#common-preconditions) for the shared checks and errors.

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
}
```

**`success` means dispatched, not applied.** The viewer sends a message to the simulator and
returns immediately. `success: true` confirms the message was sent — it does not confirm the
simulator started or stopped the script. Confirmation, if it arrives, comes later as an
`object.update` notification. Treat the response as an acknowledgement and wait for the update
before reporting the new run state to the user.

**Permissions.** Requires `PERM_MODIFY` on the script. This method validates independently of the
shared item validator and accepts scripts only.

**Errors:**

| Condition | Code | `error.message` |
| --------- | ---- | --------------- |
| `prim_id` or `item_id` missing | `-32602` | `Invalid params: prim_id and item_id are required` |
| Prim not found | `-32602` | `Invalid params: Prim not found` |
| Object is not published | `-32003` | `Object is not published` |
| Script not in the prim's inventory | `-32602` | `Invalid params: Script not found in prim inventory` |
| Item is not a script | `-32602` | `Invalid params: Item is not a script` |
| No modify permission on the script | `-32003` | `No modify permission on script` |

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
}
```

**`success` means dispatched, not applied.** As with `object.script.set_running`, the viewer sends
a message to the simulator and returns immediately. `success: true` does not confirm the script
was reset.

**Permissions.** Requires `PERM_MODIFY` on the script. This method validates independently of the
shared item validator and accepts scripts only.

**Errors:** identical to `object.script.set_running` above.

---

### ObjectRequest

**JSON-RPC Method:** `object.request` (call from extension to viewer)

Requests the viewer to publish a specific in-world object. The viewer responds synchronously to confirm the request was accepted, then asynchronously sends an `object.publish` notification with the full object tree.

This is typically called immediately after the handshake completes when the extension was launched by the viewer with an `object=<uuid>` URI parameter.

```typescript
interface ObjectRequestParams {
  object_id: string;  // UUID of the root prim to request exploring
}

interface ObjectRequestResponse {
  success: boolean;
}
```

**Fields:**

- `object_id`: UUID of the root prim of the linkset to explore.
- `success`: Whether the viewer accepted the request. A `true` response does not mean `object.publish` has been sent yet — it means the viewer will send it. Failures are returned as JSON-RPC errors — see below.

**Permissions.** Requires modify permission on the object.

**Errors:**

| Condition | Code | `error.message` |
| --------- | ---- | --------------- |
| `object_id` missing | `-32602` | `Invalid params: No object_id specified` |
| Object not found | `-32602` | `Invalid params: Object not found` |
| No modify permission on the object | `-32003` | `Permission denied` |
| Publish could not be started | `-32603` | `Internal error: Failed to initiate publish` |

**Sequence:**
1. Extension calls `object.request`
2. Viewer responds with `{ success: true }` (or error)
3. Viewer sends `object.publish` notification (asynchronously, when ready)

---

### ObjectList

**JSON-RPC Method:** `object.list` (call from extension to viewer)

Requests the complete list of currently explored objects. Called by the extension immediately after the handshake completes (`session.ok`) to restore state for any objects the viewer already has open for exploration.

The viewer responds synchronously with all explored objects in the same format as `object.publish` notifications. No follow-up notifications are sent.

```typescript
// No request parameters

interface ObjectListResponse {
  objects: PublishedObject[];  // All currently explored objects; empty array if none
}
```

**Fields:**

- `objects`: Array of `PublishedObject` records (same shape as the `object` field in `object.publish`). Empty array when no objects are currently being explored.

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
}
```

**Fields:**

- `prim_id`: UUID of the prim to modify. Child prims are addressable directly by UUID.
- `name` (optional): New display name for the prim. If omitted, name remains unchanged.
- `description` (optional): New description for the prim. If omitted, description remains unchanged.
- `permissions` (optional): Permission changes.
  - `next_owner`: Permission mask applied when the object is transferred. Uses same bit flags as `ItemPermissions` (e.g., `PERM_MODIFY=0x4000`, `PERM_COPY=0x8000`, `PERM_TRANSFER=0x2000`).
- `success`: Whether the property messages were dispatched.

**`success` means dispatched, not applied.** Each supplied property is sent to the simulator as a
separate message and the viewer returns immediately. `success: true` confirms the messages were
sent, not that any of them took effect — and because they travel independently, one may be applied
while another is not. Confirmation arrives later as an `object.update` notification.

**Permissions.** Requires modify permission on the prim.

**Errors:**

| Condition | Code | `error.message` |
| --------- | ---- | --------------- |
| `prim_id` missing | `-32602` | `Invalid params: prim_id is required` |
| No property supplied | `-32602` | `Invalid params: At least one property (name, description, or permissions) must be specified` |
| Prim not found | `-32602` | `Invalid params: Prim not found` |
| Object is not published | `-32003` | `Object is not published` |
| No modify permission on the prim | `-32003` | `No modify permission on object` |

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
}
```

**Fields:**

- `prim_id`: UUID of the prim that owns the item. Child prims are addressable directly by UUID.
- `item_id`: Inventory item UUID.
- `name` (optional): New display name for the item. Should not include file extension (e.g., `.lsl`, `.luau`). If omitted, name remains unchanged.
- `description` (optional): New description for the item. If omitted, description remains unchanged.
- `permissions` (optional): Permission changes.
  - `next_owner`: Permission mask applied when the item is transferred. Uses same bit flags as `ItemPermissions` (e.g., `PERM_MODIFY=0x4000`, `PERM_COPY=0x8000`, `PERM_TRANSFER=0x2000`).
- `success`: Whether the property messages were dispatched.

**`success` means dispatched, not applied.** The viewer sends the change to the simulator and
returns immediately; confirmation arrives later as an `object.update` notification.

**Permissions.** Requires `PERM_MODIFY` on the item and modify permission on the containing prim.
See [Common preconditions](#common-preconditions) for the shared checks and errors.

**Errors:** as listed under [Common preconditions](#common-preconditions), plus `-32602` when no
property field is supplied.

**Notes:**
- At least one property field (`name`, `description`, or `permissions`) must be specified.
- An `object.update` notification will fire after successful modification.
- Owner permissions cannot be modified directly — only `next_owner` can be changed.
- If the item is renamed, the virtual filesystem path will change and the extension must handle the rename appropriately.

---

## Command Interfaces

These interfaces provide a general-purpose, bidirectional command channel. Either side may invoke a named command on the other side and receive a structured result. The feature is optional and must be negotiated via the `commands` flag in the session handshake.

### CommandExecute

**JSON-RPC Method:** `command.execute` (call, bidirectional)

Invokes a named command on the receiving side. Commands are identified by a namespaced string and carry an optional freeform parameter map.

```typescript
interface CommandExecuteParams {
  command: string;                       // namespaced command id, e.g. "viewer.teleport"
  params?: Record<string, unknown>;      // command-specific arguments
}

interface CommandExecuteResponse {
  success: boolean;
  result?: unknown;                      // optional command-specific return value
}
```

**Fields:**

- `command`: Namespaced command identifier. The prefix before the first `.` identifies the side that owns and executes the command:
  - `viewer.*` — commands executed by the viewer (e.g. `viewer.teleport`, `viewer.camera.focus`)
  - `editor.*` — commands executed by the extension (e.g. `editor.open_file`, `editor.show_message`)
- `params` (optional): Command-specific argument map. Structure varies by command.
- `success`: `true` when the command executed. Failures are returned as JSON-RPC errors — see below.
- `result` (optional): Command-specific return value. Only present when the command produces output.

**Errors:**

| Condition | Code | `error.message` |
| --------- | ---- | --------------- |
| `command` missing | `-32602` | `Invalid params: command is required` |
| Command not registered | `-32602` | `Invalid params: Unknown command: <command>` |

The invoked command's own handler may raise further errors — `-32602` for bad arguments,
`-32003` when the action is not permitted, `-32603` on internal failure. Clients must handle any
error code, not only the two above.

**Capability gate:** A side MUST NOT send `command.execute` unless the peer advertised
`commands: true` in the handshake. A receiver that receives the call without having negotiated the
feature should respond with a JSON-RPC error. **Not currently enforced on receive by the viewer**
— the gate is applied only when sending. Implementation is tracked separately.

**Example — extension asks viewer to teleport:**

```json
{
  "jsonrpc": "2.0",
  "method": "command.execute",
  "id": 7,
  "params": {
    "command": "viewer.teleport",
    "params": { "object_id": "550e8400-e29b-41d4-a716-446655440000" }
  }
}
```

```json
{
  "jsonrpc": "2.0",
  "id": 7,
  "result": { "success": true }
}
```

**Example — extension asks viewer to save object back to contents:**

```json
{
  "jsonrpc": "2.0",
  "method": "command.execute",
  "id": 9,
  "params": {
    "command": "viewer.object.save_back_to_contents",
    "params": { "object_id": "550e8400-e29b-41d4-a716-446655440000" }
  }
}
```

```json
{
  "jsonrpc": "2.0",
  "id": 9,
  "result": { "success": true, "result": { "object_id": "550e8400-e29b-41d4-a716-446655440000" } }
}
```

**Example — viewer asks extension to show a message:**

```json
{
  "jsonrpc": "2.0",
  "method": "command.execute",
  "id": 8,
  "params": {
    "command": "editor.show_message",
    "params": { "message": "Script reset complete", "level": "info" }
  }
}
```

---

### CommandList

**JSON-RPC Method:** `command.list` (call, bidirectional)

Requests the list of commands the receiving side supports. Intended for tooling and autocomplete; implementations may omit this method and return an error response if discovery is not needed.

This method takes no parameters.

**Response:**

```typescript
interface CommandListResponse {
  commands: CommandInfo[];
}

interface CommandInfo {
  command: string;
  description?: string;
  params?: Record<string, CommandParamInfo>;
}

interface CommandParamInfo {
  type: "string" | "number" | "boolean" | "object" | "array";
  required?: boolean;
  description?: string;
}
```

**Response Fields:**

- `commands`: Array of commands the responder supports. Each entry describes one command.
  - `command`: The namespaced command identifier.
  - `description` (optional): Human-readable description of what the command does.
  - `params` (optional): Map of parameter names to their type descriptors. **Not currently
    populated** — the viewer returns only `command` and `description`, so parameter discovery
    does not work. Implementation is tracked separately.

**Known viewer commands:**

| Command | Required params | Description |
|---------|----------------|-------------|
| `viewer.teleport` | `object_id: string` | Teleport agent to an in-world object. |
| `viewer.camera.focus` | `object_id: string` | Zoom camera to an in-world object (same behavior as context menu Zoom In). |
| `viewer.object.save_back_to_contents` | `object_id: string` | Save an in-world object back to source object contents. |

**Known extension commands:**

| Command | Required params | Description |
|---------|----------------|-------------|
| `editor.open_file` | `path: string` | Open a file in the editor. Optional `line: number`. |
| `editor.show_message` | `message: string` | Show a notification. Optional `level: "info" \| "warn" \| "error"`. |
