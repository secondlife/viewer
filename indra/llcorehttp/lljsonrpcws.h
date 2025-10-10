/**
 * @file lljsonrpcws.h
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

#pragma once

#include "llwebsocketmgr.h"
#include "llsd.h"
#include "lluuid.h"

#include <functional>
#include <unordered_map>
#include <memory>

/**
 * @class LLJSONRPCConnection
 * @brief JSON-RPC 2.0 WebSocket connection implementation
 *
 * This class implements the JSON-RPC 2.0 protocol over WebSocket connections.
 * It handles request/response patterns, notifications, method registration,
 * and error handling according to the JSON-RPC 2.0 specification.
 *
 * ## JSON-RPC 2.0 Protocol Features
 *
 * - **Requests**: Method calls that expect a response
 * - **Notifications**: Method calls that do not expect a response
 * - **Batch Operations**: Multiple requests/notifications in a single message
 * - **Error Handling**: Standardized error codes and messages
 * - **ID Correlation**: Request/response correlation using unique identifiers
 *
 * ## Method Handler Registration
 *
 * Methods use an enhanced handler signature that provides method name and request ID context:
 *
 * @code
 * connection->registerMethod("echo", [](const std::string& method, const LLSD& id, const LLSD& params) -> LLSD {
 *     LL_INFOS("JSONRPC") << "Method " << method << " called with ID " << id.asString() << LL_ENDL;
 *     return params; // Echo back the parameters
 * });
 *
 * connection->registerMethod("add", [](const std::string& method, const LLSD& id, const LLSD& params) -> LLSD {
 *     if (params.isArray() && params.size() >= 2) {
 *         LL_INFOS("JSONRPC") << "Adding numbers via " << method << LL_ENDL;
 *         return params[0].asReal() + params[1].asReal();
 *     }
 *     throw LLJSONRPCConnection::InvalidParams("Expected array with 2 numbers");
 * });
 * @endcode
 *
 * The enhanced signature enables:
 * - Method context awareness for shared handlers
 * - Request correlation and distributed tracing
 * - Distinction between notifications (id undefined) and requests
 * - Enhanced logging and error reporting with context
 *
 * ## Making RPC Calls
 *
 * @code
 * // Asynchronous request with callback
 * LLSD params;
 * params.append(5);
 * params.append(3);
 * connection->call("add", params, [](const LLSD& result, const LLSD& error) {
 *     if (error.isUndefined()) {
 *         LL_INFOS() << "Result: " << result.asReal() << LL_ENDL;
 *     } else {
 *         LL_WARNS() << "Error: " << error["message"].asString() << LL_ENDL;
 *     }
 * });
 *
 * // Fire-and-forget notification
 * connection->notify("log", LLSD("Server started"));
 * @endcode
 */
class LLJSONRPCConnection : public LLWebsocketMgr::WSConnection
{
public:
    using ptr_t = std::shared_ptr<LLJSONRPCConnection>;

    /// Method handler function signature
    /// @param method The method name that was called
    /// @param id The request ID (undefined for notifications)
    /// @param params The parameters passed to the method
    /// @return The result to return to the caller
    /// @throw RPCError-derived exceptions for error responses
    using MethodHandler = std::function<LLSD(const std::string& method, const LLSD& id, const LLSD& params)>;

    /// Response callback function signature
    /// @param result The result from a successful call (undefined if error occurred)
    /// @param error The error object if call failed (undefined if successful)
    using ResponseCallback = std::function<void(const LLSD& result, const LLSD& error)>;

    /**
     * @brief JSON-RPC error base class
     */
    class RPCError : public std::runtime_error
    {
    public:
        // JSON-RPC 2.0 Standard Error Codes
        static constexpr S32 PARSE_ERROR = -32700;              ///< Invalid JSON was received by the server
        static constexpr S32 INVALID_REQUEST = -32600;          ///< The JSON sent is not a valid Request object
        static constexpr S32 METHOD_NOT_FOUND = -32601;         ///< The method does not exist / is not available
        static constexpr S32 INVALID_PARAMS = -32602;           ///< Invalid method parameter(s)
        static constexpr S32 INTERNAL_ERROR = -32603;           ///< Internal JSON-RPC error

