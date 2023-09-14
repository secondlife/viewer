/**
 * @file llaccordionctrl.cpp
 * @brief Accordion panel  implementation
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

#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/media_stream_interface.h"
#include "api/media_stream_track.h"

namespace llwebrtc
{

void LLWebRTCImpl::init()
{
    mAnswerReceived = false;
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

    mSignalingThread->PostTask(
        [this]()
        {
            mDeviceModule = webrtc::CreateAudioDeviceWithDataObserver(webrtc::AudioDeviceModule::AudioLayer::kPlatformDefaultAudio,
                                                                      mTaskQueueFactory.get(),
                                                                      std::unique_ptr<webrtc::AudioDeviceDataObserver>(this));
            mDeviceModule->Init();
            updateDevices();
        });
}

void LLWebRTCImpl::refreshDevices()
{
    mSignalingThread->PostTask([this]() { updateDevices(); });
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
    mSignalingThread->PostTask(
        [this, id]()
        {
            int16_t captureDeviceCount = mDeviceModule->RecordingDevices();
            for (int16_t index = 0; index < captureDeviceCount; index++)
            {
                char name[webrtc::kAdmMaxDeviceNameSize];
                char guid[webrtc::kAdmMaxGuidSize];
                mDeviceModule->RecordingDeviceName(index, name, guid);
                if (id == guid || id == name)
                {
                    mDeviceModule->SetRecordingDevice(index);
                    break;
                }
            }
        });
}

void LLWebRTCImpl::setRenderDevice(const std::string &id)
{
    mSignalingThread->PostTask(
        [this, id]()
        {
            int16_t renderDeviceCount = mDeviceModule->RecordingDevices();
            for (int16_t index = 0; index < renderDeviceCount; index++)
            {
                char name[webrtc::kAdmMaxDeviceNameSize];
                char guid[webrtc::kAdmMaxGuidSize];
                mDeviceModule->PlayoutDeviceName(index, name, guid);
                if (id == guid || id == name)
                {
                    mDeviceModule->SetPlayoutDevice(index);
                    break;
                }
            }
        });
}

void LLWebRTCImpl::updateDevices()
{
    int16_t                 renderDeviceCount = mDeviceModule->PlayoutDevices();
    LLWebRTCVoiceDeviceList renderDeviceList;
    for (int16_t index = 0; index < renderDeviceCount; index++)
    {
        char name[webrtc::kAdmMaxDeviceNameSize];
        char guid[webrtc::kAdmMaxGuidSize];
        mDeviceModule->PlayoutDeviceName(index, name, guid);
        renderDeviceList.emplace_back(name, guid);
    }
    for (auto &observer : mVoiceDevicesObserverList)
    {
        observer->OnRenderDevicesChanged(renderDeviceList);
    }

    int16_t                 captureDeviceCount = mDeviceModule->RecordingDevices();
    LLWebRTCVoiceDeviceList captureDeviceList;
    for (int16_t index = 0; index < captureDeviceCount; index++)
    {
        char name[webrtc::kAdmMaxDeviceNameSize];
        char guid[webrtc::kAdmMaxGuidSize];
        mDeviceModule->RecordingDeviceName(index, name, guid);
        captureDeviceList.emplace_back(name, guid);
    }
    for (auto &observer : mVoiceDevicesObserverList)
    {
        observer->OnCaptureDevicesChanged(captureDeviceList);
    }
}

void LLWebRTCImpl::setTuningMode(bool enable)
{
    mSignalingThread->PostTask(
        [this, enable]()
        {
            if (enable)
            {
                mDeviceModule->InitMicrophone();
                mDeviceModule->InitRecording();
                mDeviceModule->StartRecording();
                mDeviceModule->SetMicrophoneMute(false);
            }
            else
            {
                mDeviceModule->StopRecording();
            }
        });
}

double LLWebRTCImpl::getTuningMicrophoneEnergy() { return mTuningEnergy; }

void LLWebRTCImpl::OnCaptureData(const void    *audio_samples,
                                 const size_t   num_samples,
                                 const size_t   bytes_per_sample,
                                 const size_t   num_channels,
                                 const uint32_t samples_per_sec)
{
    if (bytes_per_sample != 4)
    {
        return;
    }

    double       energy  = 0;
    const short *samples = (const short *) audio_samples;
    for (size_t index = 0; index < num_samples * num_channels; index++)
    {
        double sample = (static_cast<double>(samples[index]) / (double) 32768);
        energy += sample * sample;
    }
    mTuningEnergy = std::sqrt(energy);
}

void LLWebRTCImpl::OnRenderData(const void    *audio_samples,
                                const size_t   num_samples,
                                const size_t   bytes_per_sample,
                                const size_t   num_channels,
                                const uint32_t samples_per_sec)
{
}

//
// LLWebRTCSignalInterface
//

void LLWebRTCImpl::setSignalingObserver(LLWebRTCSignalingObserver *observer) { mSignalingObserverList.emplace_back(observer); }

void LLWebRTCImpl::unsetSignalingObserver(LLWebRTCSignalingObserver *observer)
{
    std::vector<LLWebRTCSignalingObserver *>::iterator it =
        std::find(mSignalingObserverList.begin(), mSignalingObserverList.end(), observer);
    if (it != mSignalingObserverList.end())
    {
        mSignalingObserverList.erase(it);
    }
}

bool LLWebRTCImpl::initializeConnection()
{
    RTC_DCHECK(!mPeerConnection);
    RTC_DCHECK(!mPeerConnectionFactory);
    mAnswerReceived        = false;
    mPeerConnectionFactory = webrtc::CreatePeerConnectionFactory(mNetworkThread.get(),
                                                                 mWorkerThread.get(),
                                                                 mSignalingThread.get(),
                                                                 nullptr /* default_adm */,
                                                                 webrtc::CreateBuiltinAudioEncoderFactory(),
                                                                 webrtc::CreateBuiltinAudioDecoderFactory(),
                                                                 nullptr /* video_encoder_factory */,
                                                                 nullptr /* video_decoder_factory */,
                                                                 nullptr /* audio_mixer */,
                                                                 nullptr /* audio_processing */);

    if (!mPeerConnectionFactory)
    {
        shutdownConnection();
        return false;
    }

    webrtc::PeerConnectionInterface::RTCConfiguration config;
    config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
    webrtc::PeerConnectionInterface::IceServer server;
    server.uri = "stun:stun.l.google.com:19302";
    // config.servers.push_back(server);
    // server.uri = "stun:stun1.l.google.com:19302";
    // config.servers.push_back(server);
    // server.uri = "stun:stun2.l.google.com:19302";
    // config.servers.push_back(server);
    // server.uri = "stun:stun3.l.google.com:19302";
    // config.servers.push_back(server);
    // server.uri = "stun:stun4.l.google.com:19302";
    // config.servers.push_back(server);

    webrtc::PeerConnectionDependencies pc_dependencies(this);
    auto error_or_peer_connection = mPeerConnectionFactory->CreatePeerConnectionOrError(config, std::move(pc_dependencies));
    if (error_or_peer_connection.ok())
    {
        mPeerConnection = std::move(error_or_peer_connection.value());
    }
    else
    {
        shutdownConnection();
        return false;
    }

    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << mPeerConnection->signaling_state();

    cricket::AudioOptions audioOptions;
    audioOptions.auto_gain_control = true;
    audioOptions.echo_cancellation = true;
    audioOptions.noise_suppression = true;

    rtc::scoped_refptr<webrtc::MediaStreamInterface> stream = mPeerConnectionFactory->CreateLocalMediaStream("SLStream");
    rtc::scoped_refptr<webrtc::AudioTrackInterface>  audio_track(
        mPeerConnectionFactory->CreateAudioTrack("SLAudio", mPeerConnectionFactory->CreateAudioSource(cricket::AudioOptions()).get()));
    audio_track->set_enabled(true);
    stream->AddTrack(audio_track);

    mPeerConnection->AddTrack(audio_track, {"SLStream"});
    mPeerConnection->SetLocalDescription(rtc::scoped_refptr<webrtc::SetLocalDescriptionObserverInterface>(this));

    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << mPeerConnection->signaling_state();

    return mPeerConnection != nullptr;
}

