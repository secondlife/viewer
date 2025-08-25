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

#include "llwebsocketmgr.h"
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
class LLScriptEdCore;

/**
 * @class LLScriptEditorWSConnection
 * @brief WebSocket connection specialized for external script editor communication
 *
 * This class handles WebSocket communication between the Second Life viewer
 * and external script editors. It manages script content synchronization,
 * compilation status updates, and editor metadata exchange.
 *
 * ## Message Protocol
 *
 * The connection uses JSON messages with the following structure:
 * - `type`: Message type identifier
 * - `data`: Message payload (varies by type)
 * - `timestamp`: Message timestamp for ordering
 * - `id`: Optional message ID for request/response correlation
 *
 * ### Supported Message Types:
 *
 * #### From Editor to Viewer:
 * - `script_updated`: Script content has been modified
 * - `save_request`: Request to save script to SL servers
 * - `compile_request`: Request to compile script
 * - `editor_ready`: Editor initialization complete
 * - `ping`: Connection health check
 *
 * #### From Viewer to Editor:
 * - `script_content`: Full script content
 * - `compile_result`: Compilation success/failure with errors
 * - `save_result`: Save operation result
 * - `metadata`: Script and object metadata
 * - `pong`: Response to ping
 */
class LLScriptEditorWSConnection : public LLWebsocketMgr::WSConnection
{
public:

    LLScriptEditorWSConnection(const LLWebsocketMgr::WSServer::ptr_t server,
                              const LLWebsocketMgr::connection_h& handle):
        LLWebsocketMgr::WSConnection(server, handle),
        mMessageSequence(0)
    { }

    ~LLScriptEditorWSConnection() override = default;


    // Connection lifecycle overrides
    void onOpen() override;
    void onClose() override;
    void onMessage(const std::string& message) override;

private:
    /**
     * @brief Handle connect/connection messages from editor
     * @param message Parsed LLSD message
     */
    void processConnectMessage(const LLSD& message);

    std::string mEditorId;              ///< Unique identifier for this editor session
    LLSD mEditorCapabilities;           ///< Editor capabilities metadata
    U32 mMessageSequence;               ///< Message sequence counter
    std::string mScriptId;              ///< Unique identifier for the script being edited
};

/**
 * @class LLScriptEditorWSServer
 * @brief WebSocket server for external script editor integration
 *
 * This server manages WebSocket connections from external script editors,
 * providing a bridge between the Second Life viewer's script editing
 * functionality and external development tools.
 *
 * ## Architecture
 *
 * The server acts as a communication hub between:
 * - LLLiveLSLEditor instances (in-world script editing)
 * - External script editors (VS Code, Atom, Sublime Text, etc.)
 * - Script compilation and save services
 *
 * ## Usage
 *
 * @code
 * // Create and start the server
 * auto server = std::make_shared<LLScriptEditorWSServer>("script_editor_server", 8080);
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
 * - Editor authentication via connection handshake
 * - Script content encryption for sensitive projects
 * - Rate limiting to prevent abuse
 */
class LLScriptEditorWSServer : public LLWebsocketMgr::WSServer
{
public:
    static constexpr char const* DEFAULT_SERVER_NAME = "script_editor_server";
    static constexpr U16         DEFAULT_SERVER_PORT = 9020;

    using ptr_t = std::shared_ptr<LLScriptEditorWSServer>;

    LLScriptEditorWSServer(const std::string_view name, U16 port, bool local_only = true);

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

protected:
    LLWebsocketMgr::WSConnection::ptr_t connectionFactory(WSServer::ptr_t server, LLWebsocketMgr::connection_h handle) override;

private:
    using map_id_to_editor_t = std::unordered_map<std::string, LLHandle<LLPanel> >;

    map_id_to_editor_t  mScriptEditors;

    std::set<std::shared_ptr<LLScriptEditorWSConnection>> mActiveConnections;

    /**
     * @brief Connection timeout management
     */
    LLTimer mCleanupTimer;
    static constexpr F32 CLEANUP_INTERVAL = 60.0f; // seconds
    static constexpr F32 CONNECTION_TIMEOUT = 300.0f; // 5 minutes
};
