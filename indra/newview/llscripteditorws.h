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
#include "llpublishedobjectmgr.h"
#include "llsd.h"
#include "lluuid.h"
#include "llhandle.h"
#include "lltimer.h"
#include "lleventtimer.h"

#include <memory>
#include <string>
#include <map>
#include <set>
#include <atomic>

// Forward declarations
class LLLiveLSLEditor;
class LLScriptEdContainer;
class LLScriptEditorWSServer;
class LLChat;
class LLPanel;
class LLViewerObject;
class LLInventoryItem;

class LLScriptEditorWSConnection : public LLJSONRPCConnection, public std::enable_shared_from_this<LLScriptEditorWSConnection>
{
public:
    using ptr_t  = std::shared_ptr<LLScriptEditorWSConnection>;
    using wptr_t = std::weak_ptr<LLScriptEditorWSConnection>;

    enum class DisconnectReason : S32
    {
        NORMAL         = 0,
        EDITOR_CLOSED  = 1,
        PROTOCOL_ERROR = 2,
        TIMEOUT        = 3,
        INTERNAL_ERROR = 4
    };

    LLScriptEditorWSConnection(const LLWebsocketMgr::WSServer::ptr_t server, const LLWebsocketMgr::connection_h& handle) :
        LLJSONRPCConnection(server, handle)
    {
        // Reserve id 0 as the "unassigned" sentinel used by EditorSubscription;
        // on wrap, skip past it.
        U32 id;
        do
        {
            id = sNextConnectionID.fetch_add(1, std::memory_order_relaxed);
        }
        while (id == 0);
        mConnectionID = id;
    }

    ~LLScriptEditorWSConnection() override = default;

    U32 getConnectionID() const { return mConnectionID; }

    // Connection lifecycle overrides
    void onOpen() override;
    void onClose() override;

    void sendDisconnect(DisconnectReason reason = DisconnectReason::NORMAL, const std::string& message = "Goodbye");

private:
    using string_set_t = std::set<std::string>;
    /**
     * @brief Handle the handshake response from the client
     * @param result The response data from the client containing client information
     */
    void handleHandshakeResponse(const LLSD& result);
    std::string generateChallenge();

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
    LLUUID       mChallenge;
    std::string  mChallengeFile;   ///< Temporary file used for challenge-response verification

    static std::atomic<U32> sNextConnectionID;
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
    static constexpr U32 ALL_CONNECTIONS = 0xFFFFFFFF;
    enum class SubscriptionError
    {
        SUCCESS = 0,
        INVALID_EDITOR,
        INVALID_SUBSCRIPTION,
        ALREADY_SUBSCRIBED,
        INTERNAL_ERROR
    };

    static constexpr const char* DEFAULT_SERVER_NAME = "script_editor_server";
    static constexpr U16         DEFAULT_SERVER_PORT = 9020;

    using ptr_t = std::shared_ptr<LLScriptEditorWSServer>;
    using wptr_t = std::weak_ptr<LLScriptEditorWSServer>;

    LLScriptEditorWSServer(const std::string& name, U16 port, bool local_only = true);

    ~LLScriptEditorWSServer() override = default;

    static LLScriptEditorWSServer::ptr_t getServer();
    static LLScriptEditorWSServer::ptr_t ensureServerRunning();
    static std::string                   buildVSCodeURI(const LLUUID& object_id = LLUUID::null,
                                                        const LLUUID& script_id = LLUUID::null);
    static bool                          launchVSCode(const LLUUID& object_id = LLUUID::null,
                                                      const LLUUID& script_id = LLUUID::null);

    void onStarted() override;
    void onStopped() override;
    void onConnectionOpened(const LLWebsocketMgr::WSConnection::ptr_t& connection) override;
    void onConnectionClosed(const LLWebsocketMgr::WSConnection::ptr_t& connection) override;

    bool subscribeScriptEditor(const LLUUID& object_id, const LLUUID& item_id, std::string_view script_name,
        const LLHandle<LLPanel>& editor_handle, const std::string &script_id);
    void unsubscribeEditor(const std::string &script_id);

    void notifyScript(const std::string& script_id, const std::string& method, const LLSD& message) const;
    void sendUnsubscribeScriptEditor(const std::string& script_id);
    void sendCompileResults(const std::string& script_id, const LLSD& results) const;

    LLHandle<LLPanel> findEditorForScript(const std::string& script_id) const;

    void forwardChatToIDE(const LLChat& chat_msg) const;

    std::set<std::string> getActiveScripts() const;

    // --- Object Content Publishing ---
    bool publishObject(const LLUUID& object_id);
    void unpublishObject(const LLUUID& object_id, const std::string& reason = "");
    bool isObjectPublished(const LLUUID& object_id) const;
    void onPrimInventoryReady(const LLUUID& object_id, const LLUUID& prim_id);
    void onPrimInventoryChanged(const LLUUID& object_id, const LLUUID& prim_id);
    void onObjectPropertyChanged(const LLUUID& prim_id, const std::string& name, const std::string& desc, S16 inventory_serial = -1);
    void onLinksetChildAdded(const LLUUID& root_id, LLViewerObject* child);
    void onLinksetChildRemoved(const LLUUID& root_id, const LLUUID& child_id);

