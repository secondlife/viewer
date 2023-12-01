/**
 * @file llwebrtc.cpp
 * @brief WebRTC interface implementation
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

#include "llwebrtc_impl.h"
#include <algorithm>
#include <format>

#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/media_stream_interface.h"
#include "api/media_stream_track.h"

namespace llwebrtc
{

const float VOLUME_SCALE_WEBRTC = 100;

LLAudioDeviceObserver::LLAudioDeviceObserver() : mMicrophoneEnergy(0.0), mSumVector {0} {}

float LLAudioDeviceObserver::getMicrophoneEnergy() { return mMicrophoneEnergy; }

void LLAudioDeviceObserver::OnCaptureData(const void    *audio_samples,
                                          const size_t   num_samples,
                                          const size_t   bytes_per_sample,
                                          const size_t   num_channels,
                                          const uint32_t samples_per_sec)
{
    float        energy  = 0;
    const short *samples = (const short *) audio_samples;
    for (size_t index = 0; index < num_samples * num_channels; index++)
    {
        float sample = (static_cast<float>(samples[index]) / (float) 32768);
        energy += sample * sample;
    }
    
    // smooth it.
    size_t buffer_size = sizeof(mSumVector) / sizeof(mSumVector[0]);
    float  totalSum    = 0;
    int    i;
    for (i = 0; i < (buffer_size - 1); i++)
    {
        mSumVector[i] = mSumVector[i + 1];
        totalSum += mSumVector[i];
    }
    mSumVector[i] = energy;
    totalSum += energy;
    mMicrophoneEnergy = std::sqrt(totalSum / (num_samples * buffer_size));
}

void LLAudioDeviceObserver::OnRenderData(const void    *audio_samples,
                                         const size_t   num_samples,
                                         const size_t   bytes_per_sample,
                                         const size_t   num_channels,
                                         const uint32_t samples_per_sec)
{
}

void LLWebRTCImpl::init()
{
    RTC_DCHECK(mPeerConnectionFactory);
    mPlayoutDevice  = -1;
    mRecordingDevice = -1;
    rtc::InitializeSSL();
    mTaskQueueFactory = webrtc::CreateDefaultTaskQueueFactory();
    
    mNetworkThread = rtc::Thread::CreateWithSocketServer();
    mNetworkThread->SetName("WebRTCNetworkThread", nullptr);
    mNetworkThread->Start();
    mWorkerThread = rtc::Thread::Create();
    mWorkerThread->SetName("WebRTCWorkerThread", nullptr);
    mWorkerThread->Start();
    mSignalingThread = rtc::Thread::Create();
    mSignalingThread->SetName("WebRTCSignalingThread", nullptr);
    mSignalingThread->Start();
    
    mWorkerThread->PostTask(
                            [this]()
                            {
                                mTuningAudioDeviceObserver = new LLAudioDeviceObserver;
                                mTuningDeviceModule =
                                webrtc::CreateAudioDeviceWithDataObserver(webrtc::AudioDeviceModule::AudioLayer::kPlatformDefaultAudio,
                                                                          mTaskQueueFactory.get(),
                                                                          std::unique_ptr<webrtc::AudioDeviceDataObserver>(mTuningAudioDeviceObserver));
                                mTuningDeviceModule->Init();
                                mTuningDeviceModule->SetStereoRecording(false);
                                mTuningDeviceModule->SetStereoPlayout(true);
                                mTuningDeviceModule->EnableBuiltInAEC(false);
                                updateDevices();
                            });
    
    rtc::scoped_refptr<webrtc::AudioProcessing> apm = webrtc::AudioProcessingBuilder().Create();
    webrtc::AudioProcessing::Config             apm_config;
    apm_config.echo_canceller.enabled         = false;
    apm_config.echo_canceller.mobile_mode     = false;
    apm_config.gain_controller1.enabled       = true;
    apm_config.gain_controller1.mode          = webrtc::AudioProcessing::Config::GainController1::kAdaptiveAnalog;
    apm_config.gain_controller2.enabled       = true;
    apm_config.high_pass_filter.enabled       = true;
    apm_config.noise_suppression.enabled      = true;
    apm_config.noise_suppression.level        = webrtc::AudioProcessing::Config::NoiseSuppression::kVeryHigh;
    apm_config.transient_suppression.enabled  = true;
    apm_config.pipeline.multi_channel_render  = true;
    apm_config.pipeline.multi_channel_capture = true;
    
    mWorkerThread->BlockingCall(
                                [this]()
                                {
                                    mPeerAudioDeviceObserver = new LLAudioDeviceObserver;
                                    mPeerDeviceModule =
                                    webrtc::CreateAudioDeviceWithDataObserver(webrtc::AudioDeviceModule::AudioLayer::kPlatformDefaultAudio,
                                                                              mTaskQueueFactory.get(),
                                                                              std::unique_ptr<webrtc::AudioDeviceDataObserver>(mPeerAudioDeviceObserver));
                                    mPeerDeviceModule->Init();
                                    mPeerDeviceModule->SetPlayoutDevice(mPlayoutDevice);
                                    mPeerDeviceModule->SetRecordingDevice(mRecordingDevice);
                                    mPeerDeviceModule->SetStereoRecording(false);
                                    mPeerDeviceModule->SetStereoPlayout(true);
                                    mPeerDeviceModule->EnableBuiltInAEC(false);
                                    mPeerDeviceModule->InitMicrophone();
                                    mPeerDeviceModule->InitSpeaker();
                                    mPeerDeviceModule->InitRecording();
                                    mPeerDeviceModule->InitPlayout();
                                    mPeerDeviceModule->StartPlayout();
                                    mPeerDeviceModule->StartRecording();
                                });
    
    mPeerConnectionFactory = webrtc::CreatePeerConnectionFactory(mNetworkThread.get(),
                                                                 mWorkerThread.get(),
                                                                 mSignalingThread.get(),
                                                                 mPeerDeviceModule,
                                                                 webrtc::CreateBuiltinAudioEncoderFactory(),
                                                                 webrtc::CreateBuiltinAudioDecoderFactory(),
                                                                 nullptr /* video_encoder_factory */,
                                                                 nullptr /* video_decoder_factory */,
                                                                 nullptr /* audio_mixer */,
                                                                 apm);
    apm->ApplyConfig(apm_config);
}

