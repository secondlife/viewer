/**
 * @file llscripteditorws.cpp
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

/*
 * ============================================================================
 * WEBSOCKET MESSAGE FORMAT SPECIFICATION
 * ============================================================================
 *
 * This WebSocket implementation uses a structured JSON message format for
 * communication between the Second Life viewer and external script editors.
 * All messages are encoded as UTF-8 JSON strings.
 *
 * ## BASE MESSAGE STRUCTURE
 *
 * Every WebSocket message follows this mandatory structure:
 *
 * ```json
 * {
 *   "command": "message_type_identifier",    // REQUIRED: Command/message type
 *   "data": {                                // OPTIONAL: Command-specific payload
 *     // ... command-specific fields ...
 *   },
 *   "timestamp": "2025-01-27T15:30:00Z",    // OPTIONAL: ISO8601 timestamp
 *   "session_id": "uuid-string",             // OPTIONAL: Session identifier
 *   "message_id": "unique-id"                // OPTIONAL: For request/response correlation
 * }
 * ```
 *
 * ## MESSAGE TYPES
 *
 * ### 1. SERVER-TO-CLIENT MESSAGES
 *
 * #### capabilities
 * Sent immediately when client connects to announce server capabilities.
 * ```json
 * {
 *   "command": "capabilities",
 *   "data": {
 *     "server_name": "Second Life Script Editor WebSocket Server",
 *     "server_version": "1.0.0",
 *     "protocol_version": "1.0",
 *     "viewer_name": "Second Life",
 *     "viewer_version": "Second Life Release 6.4.26",
 *     "session_id": "generated-uuid",
 *     "timestamp": "2025-01-27T15:30:00Z",
 *     "capabilities": ["script_editing", "compilation", "metadata", ...],
 *     "supported_languages": ["lsl", "luau"],
 *     "features": {
 *       "live_compilation": true,
 *       "debugging": false,
 *       "breakpoints": false
 *     }
 *   }
 * }
 * ```
 *
 * #### error
 * Sent when message processing fails or protocol violations occur.
 * ```json
 * {
 *   "command": "error",
 *   "data": {
 *     "error_code": "PARSE_ERROR|INVALID_MESSAGE|UNKNOWN_MESSAGE_TYPE",
 *     "error_message": "Human-readable error description"
 *   }
 * }
 * ```
 *
 * ### 2. CLIENT-TO-SERVER MESSAGES
 *
 * #### connect
 * Client handshake message to establish connection and negotiate capabilities.
 * ```json
 * {
 *   "command": "connect",
 *   "data": {
 *     "client_name": "VS Code LSL Extension",
 *     "client_version": "1.2.3",
 *     "protocol_version": "1.0",
 *     "capabilities": ["script_editing", "syntax_highlighting"],
 *     "supported_languages": ["lsl", "luau"],
 *     "features": {
 *       "auto_completion": true,
 *       "live_preview": false
 *     },
 *     "session_id": "optional-client-provided-uuid"
 *   }
 * }
 * ```
 *
 * ### 3. BIDIRECTIONAL MESSAGES
 *
 * #### connect_ack
 * Server response to client connect message.
 * ```json
 * {
 *   "command": "connect_ack",
 *   "data": {
 *     "status": "connected|rejected",
 *     "message": "Connection established successfully",
 *     "session_id": "established-session-uuid",
 *     "timestamp": "2025-01-27T15:30:00Z",
 *     "mutual_capabilities": ["script_editing", "compilation"],
 *     "supported_languages": ["lsl", "luau"],
 *     "max_script_size": 65536,
 *     "heartbeat_interval": 30,
 *     // For rejected connections:
 *     "error": "incompatible_protocol|missing_capabilities|connection_failed"
 *   }
 * }
 * ```
 *
 * ## FUTURE MESSAGE TYPES (Planned)
 *
 * The following message types are referenced in the protocol design but not
 * yet implemented:
 *
 * - `script_updated`: Editor notifies viewer of script content changes
 * - `save_request`: Editor requests script save to SL servers
 * - `compile_request`: Editor requests script compilation
 * - `script_content`: Viewer sends full script content to editor
 * - `compile_result`: Viewer sends compilation results to editor
 * - `save_result`: Viewer sends save operation results to editor
 * - `metadata`: Script and object metadata exchange
 * - `ping`/`pong`: Connection health check messages
 * - `editor_ready`: Editor initialization complete notification
 *
 * ## PROTOCOL RULES
 *
 * 1. **Encoding**: All messages must be valid UTF-8 JSON
 * 2. **Required Fields**: Every message MUST have a "command" field
 * 3. **Case Sensitivity**: All field names are case-sensitive
 * 4. **Protocol Version**: Currently only "1.0" is supported
 * 5. **Error Handling**: Invalid messages trigger "error" responses
 * 6. **Connection Flow**: Client must send "connect" before other commands
 * 7. **Session Management**: session_id tracks individual editor sessions
 * 8. **Capability Negotiation**: Features limited to mutual capabilities
 *
 * ## ERROR CODES
 *
 * - `PARSE_ERROR`: Invalid JSON syntax
 * - `PARSE_EXCEPTION`: JSON parsing threw exception
 * - `INVALID_MESSAGE`: Missing required "command" field
 * - `UNKNOWN_MESSAGE_TYPE`: Unrecognized command type
 * - `INCOMPATIBLE_PROTOCOL`: Unsupported protocol version
 * - `MISSING_CAPABILITIES`: Required capabilities not supported
 * - `CONNECTION_FAILED`: General connection establishment failure
 *
 * ## IMPLEMENTATION NOTES
 *
 * - Messages are parsed using boost::json and converted to LLSD internally
 * - All timestamps use ISO 8601 format in UTC timezone
 * - UUIDs are generated using LLUUID::generateNewID() for session tracking
 * - Maximum script size is currently limited to 65536 bytes
 * - WebSocket server binds to localhost only for security by default
 * - Protocol designed for extensibility with additional message types
 *
 * ============================================================================
 */


