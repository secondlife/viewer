/**
 * @file llscripteditorws.cpp
 * @brief JSON-RPC 2.0 WebSocket server implementation for external script editor integration
 *
 * For a full description of the JSON-RPC protocol and all supported methods,
 * see doc/external-editor-json-rpc.md in the repository root.
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

#include "llviewerprecompiledheaders.h"
#include "llscripteditorws.h"

#include "llagent.h"
#include "llappviewer.h"
#include "llchat.h"
#include "lldate.h"
#include "llerror.h"
#include "lleventcoro.h"
#include "lleventfilter.h"
#include "llevents.h"
#include "llfilesystem.h"
#include "llfloaterperms.h"
#include "llfloaterreg.h"
#include "llinventorytype.h"
#include "llinventorydefines.h"
#include "llnotecard.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
#include "llprocess.h"
#include "llregex.h"
#include "llsdjson.h"
#include "llselectmgr.h"
#include "lltrans.h"
#include "lluuid.h"
#include "llversioninfo.h"
#include "llviewerassetstorage.h"
#include "llviewerassettype.h"
#include "llviewerassetupload.h"
#include "llviewercontrol.h"
#include "llviewerinventory.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llviewerregion.h"
#include "llviewertexteditor.h"
#include "llvoinventorylistener.h"
#include "roles_constants.h"

namespace
{
    // Per-operation timeouts (seconds) for coroutine-based async RPC handlers.
    constexpr F32 ASSET_FETCH_TIMEOUT     = 30.0f;
    constexpr F32 SCRIPT_UPLOAD_TIMEOUT   = 60.0f;
    constexpr F32 NOTECARD_UPLOAD_TIMEOUT = 30.0f;
    constexpr F32 ITEM_CREATE_TIMEOUT     = 30.0f;

    // Linkset flush coalescing delays (seconds).
    constexpr F32 LINKSET_ADD_FLUSH_DELAY    = 5.0f;
    constexpr F32 LINKSET_REMOVE_FLUSH_DELAY = 0.2f;

    // Creates a uniquely-named LLEventMailDrop under "<prefix>.<uuid>", passes
    // its name to kickoff (which arranges for one post to that pump), then
    // suspends the current coroutine up to 	imeout seconds for the result.
    // Throws RequestTimeoutError(timeout_msg) if the deadline elapses.
    template <typename Kickoff>
    LLSD await_async_result(const std::string& pump_prefix,
                            F32 timeout,
                            const std::string& timeout_msg,
                            Kickoff&& kickoff)
    {
        LLEventMailDrop pump(pump_prefix + "." + LLUUID::generateNewID().asString(), true);
        std::string pump_name = pump.getName();
        std::forward<Kickoff>(kickoff)(pump_name);
        LLSD result = llcoro::suspendUntilEventOnWithTimeout(
            pump, timeout, LLSD().with("timeout", true));
        if (result.has("timeout"))
        {
            throw LLJSONRPCConnection::RequestTimeoutError(timeout_msg);
        }
        return result;
    }

    // Builds the (success, failure) callback pair used by LLResourceUploadInfo-
    // derived uploads. Both outcomes post a single LLSD to pump_name:
    //   - success: the server's response LLSD with item_id/task_id added.
    //   - failure: { "failed": true, "reason": <reason> }.
    auto make_asset_upload_callbacks(const std::string& pump_name)
    {
        auto on_success = [pump_name](LLUUID item_id, LLUUID task_id, LLUUID /*new_asset_id*/, LLSD response)
        {
            response["item_id"] = item_id;
            response["task_id"] = task_id;
            LLEventPumps::instance().post(pump_name, response);
        };
        auto on_failure = [pump_name](LLUUID /*item_id*/, LLUUID /*task_id*/, LLSD /*response*/, std::string reason)
        {
            LLSD failure;
            failure["failed"] = true;
            failure["reason"] = reason;
            LLEventPumps::instance().post(pump_name, failure);
            return false;
        };
        return std::make_pair(std::move(on_success), std::move(on_failure));
    }

    // Returns [root, *root->getChildren()] in stable order. Root must be non-null.
    std::vector<LLViewerObject*> collect_linkset(LLViewerObject* root)
    {
        std::vector<LLViewerObject*> prims;
        const auto& children = root->getChildren();
        prims.reserve(1 + children.size());
        prims.push_back(root);
        for (LLViewerObject* child : children)
        {
            prims.push_back(child);
        }
        return prims;
    }

    // Returns the value of NV pair key on obj as a string, or empty if
    // obj / pair / string is null or empty. NUL-safe.
    std::string nv_string(LLViewerObject* obj, const char* key)
    {
        if (!obj)
        {
            return std::string();
        }
        LLNameValue* nv = obj->getNVPair(key);
        if (!nv)
        {
            return std::string();
        }
        const char* s = nv->getString();
        if (!s || s[0] == '\0')
        {
            return std::string();
        }
        return std::string(s);
    }
}

class LLPublishedPrimListener : public LLVOInventoryListener
{
public:
    LLPublishedPrimListener(LLScriptEditorWSServer* server, const LLUUID& object_id, const LLUUID& prim_id,
                            LLViewerObject* object)
        : mServer(server)
        , mObjectID(object_id)
        , mPrimID(prim_id)
    {
        registerVOInventoryListener(object, nullptr);
    }

    ~LLPublishedPrimListener() override = default;

    void inventoryChanged(LLViewerObject* object,
                         LLInventoryObject::object_list_t* inventory,
                         S32 serial_num, void* user_data) override
    {
        if (mServer)
        {
            if (mServer->isObjectPublished(mObjectID))
            {
                mServer->onPrimInventoryChanged(mObjectID, mPrimID);
            }
            else
            {
                mServer->onPrimInventoryReady(mObjectID, mPrimID);
            }
        }
    }

    const LLUUID& getObjectID() const { return mObjectID; }
    const LLUUID& getPrimID() const { return mPrimID; }

private:
    LLScriptEditorWSServer* mServer;    // non-owning; server always outlives listeners
    LLUUID                  mObjectID;  // root object this prim belongs to
    LLUUID                  mPrimID;    // this specific prim
};

//========================================================================
LLScriptEditorWSServer::LLScriptEditorWSServer(const std::string& name, U16 port, bool local_only)
    : LLJSONRPCServer(name, port, local_only)
{
    LL_INFOS("ScriptEditorWS") << "Created JSON-RPC script editor server: " << name
                               << " on port " << port << LL_ENDL;
}

LLScriptEditorWSServer::ptr_t LLScriptEditorWSServer::getServer()
{
    if (!LLWebsocketMgr::instanceExists())
    {
        return nullptr;
    }
    LLWebsocketMgr&               wsmgr  = LLWebsocketMgr::instance();
    return std::static_pointer_cast<LLScriptEditorWSServer>(
            wsmgr.findServerByName(LLScriptEditorWSServer::DEFAULT_SERVER_NAME));
}

LLScriptEditorWSServer::ptr_t LLScriptEditorWSServer::ensureServerRunning()
{
    if (!gSavedSettings.getBOOL("ExternalWebsocketSyncEnable"))
    {
        LL_DEBUGS("ScriptEditorWS") << "WebSocket server is disabled by ExternalWebsocketSyncEnable" << LL_ENDL;
        return nullptr;
    }

    LLWebsocketMgr& wsmgr = LLWebsocketMgr::instance();
    ptr_t server = std::static_pointer_cast<LLScriptEditorWSServer>(
        wsmgr.findServerByName(DEFAULT_SERVER_NAME));

    if (!server)
    {
        U16  port       = static_cast<U16>(gSavedSettings.getS32("ExternalWebsocketSyncPort"));
        bool local_only = gSavedSettings.getBOOL("ExternalWebsocketSyncLocal");
        server = std::make_shared<LLScriptEditorWSServer>(DEFAULT_SERVER_NAME, port, local_only);
        wsmgr.addServer(server);
    }

    if (!server->isRunning())
    {
        if (!wsmgr.startServer(DEFAULT_SERVER_NAME))
        {
            LL_WARNS("ScriptEditorWS") << "Failed to start script editor websocket server" << LL_ENDL;
            return nullptr;
        }
    }

    return server;
}

std::string LLScriptEditorWSServer::buildVSCodeURI(const LLUUID& object_id,
                                                    const LLUUID& script_id)
{
    std::ostringstream uri;
    uri << "vscode://lindenlab.sl-vscode-plugin/connect";

    U16 port = static_cast<U16>(gSavedSettings.getS32("ExternalWebsocketSyncPort"));
    uri << "?port=" << port;

    if (object_id.notNull())
    {
        uri << "&object=" << object_id.asString();
    }

    if (script_id.notNull())
    {
        uri << "&script=" << script_id.asString();
    }

    return uri.str();
}

bool LLScriptEditorWSServer::launchVSCode(const LLUUID& object_id,
                                           const LLUUID& script_id)
{
    ptr_t server = ensureServerRunning();
    if (!server)
    {
        LL_WARNS("ScriptEditorWS") << "Cannot launch VS Code: WebSocket server failed to start" << LL_ENDL;
        return false;
    }

    std::string uri = buildVSCodeURI(object_id, script_id);

    LLProcess::Params params;
#if LL_WINDOWS
    // On Windows, VS Code's 'code' is a batch file (.cmd) which APR cannot
    // launch directly. Invoke it through cmd.exe instead.
    // The URI may contain '&' which cmd.exe treats as a command separator,
    // so the entire argument list is passed as a single quoted string.
    params.executable = "cmd.exe";
    params.args.add("/c");
    params.args.add("code --open-url \"" + uri + "\"");
#else
    params.executable = "code";
    params.args.add("--open-url");
    params.args.add(uri);
#endif
    params.autokill = false;

    LLProcessPtr process = LLProcess::create(params);
    if (!process)
    {
        LL_WARNS("ScriptEditorWS") << "Failed to launch VS Code. "
            << "Ensure the 'code' command is available on your PATH." << LL_ENDL;
        return false;
    }

    LL_INFOS("ScriptEditorWS") << "Launched VS Code with URI: " << uri << LL_ENDL;
    return true;
}


LLWebsocketMgr::WSConnection::ptr_t LLScriptEditorWSServer::connectionFactory(LLWebsocketMgr::WSServer::ptr_t server,
                                                                              LLWebsocketMgr::connection_h handle)
{
    auto connection = std::make_shared<LLScriptEditorWSConnection>(server, handle);
    mActiveConnections[connection->getConnectionID()] = connection;

    // Call setupConnectionMethods to register any global methods
    setupConnectionMethods(connection);

    return connection;
}

void LLScriptEditorWSServer::onStarted()
{
    LLSyntaxDefCache& syntax_id_mgr = LLSyntaxDefCache::instance();
    wptr_t that(std::static_pointer_cast<LLScriptEditorWSServer>(shared_from_this()));

    mLastSyntaxId = syntax_id_mgr.getSyntaxID();
    mLanguageChangeSignal = syntax_id_mgr.addSyntaxIDCallback(
        [that]()
        {
            auto server = that.lock();
            if (server && server->isRunning())
            {
                server->broadcastLanguageChange();
            }
        });
}

void LLScriptEditorWSServer::onStopped()
{
    mLanguageChangeSignal.disconnect();
    mLastSyntaxId.setNull();

    // Connections are already closed -- clean up all internal state silently.
    // Do not attempt to send notifications; the sockets are gone.

    for (auto& [id, pending] : mPendingPublishes)
    {
        pending.mListeners.clear();
    }
    mPendingPublishes.clear();

    for (auto& [id, info] : mPublishedObjects)
    {
        info.mListeners.clear();
    }
    mPublishedObjects.clear();

    mSubscriptions.clear();
    mActiveConnections.clear();

    LL_INFOS("ScriptEditorWS") << "Script editor WebSocket server stopped, all state cleaned up" << LL_ENDL;
}