void LLWebRTCImpl::terminate()
{
    for (auto& connection : mPeerConnections)
    {
        connection->terminate();
    }
    mPeerConnections.clear();

    mSignalingThread->BlockingCall(
                                   [this]()
                                   {
                                       mPeerConnectionFactory = nullptr;
                                   });
    mWorkerThread->BlockingCall(
                                [this]()
                                {
                                    if (mTuningDeviceModule)
                                    {
                                        mTuningDeviceModule->StopRecording();
                                        mTuningDeviceModule->Terminate();
                                    }
                                    if (mPeerDeviceModule)
                                    {
                                        mPeerDeviceModule->StopRecording();
                                        mPeerDeviceModule->Terminate();
                                    }
                                    mTuningDeviceModule = nullptr;
                                    mPeerDeviceModule = nullptr;
                                    mTaskQueueFactory   = nullptr;
                                });
}

void LLWebRTCImpl::refreshDevices()
{
    mWorkerThread->PostTask([this]() { updateDevices(); });
}

void LLWebRTCImpl::setDevicesObserver(LLWebRTCDevicesObserver *observer) { mVoiceDevicesObserverList.emplace_back(observer); }

void LLWebRTCImpl::unsetDevicesObserver(LLWebRTCDevicesObserver *observer)
{
    std::vector<LLWebRTCDevicesObserver *>::iterator it =
    std::find(mVoiceDevicesObserverList.begin(), mVoiceDevicesObserverList.end(), observer);
    if (it != mVoiceDevicesObserverList.end())
    {
        mVoiceDevicesObserverList.erase(it);
    }
}