void LLWebRTCImpl::shutdownConnection()
{
    mPeerConnection        = nullptr;
    mPeerConnectionFactory = nullptr;
}

void LLWebRTCImpl::AnswerAvailable(const std::string &sdp)
{
    mSignalingThread->PostTask(
        [this, sdp]()
        {
            RTC_LOG(LS_INFO) << __FUNCTION__ << " " << mPeerConnection->peer_connection_state();
            mPeerConnection->SetRemoteDescription(webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, sdp),
                                                  rtc::scoped_refptr<webrtc::SetRemoteDescriptionObserverInterface>(this));
            mAnswerReceived = true;
            for (auto &observer : mSignalingObserverList)
            {
                for (auto &candidate : mCachedIceCandidates)
                {
                    LLWebRTCIceCandidate ice_candidate;
                    ice_candidate.candidate = candidate->candidate().ToString();
                    ice_candidate.mline_index = candidate->sdp_mline_index();
                    ice_candidate.sdp_mid     = candidate->sdp_mid();
                    observer->OnIceCandidate(ice_candidate);
                }
                mCachedIceCandidates.clear();
                if (mPeerConnection->ice_gathering_state() == webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringComplete)
                {
                    for (auto &observer : mSignalingObserverList)
                    {
                        observer->OnIceGatheringState(llwebrtc::LLWebRTCSignalingObserver::IceGatheringState::ICE_GATHERING_COMPLETE);
                    }
                }
            }
        });
}

