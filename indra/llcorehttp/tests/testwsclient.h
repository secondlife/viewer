/**
 * @file testwsclient.h
 * @brief Minimal WebSocket client for driving a server under test
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

#ifndef LL_TESTWSCLIENT_H
#define LL_TESTWSCLIENT_H

// MSVC does not report __cplusplus >= 201103L without /Zc:__cplusplus, so
// websocketpp's own C++11 detection would otherwise fall back to
// boost::random_device, which nothing here links.
#define _WEBSOCKETPP_CPP11_RANDOM_DEVICE_

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// A minimal WebSocket client for driving a server under test: connects,
// optionally with an Origin header, and can send and collect messages.
class TestWSClient
{
public:
    using client_t = websocketpp::client<websocketpp::config::asio_client>;

    TestWSClient()
    {
        mClient.clear_access_channels(websocketpp::log::alevel::all);
        mClient.clear_error_channels(websocketpp::log::elevel::all);
        mClient.init_asio();

        mClient.set_open_handler([this](websocketpp::connection_hdl hdl)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mHandle = hdl;
            mOpened = true;
            mCondVar.notify_all();
        });
        mClient.set_fail_handler([this](websocketpp::connection_hdl)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mFailed = true;
            mCondVar.notify_all();
        });
        mClient.set_close_handler([this](websocketpp::connection_hdl)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mClosed = true;
            mCondVar.notify_all();
        });
        mClient.set_message_handler([this](websocketpp::connection_hdl, client_t::message_ptr msg)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            mReceived.push_back(msg->get_payload());
            mCondVar.notify_all();
        });
    }

    ~TestWSClient()
    {
        close();
        mClient.stop();
        if (mThread.joinable())
        {
            mThread.join();
        }
    }

    // Initiates a close and waits for the ack, so teardown looks like a
    // normal closure instead of a dropped socket. Safe to call more than
    // once, and safe if the connection never opened.
    void close(std::chrono::seconds timeout = std::chrono::seconds(2))
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (!mOpened || mClosed)
        {
            return;
        }
        websocketpp::lib::error_code ec;
        mClient.close(mHandle, websocketpp::close::status::normal, "test done", ec);
        mCondVar.wait_for(lock, timeout, [this]() { return mClosed; });
    }

    // Connects to 127.0.0.1:port, optionally with an Origin header, and
    // waits up to timeout for the handshake to open or fail.
    // force_origin_header sends "Origin:" with an empty value instead of
    // omitting the header entirely, for origin.empty().
    bool connect(U16 port, const std::string& origin, bool force_origin_header = false,
                 std::chrono::seconds timeout = std::chrono::seconds(5))
    {
        websocketpp::lib::error_code ec;
        client_t::connection_ptr con = mClient.get_connection(
            "ws://127.0.0.1:" + std::to_string(port) + "/", ec);
        if (ec)
        {
            return false;
        }
        if (!origin.empty() || force_origin_header)
        {
            con->append_header("Origin", origin);
        }
        mClient.connect(con);
        mThread = std::thread([this]() { mClient.run(); });

        std::unique_lock<std::mutex> lock(mMutex);
        return mCondVar.wait_for(lock, timeout, [this]() { return mOpened || mFailed; }) && mOpened;
    }

    bool send(const std::string& message)
    {
        websocketpp::lib::error_code ec;
        mClient.send(mHandle, message, websocketpp::frame::opcode::text, ec);
        return !ec;
    }

    // Waits up to timeout for at least one message to arrive, then
    // removes and returns it; empty where none arrived in time.
    std::string waitForMessage(std::chrono::seconds timeout = std::chrono::seconds(5))
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (!mCondVar.wait_for(lock, timeout, [this]() { return !mReceived.empty(); }))
        {
            return std::string();
        }
        std::string message = mReceived.front();
        mReceived.erase(mReceived.begin());
        return message;
    }

    bool isOpen() const { return mOpened && !mClosed; }

private:
    client_t                    mClient;
    websocketpp::connection_hdl mHandle;
    std::thread                 mThread;
    std::mutex                  mMutex;
    std::condition_variable     mCondVar;
    bool                        mOpened{ false };
    bool                        mFailed{ false };
    bool                        mClosed{ false };
    std::vector<std::string>    mReceived;
};

#endif // LL_TESTWSCLIENT_H
