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
#include "llagentcamera.h"
#include "llappviewer.h"
#include "llchat.h"
#include "llcompilequeue.h"
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
#include "llnotificationsutil.h"
#include "llpreviewnotecard.h"
#include "llpreviewscript.h"
#include "llprocess.h"
#include "llregex.h"
#include "llmd5.h"
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
#include "llviewermenu.h"
#include "llviewertexteditor.h"
#include "llvoinventorylistener.h"
#include "roles_constants.h"

#include <array>

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

    static const boost::regex LUAU_LOCATION_PATTERN(
        R"(^([^:]*):([0-9]+):\s*(.*)$)");

    static const boost::regex LSL_LOCATION_PATTERN(
        R"(\((\d+), (\d+)\) : ([^:]+) : (.+))");

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

    LLSD object_id_command_params()
    {
        LLSD params(LLSD::emptyMap());
        params["object_id"]["type"] = "string";
        params["object_id"]["required"] = true;
        return params;
    }

}

//========================================================================
LLScriptEditorWSServer::LLScriptEditorWSServer(const std::string& name, U16 port, bool local_only):
    LLJSONRPCServer(name, port, local_only),
    mPublishedObjectManager(
        this,
        [this](const LLPublishedObjectMgr::RuntimeChatEvent& event)
        {
            sendRuntimeEvent(event);
        })
{
    LL_INFOS("ScriptEditorWS") << "Created JSON-RPC script editor server: " << name
                               << " on port " << port << LL_ENDL;

    registerCommand({ "viewer.teleport", "Teleport agent to an in-world object",
                      object_id_command_params() },
        [](U32, const LLSD& p) -> LLSD
        {
            LLUUID object_id = p["object_id"].asUUID();
            if (object_id.isNull())
                throw LLJSONRPCConnection::InvalidParams("object_id is required");

            LLViewerObject* object = gObjectList.findObject(object_id);
            if (!object)
                throw LLJSONRPCConnection::InvalidParams("object_id not found");

            LLVector3d global_pos = object->getPositionGlobal();
            gAgent.teleportViaLocation(global_pos);

            LLSD response;
            response["success"] = true;
            return response;
        });

    registerCommand({ "viewer.camera.focus",
                      "Zoom camera to an in-world object (same behavior as context menu Zoom In)",
                      object_id_command_params() },
        [](U32, const LLSD& p) -> LLSD
        {
            LLUUID object_id = p["object_id"].asUUID();
            if (object_id.isNull())
                throw LLJSONRPCConnection::InvalidParams("object_id is required");

            if (!handle_zoom_to_object(object_id))
            {
                throw LLJSONRPCConnection::InternalError(
                    "Object not found or not reachable");
            }

            LLSD response;
            response["success"] = true;
            return response;
        });

    registerCommand({ "viewer.object.save_back_to_contents",
                      "Save an in-world object back to source object contents",
                      object_id_command_params() },
        [this](U32 connection_id, const LLSD& p) -> LLSD
        {
            return this->handleSaveBackToObjectContents(connection_id, p);
        });

    registerCommand({ "viewer.script.reset_all",
                      "Reset all scripts in an in-world object",
                      object_id_command_params() },
        [this](U32 connection_id, const LLSD& p) -> LLSD
        {
            return this->handleObjectScriptResetAll(connection_id, p);
        });

    registerCommand({ "viewer.script.recompile_all",
                      "Recompile all scripts in an in-world object",
                      LLSDMap("object_id",
                              LLSDMap("type", "string")
                                  ("required", true))
                          ("target",
                              LLSDMap("type", "string")
                                  ("required", true)
                                  ("description",
                                      "Compilation target: luau, lsl2, mono, or auto")) },
        [this](U32 connection_id, const LLSD& p) -> LLSD
        {
            return this->handleObjectScriptRecompileAll(connection_id, p);
        });
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

bool LLScriptEditorWSServer::isEnabled()
{
    static LLCachedControl<bool> OPT_ENABLESCRIPTEDITORWS(gSavedSettings, "ExternalWebsocketSyncEnable", false);
    return OPT_ENABLESCRIPTEDITORWS;
}

bool LLScriptEditorWSServer::isTightIntegration()
{
    static LLCachedControl<bool> OPT_TIGHTINTEGRATION(gSavedSettings, "ExternalWebsocketSyncTightIntegration", false);
    return OPT_TIGHTINTEGRATION;
}

LLScriptEditorWSServer::ptr_t LLScriptEditorWSServer::ensureServerRunning()
{
    if (!LLScriptEditorWSServer::isEnabled())
    {
        LL_DEBUGS("ScriptEditorWS") << "WebSocket server is disabled by ExternalWebsocketSyncEnable" << LL_ENDL;
        return nullptr;
    }

    LLWebsocketMgr& wsmgr = LLWebsocketMgr::instance();
    ptr_t server = std::static_pointer_cast<LLScriptEditorWSServer>(
        wsmgr.findServerByName(DEFAULT_SERVER_NAME));

    if (server && !server->isRunning())
    {
        // thread stopped/was joined
        wsmgr.removeServer(DEFAULT_SERVER_NAME);
        server.reset();
    }

    if (!server)
    {
        U16  port       = static_cast<U16>(gSavedSettings.getS32("ExternalWebsocketSyncPort"));
        bool local_only = gSavedSettings.getBOOL("ExternalWebsocketSyncLocal");
        server = std::make_shared<LLScriptEditorWSServer>(DEFAULT_SERVER_NAME, port, local_only);
        wsmgr.addServer(server);
    }

    if (!server->isRunning())
    {
        U16 port = static_cast<U16>(gSavedSettings.getS32("ExternalWebsocketSyncPort"));
        LLSD args;
        args["PORT"] = static_cast<S32>(port);

        if (!wsmgr.startServer(DEFAULT_SERVER_NAME))
        {
            LL_WARNS("ScriptEditorWS") << "Failed to start script editor websocket server" << LL_ENDL;
            LLNotificationsUtil::add("ExternalEditorServerFailed", args);
            return nullptr;
        }

        LLNotificationsUtil::add("ExternalEditorServerStarted", args);
    }

    return server;
}

std::string LLScriptEditorWSServer::buildScriptSubscriptionId(const LLUUID& object_id,
                                                              const LLUUID& item_id)
{
    std::string script_id = object_id.asString() + "_" + item_id.asString();

    std::array<char, MD5HEX_STR_SIZE> script_id_hash_str = {};
    LLMD5 script_id_hash((const U8*)script_id.c_str());
    script_id_hash.hex_digest(script_id_hash_str.data());

    return std::string(script_id_hash_str.data());
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

    mPublishedObjectManager.clearAllStateWithListenerCleanup();

    mSubscriptions.clear();
    mActiveConnections.clear();

    LL_INFOS("ScriptEditorWS") << "Script editor WebSocket server stopped, all state cleaned up" << LL_ENDL;

    LLNotificationsUtil::add("ExternalEditorServerStopped");
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
            ItemRef item_ref;
            item_ref.mPrimID = object_id;
            item_ref.mItemID = item_id;
            item_ref.mScriptName = script_name;
            mSubscriptions.emplace(script_id,
                EditorSubscription(item_ref, editor_handle));
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

        if (connection_id != 0)
        {
            auto cit = mConnectionSubscriptionCounts.find(connection_id);
            if (cit != mConnectionSubscriptionCounts.end())
            {
                if (--cit->second <= 0)
                {
                    mConnectionSubscriptionCounts.erase(cit);
                }
            }
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

        script_connection->registerAsyncMethod("script.subscribe",
            bindHandler([connection_id](LLScriptEditorWSServer& s, auto&, auto&, const LLSD& params)
            {
                return s.handleScriptSubscribe(connection_id, params);
            }));

        script_connection->registerMethod("script.list",
            bindHandler([](LLScriptEditorWSServer& s, auto&, auto&, auto&)
            {
                return s.handleFileWatcherFileListRequest();
            }));

        script_connection->registerAsyncMethod("object.unpublish",
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

        script_connection->registerAsyncMethod("command.execute",
            bindHandler([connection_id](LLScriptEditorWSServer& s, auto&, auto&, const LLSD& params)
            {
                return s.handleCommandExecute(connection_id, params);
            }));

        script_connection->registerMethod("command.list",
            bindHandler([](LLScriptEditorWSServer& s, auto&, auto&, auto&)
            {
                return s.handleCommandList();
            }));
    }
}

LLSD LLScriptEditorWSServer::handleObjectList() const
{
    LLSD response;
    response["objects"] = mPublishedObjectManager.buildObjectListLLSD();
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

LLSD LLScriptEditorWSServer::handleObjectScriptResetAll(U32 connection_id, const LLSD& params)
{
    LLUUID prim_id = params["object_id"].asUUID();
    if (prim_id.isNull())
    {
        throw LLJSONRPCConnection::InvalidParams("object_id is required");
    }

    LLViewerObject* prim = gObjectList.findObject(prim_id);
    if (!prim)
    {
        throw LLJSONRPCConnection::InvalidParams("Object not found");
    }

    LLViewerObject* root = prim->getRootEdit();
    if (!root || !isObjectPublished(root->getID()))
    {
        throw LLJSONRPCConnection::ForbiddenError("Object is not published");
    }

    if (!prim->flagScripted())
    {
        throw LLJSONRPCConnection::InvalidParams(
            "Prim contains no scripts");
    }

    if (!prim->permModify())
    {
        throw LLJSONRPCConnection::ForbiddenError(
            "No modify permission on prim");
    }

    LLUUID queue_id;
    queue_id.generate();

    LLFloaterScriptQueue* queue =
        LLFloaterReg::getTypedInstance<LLFloaterScriptQueue>(
            "reset_queue", LLSD(queue_id));
    if (!queue)
    {
        throw LLJSONRPCConnection::InternalError(
            "Unable to open reset queue");
    }

    queue->addObject(prim->getID(), prim->getID().asString());
    if (!queue->start())
    {
        queue->closeFloater();
        throw LLJSONRPCConnection::InternalError(
            "Unable to start reset queue");
    }

    queue->setTitle(LLTrans::getString("ResetQueueTitle"));

    LLSD response;
    response["success"] = true;
    response["object_id"] = prim->getID();
    response["queued"] = true;
    return response;
}

LLSD LLScriptEditorWSServer::handleObjectScriptRecompileAll(
    U32 connection_id, const LLSD& params)
{
    LLUUID object_id = params["object_id"].asUUID();
    if (object_id.isNull())
    {
        throw LLJSONRPCConnection::InvalidParams("object_id is required");
    }

    std::string target = params["target"].asString();
    if (target != "luau" &&
        target != "lsl2" &&
        target != "mono" &&
        target != "auto")
    {
        throw LLJSONRPCConnection::InvalidParams(
            "target must be 'luau', 'lsl2', 'mono', or 'auto'");
    }

    LLViewerObject* object = gObjectList.findObject(object_id);
    if (!object)
    {
        throw LLJSONRPCConnection::InvalidParams("Object not found");
    }

    LLViewerObject* root = object->getRootEdit();
    if (!root || root->getID() != object_id || !isObjectPublished(root->getID()))
    {
        throw LLJSONRPCConnection::ForbiddenError("Object is not published");
    }

    if (!root->flagScripted())
    {
        throw LLJSONRPCConnection::InvalidParams(
            "Object contains no scripts");
    }

    if (!root->permModify())
    {
        throw LLJSONRPCConnection::ForbiddenError(
            "No modify permission on object");
    }

    if (target == "luau")
    {
        target = "auto-luau";
    }

    LLUUID queue_id;
    queue_id.generate();

    LLFloaterCompileQueue* queue =
        LLFloaterReg::getTypedInstance<LLFloaterCompileQueue>(
            "compile_queue", LLSD(queue_id));
    if (!queue)
    {
        throw LLJSONRPCConnection::InternalError(
            "Unable to open compile queue");
    }

    queue->setCompileTarget(target);
    queue->addObject(root->getID(), root->getID().asString());
    if (!queue->start())
    {
        queue->closeFloater();
        throw LLJSONRPCConnection::InternalError(
            "Unable to start compile queue");
    }

    queue->setTitle(LLTrans::getString("CompileQueueTitle"));

    LLSD response;
    response["success"] = true;
    response["object_id"] = root->getID();
    response["target"] = (target == "auto-luau") ? "luau" : target;
    response["queued"] = true;
    return response;
}

LLSD LLScriptEditorWSServer::handleObjectModify(U32 connection_id, const LLSD& params)
{
    // Step 1: Parameter Validation
    LLUUID prim_id = params["prim_id"].asUUID();
    if (prim_id.isNull())
        throw LLJSONRPCConnection::InvalidParams("prim_id is required");

    bool has_name = params.has("name");
    bool has_desc = params.has("description");
    bool has_perms = params.has("permissions") && params["permissions"].has("next_owner");

    if (!has_name && !has_desc && !has_perms)
        throw LLJSONRPCConnection::InvalidParams(
            "At least one property (name, description, or permissions) must be specified");

    // Step 2: Find and Validate Object
    LLViewerObject* prim = gObjectList.findObject(prim_id);
    if (!prim)
        throw LLJSONRPCConnection::InvalidParams("Prim not found");

    LLViewerObject* root = prim->getRootEdit();
    if (!root || !isObjectPublished(root->getID()))
        throw LLJSONRPCConnection::ForbiddenError("Object is not published");

    if (!prim->permModify())
        throw LLJSONRPCConnection::ForbiddenError("No modify permission on object");

    // Step 3: Send Property Update Messages
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

    // Step 4: Return Success Response
    LLSD response;
    response["success"] = true;
    response["prim_id"] = prim_id.asString();
    return response;
}

LLSD LLScriptEditorWSServer::handleObjectItemModify(U32 connection_id, const LLSD& params)
{
    // Step 1: Parameter Validation
    if (!params.has("prim_id") || !params.has("item_id"))
        throw LLJSONRPCConnection::InvalidParams("prim_id and item_id are required");

    bool has_name = params.has("name");
    bool has_desc = params.has("description");
    bool has_perms = params.has("permissions") && params["permissions"].has("next_owner");

    if (!has_name && !has_desc && !has_perms)
        throw LLJSONRPCConnection::InvalidParams(
            "At least one property (name, description, or permissions) must be specified");

    // Step 2: Validate Published Item (reuse existing helper)
    ValidatedItem v = validatePublishedItem(params, PERM_MODIFY);

    LLUUID prim_id = params["prim_id"].asUUID();
    LLUUID item_id = params["item_id"].asUUID();

    // Step 3: Create Modified Item Copy
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

    // Step 4: Send UpdateTaskInventory Message
    v.prim->updateInventory(new_item, TASK_INVENTORY_ITEM_KEY, false);

    // Step 5: Return Success Response
    LLSD response;
    response["success"] = true;
    response["prim_id"] = prim_id.asString();
    response["item_id"] = item_id.asString();
    return response;
}

void LLScriptEditorWSServer::registerCommand(const WSCommandInfo& info, WSCommandHandler handler)
{
    mCommandRegistry.emplace(info.command, std::make_pair(info, std::move(handler)));
}

bool LLScriptEditorWSConnection::hasFeature(const std::string& feature) const
{
    return mFeatures.count(feature) > 0;
}

LLSD LLScriptEditorWSServer::handleSaveBackToObjectContents(U32 connection_id, const LLSD& params)
{
    LLUUID object_id = params["object_id"].asUUID();
    if (object_id.isNull())
    {
        throw LLJSONRPCConnection::InvalidParams("object_id is required");
    }

    const LLPublishedObjectMgr::PublishedObjectInfo* published_info =
        mPublishedObjectManager.getPublished(object_id);
    if (!published_info)
    {
        throw LLJSONRPCConnection::InvalidParams(
            "Object is not published");
    }

    if (!published_info->mCanSaveBackToContents || published_info->mSourceTaskID.isNull())
    {
        throw LLJSONRPCConnection::ForbiddenError(
            "Save back is not available for this object");
    }

    LLViewerObject* root = gObjectList.findObject(object_id);
    if (!root)
    {
        throw LLJSONRPCConnection::InvalidParams(
            "object_id not found");
    }

    if (!save_object_back_to_contents(root, published_info->mSourceTaskID))
    {
        throw LLJSONRPCConnection::InternalError(
            "Failed to save object back to contents");
    }

    LL_DEBUGS("ScriptEditorWS") << "Save-back requested via command for object "
                                << object_id << " on connection " << connection_id << LL_ENDL;

    LLSD response;
    response["success"] = true;

    LLSD result;
    result["object_id"] = object_id;
    response["result"] = result;
    return response;
}

LLSD LLScriptEditorWSServer::handleCommandExecute(U32 connection_id, const LLSD& params)
{
    const std::string command = params["command"].asString();
    if (command.empty())
    {
        throw LLJSONRPCConnection::InvalidParams("command is required");
    }

    auto it = mCommandRegistry.find(command);
    if (it == mCommandRegistry.end())
    {
        throw LLJSONRPCConnection::InvalidParams(
            "Unknown command: " + command);
    }

    return it->second.second(connection_id, params["params"]);
}

LLSD LLScriptEditorWSServer::handleCommandList()
{
    LLSD commands(LLSD::emptyArray());
    for (const auto& [name, entry] : mCommandRegistry)
    {
        LLSD info;
        info["command"]     = entry.first.command;
        info["description"] = entry.first.description;
        if (!entry.first.params.isUndefined())
        {
            info["params"] = entry.first.params;
        }
        commands.append(info);
    }
    LLSD response;
    response["commands"] = commands;
    return response;
}

void LLScriptEditorWSServer::sendCommandExecute(
    U32 connection_id, const std::string& command, const LLSD& params)
{
    auto it = mActiveConnections.find(connection_id);
    if (it == mActiveConnections.end())
    {
        return;
    }

    auto connection = it->second.lock();
    if (!connection || !connection->hasFeature("commands"))
    {
        return;
    }

    LLSD call_params;
    call_params["command"] = command;
    call_params["params"]  = params;

    connection->call("command.execute", call_params,
        [command](const LLSD& result, const LLSD& error)
        {
            LL_WARNS_IF(!error.isUndefined() || !result["success"].asBoolean(), "WSCommand")
                << "command.execute failed for " << command << LL_ENDL;
        });
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

LLSD LLScriptEditorWSServer::handlePing(
    const LLJSONRPCConnection::ptr_t& connection,
    const LLSD& params) const
{
    LLSD result;
    result["pong"] = "pong";

    if (params.has("timestamp"))
    {
        result["timestamp"] = params["timestamp"];
    }

    result["server_time"] = static_cast<LLSD::Integer>(
        LLDate::now().secondsSinceEpoch() * 1000.0);
    return result;
}

LLSD LLScriptEditorWSServer::handleGetVersion(
    const LLJSONRPCConnection::ptr_t& connection,
    const LLSD& params) const
{
    LLSD result;
    result["client_name"] = LLVersionInfo::instance().getChannel();
    result["client_version"] = LLVersionInfo::instance().getVersion();
    return result;
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
        throw LLJSONRPCConnection::InvalidParams(
            "No syntax category specified");
    }

    response["id"] = mLastSyntaxId;
    if (category == "defs.lua")
    {
        response["defs"] = LLSyntaxDefCache::instance().getLuaKeywords();
    }
    else if (category == "defs.lsl")
    {
        response["defs"] = LLSyntaxDefCache::instance().getLSLKeywords();
    }
    else
    {
        throw LLJSONRPCConnection::InvalidParams(
            "Unknown syntax category requested");
    }

    if (!response["defs"].isDefined())
    {
        throw LLJSONRPCConnection::InternalError(
            "Syntax definitions are unavailable");
    }

    response["success"] = true;
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
        throw LLJSONRPCConnection::InvalidParams(
            "No filename specified");
    }
    if (!cache.hasCacheFile(filename))
    {
        throw LLJSONRPCConnection::InvalidParams(
            "Requested syntax cache file not found");
    }
    if (as_json)
    {
        LLSD file_content   = cache.loadCacheFileAsLLSD(filename);
        if (file_content.isDefined())
        {
            response["content"] = file_content;
        }
        else
        {
            throw LLJSONRPCConnection::InternalError(
                "Failed to load and format syntax cache file.");
        }
    }
    else
    {
        std::string content = cache.loadCacheFile(filename);
        if (!content.empty())
        {
            response["content"] = content;
        }
        else
        {
            throw LLJSONRPCConnection::InternalError(
                "Failed to load syntax cache file");
        }
    }
    response["success"] = true;
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
            LLUUID prim_id = (*it).second.mItemRef.mPrimID;
            LLUUID root_id = prim_id;
            LLViewerObject* object = gObjectList.findObject(prim_id);
            if (object)
            {
                LLViewerObject* root = object->getRootEdit();
                if (root)
                {
                    root_id = root->getID();
                }
            }

            response["object_id"] = prim_id;
            response["root_id"] = root_id;
            //response["object_name"] = object ? object->getName() : "Unknown";
            response["item_id"] = (*it).second.mItemRef.mItemID;
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
        throw LLJSONRPCConnection::InvalidParams(
            "No object_id specified");
    }

    LLViewerObject* object = gObjectList.findObject(object_id);
    if (!object)
    {
        throw LLJSONRPCConnection::InvalidParams(
            "Object not found");
    }

    if (!object->permModify())
    {
        throw LLJSONRPCConnection::ForbiddenError(
            "Permission denied");
    }

    bool accepted = publishObject(object_id);
    if (!accepted)
    {
        throw LLJSONRPCConnection::InternalError(
            "Failed to initiate publish");
    }

    response["success"] = true;
    return response;
}

// Helper function to validate that the specified prim and
// item are valid, published, and have the required permissions.
// Throws JSON-RPC exceptions if validation fails.
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
    response["encoding"] = "utf-8";
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

    // The task inventory can be refreshed while the upload is in flight,
    // invalidating the raw item pointer returned by validatePublishedItem().
    // Keep stable identifiers for use after await_async_result().
    const LLUUID prim_id = prim->getID();
    const LLUUID item_id = item->getUUID();

    std::string url = prim->getRegion()->getCapability("UpdateScriptTask");
    if (url.empty())
        throw LLJSONRPCConnection::InternalError("UpdateScriptTask capability not available");

    LLSD cb_result = await_async_result(
        "objectContentSave", SCRIPT_UPLOAD_TIMEOUT, "Script upload/compile timed out",
        [&, prim_id, item_id](const std::string& pump_name)
        {
            auto [on_success, on_failure] = make_asset_upload_callbacks(pump_name);
            const LLViewerInventoryItem* viewer_item =
                dynamic_cast<const LLViewerInventoryItem*>(item);
            bool is_running = viewer_item ? viewer_item->getIsRunning() : false;
            if (params.has("running"))
            {
                is_running = params["running"].asBoolean();
            }
            LLResourceUploadInfo::ptr_t uploadInfo(std::make_shared<LLScriptAssetUpload>(
                prim_id, item_id,
                compile_target, is_running, LLUUID::null, content,
                std::move(on_success), std::move(on_failure)));
            LLViewerAssetUpload::EnqueueInventoryUpload(url, uploadInfo);
        });

    if (cb_result.has("failed"))
        throw LLJSONRPCConnection::InternalError("Upload failed: " + cb_result["reason"].asString());

    LLSD response;
    response["success"]  = true;
    response["prim_id"]  = prim_id;
    response["item_id"]  = item_id;
    response["compiled"] = cb_result["compiled"];
    if (!cb_result["compiled"].asBoolean() && cb_result.has("errors"))
    {
        response["diagnostics"] = LLSD::emptyArray();

        const bool is_lua =
            compile_target == "luau" ||
            compile_target == "lsl-luau";

        for (const auto& error : llsd::inArray(cb_result["errors"]))
        {
            boost::smatch match;
            LLSD diagnostic;
            diagnostic["level"] = "ERROR";

            if (is_lua &&
                boost::regex_match(
                    error.asString(),
                    match,
                    LUAU_LOCATION_PATTERN))
            {
                diagnostic["row"] = std::stoi(match[2].str());
                diagnostic["column"] = 0;
                diagnostic["message"] = match[3].str();
            }
            else if (!is_lua &&
                     boost::regex_match(
                         error.asString(),
                         match,
                         LSL_LOCATION_PATTERN))
            {
                diagnostic["row"] =
                    std::stoi(match[1].str()) + 1;
                diagnostic["column"] =
                    std::stoi(match[2].str()) + 1;
                diagnostic["level"] = match[3].str();
                diagnostic["message"] = match[4].str();
                diagnostic["format"] = "lsl";
            }
            else
            {
                diagnostic["row"] = 0;
                diagnostic["column"] = 0;
                diagnostic["message"] = error.asString();
            }

            response["diagnostics"].append(diagnostic);
        }
    }

    // If the script is open in the viewer's editor, update it
    LLSD floater_key;
    floater_key["taskid"] = prim_id;
    floater_key["itemid"] = item_id;
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
    // The task inventory can be refreshed while the upload is in flight,
    // invalidating the raw item pointer returned by validatePublishedItem().
    // Keep stable identifiers for use after await_async_result().
    const LLUUID prim_id = prim->getID();
    const LLUUID item_id = item->getUUID();

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
        [&, prim_id, item_id](const std::string& pump_name)
        {
            auto [on_success, on_failure] = make_asset_upload_callbacks(pump_name);
            LLResourceUploadInfo::ptr_t uploadInfo(std::make_shared<LLBufferedAssetUploadInfo>(
                prim_id, item_id,
                LLAssetType::AT_NOTECARD, ostr.str(),
                std::move(on_success), std::move(on_failure)));
            LLViewerAssetUpload::EnqueueInventoryUpload(url, uploadInfo);
        });

    if (cb_result.has("failed"))
        throw LLJSONRPCConnection::InternalError("Upload failed: " + cb_result["reason"].asString());

    LLSD response;
    response["success"] = true;
    response["prim_id"] = prim_id;
    response["item_id"] = item_id;

    // If the notecard is open in the viewer's editor, update it
    LLSD floater_key;
    floater_key["taskid"] = prim_id;
    floater_key["itemid"] = item_id;
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

    const LLUUID prim_id = v.prim->getID();
    const LLUUID root_id = v.root->getID();
    const LLUUID item_id = v.item->getUUID();

    // Optimistic local delete then emit immediate update
    // for published clients and request authoritative server refresh.
    v.prim->removeInventory(item_id);
    onPrimInventoryChanged(root_id, prim_id);

    if (!mPublishedObjectManager.hasInventoryRequestStart(prim_id))
    {
        v.prim->dirtyInventory();
        mPublishedObjectManager.setInventoryRequestStart(
            prim_id,
            LLTimer::getTotalSeconds().value());
        v.prim->requestInventory();
    }

    LLSD response;
    response["success"] = true;
    response["prim_id"] = prim_id;
    response["item_id"] = item_id;
    return response;
}