void LLScriptEditorWSServer::onConnectionOpened(const LLWebsocketMgr::WSConnection::ptr_t& connection)
{
    // Call parent class to handle JSON-RPC setup and standard methods
    LLJSONRPCServer::onConnectionOpened(connection);

    LL_INFOS("ScriptEditorWS") << "New script editor client connected via JSON-RPC" << LL_ENDL;

}

void LLScriptEditorWSServer::onConnectionClosed(const LLWebsocketMgr::WSConnection::ptr_t& connection)
{
    // Call parent class to handle JSON-RPC cleanup
    LLJSONRPCServer::onConnectionClosed(connection);

    LL_INFOS("ScriptEditorWS") << "Script editor client disconnected" << LL_ENDL;

    // Remove from active connections
    auto script_connection = std::dynamic_pointer_cast<LLScriptEditorWSConnection>(connection);
    if (script_connection)
    {
        U32 connection_id = script_connection->getConnectionID();
        unsubscribeConnection(connection_id);
        mActiveConnections.erase(connection_id);

        LL_DEBUGS("ScriptEditorWS") << "Removed connection from active connections. Total: "
                                   << mActiveConnections.size() << LL_ENDL;
        // TODO: When connections reach 0, stop the server after a timeout.
    }
}

bool LLScriptEditorWSServer::subscribeScriptEditor(const LLUUID& object_id, const LLUUID& item_id, std::string_view script_name,
    const LLHandle<LLPanel>& editor_handle, const std::string& script_id)
{
    if (editor_handle.isDead())
    {
        return false;
    }

    auto it = mSubscriptions.find(script_id);
    if (it == mSubscriptions.end())
    {
        // New subscription
        mSubscriptions.emplace(script_id,
            EditorSubscription(object_id, item_id, script_name, editor_handle));
    }
    else
    {
        // Refresh existing subscription with the new editor handle
        it->second.mEditorHandle = editor_handle;
    }
    return true;
}

void LLScriptEditorWSServer::unsubscribeEditor(const std::string &script_id)
{
    auto it = mSubscriptions.find(script_id);
    if (it != mSubscriptions.end())
    {
        S32 connection_id = it->second.mConnectionID;
        auto connection = it->second.mConnection.lock();
        mSubscriptions.erase(it);

        // Maintain per-connection count; erase entry when it hits zero.
        bool last_for_connection = false;
        if (connection_id != 0)
        {
            auto cit = mConnectionSubscriptionCounts.find(connection_id);
            if (cit != mConnectionSubscriptionCounts.end())
            {
                if (--cit->second <= 0)
                {
                    mConnectionSubscriptionCounts.erase(cit);
                    last_for_connection = true;
                }
            }
            else
            {
                // No counter entry means no other subs referenced this connection.
                last_for_connection = true;
            }
        }

        if (connection && last_for_connection)
        { // We have removed the last subscription, close the connection
            LL_DEBUGS("ScriptEditorWS") << "Closing connection ID " << connection_id <<
                " as last subscription was removed" << LL_ENDL;
            connection->sendDisconnect(LLScriptEditorWSConnection::DisconnectReason::EDITOR_CLOSED, "Editor closed");
        }

    }
}

void LLScriptEditorWSServer::unsubscribeConnection(U32 connection_id)
{
    for (auto it = mSubscriptions.begin(); it != mSubscriptions.end(); ++it)
    {
        if (it->second.mConnectionID == connection_id)
        {
            LL_DEBUGS("ScriptEditorWS") << "Unsubscribing script " << it->first
                                       << " from connection ID " << connection_id << LL_ENDL;
            it->second.mConnectionID = 0;
            it->second.mConnection.reset();
        }
    }
    // All subs for this connection now have mConnectionID == 0.
    mConnectionSubscriptionCounts.erase(connection_id);
}

LLScriptEditorWSServer::SubscriptionError LLScriptEditorWSServer::updateScriptSubscription(const std::string &script_id, U32 connection_id)
{
    auto it = mSubscriptions.find(script_id);
    if (it != mSubscriptions.end())
    {
        if (it->second.mEditorHandle.isDead())
        {
            unsubscribeEditor(script_id);
            return SubscriptionError::INVALID_EDITOR;
        }

        auto con_it = mActiveConnections.find(connection_id);
        if (con_it == mActiveConnections.end())
        {
            return SubscriptionError::INTERNAL_ERROR;
        }

        if ((it->second.mConnectionID != 0) && !it->second.mConnection.expired()
            && it->second.mConnection.lock()->isConnected())
        {
            LL_WARNS("ScriptEditorWS") << "Script " << script_id << " is already subscribed on connection ID " << it->second.mConnectionID
                                       << ", cannot subscribe again on connection ID " << connection_id << LL_ENDL;
            // In the future we may want to support multiple connections per script.
            // That would imply it was open in multiple editors.
            return SubscriptionError::ALREADY_SUBSCRIBED;
        }

        // If this entry was previously bound to a different (dead) connection,
        // it would have been cleared by unsubscribeConnection, so mConnectionID
        // is always 0 here.
        it->second.mConnectionID = connection_id;
        it->second.mConnection   = con_it->second;
        ++mConnectionSubscriptionCounts[connection_id];
        return SubscriptionError::SUCCESS;
    }
    return SubscriptionError::INVALID_SUBSCRIPTION;
}


LLHandle<LLPanel> LLScriptEditorWSServer::findEditorForScript(const std::string& script_id) const
{
    auto it = mSubscriptions.find(script_id);
    if (it != mSubscriptions.end())
    {
        return it->second.mEditorHandle;
    }
    return LLHandle<LLPanel>();
}

std::set<std::string> LLScriptEditorWSServer::getActiveScripts() const
{
    std::set<std::string> active_scripts;
    for (const auto& [script_id, subinfo] : mSubscriptions)
    {
        if (!subinfo.mEditorHandle.isDead())
        {
            active_scripts.insert(script_id);
        }
    }
    return active_scripts;
}

void LLScriptEditorWSServer::setupConnectionMethods(LLJSONRPCConnection::ptr_t connection)
{
    // Call parent class to register global JSON-RPC methods
    LLJSONRPCServer::setupConnectionMethods(connection);

    // Cast to our specific connection type to access script editor functionality
    auto script_connection = std::dynamic_pointer_cast<LLScriptEditorWSConnection>(connection);
    if (script_connection)
    {
        LL_DEBUGS("ScriptEditorWS") << "Setting up script editor connection methods" << LL_ENDL;
        U32 connection_id = script_connection->getConnectionID();

        // Sync methods (run on the WebSocket I/O thread; must not touch
        // main-thread-only viewer state).
        script_connection->registerMethod("language.syntax.id",
            bindHandler([](LLScriptEditorWSServer& s, auto&, auto&, auto&)
            {
                return s.handleLanguageIdRequest();
            }));

        script_connection->registerMethod("language.syntax",
            bindHandler([](LLScriptEditorWSServer& s, auto&, auto&, const LLSD& params)
            {
                return s.handleSyntaxRequest(params);
            }));

        script_connection->registerMethod("language.syntax.cache",
            bindHandler([](LLScriptEditorWSServer& s, auto&, auto&, auto&)
            {
                return s.handleSyntaxCacheRequest();
            }));

        script_connection->registerMethod("language.syntax.get",
            bindHandler([](LLScriptEditorWSServer& s, auto&, auto&, const LLSD& params)
            {
                return s.handleSyntaxCacheFileRequest(params);
            }));

        script_connection->registerMethod("script.subscribe",
            bindHandler([connection_id](LLScriptEditorWSServer& s, auto&, auto&, const LLSD& params)
            {
                return s.handleScriptSubscribe(connection_id, params);
            }));

        script_connection->registerMethod("script.list",
            bindHandler([](LLScriptEditorWSServer& s, auto&, auto&, auto&)
            {
                return s.handleFileWatcherFileListRequest();
            }));

        script_connection->registerMethod("object.unpublish",
            bindHandler([connection_id](LLScriptEditorWSServer& s, auto&, auto&, const LLSD& params)
            {
                return s.handleObjectUnpublish(connection_id, params);
            }));

        // Async methods (dispatched to the main thread inside a coroutine).
        script_connection->registerAsyncMethod("script.unsubscribe",
            bindHandler([connection_id](LLScriptEditorWSServer& s, auto&, auto&, const LLSD& params)
            {
                return s.handleScriptUnsubscribe(connection_id, params);
            }));

        script_connection->registerAsyncMethod("object.request",
            bindHandler([connection_id](LLScriptEditorWSServer& s, auto&, auto&, const LLSD& params)
            {
                return s.handleObjectRequest(connection_id, params);
            }));

        script_connection->registerAsyncMethod("object.content.get",
            bindHandler([](LLScriptEditorWSServer& s, const std::string& method, const LLSD& id, const LLSD& params)
            {
                return s.handleObjectContentGet(method, id, params);
            }));

        script_connection->registerAsyncMethod("object.content.save",
            bindHandler([](LLScriptEditorWSServer& s, const std::string& method, const LLSD& id, const LLSD& params)
            {
                return s.handleObjectContentSave(method, id, params);
            }));

        script_connection->registerAsyncMethod("object.item.delete",
            bindHandler([connection_id](LLScriptEditorWSServer& s, auto&, auto&, const LLSD& params)
            {
                return s.handleObjectItemDelete(connection_id, params);
            }));

        script_connection->registerAsyncMethod("object.item.create",
            bindHandler([](LLScriptEditorWSServer& s, const std::string& method, const LLSD& id, const LLSD& params)
            {
                return s.handleObjectItemCreate(method, id, params);
            }));

        script_connection->registerAsyncMethod("object.list",
            bindHandler([](LLScriptEditorWSServer& s, auto&, auto&, auto&)
            {
                return s.handleObjectList();
            }));

        script_connection->registerAsyncMethod("object.script.set_running",
            bindHandler([connection_id](LLScriptEditorWSServer& s, auto&, auto&, const LLSD& params)
            {
                return s.handleObjectScriptSetRunning(connection_id, params);
            }));

        script_connection->registerAsyncMethod("object.script.reset",
            bindHandler([connection_id](LLScriptEditorWSServer& s, auto&, auto&, const LLSD& params)
            {
                return s.handleObjectScriptReset(connection_id, params);
            }));

        script_connection->registerAsyncMethod("object.modify",
            bindHandler([connection_id](LLScriptEditorWSServer& s, auto&, auto&, const LLSD& params)
            {
                return s.handleObjectModify(connection_id, params);
            }));

        script_connection->registerAsyncMethod("object.item.modify",
            bindHandler([connection_id](LLScriptEditorWSServer& s, auto&, auto&, const LLSD& params)
            {
                return s.handleObjectItemModify(connection_id, params);
            }));
    }
}

