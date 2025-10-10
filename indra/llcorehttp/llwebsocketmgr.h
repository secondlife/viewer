/**
 * @file llwebsocketmgr.h
 * @brief WebSocket manager singleton for managing WebSocket servers and connections
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

#include "llsingleton.h"
#include "llsd.h"
#include "lluuid.h"
#include "llhost.h"
#include "llmutex.h"

#include <memory>
#include <map>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>

#include <boost/json.hpp>

#include <websocketpp/common/connection_hdl.hpp>

struct Server_impl;

/**
 * @class LLWebsocketMgr
 * @brief Singleton manager for WebSocket connections and servers
 *
 * This class provides a high-level interface for managing WebSocket connections
 * and servers using websocketpp library. It handles both client and server
 * connections, provides thread-safe operations, and integrates with the
 * existing Linden Lab infrastructure.
 */
class LLWebsocketMgr: public LLSingleton<LLWebsocketMgr>
{
    LLSINGLETON(LLWebsocketMgr) = default;
    virtual ~LLWebsocketMgr() = default;
    LOG_CLASS(LLWebsocketMgr);

public:
    using connection_h = websocketpp::connection_hdl;
    class WSServer;

    enum connection_state_t
    { // must map to websocketpp::session::state
        connection_connecting = 0,
        connection_open       = 1,
        connection_closing    = 2,
        connection_closed     = 3
    };

    class WSConnection
    {
        friend class LLWebsocketMgr;

    public:
        using ptr_t = std::shared_ptr<WSConnection>;

        /**
         * @brief Constructor for WSConnection
         * @param server Shared pointer to the parent WSServer
         * @param handle WebSocket connection handle from websocketpp
         */
        WSConnection(const std::shared_ptr<WSServer> &server, const connection_h& handle):
            mConnectionHandle(handle),
            mOwningServer(server)
        {}

        virtual ~WSConnection() = default;

        /**
         * Override this method in derived classes to handle connection establishment.
         * This is called after the WebSocket handshake is complete and the connection
         * is ready to send/receive messages.
         */
        virtual void onOpen() {}

        /**
         * Override this method in derived classes to handle connection closure.
         * This is called when the connection has been terminated, either normally
         * or due to an error condition.
         */
        virtual void onClose() {}

        /**
         * @brief Called when a message is received
         * @param message The received message as a string
         *
         * Override this method in derived classes to handle incoming messages.
         * Currently only text messages are supported.
         */
        virtual void onMessage(const std::string& message) {}

        /**
         * @brief Send a message to the connected client
         * @param message The message string to send
         * @return true if the message was queued successfully, false on error
         *
         * Sends a text message to the remote endpoint. The message is queued
         * asynchronously and may not be sent immediately.
         */
        bool sendMessage(const std::string& message) const;
        bool sendMessage(const boost::json::value& json) const;
        bool sendMessage(const LLSD& data) const;

        /**
         * @brief Close the WebSocket connection gracefully
         * @param code Optional close code (default: normal closure)
         * @param reason Optional reason string (default: empty)
         *
         * Initiates a graceful WebSocket close handshake. The connection will
         * send a close frame with the specified code and reason, then wait for
         * the remote endpoint to respond with its own close frame before
         * actually closing the underlying TCP connection.
         *
         * Common close codes:
         * - 1000: Normal closure (default)
         * - 1001: Going away (server shutting down, page navigating away)
         * - 1002: Protocol error
         * - 1003: Unsupported data type
         * - 1008: Policy violation
         * - 1009: Message too big
         *
         * @note After calling this method, no further messages should be sent
         * @note The onClose() callback will be invoked when the close handshake completes
         */
        void closeConnection(U16 code = 1000, const std::string& reason = std::string());

        bool isConnected() const;

    protected:
        connection_h mConnectionHandle;
        std::weak_ptr<WSServer> mOwningServer; // Back-reference to the server this connection belongs to
    };