        // Server Error Range (-32000 to -32099)
        static constexpr S32 SERVER_ERROR_MIN = -32099;         ///< Server error range minimum
        static constexpr S32 SERVER_ERROR_MAX = -32000;         ///< Server error range maximum

        // Common server-specific errors
        static constexpr S32 CONNECTION_CLOSED = -32000;        ///< Connection closed unexpectedly
        static constexpr S32 REQUEST_TIMEOUT = -32001;          ///< Request timed out
        static constexpr S32 UNAUTHORIZED = -32002;             ///< Authentication required
        static constexpr S32 FORBIDDEN = -32003;                ///< Access denied
        static constexpr S32 RATE_LIMITED = -32004;             ///< Too many requests
        static constexpr S32 SERVICE_UNAVAILABLE = -32005;      ///< Service temporarily unavailable
        static constexpr S32 MESSAGE_TOO_LARGE = -32006;        ///< Message exceeds maximum size
        static constexpr S32 INVALID_SESSION = -32007;          ///< Session expired or invalid

        RPCError(S32 code, const std::string& message, const LLSD& data = LLSD())
            : std::runtime_error(message), mCode(code), mData(data) {}

        S32 getCode() const { return mCode; }
        const LLSD& getData() const { return mData; }

    protected:
        S32 mCode;
        LLSD mData;
    };

    /// Standard JSON-RPC error classes using named constants
    class ParseError : public RPCError {
    public:
        ParseError(const std::string& details = "")
            : RPCError(PARSE_ERROR, "Parse error" + (details.empty() ? "" : ": " + details)) {}
    };

    class InvalidRequest : public RPCError {
    public:
        InvalidRequest(const std::string& details = "")
            : RPCError(INVALID_REQUEST, "Invalid Request" + (details.empty() ? "" : ": " + details)) {}
    };

    class MethodNotFound : public RPCError {
    public:
        MethodNotFound(const std::string& method = "")
            : RPCError(METHOD_NOT_FOUND, "Method not found" + (method.empty() ? "" : ": " + method)) {}
    };

    class InvalidParams : public RPCError {
    public:
        InvalidParams(const std::string& details = "")
            : RPCError(INVALID_PARAMS, "Invalid params" + (details.empty() ? "" : ": " + details)) {}
    };

    class InternalError : public RPCError {
    public:
        InternalError(const std::string& details = "")
            : RPCError(INTERNAL_ERROR, "Internal error" + (details.empty() ? "" : ": " + details)) {}
    };

    /// Server-specific errors (in the -32000 to -32099 range)
    class ConnectionClosedError : public RPCError {
    public:
        ConnectionClosedError(const std::string& details = "Connection closed")
            : RPCError(CONNECTION_CLOSED, details) {}
    };

    class RequestTimeoutError : public RPCError {
    public:
        RequestTimeoutError(const std::string& details = "Request timed out")
            : RPCError(REQUEST_TIMEOUT, details) {}
    };

    class UnauthorizedError : public RPCError {
    public:
        UnauthorizedError(const std::string& details = "Authentication required")
            : RPCError(UNAUTHORIZED, details) {}
    };

    class ForbiddenError : public RPCError {
    public:
        ForbiddenError(const std::string& details = "Access denied")
            : RPCError(FORBIDDEN, details) {}
    };

    class RateLimitedError : public RPCError {
    public:
        RateLimitedError(const std::string& details = "Too many requests")
            : RPCError(RATE_LIMITED, details) {}
    };

    class ServiceUnavailableError : public RPCError {
    public:
        ServiceUnavailableError(const std::string& details = "Service temporarily unavailable")
            : RPCError(SERVICE_UNAVAILABLE, details) {}
    };

