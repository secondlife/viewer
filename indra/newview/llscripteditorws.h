/**
 * @file llscripteditorws.h
 * @brief WebSocket server and connection classes for external script editor integration
 *
 * $LicenseInfo:firstyear=2025&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2025, Linden Research, Inc.
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

#pragma once

#include "lljsonrpcws.h"
#include "llsd.h"
#include "lluuid.h"
#include "llhandle.h"
#include "lltimer.h"

#include <memory>
#include <string>
#include <map>
#include <set>

// Forward declarations
class LLLiveLSLEditor;
class LLScriptEdContainer;
class LLScriptEditorWSServer;

/**
 * @class LLScriptEditorWSConnection
 * @brief JSON-RPC WebSocket connection specialized for external script editor communication
 *
 * This class handles JSON-RPC 2.0 communication between the Second Life viewer
 * and external script editors. It provides a clean base for implementing
 * script editor integration using the standard JSON-RPC 2.0 protocol.
 *
 * ## Usage
 *
 * @code
 * // Create server and let base JSON-RPC handle method registration
 * auto server = std::make_shared<LLScriptEditorWSServer>("script_editor_server", 9020);
 *
 * // Register custom methods as needed
 * connection->registerMethod("custom.method", handler);
 * @endcode
 */
class LLScriptEditorWSConnection : public LLJSONRPCConnection,
    public std::enable_shared_from_this<LLScriptEditorWSConnection>
{
public:
    enum DisconnectReason
    {
        REASON_NORMAL = 0,
        REASON_EDITOR_CLOSED = 1,
        REASON_PROTOCOL_ERROR = 2,
        REASON_TIMEOUT = 3,
        REASON_INTERNAL_ERROR = 4
    };

    LLScriptEditorWSConnection(const LLWebsocketMgr::WSServer::ptr_t server,
                              const LLWebsocketMgr::connection_h& handle)
        : LLJSONRPCConnection(server, handle)
    { }

    ~LLScriptEditorWSConnection() override = default;


    // Connection lifecycle overrides
    void onOpen() override;
    void onClose() override;

    /**
     * @brief Send session disconnect message to the external editor
     * @param reason Numeric reason code for the disconnect (default 0 for normal closure)
     * @param message Human-readable disconnect message (default "Goodbye")
     */
    void sendDisconnect(S32 reason = 0, const std::string& message = "Goodbye");

private:
    using string_set_t = std::set<std::string>;
    /**
     * @brief Handle the handshake response from the client
     * @param result The response data from the client containing client information
     */
    void handleHandshakeResponse(const LLSD& result);

    bool connectToEditor(const std::string& script_id);
    void cleanupConnection();

    LLScriptEdContainer* getEditor() const;
    std::shared_ptr<LLScriptEditorWSServer> getServer() const;

    std::string mEditorId;              ///< Unique identifier for this editor session
    LLSD mEditorCapabilities;           ///< Editor capabilities metadata
    std::string mScriptId;              ///< Unique identifier for the script being edited
    bool mEditorReady;                  ///< Whether editor has completed initialization
    LLHandle<LLPanel> mEditorPanel;     ///< Handle to the associated LSL editor panel

    // Client handshake response data
    std::string mClientName;            ///< Name of the external editor client
    std::string mClientVersion;         ///< Version of the external editor client
    std::string mProtocolVersion;       ///< JSON-RPC protocol version supported by client
    std::string mScriptName;            ///< Name of the script being edited
    std::string mScriptLanguage;        ///< Programming language of the script (lsl, luau, etc.)
    string_set_t mLanguages;            ///< Set of supported scripting languages
    string_set_t mFeatures;             ///< Active client features (live_sync, compilation, etc.)
};

