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

#include <websocketpp/common/connection_hdl.hpp>

#if 0
// Forward declarations
namespace websocketpp {
    namespace config {
        struct asio_client;
        struct asio;
    }
    template <typename config>
    class client;
    template <typename config>
    class server;
    class connection_hdl;
}

namespace LLCore {
    namespace WebSocket {

        /// WebSocket connection state enumeration
        enum class ConnectionState
        {
            DISCONNECTED,
            CONNECTING,
            CONNECTED,
            DISCONNECTING,
            FAILED
        };

        /// WebSocket message types
        enum class MessageType
        {
            TEXT,
            BINARY
        };

        /// WebSocket event types for callbacks
        enum class EventType
        {
            OPEN,
            CLOSE,
            MESSAGE,
            ERROR
        };

        /// Forward declarations for internal classes
        class WSConnection;
        class WSServer;

        /// Callback function types
        using EventCallback = std::function<void(EventType event, const LLUUID& connection_id, const LLSD& data)>;
        using MessageCallback = std::function<void(const LLUUID& connection_id, const std::string& message, MessageType type)>;

        /// WebSocket connection handle type
        using ConnectionHandle = LLUUID;
        using ServerHandle = LLUUID;

    } // namespace WebSocket
} // namespace LLCore
#endif


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

    class WSConnection
    {
        friend class LLWebsocketMgr;

    public:
        using ptr_t = std::shared_ptr<WSConnection>;
        WSConnection(const std::shared_ptr<WSServer> &server, const connection_h& handle):
            mConnectionHandle(handle),
            mServer(server)
        {}

        virtual ~WSConnection() = default;

        virtual void onOpen() {}
        virtual void onClose() {}
        virtual void onMessage(const std::string& message) {}

        bool         sendMessage(const std::string& message);

    private:
        connection_h mConnectionHandle;
        std::shared_ptr<WSServer> mServer; // Back-reference to the server this connection belongs to
    };

    class WSServer: public std::enable_shared_from_this<WSServer>
    {
        friend struct Server_impl;
        friend class WSConnection;
        friend class LLWebsocketMgr;

    public:
        using ptr_t = std::shared_ptr<WSServer>;

        WSServer(std::string_view name, U16 port, bool local_only = true);
        virtual ~WSServer() = default;

        virtual void    onConnectionOpened(const WSConnection::ptr_t& connection) { }
        virtual void    onConnectionClosed(const WSConnection::ptr_t& connection) { }

        bool            isRunning() const;

        void            broadcastMessage(const std::string& message);
    protected:
        virtual WSConnection::ptr_t connectionFactory(WSServer::ptr_t server, connection_h handle);

        bool            start();
        void            stop();

        bool            sendMessageTo(const connection_h& handle, const std::string& message);

    private:
        using connection_map_t = std::map<connection_h, WSConnection::ptr_t, std::owner_less<connection_h> >;

        WSConnection::ptr_t getConnection(const connection_h& handle);

        void                handleOpenConnection(const connection_h& handle);
        void                handleCloseConnection(const connection_h& handle);
        void                handleMessage(const connection_h& handle, const std::string& message);

        std::string                  mServerName;
        std::unique_ptr<Server_impl> mImpl;
        connection_map_t             mConnections;
        LLMutex                      mConnectionMutex;
    };

    // Server and Connection Management
    WSServer::ptr_t findServerByName(const std::string &name) const;

    bool            addServer(const WSServer::ptr_t& server);
    bool            removeServer(const std::string &name);

    bool            startServer(const std::string &name) const;
    void            stopServer(const std::string &name) const;

protected:
    void            initSingleton() override;
    void            cleanupSingleton() override;

private:
    using server_map_t = std::map<std::string, WSServer::ptr_t>;

    void            stopAllServers();

    server_map_t mServers;
};