LLSD LLScriptEditorWSServer::handleObjectList() const
{
    LLSD objects = LLSD::emptyArray();
    for (const auto& [object_id, info] : mPublishedObjects)
    {
        LLViewerObject* root = gObjectList.findObject(object_id);
        if (!root)
        {
            LL_DEBUGS("ScriptEditorWS") << "object.list: skipping " << object_id
                << " (no longer in scene)" << LL_ENDL;
            continue;
        }

        // Use cached names from PublishedObjectInfo, but fetch live inventory
        LLSD pub;
        pub["object_id"]          = info.mObjectID;
        pub["object_name"]        = info.mObjectName;
        pub["object_description"] = info.mObjectDescription;
        pub["owner_id"]           = info.mOwnerID;
        if (!info.mRegionName.empty())
        {
            pub["region"] = info.mRegionName;
        }
        pub["inventory"] = buildPrimInventoryLLSD(root);

        LLSD linked_objects = LLSD::emptyArray();
        for (const auto& prim_info : info.mPrims)
        {
            if (prim_info.mLinkNumber == 1) continue;  // skip root

            LLViewerObject* child = gObjectList.findObject(prim_info.mPrimID);
            if (!child) continue;

            LLSD link;
            link["link_id"]     = prim_info.mPrimID;
            link["link_number"] = prim_info.mLinkNumber;
            link["link_name"]   = prim_info.mPrimName;  // Cached name
            link["inventory"]   = buildPrimInventoryLLSD(child);
            linked_objects.append(link);
        }
        if (linked_objects.size() > 0)
        {
            pub["linked_objects"] = linked_objects;
        }

        objects.append(pub);
    }

    LLSD response;
    response["objects"] = objects;
    return response;
}

LLSD LLScriptEditorWSServer::handleObjectScriptSetRunning(U32 connection_id, const LLSD& params)
{
    LLUUID prim_id = params["prim_id"].asUUID();
    LLUUID item_id = params["item_id"].asUUID();
    bool running = params["running"].asBoolean();

    if (prim_id.isNull() || item_id.isNull())
        throw LLJSONRPCConnection::InvalidParams("prim_id and item_id are required");

    LLViewerObject* prim = gObjectList.findObject(prim_id);
    if (!prim)
        throw LLJSONRPCConnection::InvalidParams("Prim not found");

    LLViewerObject* root = prim->getRootEdit();
    if (!root || !isObjectPublished(root->getID()))
        throw LLJSONRPCConnection::ForbiddenError("Object is not published");

    LLInventoryItem* item = dynamic_cast<LLInventoryItem*>(prim->getInventoryObject(item_id));
    if (!item)
        throw LLJSONRPCConnection::InvalidParams("Script not found in prim inventory");

    if (item->getType() != LLAssetType::AT_LSL_TEXT)
        throw LLJSONRPCConnection::InvalidParams("Item is not a script");

    if (!gAgent.allowOperation(PERM_MODIFY, item->getPermissions(), GP_OBJECT_MANIPULATE))
        throw LLJSONRPCConnection::ForbiddenError("No modify permission on script");

    // Send SetScriptRunning message to simulator
    LLMessageSystem* msg = gMessageSystem;
    msg->newMessageFast(_PREHASH_SetScriptRunning);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    msg->nextBlockFast(_PREHASH_Script);
    msg->addUUIDFast(_PREHASH_ObjectID, prim_id);
    msg->addUUIDFast(_PREHASH_ItemID, item_id);
    msg->addBOOLFast(_PREHASH_Running, running);
    msg->sendReliable(prim->getRegion()->getHost());

    LLSD response;
    response["success"] = true;
    return response;
}

LLSD LLScriptEditorWSServer::handleObjectScriptReset(U32 connection_id, const LLSD& params)
{
    LLUUID prim_id = params["prim_id"].asUUID();
    LLUUID item_id = params["item_id"].asUUID();

    if (prim_id.isNull() || item_id.isNull())
        throw LLJSONRPCConnection::InvalidParams("prim_id and item_id are required");

    LLViewerObject* prim = gObjectList.findObject(prim_id);
    if (!prim)
        throw LLJSONRPCConnection::InvalidParams("Prim not found");

    LLViewerObject* root = prim->getRootEdit();
    if (!root || !isObjectPublished(root->getID()))
        throw LLJSONRPCConnection::ForbiddenError("Object is not published");

    LLInventoryItem* item = dynamic_cast<LLInventoryItem*>(prim->getInventoryObject(item_id));
    if (!item)
        throw LLJSONRPCConnection::InvalidParams("Script not found in prim inventory");

    if (item->getType() != LLAssetType::AT_LSL_TEXT)
        throw LLJSONRPCConnection::InvalidParams("Item is not a script");

    if (!gAgent.allowOperation(PERM_MODIFY, item->getPermissions(), GP_OBJECT_MANIPULATE))
        throw LLJSONRPCConnection::ForbiddenError("No modify permission on script");

    // Send ScriptReset message to simulator
    LLMessageSystem* msg = gMessageSystem;
    msg->newMessageFast(_PREHASH_ScriptReset);
    msg->nextBlockFast(_PREHASH_AgentData);
    msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
    msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
    msg->nextBlockFast(_PREHASH_Script);
    msg->addUUIDFast(_PREHASH_ObjectID, prim_id);
    msg->addUUIDFast(_PREHASH_ItemID, item_id);
    msg->sendReliable(prim->getRegion()->getHost());

    LLSD response;
    response["success"] = true;
    return response;
}

LLSD LLScriptEditorWSServer::handleObjectModify(U32 connection_id, const LLSD& params)
{
    // ─────────────────────────────────────────────────────────────
    // Step 1: Parameter Validation
    // ─────────────────────────────────────────────────────────────
    LLUUID prim_id = params["prim_id"].asUUID();
    if (prim_id.isNull())
        throw LLJSONRPCConnection::InvalidParams("prim_id is required");

    bool has_name = params.has("name");
    bool has_desc = params.has("description");
    bool has_perms = params.has("permissions") && params["permissions"].has("next_owner");

    if (!has_name && !has_desc && !has_perms)
        throw LLJSONRPCConnection::InvalidParams(
            "At least one property (name, description, or permissions) must be specified");

    // ─────────────────────────────────────────────────────────────
    // Step 2: Find and Validate Object
    // ─────────────────────────────────────────────────────────────
    LLViewerObject* prim = gObjectList.findObject(prim_id);
    if (!prim)
        throw LLJSONRPCConnection::InvalidParams("Prim not found");

    LLViewerObject* root = prim->getRootEdit();
    if (!root || !isObjectPublished(root->getID()))
        throw LLJSONRPCConnection::ForbiddenError("Object is not published");

    if (!prim->permModify())
        throw LLJSONRPCConnection::ForbiddenError("No modify permission on object");

    // ─────────────────────────────────────────────────────────────
    // Step 3: Send Property Update Messages
    // ─────────────────────────────────────────────────────────────
    LLMessageSystem* msg = gMessageSystem;
    LLHost host = prim->getRegion()->getHost();
    U32 local_id = prim->getLocalID();

    if (has_name)
    {
        std::string new_name = params["name"].asString();
        msg->newMessageFast(_PREHASH_ObjectName);
        msg->nextBlockFast(_PREHASH_AgentData);
        msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
        msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
        msg->nextBlockFast(_PREHASH_ObjectData);
        msg->addU32Fast(_PREHASH_LocalID, local_id);
        msg->addStringFast(_PREHASH_Name, new_name);
        msg->sendReliable(host);
    }

    if (has_desc)
    {
        std::string new_desc = params["description"].asString();
        msg->newMessageFast(_PREHASH_ObjectDescription);
        msg->nextBlockFast(_PREHASH_AgentData);
        msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
        msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
        msg->nextBlockFast(_PREHASH_ObjectData);
        msg->addU32Fast(_PREHASH_LocalID, local_id);
        msg->addStringFast(_PREHASH_Description, new_desc);
        msg->sendReliable(host);
    }

    if (has_perms)
    {
        U32 next_owner_mask = static_cast<U32>(params["permissions"]["next_owner"].asInteger());
        msg->newMessageFast(_PREHASH_ObjectPermissions);
        msg->nextBlockFast(_PREHASH_AgentData);
        msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
        msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
        msg->nextBlockFast(_PREHASH_HeaderData);
        msg->addBOOLFast(_PREHASH_Override, false);
        msg->nextBlockFast(_PREHASH_ObjectData);
        msg->addU32Fast(_PREHASH_ObjectLocalID, local_id);
        msg->addU8Fast(_PREHASH_Field, PERM_NEXT_OWNER);
        msg->addBOOLFast(_PREHASH_Set, true);
        msg->addU32Fast(_PREHASH_Mask, next_owner_mask);
        msg->sendReliable(host);
    }

    // ─────────────────────────────────────────────────────────────
    // Step 4: Return Success Response
    // ─────────────────────────────────────────────────────────────
    LLSD response;
    response["success"] = true;
    response["prim_id"] = prim_id.asString();
    return response;
}

LLSD LLScriptEditorWSServer::handleObjectItemModify(U32 connection_id, const LLSD& params)
{
    // ─────────────────────────────────────────────────────────────
    // Step 1: Parameter Validation
    // ─────────────────────────────────────────────────────────────
    if (!params.has("prim_id") || !params.has("item_id"))
        throw LLJSONRPCConnection::InvalidParams("prim_id and item_id are required");

    bool has_name = params.has("name");
    bool has_desc = params.has("description");
    bool has_perms = params.has("permissions") && params["permissions"].has("next_owner");

    if (!has_name && !has_desc && !has_perms)
        throw LLJSONRPCConnection::InvalidParams(
            "At least one property (name, description, or permissions) must be specified");

    // ─────────────────────────────────────────────────────────────
    // Step 2: Validate Published Item (reuse existing helper)
    // ─────────────────────────────────────────────────────────────
    ValidatedItem v = validatePublishedItem(params, PERM_MODIFY);

    LLUUID prim_id = params["prim_id"].asUUID();
    LLUUID item_id = params["item_id"].asUUID();

    // ─────────────────────────────────────────────────────────────
    // Step 3: Create Modified Item Copy
    // ─────────────────────────────────────────────────────────────
    LLPointer<LLViewerInventoryItem> new_item =
        new LLViewerInventoryItem(static_cast<LLViewerInventoryItem*>(v.item));

    if (has_name)
    {
        new_item->rename(params["name"].asString());
    }

    if (has_desc)
    {
        new_item->setDescription(params["description"].asString());
    }

    if (has_perms)
    {
        LLPermissions perm = new_item->getPermissions();
        U32 next_owner_mask = static_cast<U32>(params["permissions"]["next_owner"].asInteger());
        perm.setMaskNext(next_owner_mask);
        new_item->setPermissions(perm);
    }

    // ─────────────────────────────────────────────────────────────
    // Step 4: Send UpdateTaskInventory Message
    // ─────────────────────────────────────────────────────────────
    v.prim->updateInventory(new_item, TASK_INVENTORY_ITEM_KEY, false);

    // ─────────────────────────────────────────────────────────────
    // Step 5: Return Success Response
    // ─────────────────────────────────────────────────────────────
    LLSD response;
    response["success"] = true;
    response["prim_id"] = prim_id.asString();
    response["item_id"] = item_id.asString();
    return response;
}

void LLScriptEditorWSServer::broadcastLanguageChange()
{
    LLUUID syntax_id = LLSyntaxDefCache::instance().getSyntaxID();

    if (syntax_id != mLastSyntaxId)
    {
        mLastSyntaxId = syntax_id;
        LLSD params;
        params["id"] = syntax_id;

        if (isRunning())
        {
            broadcastNotification("language.syntax.change", params);
        }
    }
}

LLSD LLScriptEditorWSServer::handleLanguageIdRequest() const
{
    LLSD response;

    response["id"] = mLastSyntaxId;
    return response;
}

LLSD LLScriptEditorWSServer::handleSyntaxRequest(const LLSD& params) const
{
    LLSD        response(LLSD::emptyMap());
    std::string category = params["kind"].asString();

    if (category.empty())
    {
        response["error"] = "No syntax category specified";
        response["success"] = false;
        return response;
    }

    response["id"] = mLastSyntaxId;
    if (category == "defs.lua")
    {
        response["defs"] = LLSyntaxDefCache::instance().getLuaKeywords();
        response["success"] = response["defs"].isDefined();
    }
    else if (category == "defs.lsl")
    {
        response["defs"] = LLSyntaxDefCache::instance().getLSLKeywords();
        response["success"] = response["defs"].isDefined();
    }
    else
    {
        response["error"] = "Unknown syntax category requested";
        response["success"] = false;
    }
    return response;
}

