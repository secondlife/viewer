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

#include <thread>
#include <atomic>
#include <chrono>

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
/**
 * @struct Server_impl
 * @brief Internal implementation wrapper for websocketpp server functionality
 *
 * This structure serves as a PIMPL (Pointer to Implementation) wrapper around
 * the websocketpp::server template, providing a clean interface between the
 * high-level WSServer class and the low-level websocketpp library. It handles
 * all direct websocketpp interactions including server lifecycle management,
 * event handling, and connection management.
 *
 * The Server_impl follows Linden Lab conventions while maintaining compatibility
 * with the websocketpp library. It provides thread-safe operations where possible
 * and integrates with the existing logging infrastructure.
 *
 * @note This class uses the websocketpp::config::asio configuration which provides
 * ASIO-based networking without TLS/SSL support.
 */
struct Server_impl
{
    /**
     * @brief Constructor - Initializes the websocketpp server with configuration
     * @param owner Pointer to the owning WSServer instance (must not be null)
     * @param port The port number to bind the server to (1-65535)
     * @param local_only If true, binds only to localhost; if false, binds to all interfaces
     *
     * Sets up the websocketpp server instance and registers lambda-based event handlers
     * for connection open, close, and message events. The handlers delegate back to the
     * owning WSServer instance for processing, maintaining the abstraction layer.
     *
     * @warning The owner pointer must remain valid for the lifetime of this object
     * @pre port must be a valid port number (typically > 1024 for non-privileged access)
     */
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

    /**
     * @brief Initialize the websocketpp server and configure listening
     *
     * Performs the initial setup of the websocketpp server by calling init_asio()
     * to initialize the ASIO networking layer, then configures the server to listen
     * on the specified port. The binding behavior depends on the mLocalOnly flag:
     * - If mLocalOnly is true: binds to "127.0.0.1" (localhost only)
     * - If mLocalOnly is false: binds to all available network interfaces
     *
     * @note This method must be called before attempting to start the server
     * @pre The server must not already be initialized
     * @post The server is ready to accept connections when start() is called
     */
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

    /**
     * @brief Start the websocket server and begin accepting connections
     * @return true if server started successfully, false on error
     *
     * Runs a controlled event loop that periodically checks the stop flag for clean shutdown.
     * Instead of calling run() once and blocking indefinitely, this implementation uses
     * run_for() with a timeout to process events in chunks, checking mOwner->mShouldStop between
     * iterations to allow for responsive termination.
     *
     * @note This method blocks the calling thread until the server stops
     * @pre init() must have been called successfully
     * @post On success, the server is actively accepting connections
     * @warning Any exceptions during startup are caught and logged as warnings
     */
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

            // Run controlled event loop with periodic stop flag checking
            while (!mOwner->mShouldStop && !mServer.stopped())
            {
                // Process events for up to 100ms, then check the stop flag
                std::chrono::milliseconds timeout(100);
                std::size_t handlers_run = mServer.get_io_service().run_for(timeout);

                // If no handlers were run and the server isn't stopped,
                // reset the io_service for the next iteration
                if (handlers_run == 0 && !mServer.stopped() && !mOwner->mShouldStop)
                {
                    mServer.get_io_service().restart();
                }
            }

