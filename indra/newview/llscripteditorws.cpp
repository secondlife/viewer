/**
 * @file llscripteditorws.cpp
 * @brief JSON-RPC 2.0 WebSocket server implementation for external script editor integration
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
#include "llpreviewscript.h"
#include "llappviewer.h"
#include "lltrans.h"
#include "lldate.h"
#include "llerror.h"
#include "lluuid.h"
#include "llversioninfo.h"
#include "llagent.h"

//========================================================================
LLScriptEditorWSServer::LLScriptEditorWSServer(const std::string& name, U16 port, bool local_only)
    : LLJSONRPCServer(name, port, local_only)
{
    LL_INFOS("ScriptEditorWS") << "Created JSON-RPC script editor server: " << name
                               << " on port " << port << LL_ENDL;
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
    LLSyntaxIdLSL& syntax_id_mgr = LLSyntaxIdLSL::instance();
    wptr_t that(std::static_pointer_cast<LLScriptEditorWSServer>(shared_from_this()));

    mLastSyntaxId = syntax_id_mgr.getSyntaxID();
    mLanguageChangeSignal = syntax_id_mgr.addSyntaxIDCallback(
        [that]()
        {
            auto server = that.lock();
            if (server && server->isRunning())
            {
                server->broadcastLangugeChange();
            }
        });
}

void LLScriptEditorWSServer::onStopped()
{
    mLanguageChangeSignal.disconnect();
    mLastSyntaxId.setNull();
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
        // TODO: When connections reach 0, stop the server aftera a timeout.
    }
}

bool LLScriptEditorWSServer::subscribeScriptEditor(const LLHandle<LLPanel>& editor_handle, const std::string &script_id)
{
    if (!editor_handle.isDead())
    {
        auto it = mSubscriptions.find(script_id);
        if (it == mSubscriptions.end())
        {   // Don't readd if already subscribed
            mSubscriptions.emplace(script_id, EditorSubscription{ editor_handle, LLScriptEditorWSConnection::wptr_t() });
            return false;
        }
        else
        { // Update existing subscription with new editor handle
            it->second.mEditorHandle = editor_handle;
        }
        return true;
    }
    return false;
}

void LLScriptEditorWSServer::unsubscribeEditor(const std::string &script_id)
{
    auto it = mSubscriptions.find(script_id);
    if (it != mSubscriptions.end())
    {
        mSubscriptions.erase(it);
    }
}

void LLScriptEditorWSServer::unsubscribeConnection(U32 connection_id)
{
    for (auto it = mSubscriptions.begin(); it != mSubscriptions.end(); )
    {
        if (it->second.mConnectionID == connection_id)
        {
            LL_DEBUGS("ScriptEditorWS") << "Unsubscribing script " << it->first
                                       << " from connection ID " << connection_id << LL_ENDL;
            it = mSubscriptions.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

LLScriptEditorWSServer::SubscriptionError_t LLScriptEditorWSServer::updateScriptSubscription(const std::string &script_id, U32 connection_id)
{
    auto it = mSubscriptions.find(script_id);
    if (it != mSubscriptions.end())
    {
        if (it->second.mEditorHandle.isDead())
        {
            unsubscribeEditor(script_id);
            return SUBSCRIPTION_INVALID_EDITOR;
        }

        auto con_it = mActiveConnections.find(connection_id);
        if (con_it == mActiveConnections.end())
        {
            return SUBSCRIPTION_INTERNAL_ERROR;
        }

        if ((it->second.mConnectionID != 0) && !it->second.mConnection.expired()
            && it->second.mConnection.lock()->isConnected())
        {
            LL_WARNS("ScriptEditorWS") << "Script " << script_id << " is already subscribed on connection ID " << it->second.mConnectionID
                                       << ", cannot subscribe again on connection ID " << connection_id << LL_ENDL;
            // In the future we may want to support multiple connections per script.
            // That would imply it was open in multiple editors.
            return SUBSCRIPTION_ALREADY_SUBSCRIBED;
        }

        it->second.mConnectionID = connection_id;
        it->second.mConnection   = con_it->second;
        return SUBSCRIPTION_SUCCESS;
    }
    return SUBSCRIPTION_INVALID_SUBSCRIPTION;
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

std::shared_ptr<LLScriptEditorWSConnection> LLScriptEditorWSServer::findConnectionForScript(const std::string& script_id)
{
    // TODO: Implement logic to find connection handling a specific script
    // This would require tracking which connection is responsible for which script
    return nullptr;
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
        wptr_t that(std::static_pointer_cast<LLScriptEditorWSServer>(shared_from_this()));

        U32 connection_id = script_connection->getConnectionID();

        script_connection->registerMethod("language.syntax.id",
            [that](const std::string&, const LLSD&, const LLSD&) -> LLSD
            {
                auto server = that.lock();
                if (server)
                {
                    return server->handleLanguageIdRequest();
                }
                return LLSD();
            });
        script_connection->registerMethod("language.syntax",
            [that](const std::string&, const LLSD&, const LLSD& params)
            {
                auto server = that.lock();
                if (server)
                {
                    return server->handleSyntaxRequest(params);
                }
                return LLSD();
            });
        script_connection->registerMethod("script.subscribe",
            [that, connection_id](const std::string&, const LLSD&, const LLSD& params) -> LLSD
            {
                auto server = that.lock();
                if (server)
                {
                    return server->handleScriptSubscribe(connection_id, params);
                }
                return LLSD();
            });
        script_connection->registerMethod("script.unsubscribe", [](const std::string&, const LLSD&, const LLSD& params) -> LLSD
            {   // this is a notification, no response expected
                return LLSD();
            });
        // script_connection->registerMethod("language.syntax", )
    }
}

void LLScriptEditorWSServer::broadcastLangugeChange()
{
    LLUUID syntax_id = LLSyntaxIdLSL::instance().getSyntaxID();

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

    response["id"] = mLastSyntaxId;

    if (category == "types.luau")
    {
        response["types"] = LLSyntaxLua::instance().getTypesXML();
    }
    else
    {
        LLSD syntax = LLSyntaxIdLSL::instance().getKeywordsXML();

        // TODO: support language definitions and additional modules.

        if (syntax.has(category))
        {
            response[category] = syntax[category];
        }
    }
    return response;
}

LLSD LLScriptEditorWSServer::handleScriptSubscribe(U32 connection_id, const LLSD& params)
{
    LLSD response(LLSD::emptyMap());

    std::string script_id = params["script_id"].asString();
    std::string script_name = params["script_name"].asString();
    std::string language    = params["script_language"].asString();

    SubscriptionError_t result = updateScriptSubscription(script_id, connection_id);

    response["script_id"] = script_id;
    response["success"]   = (result == SUBSCRIPTION_SUCCESS);
    response["status"]    = result;

    LL_WARNS_IF(result != SUBSCRIPTION_SUCCESS, "ScriptEditorWS")
        << "Script connect request for script " << script_id << " failed with status " << result << LL_ENDL;
    switch (result)
    {
    case SUBSCRIPTION_SUCCESS:
        response["message"] = "OK";
        break;
    case SUBSCRIPTION_INVALID_EDITOR:
        response["message"] = "Invalid editor handle";
        break;
    case SUBSCRIPTION_INVALID_SUBSCRIPTION:
        response["message"] = "No subscription found for script";
        break;
    case SUBSCRIPTION_ALREADY_SUBSCRIBED:
        response["message"] = "Script already subscribed";
        break;
    case SUBSCRIPTION_INTERNAL_ERROR:
        response["message"] = "Internal server error";
        break;
    }

    if (result == SUBSCRIPTION_SUCCESS)
    {
        //TODO: Build an info block for the subscribed script.
        //buildScriptSubscriptionInfo(result);
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

void LLScriptEditorWSServer::sendUnsubscribeScriptEditor(const std::string& script_id)
{
    auto it = mSubscriptions.find(script_id);
    if (it != mSubscriptions.end())
    {
        auto connection = it->second.mConnection.lock();
        if (connection)
        {
            LLSD params;
            params["script_id"] = script_id;
            connection->notify("script.unsubscribe", params);
        }
    }
}

//========================================================================
U32 LLScriptEditorWSConnection::sNextConnectionID = 1;

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

    // handshake["challenge"] = ... TODO: simple challenge, write to a file and have the client echo it back?

    LLSD languages = LLSD::emptyArray();
    languages.append("lsl");
    languages.append("luau");
    handshake["supported_languages"] = languages;

    // Features object
    LLSD features;
    features["live_sync"]        = true;
    features["compilation"]      = true;
    handshake["features"]        = features;

    wptr_t that = shared_from_this();

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

void LLScriptEditorWSConnection::handleHandshakeResponse(const LLSD& result)
{
    LL_INFOS("ScriptEditorWS") << "Processing handshake response from client" << LL_ENDL;

    // Extract and validate client information
    mClientName = result["client_name"].asString();
    mClientVersion = result["client_version"].asString();
    mProtocolVersion = result["protocol_version"].asString();

    // TODO: Validate challenge_response if implemented

    // Validate protocol compatibility
    if (mProtocolVersion != "1.0")
    {
        LL_WARNS("ScriptEditorWS") << "Protocol version mismatch. Expected: 1.0, Got: "
                                    << mProtocolVersion << LL_ENDL;
    }

    // Store script information if provided
    mScriptName = result["script_name"].asString();
    mScriptLanguage = result["script_language"].asString();

    // Store supported languages
    for (const auto& lang : llsd::inArray( result["languages"]))
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
