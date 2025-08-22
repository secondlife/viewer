/**
 * @file llwebsocketmgr.cpp
 * @brief WebSocket manager singleton implementation
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

#include "llwebsocketmgr.h"
#include "llerror.h"
#include "llsdserialize.h"
#include "llhost.h"

#include <websocketpp/config/boost_config.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/server.hpp>

//------------------------------------------------------------------------
namespace
{
    using Server_t      = websocketpp::server<websocketpp::config::asio>;
    using Client_t      = websocketpp::client<websocketpp::config::asio>;
    using Connection_t  = websocketpp::connection<websocketpp::config::asio>;
}

//------------------------------------------------------------------------

//------------------------------------------------------------------------
// LLWebsocketMgr Implementation
//

void LLWebsocketMgr::initSingleton()
{ }

void LLWebsocketMgr::cleanupSingleton()
{
    stopAllServers();
}

LLWebsocketMgr::WSServer::ptr_t LLWebsocketMgr::findServerByName(const std::string &name) const
{
    auto it = mServers.find(std::string(name));
    if (it != mServers.end())
    {
        return it->second;
    }
    return nullptr;
}

bool LLWebsocketMgr::addServer(const LLWebsocketMgr::WSServer::ptr_t& server)
{
    if (!server)
    {
        LL_WARNS("WebSocket") << "Attempted to add a null server" << LL_ENDL;
        return false;
    }

    auto it = mServers.find(server->mServerName);
    if (it != mServers.end())
    {
        LL_WARNS("WebSocket") << "Server with name " << server->mServerName << " already exists" << LL_ENDL;
        return false;
    }
    mServers[server->mServerName] = server;
    LL_INFOS("WebSocket") << "Added WebSocket server: " << server->mServerName << LL_ENDL;
    return true;
}

bool LLWebsocketMgr::removeServer(const std::string& name)
{
    auto it = mServers.find(name);
    if (it == mServers.end())
    {
        LL_WARNS("WebSocket") << "No server found with name " << name << " to remove" << LL_ENDL;
        return false;
    }
    if (it->second && it->second->isRunning())
        it->second->stop();
    mServers.erase(it);
    LL_INFOS("WebSocket") << "Removed WebSocket server: " << name << LL_ENDL;
    return true;
}


bool LLWebsocketMgr::startServer(const std::string &name) const
{
    LLWebsocketMgr::WSServer::ptr_t server = findServerByName(name);
    if (!server)
    {
        LL_WARNS("WebSocket") << "No server found with name " << name << " to start" << LL_ENDL;
        return false;
    }
    if (server->isRunning())
    {
        LL_WARNS("WebSocket") << "Server " << name << " is already running" << LL_ENDL;
        return false;
    }
    return server->start();
}

void LLWebsocketMgr::stopServer(const std::string& name) const
{
    LLWebsocketMgr::WSServer::ptr_t server = findServerByName(name);
    if (!server)
    {
        LL_WARNS("WebSocket") << "No server found with name " << name << " to stop" << LL_ENDL;
        return;
    }
    if (!server->isRunning())
    {
        LL_WARNS("WebSocket") << "Server " << name << " is not running" << LL_ENDL;
        return;
    }
    server->stop();
}

void LLWebsocketMgr::stopAllServers()
{
    for (auto &[name, server] : mServers)
    {
        if (server && server->isRunning())
        {
            LL_INFOS("WebSocket") << "Stopping server: " << name << LL_ENDL;
            server->stop();
        }
    }

    mServers.clear();
}


//------------------------------------------------------------------------
struct Server_impl
{
    Server_impl(LLWebsocketMgr::WSServer *owner, U16 port, bool local_only) :
        mOwner(owner),
        mPort(port),
        mLocalOnly(local_only)
    {
        mServer.set_open_handler([this](websocketpp::connection_hdl hdl) { this->onOpen(hdl); });
        mServer.set_close_handler([this](websocketpp::connection_hdl hdl) { this->onClose(hdl); });
        mServer.set_message_handler([this](websocketpp::connection_hdl hdl, Server_t::message_ptr msg) { this->onMessage(hdl, msg); });
    }

    ~Server_impl() = default;

    void init()
    {
        mServer.init_asio();
        if (mLocalOnly)
        {
            std::stringstream port_str;
            port_str << mPort;
            mServer.listen("127.0.0.1", port_str.str());
        }
        else
        {
            mServer.listen(mPort);
        }
    }

    bool start()
    {
        if (!mServer.stopped())
        {
            LL_WARNS("WebSocket") << "WebSocket server is already running" << LL_ENDL;
            return false;
        }
        try
        {
            mServer.start_accept();
            mServer.run();
        }
        catch (const std::exception& e)
        {
            LL_WARNS("WebSocket") << "WebSocket server encountered an error: " << e.what() << LL_ENDL;
            return false;
        }
        return true;
    }

    void stop()
    {
        if (mServer.stopped())
        {
            return;
        }
        try
        {
            mServer.stop_listening();
            mServer.stop();
        }
        catch (const std::exception&)
        {
            LL_WARNS("WebSocket") << "Error stopping WebSocket server" << LL_ENDL;
        }
    }

    void onOpen(websocketpp::connection_hdl hdl) const
    {
        assert(mOwner); // mOwner should never be null. If it is, something is very wrong!

        mOwner->handleOpenConnection(hdl);
    }

    void onClose(websocketpp::connection_hdl hdl) const
    {
        assert(mOwner); // mOwner should never be null
        mOwner->handleCloseConnection(hdl);
    }

    void onMessage(websocketpp::connection_hdl hdl, Server_t::message_ptr msg) const
    {
        assert(mOwner); // mOwner should never be null
        LLWebsocketMgr::WSConnection::ptr_t connection = mOwner->getConnection(hdl);
        if (!connection)
        {
            LL_WARNS("WebSocket") << "Received message for unknown connection" << LL_ENDL;
            return;
        }

        // TODO: check the FIN bit and handle fragmented messages if needed
        // TODO: check terminal and close codes and handle connection closure if needed
        // TODO: handle binary messages if needed
        mOwner->handleMessage(hdl, msg->get_payload());
    }

    //-------------------------------------------
    Server_t                    mServer;
    LLWebsocketMgr::WSServer*   mOwner{ nullptr }; // Back-reference to the owning server, note that this pointer can never be null
    U16                         mPort{ 0 };
    bool                        mLocalOnly{ true };
};

//------------------------------------------------------------------------
LLWebsocketMgr::WSServer::WSServer(std::string_view name, U16 port, bool local_only):
    mServerName(name),
    mImpl(std::make_unique<Server_impl>(this, port, local_only))
{
    mImpl->init();

    // Initialize the server with the given name and host
    LL_INFOS("WebSocket") << "Creating WebSocket server: " << name <<
        " listening " << (mImpl->mLocalOnly ? "locally" : "ON ALL INTERFACES") <<
        " on port " << mImpl->mPort << LL_ENDL;
}

LLWebsocketMgr::WSConnection::ptr_t LLWebsocketMgr::WSServer::connectionFactory(LLWebsocketMgr::WSServer::ptr_t server,
    LLWebsocketMgr::connection_h handle)
{
    return std::make_shared<LLWebsocketMgr::WSConnection>(server, handle);
}

bool LLWebsocketMgr::WSServer::start()
{
    LL_ERRS_IF(!mImpl, "WebSocket") << "WebSocket server " << mServerName << " implementation is null !" << LL_ENDL;
    return mImpl->start();
}

void LLWebsocketMgr::WSServer::stop()
{
    LL_ERRS_IF(!mImpl, "WebSocket") << "WebSocket server " << mServerName << " implementation is null !" << LL_ENDL;
    mImpl->stop();
}

bool LLWebsocketMgr::WSServer::isRunning() const
{
    LL_ERRS_IF(!mImpl, "WebSocket") << "WebSocket server " << mServerName << " implementation is null !" << LL_ENDL;
    return !mImpl->mServer.stopped();
}

void LLWebsocketMgr::WSServer::broadcastMessage(const std::string& message)
{
    LL_ERRS_IF(!mImpl, "WebSocket") << "WebSocket server " << mServerName << " implementation is null !" << LL_ENDL;
    LLMutexLock lock(&mConnectionMutex);
    for (const auto& [handle, conn] : mConnections)
    {
        sendMessageTo(handle, message);
    }
}

bool LLWebsocketMgr::WSServer::sendMessageTo(const connection_h& handle, const std::string& message)
{
    LL_ERRS_IF(!mImpl, "WebSocket") << "WebSocket server " << mServerName << " implementation is null !" << LL_ENDL;
    websocketpp::lib::error_code ec;
    mImpl->mServer.send(handle, message, websocketpp::frame::opcode::text, ec);
    if (ec)
    {
        LL_WARNS("WebSocket") << mServerName << " failed to send message: " << ec.message() << LL_ENDL;
        return false;
    }
    return true;
}

LLWebsocketMgr::WSConnection::ptr_t LLWebsocketMgr::WSServer::getConnection(const connection_h& handle)
{
    LLMutexLock lock(&mConnectionMutex);
    auto        it = mConnections.find(handle);
    if (it != mConnections.end())
    {
        return it->second;
    }
    return nullptr;
}

void LLWebsocketMgr::WSServer::handleOpenConnection(const connection_h& handle)
{
    WSConnection::ptr_t connection;
    size_t              size(0);
    {
        LLMutexLock lock(&mConnectionMutex);
        auto        it = mConnections.find(handle);
        if (it == mConnections.end())
        {
            connection = connectionFactory(shared_from_this(), handle);
            if (!connection)
            {
                LL_WARNS("WebSocket") << "Failed to create connection for websocket server " << mServerName << LL_ENDL;
                return;
            }
            mConnections[handle] = connection;
        }
        else
        {
            connection = it->second;
        }

        if (!connection)
        {
            LL_WARNS("WebSocket") << mServerName << " failed to create connection object" << LL_ENDL;
            return;
        }
        mConnections[handle] = connection;
        size = mConnections.size();
    }

    onConnectionOpened(connection); // TODO: consider letting the server reject the connection here
    connection->onOpen();
    LL_INFOS("WebSocket") << mServerName << " opened new connection, total connections: " << size << LL_ENDL;
}

void LLWebsocketMgr::WSServer::handleCloseConnection(const connection_h& handle)
{
    size_t              size(0);
    WSConnection::ptr_t connection;
    {
        LLMutexLock lock(&mConnectionMutex);
        auto        it = mConnections.find(handle);
        if (it != mConnections.end())
        {
            connection = it->second;
            mConnections.erase(it);
        }
        size = mConnections.size();
    }
    if (connection)
    {
        connection->onClose();
        onConnectionClosed(connection);
        LL_INFOS("WebSocket") << mServerName << " closed connection, total connections: " << size << LL_ENDL;
    }
    else
    {
        LL_WARNS("WebSocket") << mServerName << " attempted to close unknown connection" << LL_ENDL;
    }
}

void LLWebsocketMgr::WSServer::handleMessage(const connection_h& handle, const std::string& message)
{
    WSConnection::ptr_t connection = getConnection(handle);
    if (connection)
    {
        connection->onMessage(message);
    }
    else
    {
        LL_WARNS("WebSocket") << mServerName << " received message for unknown connection" << LL_ENDL;
    }
}

//------------------------------------------------------------------------
bool LLWebsocketMgr::WSConnection::sendMessage(const std::string& message)
{
    if (!mServer)
    {
        LL_WARNS("WebSocket") << "Attempted to send message on connection with null server reference" << LL_ENDL;
        return false;
    }
    return mServer->sendMessageTo(mConnectionHandle, message);
}
