/**
 * @file llwebrtc_testpeer.h
 * @brief A minimal WebRTC "voice server" for llwebrtc unit tests.
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

/*
 * The test peer plays the part of the Second Life WebRTC voice server so
 * llwebrtc can be tested over a real connection: it answers llwebrtc's
 * offer, completes ICE/DTLS/SCTP, echoes data channel traffic, and can be
 * told to misbehave (close abruptly, or freeze as if the server hung).
 *
 * Like llwebrtc it's a shared library wrapping the native webrtc library,
 * built with the same settings, and for the same reasons: native webrtc
 * can't be compiled into code built with the viewer's settings.  The
 * interface follows llwebrtc.h's rules, since the two sides of it use
 * different C runtimes: only standard types cross it, inputs are passed by
 * const reference (and copied on the far side), and results come back
 * through observer callbacks rather than being returned.
 *
 * Each test peer runs its own webrtc threads and uses webrtc's dummy audio
 * device, so it needs no audio hardware.  Callbacks arrive on the test
 * peer's own signaling thread.
 */

#ifndef LLWEBRTC_TESTPEER_H
#define LLWEBRTC_TESTPEER_H

#include <string>

#ifdef LL_TESTPEER_MAKEDLL
#if defined(_WIN32)
#define LLTESTPEER_API __declspec(dllexport)
#else
#define LLTESTPEER_API __attribute__((visibility("default")))
#endif
#else
#define LLTESTPEER_API /**/
#endif

namespace llwebrtc_testpeer
{

class LLWebRTCTestPeerObserver
{
public:
    typedef enum
    {
        CONNECTION_NEW = 0,
        CONNECTION_CONNECTING,
        CONNECTION_CONNECTED,
        CONNECTION_DISCONNECTED,
        CONNECTION_FAILED,
        CONNECTION_CLOSED
    } EConnectionState;

    // The answer to an offer passed to answerOffer().  It's only produced
    // once ICE gathering is complete, so it carries all of the test peer's
    // candidates: llwebrtc has no way to accept remote candidates trickled
    // after the answer, just as with the real server.
    virtual void OnAnswerAvailable(const std::string& sdp) = 0;

    // answerOffer() failed; 'error' describes why.
    virtual void OnAnswerFailed(const std::string& error) = 0;

    virtual void OnConnectionStateChange(EConnectionState state) = 0;

    // llwebrtc's data channel has opened on the test peer's side.
    virtual void OnDataChannelOpen() = 0;

    virtual void OnDataReceived(const std::string& data, bool binary) = 0;

protected:
    ~LLWebRTCTestPeerObserver() = default;
};

class LLWebRTCTestPeer
{
public:
    // Applies llwebrtc's offer and starts building an answer, delivered
    // later through OnAnswerAvailable() or OnAnswerFailed().  The test peer
    // also adds an audio track, so llwebrtc sees a remote track just as it
    // does with the real server.
    virtual void answerOffer(const std::string& offer_sdp) = 0;

    // Adds an ICE candidate trickled by llwebrtc, in the form llwebrtc
    // reports it through LLWebRTCSignalingObserver::OnIceCandidate().
    // Optional: without it ICE still connects via peer-reflexive discovery.
    virtual void addRemoteCandidate(const std::string& sdp_mid, int mline_index, const std::string& candidate) = 0;

    // Sends over llwebrtc's data channel, once it's open.
    virtual void sendData(const std::string& data, bool binary) = 0;

    // When enabled, everything received on the data channel is sent back.
    virtual void setEchoData(bool echo) = 0;

    // Closes the connection from the server side, as the real server does
    // when it ends a session.  Blocks until the close is done.
    virtual void close() = 0;

    // Stops all of the test peer's network activity, as if the server had
    // hung or dropped off the network: nothing is sent and nothing is
    // processed, so llwebrtc sees the connection go silent rather than
    // close.  thaw() resumes.  destroyTestPeer() thaws automatically.
    virtual void freeze() = 0;
    virtual void thaw() = 0;

protected:
    ~LLWebRTCTestPeer() = default;
};

// Creates a test peer reporting to 'observer', or returns null if webrtc
// couldn't be set up.  'observer' must outlive the test peer.
LLTESTPEER_API LLWebRTCTestPeer* createTestPeer(LLWebRTCTestPeerObserver* observer);

// Tears the test peer down.  No observer callbacks are made once this
// returns.
LLTESTPEER_API void destroyTestPeer(LLWebRTCTestPeer* peer);

} // namespace llwebrtc_testpeer

#endif // LLWEBRTC_TESTPEER_H