void LLWebRTCImpl::setCaptureDevice(const std::string &id)
{
    mWorkerThread->PostTask(
                            [this, id]()
                            {
                                int16_t tuningRecordingDevice = 0;
                                int16_t captureDeviceCount = mTuningDeviceModule->RecordingDevices();
                                for (int16_t i = 0; i < captureDeviceCount; i++)
                                {
                                    char name[webrtc::kAdmMaxDeviceNameSize];
                                    char guid[webrtc::kAdmMaxGuidSize];
                                    mTuningDeviceModule->RecordingDeviceName(i, name, guid);
                                    if (id == guid || id == "Default")  // first one in list is default
                                    {
                                        RTC_LOG(LS_INFO) << __FUNCTION__ << "Set recording device to " << name << " " << guid << " " << i;
                                        tuningRecordingDevice = i;
                                        break;
                                    }
                                }
                                mTuningDeviceModule->StopRecording();
                                mTuningDeviceModule->SetRecordingDevice(tuningRecordingDevice);
                                mTuningDeviceModule->InitMicrophone();
                                mTuningDeviceModule->InitRecording();
                                mTuningDeviceModule->StartRecording();
                                bool was_peer_recording = false;
                                if (mPeerDeviceModule)
                                {
                                    int16_t captureDeviceCount = mTuningDeviceModule->RecordingDevices();
                                    for (int16_t i = 0; i < captureDeviceCount; i++)
                                    {
                                        char name[webrtc::kAdmMaxDeviceNameSize];
                                        char guid[webrtc::kAdmMaxGuidSize];
                                        mTuningDeviceModule->RecordingDeviceName(i, name, guid);
                                        if (id == guid || id == "Default")  // first one in list is default
                                        {
                                            RTC_LOG(LS_INFO)
                                                << __FUNCTION__ << "Set recording device to " << name << " " << guid << " " << i;
                                            mRecordingDevice = i;
                                            break;
                                        }
                                    }
                                    was_peer_recording = mPeerDeviceModule->Recording();
                                    if (was_peer_recording)
                                    {
                                        mPeerDeviceModule->StopRecording();
                                    }
                                    mPeerDeviceModule->SetRecordingDevice(mRecordingDevice);
                                    mPeerDeviceModule->InitMicrophone();
                                    mPeerDeviceModule->InitRecording();
                                    if (was_peer_recording)
                                    {
                                        mPeerDeviceModule->StartRecording();
                                    }
                                }
                            });
}

void LLWebRTCImpl::setRenderDevice(const std::string &id)
{
    mWorkerThread->PostTask(
                            [this, id]()
                            {
                                int16_t renderDeviceCount = mTuningDeviceModule->PlayoutDevices();
                                int16_t tuningPlayoutDevice = 0;
                                for (int16_t i = 0; i < renderDeviceCount; i++)
                                {
                                    char name[webrtc::kAdmMaxDeviceNameSize];
                                    char guid[webrtc::kAdmMaxGuidSize];
                                    mTuningDeviceModule->PlayoutDeviceName(i, name, guid);
                                    if (id == guid || id == "Default")
                                    {
                                        RTC_LOG(LS_INFO) << __FUNCTION__ << "Set recording device to " << name << " " << guid << " " << i;
                                        tuningPlayoutDevice = i;
                                        break;
                                    }
                                }
                                mTuningDeviceModule->SetSpeakerMute(true);
                                bool was_tuning_playing = mTuningDeviceModule->Playing();
                                if (was_tuning_playing)
                                {
                                    mTuningDeviceModule->StopPlayout();
                                }
                                if (mPeerDeviceModule)
                                {
                                    mPeerDeviceModule->SetSpeakerMute(true);
                                }
                                
                                mTuningDeviceModule->SetPlayoutDevice(tuningPlayoutDevice);
                                mTuningDeviceModule->InitSpeaker();
                                mTuningDeviceModule->InitPlayout();
                                if (was_tuning_playing)
                                {
                                    mTuningDeviceModule->StartPlayout();
                                }
                                renderDeviceCount = mPeerDeviceModule->PlayoutDevices();

                                if (mPeerDeviceModule)
                                {
                                    for (int16_t i = 0; i < renderDeviceCount; i++)
                                    {
                                        char name[webrtc::kAdmMaxDeviceNameSize];
                                        char guid[webrtc::kAdmMaxGuidSize];
                                        mPeerDeviceModule->PlayoutDeviceName(i, name, guid);
                                        if (id == guid || id == "Default")
                                        {
                                            RTC_LOG(LS_INFO)
                                                << __FUNCTION__ << "Set recording device to " << name << " " << guid << " " << i;
                                            mPlayoutDevice = i;
                                            break;
                                        }
                                    }
                                    mPeerDeviceModule->SetPlayoutDevice(mPlayoutDevice);
                                    mPeerDeviceModule->InitSpeaker();
                                    mPeerDeviceModule->InitPlayout();
                                    mPeerDeviceModule->StartPlayout();
                                    mPeerDeviceModule->SetSpeakerMute(false);
                                    
                                }
                                mTuningDeviceModule->SetSpeakerMute(false);
                            });
}