LLSD LLScriptEditorWSServer::handleObjectUnpublish(U32 connection_id, const LLSD& params)
{
    LLUUID object_id = params["object_id"].asUUID();
    if (object_id.isNull())
    {
        throw LLJSONRPCConnection::InvalidParams("object_id is required");
    }

    if (!mPublishedObjectManager.hasPublished(object_id))
    {
        throw LLJSONRPCConnection::InvalidParams("Object is not published");
    }

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

    // Set up event pump to wait for inventory change
    LLEventMailDrop result_pump("objectItemCreate." + LLUUID::generateNewID().asString(), true);

    // Reject if another item.create is already in flight for this prim; the
    // map keys by prim, so two concurrent creates would clobber one another.
    if (!mPublishedObjectManager.reservePendingItemCreate(prim_id, result_pump.getName()))
    {
        throw LLJSONRPCConnection::InvalidRequest(
            "An item.create is already in flight for this prim");
    }

    // RAII: guarantee the pending entry is cleared on every exit path (throw
    // or normal return), so no exception between here and the erase-on-post
    // in onPrimInventoryChanged can leave a stale entry behind. Uses a
    // shared_ptr custom deleter as a lightweight scope guard.
    std::shared_ptr<void> pending_guard(nullptr, [this, prim_id](void*)
    {
        mPublishedObjectManager.clearPendingItemCreate(prim_id);
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
        // Fallback: legacy RezScript UDP (scripts only -- notecards already rejected above)
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
    LLSD params;
    params["script_id"] = script_id;

    notifyScript(script_id, "script.unsubscribe", params);
}

void LLScriptEditorWSServer::sendCompileResults(const std::string &script_id, const LLSD &results) const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
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
        params["diagnostics"] = LLSD::emptyArray();

        if (is_lua)
        {   // lua errors: ":line: message", line is 1-based
            for (const auto& err : llsd::inArray(results["errors"]))
            {
                boost::smatch match;
                LLSD err_entry;

                err_entry["column"] = 0; // TODO: Lua compiler does not provide column info
                err_entry["level"]  = "ERROR";

                if (boost::regex_match(err.asString(), match, LUAU_LOCATION_PATTERN))
                {
                    S32 line_number = std::stoi(match[2].str());
                    std::string message = match[3].str();

                    err_entry["row"] = line_number;
                    err_entry["message"] = message;
                }
                else
                {
                    err_entry["row"] = 0;
                    err_entry["message"] = err.asString();
                }
                params["diagnostics"].append(err_entry);
            }
        }
        else
        {   // lsl errors: "(line, column) : SEVERITY : message", line and column are 0-based
            for (const auto& err : llsd::inArray(results["errors"]))
            {
                boost::smatch match;
                LLSD err_entry;

                if (boost::regex_match(err.asString(), match, LSL_LOCATION_PATTERN))
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
                params["diagnostics"].append(err_entry);
            }
        }
    }

    notifyScript(script_id, "script.compiled", params);
}