#include "llviewerprecompiledheaders.h"
#include "llscripteditorws.h"
#include "llpreviewscript.h"
#include "llappviewer.h"
#include "lltrans.h"
#include "lldate.h"
#include "llerror.h"
#include "lluuid.h"
#include "llsdjson.h"
#include <boost/json.hpp>

//------------------------------------------------------------------------

LLScriptEditorWSServer::LLScriptEditorWSServer(const std::string_view name, U16 port, bool local_only):
    LLWebsocketMgr::WSServer(name, port, local_only)
{
}

LLWebsocketMgr::WSConnection::ptr_t LLScriptEditorWSServer::connectionFactory(WSServer::ptr_t server, LLWebsocketMgr::connection_h handle)
{
    return std::make_shared<LLScriptEditorWSConnection>(server, handle);
}

//------------------------------------------------------------------------
void LLScriptEditorWSServer::onConnectionOpened(const LLWebsocketMgr::WSConnection::ptr_t& connection)
{
    LL_INFOS("ScriptEditorWS") << "New script editor client connected" << LL_ENDL;

    // Build capabilities message to send to the newly connected external editor
    LLSD capabilities_message;
    capabilities_message["command"] = "capabilities";

    LLSD& data = capabilities_message["data"];

    // Server identification and version information
    data["server_name"] = "Second Life Script Editor WebSocket Server";
    data["server_version"] = "1.0.0";
    data["protocol_version"] = "1.0";

    // Viewer information
    data["viewer_name"] = LLTrans::getString("APP_NAME");
    data["viewer_version"] = LLAppViewer::instance()->getSecondLifeTitle();

    // Session information
    data["session_id"] = LLUUID::generateNewID().asString();
    data["timestamp"] = LLDate::now().asString();

    // Server capabilities - what the viewer/server supports
    LLSD capabilities = LLSD::emptyArray();
    capabilities.append("script_editing");          // Basic script content editing
    capabilities.append("script_synchronization");  // Real-time sync between viewer and editor
    capabilities.append("compilation");             // Compile results with errors/warnings
    capabilities.append("metadata");                // Script and object metadata
    capabilities.append("syntax_highlighting");     // LSL/Luau syntax information
    capabilities.append("error_reporting");         // Detailed error reporting

    data["capabilities"] = capabilities;

    // Language support information
    LLSD languages = LLSD::emptyArray();
    languages.append("lsl");    // Linden Scripting Language
    languages.append("luau");   // Luau scripting language
    data["supported_languages"] = languages;

    // Feature flags
    LLSD features;
    features["live_compilation"] = true;
    features["debugging"] = false;        // Not implemented yet
    features["breakpoints"] = false;      // Not implemented yet
    data["features"] = features;

    // Send the capabilities message to the newly connected client
    if (connection->sendMessage(capabilities_message))
    {
        LL_INFOS("ScriptEditorWS") << "Sent capabilities message to new client" << LL_ENDL;
    }
    else
    {
        LL_WARNS("ScriptEditorWS") << "Failed to send capabilities message to new client" << LL_ENDL;
    }
}

