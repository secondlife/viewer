/**
 * @file test_websocketmgr.hpp
 * @brief A WebSocket server lets programs in and turns pages away.
 *
 * $LicenseInfo:firstyear=2026&license=viewerlgpl$
 * Alchemy Viewer Source Code
 * Copyright (C) 2026, Rye <rye@alchemyviewer.org>
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
 * $/LicenseInfo$
 */

#ifndef TEST_LLCORE_WEBSOCKETMGR_H_
#define TEST_LLCORE_WEBSOCKETMGR_H_

#include "llwebsocketmgr.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>

#include <chrono>
#include <istream>
#include <random>

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
    // nothing answered at all. The request is what a browser or a program
    // sends, written out, so that what is tested is what the server says.
    int upgradeFrom(U16 port, const std::string& origin)
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
        if (!origin.empty())
        {
            request += "Origin: " + origin + "\r\n";
        }
        request += "\r\n";
        boost::asio::write(socket, boost::asio::buffer(request), ec);
        if (ec)
        {
            return -1;
        }

        // The status line, or nothing within the time.
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
}

namespace tut
{
    struct WebsocketMgrTestData
    {
    };

    typedef test_group<WebsocketMgrTestData> WebsocketMgrTestGroupType;
    typedef WebsocketMgrTestGroupType::object WebsocketMgrTestObjectType;
    WebsocketMgrTestGroupType WebsocketMgrTestGroup("LLWebsocketMgr Tests");

    template<> template<>
    void WebsocketMgrTestObjectType::test<1>()
    {
        set_test_name("a program, which sends no origin, is let in; a page, which always does, is refused");

        const U16 port = unusedPort();
        if (!port)
        {
            skip("no port free to listen on");
        }
        const std::string name = "origin_test";
        LLWebsocketMgr&   manager = LLWebsocketMgr::instance();
        ensure("added", manager.addServer(std::make_shared<LLWebsocketMgr::WSServer>(name, port, true)));
        ensure("started", manager.startServer(name));

        const int program = upgradeFrom(port, std::string());
        const int page    = upgradeFrom(port, "https://example.com");
        const int opaque  = upgradeFrom(port, "null");
        const int local   = upgradeFrom(port, "http://127.0.0.1:8080");
        manager.removeServer(name);

        ensure_equals("no origin: in", program, 101);
        ensure_equals("a web page: refused", page, 403);
        ensure_equals("a page with an opaque origin, a sandboxed frame or a file: refused", opaque, 403);
        ensure_equals("a page served from this machine: refused", local, 403);
    }
}

#endif