void LLScriptEditorWSServer::forwardChatToIDE(
    const LLChat& chat_msg,
    LLPublishedObjectMgr::RuntimeEventAggregator::Channel channel) const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;

    mPublishedObjectManager.ingestRuntimeChat(chat_msg, channel);
}

void LLScriptEditorWSServer::sendRuntimeEvent(
    const LLPublishedObjectMgr::RuntimeChatEvent& event) const
{

    std::string script_id;
    if (event.mItemID.notNull())
    {
        script_id = buildScriptSubscriptionId(event.mPrimID, event.mItemID);
    }

    if (!isObjectPublished(event.mRootID) &&
        (script_id.empty() || mSubscriptions.find(script_id) == mSubscriptions.end()))
    {
        return;
    }

    LLSD message;
    if (!script_id.empty())
    {
        message["script_id"] = script_id;
    }
    message["object_id"] = event.mRootID;
    message["prim_id"] = event.mPrimID;
    message["item_id"] = event.mItemID;
    message["object_name"] = event.mObjectName;
    message["message"] = event.mMessage;

    LLSD item;
    item["root_id"] = event.mRootID;
    item["prim_id"] = event.mPrimID;
    item["item_id"] = event.mItemID;
    item["name"] = event.mScriptName;
    item["language"] =
        event.mVM == LLPublishedObjectMgr::RuntimeEventAggregator::VM::LUAU
            ? "luau"
            : "lsl";
    message["item"] = item;

    switch (event.mChannel)
    {
    case LLPublishedObjectMgr::RuntimeEventAggregator::Channel::DEBUG:
        message["channel"] = "debug";
        break;
    case LLPublishedObjectMgr::RuntimeEventAggregator::Channel::OWNER_SAY:
        message["channel"] = "owner_say";
        break;
    }

    if (event.mIsError)
    {
        message["error"] = event.mError;
        message["line"] = event.mLine;
        message["column"] = event.mColumn;
        message["stack"] = LLSD::emptyArray();
        for (const auto& line : event.mStack)
        {
            message["stack"].append(line);
        }
    }

    notifyAll(event.mIsError ? "runtime.error" : "runtime.debug", message);
}

