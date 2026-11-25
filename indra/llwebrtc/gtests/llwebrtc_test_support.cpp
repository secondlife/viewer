/**
 * @file llwebrtc_test_support.cpp
 * @brief Global llwebrtc init/terminate for the unit tests, plus log capture.
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

#include "llwebrtc_test_support.h"

#include <algorithm>
#include <iostream>
#include <optional>

#if defined(_WIN32)
#include <winsock2.h>
#endif

namespace llwebrtc_test
{

void RecordingLogCallback::LogMessage(LogLevel level, const std::string& message)
{
    // Echo problems to the console so they show up next to a failing test.
    if (level >= LOG_LEVEL_WARNING)
    {
        std::cerr << "[llwebrtc " << (level == LOG_LEVEL_ERROR ? "ERROR" : "WARNING") << "] " << message;
        if (message.empty() || message.back() != '\n')
        {
            std::cerr << '\n';
        }
    }

    std::lock_guard<std::mutex> lock(mMutex);
    mEntries.push_back({ level, message });
}

std::vector<RecordingLogCallback::Entry> RecordingLogCallback::entries() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mEntries;
}

bool RecordingLogCallback::contains(const std::string& text) const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return std::any_of(mEntries.begin(), mEntries.end(),
                       [&text](const Entry& entry) { return entry.mMessage.find(text) != std::string::npos; });
}

RecordingLogCallback& logCallback()
{
    // Intentionally leaked: webrtc threads may still log during static
    // destruction if terminate() timed out and detached its shutdown thread.
    static RecordingLogCallback* sLogCallback = new RecordingLogCallback;
    return *sLogCallback;
}

bool startNetworking()
{
#if defined(_WIN32)
    // llwebrtc doesn't initialize Winsock itself; in the viewer it has
    // already been started by the time voice comes up.  Without it webrtc
    // can't create sockets.  WSAStartup is reference counted, so this pairs
    // with stopNetworking() and may be nested.
    WSADATA wsa_data;
    return WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0;
#else
    return true;
#endif
}

void stopNetworking()
{
#if defined(_WIN32)
    WSACleanup();
#endif
}

TestServer::TestServer() : mPeer(llwebrtc_testpeer::createTestPeer(this))
{
}

TestServer::~TestServer()
{
    if (mPeer)
    {
        llwebrtc_testpeer::destroyTestPeer(mPeer);
    }
}

testing::AssertionResult TestServer::waitForAnswer(std::string& answer)
{
    std::optional<std::pair<bool, std::string>> result = mAnswer.wait();
    if (!result)
    {
        return testing::AssertionFailure() << "the test server never answered the offer";
    }
    if (!result->first)
    {
        return testing::AssertionFailure() << "the test server couldn't answer the offer: " << result->second;
    }
    answer = result->second;
    return testing::AssertionSuccess();
}

testing::AssertionResult negotiateAnswer(llwebrtc::LLWebRTCPeerConnectionInterface* connection,
                                         RecordingSignalingObserver& observer,
                                         TestServer& server,
                                         std::string& answer)
{
    if (!connection)
    {
        return testing::AssertionFailure() << "no llwebrtc connection";
    }
    if (!server.peer())
    {
        return testing::AssertionFailure() << "the test server couldn't be created";
    }
    if (!connection->initializeConnection(llwebrtc::LLWebRTCPeerConnectionInterface::InitOptions()))
    {
        return testing::AssertionFailure() << "initializeConnection() failed";
    }

    std::optional<std::string> offer = observer.mOffer.wait();
    if (!offer)
    {
        return testing::AssertionFailure() << "llwebrtc never produced an offer";
    }

    server.peer()->answerOffer(*offer);
    return server.waitForAnswer(answer);
}

testing::AssertionResult connectToServer(llwebrtc::LLWebRTCPeerConnectionInterface* connection,
                                         RecordingSignalingObserver& observer,
                                         TestServer& server)
{
    std::string answer;
    testing::AssertionResult negotiated = negotiateAnswer(connection, observer, server, answer);
    if (!negotiated)
    {
        return negotiated;
    }

    connection->AnswerAvailable(answer);

    // These need a working network path between llwebrtc and the test peer
    // on this machine; llwebrtc ignores loopback interfaces, so a machine
    // with no network adapter at all can't connect.
    if (!observer.mAudioEstablished.waitFor(1))
    {
        return testing::AssertionFailure()
            << "llwebrtc never reported audio established (connection didn't reach connected); "
            << "renegotiation requests: " << observer.mRenegotiationNeeded.count();
    }
    if (!observer.mDataChannelReady.waitFor(1))
    {
        return testing::AssertionFailure() << "llwebrtc's data channel never opened";
    }
    if (!server.mDataChannelOpen.waitFor(1))
    {
        return testing::AssertionFailure() << "the test server never saw the data channel open";
    }
    return testing::AssertionSuccess();
}

} // namespace llwebrtc_test
