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
#include "llcoros.h"
#include "llmainthreadtask.h"
#include "lleventtimer.h"
#include "lltimer.h"

#include <boost/json.hpp>

//========================================================================
// LLJSONRPCConnection Implementation
//========================================================================

void LLJSONRPCConnection::onOpen()
{
    LL_INFOS("JSONRPC") << "JSON-RPC connection opened" << LL_ENDL;

    // Start the recurring timeout sweep timer on the main thread. The timer
    // is canceled in onClose() before the connection can be destroyed, so
    // capturing `this` is safe. Keep a weak_ptr so we can safely test
    // whether the timer instance still exists at cancellation time.
    LLEventTimer* timer = LLEventTimer::run_every(TIMEOUT_SWEEP_INTERVAL,
        [this]() { sweepTimeouts(); });
    mTimeoutTimer = timer->getWeak();
}

void LLJSONRPCConnection::onClose()
{
    // Cancel the sweep timer if it is still alive. LLEventTimer's instance
    // tracker keeps a shared_ptr with a no-op deleter, so raw `delete` is
    // the documented cancellation idiom (see lleventtimer.h).
    if (auto timer = mTimeoutTimer.lock())
    {
        delete timer.get();
    }
    mTimeoutTimer.reset();

    // Move the pending-request map out under the lock so we can invoke the
    // callbacks without holding it (callbacks may themselves call into this
    // connection).
    std::unordered_map<std::string, ResponseCallback> pending;
    {
        LLMutexLock lock(&mMutex);
        pending.swap(mPendingRequests);
        // Deadlines correspond to entries in mPendingRequests; drop them.
        std::priority_queue<PendingDeadline> empty;
        mPendingDeadlines.swap(empty);
    }

    LL_INFOS("JSONRPC") << "JSON-RPC connection closed, clearing "
                        << pending.size() << " pending requests" << LL_ENDL;

    for (auto& [id, callback] : pending)
    {
        if (callback)
        {
            LLSD error;
            error["code"] = RPCError::CONNECTION_CLOSED;
            error["message"] = "Connection closed";
            callback(LLSD(), error);
        }
    }
}

void LLJSONRPCConnection::onMessage(const std::string& message)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_WEBSOCKET;
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
            // JSON-RPC 2.0 batch requests are intentionally not supported.
            // No known client (including the sl-vscode-plugin) sends batches,
            // and a spec-compliant implementation would require accumulating
            // responses across sync + async handlers before shipping a single
            // array frame. If a real use case appears, implement per
            // JSON-RPC 2.0 §6.
            sendError(LLSD(), InvalidRequest("Batch requests are not supported"));
            return;
        }

        // Single message
        processMessage(message_obj);
    }
    catch (const std::exception& e)
    {
        LL_WARNS("JSONRPC") << "Exception processing JSON-RPC message: " << e.what() << LL_ENDL;
        sendError(LLSD(), InternalError(e.what()));
    }
}