void LLWebRTCImpl::updateDevices()
{
    int16_t                 renderDeviceCount = mTuningDeviceModule->PlayoutDevices();
    LLWebRTCVoiceDeviceList renderDeviceList;
    for (int16_t index = 0; index < renderDeviceCount; index++)
    {
        char name[webrtc::kAdmMaxDeviceNameSize];
        char guid[webrtc::kAdmMaxGuidSize];
        mTuningDeviceModule->PlayoutDeviceName(index, name, guid);
        renderDeviceList.emplace_back(name, guid);
    }
    
    int16_t                 captureDeviceCount = mTuningDeviceModule->RecordingDevices();
    LLWebRTCVoiceDeviceList captureDeviceList;
    for (int16_t index = 0; index < captureDeviceCount; index++)
    {
        char name[webrtc::kAdmMaxDeviceNameSize];
        char guid[webrtc::kAdmMaxGuidSize];
        mTuningDeviceModule->RecordingDeviceName(index, name, guid);
        captureDeviceList.emplace_back(name, guid);
    }
    for (auto &observer : mVoiceDevicesObserverList)
    {
        observer->OnDevicesChanged(renderDeviceList, captureDeviceList);
    }
}

void LLWebRTCImpl::setTuningMode(bool enable)
{
    mWorkerThread->BlockingCall(
                                [this, enable]()
                                {
                                    if (enable)
                                    {
                                        
                                        mTuningDeviceModule->StartRecording();
                                        mTuningDeviceModule->SetMicrophoneMute(false);
                                        
                                        
                                        mTuningDeviceModule->SetSpeakerMute(false);
                                        
                                        if (mPeerDeviceModule)
                                        {
                                            mPeerDeviceModule->StopRecording();
                                            mPeerDeviceModule->SetSpeakerMute(true);
                                        }
                                    }
                                    else
                                    {
                                        if (mPeerDeviceModule)
                                        {
                                            mPeerDeviceModule->StartRecording();
                                            mPeerDeviceModule->SetSpeakerMute(false);
                                        }
                                    }
                                });
    for (auto& connection : mPeerConnections)
    {
        connection->enableTracks(enable ? false : !mMute);
    }
}

float LLWebRTCImpl::getTuningAudioLevel() { return 20 * mTuningAudioDeviceObserver->getMicrophoneEnergy(); }

float LLWebRTCImpl::getPeerAudioLevel() { return 20 * mPeerAudioDeviceObserver->getMicrophoneEnergy(); }

//
// Helpers
//

LLWebRTCPeerConnection * LLWebRTCImpl::newPeerConnection()
{
    rtc::scoped_refptr<LLWebRTCPeerConnectionImpl> peerConnection = rtc::scoped_refptr<LLWebRTCPeerConnectionImpl>(new rtc::RefCountedObject<LLWebRTCPeerConnectionImpl>());
    peerConnection->init(this);
        
    mPeerConnections.emplace_back(peerConnection);
    peerConnection->enableTracks(!mMute);
    return peerConnection.get();
}

void LLWebRTCImpl::freePeerConnection(LLWebRTCPeerConnection * peer_connection)
{
    std::vector<rtc::scoped_refptr<LLWebRTCPeerConnectionImpl>>::iterator it =
    std::find(mPeerConnections.begin(), mPeerConnections.end(), peer_connection);
    if (it != mPeerConnections.end())
    {
        (*it)->terminate();
        mPeerConnections.erase(it);
    }
}

//
// LLWebRTCPeerConnection interface
//

void LLWebRTCPeerConnectionImpl::init(LLWebRTCImpl * webrtc_impl)
{
    mWebRTCImpl = webrtc_impl;
    mPeerConnectionFactory = mWebRTCImpl->getPeerConnectionFactory();
}
void LLWebRTCPeerConnectionImpl::terminate()
{
    shutdownConnection();
}

void LLWebRTCPeerConnectionImpl::setSignalingObserver(LLWebRTCSignalingObserver *observer) { mSignalingObserverList.emplace_back(observer); }

void LLWebRTCPeerConnectionImpl::unsetSignalingObserver(LLWebRTCSignalingObserver *observer)
{
    std::vector<LLWebRTCSignalingObserver *>::iterator it =
    std::find(mSignalingObserverList.begin(), mSignalingObserverList.end(), observer);
    if (it != mSignalingObserverList.end())
    {
        mSignalingObserverList.erase(it);
    }
}