    class MessageTooLargeError : public RPCError {
    public:
        MessageTooLargeError(const std::string& details = "Message exceeds maximum size")
            : RPCError(MESSAGE_TOO_LARGE, details) {}
    };

    class InvalidSessionError : public RPCError {
    public:
        InvalidSessionError(const std::string& details = "Session expired or invalid")
            : RPCError(INVALID_SESSION, details) {}
    };


    LLJSONRPCConnection(const LLWebsocketMgr::WSServer::ptr_t server,
                       const LLWebsocketMgr::connection_h& handle)
        : LLWebsocketMgr::WSConnection(server, handle) {}

    virtual ~LLJSONRPCConnection() = default;

    // WebSocket connection lifecycle
    void onOpen() override;
    void onClose() override;
    void onMessage(const std::string& message) override;

    /**
     * @brief Register a method handler
     * @param method The method name to register
     * @param handler The function to call when this method is invoked
     */
    void registerMethod(const std::string& method, MethodHandler handler);

    /**
     * @brief Unregister a method handler
     * @param method The method name to unregister
     */
    void unregisterMethod(const std::string& method);

    /**
     * @brief Make an asynchronous JSON-RPC call
     * @param method The method name to call
     * @param params The parameters to pass
     * @param callback Callback for the response (optional)
     * @return The request ID for correlation
     */
    LLSD call(const std::string& method, const LLSD& params = LLSD(),
             ResponseCallback callback = nullptr);

    /**
     * @brief Send a JSON-RPC notification (no response expected)
     * @param method The method name
     * @param params The parameters to pass
     */
    bool notify(const std::string& method, const LLSD& params = LLSD());

    /**
     * @brief Send a successful response to a request
     * @param id The request ID from the original request
     * @param result The result to return
     */
    bool sendResponse(const LLSD& id, const LLSD& result);

    /**
     * @brief Send an error response to a request
     * @param id The request ID from the original request (can be null)
     * @param error The RPCError to send
     */
    bool sendError(const LLSD& id, const RPCError& error);

    /**
     * @brief Send a batch of requests/notifications
     * @param batch Array of request/notification objects
     * @param callback Callback for batch response (optional)
     */
    bool sendBatch(const LLSD& batch, ResponseCallback callback = nullptr);

protected:
    /**
     * @brief Process a single JSON-RPC message
     * @param message_obj The parsed JSON message
     */
    void processMessage(const LLSD& message_obj);

    /**
     * @brief Process a JSON-RPC request
     * @param request The request object
     */
    void processRequest(const LLSD& request);

    /**
     * @brief Process a JSON-RPC response
     * @param response The response object
     */
    void processResponse(const LLSD& response);

    /**
     * @brief Validate a JSON-RPC message structure
     * @param message The message to validate
     * @param is_request True if validating a request, false for response
     */
    bool validateMessage(const LLSD& message, bool is_request = true);

    /**
     * @brief Generate the next unique request ID
     * @return A server-unique request ID
     *
     * Generates a server-wide unique identifier using an atomic counter.
     * This ensures request IDs are unique across all connections within the
     * server instance, providing efficient ID generation with guaranteed uniqueness.
     *
     * IDs follow the format "rpc_{counter}" where counter is a monotonically
     * increasing 64-bit value starting from 1. This approach provides:
     * - Guaranteed uniqueness within server scope
     * - High performance (atomic increment operation)
     * - Predictable, sequential ordering for debugging
     * - Thread-safe generation across multiple connections
     */
    LLSD generateId();

private:
    std::unordered_map<std::string, MethodHandler> mMethodHandlers;
    std::unordered_map<std::string, ResponseCallback> mPendingRequests;
};

