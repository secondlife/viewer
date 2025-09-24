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

class LLScriptEditorWSConnection : public LLJSONRPCConnection, public std::enable_shared_from_this<LLScriptEditorWSConnection>
{
public:
    using ptr_t  = std::shared_ptr<LLScriptEditorWSConnection>;
    using wptr_t = std::weak_ptr<LLScriptEditorWSConnection>;

    enum DisconnectReason
    {
        REASON_NORMAL         = 0,
        REASON_EDITOR_CLOSED  = 1,
        REASON_PROTOCOL_ERROR = 2,
        REASON_TIMEOUT        = 3,
        REASON_INTERNAL_ERROR = 4
    };

    LLScriptEditorWSConnection(const LLWebsocketMgr::WSServer::ptr_t server, const LLWebsocketMgr::connection_h& handle) :
        LLJSONRPCConnection(server, handle)
    {
        mConnectionID = sNextConnectionID++;
    }

    ~LLScriptEditorWSConnection() override = default;

    U32 getConnectionID() const { return mConnectionID; }

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

    LLScriptEdContainer*                    getEditor() const;
    std::shared_ptr<LLScriptEditorWSServer> getServer() const;

    U32 mConnectionID{ 0 }; ///< Unique identifier for this connection

    // Client handshake response data
    std::string  mClientName;      ///< Name of the external editor client
    std::string  mClientVersion;   ///< Version of the external editor client
    std::string  mProtocolVersion; ///< JSON-RPC protocol version supported by client
    std::string  mScriptName;      ///< Name of the script being edited
    std::string  mScriptLanguage;  ///< Programming language of the script (lsl, luau, etc.)
    string_set_t mLanguages;       ///< Set of supported scripting languages
    string_set_t mFeatures;        ///< Active client features (live_sync, compilation, etc.)

    static U32   sNextConnectionID;
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
    enum SubscriptionError_t
    {
        SUBSCRIPTION_SUCCESS = 0,
        SUBSCRIPTION_INVALID_EDITOR,
        SUBSCRIPTION_INVALID_SUBSCRIPTION,
        SUBSCRIPTION_ALREADY_SUBSCRIBED,
        SUBSCRIPTION_INTERNAL_ERROR
    };

    static constexpr char const* DEFAULT_SERVER_NAME = "script_editor_server";
    static constexpr U16         DEFAULT_SERVER_PORT = 9020;

    using ptr_t = std::shared_ptr<LLScriptEditorWSServer>;
    using wptr_t = std::weak_ptr<LLScriptEditorWSServer>;

    LLScriptEditorWSServer(const std::string& name, U16 port, bool local_only = true);

    virtual ~LLScriptEditorWSServer() = default;

    void onStarted() override;
    void onStopped() override;
    void onConnectionOpened(const LLWebsocketMgr::WSConnection::ptr_t& connection) override;
    void onConnectionClosed(const LLWebsocketMgr::WSConnection::ptr_t& connection) override;

    bool subscribeScriptEditor(const LLHandle<LLPanel>& editor_handle, const std::string &script_id);
    void unsubscribeEditor(const std::string &script_id);

    void notifyScript(const std::string& script_id, const std::string& method, const LLSD& message) const;
    void sendUnsubscribeScriptEditor(const std::string& script_id);
    void sendCompileResults(const std::string& script_id, const LLSD& results) const;

    LLHandle<LLPanel> findEditorForScript(const std::string& script_id) const;

    /**
     * @brief Get list of active script editing sessions
     * @return Set of script IDs currently being edited
     */
    std::set<std::string> getActiveScripts() const;

protected:
    LLWebsocketMgr::WSConnection::ptr_t connectionFactory(LLWebsocketMgr::WSServer::ptr_t server,
                                                         LLWebsocketMgr::connection_h handle) override;

    void setupConnectionMethods(LLJSONRPCConnection::ptr_t connection) override;

    void broadcastLangugeChange();

    LLSD handleLanguageIdRequest() const;
    LLSD handleSyntaxRequest(const LLSD &params) const;
    LLSD handleScriptSubscribe(U32 connection_id, const LLSD& params);
    LLSD handleScriptUnsubscribe(U32 connection_id, const LLSD& params);

private:
    struct EditorSubscription
    {
        LLHandle<LLPanel> mEditorHandle;
        LLScriptEditorWSConnection::wptr_t mConnection;
        U32 mConnectionID{ 0 };
    };
    using subscriptions_t = std::unordered_map<std::string, EditorSubscription>;

    SubscriptionError_t updateScriptSubscription(const std::string &script_id, U32 connection_id);
    void                unsubscribeConnection(U32 connection_id);

    subscriptions_t mSubscriptions;
    std::map<U32, LLScriptEditorWSConnection::wptr_t> mActiveConnections;

    boost::signals2::connection mLanguageChangeSignal;
    LLUUID mLastSyntaxId;


    LLTimer mCleanupTimer;
    static constexpr F32 CLEANUP_INTERVAL = 60.0f; // seconds
    static constexpr F32 CONNECTION_TIMEOUT = 300.0f; // 5 minutes
};