void LLJSONRPCConnection::processMessage(const LLSD& message_obj)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_WEBSOCKET;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_WEBSOCKET;
    std::string method = request["method"].asString();
    LLSD params = request.has("params") ? request["params"] : LLSD();
    LLSD id = request.has("id") ? request["id"] : LLSD();
    bool is_notification = !request.has("id");

    LL_DEBUGS("JSONRPC") << "Processing " << (is_notification ? "notification" : "request")
                         << " for method: " << method << LL_ENDL;

    // Resolve the handler under the mutex, then invoke it unlocked.
    MethodHandler handler;
    bool          is_async = false;
    {
        LLMutexLock lock(&mMutex);
        auto async_it = mAsyncMethodHandlers.find(method);
        if (async_it != mAsyncMethodHandlers.end())
        {
            handler  = async_it->second;
            is_async = true;
        }
        else
        {
            auto sync_it = mMethodHandlers.find(method);
            if (sync_it != mMethodHandlers.end())
            {
                handler = sync_it->second;
            }
        }
    }

    if (is_async)
    {
        // Async handler — launched as a coroutine, response sent by the lambda.
        if (is_notification)
        {
            LL_WARNS("JSONRPC") << "Method " << method
                                << " called as notification; rejecting request" << LL_ENDL;
            throw InvalidRequest("Method " + method + " cannot be called as a notification");
        }
        ptr_t conn = std::static_pointer_cast<LLJSONRPCConnection>(getSelfPtr());
        if (!conn)
        {
            LL_WARNS("JSONRPC") << "Connection expired before method " << method
                                << " could be launched; failing request" << LL_ENDL;
            throw InternalError("Connection expired before method " + method
                                + " could be launched");
        }
        LLMainThreadTask::dispatch(
            [handler, method, id, params, conn]()
            {
                LLCoros::instance().launch(
                    "JSONRPC::" + method,
                    [handler, method, id, params, conn]()
                    {
                        try
                        {
                            LLSD result = handler(method, id, params);
                            if (conn->isConnected())
                            {
                                conn->sendResponse(id, result);
                            }
                            else
                            {
                                LL_WARNS("JSONRPC") << "Connection closed before async method "
                                                    << method << " could send response" << LL_ENDL;
                            }
                        }
                        catch (const RPCError& e)
                        {
                            if (conn->isConnected())
                            {
                                conn->sendError(id, e);
                            }
                            else
                            {
                                LL_WARNS("JSONRPC") << "Connection closed before async method "
                                                    << method << " could send error" << LL_ENDL;
                            }
                        }
                        catch (const std::exception& e)
                        {
                            if (conn->isConnected())
                            {
                                conn->sendError(id, InternalError(e.what()));
                            }
                            else
                            {
                                LL_WARNS("JSONRPC") << "Connection closed before async method "
                                                    << method << " could send error" << LL_ENDL;
                            }
                        }
                    });
            });
        return;
    }

    if (!handler)
    {
        if (!is_notification)
        {
            sendError(id, MethodNotFound(method));
        }
        return;
    }

    try
    {
        LLSD result = handler(method, id, params);

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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_WEBSOCKET;
    if (!response.has("id"))
    {
        LL_WARNS("JSONRPC") << "Response missing id field" << LL_ENDL;
        return;
    }

    std::string id = response["id"].asString();
    ResponseCallback callback;
    {
        LLMutexLock lock(&mMutex);
        auto it = mPendingRequests.find(id);
        if (it == mPendingRequests.end())
        {
            LL_WARNS("JSONRPC") << "Received response for unknown request id: " << id << LL_ENDL;
            return;
        }
        callback = std::move(it->second);
        mPendingRequests.erase(it);
    }

    if (callback)
    {
        LLSD result = response.has("result") ? response["result"] : LLSD();
        LLSD error  = response.has("error")  ? response["error"]  : LLSD();

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
                return false;
            }
            if (!error.has("code") || !error.has("message"))
            {
                LL_WARNS("JSONRPC") << "Error must have code and message" << LL_ENDL;
                return false;
            }
        }
    }
    return true;
}

void LLJSONRPCConnection::sweepTimeouts()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_WEBSOCKET;
    // Pop expired deadlines and collect their callbacks. Tombstones (entries
    // whose request already completed) are silently discarded.
    std::vector<std::pair<std::string, ResponseCallback>> expired;
    const F64 now = LLTimer::getTotalSeconds();
    {
        LLMutexLock lock(&mMutex);
        while (!mPendingDeadlines.empty() && mPendingDeadlines.top().mDeadline <= now)
        {
            std::string id = mPendingDeadlines.top().mId;
            mPendingDeadlines.pop();
            auto it = mPendingRequests.find(id);
            if (it != mPendingRequests.end())
            {
                expired.emplace_back(std::move(id), std::move(it->second));
                mPendingRequests.erase(it);
            }
        }
    }

    for (auto& [id, callback] : expired)
    {
        LL_WARNS("JSONRPC") << "Request " << id << " timed out after "
                            << REQUEST_TIMEOUT_SECONDS << " seconds" << LL_ENDL;
        if (callback)
        {
            LLSD error;
            error["code"]    = RPCError::REQUEST_TIMEOUT;
            error["message"] = "Request timed out";
            callback(LLSD(), error);
        }
    }
}

