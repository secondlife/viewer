/**
 * @file wsserver_test.cpp
 * @brief Security-focused unit tests for LLWebsocketMgr::WSServer
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2026, Linden Research, Inc.
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

#include "testwsclient.h"
#include "../test/lltut.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>

#include <atomic>
#include <chrono>
#include <istream>
#include <random>
#include <thread>

namespace
{
    // A port nothing on this machine is listening on, as far as can be
    // told; zero where none turned up.
    U16 unusedPort()
    {
        std::random_device random;
        for (int tries = 0; tries < 20; ++tries)
        {
            const U16                      port = static_cast<U16>(40000 + random() % 20000);
            boost::asio::io_context        io;
            boost::asio::ip::tcp::acceptor acceptor(io);
            boost::system::error_code      ec;
            acceptor.open(boost::asio::ip::tcp::v4(), ec);
            if (!ec)
            {
                acceptor.bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), port), ec);
            }
            if (!ec)
            {
                return port;
            }
        }
        return 0;
    }

    // The HTTP status a WebSocket upgrade request from this origin -- none,
    // where empty -- is answered with: 101 where it would open, -1 where
    // nothing answered at all.
    // force_origin_header sends "Origin:" with an empty value instead of
    // omitting the header entirely, for origin.empty().
    int upgradeFrom(U16 port, const std::string& origin, bool force_origin_header = false)
    {
        boost::asio::io_context      io;
        boost::asio::ip::tcp::socket socket(io);
        boost::system::error_code    ec;
        socket.connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), port), ec);
        if (ec)
        {
            return -1;
        }

        std::string request = "GET / HTTP/1.1\r\n"
                              "Host: 127.0.0.1:" + std::to_string(port) + "\r\n"
                              "Upgrade: websocket\r\n"
                              "Connection: Upgrade\r\n"
                              "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                              "Sec-WebSocket-Version: 13\r\n";
        if (!origin.empty() || force_origin_header)
        {
            request += "Origin: " + origin + "\r\n";
        }
        request += "\r\n";
        boost::asio::write(socket, boost::asio::buffer(request), ec);
        if (ec)
        {
            return -1;
        }

        boost::asio::streambuf response;
        bool                   answered = false;
        boost::asio::async_read_until(socket, response, "\r\n",
                                      [&](const boost::system::error_code& error, std::size_t) { answered = !error; });
        io.run_for(std::chrono::seconds(10));
        socket.close(ec);
        if (!answered)
        {
            return -1;
        }
        std::istream line(&response);
        std::string  version;
        int          status = -1;
        line >> version >> status;
        return status;
    }

    // Polls a predicate until it is true or the deadline passes. A client
    // seeing its own handshake complete says nothing about whether the
    // server's own thread has finished its side of onOpen bookkeeping yet.
    template <typename Predicate>
    bool waitUntil(Predicate predicate, std::chrono::milliseconds timeout = std::chrono::milliseconds(2000))
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (!predicate())
        {
            if (std::chrono::steady_clock::now() >= deadline)
            {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return true;
    }

    // The machine's own non-loopback IPv4 address, as seen when routing
    // toward the internet (no packet is actually sent); empty where none
    // could be found, such as a loopback-only sandbox.
    std::string localNonLoopbackAddress()
    {
        boost::asio::io_context      io;
        boost::asio::ip::udp::socket socket(io);
        boost::system::error_code    ec;
        socket.connect(boost::asio::ip::udp::endpoint(boost::asio::ip::make_address("8.8.8.8"), 53), ec);
        if (ec)
        {
            return std::string();
        }
        return socket.local_endpoint(ec).address().to_string();
    }

    // Whether a TCP connection to host:port succeeds within a short time;
    // refused or silently dropped are both treated as "no".
    bool canConnect(const std::string& host, U16 port)
    {
        boost::asio::io_context      io;
        boost::asio::ip::tcp::socket socket(io);
        boost::system::error_code    result = boost::asio::error::would_block;
        socket.async_connect(
            boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(host), port),
            [&](const boost::system::error_code& ec) { result = ec; });
        io.run_for(std::chrono::seconds(3));
        boost::system::error_code close_ec;
        socket.close(close_ec);
        return !result;
    }

    // Records what a server actually did, so tests can assert on lifecycle
    // callbacks the base class leaves as no-ops.
    class TestWSServer : public LLWebsocketMgr::WSServer
    {
    public:
        using LLWebsocketMgr::WSServer::WSServer;

        std::atomic<int> mOpenedCount{ 0 };
        std::atomic<int> mClosedCount{ 0 };

        // Empty where the base policy (reject any Origin) should apply;
        // otherwise the one origin this test server lets in.
        std::string mAllowedOrigin;

        void onConnectionOpened(const LLWebsocketMgr::WSConnection::ptr_t&) override
        {
            ++mOpenedCount;
        }

        void onConnectionClosed(const LLWebsocketMgr::WSConnection::ptr_t&) override
        {
            ++mClosedCount;
        }

        bool acceptOrigin(const std::string& origin) const override
        {
            if (!mAllowedOrigin.empty() && origin == mAllowedOrigin)
            {
                return true;
            }
            return WSServer::acceptOrigin(origin);
        }
    };
}

namespace tut
{
    struct wsserver_data
    {
        U16         mPort;
        std::string mName;

        wsserver_data() :
            mPort(unusedPort()),
            mName("wsserver_test")
        {
        }

        ~wsserver_data()
        {
            LLWebsocketMgr::instance().removeServer(mName);
        }
    };

    typedef test_group<wsserver_data> wsserver_test;
    typedef wsserver_test::object wsserver_object;
    wsserver_test wsserver_testcase("LLWebsocketMgr::WSServer");

    template<> template<>
    void wsserver_object::test<1>()
    {
        set_test_name("a program, which sends no Origin header, is let in");

        if (!mPort)
        {
            skip("no port free to listen on");
        }

        LLWebsocketMgr& manager = LLWebsocketMgr::instance();
        auto server = std::make_shared<LLWebsocketMgr::WSServer>(mName, mPort, true);
        ensure("added", manager.addServer(server));
        ensure("started", manager.startServer(mName));

        ensure_equals("no origin: connection opens", upgradeFrom(mPort, std::string()), 101);
        ensure("server clears the abruptly closed raw connection", waitUntil([&]() { return server->getConnectionCount() == 0; }));

        TestWSClient client;
        ensure("no origin: client connects over the network", client.connect(mPort, std::string()));
        ensure("server sees the open connection", waitUntil([&]() { return server->getConnectionCount() == 1; }));

        client.close();
        manager.removeServer(mName);
    }

    template<> template<>
    void wsserver_object::test<2>()
    {
        set_test_name("a page, which sends an Origin header, is refused");

        if (!mPort)
        {
            skip("no port free to listen on");
        }

        LLWebsocketMgr& manager = LLWebsocketMgr::instance();
        auto server = std::make_shared<LLWebsocketMgr::WSServer>(mName, mPort, true);
        ensure("added", manager.addServer(server));
        ensure("started", manager.startServer(mName));

        ensure_equals("a web page: refused", upgradeFrom(mPort, "https://example.com"), 403);

        TestWSClient client;
        ensure("a web page: client never opens over the network", !client.connect(mPort, "https://example.com"));
        ensure_equals("server never sees a connection", server->getConnectionCount(), (size_t)0);

        manager.removeServer(mName);
    }

    template<> template<>
    void wsserver_object::test<3>()
    {
        set_test_name("a page with an opaque origin (a sandboxed frame or a file) is refused");

        if (!mPort)
        {
            skip("no port free to listen on");
        }

        LLWebsocketMgr& manager = LLWebsocketMgr::instance();
        auto server = std::make_shared<LLWebsocketMgr::WSServer>(mName, mPort, true);
        ensure("added", manager.addServer(server));
        ensure("started", manager.startServer(mName));

        ensure_equals("an opaque origin: refused", upgradeFrom(mPort, "null"), 403);

        TestWSClient client;
        ensure("an opaque origin: client never opens over the network", !client.connect(mPort, "null"));
        ensure_equals("server never sees a connection", server->getConnectionCount(), (size_t)0);

        manager.removeServer(mName);
    }

    template<> template<>
    void wsserver_object::test<4>()
    {
        set_test_name("a page served from this machine is refused");

        if (!mPort)
        {
            skip("no port free to listen on");
        }

        LLWebsocketMgr& manager = LLWebsocketMgr::instance();
        auto server = std::make_shared<LLWebsocketMgr::WSServer>(mName, mPort, true);
        ensure("added", manager.addServer(server));
        ensure("started", manager.startServer(mName));

        ensure_equals("a page on this machine: refused", upgradeFrom(mPort, "http://127.0.0.1:8080"), 403);

        TestWSClient client;
        ensure("a page on this machine: client never opens over the network", !client.connect(mPort, "http://127.0.0.1:8080"));
        ensure_equals("server never sees a connection", server->getConnectionCount(), (size_t)0);

        manager.removeServer(mName);
    }

    template<> template<>
    void wsserver_object::test<5>()
    {
        set_test_name("an Origin header present but empty is treated the same as none");

        if (!mPort)
        {
            skip("no port free to listen on");
        }

        LLWebsocketMgr& manager = LLWebsocketMgr::instance();
        auto server = std::make_shared<LLWebsocketMgr::WSServer>(mName, mPort, true);
        ensure("added", manager.addServer(server));
        ensure("started", manager.startServer(mName));

        ensure_equals("empty origin: connection opens", upgradeFrom(mPort, std::string(), true), 101);
        ensure("server clears the abruptly closed raw connection", waitUntil([&]() { return server->getConnectionCount() == 0; }));

        TestWSClient client;
        ensure("empty origin: client connects over the network", client.connect(mPort, std::string(), true));
        ensure("server sees the open connection", waitUntil([&]() { return server->getConnectionCount() == 1; }));

        client.close();
        manager.removeServer(mName);
    }

    template<> template<>
    void wsserver_object::test<7>()
    {
        set_test_name("local_only binds to loopback only; the machine's own LAN address is unreachable");

        if (!mPort)
        {
            skip("no port free to listen on");
        }

        const std::string lan_address = localNonLoopbackAddress();
        if (lan_address.empty())
        {
            skip("no non-loopback network interface to test against");
        }

        LLWebsocketMgr& manager = LLWebsocketMgr::instance();
        auto server = std::make_shared<LLWebsocketMgr::WSServer>(mName, mPort, true);
        ensure("added", manager.addServer(server));
        ensure("started", manager.startServer(mName));

        ensure("loopback: reachable", canConnect("127.0.0.1", mPort));
        ensure("LAN address: unreachable", !canConnect(lan_address, mPort));

        manager.removeServer(mName);
    }

    template<> template<>
    void wsserver_object::test<8>()
    {
        set_test_name("a refused connection never reaches onConnectionOpened, and an accepted one does");

        if (!mPort)
        {
            skip("no port free to listen on");
        }

        LLWebsocketMgr& manager = LLWebsocketMgr::instance();
        auto server = std::make_shared<TestWSServer>(mName, mPort, true);
        ensure("added", manager.addServer(server));
        ensure("started", manager.startServer(mName));

        ensure_equals("a web page: refused", upgradeFrom(mPort, "https://example.com"), 403);
        ensure_equals("refused: onConnectionOpened never fires", server->mOpenedCount.load(), 0);

        TestWSClient client;
        ensure("no origin: client connects over the network", client.connect(mPort, std::string()));
        ensure("accepted: onConnectionOpened fires once", waitUntil([&]() { return server->mOpenedCount.load() == 1; }));

        client.close();
        manager.removeServer(mName);
    }

    template<> template<>
    void wsserver_object::test<9>()
    {
        set_test_name("multiple simultaneous connections are each tracked independently");

        if (!mPort)
        {
            skip("no port free to listen on");
        }

        LLWebsocketMgr& manager = LLWebsocketMgr::instance();
        auto server = std::make_shared<TestWSServer>(mName, mPort, true);
        ensure("added", manager.addServer(server));
        ensure("started", manager.startServer(mName));

        TestWSClient client_a;
        TestWSClient client_b;
        TestWSClient client_c;
        ensure("first client connects", client_a.connect(mPort, std::string()));
        ensure("second client connects", client_b.connect(mPort, std::string()));
        ensure("third client connects", client_c.connect(mPort, std::string()));

        ensure("server counts all three connections", waitUntil([&]() { return server->getConnectionCount() == 3; }));
        ensure("onConnectionOpened fires once per connection", waitUntil([&]() { return server->mOpenedCount.load() == 3; }));

        client_a.close();
        client_b.close();
        client_c.close();
        manager.removeServer(mName);
    }
}