void LLScriptEditorWSServer::notifyConnection(U32 connection_id, const std::string& method, const LLSD& params) const
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
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
    if (node && !node->mName.empty())
    {
        return node->mName;
    }

    // Never emit an empty prim/object name to downstream tooling.
    return obj->getID().asString();
}

bool LLScriptEditorWSServer::publishObject(const LLUUID& object_id)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
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

    // Request object properties for each prim in the linkset (root + children),
    // matching the hover path so name/description metadata is refreshed.
    for (LLViewerObject* prim : prims)
    {
        LLSelectMgr::instance().requestObjectPropertiesFamily(prim);
    }

    // Set up a PendingPublish to coordinate inventory loading across all prims.
    // We register a listener and call requestInventory() on every prim.
    // If inventory is already loaded, requestInventory() fires the callback
    // synchronously via doInventoryCallback(), so all_ready will naturally
    // become true before this function returns in the common case.
    mPublishedObjectManager.beginPendingPublish(object_id, prims);

    // Request inventory for each prim. If already loaded, onPrimInventoryReady()
    // will be called immediately (possibly building and sending the publish
    // before this loop even finishes).
    for (LLViewerObject* prim : prims)
    {
        if (!mPublishedObjectManager.hasPendingPublish(object_id))
        {
            break;  // publish completed synchronously during a previous iteration
        }
        mPublishedObjectManager.setInventoryRequestStart(prim->getID(), LLTimer::getTotalSeconds().value());
        prim->requestInventory();
    }

    return true;
}