/**
 * @class LLScriptEditorWSServer
 * @brief JSON-RPC 2.0 WebSocket server for external script editor integration
 *
 * This server extends the JSON-RPC server to provide specialized functionality
 * for external script editor integration. It manages WebSocket connections from
 * external script editors and provides a structured JSON-RPC 2.0 interface
 * between the Second Life viewer's script editing functionality and external
 * development tools.
 *
 * ## Architecture
 *
 * The server acts as a JSON-RPC communication hub between:
 * - LLLiveLSLEditor instances (in-world script editing)
 * - External script editors (VS Code, Atom, Sublime Text, etc.)
 * - Script compilation and save services
 *
 * ## Usage
 *
 * @code
 * // Create and start the JSON-RPC server
 * auto server = std::make_shared<LLScriptEditorWSServer>("script_editor_server", 9020);
 * LLWebsocketMgr::getInstance()->addServer(server);
 * LLWebsocketMgr::getInstance()->startServer("script_editor_server");
 *
 * // Associate with an LSL editor
 * server->associateEditor(editor_handle, script_id);
 * @endcode
 *
 * ## Security Considerations
 *
 * - Server binds to localhost only by default for security
 * - JSON-RPC 2.0 structured protocol with validation
 * - Rate limiting handled by base JSON-RPC server
 * - Error handling with standardized JSON-RPC error codes
 */
class LLScriptEditorWSServer : public LLJSONRPCServer
{
public:
    static constexpr char const* DEFAULT_SERVER_NAME = "script_editor_server";
    static constexpr U16         DEFAULT_SERVER_PORT = 9020;

    using ptr_t = std::shared_ptr<LLScriptEditorWSServer>;

    LLScriptEditorWSServer(const std::string& name, U16 port, bool local_only = true);

    virtual ~LLScriptEditorWSServer() = default;

    // Server lifecycle callbacks
    void onConnectionOpened(const LLWebsocketMgr::WSConnection::ptr_t& connection) override;
    void onConnectionClosed(const LLWebsocketMgr::WSConnection::ptr_t& connection) override;

    bool associateEditor(const LLHandle<LLPanel>& editor_handle, const std::string& script_id);
    void dissociateEditor(const std::string& script_id);

    LLHandle<LLPanel>                           findEditorForScript(const std::string& script_id) const;
    std::shared_ptr<LLScriptEditorWSConnection> findConnectionForScript(const std::string& script_id);

    /**
     * @brief Get list of active script editing sessions
     * @return Set of script IDs currently being edited
     */
    std::set<std::string> getActiveScripts() const;

    /**
     * @brief Send script content to all connected editors for a specific script
     * @param script_id The script identifier
     * @param content The script content
     * @param metadata Optional metadata about the script
     */
    void broadcastScriptUpdate(const std::string& script_id, const std::string& content, const LLSD& metadata = LLSD());

    /**
     * @brief Send compilation results to all connected editors for a specific script
     * @param script_id The script identifier
     * @param success Whether compilation succeeded
     * @param errors Array of compilation errors/warnings
     */
    void broadcastCompilationResult(const std::string& script_id, bool success, const LLSD& errors = LLSD());

protected:
    LLWebsocketMgr::WSConnection::ptr_t connectionFactory(LLWebsocketMgr::WSServer::ptr_t server,
                                                         LLWebsocketMgr::connection_h handle) override;

    /**
     * @brief Apply global method handlers to a new connection
     * @param connection The connection to configure
     *
     * Override this method to customize which methods are registered on
     * new connections. The base implementation registers all global methods,
     * but derived classes can add additional script-specific methods.
     */
    virtual void setupConnectionMethods(LLJSONRPCConnection::ptr_t connection) override;

private:
    using map_id_to_editor_t = std::unordered_map<std::string, LLHandle<LLPanel>>;

    map_id_to_editor_t  mScriptEditors;
    std::set<std::shared_ptr<LLScriptEditorWSConnection>> mActiveConnections;

    /**
     * @brief Connection timeout management
     */
    LLTimer mCleanupTimer;
    static constexpr F32 CLEANUP_INTERVAL = 60.0f; // seconds
    static constexpr F32 CONNECTION_TIMEOUT = 300.0f; // 5 minutes
};