            LL_INFOS("WebSocket") << "WebSocket server event loop exited cleanly" << LL_ENDL;
            return true;
        }
        catch (const websocketpp::exception& e)
        {
            LL_WARNS("WebSocket") << "WebSocket server exception: " << e.what() << LL_ENDL;
            return false;
        }
        catch (const std::exception& e)
        {
            LL_WARNS("WebSocket") << "WebSocket server std::exception: " << e.what() << LL_ENDL;
            return false;
        }
        catch (...)
        {
            LL_WARNS("WebSocket") << "WebSocket server unknown exception" << LL_ENDL;
            return false;
        }
    }

    /**
     * @brief Stop the websocket server and cease accepting new connections
     *
     * Gracefully shuts down the server by first stopping the listener to prevent
     * new connections, then stopping the ASIO event loop. Existing connections
     * may remain active briefly during the shutdown process.
     *
     * The method performs a safe shutdown by checking if the server is already
     * stopped before attempting shutdown operations. Any exceptions during shutdown
     * are caught and logged but do not propagate.
     *
     * @note This method is non-blocking and safe to call multiple times
     * @post The server will no longer accept new connections
     * @post Existing connections will be cleanly terminated
     */
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

    /**
     * @brief Handle new connection establishment event
     * @param hdl WebSocket connection handle from websocketpp
     *
     * Called automatically by the websocketpp library when a new client connection
     * is successfully established. This method serves as a bridge between the
     * low-level websocketpp callback and the high-level WSServer interface.
     *
     * @note This is an internal callback method called by websocketpp
     * @pre mOwner must be valid (assertion will fail if null)
     * @post The owning WSServer will be notified of the new connection
     */
    void onOpen(websocketpp::connection_hdl hdl) const
    {
        LL_ERRS_IF(!mOwner, "WebSocket") << "mOwner should never be null. If it is, something is very wrong!" << LL_ENDL;

        mOwner->handleOpenConnection(hdl);
    }

    /**
     * @brief Handle connection closure event
     * @param hdl WebSocket connection handle from websocketpp
     *
     * Called automatically by the websocketpp library when a client connection
     * is closed, either by the client, server, or due to a network error.
     * This method delegates the event to the owning WSServer for processing.
     *
     * @note This is an internal callback method called by websocketpp
     * @pre mOwner must be valid (assertion will fail if null)
     * @post The owning WSServer will be notified of the connection closure
     */
    void onClose(websocketpp::connection_hdl hdl) const
    {
        LL_ERRS_IF(!mOwner, "WebSocket") << "mOwner should never be null" << LL_ENDL;
        mOwner->handleCloseConnection(hdl);
    }

    /**
     * @brief Handle incoming message from client
     * @param hdl WebSocket connection handle identifying the sender
     * @param msg Shared pointer to the message object containing payload and metadata
     *
     * Called automatically by the websocketpp library when a complete message is
     * received from a client. This method validates the connection exists, extracts
     * the message payload, and forwards it to the appropriate connection handler.
     *
     * Currently handles text messages only. Binary message support and message
     * fragmentation handling are noted as TODO items for future implementation.
     *
     * @note This is an internal callback method called by websocketpp
     * @pre mOwner must be valid (assertion will fail if null)
     * @post If connection exists, the message is forwarded for processing
     * @todo Add support for binary messages
     * @todo Implement fragmented message handling
     * @todo Process connection close codes for graceful closure
     */
    void onMessage(websocketpp::connection_hdl hdl, Server_t::message_ptr msg) const
    {
        LL_ERRS_IF(!mOwner, "WebSocket") << "mOwner should never be null" << LL_ENDL;
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
    Server_t                    mServer;       ///< The underlying websocketpp server instance
    LLWebsocketMgr::WSServer*   mOwner{ nullptr }; ///< Back-reference to the owning WSServer instance (guaranteed non-null)
    U16                         mPort{ 0 };    ///< TCP port number the server listens on
    bool                        mLocalOnly{ true }; ///< Whether to bind to localhost only (true) or all interfaces (false)
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

LLWebsocketMgr::WSServer::~WSServer()
{
    // Ensure the server is stopped before destruction
    stop();
}

LLWebsocketMgr::WSConnection::ptr_t LLWebsocketMgr::WSServer::connectionFactory(LLWebsocketMgr::WSServer::ptr_t server,
    LLWebsocketMgr::connection_h handle)
{
    return std::make_shared<LLWebsocketMgr::WSConnection>(server, handle);
}

bool LLWebsocketMgr::WSServer::start()
{
    LL_ERRS_IF(!mImpl, "WebSocket") << "WebSocket server " << mServerName << " implementation is null !" << LL_ENDL;

    LLMutexLock lock(&mThreadMutex);

    // Check if already running
    if (isRunning())
    {
        LL_WARNS("WebSocket") << "Server " << mServerName << " is already running" << LL_ENDL;
        return false;
    }

    // Reset the stop flag
    mShouldStop = false;

    // Start the server thread
    mServerThread = std::thread([this]() {
        LL_INFOS("WebSocket") << "WebSocket server thread starting for: " << mServerName << LL_ENDL;

        // Run the controlled server loop that checks the stop flag
        // Server_impl accesses mShouldStop through the mOwner pointer
        bool success = mImpl->start();

        if (!success)
        {
            LL_WARNS("WebSocket") << "WebSocket server thread failed to start for: " << mServerName << LL_ENDL;
        }

        LL_INFOS("WebSocket") << "WebSocket server thread exiting for: " << mServerName << LL_ENDL;
    });

    LL_INFOS("WebSocket") << "Started WebSocket server thread: " << mServerName << LL_ENDL;
    return true;
}

void LLWebsocketMgr::WSServer::stop()
{
    LL_ERRS_IF(!mImpl, "WebSocket") << "WebSocket server " << mServerName << " implementation is null !" << LL_ENDL;

    {
        LLMutexLock lock(&mThreadMutex);

        // Check if already stopped
        if (!isRunning())
        {
            return;
        }

        LL_INFOS("WebSocket") << "Stopping WebSocket server: " << mServerName << LL_ENDL;

        // Signal the thread to stop
        mShouldStop = true;

        // Stop the websocket server (this will cause the controlled run loop to exit)
        mImpl->stop();
    } // Release the lock here

    // Wait for the thread to finish (outside the lock to avoid deadlock)
    if (mServerThread.joinable())
    {
        mServerThread.join();
        LL_INFOS("WebSocket") << "WebSocket server thread joined for: " << mServerName << LL_ENDL;
    }
}

bool LLWebsocketMgr::WSServer::isRunning() const
{
    LL_ERRS_IF(!mImpl, "WebSocket") << "WebSocket server " << mServerName << " implementation is null !" << LL_ENDL;

    // Check both the thread state, websocket server state, and the stop flag
    return mServerThread.joinable() && !mImpl->mServer.stopped() && !mShouldStop;
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

bool LLWebsocketMgr::WSServer::closeConnection(const connection_h& handle, U16 code, const std::string& reason)
{
    LL_ERRS_IF(!mImpl, "WebSocket") << "WebSocket server " << mServerName << " implementation is null !" << LL_ENDL;

    try
    {
        websocketpp::lib::error_code ec;
        mImpl->mServer.close(handle, code, reason, ec);
        if (ec)
        {
            LL_WARNS("WebSocket") << mServerName << " failed to close connection: " << ec.message() << LL_ENDL;
            return false;
        }

        LL_INFOS("WebSocket") << mServerName << " initiated close for connection with code "
                              << code << " and reason: " << reason << LL_ENDL;
        return true;
    }
    catch (const websocketpp::exception& e)
    {
        LL_WARNS("WebSocket") << mServerName << " exception closing connection: " << e.what() << LL_ENDL;
        return false;
    }
    catch (const std::exception& e)
    {
        LL_WARNS("WebSocket") << mServerName << " std::exception closing connection: " << e.what() << LL_ENDL;
        return false;
    }
    catch (...)
    {
        LL_WARNS("WebSocket") << mServerName << " unknown exception closing connection" << LL_ENDL;
        return false;
    }
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

void LLWebsocketMgr::WSConnection::closeConnection(U16 code, const std::string& reason)
{
    if (!mServer)
    {
        LL_WARNS("WebSocket") << "Attempted to close connection with null server reference" << LL_ENDL;
        return;
    }

    LL_INFOS("WebSocket") << "WSConnection closing connection with code " << code
                          << " and reason: " << (reason.empty() ? "(no reason)" : reason) << LL_ENDL;

    if (!mServer->closeConnection(mConnectionHandle, code, reason))
    {
        LL_WARNS("WebSocket") << "Failed to close connection through server" << LL_ENDL;
    }
}