    static bool isEnabled();
    static bool isTightIntegration();

protected:
    LLWebsocketMgr::WSConnection::ptr_t connectionFactory(LLWebsocketMgr::WSServer::ptr_t server,
                                                         LLWebsocketMgr::connection_h handle) override;

    void setupConnectionMethods(LLJSONRPCConnection::ptr_t connection) override;

    void broadcastLanguageChange();

    LLSD handleLanguageIdRequest() const;
    LLSD handleSyntaxRequest(const LLSD &params) const;
    LLSD handleSyntaxCacheRequest() const;
    LLSD handleSyntaxCacheFileRequest(const LLSD& params) const;
    LLSD handleScriptSubscribe(U32 connection_id, const LLSD& params);
    LLSD handleScriptUnsubscribe(U32 connection_id, const LLSD& params);
    LLSD handleFileWatcherFileListRequest() const;
    LLSD handleObjectRequest(U32 connection_id, const LLSD& params);
    LLSD handleObjectContentGet(const std::string& method, const LLSD& id, const LLSD& params);
    LLSD handleObjectContentSave(const std::string& method, const LLSD& id, const LLSD& params);
    LLSD saveScript(LLViewerObject* prim, LLInventoryItem* item, const std::string& content, const LLSD& params);
    LLSD saveNotecard(LLViewerObject* prim, LLInventoryItem* item, const std::string& content);
    LLSD handleObjectItemDelete(U32 connection_id, const LLSD& params);
    LLSD handleObjectItemCreate(const std::string& method, const LLSD& id, const LLSD& params);
    LLSD handleObjectUnpublish(U32 connection_id, const LLSD& params);
    LLSD handleObjectList() const;
    LLSD handleObjectScriptSetRunning(U32 connection_id, const LLSD& params);
    LLSD handleObjectScriptReset(U32 connection_id, const LLSD& params);
    LLSD handleObjectModify(U32 connection_id, const LLSD& params);
    LLSD handleObjectItemModify(U32 connection_id, const LLSD& params);

    struct ValidatedItem
    {
        LLViewerObject*    prim{ nullptr };
        LLViewerObject*    root{ nullptr };
        LLInventoryItem*   item{ nullptr };
        LLAssetType::EType type{ LLAssetType::AT_NONE };
    };
    ValidatedItem validatePublishedItem(const LLSD& params, U32 permMask) const;

    // --- Object Content Publishing (helpers) ---
    static std::string getPrimName(LLViewerObject* obj);
    void notifyConnection(U32 connection_id, const std::string& method, const LLSD& params) const;
    void notifyAll(const std::string& method, const LLSD& params) const;
    void buildAndSendPublish(const LLUUID& object_id);
    void scheduleLinksetFlush(const LLUUID& root_id, F32 delay);
    void cancelLinksetFlushTimer(const LLUUID& root_id);
    void flushLinksetUpdate(const LLUUID& root_id);
    static LLSD errorResponse(const std::string& message);

    /// Wraps `fn` in a MethodHandler with a weak-ptr guard on this server,
    /// so the handler safely no-ops after server shutdown. `fn` is called
    /// with (LLScriptEditorWSServer&, method, id, params) and returns LLSD.
    template <typename Fn>
    LLJSONRPCConnection::MethodHandler bindHandler(Fn fn)
    {
        std::weak_ptr<LLWebsocketMgr::WSServer> weak_base = weak_from_this();
        return [weak_base, fn = std::move(fn)]
               (const std::string& method, const LLSD& id, const LLSD& params) -> LLSD
        {
            auto base = weak_base.lock();
            if (!base)
            {
                return LLSD();
            }
            auto server = std::static_pointer_cast<LLScriptEditorWSServer>(base);
            return fn(*server, method, id, params);
        };
    }

private:
    struct EditorSubscription
    {
        EditorSubscription(const LLUUID &object_id, const LLUUID &item_id, std::string_view script_name, LLHandle<LLPanel> editor_handle):
            mObjectID(object_id),
            mItemID(item_id),
            mScriptName(script_name),
            mEditorHandle(editor_handle)
        {}
        U32 mConnectionID{ 0 };
        LLUUID mObjectID;
        LLUUID mItemID;
        std::string mScriptName;
        LLScriptEditorWSConnection::wptr_t mConnection;
        LLHandle<LLPanel> mEditorHandle;
    };
    using subscriptions_t = std::unordered_map<std::string, EditorSubscription>;

    SubscriptionError updateScriptSubscription(const std::string &script_id, U32 connection_id);
    void                unsubscribeConnection(U32 connection_id);

    subscriptions_t mSubscriptions;
    // Per-connection subscription count. Invariant: for c != 0,
    // mConnectionSubscriptionCounts[c] == count of entries in mSubscriptions
    // whose mConnectionID == c. Maintained transactionally at every site that
    // mutates SubscriptionInfo::mConnectionID.
    std::unordered_map<U32, S32> mConnectionSubscriptionCounts;
    std::map<U32, LLScriptEditorWSConnection::wptr_t> mActiveConnections;

    mutable LLPublishedObjectMgr mPublishedObjectManager;

    boost::signals2::connection mLanguageChangeSignal;
    LLUUID mLastSyntaxId;


    LLTimer mCleanupTimer;
    static constexpr F32 CLEANUP_INTERVAL = 60.0f; // seconds
    static constexpr F32 CONNECTION_TIMEOUT = 300.0f; // 5 minutes

};