bool LLWebRTCPeerConnectionImpl::initializeConnection()
{
    RTC_DCHECK(!mPeerConnection);
    mAnswerReceived = false;

    webrtc::PeerConnectionInterface::RTCConfiguration config;
    config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
    webrtc::PeerConnectionInterface::IceServer server;
    server.uri = "stun:roxie-turn.staging.secondlife.io:3478";
    config.servers.push_back(server);
    server.uri = "stun:stun.l.google.com:19302";
    config.servers.push_back(server);
    server.uri = "stun:stun1.l.google.com:19302";
    config.servers.push_back(server);
    server.uri = "stun:stun2.l.google.com:19302";
    config.servers.push_back(server);
    server.uri = "stun:stun3.l.google.com:19302";
    config.servers.push_back(server);
    server.uri = "stun:stun4.l.google.com:19302";
    config.servers.push_back(server);
    
    webrtc::PeerConnectionDependencies pc_dependencies(this);
    auto error_or_peer_connection = mPeerConnectionFactory->CreatePeerConnectionOrError(config, std::move(pc_dependencies));
    if (error_or_peer_connection.ok())
    {
        mPeerConnection = std::move(error_or_peer_connection.value());
    }
    else
    {
        RTC_LOG(LS_ERROR) << __FUNCTION__ << "Error creating peer connection: " << error_or_peer_connection.error().message();
        shutdownConnection();
        return false;
    }
    
    webrtc::DataChannelInit init;
    init.ordered = true;
    
    auto data_channel_or_error = mPeerConnection->CreateDataChannelOrError("SLData", &init);
    if (data_channel_or_error.ok())
    {
        mDataChannel = std::move(data_channel_or_error.value());
        
        mDataChannel->RegisterObserver(this);
    }
    
    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << mPeerConnection->signaling_state();
    
    cricket::AudioOptions audioOptions;
    audioOptions.auto_gain_control = true;
    audioOptions.echo_cancellation = false;  // incompatible with opus stereo
    audioOptions.noise_suppression = true;
    
    mLocalStream = mPeerConnectionFactory->CreateLocalMediaStream("SLStream");
    rtc::scoped_refptr<webrtc::AudioTrackInterface>  audio_track(
                                                                 mPeerConnectionFactory->CreateAudioTrack("SLAudio", mPeerConnectionFactory->CreateAudioSource(audioOptions).get()));
    audio_track->set_enabled(true);
    mLocalStream->AddTrack(audio_track);
    
    mPeerConnection->AddTrack(audio_track, {"SLStream"});
    
    auto senders = mPeerConnection->GetSenders();
    
    for (auto &sender : senders)
    {
        webrtc::RtpParameters      params;
        webrtc::RtpCodecParameters codecparam;
        codecparam.name                       = "opus";
        codecparam.kind                       = cricket::MEDIA_TYPE_AUDIO;
        codecparam.clock_rate                 = 48000;
        codecparam.num_channels               = 2;
        codecparam.parameters["stereo"]       = "1";
        codecparam.parameters["sprop-stereo"] = "1";
        params.codecs.push_back(codecparam);
        sender->SetParameters(params);
    }
    
    auto receivers = mPeerConnection->GetReceivers();
    for (auto &receiver : receivers)
    {
        webrtc::RtpParameters      params;
        webrtc::RtpCodecParameters codecparam;
        codecparam.name                       = "opus";
        codecparam.kind                       = cricket::MEDIA_TYPE_AUDIO;
        codecparam.clock_rate                 = 48000;
        codecparam.num_channels               = 2;
        codecparam.parameters["stereo"]       = "1";
        codecparam.parameters["sprop-stereo"] = "1";
        params.codecs.push_back(codecparam);
        receiver->SetParameters(params);
    }
    
    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions offerOptions;
    mPeerConnection->CreateOffer(this, offerOptions);
    
    return true;
}

void LLWebRTCPeerConnectionImpl::shutdownConnection()
{
    mWebRTCImpl->SignalingBlockingCall(
                                   [this]()
                                   {
                                       if (mPeerConnection)
                                       {
                                           mPeerConnection->Close();
                                           mPeerConnection = nullptr;
                                       }
                                   });

    mWebRTCImpl->NetworkBlockingCall(
                                 [this]()
                                 {
                                     if (mDataChannel)
                                     {
                                         mDataChannel->Close();
                                         mDataChannel = nullptr;
                                     }
                                 });
}

void LLWebRTCPeerConnectionImpl::enableTracks(bool enable)
{
    // set_enabled shouldn't be done on the worker thread
    if (mPeerConnection)
    {
        auto senders = mPeerConnection->GetSenders();
        for (auto &sender : senders)
        {
            sender->track()->set_enabled(enable);
        }
    }
}

void LLWebRTCPeerConnectionImpl::AnswerAvailable(const std::string &sdp)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << " Remote SDP: " << sdp;
    
    mWebRTCImpl->PostSignalingTask(
                               [this, sdp]()
                               {
                                   if (mPeerConnection)
                                   {
                                       RTC_LOG(LS_INFO) << __FUNCTION__ << " " << mPeerConnection->peer_connection_state();
                                       mPeerConnection->SetRemoteDescription(webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp),
                                                                             rtc::scoped_refptr<webrtc::SetRemoteDescriptionObserverInterface>(this));
                                   }                   
                               });
}

