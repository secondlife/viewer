/**
 * @file lljsonrpcws.cpp
 * @brief JSON-RPC 2.0 WebSocket server and connection implementation
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

#include "linden_common.h"

#include "lljsonrpcws.h"
#include "llerror.h"
#include "llsdjson.h"
#include "lldate.h"

#include <boost/json.hpp>

//========================================================================
// LLJSONRPCConnection Implementation
//========================================================================

void LLJSONRPCConnection::onOpen()
{
    LL_INFOS("JSONRPC") << "JSON-RPC connection opened" << LL_ENDL;
}

void LLJSONRPCConnection::onClose()
{
    LL_INFOS("JSONRPC") << "JSON-RPC connection closed, clearing "
                        << mPendingRequests.size() << " pending requests" << LL_ENDL;

    // Cancel all pending requests
    for (auto& [id, callback] : mPendingRequests)
    {
        if (callback)
        {
            LLSD error;
            error["code"] = RPCError::CONNECTION_CLOSED; // Use named constant instead of magic number
            error["message"] = "Connection closed";
            callback(LLSD(), error);
        }
    }
    mPendingRequests.clear();
}

void LLJSONRPCConnection::onMessage(const std::string& message)
{
    LL_DEBUGS("JSONRPC") << "Received JSON-RPC message: " << message << LL_ENDL;

    try
    {
        // Parse JSON message
        boost::system::error_code ec;
        boost::json::value json_value = boost::json::parse(message, ec);

        if (ec.failed())
        {
            LL_WARNS("JSONRPC") << "Failed to parse JSON: " << ec.message() << LL_ENDL;
            sendError(LLSD(), ParseError(ec.message()));
            return;
        }

        // Convert to LLSD
        LLSD message_obj = LlsdFromJson(json_value);

        // Handle batch vs single message
        if (message_obj.isArray())
        {
            // Batch request
            if (message_obj.size() == 0)
            {
                sendError(LLSD(), InvalidRequest("Empty batch"));
                return;
            }

            // Process each message in the batch
            for (S32 i = 0; i < message_obj.size(); ++i)
            {
                processMessage(message_obj[i]);
            }
        }
        else
        {
            // Single message
            processMessage(message_obj);
        }
    }
    catch (const std::exception& e)
    {
        LL_WARNS("JSONRPC") << "Exception processing JSON-RPC message: " << e.what() << LL_ENDL;
        sendError(LLSD(), InternalError(e.what()));
    }
}

void LLJSONRPCConnection::processMessage(const LLSD& message_obj)
{
    try
    {
        // Determine if this is a request, notification, or response
        if (message_obj.has("method"))
        {
            // This is a request or notification
            if (validateMessage(message_obj, true))
            {
                processRequest(message_obj);
            }
        }
        else if (message_obj.has("result") || message_obj.has("error"))
        {
            // This is a response
            if (validateMessage(message_obj, false))
            {
                processResponse(message_obj);
            }
        }
        else
        {
            LL_WARNS("JSONRPC") << "Message must contain 'method' or 'result'/'error'" << LL_ENDL;
        }
    }
    catch (const RPCError& e)
    {
        LLSD id = message_obj.has("id") ? message_obj["id"] : LLSD();
        sendError(id, e);
    }
}

void LLJSONRPCConnection::processRequest(const LLSD& request)
{
    std::string method = request["method"].asString();
    LLSD params = request.has("params") ? request["params"] : LLSD();
    LLSD id = request.has("id") ? request["id"] : LLSD();
    bool is_notification = !request.has("id");

    LL_DEBUGS("JSONRPC") << "Processing " << (is_notification ? "notification" : "request")
                         << " for method: " << method << LL_ENDL;

    // Find method handler
    auto it = mMethodHandlers.find(method);
    if (it == mMethodHandlers.end())
    {
        if (!is_notification)
        {
            sendError(id, MethodNotFound(method));
        }
        return;
    }

    try
    {
        // Call the method handler with method name, ID, and parameters
        LLSD result = it->second(method, id, params);

        if (!is_notification)
        {
            sendResponse(id, result);
        }
    }
    catch (const RPCError& e)
    {
        if (!is_notification)
        {
            sendError(id, e);
        }
        else
        {
            LL_WARNS("JSONRPC") << "Error in notification handler for " << method
                               << ": " << e.what() << LL_ENDL;
        }
    }
    catch (const std::exception& e)
    {
        if (!is_notification)
        {
            sendError(id, InternalError(e.what()));
        }
        else
        {
            LL_WARNS("JSONRPC") << "Exception in notification handler for " << method
                               << ": " << e.what() << LL_ENDL;
        }
    }
}

void LLJSONRPCConnection::processResponse(const LLSD& response)
{
    if (!response.has("id"))
    {
        LL_WARNS("JSONRPC") << "Response missing id field" << LL_ENDL;
        return;
    }

    std::string id = response["id"].asString();
    auto it = mPendingRequests.find(id);
    if (it == mPendingRequests.end())
    {
        LL_WARNS("JSONRPC") << "Received response for unknown request id: " << id << LL_ENDL;
        return;
    }

    ResponseCallback callback = it->second;
    mPendingRequests.erase(it);

    if (callback)
    {
        LLSD result = response.has("result") ? response["result"] : LLSD();
        LLSD error = response.has("error") ? response["error"] : LLSD();

        callback(result, error);
    }
}

bool LLJSONRPCConnection::validateMessage(const LLSD& message, bool is_request)
{
    // Check JSON-RPC version
    if (!message.has("jsonrpc") || message["jsonrpc"].asString() != "2.0")
    {
        LL_WARNS("JSONRPC") << "Missing or invalid jsonrpc version" << LL_ENDL;
        return false;
    }

    if (is_request)
    {
        // Request/notification validation
        if (!message.has("method"))
        {
            LL_WARNS("JSONRPC") << "Missing method field" << LL_ENDL;
            return false;
        }

        if (!message["method"].isString())
        {
            LL_WARNS("JSONRPC") << "Method must be a string" << LL_ENDL;
            return false;
        }

        // Params are optional but must be array or object if present
        if (message.has("params"))
        {
            if (!message["params"].isArray() && !message["params"].isMap())
            {
                LL_WARNS("JSONRPC") << "Params must be array or object" << LL_ENDL;
                return false;
            }
        }
    }
    else
    {
        // Response validation
        if (!message.has("id"))
        {
            LL_WARNS("JSONRPC") << "Response missing id field" << LL_ENDL;
            return false;
        }

        // Must have either result or error, but not both
        bool has_result = message.has("result");
        bool has_error = message.has("error");

        if (!has_result && !has_error)
        {
            LL_WARNS("JSONRPC") << "Response must have result or error" << LL_ENDL;
            return false;
        }

        if (has_result && has_error)
        {
            LL_WARNS("JSONRPC") << "Response cannot have both result and error" << LL_ENDL;
            return false;
        }

        // Error must be an object with code and message
        if (has_error)
        {
            LLSD error = message["error"];
            if (!error.isMap())
            {
                LL_WARNS("JSONRPC") << "Error must be an object" << LL_ENDL;
            }
            if (!error.has("code") || !error.has("message"))
            {
                LL_WARNS("JSONRPC") << "Error must have code and message" << LL_ENDL;
            }
        }
    }
    return true;
}

LLSD LLJSONRPCConnection::generateId()
{
    // Server-wide atomic counter for efficient unique ID generation
    // Start from 1000 to avoid conflicts with any manual test IDs
    static std::atomic<U64> sRequestIdCounter{1000};

    // Generate server-unique sequential ID
    U64 id = sRequestIdCounter.fetch_add(1);
    return LLSD(llformat("rpc_%llu", id));
}

void LLJSONRPCConnection::registerMethod(const std::string& method, MethodHandler handler)
{
    mMethodHandlers[method] = handler;
    LL_DEBUGS("JSONRPC") << "Registered method: " << method << LL_ENDL;
}

void LLJSONRPCConnection::unregisterMethod(const std::string& method)
{
    mMethodHandlers.erase(method);
    LL_DEBUGS("JSONRPC") << "Unregistered method: " << method << LL_ENDL;
}

LLSD LLJSONRPCConnection::call(const std::string& method, const LLSD& params, ResponseCallback callback)
{
    LLSD request;
    request["jsonrpc"] = "2.0";
    request["method"] = method;

    if (!params.isUndefined())
    {
        request["params"] = params;
    }

    LLSD id = generateId();
    request["id"] = id;

    // Store callback if provided
    if (callback)
    {
        mPendingRequests[id.asString()] = callback;
    }

    // Send the request
    if (!sendMessage(LlsdToJson(request)))
    {
        // Remove from pending if send failed
        if (callback)
        {
            mPendingRequests.erase(id.asString());
        }
        LL_WARNS("JSONRPC") << "Failed to send request" << LL_ENDL;
        return LLSD();
    }

    LL_DEBUGS("JSONRPC") << "Sent request: " << method << " with id: " << id.asString() << LL_ENDL;
    return id;
}

bool LLJSONRPCConnection::notify(const std::string& method, const LLSD& params)
{
    LLSD notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = method;

    if (!params.isUndefined())
    {
        notification["params"] = params;
    }

    // Notifications don't have an id

    if (!sendMessage(LlsdToJson(notification)))
    {
        LL_WARNS("JSONRPC") << "Failed to send notification" << LL_ENDL;
        return false;
    }

    LL_DEBUGS("JSONRPC") << "Sent notification: " << method << LL_ENDL;
    return true;
}

bool LLJSONRPCConnection::sendResponse(const LLSD& id, const LLSD& result)
{
    LLSD response;
    response["jsonrpc"] = "2.0";
    response["result"] = result;
    response["id"] = id;

    if (!sendMessage(LlsdToJson(response)))
    {
        LL_WARNS("JSONRPC") << "Failed to send response for id: " << id.asString() << LL_ENDL;
        return false;
    }
    LL_DEBUGS("JSONRPC") << "Sent response for id: " << id.asString() << LL_ENDL;
    return true;
}

bool LLJSONRPCConnection::sendError(const LLSD& id, const RPCError& error)
{
    LLSD response;
    response["jsonrpc"] = "2.0";

    LLSD error_obj;
    error_obj["code"] = error.getCode();
    error_obj["message"] = error.what();

    if (!error.getData().isUndefined())
    {
        error_obj["data"] = error.getData();
    }

    response["error"] = error_obj;
    response["id"] = id.isUndefined() ? LLSD() : id;  // null for parse errors

    if (!sendMessage(LlsdToJson(response)))
    {
        LL_WARNS("JSONRPC") << "Failed to send error response" << LL_ENDL;
        return false;
    }
    LL_DEBUGS("JSONRPC") << "Sent error response: " << error.what() << LL_ENDL;
    return true;
}

bool LLJSONRPCConnection::sendBatch(const LLSD& batch, ResponseCallback callback)
{
    if (!batch.isArray() || batch.size() == 0)
    {
        LL_WARNS("JSONRPC") << "Batch must be non-empty array" << LL_ENDL;
        return false;
    }

    // For batch requests with callbacks, we need to track multiple responses
    // This is complex as we need to correlate all responses before calling callback
    // For now, we'll send the batch but won't support batch response callbacks
    if (callback)
    {
        LL_WARNS("JSONRPC") << "Batch response callbacks not yet implemented" << LL_ENDL;
    }

    if (!sendMessage(LlsdToJson(batch)))
    {
        LL_WARNS("JSONRPC") << "Failed to send batch" << LL_ENDL;
        return false;
    }

    LL_DEBUGS("JSONRPC") << "Sent batch with " << batch.size() << " messages" << LL_ENDL;
    return true;
}

//========================================================================
// LLJSONRPCServer Implementation
//========================================================================

LLJSONRPCServer::LLJSONRPCServer(const std::string& name, U16 port, bool local_only)
    : LLWebsocketMgr::WSServer(name, port, local_only), mServerName(name)
{
    LL_INFOS("JSONRPC") << "Created JSON-RPC server: " << name
                        << " on port " << port << LL_ENDL;

    // Register standard JSON-RPC methods
    registerGlobalMethod("system.listMethods", [this](const std::string& method, const LLSD& id, const LLSD& params) -> LLSD {
        LL_DEBUGS("JSONRPC") << "System method " << method << " called" << LL_ENDL;
        return getMethodList();
    });

    registerGlobalMethod("system.getStats", [this](const std::string& method, const LLSD& id, const LLSD& params) -> LLSD {
        LL_DEBUGS("JSONRPC") << "System method " << method << " called" << LL_ENDL;
        return getServerStats();
    });

    registerGlobalMethod("system.ping", [](const std::string& method, const LLSD& id, const LLSD& params) -> LLSD {
        LL_DEBUGS("JSONRPC") << "System method " << method << " called" << LL_ENDL;
        LLSD result;
        result["pong"] = LLDate::now().asString();
        result["params"] = params;
        return result;
    });
}

LLWebsocketMgr::WSConnection::ptr_t LLJSONRPCServer::connectionFactory(LLWebsocketMgr::WSServer::ptr_t server,
                                                                       LLWebsocketMgr::connection_h handle)
{
    auto connection = std::make_shared<LLJSONRPCConnection>(server, handle);
    setupConnectionMethods(connection);
    return connection;
}

void LLJSONRPCServer::onConnectionOpened(const LLWebsocketMgr::WSConnection::ptr_t& connection)
{
    LL_INFOS("JSONRPC") << "JSON-RPC client connected, total connections: "
                        << getConnectionCount() << LL_ENDL;
}

void LLJSONRPCServer::onConnectionClosed(const LLWebsocketMgr::WSConnection::ptr_t& connection)
{
    LL_INFOS("JSONRPC") << "JSON-RPC client disconnected, total connections: "
                        << getConnectionCount() << LL_ENDL;
}

void LLJSONRPCServer::setupConnectionMethods(LLJSONRPCConnection::ptr_t connection)
{
    LLMutexLock lock(&mGlobalMethodsMutex);

    // Register all global methods on the new connection
    for (const auto& [method, handler] : mGlobalMethods)
    {
        connection->registerMethod(method, handler);
    }
}

void LLJSONRPCServer::registerGlobalMethod(const std::string& method, MethodHandler handler)
{
    {
        LLMutexLock lock(&mGlobalMethodsMutex);
        mGlobalMethods[method] = handler;
    }

    // Apply to all existing connections - we need to iterate through connections
    // Since mConnections is private, we need to use broadcastMessage or find another approach
    // For now, we'll only apply to new connections

    LL_INFOS("JSONRPC") << "Registered global method: " << method << LL_ENDL;
}

void LLJSONRPCServer::unregisterGlobalMethod(const std::string& method)
{
    {
        LLMutexLock lock(&mGlobalMethodsMutex);
        mGlobalMethods.erase(method);
    }

    // For existing connections, we would need access to them
    // This is a limitation of the current design - methods added after connection
    // establishment won't be retroactively applied

    LL_INFOS("JSONRPC") << "Unregistered global method: " << method << LL_ENDL;
}

LLSD LLJSONRPCServer::getMethodList() const
{
    LLMutexLock lock(&mGlobalMethodsMutex);

    LLSD methods = LLSD::emptyArray();
    for (const auto& [method, handler] : mGlobalMethods)
    {
        methods.append(method);
    }

    return methods;
}

void LLJSONRPCServer::broadcastNotification(const std::string& method, const LLSD& params)
{
    // Use custom broadcast logic since we need to call notify() on each JSON-RPC connection
    // We can't use the base broadcastMessage() because we need structured JSON-RPC messages

    // Create the notification message
    LLSD notification;
    notification["jsonrpc"] = "2.0";
    notification["method"] = method;
    if (!params.isUndefined())
    {
        notification["params"] = params;
    }

    // Use the base class broadcast functionality
    broadcastMessage(boost::json::serialize(LlsdToJson(notification)));

    mTotalNotificationsSent += getConnectionCount();
    LL_DEBUGS("JSONRPC") << "Broadcast notification: " << method
                         << " to " << getConnectionCount() << " clients" << LL_ENDL;
}

void LLJSONRPCServer::broadcastCall(const std::string& method, const LLSD& params,
                                   BatchResponseCallback callback)
{
    if (callback)
    {
        LL_WARNS("JSONRPC") << "Broadcast call response callbacks not yet implemented" << LL_ENDL;
    }

    // Create the request message with a server-unique ID
    LLSD request;
    request["jsonrpc"] = "2.0";
    request["method"] = method;

    // Use the same ID generation as connections for consistency
    static std::atomic<U64> sBroadcastIdCounter{10000000}; // Start at 10M to clearly distinguish from regular requests
    U64 id = sBroadcastIdCounter.fetch_add(1);
    request["id"] = LLSD(llformat("broadcast_%llu", id));

    if (!params.isUndefined())
    {
        request["params"] = params;
    }

    // Use the base class broadcast functionality
    broadcastMessage(boost::json::serialize(LlsdToJson(request)));

    LL_DEBUGS("JSONRPC") << "Broadcast call: " << method
                         << " to " << getConnectionCount() << " clients" << LL_ENDL;
}

LLSD LLJSONRPCServer::getServerStats() const
{
    LLSD stats;
    stats["server_name"] = mServerName;
    stats["connection_count"] = static_cast<S32>(getConnectionCount());
    stats["is_running"] = isRunning();

    {
        LLMutexLock lock(&mGlobalMethodsMutex);
        stats["global_method_count"] = static_cast<S32>(mGlobalMethods.size());
    }

    stats["total_requests_handled"] = static_cast<LLSD::Integer>(mTotalRequestsHandled.load());
    stats["total_notifications_sent"] = static_cast<LLSD::Integer>(mTotalNotificationsSent.load());
    stats["uptime"] = LLDate::now().asString();

    return stats;
}
