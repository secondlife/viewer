/**
 * @file llwebrtc_testpeer.cpp
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

#define LL_TESTPEER_MAKEDLL

// Same platform setup as llwebrtc_impl.h; the webrtc headers rely on it.
#if defined(_WIN32) || defined(_WIN64)
#define WEBRTC_WIN 1
#elif defined(__APPLE__)
#define WEBRTC_MAC 1
#define WEBRTC_POSIX 1
#elif __linux__
#define WEBRTC_LINUX 1
#define WEBRTC_POSIX 1
#endif

#include "llwebrtc_testpeer.h"

#ifdef WEBRTC_WIN
#pragma warning(push)
#pragma warning(disable : 4996) // 'deprecated' -- see llwebrtc_impl.h
#pragma warning(disable : 4068) // unknown (clang) pragmas in the webrtc headers
#endif

#include "api/audio/create_audio_device_module.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/create_peerconnection_factory.h"
#include "api/data_channel_interface.h"
#include "api/environment/environment.h"
#include "api/environment/environment_factory.h"
#include "api/jsep.h"
#include "api/media_stream_interface.h"
#include "api/peer_connection_interface.h"
#include "api/scoped_refptr.h"
#include "api/set_local_description_observer_interface.h"
#include "api/set_remote_description_observer_interface.h"
#include "rtc_base/copy_on_write_buffer.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/thread.h"

#ifdef WEBRTC_WIN
#pragma warning(pop)
#endif

#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace llwebrtc_testpeer
{
namespace
{

using Observer = LLWebRTCTestPeerObserver;

void initializeWebRTCOnce()
{
    static std::once_flag sOnce;
    std::call_once(sOnce,
        []()
        {
            // This library has its own copy of webrtc (and of its SSL and
            // logging state), separate from llwebrtc's.
            webrtc::InitializeSSL();

            // Quiet unless asked: set LLWEBRTC_TESTPEER_LOG=1 to debug.
            const char* log = std::getenv("LLWEBRTC_TESTPEER_LOG");
            webrtc::LogMessage::LogToDebug((log && *log) ? webrtc::LS_VERBOSE : webrtc::LS_NONE);
        });
}

// Adapt a callable to webrtc's ref-counted SDP observer interfaces.
class SetLocalDescriptionObserver : public webrtc::SetLocalDescriptionObserverInterface
{
public:
    explicit SetLocalDescriptionObserver(std::function<void(webrtc::RTCError)> callback) : mCallback(std::move(callback)) {}
    void OnSetLocalDescriptionComplete(webrtc::RTCError error) override { mCallback(std::move(error)); }

private:
    std::function<void(webrtc::RTCError)> mCallback;
};

class SetRemoteDescriptionObserver : public webrtc::SetRemoteDescriptionObserverInterface
{
public:
    explicit SetRemoteDescriptionObserver(std::function<void(webrtc::RTCError)> callback) : mCallback(std::move(callback)) {}
    void OnSetRemoteDescriptionComplete(webrtc::RTCError error) override { mCallback(std::move(error)); }

private:
    std::function<void(webrtc::RTCError)> mCallback;
};

Observer::EConnectionState toObserverState(webrtc::PeerConnectionInterface::PeerConnectionState state)
{
    using State = webrtc::PeerConnectionInterface::PeerConnectionState;
    switch (state)
    {
        case State::kNew:          return Observer::CONNECTION_NEW;
        case State::kConnecting:   return Observer::CONNECTION_CONNECTING;
        case State::kConnected:    return Observer::CONNECTION_CONNECTED;
        case State::kDisconnected: return Observer::CONNECTION_DISCONNECTED;
        case State::kFailed:       return Observer::CONNECTION_FAILED;
        case State::kClosed:       return Observer::CONNECTION_CLOSED;
    }
    return Observer::CONNECTION_FAILED;
}

// Threading: everything touching the peer connection, the data channel or
// the observer runs on the signaling thread.  webrtc delivers all of its
// PeerConnectionObserver and DataChannelObserver callbacks there too, so
// once shutdown() has synchronized with that thread no further callbacks to
// the test's observer are possible.
class TestPeer final : public LLWebRTCTestPeer,
                       public webrtc::PeerConnectionObserver,
                       public webrtc::DataChannelObserver
{
public:
    explicit TestPeer(Observer* observer) :
        mEnv(webrtc::CreateEnvironment()),
        mObserver(observer)
    {
    }

    ~TestPeer() override { shutdown(); }

    bool init()
    {
        initializeWebRTCOnce();

        mNetworkThread = webrtc::Thread::CreateWithSocketServer();
        mNetworkThread->SetName("TestPeerNetworkThread", nullptr);
        mNetworkThread->Start();
        mWorkerThread = webrtc::Thread::Create();
        mWorkerThread->SetName("TestPeerWorkerThread", nullptr);
        mWorkerThread->Start();
        mSignalingThread = webrtc::Thread::Create();
        mSignalingThread->SetName("TestPeerSignalingThread", nullptr);
        mSignalingThread->Start();

        // The dummy audio device needs no hardware.
        mWorkerThread->BlockingCall(
            [this]()
            {
                mDeviceModule = webrtc::CreateAudioDeviceModule(mEnv, webrtc::AudioDeviceModule::AudioLayer::kDummyAudio);
            });
        if (!mDeviceModule)
        {
            return false;
        }

        mFactory = webrtc::CreatePeerConnectionFactory(mNetworkThread.get(),
                                                       mWorkerThread.get(),
                                                       mSignalingThread.get(),
                                                       mDeviceModule,
                                                       webrtc::CreateBuiltinAudioEncoderFactory(),
                                                       webrtc::CreateBuiltinAudioDecoderFactory(),
                                                       nullptr /* video_encoder_factory */,
                                                       nullptr /* video_decoder_factory */,
                                                       nullptr /* audio_mixer */,
                                                       nullptr /* audio_processing */);
        return mFactory != nullptr;
    }

    void shutdown()
    {
        if (!mSignalingThread)
        {
            return;
        }

        // Stop callbacks first, and make sure a frozen network thread can't
        // deadlock the teardown below.
        mShuttingDown = true;
        thaw();

        mSignalingThread->BlockingCall(
            [this]()
            {
                closeOnSignalingThread();
                mFactory = nullptr;
            });
        mWorkerThread->BlockingCall([this]() { mDeviceModule = nullptr; });

        mSignalingThread.reset();
        mWorkerThread.reset();
        mNetworkThread.reset();
    }

    //
    // LLWebRTCTestPeer
    //

    void answerOffer(const std::string& offer_sdp) override
    {
        mSignalingThread->PostTask([this, offer = std::string(offer_sdp)]() { startAnswer(offer); });
    }

    void addRemoteCandidate(const std::string& sdp_mid, int mline_index, const std::string& candidate) override
    {
        mSignalingThread->PostTask(
            [this, sdp_mid = std::string(sdp_mid), mline_index, candidate = std::string(candidate)]()
            {
                if (mShuttingDown || !mPeerConnection)
                {
                    return;
                }
                // llwebrtc reports the candidate without the SDP attribute name.
                webrtc::SdpParseError error;
                std::unique_ptr<webrtc::IceCandidate> ice_candidate(
                    webrtc::CreateIceCandidate(sdp_mid, mline_index, "candidate:" + candidate, &error));
                if (!ice_candidate)
                {
                    RTC_LOG(LS_ERROR) << "Test peer couldn't parse candidate '" << candidate << "': " << error.description;
                    return;
                }
                mPeerConnection->AddIceCandidate(std::move(ice_candidate), [](webrtc::RTCError) {});
            });
    }

    void sendData(const std::string& data, bool binary) override
    {
        mSignalingThread->PostTask(
            [this, data = std::string(data), binary]()
            {
                if (!mShuttingDown && mDataChannel)
                {
                    mDataChannel->Send(webrtc::DataBuffer(webrtc::CopyOnWriteBuffer(data.data(), data.size()), binary));
                }
            });
    }

    void setEchoData(bool echo) override { mEchoData = echo; }

    void close() override
    {
        thaw();
        mSignalingThread->BlockingCall([this]() { closeOnSignalingThread(); });
    }

    void freeze() override
    {
        {
            std::lock_guard<std::mutex> lock(mFreezeMutex);
            if (mFrozen)
            {
                return;
            }
            mFrozen = true;
        }
        // Park the network thread: every packet, timer and socket read on
        // it waits behind this task until thaw().
        mNetworkThread->PostTask(
            [this]()
            {
                std::unique_lock<std::mutex> lock(mFreezeMutex);
                mFreezeCondition.wait(lock, [this]() { return !mFrozen; });
            });
    }

    void thaw() override
    {
        {
            std::lock_guard<std::mutex> lock(mFreezeMutex);
            mFrozen = false;
        }
        mFreezeCondition.notify_all();
    }

    //
    // webrtc::PeerConnectionObserver
    //

    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState) override {}

    void OnDataChannel(webrtc::scoped_refptr<webrtc::DataChannelInterface> channel) override
    {
        mDataChannel = channel;
        mDataChannel->RegisterObserver(this);
        OnStateChange(); // it may already be open
    }

    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState state) override
    {
        if (state == webrtc::PeerConnectionInterface::kIceGatheringComplete)
        {
            maybeSendAnswer();
        }
    }

    void OnIceCandidate(const webrtc::IceCandidate*) override
    {
        // Not trickled: the answer is held until gathering completes and
        // carries every candidate (see OnAnswerAvailable()).
    }

    void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState state) override
    {
        notify([state](Observer& observer) { observer.OnConnectionStateChange(toObserverState(state)); });
    }

    //
    // webrtc::DataChannelObserver
    //

    void OnStateChange() override
    {
        if (mDataChannel && mDataChannel->state() == webrtc::DataChannelInterface::kOpen && !mDataChannelOpenReported)
        {
            mDataChannelOpenReported = true;
            notify([](Observer& observer) { observer.OnDataChannelOpen(); });
        }
    }

    void OnMessage(const webrtc::DataBuffer& buffer) override
    {
        std::string data(reinterpret_cast<const char*>(buffer.data.cdata()), buffer.size());
        notify([&data, &buffer](Observer& observer) { observer.OnDataReceived(data, buffer.binary); });

        if (mEchoData && mDataChannel && !mShuttingDown)
        {
            mDataChannel->Send(buffer);
        }
    }