void LLWebRTCPeerConnectionImpl::setMute(bool mute)
{
    mMute = mute;
    if (mPeerConnection)
    {
        auto senders = mPeerConnection->GetSenders();
        
        RTC_LOG(LS_INFO) << __FUNCTION__ << (mute ? "disabling" : "enabling") << " streams count " << senders.size();
        for (auto &sender : senders)
        {
            auto track = sender->track();
            if(track)
            {
                track->set_enabled(!mMute);
            }
        }
    }
}

void LLWebRTCPeerConnectionImpl::setReceiveVolume(float volume)
{
    mWebRTCImpl->PostSignalingTask(
        [this, volume]()
        {
            auto receivers = mPeerConnection->GetReceivers();

            for (auto &receiver : receivers)
            {
                for (auto &stream : receiver->streams())
                {
                    for (auto &track : stream->GetAudioTracks())
                    {
                        track->GetSource()->SetVolume(volume);
                    }
                }
            }
        });
}

void LLWebRTCPeerConnectionImpl::setSendVolume(float volume)
{
    if (mLocalStream)
    {
        for (auto &track : mLocalStream->GetAudioTracks())
        {
            track->GetSource()->SetVolume(volume);
        }
    }
}

//
// PeerConnectionObserver implementation.
//

void LLWebRTCPeerConnectionImpl::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface>                     receiver,
                                            const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &streams)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << receiver->id();
    webrtc::RtpParameters      params;
    webrtc::RtpCodecParameters codecparam;
    codecparam.name                       = "opus";
    codecparam.kind                       = cricket::MEDIA_TYPE_AUDIO;
    codecparam.clock_rate                 = 48000;
    codecparam.num_channels               = 2;
    codecparam.parameters["stereo"]       = "1";
    codecparam.parameters["sprop-stereo"] = "1";
    params.codecs.push_back(codecparam);
    receiver->SetParameters(params);
}

void LLWebRTCPeerConnectionImpl::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << receiver->id();
}

void LLWebRTCPeerConnectionImpl::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel)
{
    mDataChannel = channel;
    channel->RegisterObserver(this);
}

void LLWebRTCPeerConnectionImpl::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state)
{
    LLWebRTCSignalingObserver::IceGatheringState webrtc_new_state = LLWebRTCSignalingObserver::IceGatheringState::ICE_GATHERING_NEW;
    switch (new_state)
    {
        case webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringNew:
            webrtc_new_state = LLWebRTCSignalingObserver::IceGatheringState::ICE_GATHERING_NEW;
            break;
        case webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringGathering:
            webrtc_new_state = LLWebRTCSignalingObserver::IceGatheringState::ICE_GATHERING_GATHERING;
            break;
        case webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringComplete:
            webrtc_new_state = LLWebRTCSignalingObserver::IceGatheringState::ICE_GATHERING_COMPLETE;
            break;
        default:
            RTC_LOG(LS_ERROR) << __FUNCTION__ << " Bad Ice Gathering State" << new_state;
            webrtc_new_state = LLWebRTCSignalingObserver::IceGatheringState::ICE_GATHERING_NEW;
            return;
    }
    
    if (mAnswerReceived)
    {
        for (auto &observer : mSignalingObserverList)
        {
            observer->OnIceGatheringState(webrtc_new_state);
        }
    }
}

// Called any time the PeerConnectionState changes.
void LLWebRTCPeerConnectionImpl::OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState new_state)
{
    RTC_LOG(LS_ERROR) << __FUNCTION__ << " Peer Connection State Change " << new_state;
    
    switch (new_state)
    {
        case webrtc::PeerConnectionInterface::PeerConnectionState::kConnected:
        {
            if (new_state == webrtc::PeerConnectionInterface::PeerConnectionState::kConnected)
            {
                mWebRTCImpl->PostWorkerTask([this]() {
                    for (auto &observer : mSignalingObserverList)
                    {
                        observer->OnAudioEstablished(this);
                    }
                });
            }
            break;
        }
        case webrtc::PeerConnectionInterface::PeerConnectionState::kFailed:
        case webrtc::PeerConnectionInterface::PeerConnectionState::kDisconnected:
        {
            for (auto &observer : mSignalingObserverList)
            {
                observer->OnRenegotiationNeeded();
            }
            
            break;
        }
        default:
        {
            break;
        }
    }
}

