/**
 * @file llwebrtc_impl.h
 * @brief WebRTC dynamic library implementation header
 *
 * $LicenseInfo:firstyear=2023&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
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

#ifndef LLWEBRTC_IMPL_H
#define LLWEBRTC_IMPL_H

#define LL_MAKEDLL
#if defined(_WIN32) || defined(_WIN64)
#define WEBRTC_WIN 1
#elif defined(__APPLE__)
#define WEBRTC_MAC 1
#define WEBRTC_POSIX 1
#elif __linux__
#define WEBRTC_LINUX 1
#define WEBRTC_POSIX 1
#endif

#include "llwebrtc.h"
// WebRTC Includes
#ifdef WEBRTC_WIN
#pragma warning(push)
#pragma warning(disable : 4996) // ignore 'deprecated.'  We don't use the functions marked
                                // deprecated in the webrtc headers, but msvc complains anyway.
                                // Clang doesn't, and that's generally what webrtc uses.
#pragma warning(disable : 4068) // ignore 'invalid pragma.'  There are clang pragma's in
                                // the webrtc headers, which msvc doesn't recognize.
#endif // WEBRTC_WIN

#include "api/scoped_refptr.h"
#include "rtc_base/ref_count.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/thread.h"
#include "api/peer_connection_interface.h"
#include "api/media_stream_interface.h"
#include "api/create_peerconnection_factory.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_device/include/audio_device_data_observer.h"
#include "rtc_base/task_queue.h"
#include "api/task_queue/task_queue_factory.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "modules/audio_device/include/audio_device_defines.h"

namespace llwebrtc
{

class LLWebRTCPeerConnectionImpl;

class LLWebRTCLogSink : public rtc::LogSink {
public:
    LLWebRTCLogSink(LLWebRTCLogCallback* callback) :
    mCallback(callback)
    {
    }

    // Destructor: close the log file
    ~LLWebRTCLogSink() override
    {
    }

    void OnLogMessage(const std::string& msg,
                      rtc::LoggingSeverity severity) override
    {
        if (mCallback)
        {
            switch(severity)
            {
                case rtc::LS_VERBOSE:
                    mCallback->LogMessage(LLWebRTCLogCallback::LOG_LEVEL_VERBOSE, msg);
                    break;
                case rtc::LS_INFO:
                    mCallback->LogMessage(LLWebRTCLogCallback::LOG_LEVEL_VERBOSE, msg);
                    break;
                case rtc::LS_WARNING:
                    mCallback->LogMessage(LLWebRTCLogCallback::LOG_LEVEL_VERBOSE, msg);
                    break;
                case rtc::LS_ERROR:
                    mCallback->LogMessage(LLWebRTCLogCallback::LOG_LEVEL_VERBOSE, msg);
                    break;
                default:
                    break;
            }
        }
    }

    void OnLogMessage(const std::string& message) override
    {
        if (mCallback)
        {
            mCallback->LogMessage(LLWebRTCLogCallback::LOG_LEVEL_VERBOSE, message);
        }
    }

private:
    LLWebRTCLogCallback* mCallback;
};

// Implements a class allowing capture of audio data
// to determine audio level of the microphone.
class LLAudioDeviceObserver : public webrtc::AudioDeviceDataObserver
{
  public:
    LLAudioDeviceObserver();

    // Retrieve the RMS audio loudness
    float getMicrophoneEnergy();

    // Data retrieved from the caputure device is
    // passed in here for processing.
    void OnCaptureData(const void    *audio_samples,
                       const size_t   num_samples,
                       const size_t   bytes_per_sample,
                       const size_t   num_channels,
                       const uint32_t samples_per_sec) override;

    // This is for data destined for the render device.
    // not currently used.
    void OnRenderData(const void    *audio_samples,
                      const size_t   num_samples,
                      const size_t   bytes_per_sample,
                      const size_t   num_channels,
                      const uint32_t samples_per_sec) override;

  protected:
    static const int NUM_PACKETS_TO_FILTER = 30;  // 300 ms of smoothing (30 frames)
    float mSumVector[NUM_PACKETS_TO_FILTER];
    float mMicrophoneEnergy;
};

// Used to process/retrieve audio levels after
// all of the processing (AGC, AEC, etc.) for display in-world to the user.
class LLCustomProcessor : public webrtc::CustomProcessing
{
  public:
    LLCustomProcessor();
    ~LLCustomProcessor() override {}

    // (Re-) Initializes the submodule.
    void Initialize(int sample_rate_hz, int num_channels) override;

    // Analyzes the given capture or render signal.
    void Process(webrtc::AudioBuffer *audio) override;

    // Returns a string representation of the module state.
    std::string ToString() const override { return ""; }

    float getMicrophoneEnergy() { return mMicrophoneEnergy; }

    void setGain(float gain) { mGain = gain; }

  protected:
    static const int NUM_PACKETS_TO_FILTER = 30;  // 300 ms of smoothing
    int              mSampleRateHz;
    int              mNumChannels;

    float mSumVector[NUM_PACKETS_TO_FILTER];
    float mMicrophoneEnergy;
    float mGain;
};


// Primary singleton implementation for interfacing
// with the native webrtc library.
class LLWebRTCImpl : public LLWebRTCDeviceInterface, public webrtc::AudioDeviceSink
{
  public:
    LLWebRTCImpl(LLWebRTCLogCallback* logCallback);
    ~LLWebRTCImpl()
    {
        delete mLogSink;
    }

    void init();
    void terminate();

    //
    // LLWebRTCDeviceInterface
    //

    void setAudioConfig(LLWebRTCDeviceInterface::AudioConfig config = LLWebRTCDeviceInterface::AudioConfig()) override;

    void refreshDevices() override;

    void setDevicesObserver(LLWebRTCDevicesObserver *observer) override;
    void unsetDevicesObserver(LLWebRTCDevicesObserver *observer) override;

    void setCaptureDevice(const std::string& id) override;
    void setRenderDevice(const std::string& id) override;

    void setTuningMode(bool enable) override;
    float getTuningAudioLevel() override;
    float getPeerConnectionAudioLevel() override;

    void setPeerConnectionGain(float gain) override;

    //
    // AudioDeviceSink
    //
    void OnDevicesUpdated() override;

    //
    // Helpers
    //

    // The following thread helpers allow the
    // LLWebRTCPeerConnectionImpl class to post
    // tasks to the native webrtc threads.
    void PostWorkerTask(absl::AnyInvocable<void() &&> task,
                  const webrtc::Location& location = webrtc::Location::Current())
    {
        mWorkerThread->PostTask(std::move(task), location);
    }

    void PostSignalingTask(absl::AnyInvocable<void() &&> task,
                  const webrtc::Location& location = webrtc::Location::Current())
    {
        mSignalingThread->PostTask(std::move(task), location);
    }

    void PostNetworkTask(absl::AnyInvocable<void() &&> task,
                  const webrtc::Location& location = webrtc::Location::Current())
    {
        mNetworkThread->PostTask(std::move(task), location);
    }

    void WorkerBlockingCall(rtc::FunctionView<void()> functor,
                  const webrtc::Location& location = webrtc::Location::Current())
    {
        mWorkerThread->BlockingCall(std::move(functor), location);
    }

    void SignalingBlockingCall(rtc::FunctionView<void()> functor,
                  const webrtc::Location& location = webrtc::Location::Current())
    {
        mSignalingThread->BlockingCall(std::move(functor), location);
    }

    void NetworkBlockingCall(rtc::FunctionView<void()> functor,
                  const webrtc::Location& location = webrtc::Location::Current())
    {
        mNetworkThread->BlockingCall(std::move(functor), location);
    }

    // Allows the LLWebRTCPeerConnectionImpl class to retrieve the
    // native webrtc PeerConnectionFactory.
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> getPeerConnectionFactory()
    {
        return mPeerConnectionFactory;
    }

    // create or destroy a peer connection.
    LLWebRTCPeerConnectionInterface* newPeerConnection();
    void freePeerConnection(LLWebRTCPeerConnectionInterface* peer_connection);

    // enables/disables capture via the capture device
    void setRecording(bool recording);

    void setPlayout(bool playing);

  protected:
    LLWebRTCLogSink*                                           mLogSink;

    // The native webrtc threads
    std::unique_ptr<rtc::Thread>                               mNetworkThread;
    std::unique_ptr<rtc::Thread>                               mWorkerThread;
    std::unique_ptr<rtc::Thread>                               mSignalingThread;

    // The factory that allows creation of native webrtc PeerConnections.
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> mPeerConnectionFactory;

    rtc::scoped_refptr<webrtc::AudioProcessing>                mAudioProcessingModule;

    // more native webrtc stuff
    std::unique_ptr<webrtc::TaskQueueFactory>                  mTaskQueueFactory;


    // Devices
    void updateDevices();
    rtc::scoped_refptr<webrtc::AudioDeviceModule>              mTuningDeviceModule;
    rtc::scoped_refptr<webrtc::AudioDeviceModule>              mPeerDeviceModule;
    std::vector<LLWebRTCDevicesObserver *>                     mVoiceDevicesObserverList;

    // accessors in native webrtc for devices aren't apparently implemented yet.
    bool                                                       mTuningMode;
    int32_t                                                    mRecordingDevice;
    LLWebRTCVoiceDeviceList                                    mRecordingDeviceList;

    int32_t                                                    mPlayoutDevice;
    LLWebRTCVoiceDeviceList                                    mPlayoutDeviceList;

    bool                                                       mMute;

    LLAudioDeviceObserver *                                    mTuningAudioDeviceObserver;
    LLCustomProcessor *                                        mPeerCustomProcessor;

    // peer connections
    std::vector<rtc::scoped_refptr<LLWebRTCPeerConnectionImpl>>     mPeerConnections;
};


// The implementation of a peer connection, which contains
// the various interfaces used by the viewer to interact with
// the webrtc connection.
class LLWebRTCPeerConnectionImpl : public LLWebRTCPeerConnectionInterface,
                                   public LLWebRTCAudioInterface,
                                   public LLWebRTCDataInterface,
                                   public webrtc::PeerConnectionObserver,
                                   public webrtc::CreateSessionDescriptionObserver,
                                   public webrtc::SetRemoteDescriptionObserverInterface,
                                   public webrtc::SetLocalDescriptionObserverInterface,
                                   public webrtc::DataChannelObserver

{
  public:
    LLWebRTCPeerConnectionImpl();
    ~LLWebRTCPeerConnectionImpl();

    void init(LLWebRTCImpl * webrtc_impl);
    void terminate();

    virtual void AddRef() const override = 0;
    virtual rtc::RefCountReleaseStatus Release() const override = 0;

    //
    // LLWebRTCPeerConnection
    //
    bool initializeConnection(const InitOptions& options) override;
    bool shutdownConnection() override;

    void setSignalingObserver(LLWebRTCSignalingObserver *observer) override;
    void unsetSignalingObserver(LLWebRTCSignalingObserver *observer) override;
    void AnswerAvailable(const std::string &sdp) override;

    //
    // LLWebRTCAudioInterface
    //
    void setMute(bool mute) override;
    void setReceiveVolume(float volume) override;  // volume between 0.0 and 1.0
    void setSendVolume(float volume) override;  // volume between 0.0 and 1.0

    //
    // LLWebRTCDataInterface
    //
    void sendData(const std::string& data, bool binary=false) override;
    void setDataObserver(LLWebRTCDataObserver *observer) override;
    void unsetDataObserver(LLWebRTCDataObserver *observer) override;

    //
    // PeerConnectionObserver implementation.
    //

    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {}
    void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                    const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &streams) override;
    void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override;
    void OnRenegotiationNeeded() override {}
    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {};
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
    void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override;
    void OnIceConnectionReceivingChange(bool receiving) override {}
    void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState new_state) override;

    //
    // CreateSessionDescriptionObserver implementation.
    //
    void OnSuccess(webrtc::SessionDescriptionInterface *desc) override;
    void OnFailure(webrtc::RTCError error) override;

    //
    // SetRemoteDescriptionObserverInterface implementation.
    //
    void OnSetRemoteDescriptionComplete(webrtc::RTCError error) override;

    //
    // SetLocalDescriptionObserverInterface implementation.
    //
    void OnSetLocalDescriptionComplete(webrtc::RTCError error) override;

    //
    // DataChannelObserver implementation.
    //
    void OnStateChange() override;
    void OnMessage(const webrtc::DataBuffer& buffer) override;

    // Helpers
    void resetMute();
    void enableSenderTracks(bool enable);
    void enableReceiverTracks(bool enable);

  protected:

    LLWebRTCImpl * mWebRTCImpl;

    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> mPeerConnectionFactory;

    typedef enum {
        MUTE_INITIAL,
        MUTE_MUTED,
        MUTE_UNMUTED,
    } EMicMuteState;
    EMicMuteState mMute;

    // signaling
    std::vector<LLWebRTCSignalingObserver *>  mSignalingObserverList;
    std::vector<std::unique_ptr<webrtc::IceCandidateInterface>>  mCachedIceCandidates;
    bool mAnswerReceived;

    rtc::scoped_refptr<webrtc::PeerConnectionInterface> mPeerConnection;
    rtc::scoped_refptr<webrtc::MediaStreamInterface> mLocalStream;

    // data
    std::vector<LLWebRTCDataObserver *> mDataObserverList;
    rtc::scoped_refptr<webrtc::DataChannelInterface> mDataChannel;
};

}

#if WEBRTC_WIN
#pragma warning(pop)
#endif

#endif // LLWEBRTC_IMPL_H