bool LLScriptEditorWSServer::isObjectPublished(const LLUUID& object_id) const
{
    return mPublishedObjectManager.hasPublished(object_id);
}

void LLScriptEditorWSServer::onPrimInventoryReady(const LLUUID& object_id, const LLUUID& prim_id)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
    if (mPublishedObjectManager.handlePrimInventoryReadyEvent(object_id, prim_id))
    {
        LL_DEBUGS("ScriptEditorWS") << "All prim inventories ready for object " << object_id << LL_ENDL;
        buildAndSendPublish(object_id);
    }
}

void LLScriptEditorWSServer::buildAndSendPublish(const LLUUID& object_id)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
    if (!mPublishedObjectManager.hasPendingPublish(object_id))
    {
        LL_WARNS("ScriptEditorWS") << "buildAndSendPublish: no pending publish for " << object_id << LL_ENDL;
        return;
    }

    LLViewerObject* root = gObjectList.findObject(object_id);
    if (!root)
    {
        LL_WARNS("ScriptEditorWS") << "buildAndSendPublish: root object gone: " << object_id << LL_ENDL;
        mPublishedObjectManager.cancelPendingPublish(object_id);
        return;
    }

    LLSD pub = mPublishedObjectManager.buildPublishedObjectLLSD(root);

    // Store in the published registry
    LLPublishedObjectMgr::PublishedObjectInfo info;
    info.mObjectID          = root->getID();
    info.mOwnerID           = root->mOwnerID;
    info.mObjectName        = pub["object_name"].asString();
    info.mObjectDescription = pub["object_description"].asString();
    if (root->getRegion())
    {
        info.mRegionName = root->getRegion()->getName();
    }
    LLSelectNode* root_select_node = LLSelectMgr::instance().getSelection()->findNode(root);
    if (root_select_node
        && root_select_node->mValid
        && !root_select_node->mFromTaskID.isNull()
        && !root->isAttachment())
    {
        info.mCanSaveBackToContents = true;
        info.mSourceTaskID = root_select_node->mFromTaskID;
    }
    else
    {
        info.mCanSaveBackToContents = false;
        info.mSourceTaskID.setNull();
    }

    S32 link_num = 1;
    std::vector<LLViewerObject*> prims = collect_linkset(root);
    for (LLViewerObject* prim : prims)
    {
        LLPublishedObjectMgr::PublishedPrimInfo prim_info;
        prim_info.mPrimID          = prim->getID();
        prim_info.mPrimName        = getPrimName(prim);  // Use helper with selection fallback
        prim_info.mLinkNumber      = link_num++;
        prim_info.mInventorySerial = static_cast<S16>(prim->getInventorySerial());
        info.mPrims.push_back(prim_info);
    }

    LLPublishedObjectMgr::PublishedObjectInfo& published_info = mPublishedObjectManager.finalizePendingPublish(object_id, std::move(info));

    // Align outgoing publish payload with any property responses that arrived
    // while inventory-gated publish was still pending.
    pub["object_name"] = published_info.mObjectName;
    pub["can_save_back"] = published_info.mCanSaveBackToContents;
    pub["object_description"] = published_info.mObjectDescription;
    if (pub.has("linked_objects"))
    {
        LLSD& linked_objects = pub["linked_objects"];
        for (S32 i = 0; i < linked_objects.size(); ++i)
        {
            const LLUUID link_id = linked_objects[i]["link_id"].asUUID();
            auto prim_it = std::find_if(
                published_info.mPrims.begin(),
                published_info.mPrims.end(),
                [&](const LLPublishedObjectMgr::PublishedPrimInfo& p)
                {
                    return p.mPrimID == link_id;
                });
            if (prim_it != published_info.mPrims.end())
            {
                linked_objects[i]["link_name"] = prim_it->mPrimName;
                linked_objects[i]["link_description"] = prim_it->mPrimDescription;
            }
        }
    }

    // Send notification
    LLSD message;
    message["object"] = pub;
    notifyAll("object.publish", message);

    LL_INFOS("ScriptEditorWS") << "Published object " << object_id
        << " (" << pub["object_name"].asString() << ") with "
        << (prims.size() - 1) << " linked prim(s)" << LL_ENDL;

    // Re-request object properties now that the object is published so
    // onObjectPropertyChanged can emit object.update for root and linked prims.
    for (LLViewerObject* prim : prims)
    {
        LLSelectMgr::instance().requestObjectPropertiesFamily(prim);
    }
}