static std::string iceCandidateToTrickleString(const webrtc::IceCandidateInterface *candidate)
{
    std::ostringstream candidate_stream;
    
    candidate_stream <<
    candidate->candidate().foundation() << " " <<
    std::to_string(candidate->candidate().component()) << " " <<
    candidate->candidate().protocol() << " " <<
    std::to_string(candidate->candidate().priority()) << " " <<
    candidate->candidate().address().ipaddr().ToString() << " " <<
    candidate->candidate().address().PortAsString() << " typ ";
    
    if (candidate->candidate().type() == cricket::LOCAL_PORT_TYPE)
    {
        candidate_stream << "host";
    }
    else if (candidate->candidate().type() == cricket::STUN_PORT_TYPE)
    {
        candidate_stream << "srflx " <<
        "raddr " << candidate->candidate().related_address().ipaddr().ToString() << " " <<
        "rport " << candidate->candidate().related_address().PortAsString();
    }
    else if (candidate->candidate().type() == cricket::RELAY_PORT_TYPE)
    {
        candidate_stream << "relay " <<
        "raddr " << candidate->candidate().related_address().ipaddr().ToString() << " " <<
        "rport " << candidate->candidate().related_address().PortAsString();
    }
    else if (candidate->candidate().type() == cricket::PRFLX_PORT_TYPE)
    {
        candidate_stream << "prflx " <<
        "raddr " << candidate->candidate().related_address().ipaddr().ToString() << " " <<
        "rport " << candidate->candidate().related_address().PortAsString();
    }
    else {
        RTC_LOG(LS_ERROR) << __FUNCTION__ << " Unknown candidate type " << candidate->candidate().type();
    }
    if (candidate->candidate().protocol() == "tcp")
    {
        candidate_stream << " tcptype " << candidate->candidate().tcptype();
    }
    
    return candidate_stream.str();
}

void LLWebRTCPeerConnectionImpl::OnIceCandidate(const webrtc::IceCandidateInterface *candidate)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << candidate->sdp_mline_index();
    
    if (!candidate)
    {
        RTC_LOG(LS_ERROR) << __FUNCTION__ << " No Ice Candidate Given";
        return;
    }
    if (mAnswerReceived)
    {
        for (auto &observer : mSignalingObserverList)
        {
            LLWebRTCIceCandidate ice_candidate;
            ice_candidate.candidate   = iceCandidateToTrickleString(candidate);
            ice_candidate.mline_index = candidate->sdp_mline_index();
            ice_candidate.sdp_mid     = candidate->sdp_mid();
            observer->OnIceCandidate(ice_candidate);
        }
    }
    else
    {
        mCachedIceCandidates.push_back(
                                       webrtc::CreateIceCandidate(candidate->sdp_mid(), candidate->sdp_mline_index(), candidate->candidate()));
    }
}

//
// CreateSessionDescriptionObserver implementation.
//
void LLWebRTCPeerConnectionImpl::OnSuccess(webrtc::SessionDescriptionInterface *desc)
{
    std::string sdp;
    desc->ToString(&sdp);
    RTC_LOG(LS_INFO) << sdp;
    ;
    // mangle the sdp as this is the only way currently to bump up
    // the send audio rate to 48k
    std::istringstream sdp_stream(sdp);
    std::ostringstream sdp_mangled_stream;
    std::string        sdp_line;
    std::string opus_payload;
    while (std::getline(sdp_stream, sdp_line))
    {
        int bandwidth  = 0;
        int payload_id = 0;
        // force mono down, stereo up
        if (std::sscanf(sdp_line.c_str(), "a=rtpmap:%i opus/%i/2", &payload_id, &bandwidth) == 2)
        {
            sdp_mangled_stream << sdp_line << "\n";
            opus_payload = std::to_string(payload_id);
        }
        else if (sdp_line.find("a=fmtp:" + opus_payload) == 0)
        {
            sdp_mangled_stream << sdp_line << "a=fmtp:" << opus_payload
            << " minptime=10;useinbandfec=1;stereo=1;sprop-stereo=1;maxplaybackrate=48000\n";
        }
        else
        {
            sdp_mangled_stream << sdp_line << "\n";
        }
    }
    
    webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, sdp_mangled_stream.str());
    
    
    
    mPeerConnection->SetLocalDescription(std::unique_ptr<webrtc::SessionDescriptionInterface>(
                                                                                              webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, sdp_mangled_stream.str())),
                                         rtc::scoped_refptr<webrtc::SetLocalDescriptionObserverInterface>(this));
    RTC_LOG(LS_INFO) << __FUNCTION__ << " Local SDP: " << sdp_mangled_stream.str();
    
    
    for (auto &observer : mSignalingObserverList)
    {
        observer->OnOfferAvailable(sdp_mangled_stream.str());
    }
}

