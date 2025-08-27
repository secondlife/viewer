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

/**
 * This implementation provides JSON-RPC 2.0 WebSocket communication between
 * the Second Life viewer and external script editors. It uses the standard
 * JSON-RPC 2.0 protocol without pre-defined script-specific methods,
 * allowing for flexible integration approaches.
 *
 * ## JSON-RPC Integration
 *
 * The connection provides a clean JSON-RPC 2.0 interface that can be
 * extended with script-specific functionality as needed:
 *
 * ### Server-to-Client (Viewer to Editor):
 * - `session.handshake`: Welcome message on connection
 * - `session.disconnect`: Notify editor of disconnection
 *
 * ### Notifications (no response expected):
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
    mActiveConnections.insert(connection);

    // Call setupConnectionMethods to register any global methods
    setupConnectionMethods(connection);

    return connection;
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
        mActiveConnections.erase(script_connection);

        LL_INFOS("ScriptEditorWS") << "Removed connection from active connections. Total: "
                                   << mActiveConnections.size() << LL_ENDL;
        // TODO: When connections reach 0, stop the server aftera a timeout.
    }
}

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

std::shared_ptr<LLScriptEditorWSConnection> LLScriptEditorWSServer::findConnectionForScript(const std::string& script_id)
{
    // TODO: Implement logic to find connection handling a specific script
    // This would require tracking which connection is responsible for which script
    return nullptr;
}

std::set<std::string> LLScriptEditorWSServer::getActiveScripts() const
{
    std::set<std::string> active_scripts;
    for (const auto& [script_id, editor_handle] : mScriptEditors)
    {
        if (!editor_handle.isDead())
        {
            active_scripts.insert(script_id);
        }
    }
    return active_scripts;
}

void LLScriptEditorWSServer::broadcastScriptUpdate(const std::string& script_id, const std::string& content, const LLSD& metadata)
{
    LL_DEBUGS("ScriptEditorWS") << "Broadcasting script update for script: " << script_id << LL_ENDL;

    LLSD params;
    params["script_id"] = script_id;
    params["content"] = content;
    params["timestamp"] = LLDate::now().asString();

    if (!metadata.isUndefined())
    {
        params["metadata"] = metadata;
    }

    // Send to all connected editors as a notification
    broadcastNotification("script.update", params);
}

void LLScriptEditorWSServer::broadcastCompilationResult(const std::string& script_id, bool success, const LLSD& errors)
{
    LL_DEBUGS("ScriptEditorWS") << "Broadcasting compilation result for script: " << script_id
                                << " (success: " << success << ")" << LL_ENDL;

    LLSD params;
    params["script_id"] = script_id;
    params["success"] = success;
    params["timestamp"] = LLDate::now().asString();

    if (!errors.isUndefined() && errors.isArray())
    {
        params["errors"] = errors;
    }

    // Send to all connected editors as a notification
    broadcastNotification("compilation.result", params);
}

void LLScriptEditorWSServer::setupConnectionMethods(LLJSONRPCConnection::ptr_t connection)
{
    // Call parent class to register global JSON-RPC methods
    LLJSONRPCServer::setupConnectionMethods(connection);

    // Cast to our specific connection type to access script editor functionality
    auto script_connection = std::dynamic_pointer_cast<LLScriptEditorWSConnection>(connection);
    if (script_connection)
    {
        LL_INFOS("ScriptEditorWS") << "Setting up script editor connection methods" << LL_ENDL;

        // Here derived classes could add script-specific method registrations
        // For now, the base LLScriptEditorWSConnection doesn't register any specific methods
        // but this provides a hook for future customization

        // Example of how custom methods could be registered:
        // script_connection->registerMethod("script.custom", handler);
    }
}

//========================================================================
LLScriptEdContainer* LLScriptEditorWSConnection::getEditor() const
{
    return mEditorPanel.isDead() ? nullptr : dynamic_cast<LLScriptEdContainer*>(mEditorPanel.get());
}

std::shared_ptr<LLScriptEditorWSServer> LLScriptEditorWSConnection::getServer() const
{
    return std::static_pointer_cast<LLScriptEditorWSServer>(mOwningServer.lock());
}

void LLScriptEditorWSConnection::onOpen()
{
    // Call parent class to set up JSON-RPC infrastructure
    LLJSONRPCConnection::onOpen();

    LL_INFOS("ScriptEditorWS") << "Script editor JSON-RPC connection opened" << LL_ENDL;

    // Generate unique editor session ID
    mEditorId = LLUUID::generateNewID().asString();
    mEditorReady = false;

    LL_INFOS("ScriptEditorWS") << "Initialized editor session: " << mEditorId << LL_ENDL;

    // Build hello data according to the protocol specification
    LLSD handshake;
    handshake["server_version"]   = "1.0.0";
    handshake["protocol_version"] = "1.0";
    handshake["viewer_name"]      = LLVersionInfo::instance().getChannel();
    handshake["viewer_version"]   = LLVersionInfo::instance().getVersion();

    // Supported languages array
    LLSD languages = LLSD::emptyArray();
    languages.append("lsl");
    languages.append("luau");
    handshake["supported_languages"] = languages;

    // Features object
    LLSD features;
    features["live_sync"]        = true;
    features["compilation"]      = true;
    features["syntax_highlight"] = true;
    handshake["features"]        = features;

    // Send editor.handshake method call to the client and handle response
    call("session.handshake", handshake, [this](const LLSD& result, const LLSD& error) {
        if (error.isUndefined())
        {
            handleHandshakeResponse(result);
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

    LL_INFOS("ScriptEditorWS") << "Script editor JSON-RPC connection closed for session: "
                               << mEditorId << LL_ENDL;

    cleanupConnection();

    // Clean up editor-specific state
    mEditorId.clear();
    mEditorCapabilities.clear();
    mScriptId.clear();
    mEditorReady = false;

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

    // Validate protocol compatibility
    if (mProtocolVersion != "1.0")
    {
        LL_WARNS("ScriptEditorWS") << "Protocol version mismatch. Expected: 1.0, Got: "
                                    << mProtocolVersion << LL_ENDL;
    }

    // Store script information if provided
    mScriptName = result["script_name"].asString();
    mScriptLanguage = result["script_language"].asString();
    mScriptId = result["script_id"].asString();

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

    connectToEditor(mScriptId);
    // Mark editor as ready
    mEditorReady = true;

    LL_INFOS("ScriptEditorWS") << "Handshake completed successfully for session: " << mEditorId << LL_ENDL;
}

bool LLScriptEditorWSConnection::connectToEditor(const std::string& script_id)
{
    LLScriptEditorWSServer::ptr_t server = std::dynamic_pointer_cast<LLScriptEditorWSServer>(mOwningServer.lock());
    if (!server)
    {
        LL_WARNS("ScriptEditorWS") << "Cannot connect to editor - server reference lost" << LL_ENDL;
        return false;
    }

    mEditorPanel = server->findEditorForScript(script_id);

    LLScriptEdContainer* editor_core = getEditor();
    if (!editor_core)
    {
        LL_INFOS("ScriptEditorWS") << "Could not find editor: " << script_id << LL_ENDL;
        // TODO: Disconnect the client if no editor found
        return false;
    }

    return true;
}

void LLScriptEditorWSConnection::cleanupConnection()
{
    LL_INFOS("ScriptEditorWS") << "Cleaning up connection for editor session: " << mEditorId << LL_ENDL;

    LLScriptEditorWSServer::ptr_t server = getServer();
    if (server)
    {
        server->dissociateEditor(mScriptId);
    }

    LLScriptEdContainer* editor_core = getEditor();

    if (editor_core)
    {
        editor_core->cleanupWebSocket();

        // Notify the editor panel of disconnection
        //editor_core->onExternalEditorDisconnected();
    }

    mEditorPanel = LLHandle<LLPanel>();
}


void LLScriptEditorWSConnection::sendDisconnect(S32 reason, const std::string& message)
{
    LL_INFOS("ScriptEditorWS") << "Sending disconnect message to editor (reason: "
                               << reason << ", message: " << message << ")" << LL_ENDL;

    LLSD params;
    params["reason"] = reason;
    params["message"] = message;

    notify("session.disconnect", params);
}