void LLScriptEditorWSServer::onLinksetChildAdded(const LLUUID& root_id, LLViewerObject* child)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
    if (!child)
    {
        return;
    }

    if (!mPublishedObjectManager.reconcileLinksetChildAdded(
            root_id,
            child,
            LLTimer::getTotalSeconds().value()))
    {
        return;
    }

    // Request inventory (async; fires onPrimInventoryChanged when ready).
    child->requestInventory();

    // Start safety-timeout timer (no-op if one is already pending for this root)
    scheduleLinksetFlush(root_id, LINKSET_ADD_FLUSH_DELAY);
}

void LLScriptEditorWSServer::onLinksetChildRemoved(const LLUUID& root_id, const LLUUID& child_id)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
    if (!mPublishedObjectManager.reconcileLinksetChildRemoved(root_id, child_id))
    {
        return;
    }

    // Schedule coalesced flush - multiple simultaneous removes share one timer
    scheduleLinksetFlush(root_id, LINKSET_REMOVE_FLUSH_DELAY);
}

void LLScriptEditorWSServer::scheduleLinksetFlush(const LLUUID& root_id, F32 delay)
{
    // No-op if a timer is already pending for this root_id
    if (mPublishedObjectManager.hasActiveLinksetFlushTimer(root_id))
    {
        return;
    }

    wptr_t weak = std::static_pointer_cast<LLScriptEditorWSServer>(shared_from_this());
    LLEventTimer* t = LLEventTimer::run_after(delay, [weak, root_id]()
    {
        if (auto self = weak.lock())
        {
            self->mPublishedObjectManager.clearLinksetFlushTimer(root_id);
            self->mPublishedObjectManager.clearPendingNewChildren(root_id); // clear any remaining pending children (timeout path)
            self->flushLinksetUpdate(root_id);
        }
    });
    mPublishedObjectManager.setLinksetFlushTimer(root_id, t->getWeak());
}

