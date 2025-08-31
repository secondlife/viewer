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
#include "rtc_base/logging.h"
#include "api/peer_connection_interface.h"
#include "api/media_stream_interface.h"
#include "api/create_peerconnection_factory.h"
#include "modules/audio_device/include/audio_device.h"
#include "modules/audio_device/include/audio_device_data_observer.h"
#include "api/task_queue/task_queue_factory.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "modules/audio_device/include/audio_device_defines.h"

namespace llwebrtc
{

class LLWebRTCPeerConnectionImpl;

class LLWebRTCLogSink : public webrtc::LogSink
{
public:
    LLWebRTCLogSink(LLWebRTCLogCallback* callback) : mCallback(callback) {}

    // Destructor: close the log file
    ~LLWebRTCLogSink() override {}

    void OnLogMessage(const std::string& msg, webrtc::LoggingSeverity severity) override
    {
        if (mCallback)
        {
            switch (severity)
            {
                case webrtc::LS_VERBOSE:
                    mCallback->LogMessage(LLWebRTCLogCallback::LOG_LEVEL_VERBOSE, msg);
                    break;
                case webrtc::LS_INFO:
                    mCallback->LogMessage(LLWebRTCLogCallback::LOG_LEVEL_VERBOSE, msg);
                    break;
                case webrtc::LS_WARNING:
                    mCallback->LogMessage(LLWebRTCLogCallback::LOG_LEVEL_VERBOSE, msg);
                    break;
                case webrtc::LS_ERROR:
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

// -----------------------------------------------------------------------------
// A proxy transport that forwards capture data to two AudioTransport sinks:
//  - the "engine" (libwebrtc's VoiceEngine)
//  - the "user" (your app's listener)
//
// Playout (NeedMorePlayData) goes only to the engine by default to avoid
// double-writing into the output buffer. See notes below if you want a tap.
// -----------------------------------------------------------------------------
class LLWebRTCAudioTransport : public webrtc::AudioTransport
{
public:
    LLWebRTCAudioTransport();

    void SetEngineTransport(webrtc::AudioTransport* t);

    // -------- Capture path: fan out to both sinks --------
    int32_t RecordedDataIsAvailable(const void* audio_data,
                                    size_t      number_of_samples,
                                    size_t      bytes_per_sample,
                                    size_t      number_of_channels,
                                    uint32_t    samples_per_sec,
                                    uint32_t    total_delay_ms,
                                    int32_t     clock_drift,
                                    uint32_t    current_mic_level,
                                    bool        key_pressed,
                                    uint32_t&   new_mic_level) override;

    // -------- Playout path: delegate to engine only --------
    int32_t NeedMorePlayData(size_t   number_of_samples,
                             size_t   bytes_per_sample,
                             size_t   number_of_channels,
                             uint32_t samples_per_sec,
                             void*    audio_data,
                             size_t&  number_of_samples_out,
                             int64_t* elapsed_time_ms,
                             int64_t* ntp_time_ms) override;

    // Method to pull mixed render audio data from all active VoE channels.
    // The data will not be passed as reference for audio processing internally.
    void PullRenderData(int      bits_per_sample,
                        int      sample_rate,
                        size_t   number_of_channels,
                        size_t   number_of_frames,
                        void*    audio_data,
                        int64_t* elapsed_time_ms,
                        int64_t* ntp_time_ms) override;

    float GetMicrophoneEnergy() { return mMicrophoneEnergy; }

private:
    std::atomic<webrtc::AudioTransport*> engine_{ nullptr };
    static const int                     NUM_PACKETS_TO_FILTER = 30; // 300 ms of smoothing (30 frames)
    float                                mSumVector[NUM_PACKETS_TO_FILTER];
    float                                mMicrophoneEnergy;
};


// -----------------------------------------------------------------------------
// LLWebRTCAudioDeviceModule
// - Wraps a real ADM to provide microphone energy for tuning
// -----------------------------------------------------------------------------
class LLWebRTCAudioDeviceModule : public webrtc::AudioDeviceModule
{
public:
    explicit LLWebRTCAudioDeviceModule(webrtc::scoped_refptr<webrtc::AudioDeviceModule> inner) : inner_(std::move(inner)), tuning_(false)
    {
        RTC_CHECK(inner_);
    }

    // ----- AudioDeviceModule interface: we mostly forward to |inner_| -----
    int32_t ActiveAudioLayer(AudioLayer* audioLayer) const override { return inner_->ActiveAudioLayer(audioLayer); }

    int32_t RegisterAudioCallback(webrtc::AudioTransport* engine_transport) override
    {
        // The engine registers its transport here. We put our audio transport between engine and ADM.
        audio_transport_.SetEngineTransport(engine_transport);
        // Register our proxy with the real ADM.
        return inner_->RegisterAudioCallback(&audio_transport_);
    }

    int32_t Init() override { return inner_->Init(); }
    int32_t Terminate() override { return inner_->Terminate(); }
    bool    Initialized() const override { return inner_->Initialized(); }

    // --- Device enumeration/selection (forward) ---
    int16_t PlayoutDevices() override { return inner_->PlayoutDevices(); }
    int16_t RecordingDevices() override { return inner_->RecordingDevices(); }
    int32_t PlayoutDeviceName(uint16_t index, char name[webrtc::kAdmMaxDeviceNameSize], char guid[webrtc::kAdmMaxGuidSize]) override
    {
        return inner_->PlayoutDeviceName(index, name, guid);
    }
    int32_t RecordingDeviceName(uint16_t index, char name[webrtc::kAdmMaxDeviceNameSize], char guid[webrtc::kAdmMaxGuidSize]) override
    {
        return inner_->RecordingDeviceName(index, name, guid);
    }
    int32_t SetPlayoutDevice(uint16_t index) override { return inner_->SetPlayoutDevice(index); }
    int32_t SetRecordingDevice(uint16_t index) override { return inner_->SetRecordingDevice(index); }

    // Windows default/communications selectors, if your branch exposes them:
    int32_t SetPlayoutDevice(WindowsDeviceType type) override { return inner_->SetPlayoutDevice(type); }
    int32_t SetRecordingDevice(WindowsDeviceType type) override { return inner_->SetRecordingDevice(type); }

    // --- Init/start/stop (forward) ---
    int32_t InitPlayout() override { return inner_->InitPlayout(); }
    bool    PlayoutIsInitialized() const override { return inner_->PlayoutIsInitialized(); }
    int32_t StartPlayout() override {
        if (tuning_) return 0;  // For tuning, don't allow playout
        return inner_->StartPlayout();
    }
    int32_t StopPlayout() override { return inner_->StopPlayout(); }
    bool    Playing() const override { return inner_->Playing(); }

    int32_t InitRecording() override { return inner_->InitRecording(); }
    bool    RecordingIsInitialized() const override { return inner_->RecordingIsInitialized(); }
    int32_t StartRecording() override { return inner_->StartRecording(); }
    int32_t StopRecording() override {
        if (tuning_) return 0;  // if we're tuning, disregard the StopRecording we get from disabling the streams
        return inner_->StopRecording();
    }
    int32_t ForceStartRecording() { return inner_->StartRecording(); }
    int32_t ForceStopRecording() { return inner_->StopRecording(); }
    bool    Recording() const override { return inner_->Recording(); }

    // --- Stereo opts (forward if available on your branch) ---
    int32_t SetStereoPlayout(bool enable) override { return inner_->SetStereoPlayout(enable); }
    int32_t SetStereoRecording(bool enable) override { return inner_->SetStereoRecording(enable); }
    int32_t PlayoutIsAvailable(bool* available) override { return inner_->PlayoutIsAvailable(available); }
    int32_t RecordingIsAvailable(bool* available) override { return inner_->RecordingIsAvailable(available); }

    // --- AGC/Volume/Mute/etc. (forward) ---
    int32_t SetMicrophoneVolume(uint32_t volume) override { return inner_->SetMicrophoneVolume(volume); }
    int32_t MicrophoneVolume(uint32_t* volume) const override { return inner_->MicrophoneVolume(volume); }

    // --- Speaker/Microphone init (forward) ---
    int32_t InitSpeaker() override { return inner_->InitSpeaker(); }
    bool    SpeakerIsInitialized() const override { return inner_->SpeakerIsInitialized(); }
    int32_t InitMicrophone() override { return inner_->InitMicrophone(); }
    bool    MicrophoneIsInitialized() const override { return inner_->MicrophoneIsInitialized(); }

    // --- Speaker Volume (forward) ---
    int32_t SpeakerVolumeIsAvailable(bool* available) override { return inner_->SpeakerVolumeIsAvailable(available); }
    int32_t SetSpeakerVolume(uint32_t volume) override { return inner_->SetSpeakerVolume(volume); }
    int32_t SpeakerVolume(uint32_t* volume) const override { return inner_->SpeakerVolume(volume); }
    int32_t MaxSpeakerVolume(uint32_t* maxVolume) const override { return inner_->MaxSpeakerVolume(maxVolume); }
    int32_t MinSpeakerVolume(uint32_t* minVolume) const override { return inner_->MinSpeakerVolume(minVolume); }

    // --- Microphone Volume (forward) ---
    int32_t MicrophoneVolumeIsAvailable(bool* available) override { return inner_->MicrophoneVolumeIsAvailable(available); }
    int32_t MaxMicrophoneVolume(uint32_t* maxVolume) const override { return inner_->MaxMicrophoneVolume(maxVolume); }
    int32_t MinMicrophoneVolume(uint32_t* minVolume) const override { return inner_->MinMicrophoneVolume(minVolume); }

    // --- Speaker Mute (forward) ---
    int32_t SpeakerMuteIsAvailable(bool* available) override { return inner_->SpeakerMuteIsAvailable(available); }
    int32_t SetSpeakerMute(bool enable) override { return inner_->SetSpeakerMute(enable); }
    int32_t SpeakerMute(bool* enabled) const override { return inner_->SpeakerMute(enabled); }

    // --- Microphone Mute (forward) ---
    int32_t MicrophoneMuteIsAvailable(bool* available) override { return inner_->MicrophoneMuteIsAvailable(available); }
    int32_t SetMicrophoneMute(bool enable) override { return inner_->SetMicrophoneMute(enable); }
    int32_t MicrophoneMute(bool* enabled) const override { return inner_->MicrophoneMute(enabled); }

    // --- Stereo Support (forward) ---
    int32_t StereoPlayoutIsAvailable(bool* available) const override { return inner_->StereoPlayoutIsAvailable(available); }
    int32_t StereoPlayout(bool* enabled) const override { return inner_->StereoPlayout(enabled); }
    int32_t StereoRecordingIsAvailable(bool* available) const override { return inner_->StereoRecordingIsAvailable(available); }
    int32_t StereoRecording(bool* enabled) const override { return inner_->StereoRecording(enabled); }

    // --- Delay/Timing (forward) ---
    int32_t PlayoutDelay(uint16_t* delayMS) const override { return inner_->PlayoutDelay(delayMS); }

    // --- Built-in Audio Processing (forward) ---
    bool    BuiltInAECIsAvailable() const override { return inner_->BuiltInAECIsAvailable(); }
    bool    BuiltInAGCIsAvailable() const override { return inner_->BuiltInAGCIsAvailable(); }
    bool    BuiltInNSIsAvailable() const override { return inner_->BuiltInNSIsAvailable(); }
    int32_t EnableBuiltInAEC(bool enable) override { return inner_->EnableBuiltInAEC(enable); }
    int32_t EnableBuiltInAGC(bool enable) override { return inner_->EnableBuiltInAGC(enable); }
    int32_t EnableBuiltInNS(bool enable) override { return inner_->EnableBuiltInNS(enable); }

    // --- Additional AudioDeviceModule methods (forward) ---
    int32_t GetPlayoutUnderrunCount() const override { return inner_->GetPlayoutUnderrunCount(); }

    // Used to generate RTC stats. If not implemented, RTCAudioPlayoutStats will
    // not be present in the stats.
    std::optional<Stats> GetStats() const override { return inner_->GetStats(); }

// Only supported on iOS.
#if defined(WEBRTC_IOS)
    virtual int GetPlayoutAudioParameters(AudioParameters* params) const override { return inner_->GetPlayoutAudioParameters(params); }
    virtual int GetRecordAudioParameters(AudioParameters* params) override { return inner_->GetRecordAudioParameters(params); }
#endif // WEBRTC_IOS

    virtual int32_t GetPlayoutDevice() const override { return inner_->GetPlayoutDevice(); }
    virtual int32_t GetRecordingDevice() const override { return inner_->GetRecordingDevice(); }

    // tuning microphone energy calculations
    float GetMicrophoneEnergy() { return audio_transport_.GetMicrophoneEnergy(); }
    void  SetTuning(bool tuning)
    {
        tuning_ = tuning;
        inner_->InitRecording();
        inner_->StartRecording();
        if (tuning)
        {
            inner_->StopPlayout();
        }
        else
        {
            inner_->StartPlayout();
        }
    }

protected:
    ~LLWebRTCAudioDeviceModule() override = default;

private:
    webrtc::scoped_refptr<webrtc::AudioDeviceModule> inner_;
    LLWebRTCAudioTransport                        audio_transport_;

    bool tuning_;
};

class LLCustomProcessorState
{

public:
    float getMicrophoneEnergy() { return mMicrophoneEnergy.load(std::memory_order_relaxed); }
    void setMicrophoneEnergy(float energy) { mMicrophoneEnergy.store(energy, std::memory_order_relaxed); }

    void setGain(float gain) { mGain.store(gain, std::memory_order_relaxed); }
    float getGain() { return mGain.load(std::memory_order_relaxed); }

 protected:
    std::atomic<float> mMicrophoneEnergy{ 0.0f };
    std::atomic<float> mGain{ 0.0f };
};

using LLCustomProcessorStatePtr = std::shared_ptr<LLCustomProcessorState>;

// Used to process/retrieve audio levels after
// all of the processing (AGC, AEC, etc.) for display in-world to the user.
class LLCustomProcessor : public webrtc::CustomProcessing
{
  public:
    LLCustomProcessor(LLCustomProcessorStatePtr state);
    ~LLCustomProcessor() override {}

    // (Re-) Initializes the submodule.
    void Initialize(int sample_rate_hz, int num_channels) override;

    // Analyzes the given capture or render signal.
    void Process(webrtc::AudioBuffer *audio) override;

    // Returns a string representation of the module state.
    std::string ToString() const override { return ""; }

  protected:
    static const int NUM_PACKETS_TO_FILTER = 30;  // 300 ms of smoothing
    int              mSampleRateHz;
    int              mNumChannels;

    float mSumVector[NUM_PACKETS_TO_FILTER];
    friend LLCustomProcessorState;
    LLCustomProcessorStatePtr mState;
};


// Primary singleton implementation for interfacing
// with the native webrtc library.
class LLWebRTCImpl : public LLWebRTCDeviceInterface, public webrtc::AudioDeviceObserver
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
    // AudioDeviceObserver
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

    void WorkerBlockingCall(webrtc::FunctionView<void()> functor,
                  const webrtc::Location& location = webrtc::Location::Current())
    {
        mWorkerThread->BlockingCall(std::move(functor), location);
    }

    void SignalingBlockingCall(webrtc::FunctionView<void()> functor,
                  const webrtc::Location& location = webrtc::Location::Current())
    {
        mSignalingThread->BlockingCall(std::move(functor), location);
    }

    void NetworkBlockingCall(webrtc::FunctionView<void()> functor,
                  const webrtc::Location& location = webrtc::Location::Current())
    {
        mNetworkThread->BlockingCall(std::move(functor), location);
    }

    // Allows the LLWebRTCPeerConnectionImpl class to retrieve the
    // native webrtc PeerConnectionFactory.
    webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> getPeerConnectionFactory()
    {
        return mPeerConnectionFactory;
    }

    // create or destroy a peer connection.
    LLWebRTCPeerConnectionInterface* newPeerConnection();
    void freePeerConnection(LLWebRTCPeerConnectionInterface* peer_connection);

  protected:

    void workerDeployDevices();
    LLWebRTCLogSink*                                           mLogSink;

    // The native webrtc threads
    std::unique_ptr<webrtc::Thread>                            mNetworkThread;
    std::unique_ptr<webrtc::Thread>                            mWorkerThread;
    std::unique_ptr<webrtc::Thread>                            mSignalingThread;

    // The factory that allows creation of native webrtc PeerConnections.
    webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> mPeerConnectionFactory;

    webrtc::scoped_refptr<webrtc::AudioProcessing>                mAudioProcessingModule;

    // more native webrtc stuff
    std::unique_ptr<webrtc::TaskQueueFactory>                  mTaskQueueFactory;


    // Devices
    void updateDevices();
    void deployDevices();
    bool                                                       mDevicesDeploying;
    webrtc::scoped_refptr<LLWebRTCAudioDeviceModule>              mDeviceModule;
    std::vector<LLWebRTCDevicesObserver *>                     mVoiceDevicesObserverList;

    // accessors in native webrtc for devices aren't apparently implemented yet.
    bool                                                       mTuningMode;
    int32_t                                                    mRecordingDevice;
    LLWebRTCVoiceDeviceList                                    mRecordingDeviceList;

    int32_t                                                    mPlayoutDevice;
    LLWebRTCVoiceDeviceList                                    mPlayoutDeviceList;

    bool                                                       mMute;

    LLCustomProcessorStatePtr                                  mPeerCustomProcessor;

    // peer connections
    std::vector<webrtc::scoped_refptr<LLWebRTCPeerConnectionImpl>> mPeerConnections;
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
    virtual webrtc::RefCountReleaseStatus Release() const override = 0;

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
    void OnAddTrack(webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                    const std::vector<webrtc::scoped_refptr<webrtc::MediaStreamInterface>> &streams) override;
    void OnRemoveTrack(webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
    void OnDataChannel(webrtc::scoped_refptr<webrtc::DataChannelInterface> channel) override;
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

    webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> mPeerConnectionFactory;

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

    webrtc::scoped_refptr<webrtc::PeerConnectionInterface> mPeerConnection;
    webrtc::scoped_refptr<webrtc::MediaStreamInterface> mLocalStream;

    // data
    std::vector<LLWebRTCDataObserver *> mDataObserverList;
    webrtc::scoped_refptr<webrtc::DataChannelInterface> mDataChannel;
};

}

#if WEBRTC_WIN
#pragma warning(pop)
#endif

#endif // LLWEBRTC_IMPL_H