LLSD LLScriptEditorWSServer::handleSyntaxCacheRequest() const
{
    LLSD response;
    // Add array of cached syntax definition files
    LLSD syntax_files = LLSD::emptyArray();
    for (const auto& name : LLSyntaxDefCache::instance().getCacheFileNames())
    {
        syntax_files.append(name);
    }
    response["files"] = syntax_files;
    response["success"] = true;
    return response;
}

LLSD LLScriptEditorWSServer::handleSyntaxCacheFileRequest(const LLSD& params) const
{
    std::string filename = params["filename"].asString();
    bool        as_json  = params["as_json"].asBoolean();

    LLSyntaxDefCache& cache = LLSyntaxDefCache::instance();
    LLSD              response;

    if (filename.empty())
    {
        response["error"] = "No filename specified";
        response["success"] = false;
        return response;
    }
    if (!cache.hasCacheFile(filename))
    {
        response["error"] = "Requested syntax cache file not found";
        response["success"] = false;
        return response;
    }
    bool success = false;
    if (as_json)
    {
        LLSD file_content   = cache.loadCacheFileAsLLSD(filename);
        if (file_content.isDefined())
        {
            response["content"] = file_content;
            success = true;
        }
        else
        {
            response["error"] = "Failed to load and format syntax cache file.";
        }
    }
    else
    {
        std::string content = cache.loadCacheFile(filename);
        if (!content.empty())
        {
            response["content"] = content;
            success = true;
        }
        else
        {
            response["error"] = "Failed to load syntax cache file";
        }
    }
    response["success"] = success;
    return response;
}

LLSD LLScriptEditorWSServer::handleScriptSubscribe(U32 connection_id, const LLSD& params)
{
    LLSD response(LLSD::emptyMap());

    std::string script_id = params["script_id"].asString();
    std::string script_name = params["script_name"].asString();
    std::string language    = params["script_language"].asString();

    SubscriptionError result = updateScriptSubscription(script_id, connection_id);

    response["script_id"] = script_id;
    response["success"]   = (result == SubscriptionError::SUCCESS);
    response["status"]    = static_cast<S32>(result);

    LL_WARNS_IF(result != SubscriptionError::SUCCESS, "ScriptEditorWS")
        << "Script connect request for script " << script_id << " failed with status " << static_cast<S32>(result) << LL_ENDL;
    switch (result)
    {
    case SubscriptionError::SUCCESS:
        response["message"] = "OK";
        break;
    case SubscriptionError::INVALID_EDITOR:
        response["message"] = "Invalid editor handle";
        break;
    case SubscriptionError::INVALID_SUBSCRIPTION:
        response["message"] = "No subscription found for script";
        break;
    case SubscriptionError::ALREADY_SUBSCRIBED:
        response["message"] = "Script already subscribed";
        break;
    case SubscriptionError::INTERNAL_ERROR:
        response["message"] = "Internal server error";
        break;
    }

    if (result == SubscriptionError::SUCCESS)
    {
        auto it = mSubscriptions.find(script_id);
        if (it != mSubscriptions.end())
        {
            LLViewerObject* object  = gObjectList.findObject((*it).second.mObjectID);
            response["object_id"] = (*it).second.mObjectID;
            //response["object_name"] = object ? object->getName() : "Unknown";
            response["item_id"] = (*it).second.mItemID;
        }
    }

    return response;
}

LLSD LLScriptEditorWSServer::handleScriptUnsubscribe(U32 connection_id, const LLSD& params)
{
    std::string script_id = params["script_id"].asString();

    auto it = mSubscriptions.find(script_id);
    if (it != mSubscriptions.end() && (it->second.mConnectionID == connection_id))
    {
        unsubscribeEditor(script_id);
    }
    return LLSD();
}

LLSD LLScriptEditorWSServer::handleFileWatcherFileListRequest() const
{
    LLSD response;

    response["temp_dir"] = LLFile::tmpdir();

    // Add array of script_id's from active scripts
    LLSD script_ids_array = LLSD::emptyArray();
    for (const auto& [script_id, subinfo] : mSubscriptions)
    {
        script_ids_array.append(script_id);
    }
    response["script_ids"] = script_ids_array;

    response["success"] = true;

    return response;
}

LLSD LLScriptEditorWSServer::handleObjectRequest(U32 connection_id, const LLSD& params)
{
    LLUUID object_id = params["object_id"].asUUID();
    LLSD response;

    if (object_id.isNull())
    {
        response["success"] = false;
        response["message"] = "No object_id specified";
        return response;
    }

    LLViewerObject* object = gObjectList.findObject(object_id);
    if (!object)
    {
        response["success"] = false;
        response["message"] = "Object not found";
        return response;
    }

    if (!object->permModify())
    {
        response["success"] = false;
        response["message"] = "Permission denied";
        return response;
    }

    bool accepted = publishObject(object_id);
    response["success"] = accepted;
    if (!accepted)
    {
        response["message"] = "Failed to initiate publish";
    }
    return response;
}

LLScriptEditorWSServer::ValidatedItem LLScriptEditorWSServer::validatePublishedItem(
    const LLSD& params, U32 permMask) const
{
    LLUUID prim_id = params["prim_id"].asUUID();
    LLUUID item_id = params["item_id"].asUUID();

    if (prim_id.isNull() || item_id.isNull())
        throw LLJSONRPCConnection::InvalidParams("prim_id and item_id are required");

    LLViewerObject* prim = gObjectList.findObject(prim_id);
    if (!prim)
        throw LLJSONRPCConnection::InvalidParams("Prim not found");

    LLViewerObject* root = prim->getRootEdit();
    if (!root || !isObjectPublished(root->getID()))
        throw LLJSONRPCConnection::ForbiddenError("Object is not published");

    LLInventoryItem* item = dynamic_cast<LLInventoryItem*>(prim->getInventoryObject(item_id));
    if (!item)
        throw LLJSONRPCConnection::InvalidParams("Item not found in prim inventory");

    LLAssetType::EType type = item->getType();
    if (type != LLAssetType::AT_LSL_TEXT && type != LLAssetType::AT_NOTECARD)
        throw LLJSONRPCConnection::InvalidParams("Item is not a script or notecard");

    if ((permMask & PERM_COPY) &&
        !gAgent.allowOperation(PERM_COPY, item->getPermissions(), GP_OBJECT_MANIPULATE))
        throw LLJSONRPCConnection::ForbiddenError("Insufficient permissions");

    if (permMask & PERM_MODIFY)
    {
        // Writes into task inventory require modify permission on both the
        // item AND the containing prim. A no-mod object can be published
        // (read-only), but its contents cannot be changed.
        if (!gAgent.allowOperation(PERM_MODIFY, item->getPermissions(), GP_OBJECT_MANIPULATE))
            throw LLJSONRPCConnection::ForbiddenError("Insufficient permissions");

        if (!prim->permModify())
            throw LLJSONRPCConnection::ForbiddenError("No modify permission on object");
    }

    return { prim, root, item, type };
}

LLSD LLScriptEditorWSServer::handleObjectContentGet(const std::string& method, const LLSD& id, const LLSD& params)
{
    // Permission policy for reading item contents:
    //   - Scripts:   require both PERM_COPY and PERM_MODIFY. No-copy or
    //                no-modify scripts cannot have their source exposed.
    //   - Notecards: no permission requirement -- no-mod notecards remain
    //                readable so external editors can view their contents.
    U32 required_perms = 0;
    {
        LLUUID prim_id_peek = params["prim_id"].asUUID();
        LLUUID item_id_peek = params["item_id"].asUUID();
        LLViewerObject* prim_peek = gObjectList.findObject(prim_id_peek);
        if (prim_peek)
        {
            if (auto* it = dynamic_cast<LLInventoryItem*>(prim_peek->getInventoryObject(item_id_peek)))
            {
                if (it->getType() == LLAssetType::AT_LSL_TEXT)
                    required_perms = PERM_COPY | PERM_MODIFY;
            }
        }
    }

    auto v = validatePublishedItem(params, required_perms);

    LLUUID prim_id = params["prim_id"].asUUID();
    LLUUID item_id = params["item_id"].asUUID();

    LLSD cb_result = await_async_result(
        "objectContentGet", ASSET_FETCH_TIMEOUT, "Asset fetch timed out",
        [&](const std::string& pump_name)
        {
            gAssetStorage->getInvItemAsset(
                v.prim->getRegion()->getHost(),
                gAgent.getID(),
                gAgent.getSessionID(),
                v.item->getPermissions().getOwner(),
                v.prim->getID(),
                v.item->getUUID(),
                v.item->getAssetUUID(),
                v.type,
                [pump_name](const LLUUID& asset_uuid, LLAssetType::EType asset_type, void*, S32 status, LLExtStat)
                {
                    LLSD result;
                    if (status == LL_ERR_NOERR)
                    {
                        result["asset_uuid"] = asset_uuid;
                        result["asset_type"] = static_cast<S32>(asset_type);
                    }
                    else
                    {
                        result["error"] = status;
                    }
                    LLEventPumps::instance().post(pump_name, result);
                },
                nullptr,
                true);
        });

    if (cb_result.has("error"))
    {
        S32 status = cb_result["error"].asInteger();
        if (status == LL_ERR_ASSET_REQUEST_NOT_IN_DATABASE || status == LL_ERR_FILE_EMPTY)
            throw LLJSONRPCConnection::InvalidParams("Asset not found");
        if (status == LL_ERR_INSUFFICIENT_PERMISSIONS)
            throw LLJSONRPCConnection::ForbiddenError("Insufficient permissions to read asset");
        throw LLJSONRPCConnection::InternalError("Asset fetch failed: " + std::to_string(status));
    }

    LLUUID             asset_uuid = cb_result["asset_uuid"].asUUID();
    LLAssetType::EType asset_type = static_cast<LLAssetType::EType>(cb_result["asset_type"].asInteger());

    LLFileSystem file(asset_uuid, asset_type);
    S32 file_length = file.getSize();
    if (file_length <= 0)
        throw LLJSONRPCConnection::InternalError("Asset file empty or not found in cache");

    std::vector<char> buffer(file_length + 1);
    file.read(reinterpret_cast<U8*>(buffer.data()), file_length);
    buffer[file_length] = '\0';

    std::string text_content;
    if (asset_type == LLAssetType::AT_NOTECARD)
    {
        // Notecards are stored in an envelope format -- use LLNotecard to extract the text
        LLNotecard notecard;
        std::istringstream istr(std::string(buffer.data(), file_length));
        if (notecard.importStream(istr))
        {
            text_content = notecard.getText();
        }
        else
        {
            throw LLJSONRPCConnection::InternalError("Failed to parse notecard format");
        }
    }
    else
    {
        text_content = std::string(buffer.data());
    }

    LLSD response;
    response["success"] = true;
    response["prim_id"] = prim_id;
    response["item_id"] = item_id;
    response["content"] = text_content;
    return response;
}

LLSD LLScriptEditorWSServer::handleObjectContentSave(const std::string& method, const LLSD& id, const LLSD& params)
{
    std::string content = params["content"].asString();
    if (content.empty())
        throw LLJSONRPCConnection::InvalidParams("content is required");

    auto v = validatePublishedItem(params, PERM_MODIFY);

    if (v.type == LLAssetType::AT_LSL_TEXT)
    {
        return saveScript(v.prim, v.item, content, params);
    }
    else
    {
        return saveNotecard(v.prim, v.item, content);
    }
}