void LLScriptEditorWSServer::onConnectionClosed(const LLWebsocketMgr::WSConnection::ptr_t& connection)
{
    LL_INFOS("ScriptEditorWS") << "Script editor client disconnected" << LL_ENDL;

    // Remove from active connections
    auto script_connection = std::dynamic_pointer_cast<LLScriptEditorWSConnection>(connection);
    if (script_connection)
    {
        mActiveConnections.erase(script_connection);

        // Remove from any script associations
        LL_INFOS("ScriptEditorWS") << "Removed connection from active connections. Total: "
                                   << mActiveConnections.size() << LL_ENDL;
    }
}

//------------------------------------------------------------------------
bool LLScriptEditorWSServer::associateEditor(const LLHandle<LLPanel>& editor_handle, const std::string& script_id)
{
    if (!editor_handle.isDead())
    {
        mScriptEditors[script_id] = editor_handle;
        return true;
    }
    return false;
}

void LLScriptEditorWSServer::dissociateEditor(const std::string& script_id)
{
    mScriptEditors.erase(script_id);
}

LLHandle<LLPanel> LLScriptEditorWSServer::findEditorForScript(const std::string& script_id) const
{
    auto it = mScriptEditors.find(script_id);
    if (it != mScriptEditors.end())
    {
        return it->second;
    }
    return LLHandle<LLPanel>();
}

//========================================================================
void LLScriptEditorWSConnection::onOpen()
{

}

void LLScriptEditorWSConnection::onClose()
{
}