void LLWebRTCPeerConnectionImpl::OnFailure(webrtc::RTCError error) { RTC_LOG(LS_ERROR) << ToString(error.type()) << ": " << error.message(); }

//
// SetRemoteDescriptionObserverInterface implementation.
//
void LLWebRTCPeerConnectionImpl::OnSetRemoteDescriptionComplete(webrtc::RTCError error)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << mPeerConnection->signaling_state();
    if (!error.ok())
    {
        RTC_LOG(LS_ERROR) << ToString(error.type()) << ": " << error.message();
        return;
    }
    mAnswerReceived = true;
    for (auto &observer : mSignalingObserverList)
    {
        for (auto &candidate : mCachedIceCandidates)
        {
            LLWebRTCIceCandidate ice_candidate;
            ice_candidate.candidate   = iceCandidateToTrickleString(candidate.get());
            ice_candidate.mline_index = candidate->sdp_mline_index();
            ice_candidate.sdp_mid     = candidate->sdp_mid();
            observer->OnIceCandidate(ice_candidate);
        }
    }
    mCachedIceCandidates.clear();
    OnIceGatheringChange(mPeerConnection->ice_gathering_state());
    
}

//
// SetLocalDescriptionObserverInterface implementation.
//
void LLWebRTCPeerConnectionImpl::OnSetLocalDescriptionComplete(webrtc::RTCError error)
{
    
}

//
// DataChannelObserver implementation
//

void LLWebRTCPeerConnectionImpl::OnStateChange()
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << " Data Channel State: " << webrtc::DataChannelInterface::DataStateString(mDataChannel->state());
    switch (mDataChannel->state())
    {
        case webrtc::DataChannelInterface::kOpen:
            RTC_LOG(LS_INFO) << __FUNCTION__ << " Data Channel State Open";
            for (auto &observer : mSignalingObserverList)
            {
                observer->OnDataChannelReady(this);
            }
            break;
        case webrtc::DataChannelInterface::kConnecting:
            RTC_LOG(LS_INFO) << __FUNCTION__ << " Data Channel State Connecting";
            break;
        case webrtc::DataChannelInterface::kClosing:
            RTC_LOG(LS_INFO) << __FUNCTION__ << " Data Channel State closing";
            break;
        case webrtc::DataChannelInterface::kClosed:
            RTC_LOG(LS_INFO) << __FUNCTION__ << " Data Channel State closed";
            break;
        default:
            break;
    }
}


void LLWebRTCPeerConnectionImpl::OnMessage(const webrtc::DataBuffer& buffer)
{
    std::string data((const char*)buffer.data.cdata(), buffer.size());
    for (auto &observer : mDataObserverList)
    {
        observer->OnDataReceived(data, buffer.binary);
    }
}

void LLWebRTCPeerConnectionImpl::sendData(const std::string& data, bool binary)
{
    if (mDataChannel)
    {
        rtc::CopyOnWriteBuffer cowBuffer(data.data(), data.length());
        webrtc::DataBuffer     buffer(cowBuffer, binary);
        mDataChannel->Send(buffer);
    }
}

void LLWebRTCPeerConnectionImpl::setDataObserver(LLWebRTCDataObserver* observer) { mDataObserverList.emplace_back(observer); }

void LLWebRTCPeerConnectionImpl::unsetDataObserver(LLWebRTCDataObserver* observer)
{
    std::vector<LLWebRTCDataObserver *>::iterator it =
    std::find(mDataObserverList.begin(), mDataObserverList.end(), observer);
    if (it != mDataObserverList.end())
    {
        mDataObserverList.erase(it);
    }
}

LLWebRTCImpl * gWebRTCImpl = nullptr;
LLWebRTCDeviceInterface * getDeviceInterface()
{
    return gWebRTCImpl;
}

LLWebRTCPeerConnection * newPeerConnection()
{
    return gWebRTCImpl->newPeerConnection();
}

void freePeerConnection(LLWebRTCPeerConnection *peer_connection)
{
    gWebRTCImpl->freePeerConnection(peer_connection);
}


void init()
{
    gWebRTCImpl =  new LLWebRTCImpl();
    gWebRTCImpl->init();
}

void terminate()
{
    if (gWebRTCImpl)
    {
        gWebRTCImpl->terminate();
        gWebRTCImpl = nullptr;
    }
}

}  // namespace llwebrtc