LLSD LLScriptEditorWSServer::saveScript(LLViewerObject* prim, LLInventoryItem* item,
                                         const std::string& content, const LLSD& params)
{
    // Determine compile target
    std::string compile_target;
    if (params.has("vm"))
    {
        compile_target = params["vm"].asString();
        // The client sends "luau" for the Luau VM -- but if the script is LSL
        // (not native Luau), the internal compile target is "lsl-luau".
        if (compile_target == "luau" && item->getInventorySubType() != SST_LUA)
        {
            compile_target = "lsl-luau";
        }
    }
    else
    {
        U8 subtype = item->getInventorySubType();
        std::string runtime = item->getRuntime();
        bool is_lua = (subtype == SST_LUA);
        if (!is_lua && runtime == "luau")
            compile_target = "lsl-luau";
        else if (!runtime.empty())
            compile_target = runtime;
        else
        {
            is_lua = is_lua_script(content);
            compile_target = is_lua ? "luau" : "mono";
        }
    }

    std::string url = prim->getRegion()->getCapability("UpdateScriptTask");
    if (url.empty())
        throw LLJSONRPCConnection::InternalError("UpdateScriptTask capability not available");

    LLSD cb_result = await_async_result(
        "objectContentSave", SCRIPT_UPLOAD_TIMEOUT, "Script upload/compile timed out",
        [&](const std::string& pump_name)
        {
            auto [on_success, on_failure] = make_asset_upload_callbacks(pump_name);
            bool is_running = params.has("running") ? params["running"].asBoolean() : false;
            LLResourceUploadInfo::ptr_t uploadInfo(std::make_shared<LLScriptAssetUpload>(
                prim->getID(), item->getUUID(),
                compile_target, is_running, LLUUID::null, content,
                std::move(on_success), std::move(on_failure)));
            LLViewerAssetUpload::EnqueueInventoryUpload(url, uploadInfo);
        });

    if (cb_result.has("failed"))
        throw LLJSONRPCConnection::InternalError("Upload failed: " + cb_result["reason"].asString());

    LLSD response;
    response["success"]  = true;
    response["prim_id"]  = prim->getID();
    response["item_id"]  = item->getUUID();
    response["compiled"] = cb_result["compiled"];
    if (!cb_result["compiled"].asBoolean() && cb_result.has("errors"))
    {
        response["errors"] = cb_result["errors"];
    }

    // If the script is open in the viewer's editor, update it
    LLSD floater_key;
    floater_key["taskid"] = prim->getID();
    floater_key["itemid"] = item->getUUID();
    LLLiveLSLEditor* editor = LLFloaterReg::findTypedInstance<LLLiveLSLEditor>("preview_scriptedit", floater_key);
    if (editor)
    {
        LLScriptEdCore* sed = editor->getScriptEdCore();
        if (sed)
        {
            sed->setScriptText(LLStringExplicit(content), true);
            sed->makeEditorPristine();
        }
    }

    return response;
}

LLSD LLScriptEditorWSServer::saveNotecard(LLViewerObject* prim, LLInventoryItem* item,
                                           const std::string& content)
{
    std::string url = prim->getRegion()->getCapability("UpdateNotecardTaskInventory");
    if (url.empty())
        throw LLJSONRPCConnection::InternalError("UpdateNotecardTaskInventory capability not available");

    // Use LLNotecard to produce the proper notecard format
    LLNotecard notecard;
    notecard.setText(content);

    std::ostringstream ostr;
    notecard.exportStream(ostr);

    LLSD cb_result = await_async_result(
        "objectContentSaveNotecard", NOTECARD_UPLOAD_TIMEOUT, "Notecard upload timed out",
        [&](const std::string& pump_name)
        {
            auto [on_success, on_failure] = make_asset_upload_callbacks(pump_name);
            LLResourceUploadInfo::ptr_t uploadInfo(std::make_shared<LLBufferedAssetUploadInfo>(
                prim->getID(), item->getUUID(),
                LLAssetType::AT_NOTECARD, ostr.str(),
                std::move(on_success), std::move(on_failure)));
            LLViewerAssetUpload::EnqueueInventoryUpload(url, uploadInfo);
        });

    if (cb_result.has("failed"))
        throw LLJSONRPCConnection::InternalError("Upload failed: " + cb_result["reason"].asString());

    LLSD response;
    response["success"] = true;
    response["prim_id"] = prim->getID();
    response["item_id"] = item->getUUID();

    // If the notecard is open in the viewer's editor, update it
    LLSD floater_key;
    floater_key["taskid"] = prim->getID();
    floater_key["itemid"] = item->getUUID();
    LLPreviewNotecard* nc = LLFloaterReg::findTypedInstance<LLPreviewNotecard>("preview_notecard", floater_key);
    if (nc)
    {
        LLViewerTextEditor* nc_editor = nc->getChild<LLViewerTextEditor>("Notecard Editor");
        if (nc_editor)
        {
            nc_editor->setText(content);
            nc_editor->makePristine();
        }
    }

    return response;
}

LLSD LLScriptEditorWSServer::handleObjectItemDelete(U32 connection_id, const LLSD& params)
{
    auto v = validatePublishedItem(params, PERM_MODIFY);

    v.prim->removeInventory(v.item->getUUID());

    LLSD response;
    response["success"] = true;
    response["prim_id"] = params["prim_id"].asUUID();
    response["item_id"] = params["item_id"].asUUID();
    return response;
}

LLSD LLScriptEditorWSServer::handleObjectUnpublish(U32 connection_id, const LLSD& params)
{
    LLUUID object_id = params["object_id"].asUUID();
    if (object_id.isNull())
        throw LLJSONRPCConnection::InvalidParams("object_id is required");

    auto it = mPublishedObjects.find(object_id);
    if (it == mPublishedObjects.end())
        throw LLJSONRPCConnection::InvalidParams("Object is not published");
    unpublishObject(object_id, "manual");

    LLSD response;
    response["success"]   = true;
    response["object_id"] = object_id;
    return response;
}

LLSD LLScriptEditorWSServer::handleObjectItemCreate(const std::string& method, const LLSD& id, const LLSD& params)
{
    std::string type = params["type"].asString();
    if (type != "script" && type != "notecard")
    {
        throw LLJSONRPCConnection::InvalidParams("Unsupported item type: " + type);
    }

    LLUUID prim_id = params["prim_id"].asUUID();
    if (prim_id.isNull())
    {
        throw LLJSONRPCConnection::InvalidParams("prim_id is required");
    }

    LLViewerObject* prim = gObjectList.findObject(prim_id);
    if (!prim)
    {
        throw LLJSONRPCConnection::InvalidParams("Prim not found");
    }

    LLViewerObject* root = prim->getRootEdit();
    if (!root || !isObjectPublished(root->getID()))
    {
        throw LLJSONRPCConnection::ForbiddenError("Object is not published");
    }

    std::string name = params["name"].asString();
    if (name.empty())
    {
        throw LLJSONRPCConnection::InvalidParams("name is required");
    }

    bool has_cap = prim->getRegion() && !prim->getRegion()->getCapability("CreateTaskInventoryItem").empty();

    if (type == "notecard" && !has_cap)
    {
        throw LLJSONRPCConnection::ForbiddenError("Notecard creation requires CreateTaskInventoryItem capability");
    }

    // Resolve type-specific fields
    LLAssetType::EType asset_type;
    LLInventoryType::EType inv_type;
    U8 sub_type = 0;
    const char* perm_key;
    LLSD cap_params;

    if (type == "script")
    {
        std::string vm = params["vm"].asString();
        if (vm == "luau")
        {
            sub_type = SST_LUA;
        }
        else if (vm == "mono" || vm == "lsl2")
        {
            sub_type = SST_LSL;
        }
        else
        {
            throw LLJSONRPCConnection::InvalidParams("vm must be 'luau', 'mono', or 'lsl2'");
        }

        asset_type            = LLAssetType::AT_LSL_TEXT;
        inv_type              = LLInventoryType::IT_LSL;
        perm_key              = "Scripts";
        cap_params["enabled"] = true;
        cap_params["vm"]      = vm;
    }
    else
    {
        asset_type = LLAssetType::AT_NOTECARD;
        inv_type   = LLInventoryType::IT_NOTECARD;
        perm_key   = "Notecards";
        if (params.has("text"))
        {
            cap_params["text"] = params["text"].asString();
        }
    }

    LLPermissions perms;
    perms.init(gAgent.getID(), gAgent.getID(), LLUUID::null, LLUUID::null);
    perms.initMasks(
        PERM_ALL,
        PERM_ALL,
        LLFloaterPerms::getEveryonePerms(perm_key),
        LLFloaterPerms::getGroupPerms(perm_key),
        PERM_MOVE | LLFloaterPerms::getNextOwnerPerms(perm_key));

    std::string desc;
    LLViewerAssetType::generateDescriptionFor(asset_type, desc);

    // Snapshot existing item IDs before creation
    std::set<LLUUID> existing_items;
    {
        LLInventoryObject::object_list_t inv;
        prim->getInventoryContents(inv);
        for (auto& obj : inv)
        {
            existing_items.insert(obj->getUUID());
        }
    }

    // Reject if another item.create is already in flight for this prim; the
    // map keys by prim, so two concurrent creates would clobber one another.
    if (mPendingItemCreates.find(prim_id) != mPendingItemCreates.end())
    {
        throw LLJSONRPCConnection::InvalidRequest(
            "An item.create is already in flight for this prim");
    }

    // Set up event pump to wait for inventory change
    LLEventMailDrop result_pump("objectItemCreate." + LLUUID::generateNewID().asString(), true);
    mPendingItemCreates[prim_id] = result_pump.getName();

    // RAII: guarantee the pending entry is cleared on every exit path (throw
    // or normal return), so no exception between here and the erase-on-post
    // in onPrimInventoryChanged can leave a stale entry behind. Uses a
    // shared_ptr custom deleter as a lightweight scope guard.
    std::shared_ptr<void> pending_guard(nullptr, [this, prim_id](void*)
    {
        mPendingItemCreates.erase(prim_id);
    });

    if (has_cap)
    {
        prim->createInventoryItem(asset_type, inv_type, sub_type, name, desc, perms, cap_params,
            [pump_name = result_pump.getName()](bool success, const LLSD& response)
            {
                LLEventPumps::instance().obtain(pump_name).post(response);
            });
    }
    else
    {
        // Fallback: legacy RezScript UDP (scripts only — notecards already rejected above)
        LLPointer<LLViewerInventoryItem> new_item =
            new LLViewerInventoryItem(
                LLUUID::null, LLUUID::null, perms, LLUUID::null,
                asset_type, inv_type, name, desc, LLSaleInfo::DEFAULT,
                LLInventoryItemFlags::II_FLAGS_SUBTYPE_MASK & sub_type,
                time_corrected());
        prim->saveScript(new_item, true, true, LLUUID::null);
    }

    // Wait for inventory change callback
    LLSD event = llcoro::suspendUntilEventOnWithTimeout(result_pump, ITEM_CREATE_TIMEOUT, LLSD().with("timeout", true));

    if (event.has("timeout"))
    {
        throw LLJSONRPCConnection::RequestTimeoutError("Timed out waiting for item creation");
    }

    prim = gObjectList.findObject(prim_id);
    if (!prim)
    {
        throw LLJSONRPCConnection::InternalError("Prim no longer exists");
    }

    LLSD response;

    // If cap returned item_id directly, use it
    if (event.has("success") && event["success"].asBoolean() &&
        event.has("item_id") && event["item_id"].asUUID().notNull())
    {
        response["item_id"]     = event["item_id"];
        response["name"]        = event["name"];
        response["description"] = desc;
        response["type"]        = type;
        response["prim_id"]     = prim_id;

        if (type == "script")
        {
            response["subtype"] = static_cast<S32>(sub_type);
        }

        LLSD perm_entry;
        perm_entry["owner"]      = static_cast<S32>(perms.getMaskOwner());
        perm_entry["next_owner"] = static_cast<S32>(perms.getMaskNextOwner());
        response["permissions"]  = perm_entry;
        response["creator_id"]   = gAgent.getID();
    }
    else
    {
        // Fallback: search inventory (for UDP path or if cap didn't return item_id)
        LLInventoryObject::object_list_t inv;
        prim->getInventoryContents(inv);
        for (auto& obj : inv)
        {
            if (existing_items.find(obj->getUUID()) == existing_items.end())
            {
                LLInventoryItem* created = dynamic_cast<LLInventoryItem*>(obj.get());
                if (created && created->getType() == asset_type)
                {
                    response["item_id"]     = created->getUUID();
                    response["name"]        = created->getName();
                    response["description"] = created->getDescription();
                    response["type"]        = type;

                    if (type == "script")
                    {
                        response["subtype"] = static_cast<S32>(created->getInventorySubType());
                        const std::string& runtime = created->getRuntime();
                        if (!runtime.empty())
                        {
                            response["vm"] = runtime;
                        }
                    }

                    const LLPermissions& item_perms = created->getPermissions();
                    LLSD perm_entry;
                    perm_entry["owner"]      = static_cast<S32>(item_perms.getMaskOwner());
                    perm_entry["next_owner"] = static_cast<S32>(item_perms.getMaskNextOwner());
                    response["permissions"]  = perm_entry;
                    response["creator_id"]   = item_perms.getCreator();
                    response["prim_id"]      = prim_id;
                    break;
                }
            }
        }
    }

    if (!response.has("item_id"))
    {
        throw LLJSONRPCConnection::InternalError("Item was not found in updated inventory");
    }

    return response;
}