void LLScriptEditorWSConnection::onMessage(const std::string& message)
{
    LL_DEBUGS("ScriptEditorWS") << "Received message: " << message << LL_ENDL;

    // Convert JSON string to LLSD
    LLSD parsed_message;
    try
    {
        boost::system::error_code ec;
        boost::json::value json_value = boost::json::parse(message, ec);

        if (ec.failed())
        {
            LL_WARNS("ScriptEditorWS") << "Failed to parse JSON message: " << ec.message() << LL_ENDL;

            // Send error response back to client
            LLSD error_response;
            error_response["command"] = "error";
            error_response["data"]["error_code"] = "PARSE_ERROR";
            error_response["data"]["error_message"] = "Invalid JSON format: " + std::string(ec.message());
            sendMessage(error_response);
            return;
        }

        // Convert boost::json::value to LLSD
        parsed_message = LlsdFromJson(json_value);

        LL_DEBUGS("ScriptEditorWS") << "Parsed LLSD message type: " << parsed_message.type()
                                    << ", has 'type' field: " << parsed_message.has("command") << LL_ENDL;
    }
    catch (const std::exception& e)
    {
        LL_WARNS("ScriptEditorWS") << "Exception parsing JSON message: " << e.what() << LL_ENDL;

        // Send error response back to client
        LLSD error_response;
        error_response["command"] = "error";
        error_response["data"]["error_code"] = "PARSE_EXCEPTION";
        error_response["data"]["error_message"] = "JSON parsing exception: " + std::string(e.what());
        sendMessage(error_response);
        return;
    }

    // Validate that we have a proper message structure
    if (!parsed_message.has("command"))
    {
        LL_WARNS("ScriptEditorWS") << "Received message without 'type' field" << LL_ENDL;

        LLSD error_response;
        error_response["command"] = "error";
        error_response["data"]["error_code"] = "INVALID_MESSAGE";
        error_response["data"]["error_message"] = "Message must have a 'type' field";
        sendMessage(error_response);
        return;
    }

    std::string message_type = parsed_message["command"].asString();
    LL_INFOS("ScriptEditorWS") << "Processing message of type: " << message_type << LL_ENDL;

    // Route message to appropriate handler based on type
    if (message_type == "connect")
    {
        processConnectMessage(parsed_message);
    }
    else
    {
        LL_WARNS("ScriptEditorWS") << "Received unknown message type: " << message_type << LL_ENDL;

        LLSD error_response;
        error_response["command"] = "error";
        error_response["data"]["error_code"] = "UNKNOWN_MESSAGE_TYPE";
        error_response["data"]["error_message"] = "Unknown message type: " + message_type;
        sendMessage(error_response);
    }
}

