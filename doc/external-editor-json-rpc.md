# Viewer to External Editor JSON-RPC<br>Message Interfaces Documentation

This document describes all the message interfaces defined in for WebSocket communication between the Second Life viewer and an external editor such as a VSCode extension.

## Table of Contents

- [Usage Flow](#usage-flow)
- [JSON-RPC Method Summary](#json-rpc-method-summary)
- [Session Management Interfaces](#session-management-interfaces)
  - [SessionHandshake](#sessionhandshake)
  - [SessionHandshakeResponse](#sessionhandshakeresponse)
  - [Session OK](#session-ok)
  - [SessionDisconnect](#sessiondisconnect)
- [Language and Syntax Interfaces](#language-and-syntax-interfaces)
  - [SyntaxChange](#syntaxchange)
  - [Language Syntax ID Request](#language-syntax-id-request)
  - [Language Syntax Request](#language-syntax-request)
- [Script Subscription Interfaces](#script-subscription-interfaces)
  - [ScriptSubscribe](#scriptsubscribe)
  - [ScriptSubscribeResponse](#scriptsubscriberesponse)
  - [ScriptUnsubscribe](#scriptunsubscribe)
- [Compilation Interfaces](#compilation-interfaces)
  - [CompilationError](#compilationerror)
  - [CompilationResult](#compilationresult)
- [Runtime Event Interfaces](#runtime-event-interfaces)
  - [RuntimeDebug](#runtimedebug)
  - [RuntimeError](#runtimeerror)
- [Handler and Configuration Interfaces](#handler-and-configuration-interfaces)
  - [WebSocketHandlers](#websockethandlers)
  - [ClientInfo](#clientinfo)

## Usage Flow

1. **Connection Establishment:**

   - Viewer sends `session.handshake` notification with `SessionHandshake` data
   - Extension responds with `SessionHandshakeResponse`
   - Viewer confirms with `session.ok` notification

2. **Language Information Exchange:**

   - Extension makes `language.syntax.id` call to get current syntax version
   - Extension makes `language.syntax` calls with different `kind` parameters to get specific language data
   - Viewer responds with `LanguageInfo` data containing the requested information

3. **Script Subscription Management:**

   - Extension makes `script.subscribe` call with `ScriptSubscribe` data to request live synchronization for a script
   - Viewer responds with `ScriptSubscribeResponse` indicating success or failure
   - When subscription needs to be terminated, viewer sends `script.unsubscribe` notification with `ScriptUnsubscribe` data
   - Extension handles unsubscription by cleaning up local script tracking

4. **Runtime Events:**

   - Viewer sends `language.syntax.change` notification with `SyntaxChange` when language changes
   - Viewer sends `script.compiled` notification with `CompilationResult` after script compilation
   - Viewer sends `runtime.debug` notification with `RuntimeDebug` for debug messages during script execution
   - Viewer sends `runtime.error` notification with `RuntimeError` when runtime errors occur

5. **Connection Termination:**
   - Either side can send `session.disconnect` notification with `SessionDisconnect` data
   - Connection is closed gracefully

## JSON-RPC Method Summary

| Method                          | Direction          | Type         | Interface/Parameters       |
| ------------------------------- | ------------------ | ------------ | -------------------------- |
| `session.handshake`             | Viewer → Extension | Notification | `SessionHandshake`         |
| `session.handshake` (response)  | Extension → Viewer | Response     | `SessionHandshakeResponse` |
| `session.ok`                    | Viewer → Extension | Notification | _(no interface)_           |
| `session.disconnect`            | Bidirectional      | Notification | `SessionDisconnect`        |
| `script.subscribe`              | Extension → Viewer | Call         | `ScriptSubscribe`          |
| `script.subscribe` (response)   | Viewer → Extension | Response     | `ScriptSubscribeResponse`  |
| `script.unsubscribe`            | Viewer → Extension | Notification | `ScriptUnsubscribe`        |
| `language.syntax.id`            | Extension → Viewer | Call         | _(no parameters)_          |
| `language.syntax.id` (response) | Viewer → Extension | Response     | `{ id: string }`           |
| `language.syntax`               | Extension → Viewer | Call         | `{ kind: string }`         |
| `language.syntax` (response)    | Viewer → Extension | Response     | `LanguageInfo`             |
| `language.syntax.change`        | Viewer → Extension | Notification | `SyntaxChange`             |
| `script.compiled`               | Viewer → Extension | Notification | `CompilationResult`        |
| `runtime.debug`                 | Viewer → Extension | Notification | `RuntimeDebug`             |
| `runtime.error`                 | Viewer → Extension | Notification | `RuntimeError`             |

## Session Management Interfaces

### SessionHandshake

**JSON-RPC Method:** `session.handshake` (notification from viewer)

The initial handshake message sent by the viewer to establish a connection.

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
- `challenge` (optional): Security challenge string for authentication
- `languages`: Array of supported scripting languages (e.g., ["lsl", "luau"])
- `syntax_id`: Current active syntax/language identifier
- `features`: Dictionary of feature flags indicating viewer capabilities

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
}
```

**Fields:**

- `client_name`: Name of the client (VS Code extension)
- `client_version`: Fixed version "1.0" of the client
- `protocol_version`: Protocol version the client supports
- `challenge_response` (optional): Response to the security challenge if provided
- `languages`: Array of languages supported by the client
- `features`: Dictionary of features supported by the client

### Session OK

**JSON-RPC Method:** `session.ok` (notification from viewer)

Confirmation notification sent by the viewer after successful handshake completion. This interface has no defined structure as it appears to be a simple confirmation message.

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

- `reason`: Numeric code indicating the reason for disconnection
- `message`: Human-readable description of the disconnect reason

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

- `id`: Identifier for the new syntax/language

### Language Syntax ID Request

**JSON-RPC Method:** `language.syntax.id` (call from extension to viewer)

Requests the current active language syntax identifier from the viewer. This method takes no parameters.

**Response:** Returns an object with an `id` field containing the current syntax identifier.

### Language Syntax Request

**JSON-RPC Method:** `language.syntax` (call from extension to viewer)

Requests detailed syntax information for a specific language kind.

**Parameters:**

```typescript
{
  kind: string; // The type of syntax information requested
}
```

**Fields:**

- `kind`: The type of syntax information to retrieve (e.g., "functions", "constants", "events", "types.luau")

**Response:** Returns `LanguageInfo` data containing the requested syntax information:

```typescript
interface LanguageInfo {
  id: string;
  lslDefs?: {
    controls?: any;
    types?: any;
    constants?: { [name: string]: ConstantDef };
    events?: { [name: string]: FunctionDef };
    functions?: { [name: string]: FunctionDef };
  };
  luaDefs?: {
    modules?: { [name: string]: TypeDef };
    classes?: { [name: string]: TypeDef };
    aliases?: { [name: string]: TypeDef };
    functions?: { [name: string]: FunctionDef };
  };
}
```

**Response Fields:**

- `id`: Version identifier for the language syntax
- `lslDefs` (optional): LSL-specific language definitions containing:
  - `controls` (optional): Control flow and language constructs
  - `types` (optional): LSL type definitions
  - `constants` (optional): Object containing constant definitions keyed by constant name
  - `events` (optional): Object containing event definitions keyed by event name
  - `functions` (optional): Object containing function definitions keyed by function name
- `luaDefs` (optional): Lua-specific language definitions containing:
  - `modules` (optional): Module type definitions keyed by module name
  - `classes` (optional): Class type definitions keyed by class name
  - `aliases` (optional): Type alias definitions keyed by alias name
  - `functions` (optional): Function definitions keyed by function name

The specific sections returned depend on the `kind` parameter and the active language context.

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
  object_name?: string;
  item_id?: string;
  message?: string;
}
```

**Fields:**

- `script_id`: The script identifier that was subscribed to
- `success`: Whether the subscription was successful
- `status`: Numeric status code indicating the result
- `object_id` (optional): The in-world ID of the object containing the script
- `object_name` (optional): The name of the object containing the script.
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

## Compilation Interfaces

### CompilationError

Individual compilation error record.

```typescript
interface CompilationError {
  row: number;
  column: number;
  level: "ERROR";
  message: string;
}
```

**Fields:**

- `row`: Line number where the error occurred (0-based or 1-based depending on context)
- `column`: Column position of the error
- `level`: Severity level (currently only "ERROR" is defined)
- `message`: Error description

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
- `message`: Error message description
- `error`: Specific error type or code
- `line`: Line number where the error occurred
- `stack` (optional): Stack trace information if available

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
- `onSubscribe`: Handler for script subscription requests from viewer, returns subscription response
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