void LLScriptEditorWSServer::notifyScript(const std::string& script_id, const std::string &method, const LLSD& message) const
{
    auto it = mSubscriptions.find(script_id);
    if (it != mSubscriptions.end())
    {
        auto connection = it->second.mConnection.lock();
        if (connection)
        {
            connection->notify(method, message);
        }
    }
}


void LLScriptEditorWSServer::sendUnsubscribeScriptEditor(const std::string& script_id)
{
    LLSD params;
    params["script_id"] = script_id;

    notifyScript(script_id, "script.unsubscribe", params);
}

void LLScriptEditorWSServer::sendCompileResults(const std::string &script_id, const LLSD &results) const
{
    LLHandle<LLPanel> editor_handle = findEditorForScript(script_id);
    if (editor_handle.isDead())
    {
        return;
    }
    LLScriptEdContainer* editor = dynamic_cast<LLScriptEdContainer*>(editor_handle.get());
    if (!editor)
    {
        return;
    }
    LLScriptEdCore* core = editor->getScriptEdCore();
    bool is_lua = core && (core->isLuauLanguage());

    LLSD params;
    params["script_id"] = script_id;
    params["success"]  = results["compiled"].asBoolean();
    params["running"]  = results["is_running"].asBoolean();
    if (results.has("errors"))
    {
        params["errors"] = LLSD::emptyArray();

        if (is_lua)
        {   // lua errors: ":line: message", line is 1-based
            const static boost::regex lua_err_regex(R"(^[^:]*:(\d+): (.+)$)");

            for (const auto& err : llsd::inArray(results["errors"]))
            {
                boost::smatch match;
                LLSD err_entry;

                err_entry["column"] = 0; // TODO: Lua compiler does not provide column info
                err_entry["level"]  = "ERROR";

                if (boost::regex_match(err.asString(), match, lua_err_regex))
                {
                    S32 line_number = std::stoi(match[1].str());
                    std::string message = match[2].str();

                    err_entry["row"] = line_number;
                    err_entry["message"] = message;
                }
                else
                {
                    err_entry["row"] = 0;
                    err_entry["message"] = err.asString();
                }
                params["errors"].append(err_entry);
            }
        }
        else
        {   // lsl errors: "(line, column) : SEVERITY : message", line and column are 0-based
            static const boost::regex lsl_err_regex(R"(\((\d+), (\d+)\) : ([^:]+) : (.+))");

            for (const auto& err : llsd::inArray(results["errors"]))
            {
                boost::smatch match;
                LLSD err_entry;

                if (boost::regex_match(err.asString(), match, lsl_err_regex))
                {
                    S32         line_number = std::stoi(match[1].str());
                    S32         col_number = std::stoi(match[2].str());
                    std::string severity = match[3].str();
                    std::string message = match[4].str();

                    err_entry["row"]     = line_number + 1;
                    err_entry["column"]  = col_number + 1;
                    err_entry["level"]   = severity;
                    err_entry["message"] = message;
                    err_entry["format"]  = "lsl";
                }
                else
                {
                    err_entry["row"]     = 0;
                    err_entry["column"]  = 0;
                    err_entry["level"]   = "ERROR";
                    err_entry["message"] = err.asString();
                    err_entry["format"]  = "lsl";
                }
                params["errors"].append(err_entry);
            }
        }
    }

    notifyScript(script_id, "script.compiled", params);
}

void LLScriptEditorWSServer::forwardChatToIDE(const LLChat& chat_msg) const
{
    auto it = std::find_if(mSubscriptions.begin(), mSubscriptions.end(),
                           [&chat_msg](const auto& pair) { return (pair.second.mObjectID == chat_msg.mFromID); });

    if (it == mSubscriptions.end())
    { // Not a script we are tracking
        return;
    }

    bool is_error = false;
    std::string error_message;
    std::string object_name;
    std::string script_name;
    S32         line_number = 0;
    // We have at least one script from this object, we will forward the message to the IDE
    // but first we need to see if it is a runtime error
    std::vector<std::string> lines = LLStringUtil::getTokens(chat_msg.mText, "\n");
    // If this is a runtime error, the first line will look like: "<Object Name> [script:<Script Name>] Script run-time error"
    static const std::string runtime_error_marker = "Script run-time error";
    auto ends_with = [](const std::string& s, const std::string& suffix)
    {
        return s.size() >= suffix.size() &&
               std::equal(suffix.rbegin(), suffix.rend(), s.rbegin());
    };
    if (!lines.empty() && ends_with(lines.front(), runtime_error_marker))
    {
        is_error = true;
        std::string first_line = lines.front();

        // Extract the object and script name from the first line
        static const boost::regex RUNTIME_ERR_REGEX_FLEX(R"(^(.+?)\s+\[script:([^\]]+)\]\s+Script run-time error)");
        boost::smatch m;

        S32 remove_count = 0;
        if (boost::regex_match(first_line, m, RUNTIME_ERR_REGEX_FLEX))
        {
            object_name = m[1].str();
            script_name = m[2].str();
            remove_count++;
        }

        // TODO: Build an actual error message to forward to the external editor.
        // The complete error message arrives as two or three separate chat
        // messages from the server (2 for LSL / non-owner Lua, 3 for owner Lua):
        //   Message 1: <Object Name> [script:<Script Name>] Script run-time error
        //   Message 2: <runtime error>
        //   Message 3: <script>:<line>: <actual error message>\n<call stack>
        // These need to be composited into a single error message for the IDE.
        if (lines.size() > remove_count)
        {   // The rest of the lines may contain a stack trace
            lines.erase(lines.begin(), lines.begin() + remove_count);
        }
        else
        {
            lines.clear();
        }

        // We should also check that the script name matches one of our subscriptions
        if (!script_name.empty() && (it->second.mScriptName != script_name))
        {   // right object, wrong script
            auto sit = std::find_if(mSubscriptions.begin(), mSubscriptions.end(),
                [&chat_msg, &script_name](const auto& pair)
                {
                    return (pair.second.mScriptName == script_name) && (pair.second.mObjectID == chat_msg.mFromID);
                });
            if (sit != mSubscriptions.end())
            {   // We have a better match
                it = sit;
            }
        }
    }
    std::string script_id = it->first;
    LLSD message;
    message["script_id"] = script_id;
    message["object_id"] = chat_msg.mFromID;
    message["object_name"] = chat_msg.mFromName;
    message["message"]     = chat_msg.mText;

    if (is_error)
    {
        message["error"] = error_message;
        message["line"] = line_number;
        if (!lines.empty())
        {
            message["stack"] = LLSD::emptyArray();
            for (const auto& line : lines)
            {
                message["stack"].append(line);
            }
        }
    }

    if (!it->second.mConnection.expired())
    {
        it->second.mConnection.lock()->notify(is_error ? "runtime.error" : "runtime.debug", message);
    }
}

void LLScriptEditorWSServer::notifyConnection(U32 connection_id, const std::string& method, const LLSD& params) const
{
    auto it = mActiveConnections.find(connection_id);
    if (it != mActiveConnections.end())
    {
        auto connection = it->second.lock();
        if (connection)
        {
            connection->notify(method, params);
        }
    }
}

void LLScriptEditorWSServer::notifyAll(const std::string& method, const LLSD& params) const
{
    // Serialize once, deliver many: build the JSON-RPC envelope and its wire
    // string a single time, then hand the bytes to each connection.
    LLSD envelope = LLJSONRPCConnection::makeEnvelope(
        LLSD(), method, params, LLSD(), LLSD());
    std::string payload = boost::json::serialize(LlsdToJson(envelope));

    for (const auto& pair : mActiveConnections)
    {
        auto connection = pair.second.lock();
        if (connection)
        {
            connection->sendMessage(payload);
        }
    }
}


// static
LLSD LLScriptEditorWSServer::errorResponse(const std::string& message)
{
    LLSD response;
    response["success"] = false;
    response["message"] = message;
    return response;
}

// static
std::string LLScriptEditorWSServer::getPrimName(LLViewerObject* obj)
{
    std::string name = nv_string(obj, "Name");
    if (!name.empty())
    {
        return name;
    }

    if (!obj)
    {
        return std::string();
    }

    LLSelectNode* node = LLSelectMgr::instance().getSelection()->findNode(obj);
    return (node && !node->mName.empty()) ? node->mName : std::string();
}

LLSD LLScriptEditorWSServer::buildPrimInventoryLLSD(LLViewerObject* object) const
{
    LLSD items = LLSD::emptyArray();
    if (!object) return items;

    LLInventoryObject::object_list_t contents;
    object->getInventoryContents(contents);

    for (const auto& obj : contents)
    {
        LLInventoryItem* item = dynamic_cast<LLInventoryItem*>(obj.get());
        if (!item) continue;

        LLAssetType::EType type = item->getType();

        // Filter: only scripts and notecards
        if (type != LLAssetType::AT_LSL_TEXT && type != LLAssetType::AT_NOTECARD)
        {
            continue;
        }

        LLSD entry;
        entry["item_id"]     = item->getUUID();
        entry["name"]        = item->getName();
        entry["description"] = item->getDescription();
        entry["type"]        = (type == LLAssetType::AT_LSL_TEXT) ? "script" : "notecard";

        if (type == LLAssetType::AT_LSL_TEXT)
        {
            U8 subtype = item->getInventorySubType();
            entry["subtype"] = static_cast<S32>(subtype);  // 0=LSL, 1=Luau

            const std::string& runtime = item->getRuntime();
            if (!runtime.empty())
            {
                entry["vm"] = runtime;
            }

            // Script runtime state from task inventory cap
            LLViewerInventoryItem* viewer_item = dynamic_cast<LLViewerInventoryItem*>(item);
            if (viewer_item)
            {
                entry["running"] = viewer_item->getIsRunning();
                entry["faulted"] = viewer_item->getIsFaulted();
            }
        }

        // Permissions
        const LLPermissions& perms = item->getPermissions();
        LLSD perm_entry;
        perm_entry["owner"]      = static_cast<S32>(perms.getMaskOwner());
        perm_entry["next_owner"] = static_cast<S32>(perms.getMaskNextOwner());
        entry["permissions"]     = perm_entry;

        entry["creator_id"] = perms.getCreator();

        items.append(entry);
    }

    return items;
}