void LLWebRTCImpl::setMute(bool mute)
{
    mSignalingThread->PostTask(
        [this,mute]()
        {
            auto senders = mPeerConnection->GetSenders();

            RTC_LOG(LS_INFO) << __FUNCTION__ << (mute ? "disabling" : "enabling") << " streams count "
                             << senders.size();

            for (auto& sender : senders)
            {
                sender->track()->set_enabled(!mute);
            }
        });
}

void LLWebRTCImpl::setSpeakerVolume(float volume)
{
    mSignalingThread->PostTask(
        [this, volume]()
        {
            auto receivers = mPeerConnection->GetReceivers();

            RTC_LOG(LS_INFO) << __FUNCTION__ << "Set volume" << receivers.size();
            for (auto &receiver : receivers)
            {
                webrtc::MediaStreamTrackInterface *track = receiver->track().get();
                if (track->kind() == webrtc::MediaStreamTrackInterface::kAudioKind)
                {
                    webrtc::AudioTrackInterface* audio_track = static_cast<webrtc::AudioTrackInterface*>(track);
                    webrtc::AudioSourceInterface* source = audio_track->GetSource();
                    source->SetVolume(10.0 * volume);

                }
            }
        });
}

//
// PeerConnectionObserver implementation.
//

void LLWebRTCImpl::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface>                     receiver,
                              const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &streams)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << receiver->id();
}

void LLWebRTCImpl::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << receiver->id();
}

void LLWebRTCImpl::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state)
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
void LLWebRTCImpl::OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState new_state)
{
    RTC_LOG(LS_ERROR) << __FUNCTION__ << " Peer Connection State Change " << new_state;

    switch (new_state)
    {
        case webrtc::PeerConnectionInterface::PeerConnectionState::kConnected:
        {
            if (new_state == webrtc::PeerConnectionInterface::PeerConnectionState::kConnected)
            {
                for (auto &observer : mSignalingObserverList)
                {
                    observer->OnAudioEstablished(this);
                }
            }
            break;
        }
        case webrtc::PeerConnectionInterface::PeerConnectionState::kFailed:
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

void LLWebRTCImpl::OnIceCandidate(const webrtc::IceCandidateInterface *candidate)
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
            ice_candidate.candidate   = candidate->candidate().ToString();
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
void LLWebRTCImpl::OnSuccess(webrtc::SessionDescriptionInterface *desc)
{
    std::string sdp;
    desc->ToString(&sdp);
    RTC_LOG(LS_INFO) << sdp;

    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << mPeerConnection->signaling_state();
    for (auto &observer : mSignalingObserverList)
    {
        observer->OnOfferAvailable(sdp);
    }
}

void LLWebRTCImpl::OnFailure(webrtc::RTCError error) { RTC_LOG(LS_ERROR) << ToString(error.type()) << ": " << error.message(); }

//
// SetRemoteDescriptionObserverInterface implementation.
//
void LLWebRTCImpl::OnSetRemoteDescriptionComplete(webrtc::RTCError error)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << mPeerConnection->signaling_state();
    if (!error.ok())
    {
        RTC_LOG(LS_ERROR) << ToString(error.type()) << ": " << error.message();
        return;
    }
}

//
// SetLocalDescriptionObserverInterface implementation.
//
void LLWebRTCImpl::OnSetLocalDescriptionComplete(webrtc::RTCError error)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << mPeerConnection->signaling_state();
    if (!error.ok())
    {
        RTC_LOG(LS_ERROR) << ToString(error.type()) << ": " << error.message();
        return;
    }
    auto        desc = mPeerConnection->pending_local_description();
    std::string sdp;
    desc->ToString(&sdp);
    RTC_LOG(LS_INFO) << __FUNCTION__ << " Local SDP: " << sdp;
    for (auto &observer : mSignalingObserverList)
    {
        observer->OnOfferAvailable(sdp);
    }
}

rtc::RefCountedObject<LLWebRTCImpl> *gWebRTCImpl = nullptr;
LLWebRTCDeviceInterface             *getDeviceInterface() { return gWebRTCImpl; }
LLWebRTCSignalInterface             *getSignalingInterface() { return gWebRTCImpl; }

void init()
{
    gWebRTCImpl = new rtc::RefCountedObject<LLWebRTCImpl>();
    gWebRTCImpl->AddRef();
    gWebRTCImpl->init();
}
}  // namespace llwebrtc