void LLScriptEditorWSServer::cancelLinksetFlushTimer(const LLUUID& root_id)
{
    mPublishedObjectManager.cancelLinksetFlushTimer(root_id);
}

void LLScriptEditorWSServer::flushLinksetUpdate(const LLUUID& root_id)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
    LLSD update;
    if (!mPublishedObjectManager.buildLinksetUpdateLLSD(root_id, update))
    {
        return;
    }
    notifyAll("object.update", update);

    const LLSD linked_objects = update["linked_objects"];
    LL_INFOS("ScriptEditorWS") << "Linkset update for " << root_id
        << ": " << linked_objects.size() << " child(ren)" << LL_ENDL;
}

void LLScriptEditorWSServer::onPrimInventoryChanged(const LLUUID& object_id, const LLUUID& prim_id)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
    if (!mPublishedObjectManager.hasPublished(object_id))
    {
        return;
    }

    LLViewerObject* prim = gObjectList.findObject(prim_id);
    if (!prim)
    {
        return;
    }

    auto inv_result = mPublishedObjectManager.handlePrimInventoryChangedEvent(
        object_id, prim_id, prim, LLTimer::getTotalSeconds().value());

    if (inv_result.mTimingConsumed)
    {
        LL_DEBUGS("ScriptEditorWS") << "[Phase0] inventory refresh object_id=" << object_id
            << " prim_id=" << prim_id
            << " elapsed_sec=" << inv_result.mTimingElapsedSec << LL_ENDL;
    }

    if (inv_result.mKind == LLPublishedObjectMgr::InventoryChangeKind::CHILD_READY_WAIT)
    {
        return;
    }
    if (inv_result.mKind == LLPublishedObjectMgr::InventoryChangeKind::CHILD_READY_FLUSH_NOW)
    {
        cancelLinksetFlushTimer(object_id);
        flushLinksetUpdate(object_id);
        return;
    }
    if (inv_result.mKind == LLPublishedObjectMgr::InventoryChangeKind::ROOT_INVENTORY_UPDATE ||
        inv_result.mKind == LLPublishedObjectMgr::InventoryChangeKind::CHILD_INVENTORY_UPDATE)
    {
        notifyAll("object.update", inv_result.mUpdate);
        if (inv_result.mHasPendingItemCreate)
        {
            LLEventPumps::instance().post(
                inv_result.mPendingItemCreatePump,
                LLSD().with("prim_id", prim_id));
        }

        LL_DEBUGS("ScriptEditorWS") << "Sent object.update for prim " << prim_id
                                    << " in object " << object_id << LL_ENDL;
    }
}