void LLJSONRPCConnection::testInjectPendingRequest(const std::string& id, F64 deadline, ResponseCallback callback)
{
    LLMutexLock lock(&mMutex);
    mPendingRequests[id] = std::move(callback);
    mPendingDeadlines.push({ deadline, id });
}

void LLJSONRPCConnection::testSweepTimeouts()
{
    sweepTimeouts();
}

size_t LLJSONRPCConnection::testPendingRequestCount() const
{
    LLMutexLock lock(&mMutex);
    return mPendingRequests.size();
}

LLSD LLJSONRPCConnection::generateId()
{
    // Server-wide atomic counter for efficient unique ID generation.
    // Start above zero to avoid conflicts with any manual test IDs.
    static constexpr U64 REQUEST_ID_START = 1000;
    static std::atomic<U64> sRequestIdCounter{REQUEST_ID_START};

    // Generate server-unique sequential ID
    U64 id = sRequestIdCounter.fetch_add(1);
    return LLSD(llformat("rpc_%llu", id));
}

void LLJSONRPCConnection::registerMethod(const std::string& method, MethodHandler handler)
{
    {
        LLMutexLock lock(&mMutex);
        mMethodHandlers[method] = std::move(handler);
    }
    LL_DEBUGS("JSONRPC") << "Registered method: " << method << LL_ENDL;
}

void LLJSONRPCConnection::registerAsyncMethod(const std::string& method, MethodHandler handler)
{
    {
        LLMutexLock lock(&mMutex);
        mAsyncMethodHandlers[method] = std::move(handler);
    }
    LL_DEBUGS("JSONRPC") << "Registered async method: " << method << LL_ENDL;
}

void LLJSONRPCConnection::unregisterMethod(const std::string& method)
{
    {
        LLMutexLock lock(&mMutex);
        mMethodHandlers.erase(method);
        mAsyncMethodHandlers.erase(method);
    }
    LL_DEBUGS("JSONRPC") << "Unregistered method: " << method << LL_ENDL;
}

std::set<std::string> LLJSONRPCConnection::getMethods() const
{
    LLMutexLock lock(&mMutex);
    std::set<std::string> methods;

    for (const auto& [method, handler] : mMethodHandlers)
    {
        methods.insert(method);
    }

    for (const auto& [method, handler] : mAsyncMethodHandlers)
    {
        methods.insert(method);
    }

    return methods;
}

LLSD LLJSONRPCConnection::makeEnvelope(const LLSD& id,
                                       const std::string& method,
                                       const LLSD& params,
                                       const LLSD& result,
                                       const LLSD& error)
{
    LLSD env;
    env["jsonrpc"] = "2.0";
    // Notifications (requests without an id) are the only case that omits id.
    if (!(id.isUndefined() && !method.empty()))
    {
        env["id"] = id;
    }
    if (!method.empty())
    {
        env["method"] = method;
    }
    if (params.isDefined())
    {
        env["params"] = params;
    }
    if (result.isDefined())
    {
        env["result"] = result;
    }
    if (error.isDefined())
    {
        env["error"] = error;
    }
    return env;
}

LLSD LLJSONRPCConnection::call(const std::string& method, const LLSD& params, ResponseCallback callback)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_WEBSOCKET;
    LLSD id = generateId();
    LLSD request = makeEnvelope(id, method, params, LLSD(), LLSD());
    const std::string id_str = id.asString();

    // Store callback if provided. Fire-and-forget calls (no callback) are
    // not tracked for timeouts since there is nobody to deliver the error to.
    if (callback)
    {
        LLMutexLock lock(&mMutex);
        mPendingRequests[id_str] = std::move(callback);
        mPendingDeadlines.push({ LLTimer::getTotalSeconds() + REQUEST_TIMEOUT_SECONDS, id_str });
    }

    // Send the request
    if (!sendMessage(LlsdToJson(request)))
    {
        // Remove from pending if send failed
        {
            LLMutexLock lock(&mMutex);
            mPendingRequests.erase(id_str);
        }
        LL_WARNS("JSONRPC") << "Failed to send request" << LL_ENDL;
        return LLSD();
    }

    LL_DEBUGS("JSONRPC") << "Sent request: " << method << " with id: " << id.asString() << LL_ENDL;
    return id;
}

