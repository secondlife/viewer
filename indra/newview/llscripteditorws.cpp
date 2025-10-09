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
#include "llregex.h"
#include "llviewerobject.h"
#include "llviewerobjectlist.h"
#include "llchat.h"

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
                server->broadcastLanguageChange();
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
        // TODO: When connections reach 0, stop the server after a timeout.
    }
}

bool LLScriptEditorWSServer::subscribeScriptEditor(const LLUUID& object_id, const LLUUID& item_id, std::string_view script_name,
    const LLHandle<LLPanel>& editor_handle, const std::string& script_id)
{
    if (!editor_handle.isDead())
    {
        auto it = mSubscriptions.find(script_id);
        if (it == mSubscriptions.end())
        {   // Don't re-add if already subscribed
            mSubscriptions.emplace(script_id,
                LLScriptEditorWSServer::EditorSubscription(object_id, item_id, script_name, editor_handle));
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
        S32 connection_id = it->second.mConnectionID;
        auto connection = it->second.mConnection.lock();
        mSubscriptions.erase(it);
        ptrdiff_t count = std::count_if(mSubscriptions.begin(), mSubscriptions.end(), [connection_id](const auto& pair) {
            return pair.second.mConnectionID == connection_id;
        });
        if (connection && !count)
        { // We have removed the last subscription, close the connection
            LL_DEBUGS("ScriptEditorWS") << "Closing connection ID " << connection_id <<
                " as last subscription was removed" << LL_ENDL;
            connection->sendDisconnect(LLScriptEditorWSConnection::REASON_EDITOR_CLOSED, "Editor closed");
        }

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

void LLScriptEditorWSServer::broadcastLanguageChange()
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

    if (category.empty())
    {
        response["error"] = "No syntax category specified";
        response["success"] = false;
        return response;
    }

    response["id"] = mLastSyntaxId;
    if (category == "defs.lua")
    {
        response["defs"] = LLSyntaxLua::instance().getTypesXML();
        response["success"] = response["defs"].isDefined();
    }
    else if (category == "defs.lsl")
    {
        response["defs"] = LLSyntaxIdLSL::instance().getKeywordsXML();
        response["success"] = response["defs"].isDefined();
    }
    else
    {
        response["error"] = "Unknown syntax category requested";
        response["success"] = false;
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
    if (!lines.empty() && std::equal(runtime_error_marker.rbegin(), runtime_error_marker.rend(), lines.front().rbegin()))
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

        // TODO: Build an actual error message to forward to the external editor
        // Explaination:
        // Well! Heck!
        // As it turns out, the complete error message arrives as either two or three
        // separate chat messages from the server.
        // 2 if the script is LSL or if it is Lua but not owned by the editing agent
        // 3 if the script is Lua and owned by the editing agent.
        //
        // Message 1: <Object Name> [script:<Script Name>] Script run-time error
        // Message 2: <runtime error>
        // Message 3: <script>:<line>: <actual error message>\n
        //              <call stack>
        //
        // These need to be compositited into a single error message to send to the IDE.
        //
        //if (lines.size() > 1)
        //{   // The second line is the actual error message
        //    error_message = lines[1];
        //    remove_count++;
        //    if ((error_message == "runtime error") && (lines.size() > 2))
        //    { // If the error message is just "runtime error", the next line might actually be the real message:
        //        // "lua_script:7: attempt to perform arithmetic (sub) on nil"
        //        static const boost::regex LUA_ERROR_REGEX(R"(^(.+?):(\d+):\s*(.+)$)");
        //
        //        if (boost::regex_match(first_line, m, RUNTIME_ERR_REGEX_FLEX))
        //        {
        //            line_number   = std::stoi(m[2].str());
        //            error_message = m[3].str();
        //            remove_count++;
        //        }
        //    }
        //    else
        //    {
        //        error_message = "Unknown script runtime error";
        //    }
        //}
        //else
        //{
        //    error_message = "Unknown script runtime error";
        //}
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
    handshake["syntax_id"] = LLSyntaxIdLSL::instance().getSyntaxID();

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

void LLScriptEditorWSConnection::sendDisconnect(S32 reason, const std::string& message)
{
    LL_INFOS("ScriptEditorWS") << "Sending disconnect to client: " << message << LL_ENDL;
    LLSD params;
    params["reason"]  = reason;
    params["message"] = message;
    notify("session.disconnect", params);
    closeConnection(1000, message);
}

void LLScriptEditorWSConnection::handleHandshakeResponse(const LLSD& result)
{
    LL_INFOS("ScriptEditorWS") << "Processing handshake response from client" << LL_ENDL;

    // Extract and validate client information
    mClientName = result["client_name"].asString();
    mClientVersion = result["client_version"].asString();
    mProtocolVersion = result["protocol_version"].asString();

    if (mChallenge.notNull())
    {
        // Validate challenge response
        bool valid_response = (result.has("challenge_response") &&
            (result["challenge_response"].asUUID() == mChallenge));

        LLFile::remove(mChallengeFile);
        mChallengeFile.clear();
        mChallenge.setNull();
        if (!valid_response)
        {
            LL_WARNS("ScriptEditorWS") << "Invalid or missing challenge response from client" << LL_ENDL;
            sendDisconnect(REASON_PROTOCOL_ERROR, "Invalid challenge response");
            return;
        }
    }
    LLUUID challenge_response = result["challenge_response"].asUUID();

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

    if (mChallenge.notNull())
    {
        // Remove temporary challenge file
        LLFile::remove(mChallengeFile);
        mChallenge.setNull();
        mChallengeFile.clear();
    }

    notify("session.ok");

    LL_INFOS("ScriptEditorWS") << "Handshake completed successfully." << LL_ENDL;
}

std::string LLScriptEditorWSConnection::generateChallenge()
{
    mChallenge.generate();

    mChallengeFile = std::string(LLFile::tmpdir()) + "sl_script_challenge.tmp";

    llofstream file(mChallengeFile.c_str());
    if (!file.is_open())
    {
        LL_WARNS() << "Unable to open challenge file: " << mChallengeFile << LL_ENDL;
        mChallenge.setNull();
        mChallengeFile.clear();
        return std::string();
    }

    file << mChallenge;
    file.close();

    return mChallengeFile;
}