void LLScriptEditorWSServer::onObjectPropertyChanged(
    const LLUUID& prim_id, const std::string& name, const std::string& desc, S16 inventory_serial)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
    LLViewerObject* prim = gObjectList.findObject(prim_id);
    if (!prim)
    {
        return;
    }

    LLUUID root_id = prim->getRootEdit()->getID();

    mPublishedObjectManager.recordPendingPropertyChange(root_id, prim_id, name, desc);

    bool should_refresh_inventory = mPublishedObjectManager.markPrimInventorySerialAndDetectChange(
        root_id,
        prim_id,
        inventory_serial);

    LLSD update;
    if (mPublishedObjectManager.applyPropertyChange(root_id, prim_id, name, desc, update))
    {
        notifyAll("object.update", update);
    }

    if (should_refresh_inventory && !mPublishedObjectManager.hasInventoryRequestStart(prim_id))
    {
        prim->dirtyInventory();
        mPublishedObjectManager.setInventoryRequestStart(prim_id, LLTimer::getTotalSeconds().value());
        prim->requestInventory();
    }
}

void LLScriptEditorWSServer::unpublishObject(const LLUUID& object_id, const std::string& reason)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
    if (!mPublishedObjectManager.cleanupObjectStateForUnpublish(object_id))
    {
        return;
    }

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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
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
    features["commands"]         = true;
    features["unified_diagnostics"] = true;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_SCRIPTDEV;
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