    /**
     * @class WSServer
     * @brief Base class for WebSocket servers with customizable connection handling
     *
     * WSServer provides a high-level abstraction over websocketpp servers, handling
     * threading, connection management, and event dispatching. Derive from this class
     * to create custom WebSocket servers with application-specific logic.
     *
     * ## Basic Usage
     *
     * @code
     * class MyServer : public LLWebsocketMgr::WSServer
     * {
     * public:
     *     MyServer(const std::string& name, U16 port)
     *         : WSServer(name, port, false) // Listen on all interfaces
     *     {}
     *
     *     void onConnectionOpened(const WSConnection::ptr_t& connection) override
     *     {
     *         LL_INFOS("MyServer") << "New client connected" << LL_ENDL;
     *         // Send welcome message
     *         connection->sendMessage("Welcome to the server!");
     *     }
     *
     *     void onConnectionClosed(const WSConnection::ptr_t& connection) override
     *     {
     *         LL_INFOS("MyServer") << "Client disconnected" << LL_ENDL;
     *     }
     *
     * protected:
     *     // Use custom connection class
     *     WSConnection::ptr_t connectionFactory(WSServer::ptr_t server, connection_h handle) override
     *     {
     *         return std::make_shared<MyConnection>(server, handle);
     *     }
     * };
     * @endcode
     *
     * ## Connection Management
     *
     * The server automatically manages connection lifetimes and provides several ways
     * to interact with connections:
     *
     * - `broadcastMessage()` - Send message to all connected clients
     * - `sendMessageTo()` - Send message to specific connection
     * - `closeConnection()` - Close specific connection with code/reason
     * - `getConnection()` - Get connection object by handle
     *
     * ## Thread Safety
     *
     * All public methods are thread-safe and can be called from any thread. The server
     * runs its own background thread for handling WebSocket events, while connection
     * callbacks are also executed on this background thread.
     */
    class WSServer: public std::enable_shared_from_this<WSServer>
    {
        friend struct Server_impl;
        friend class WSConnection;
        friend class LLWebsocketMgr;

    public:
        using ptr_t = std::shared_ptr<WSServer>;

        WSServer(std::string_view name, U16 port, bool local_only = true);
        virtual ~WSServer();

        virtual void    onStarted() {}
        virtual void    onStopped() {}

        virtual void    onConnectionOpened(const WSConnection::ptr_t& connection) { }
        virtual void    onConnectionClosed(const WSConnection::ptr_t& connection) { }

        bool            isRunning() const;
        size_t          getConnectionCount() const
        {
            LLMutexLock lock(&mConnectionMutex);
            return mConnections.size();
        }

        void            broadcastMessage(const std::string& message);
        virtual bool    update() { return true; }

        connection_state_t  getConnectionState(const connection_h& handle) const;

    protected:
        virtual WSConnection::ptr_t connectionFactory(WSServer::ptr_t server, connection_h handle);

        bool            start();
        void            stop();

        bool            sendMessageTo(const connection_h& handle, const std::string& message);

        /**
         * @brief Close a specific connection gracefully
         * @param handle The connection handle to close
         * @param code Close code (default: normal closure)
         * @param reason Close reason string (default: empty)
         * @return true if close was initiated successfully, false on error
         *
         * Internal method used by WSConnection to close individual connections.
         * This method is thread-safe and can be called from any thread.
         */
        bool            closeConnection(const connection_h& handle, U16 code = 1000, const std::string& reason = std::string());

    private:
        using connection_map_t = std::map<connection_h, WSConnection::ptr_t, std::owner_less<connection_h> >;

        WSConnection::ptr_t getConnection(const connection_h& handle);

        void                handleOpenConnection(const connection_h& handle);
        void                handleCloseConnection(const connection_h& handle);
        void                handleMessage(const connection_h& handle, const std::string& message);

        std::string                  mServerName;
        std::unique_ptr<Server_impl> mImpl;
        connection_map_t             mConnections;
        mutable LLMutex              mConnectionMutex;

        // Threading support
        std::thread                  mServerThread;        ///< Thread running the ASIO event loop
        std::atomic<bool>            mShouldStop{ false }; ///< Thread-safe stop flag
        mutable LLMutex              mThreadMutex;         ///< Mutex for thread synchronization
    };

    // Server and Connection Management
    WSServer::ptr_t findServerByName(const std::string &name) const;

    bool            addServer(const WSServer::ptr_t& server);
    bool            removeServer(const std::string &name);

    bool            startServer(const std::string &name) const;
    void            stopServer(const std::string &name) const;

    void            update();

protected:
    void            initSingleton() override;
    void            cleanupSingleton() override;

private:
    using server_map_t = std::map<std::string, WSServer::ptr_t>;

    void            stopAllServers();

    server_map_t mServers;
};