private:
    // Signaling thread only.
    template <typename F>
    void notify(F&& callback)
    {
        if (!mShuttingDown && mObserver)
        {
            callback(*mObserver);
        }
    }

    // Signaling thread only.
    void answerFailed(const std::string& error)
    {
        notify([&error](Observer& observer) { observer.OnAnswerFailed(error); });
    }

    // Signaling thread only.
    void startAnswer(const std::string& offer)
    {
        if (mShuttingDown)
        {
            return;
        }
        if (mPeerConnection)
        {
            answerFailed("answerOffer() may only be called once per test peer");
            return;
        }

        webrtc::PeerConnectionInterface::RTCConfiguration config;
        config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;

        auto peer_connection = mFactory->CreatePeerConnectionOrError(config, webrtc::PeerConnectionDependencies(this));
        if (!peer_connection.ok())
        {
            answerFailed(std::string("couldn't create a peer connection: ") + peer_connection.error().message());
            return;
        }
        mPeerConnection = peer_connection.MoveValue();

        webrtc::SdpParseError parse_error;
        std::unique_ptr<webrtc::SessionDescriptionInterface> description =
            webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, offer, &parse_error);
        if (!description)
        {
            answerFailed("couldn't parse the offer: " + parse_error.description + " (" + parse_error.line + ")");
            return;
        }

        mPeerConnection->SetRemoteDescription(
            std::move(description),
            webrtc::make_ref_counted<SetRemoteDescriptionObserver>([this](webrtc::RTCError error) { onRemoteDescriptionSet(std::move(error)); }));
    }

    // Signaling thread only.
    void onRemoteDescriptionSet(webrtc::RTCError error)
    {
        if (mShuttingDown || !mPeerConnection)
        {
            return;
        }
        if (!error.ok())
        {
            answerFailed(std::string("couldn't apply the offer: ") + error.message());
            return;
        }

        // Send audio back, as the server does, so llwebrtc sees a remote
        // track.  It takes over the offer's audio transceiver.
        webrtc::scoped_refptr<webrtc::AudioSourceInterface> source = mFactory->CreateAudioSource(webrtc::AudioOptions());
        webrtc::scoped_refptr<webrtc::AudioTrackInterface> track  = mFactory->CreateAudioTrack("TestPeerAudio", source.get());
        auto sender = mPeerConnection->AddTrack(track, { "TestPeerStream" });
        if (!sender.ok())
        {
            answerFailed(std::string("couldn't add an audio track: ") + sender.error().message());
            return;
        }

        // With no description given, this creates and applies the answer.
        mPeerConnection->SetLocalDescription(webrtc::make_ref_counted<SetLocalDescriptionObserver>(
            [this](webrtc::RTCError error)
            {
                if (mShuttingDown)
                {
                    return;
                }
                if (!error.ok())
                {
                    answerFailed(std::string("couldn't create an answer: ") + error.message());
                    return;
                }
                maybeSendAnswer();
            }));
    }

    // Signaling thread only.  Called both when the answer has been applied
    // and when ICE gathering completes; whichever happens last sends it.
    void maybeSendAnswer()
    {
        if (mAnswerSent || mShuttingDown || !mPeerConnection)
        {
            return;
        }
        const webrtc::SessionDescriptionInterface* description = mPeerConnection->local_description();
        if (!description ||
            mPeerConnection->ice_gathering_state() != webrtc::PeerConnectionInterface::kIceGatheringComplete)
        {
            return;
        }

        mAnswerSent = true;
        std::string sdp;
        description->ToString(&sdp);
        notify([&sdp](Observer& observer) { observer.OnAnswerAvailable(sdp); });
    }

    // Signaling thread only.
    void closeOnSignalingThread()
    {
        if (mDataChannel)
        {
            mDataChannel->UnregisterObserver();
            mDataChannel->Close();
            mDataChannel = nullptr;
        }
        if (mPeerConnection)
        {
            mPeerConnection->Close();
            mPeerConnection = nullptr;
        }
    }

    const webrtc::Environment mEnv;
    Observer* const           mObserver;

    std::unique_ptr<webrtc::Thread>                               mNetworkThread;
    std::unique_ptr<webrtc::Thread>                               mWorkerThread;
    std::unique_ptr<webrtc::Thread>                               mSignalingThread;
    webrtc::scoped_refptr<webrtc::AudioDeviceModule>              mDeviceModule;   // worker thread
    webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> mFactory;

    // Signaling thread only.
    webrtc::scoped_refptr<webrtc::PeerConnectionInterface> mPeerConnection;
    webrtc::scoped_refptr<webrtc::DataChannelInterface>    mDataChannel;
    bool                                                   mAnswerSent{ false };
    bool                                                   mDataChannelOpenReported{ false };

    std::atomic<bool> mShuttingDown{ false };
    std::atomic<bool> mEchoData{ false };

    std::mutex              mFreezeMutex;
    std::condition_variable mFreezeCondition;
    bool                    mFrozen{ false };
};

} // namespace

LLWebRTCTestPeer* createTestPeer(LLWebRTCTestPeerObserver* observer)
{
    auto peer = std::make_unique<TestPeer>(observer);
    if (!peer->init())
    {
        return nullptr;
    }
    return peer.release();
}

void destroyTestPeer(LLWebRTCTestPeer* peer)
{
    delete static_cast<TestPeer*>(peer);
}

} // namespace llwebrtc_testpeer