bool LLJSONRPCConnection::notify(const std::string& method, const LLSD& params)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_WEBSOCKET;
    LLSD notification = makeEnvelope(LLSD(), method, params, LLSD(), LLSD());

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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_WEBSOCKET;
    LLSD response = makeEnvelope(id, std::string(), LLSD(), result, LLSD());

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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_WEBSOCKET;
    LLSD error_obj;
    error_obj["code"] = error.getCode();
    error_obj["message"] = error.what();

    if (!error.getData().isUndefined())
    {
        error_obj["data"] = error.getData();
    }

    // Responses always include id; an undefined id serializes as null (used for parse errors).
    LLSD response = makeEnvelope(id, std::string(), LLSD(), LLSD(), error_obj);

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
    registerGlobalMethod("system.getStats", [this](const std::string& method, const LLSD& id, const LLSD& params) -> LLSD {
        LL_DEBUGS("JSONRPC") << "System method " << method << " called" << LL_ENDL;
        return getServerStats();
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

    std::weak_ptr<LLJSONRPCConnection> weak_connection = connection;
    connection->registerMethod(
        "system.listMethods",
        [weak_connection](
            const std::string&,
            const LLSD&,
            const LLSD&) -> LLSD
        {
            LLSD methods(LLSD::emptyArray());
            auto connection = weak_connection.lock();
            if (!connection)
            {
                return methods;
            }

            for (const std::string& method : connection->getMethods())
            {
                methods.append(method);
            }

            return methods;
        });

    connection->registerMethod(
        "system.ping",
        [this, weak_connection](
            const std::string& method,
            const LLSD& id,
            const LLSD& params) -> LLSD
        {
            LL_DEBUGS("JSONRPC") << "System method " << method
                                 << " called" << LL_ENDL;
            return handlePing(weak_connection.lock(), params);
        });

    connection->registerMethod(
        "system.getVersion",
        [this, weak_connection](
            const std::string& method,
            const LLSD& id,
            const LLSD& params) -> LLSD
        {
            LL_DEBUGS("JSONRPC") << "System method " << method
                                 << " called" << LL_ENDL;
            return handleGetVersion(weak_connection.lock(), params);
        });

    connection->registerMethod(
        "system.status",
        [this, weak_connection](
            const std::string& method,
            const LLSD& id,
            const LLSD& params) -> LLSD
        {
            LL_DEBUGS("JSONRPC") << "System method " << method
                                 << " called" << LL_ENDL;
            return handleStatus(weak_connection.lock(), params);
        });
}

LLSD LLJSONRPCServer::handlePing(
    const LLJSONRPCConnection::ptr_t& connection,
    const LLSD& params) const
{
    return LLSD("pong");
}

LLSD LLJSONRPCServer::handleStatus(
    const LLJSONRPCConnection::ptr_t& connection,
    const LLSD& params) const
{
    LLSD result;
    result["status"] = "OK";
    return result;
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
    LL_PROFILE_ZONE_SCOPED_CATEGORY_WEBSOCKET;
    // Use custom broadcast logic since we need to call notify() on each JSON-RPC connection
    // We can't use the base broadcastMessage() because we need structured JSON-RPC messages

    // Create the notification message
    LLSD notification = LLJSONRPCConnection::makeEnvelope(LLSD(), method, params, LLSD(), LLSD());

    // Use the base class broadcast functionality
    broadcastMessage(boost::json::serialize(LlsdToJson(notification)));

    // Cache the count: getConnectionCount() walks a locked container in the base.
    size_t count = getConnectionCount();
    mTotalNotificationsSent += count;
    LL_DEBUGS("JSONRPC") << "Broadcast notification: " << method
                         << " to " << count << " clients" << LL_ENDL;
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