/**
 * @class LLJSONRPCServer
 * @brief JSON-RPC 2.0 WebSocket server implementation
 *
 * This server extends the basic WebSocket server to provide JSON-RPC 2.0
 * protocol support. It manages JSON-RPC connections and provides server-wide
 * method registration and broadcasting capabilities.
 *
 * ## Server-Wide Method Registration
 *
 * Methods can be registered at the server level and will be available
 * on all connections:
 *
 * @code
 * auto server = std::make_shared<LLJSONRPCServer>("rpc_server", 8080);
 *
 * server->registerGlobalMethod("getServerInfo", [](const std::string& method, const LLSD& id, const LLSD& params) -> LLSD {
 *     LL_INFOS("JSONRPC") << "Server info requested via " << method << LL_ENDL;
 *     LLSD info;
 *     info["name"] = "My RPC Server";
 *     info["version"] = "1.0.0";
 *     info["uptime"] = LLDate::now().secondsSinceEpoch();
 *     return info;
 * });
 *
 * server->registerGlobalMethod("listMethods", [server](const std::string& method, const LLSD& id, const LLSD& params) -> LLSD {
 *     return server->getMethodList();
 * });
 * @endcode
 *
 * ## Broadcasting and Multi-client Operations
 *
 * @code
 * // Broadcast notification to all connected clients
 * server->broadcastNotification("serverAlert", LLSD("Server will restart in 5 minutes"));
 *
 * // Call a method on all clients and collect responses
 * server->broadcastCall("getClientStatus", LLSD(), [](const LLSD& responses) {
 *     for (const auto& response : llsd::inArray(responses)) {
 *         LL_INFOS() << "Client status: " << response << LL_ENDL;
 *     }
 * });
 * @endcode
 */
class LLJSONRPCServer : public LLWebsocketMgr::WSServer
{
public:
    using ptr_t = std::shared_ptr<LLJSONRPCServer>;
    using MethodHandler = LLJSONRPCConnection::MethodHandler;
    using ResponseCallback = LLJSONRPCConnection::ResponseCallback;
    using BatchResponseCallback = std::function<void(const LLSD& responses)>;

    LLJSONRPCServer(const std::string& name, U16 port, bool local_only = true);
    virtual ~LLJSONRPCServer() = default;

    // Server lifecycle callbacks
    void onConnectionOpened(const LLWebsocketMgr::WSConnection::ptr_t& connection) override;
    void onConnectionClosed(const LLWebsocketMgr::WSConnection::ptr_t& connection) override;

    /**
     * @brief Register a global method available on all connections
     * @param method The method name to register
     * @param handler The function to call when this method is invoked
     */
    void registerGlobalMethod(const std::string& method, MethodHandler handler);

    /**
     * @brief Unregister a global method
     * @param method The method name to unregister
     */
    void unregisterGlobalMethod(const std::string& method);

    /**
     * @brief Get list of registered global methods
     * @return Array of method names
     */
    LLSD getMethodList() const;

    /**
     * @brief Broadcast a notification to all connected clients
     * @param method The method name
     * @param params The parameters to pass
     */
    void broadcastNotification(const std::string& method, const LLSD& params = LLSD());

    /**
     * @brief Call a method on all connected clients
     * @param method The method name
     * @param params The parameters to pass
     * @param callback Callback to receive aggregated responses
     */
    void broadcastCall(const std::string& method, const LLSD& params = LLSD(),
                      BatchResponseCallback callback = nullptr);

    /**
     * @brief Get server statistics
     * @return Statistics object with connection count, method count, etc.
     */
    LLSD getServerStats() const;

protected:
    LLWebsocketMgr::WSConnection::ptr_t connectionFactory(LLWebsocketMgr::WSServer::ptr_t server,
                                                         LLWebsocketMgr::connection_h handle) override;

    /**
     * @brief Apply global method handlers to a new connection
     * @param connection The connection to configure
     */
    virtual void setupConnectionMethods(LLJSONRPCConnection::ptr_t connection);

private:
    std::unordered_map<std::string, MethodHandler> mGlobalMethods;
    mutable LLMutex mGlobalMethodsMutex;

    std::string mServerName;  // Store server name for stats
    std::atomic<U64> mTotalRequestsHandled{0};
    std::atomic<U64> mTotalNotificationsSent{0};
};