bool LLScriptEditorWSServer::publishObject(const LLUUID& object_id)
{
    LLViewerObject* root = gObjectList.findObject(object_id);
    if (!root)
    {
        LL_WARNS("ScriptEditorWS") << "publishObject: object not found: " << object_id << LL_ENDL;
        return false;
    }

    if (!root->permModify())
    {
        LL_WARNS("ScriptEditorWS") << "publishObject: no modify permission on object: " << object_id << LL_ENDL;
        return false;
    }

    // If already published, unpublish first to replace cleanly
    if (isObjectPublished(object_id))
    {
        unpublishObject(object_id, "republish");
    }

    // Collect root + all children
    std::vector<LLViewerObject*> prims = collect_linkset(root);

    // Set up a PendingPublish to coordinate inventory loading across all prims.
    // We register a listener and call requestInventory() on every prim.
    // If inventory is already loaded, requestInventory() fires the callback
    // synchronously via doInventoryCallback(), so all_ready will naturally
    // become true before this function returns in the common case.
    PendingPublish pending;
    pending.mObjectID     = object_id;

    for (LLViewerObject* prim : prims)
    {
        pending.mPendingPrims.insert(prim->getID());
        auto listener = std::make_unique<LLPublishedPrimListener>(
            this, object_id, prim->getID(), prim);
        pending.mListeners.push_back(std::move(listener));
    }

    mPendingPublishes[object_id] = std::move(pending);

    // Request inventory for each prim. If already loaded, onPrimInventoryReady()
    // will be called immediately (possibly building and sending the publish
    // before this loop even finishes).
    for (LLViewerObject* prim : prims)
    {
        if (mPendingPublishes.find(object_id) == mPendingPublishes.end())
        {
            break;  // publish completed synchronously during a previous iteration
        }
        prim->requestInventory();
    }

    return true;
}

bool LLScriptEditorWSServer::isObjectPublished(const LLUUID& object_id) const
{
    return mPublishedObjects.find(object_id) != mPublishedObjects.end();
}

void LLScriptEditorWSServer::onPrimInventoryReady(const LLUUID& object_id, const LLUUID& prim_id)
{
    auto it = mPendingPublishes.find(object_id);
    if (it == mPendingPublishes.end()) return;

    it->second.mPendingPrims.erase(prim_id);

    if (it->second.mPendingPrims.empty())
    {
        LL_DEBUGS("ScriptEditorWS") << "All prim inventories ready for object " << object_id << LL_ENDL;
        buildAndSendPublish(object_id);
    }
}

LLSD LLScriptEditorWSServer::buildPublishedObjectLLSD(LLViewerObject* root) const
{
    LLSD pub;
    pub["object_id"]          = root->getID();
    pub["object_name"]        = getPrimName(root);
    pub["object_description"] = nv_string(root, "Desc");
    pub["owner_id"]           = root->mOwnerID;
    if (root->getRegion())
    {
        pub["region"] = root->getRegion()->getName();
    }
    pub["inventory"] = buildPrimInventoryLLSD(root);

    LLSD linked_objects = LLSD::emptyArray();
    S32 link_number = 2;
    for (LLViewerObject* child : root->getChildren())
    {
        LLSD link;
        link["link_id"]          = child->getID();
        link["link_number"]      = link_number++;
        link["link_name"]        = getPrimName(child);
        link["link_description"] = nv_string(child, "Desc");
        link["inventory"]        = buildPrimInventoryLLSD(child);
        linked_objects.append(link);
    }
    if (linked_objects.size() > 0)
    {
        pub["linked_objects"] = linked_objects;
    }

    return pub;
}

void LLScriptEditorWSServer::buildAndSendPublish(const LLUUID& object_id)
{
    auto pending_it = mPendingPublishes.find(object_id);
    if (pending_it == mPendingPublishes.end())
    {
        LL_WARNS("ScriptEditorWS") << "buildAndSendPublish: no pending publish for " << object_id << LL_ENDL;
        return;
    }

    LLViewerObject* root = gObjectList.findObject(object_id);
    if (!root)
    {
        LL_WARNS("ScriptEditorWS") << "buildAndSendPublish: root object gone: " << object_id << LL_ENDL;
        mPendingPublishes.erase(pending_it);
        return;
    }

    LLSD pub = buildPublishedObjectLLSD(root);

    // Store in the published registry
    PublishedObjectInfo info;
    info.mObjectID          = root->getID();
    info.mOwnerID           = root->mOwnerID;
    info.mObjectName        = pub["object_name"].asString();
    info.mObjectDescription = pub["object_description"].asString();
    if (root->getRegion())
    {
        info.mRegionName = root->getRegion()->getName();
    }

    S32 link_num = 1;
    std::vector<LLViewerObject*> prims = collect_linkset(root);
    for (LLViewerObject* prim : prims)
    {
        PublishedPrimInfo prim_info;
        prim_info.mPrimID          = prim->getID();
        prim_info.mPrimName        = getPrimName(prim);  // Use helper with selection fallback
        prim_info.mLinkNumber      = link_num++;
        prim_info.mInventorySerial = static_cast<S16>(prim->getInventorySerial());
        info.mPrims.push_back(prim_info);
    }

    mPublishedObjects[object_id] = std::move(info);
    mPublishedObjects[object_id].mListeners = std::move(pending_it->second.mListeners);
    mPendingPublishes.erase(pending_it);

    // Send notification
    LLSD message;
    message["object"] = pub;
    notifyAll("object.publish", message);

    LL_INFOS("ScriptEditorWS") << "Published object " << object_id
        << " (" << pub["object_name"].asString() << ") with "
        << (prims.size() - 1) << " linked prim(s)" << LL_ENDL;
}

void LLScriptEditorWSServer::onLinksetChildAdded(const LLUUID& root_id, LLViewerObject* child)
{
    auto obj_it = mPublishedObjects.find(root_id);
    if (obj_it == mPublishedObjects.end()) return;

    const LLUUID child_id = child->getID();
    PublishedObjectInfo& info = obj_it->second;

    // Add a placeholder slot so the flush can enumerate the full linkset
    // even before inventory arrives. flushLinksetUpdate renumbers from the
    // live mPrims list, so the tentative link_number here is just informational.
    PublishedPrimInfo prim_info;
    prim_info.mPrimID          = child_id;
    prim_info.mPrimName        = getPrimName(child);
    prim_info.mLinkNumber      = static_cast<S32>(info.mPrims.size()) + 1;
    prim_info.mInventorySerial = -1; // sentinel: not yet loaded
    info.mPrims.push_back(prim_info);

    // Register a listener so we are notified when the child's inventory arrives.
    // The listener constructor calls registerVOInventoryListener internally.
    auto listener = std::make_unique<LLPublishedPrimListener>(this, root_id, child_id, child);
    info.mListeners.push_back(std::move(listener));

    // Request inventory (async; fires onPrimInventoryChanged when ready)
    child->requestInventory();

    // Mark as pending — the flush waits until this is cleared
    mNewChildPrims[root_id].insert(child_id);

    // Start safety-timeout timer (no-op if one is already pending for this root)
    scheduleLinksetFlush(root_id, LINKSET_ADD_FLUSH_DELAY);
}

void LLScriptEditorWSServer::onLinksetChildRemoved(const LLUUID& root_id, const LLUUID& child_id)
{
    auto obj_it = mPublishedObjects.find(root_id);
    if (obj_it == mPublishedObjects.end()) return;

    PublishedObjectInfo& info = obj_it->second;

    // Remove prim slot
    info.mPrims.erase(
        std::remove_if(info.mPrims.begin(), info.mPrims.end(),
            [&](const PublishedPrimInfo& p) { return p.mPrimID == child_id; }),
        info.mPrims.end());

    // Destroy the prim's inventory listener
    info.mListeners.erase(
        std::remove_if(info.mListeners.begin(), info.mListeners.end(),
            [&](const std::unique_ptr<LLPublishedPrimListener>& l)
            { return l->getPrimID() == child_id; }),
        info.mListeners.end());

    // Remove from pending-inventory set (child may have been added then removed
    // before its inventory ever arrived)
    auto nc_it = mNewChildPrims.find(root_id);
    if (nc_it != mNewChildPrims.end())
    {
        nc_it->second.erase(child_id);
        if (nc_it->second.empty())
            mNewChildPrims.erase(nc_it);
    }

    // Re-number remaining children (root stays 1, children get 2..N in order)
    S32 link_num = 2;
    for (auto& p : info.mPrims)
    {
        if (p.mPrimID != root_id)
            p.mLinkNumber = link_num++;
    }

    // Schedule coalesced flush — multiple simultaneous removes share one timer
    scheduleLinksetFlush(root_id, LINKSET_REMOVE_FLUSH_DELAY);
}

void LLScriptEditorWSServer::scheduleLinksetFlush(const LLUUID& root_id, F32 delay)
{
    // No-op if a timer is already pending for this root_id
    auto it = mLinksetFlushTimers.find(root_id);
    if (it != mLinksetFlushTimers.end() && !it->second.expired())
        return;

    wptr_t weak = std::static_pointer_cast<LLScriptEditorWSServer>(shared_from_this());
    LLEventTimer* t = LLEventTimer::run_after(delay, [weak, root_id]()
    {
        if (auto self = weak.lock())
        {
            self->mLinksetFlushTimers.erase(root_id);
            self->mNewChildPrims.erase(root_id); // clear any remaining pending children (timeout path)
            self->flushLinksetUpdate(root_id);
        }
    });
    mLinksetFlushTimers[root_id] = t->getWeak();
}

void LLScriptEditorWSServer::cancelLinksetFlushTimer(const LLUUID& root_id)
{
    auto it = mLinksetFlushTimers.find(root_id);
    if (it == mLinksetFlushTimers.end())
        return;
    if (auto locked = it->second.lock())
    {
        // LLEventTimer contract (see lleventtimer.h): the shared_ptr held by
        // LLInstanceTracker uses a no-op deleter, so this raw delete is safe
        // and is the documented way to cancel a pending timer.
        delete locked.get();
    }
    mLinksetFlushTimers.erase(it);
}

void LLScriptEditorWSServer::flushLinksetUpdate(const LLUUID& root_id)
{
    auto obj_it = mPublishedObjects.find(root_id);
    if (obj_it == mPublishedObjects.end()) return;

    const PublishedObjectInfo& info = obj_it->second;

    // Build full linked_objects replacement (children only, in link_number order)
    LLSD linked_objects(LLSD::TypeArray);
    for (const PublishedPrimInfo& prim_info : info.mPrims)
    {
        if (prim_info.mPrimID == root_id) continue; // root is not in linked_objects

        LLSD entry;
        entry["link_id"]     = prim_info.mPrimID;
        entry["link_number"] = prim_info.mLinkNumber;

        LLViewerObject* prim = gObjectList.findObject(prim_info.mPrimID);
        // Always read the name fresh from the live object so newly-linked prims
        // whose NV pair was not yet available at addChild time still get a name.
        std::string link_name = prim ? getPrimName(prim) : std::string();
        if (link_name.empty()) link_name = prim_info.mPrimName; // fallback to stored name
        entry["link_name"]   = link_name;
        entry["inventory"]   = prim ? buildPrimInventoryLLSD(prim) : LLSD(LLSD::TypeArray);

        linked_objects.append(entry);
    }

    LLSD update;
    update["object_id"]      = root_id;
    update["linked_objects"] = linked_objects;
    notifyAll("object.update", update);

    LL_INFOS("ScriptEditorWS") << "Linkset update for " << root_id
        << ": " << linked_objects.size() << " child(ren)" << LL_ENDL;
}