void LLScriptEditorWSConnection::processConnectMessage(const LLSD& message)
{
    LL_INFOS("ScriptEditorWS") << "Processing connect message from client" << LL_ENDL;

    // Extract connection information from the message
    LLSD data;
    if (message.has("data"))
    {
        data = message["data"];
    }

    // Parse client information
    std::string client_name;
    std::string client_version;
    std::string protocol_version;
    LLSD client_capabilities;
    LLSD supported_languages;
    LLSD client_features;

    // Extract client identification
    if (data.has("client_name"))
    {
        client_name = data["client_name"].asString();
        LL_INFOS("ScriptEditorWS") << "Client name: " << client_name << LL_ENDL;
    }

    if (data.has("client_version"))
    {
        client_version = data["client_version"].asString();
        LL_INFOS("ScriptEditorWS") << "Client version: " << client_version << LL_ENDL;
    }

    if (data.has("protocol_version"))
    {
        protocol_version = data["protocol_version"].asString();
        LL_INFOS("ScriptEditorWS") << "Protocol version: " << protocol_version << LL_ENDL;
    }

    // Extract client capabilities
    if (data.has("capabilities"))
    {
        client_capabilities = data["capabilities"];
        mEditorCapabilities = client_capabilities; // Store for later use
        LL_INFOS("ScriptEditorWS") << "Client capabilities count: " << client_capabilities.size() << LL_ENDL;
    }

    // Extract supported languages
    if (data.has("supported_languages"))
    {
        supported_languages = data["supported_languages"];
        LL_INFOS("ScriptEditorWS") << "Supported languages count: " << supported_languages.size() << LL_ENDL;
    }

    // Extract client features
    if (data.has("features"))
    {
        client_features = data["features"];
        LL_INFOS("ScriptEditorWS") << "Client features available" << LL_ENDL;
    }

    // Generate or extract editor session ID
    if (data.has("session_id"))
    {
        mEditorId = data["session_id"].asString();
        LL_INFOS("ScriptEditorWS") << "Using client session ID: " << mEditorId << LL_ENDL;
    }
    else
    {
        // Generate a new session ID if client didn't provide one
        mEditorId = LLUUID::generateNewID().asString();
        LL_INFOS("ScriptEditorWS") << "Generated session ID: " << mEditorId << LL_ENDL;
    }

    // Validate protocol compatibility
    bool protocol_compatible = true;
    if (!protocol_version.empty())
    {
        // For now, we only support protocol version "1.0"
        if (protocol_version != "1.0")
        {
            protocol_compatible = false;
            LL_WARNS("ScriptEditorWS") << "Unsupported protocol version: " << protocol_version
                                       << ", expected: 1.0" << LL_ENDL;
        }
    }

    // Determine if we have feature compatibility
    bool has_script_editing = false;
    if (client_capabilities.isArray())
    {
        for (LLSD::array_const_iterator it = client_capabilities.beginArray();
             it != client_capabilities.endArray(); ++it)
        {
            if (it->asString() == "script_editing")
            {
                has_script_editing = true;
                break;
            }
        }
    }

    // Build response message
    LLSD response;
    response["command"] = "connect_ack";

    LLSD& response_data = response["data"];

    if (protocol_compatible && has_script_editing)
    {
        // Successful connection
        response_data["status"] = "connected";
        response_data["message"] = "Connection established successfully";
        response_data["session_id"] = mEditorId;
        response_data["timestamp"] = LLDate::now().asString();

        // Send back our supported capabilities that match the client's
        LLSD mutual_capabilities = LLSD::emptyArray();

        // Check which capabilities we both support
        if (client_capabilities.isArray())
        {
            // Our server capabilities (from onConnectionOpened)
            std::set<std::string> server_caps = {
                "script_editing", "script_synchronization", "compilation",
                "metadata", "syntax_highlighting", "error_reporting"
            };

            for (LLSD::array_const_iterator it = client_capabilities.beginArray();
                 it != client_capabilities.endArray(); ++it)
            {
                std::string cap = it->asString();
                if (server_caps.count(cap) > 0)
                {
                    mutual_capabilities.append(cap);
                }
            }
        }

        response_data["mutual_capabilities"] = mutual_capabilities;

        // Send supported languages intersection
        LLSD mutual_languages = LLSD::emptyArray();
        if (supported_languages.isArray())
        {
            std::set<std::string> server_languages = {"lsl", "luau"};

            for (LLSD::array_const_iterator it = supported_languages.beginArray();
                 it != supported_languages.endArray(); ++it)
            {
                std::string lang = it->asString();
                if (server_languages.count(lang) > 0)
                {
                    mutual_languages.append(lang);
                }
            }
        }
        else
        {
            // Default to all our supported languages if client didn't specify
            mutual_languages.append("lsl");
            mutual_languages.append("luau");
        }

        response_data["supported_languages"] = mutual_languages;

        // Connection limits and constraints
        response_data["max_script_size"] = 65536;
        response_data["heartbeat_interval"] = 30;

        LL_INFOS("ScriptEditorWS") << "Successfully connected client: " << client_name
                                   << " v" << client_version
                                   << " with " << mutual_capabilities.size() << " mutual capabilities" << LL_ENDL;
    }
    else
    {
        // Connection failed
        response_data["status"] = "rejected";

        if (!protocol_compatible)
        {
            response_data["error"] = "incompatible_protocol";
            response_data["message"] = "Unsupported protocol version: " + protocol_version;
        }
        else if (!has_script_editing)
        {
            response_data["error"] = "missing_capabilities";
            response_data["message"] = "Client must support 'script_editing' capability";
        }
        else
        {
            response_data["error"] = "connection_failed";
            response_data["message"] = "Connection failed for unknown reason";
        }

        LL_WARNS("ScriptEditorWS") << "Rejected connection from client: " << client_name
                                   << " - " << response_data["message"].asString() << LL_ENDL;
    }

    // Send the response back to the client
    if (sendMessage(response))
    {
        LL_INFOS("ScriptEditorWS") << "Sent connect acknowledgment to client" << LL_ENDL;
    }
    else
    {
        LL_WARNS("ScriptEditorWS") << "Failed to send connect acknowledgment to client" << LL_ENDL;
    }

    // If connection was successful, we could also send any initial state or configuration
    if (response_data["status"].asString() == "connected")
    {
        // TODO: Send initial script list, active editors, or other relevant state
        // Example: sendScriptList(), sendActiveEditors(), etc.
    }
}