void LLScriptEditorWSServer::onPrimInventoryChanged(const LLUUID& object_id, const LLUUID& prim_id)
{
    auto pub_it = mPublishedObjects.find(object_id);
    if (pub_it == mPublishedObjects.end())
        return;

    LLViewerObject* prim = gObjectList.findObject(prim_id);
    if (!prim)
        return;

    // ── New-child path ────────────────────────────────────────────────────
    // When a child was linked in via onLinksetChildAdded it is placed in
    // mNewChildPrims until its first inventory response arrives here.
    auto nc_root_it = mNewChildPrims.find(object_id);
    if (nc_root_it != mNewChildPrims.end() && nc_root_it->second.count(prim_id))
    {
        // Update the placeholder with the real prim name now that we have data
        for (auto& p : pub_it->second.mPrims)
        {
            if (p.mPrimID == prim_id)
            {
                p.mPrimName        = getPrimName(prim);
                p.mInventorySerial = 0; // mark as loaded
                break;
            }
        }

        nc_root_it->second.erase(prim_id);

        if (nc_root_it->second.empty())
        {
            // All new children have inventory — cancel timeout, flush now
            mNewChildPrims.erase(nc_root_it);
            cancelLinksetFlushTimer(object_id);
            flushLinksetUpdate(object_id);
        }
        // else: still waiting for other new children
        return;
    }
    // ── Normal inventory-change path ──────────────────────────────────────

    LLSD update;
    update["object_id"] = object_id;

    LLSD inv = buildPrimInventoryLLSD(prim);
    if (prim_id == object_id)
    {
        // Root prim — use top-level inventory field (full replacement)
        update["inventory"] = inv;
    }
    else
    {
        // Child prim — wrap in changes.linked_objects.modified so the extension
        // routes the update to the correct linked prim directory
        LLSD modified_entry;
        modified_entry["link_id"]   = prim_id;
        modified_entry["inventory"] = inv;
        LLSD modified_arr = LLSD::emptyArray();
        modified_arr.append(modified_entry);
        update["changes"]["linked_objects"]["modified"] = modified_arr;
    }

    notifyAll("object.update", update);

    // Signal any pending item.create coroutine waiting on this prim
    auto create_it = mPendingItemCreates.find(prim_id);
    if (create_it != mPendingItemCreates.end())
    {
        LLEventPumps::instance().post(create_it->second, LLSD().with("prim_id", prim_id));
        mPendingItemCreates.erase(create_it);
    }

    LL_DEBUGS("ScriptEditorWS") << "Sent object.update for prim " << prim_id
                                << " in object " << object_id << LL_ENDL;
}

void LLScriptEditorWSServer::onObjectPropertyChanged(
    const LLUUID& prim_id, const std::string& name, const std::string& desc)
{
    LLViewerObject* prim = gObjectList.findObject(prim_id);
    if (!prim) return;
    LLUUID root_id = prim->getRootEdit()->getID();

    auto pub_it = mPublishedObjects.find(root_id);
    if (pub_it == mPublishedObjects.end()) return;

    LLSD update;
    update["object_id"] = root_id;

    if (prim_id == root_id)
    {
        bool name_changed = (pub_it->second.mObjectName != name);
        bool desc_changed = (pub_it->second.mObjectDescription != desc);
        if (!name_changed && !desc_changed) return;

        if (name_changed) { pub_it->second.mObjectName = name; update["object_name"] = name; }
        if (desc_changed) { pub_it->second.mObjectDescription = desc; update["object_description"] = desc; }
    }
    else
    {
        auto prim_it = std::find_if(pub_it->second.mPrims.begin(), pub_it->second.mPrims.end(),
            [&](const PublishedPrimInfo& p) { return p.mPrimID == prim_id; });
        if (prim_it == pub_it->second.mPrims.end()) return;
        if (prim_it->mPrimName == name) return;

        prim_it->mPrimName = name;
        LLSD modified_entry;
        modified_entry["link_id"]   = prim_id;
        modified_entry["link_name"] = name;
        LLSD modified_arr = LLSD::emptyArray();
        modified_arr.append(modified_entry);
        update["changes"]["linked_objects"]["modified"] = modified_arr;
    }

    notifyAll("object.update", update);
}

void LLScriptEditorWSServer::cleanupPrimListeners(const LLUUID& object_id)
{
    // Clear any pending publish listeners
    auto pending_it = mPendingPublishes.find(object_id);
    if (pending_it != mPendingPublishes.end())
    {
        pending_it->second.mListeners.clear();  // unique_ptrs call removeVOInventoryListener()
        mPendingPublishes.erase(pending_it);
    }

    // Clear published object listeners (Phase 4)
    auto pub_it = mPublishedObjects.find(object_id);
    if (pub_it != mPublishedObjects.end())
    {
        pub_it->second.mListeners.clear();
    }
}

void LLScriptEditorWSServer::unpublishObject(const LLUUID& object_id, const std::string& reason)
{
    auto it = mPublishedObjects.find(object_id);
    if (it == mPublishedObjects.end())
    {
        // May still have a pending publish in progress -- cancel it
        cleanupPrimListeners(object_id);
        return;
    }

    cleanupPrimListeners(object_id);
    mPublishedObjects.erase(it);

    // Cancel any pending linkset flush so it cannot fire after removal
    cancelLinksetFlushTimer(object_id);
    mNewChildPrims.erase(object_id);

    LLSD message;
    message["object_id"] = object_id;
    if (!reason.empty())
    {
        message["reason"] = reason;
    }
    notifyAll("object.unpublish", message);

    LL_DEBUGS("ScriptEditorWS") << "Unpublished object " << object_id
        << " reason: " << reason << LL_ENDL;
}


//========================================================================
std::atomic<U32> LLScriptEditorWSConnection::sNextConnectionID{1};

std::shared_ptr<LLScriptEditorWSServer> LLScriptEditorWSConnection::getServer() const
{
    return std::static_pointer_cast<LLScriptEditorWSServer>(mOwningServer.lock());
}

void LLScriptEditorWSConnection::onOpen()
{
    // Call parent class to set up JSON-RPC infrastructure
    LLJSONRPCConnection::onOpen();

    LL_INFOS("ScriptEditorWS") << "Script editor JSON-RPC connection opened" << LL_ENDL;

    // Build hello data
    LLSD handshake;
    handshake["server_version"]   = "1.0.0";
    handshake["protocol_version"] = "1.0";
    handshake["viewer_name"]      = LLVersionInfo::instance().getChannel();
    handshake["viewer_version"]   = LLVersionInfo::instance().getVersion();

    handshake["agent_id"] = gAgent.getID();
    handshake["agent_name"] = gAgentUsername;

    std::string challenge_file = generateChallenge();
    if (!challenge_file.empty())
    {
        handshake["challenge"] = challenge_file;
    }

    LLSD languages = LLSD::emptyArray();
    languages.append("lsl");
    languages.append("luau");
    handshake["languages"] = languages;
    handshake["syntax_id"] = LLSyntaxDefCache::instance().getSyntaxID();

    // Features object
    LLSD features;
    features["live_sync"]        = true;
    features["compilation"]      = true;
    features["syntax_cache"]     = true;
    handshake["features"]        = features;

    wptr_t that = weak_from_this();

    // Send session.handshake method call and the response
    call("session.handshake", handshake, [that](const LLSD& result, const LLSD& error) {
        if (error.isUndefined())
        {
            auto self = that.lock();
            if (self)
            {
                self->handleHandshakeResponse(result);
            }
        }
        else
        {
            LL_WARNS("ScriptEditorWS") << "Handshake failed: "
                                       << error["message"].asString() << LL_ENDL;
        }
    });

    LL_INFOS("ScriptEditorWS") << "Sent handshake call to new editor client" << LL_ENDL;
}

void LLScriptEditorWSConnection::onClose()
{
    // Call parent class to clean up JSON-RPC infrastructure
    LLJSONRPCConnection::onClose();
    mOwningServer.reset();

    // Clean up handshake response data
    mClientName.clear();
    mClientVersion.clear();
    mProtocolVersion.clear();
    mScriptName.clear();
    mScriptLanguage.clear();
    mLanguages.clear();
    mFeatures.clear();
}

void LLScriptEditorWSConnection::sendDisconnect(DisconnectReason reason, const std::string& message)
{
    LL_INFOS("ScriptEditorWS") << "Sending disconnect to client: " << message << LL_ENDL;
    LLSD params;
    params["reason"]  = static_cast<S32>(reason);
    params["message"] = message;
    notify("session.disconnect", params);
    closeConnection(1000, message);
}

void LLScriptEditorWSConnection::handleHandshakeResponse(const LLSD& result)
{
    LL_INFOS("ScriptEditorWS") << "Processing handshake response from client" << LL_ENDL;

    mClientName      = result["client_name"].asString();
    mClientVersion   = result["client_version"].asString();
    mProtocolVersion = result["protocol_version"].asString();

    // Validate challenge response (if a challenge was issued).
    const bool challenge_issued = mChallenge.notNull();
    bool       valid_response   = true;
    if (challenge_issued)
    {
        valid_response = result.has("challenge_response") &&
            (result["challenge_response"].asUUID() == mChallenge);
        mChallenge.setNull();
    }

    // Always clean up the temporary challenge file if one was created,
    // regardless of validation outcome.
    if (!mChallengeFile.empty())
    {
        LLFile::remove(mChallengeFile);
        mChallengeFile.clear();
    }

    if (challenge_issued && !valid_response)
    {
        LL_WARNS("ScriptEditorWS") << "Invalid or missing challenge response from client" << LL_ENDL;
        sendDisconnect(DisconnectReason::PROTOCOL_ERROR, "Invalid challenge response");
        return;
    }

    if (mProtocolVersion != "1.0")
    {
        LL_WARNS("ScriptEditorWS") << "Protocol version mismatch. Expected: 1.0, Got: "
                                    << mProtocolVersion << LL_ENDL;
    }

    mScriptName     = result["script_name"].asString();
    mScriptLanguage = result["script_language"].asString();

    for (const auto& lang : llsd::inArray(result["languages"]))
    {
        if (lang.isString())
        {
            mLanguages.insert(lang.asString());
        }
    }

    for (const auto& [feature, enabled] : llsd::inMap(result["features"]))
    {
        if (enabled.asBoolean())
        {
            mFeatures.insert(feature);
        }
    }

    notify("session.ok");

    LL_INFOS("ScriptEditorWS") << "Handshake completed successfully." << LL_ENDL;
}

std::string LLScriptEditorWSConnection::generateChallenge()
{
    mChallenge.generate();

    mChallengeFile = std::string(LLFile::tmpdir()) + "sl_script_challenge_" + mChallenge.asString() + ".tmp";

    llofstream file(mChallengeFile.c_str());
    if (!file.is_open())
    {
        LL_WARNS("ScriptEditorWS") << "Unable to open challenge file: " << mChallengeFile << LL_ENDL;
        mChallenge.setNull();
        mChallengeFile.clear();
        return std::string();
    }

    file << mChallenge;
    file.close();

    return mChallengeFile;
}
