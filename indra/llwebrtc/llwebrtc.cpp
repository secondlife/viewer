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
#include <string.h>

#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/audio/builtin_audio_processing_builder.h"
#include "api/media_stream_interface.h"
#include "api/media_stream_track.h"
#include "modules/audio_processing/audio_buffer.h"
#include "modules/audio_mixer/audio_mixer_impl.h"
#include "api/environment/environment_factory.h"

namespace llwebrtc
{
#if WEBRTC_WIN
static int16_t PLAYOUT_DEVICE_DEFAULT = webrtc::AudioDeviceModule::kDefaultDevice;
static int16_t RECORD_DEVICE_DEFAULT  = webrtc::AudioDeviceModule::kDefaultDevice;
#else
static int16_t PLAYOUT_DEVICE_DEFAULT = 0;
static int16_t RECORD_DEVICE_DEFAULT  = 0;
#endif


//
// LLWebRTCAudioTransport implementation
//

LLWebRTCAudioTransport::LLWebRTCAudioTransport() : mMicrophoneEnergy(0.0)
{
    memset(mSumVector, 0, sizeof(mSumVector));
}

void LLWebRTCAudioTransport::SetEngineTransport(webrtc::AudioTransport* t)
{
    engine_.store(t, std::memory_order_release);
}

int32_t LLWebRTCAudioTransport::RecordedDataIsAvailable(const void* audio_data,
                                                        size_t      number_of_samples,
                                                        size_t      bytes_per_sample,
                                                        size_t      number_of_channels,
                                                        uint32_t    samples_per_sec,
                                                        uint32_t    total_delay_ms,
                                                        int32_t     clock_drift,
                                                        uint32_t    current_mic_level,
                                                        bool        key_pressed,
                                                        uint32_t&   new_mic_level)
{
    auto* engine = engine_.load(std::memory_order_acquire);

    // 1) Deliver to engine (authoritative).
    int32_t ret = 0;
    if (engine)
    {
        ret = engine->RecordedDataIsAvailable(audio_data,
                                              number_of_samples,
                                              bytes_per_sample,
                                              number_of_channels,
                                              samples_per_sec,
                                              total_delay_ms,
                                              clock_drift,
                                              current_mic_level,
                                              key_pressed,
                                              new_mic_level);
    }

    // 2) Calculate energy for microphone level monitoring
    // calculate the energy
    float        energy  = 0;
    const short *samples = (const short *) audio_data;
    for (size_t index = 0; index < number_of_samples * number_of_channels; index++)
    {
        float sample = (static_cast<float>(samples[index]) / (float) 32767);
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
    mMicrophoneEnergy = std::sqrt(totalSum / (number_of_samples * buffer_size));

    return ret;
}

int32_t LLWebRTCAudioTransport::NeedMorePlayData(size_t   number_of_samples,
                                                 size_t   bytes_per_sample,
                                                 size_t   number_of_channels,
                                                 uint32_t samples_per_sec,
                                                 void*    audio_data,
                                                 size_t&  number_of_samples_out,
                                                 int64_t* elapsed_time_ms,
                                                 int64_t* ntp_time_ms)
{
    auto* engine = engine_.load(std::memory_order_acquire);
    if (!engine)
    {
        // No engine sink; output silence to be safe.
        const size_t bytes = number_of_samples * bytes_per_sample * number_of_channels;
        memset(audio_data, 0, bytes);
        number_of_samples_out = number_of_samples;
        return 0;
    }

    // Only the engine should fill the buffer.
    return engine->NeedMorePlayData(number_of_samples,
                                    bytes_per_sample,
                                    number_of_channels,
                                    samples_per_sec,
                                    audio_data,
                                    number_of_samples_out,
                                    elapsed_time_ms,
                                    ntp_time_ms);
}

void LLWebRTCAudioTransport::PullRenderData(int      bits_per_sample,
                                            int      sample_rate,
                                            size_t   number_of_channels,
                                            size_t   number_of_frames,
                                            void*    audio_data,
                                            int64_t* elapsed_time_ms,
                                            int64_t* ntp_time_ms)
{
    auto* engine = engine_.load(std::memory_order_acquire);

    if (engine)
    {
        engine
            ->PullRenderData(bits_per_sample, sample_rate, number_of_channels, number_of_frames, audio_data, elapsed_time_ms, ntp_time_ms);
    }
}

LLCustomProcessor::LLCustomProcessor(LLCustomProcessorStatePtr state) : mSampleRateHz(0), mNumChannels(0), mState(state)
{
    memset(mSumVector, 0, sizeof(mSumVector));
}

void LLCustomProcessor::Initialize(int sample_rate_hz, int num_channels)
{
    mSampleRateHz = sample_rate_hz;
    mNumChannels  = num_channels;
    memset(mSumVector, 0, sizeof(mSumVector));
}

void LLCustomProcessor::Process(webrtc::AudioBuffer *audio_in)
{
    webrtc::StreamConfig stream_config;
    stream_config.set_sample_rate_hz(mSampleRateHz);
    stream_config.set_num_channels(mNumChannels);
    std::vector<float *> frame;
    std::vector<float>   frame_samples;

    if (audio_in->num_channels() < 1 || audio_in->num_frames() < 480)
    {
        return;
    }

    // grab the input audio
    frame_samples.resize(stream_config.num_samples());
    frame.resize(stream_config.num_channels());
    for (size_t ch = 0; ch < stream_config.num_channels(); ++ch)
    {
        frame[ch] = &(frame_samples)[ch * stream_config.num_frames()];
    }

    audio_in->CopyTo(stream_config, &frame[0]);

    // calculate the energy
    float energy = 0;
    float gain   = mState->getGain();
    for (size_t index = 0; index < stream_config.num_samples(); index++)
    {
        float sample = frame_samples[index];
        sample       = sample * gain; // apply gain
        frame_samples[index] = sample; // write processed sample back to buffer.
        energy += sample * sample;
    }

    audio_in->CopyFrom(&frame[0], stream_config);

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
    mState->setMicrophoneEnergy(std::sqrt(totalSum / (stream_config.num_samples() * buffer_size)));
}

//
// LLWebRTCImpl implementation
//

LLWebRTCImpl::LLWebRTCImpl(LLWebRTCLogCallback* logCallback) :
    mLogSink(new LLWebRTCLogSink(logCallback)),
    mPeerCustomProcessor(nullptr),
    mMute(true),
    mTuningMode(false),
    mDevicesDeploying(false),
    mPlayoutDevice(PLAYOUT_DEVICE_DEFAULT),
    mRecordingDevice(RECORD_DEVICE_DEFAULT)
{
}

void LLWebRTCImpl::init()
{
    webrtc::InitializeSSL();

    // Normal logging is rather spammy, so turn it off.
    webrtc::LogMessage::LogToDebug(webrtc::LS_NONE);
    webrtc::LogMessage::SetLogToStderr(true);
    webrtc::LogMessage::AddLogToStream(mLogSink, webrtc::LS_VERBOSE);

    mTaskQueueFactory = webrtc::CreateDefaultTaskQueueFactory();

    // Create the native threads.
    mNetworkThread = webrtc::Thread::CreateWithSocketServer();
    mNetworkThread->SetName("WebRTCNetworkThread", nullptr);
    mNetworkThread->Start();
    mWorkerThread = webrtc::Thread::Create();
    mWorkerThread->SetName("WebRTCWorkerThread", nullptr);
    mWorkerThread->Start();
    mSignalingThread = webrtc::Thread::Create();
    mSignalingThread->SetName("WebRTCSignalingThread", nullptr);
    mSignalingThread->Start();

    mWorkerThread->BlockingCall(
        [this]()
        {
            webrtc::scoped_refptr<webrtc::AudioDeviceModule> realADM =
                webrtc::AudioDeviceModule::Create(webrtc::AudioDeviceModule::AudioLayer::kPlatformDefaultAudio, mTaskQueueFactory.get());
            mDeviceModule = webrtc::make_ref_counted<LLWebRTCAudioDeviceModule>(realADM);
            mDeviceModule->SetObserver(this);
        });

    // The custom processor allows us to retrieve audio data (and levels)
    // from after other audio processing such as AEC, AGC, etc.
    mPeerCustomProcessor = std::make_shared<LLCustomProcessorState>();
    webrtc::BuiltinAudioProcessingBuilder apb;
    apb.SetCapturePostProcessing(std::make_unique<LLCustomProcessor>(mPeerCustomProcessor));
    mAudioProcessingModule = apb.Build(webrtc::CreateEnvironment());

    webrtc::AudioProcessing::Config apm_config;
    apm_config.echo_canceller.enabled         = false;
    apm_config.echo_canceller.mobile_mode     = false;
    apm_config.gain_controller1.enabled       = false;
    apm_config.gain_controller1.mode          = webrtc::AudioProcessing::Config::GainController1::kAdaptiveAnalog;
    apm_config.gain_controller2.enabled       = false;
    apm_config.high_pass_filter.enabled       = true;
    apm_config.noise_suppression.enabled      = true;
    apm_config.noise_suppression.level        = webrtc::AudioProcessing::Config::NoiseSuppression::kVeryHigh;
    apm_config.transient_suppression.enabled  = true;
    apm_config.pipeline.multi_channel_render  = true;
    apm_config.pipeline.multi_channel_capture = false;

    mAudioProcessingModule->ApplyConfig(apm_config);

    webrtc::ProcessingConfig processing_config;
    processing_config.input_stream().set_num_channels(2);
    processing_config.input_stream().set_sample_rate_hz(48000);
    processing_config.output_stream().set_num_channels(2);
    processing_config.output_stream().set_sample_rate_hz(48000);
    processing_config.reverse_input_stream().set_num_channels(2);
    processing_config.reverse_input_stream().set_sample_rate_hz(48000);
    processing_config.reverse_output_stream().set_num_channels(2);
    processing_config.reverse_output_stream().set_sample_rate_hz(48000);

    mAudioProcessingModule->Initialize(processing_config);

    mPeerConnectionFactory = webrtc::CreatePeerConnectionFactory(mNetworkThread.get(),
                                                                 mWorkerThread.get(),
                                                                 mSignalingThread.get(),
                                                                 mDeviceModule,
                                                                 webrtc::CreateBuiltinAudioEncoderFactory(),
                                                                 webrtc::CreateBuiltinAudioDecoderFactory(),
                                                                 nullptr /* video_encoder_factory */,
                                                                 nullptr /* video_decoder_factory */,
                                                                 nullptr /* audio_mixer */,
                                                                 mAudioProcessingModule);
    mWorkerThread->PostTask(
        [this]()
        {
            mDeviceModule->EnableBuiltInAEC(false);
            workerDeployDevices();
            updateDevices();
        });

}

void LLWebRTCImpl::terminate()
{
    for (auto &connection : mPeerConnections)
    {
        connection->terminate();
    }

    // connection->terminate() above spawns a number of Signaling thread calls to
    // shut down the connection.  The following Blocking Call will wait
    // until they're done before it's executed, allowing time to clean up.

    mSignalingThread->BlockingCall([this]() { mPeerConnectionFactory = nullptr; });

    mPeerConnections.clear();

    mWorkerThread->BlockingCall(
        [this]()
        {
            if (mDeviceModule)
            {
                mDeviceModule->StopRecording();
                mDeviceModule->StopPlayout();
                mDeviceModule->Terminate();
            }
            mDeviceModule     = nullptr;
            mTaskQueueFactory = nullptr;
        });
    webrtc::LogMessage::RemoveLogToStream(mLogSink);
}

void LLWebRTCImpl::setAudioConfig(LLWebRTCDeviceInterface::AudioConfig config)
{
    webrtc::AudioProcessing::Config apm_config;
    apm_config.echo_canceller.enabled         = config.mEchoCancellation;
    apm_config.echo_canceller.mobile_mode     = false;
    apm_config.gain_controller1.enabled       = config.mAGC;
    apm_config.gain_controller1.mode          = webrtc::AudioProcessing::Config::GainController1::kAdaptiveAnalog;
    apm_config.gain_controller2.enabled       = false;
    apm_config.high_pass_filter.enabled       = true;
    apm_config.transient_suppression.enabled  = true;
    apm_config.pipeline.multi_channel_render  = true;
    apm_config.pipeline.multi_channel_capture = true;
    apm_config.pipeline.multi_channel_capture = true;

    switch (config.mNoiseSuppressionLevel)
    {
        case LLWebRTCDeviceInterface::AudioConfig::NOISE_SUPPRESSION_LEVEL_NONE:
            apm_config.noise_suppression.enabled = false;
            apm_config.noise_suppression.level = webrtc::AudioProcessing::Config::NoiseSuppression::kLow;
            break;
        case LLWebRTCDeviceInterface::AudioConfig::NOISE_SUPPRESSION_LEVEL_LOW:
            apm_config.noise_suppression.enabled = true;
            apm_config.noise_suppression.level = webrtc::AudioProcessing::Config::NoiseSuppression::kLow;
            break;
        case LLWebRTCDeviceInterface::AudioConfig::NOISE_SUPPRESSION_LEVEL_MODERATE:
            apm_config.noise_suppression.enabled = true;
            apm_config.noise_suppression.level = webrtc::AudioProcessing::Config::NoiseSuppression::kModerate;
            break;
        case LLWebRTCDeviceInterface::AudioConfig::NOISE_SUPPRESSION_LEVEL_HIGH:
            apm_config.noise_suppression.enabled = true;
            apm_config.noise_suppression.level = webrtc::AudioProcessing::Config::NoiseSuppression::kHigh;
            break;
        case LLWebRTCDeviceInterface::AudioConfig::NOISE_SUPPRESSION_LEVEL_VERY_HIGH:
            apm_config.noise_suppression.enabled = true;
            apm_config.noise_suppression.level = webrtc::AudioProcessing::Config::NoiseSuppression::kVeryHigh;
            break;
        default:
            apm_config.noise_suppression.enabled = false;
            apm_config.noise_suppression.level = webrtc::AudioProcessing::Config::NoiseSuppression::kLow;
    }
    mAudioProcessingModule->ApplyConfig(apm_config);
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

// must be run in the worker thread.
void LLWebRTCImpl::workerDeployDevices()
{
    mDeviceModule->StopPlayout();
    mDeviceModule->ForceStopRecording();
#if WEBRTC_WIN
    if (mRecordingDevice < 0) {
        mDeviceModule->SetRecordingDevice((webrtc::AudioDeviceModule::WindowsDeviceType)mRecordingDevice);
    }
    else
    {
        mDeviceModule->SetRecordingDevice(mRecordingDevice);
    }
#else
    mDeviceModule->SetRecordingDevice(mRecordingDevice);
#endif
    mDeviceModule->InitMicrophone();
    mDeviceModule->SetStereoRecording(false);
    mDeviceModule->InitRecording();

#if WEBRTC_WIN
    if (mPlayoutDevice < 0)
    {
        mDeviceModule->SetPlayoutDevice((webrtc::AudioDeviceModule::WindowsDeviceType)mPlayoutDevice);
    }
    else
    {
        mDeviceModule->SetPlayoutDevice(mPlayoutDevice);
    }
#else
    mDeviceModule->SetPlayoutDevice(mPlayoutDevice);
#endif
    mDeviceModule->InitSpeaker();
    mDeviceModule->SetStereoPlayout(true);
    mDeviceModule->InitPlayout();
    mDeviceModule->ForceStartRecording();
    uint32_t min_v = 0, max_v = 0, cur_v = 0;
    bool     have_hw = false;

    mDeviceModule->MicrophoneVolumeIsAvailable(&have_hw);
    if (have_hw)
    {
        mDeviceModule->MinMicrophoneVolume(&min_v);
        mDeviceModule->MaxMicrophoneVolume(&max_v);
        uint32_t target = min_v + (max_v - min_v) * 8 / 10; // ~80%
        mDeviceModule->SetMicrophoneVolume(target);
        mDeviceModule->MicrophoneVolume(&cur_v);
    }
    mDeviceModule->StartPlayout();
}

void LLWebRTCImpl::setCaptureDevice(const std::string &id)
{
    int16_t recordingDevice = RECORD_DEVICE_DEFAULT;
#if WEBRTC_WIN
    int16_t device_start = 0;
#else
    if (mRecordingDeviceList.size())
    {
        // no recording devices
        return;
    }
    int16_t device_start = 1;
#endif

    if (id != "Default")
    {
        for (int16_t i = device_start; i < mRecordingDeviceList.size(); i++)
        {
            if (mRecordingDeviceList[i].mID == id)
            {
                recordingDevice = i;
                break;
            }
        }
    }

    // Always deploy devices, as we may have received a device update
    // for the default device, which may be the same as mRecordingDevice
    // but still needs to be refreshed.
    mRecordingDevice = recordingDevice;
    deployDevices();
}

void LLWebRTCImpl::setRenderDevice(const std::string &id)
{
    int16_t playoutDevice = PLAYOUT_DEVICE_DEFAULT;
#if WEBRTC_WIN
    int16_t device_start = 0;
#else
    if (mPlayoutDeviceList.size())
    {
        // no playout devices
        return;
    }
    int16_t device_start = 1;
#endif
    if (id != "Default")
    {
        for (int16_t i = device_start; i < mPlayoutDeviceList.size(); i++)
        {
            if (mPlayoutDeviceList[i].mID == id)
            {
                playoutDevice = i;
                break;
            }
        }
    }

    // Always deploy devices, as we may have received a device update
    // for the default device, which may be the same as mPlayoutDevice
    // but still needs to be refreshed.
    mPlayoutDevice = playoutDevice;
    deployDevices();
}

// updateDevices needs to happen on the worker thread.
void LLWebRTCImpl::updateDevices()
{
    int16_t renderDeviceCount  = mDeviceModule->PlayoutDevices();

    mPlayoutDeviceList.clear();
#if WEBRTC_WIN
    int16_t index = 0;
#else
    // index zero is always "Default" for darwin/linux,
    // which is a special case, so skip it.
    int16_t index = 1;
#endif
    for (; index < renderDeviceCount; index++)
    {
        char name[webrtc::kAdmMaxDeviceNameSize];
        char guid[webrtc::kAdmMaxGuidSize];
        mDeviceModule->PlayoutDeviceName(index, name, guid);
        mPlayoutDeviceList.emplace_back(name, guid);
    }

    int16_t captureDeviceCount        = mDeviceModule->RecordingDevices();

    mRecordingDeviceList.clear();
#if WEBRTC_WIN
    index = 0;
#else
    // index zero is always "Default" for darwin/linux,
    // which is a special case, so skip it.
    index = 1;
#endif
    for (; index < captureDeviceCount; index++)
    {
        char name[webrtc::kAdmMaxDeviceNameSize];
        char guid[webrtc::kAdmMaxGuidSize];
        mDeviceModule->RecordingDeviceName(index, name, guid);
        mRecordingDeviceList.emplace_back(name, guid);
    }

    for (auto &observer : mVoiceDevicesObserverList)
    {
        observer->OnDevicesChanged(mPlayoutDeviceList, mRecordingDeviceList);
    }
}

void LLWebRTCImpl::OnDevicesUpdated()
{
    mRecordingDevice = RECORD_DEVICE_DEFAULT;
    mPlayoutDevice   = PLAYOUT_DEVICE_DEFAULT;

    updateDevices();
}


void LLWebRTCImpl::setTuningMode(bool enable)
{
    mTuningMode = enable;
    mDeviceModule->SetTuning(mTuningMode);
    mWorkerThread->PostTask(
        [this]
        {
            mDeviceModule->SetTuning(mTuningMode);
            mSignalingThread->PostTask(
                [this]
                {
                    for (auto& connection : mPeerConnections)
                    {
                        if (mTuningMode)
                        {
                            connection->enableSenderTracks(false);
                        }
                        else
                        {
                            connection->resetMute();
                        }
                        connection->enableReceiverTracks(!mTuningMode);
                    }
                });
        });
}

void LLWebRTCImpl::deployDevices()
{
    if (mDevicesDeploying)
    {
        return;
    }
    mDevicesDeploying = true;
    mWorkerThread->PostTask(
        [this] {
            workerDeployDevices();
            mSignalingThread->PostTask(
                [this]
                {
                    for (auto &connection : mPeerConnections)
                    {
                        if (mTuningMode)
                        {
                            connection->enableSenderTracks(false);
                        }
                        else
                        {
                            connection->resetMute();
                        }
                        connection->enableReceiverTracks(!mTuningMode);
                    }
                    mDevicesDeploying = false;
                });
        });
}

float LLWebRTCImpl::getTuningAudioLevel() { return mDeviceModule ? -20 * log10f(mDeviceModule->GetMicrophoneEnergy()) : 0.0f; }

float LLWebRTCImpl::getPeerConnectionAudioLevel() { return mPeerCustomProcessor ? -20 * log10f(mPeerCustomProcessor->getMicrophoneEnergy()) : 0.0f; }

void LLWebRTCImpl::setPeerConnectionGain(float gain) { if (mPeerCustomProcessor) mPeerCustomProcessor->setGain(gain); }


//
// Peer Connection Helpers
//

LLWebRTCPeerConnectionInterface *LLWebRTCImpl::newPeerConnection()
{
    bool empty = mPeerConnections.empty();
    webrtc::scoped_refptr<LLWebRTCPeerConnectionImpl> peerConnection = webrtc::scoped_refptr<LLWebRTCPeerConnectionImpl>(new webrtc::RefCountedObject<LLWebRTCPeerConnectionImpl>());
    peerConnection->init(this);

    mPeerConnections.emplace_back(peerConnection);
    peerConnection->enableSenderTracks(false);
    return peerConnection.get();
}

void LLWebRTCImpl::freePeerConnection(LLWebRTCPeerConnectionInterface* peer_connection)
{
    std::vector<webrtc::scoped_refptr<LLWebRTCPeerConnectionImpl>>::iterator it =
    std::find(mPeerConnections.begin(), mPeerConnections.end(), peer_connection);
    if (it != mPeerConnections.end())
    {
        mPeerConnections.erase(it);
    }
}


//
// LLWebRTCPeerConnectionImpl implementation.
//
// Most peer connection (signaling) happens on
// the signaling thread.

LLWebRTCPeerConnectionImpl::LLWebRTCPeerConnectionImpl() :
    mWebRTCImpl(nullptr),
    mPeerConnection(nullptr),
    mMute(MUTE_INITIAL),
    mAnswerReceived(false)
{
}

LLWebRTCPeerConnectionImpl::~LLWebRTCPeerConnectionImpl()
{
    mSignalingObserverList.clear();
    mDataObserverList.clear();
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
    mWebRTCImpl->SignalingBlockingCall(
        [this]()
        {
            if (mPeerConnection)
            {
                if (mDataChannel)
                {
                    {
                        mDataChannel->Close();
                        mDataChannel = nullptr;
                    }
                }

                // to remove 'Secondlife is recording' icon from taskbar
                // if user was speaking
                auto senders = mPeerConnection->GetSenders();
                for (auto& sender : senders)
                {
                    auto track = sender->track();
                    if (track)
                    {
                        track->set_enabled(false);
                    }
                }

                mPeerConnection->Close();
                if (mLocalStream)
                {
                    auto tracks = mLocalStream->GetAudioTracks();
                    for (auto& track : tracks)
                    {
                        mLocalStream->RemoveTrack(track);
                    }
                    mLocalStream = nullptr;
                }
                mPeerConnection = nullptr;

                for (auto &observer : mSignalingObserverList)
                {
                    observer->OnPeerConnectionClosed();
                }
            }
        });
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


bool LLWebRTCPeerConnectionImpl::initializeConnection(const LLWebRTCPeerConnectionInterface::InitOptions& options)
{
    RTC_DCHECK(!mPeerConnection);
    mAnswerReceived = false;

    mWebRTCImpl->PostSignalingTask(
        [this,options]()
        {
            webrtc::PeerConnectionInterface::RTCConfiguration config;
            for (auto server : options.mServers)
            {
                webrtc::PeerConnectionInterface::IceServer ice_server;
                for (auto url : server.mUrls)
                {
                    ice_server.urls.push_back(url);
                }
                ice_server.username = server.mUserName;
                ice_server.password = server.mPassword;
                config.servers.push_back(ice_server);
            }
            config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;

            config.set_min_port(60000);
            config.set_max_port(60100);

            webrtc::PeerConnectionDependencies pc_dependencies(this);
            auto error_or_peer_connection = mPeerConnectionFactory->CreatePeerConnectionOrError(config, std::move(pc_dependencies));
            if (error_or_peer_connection.ok())
            {
                mPeerConnection = std::move(error_or_peer_connection.value());
            }
            else
            {
                RTC_LOG(LS_ERROR) << __FUNCTION__ << "Error creating peer connection: " << error_or_peer_connection.error().message();
                for (auto &observer : mSignalingObserverList)
                {
                    observer->OnRenegotiationNeeded();
                }
                return;
            }

            webrtc::DataChannelInit init;
            init.ordered = true;

            auto data_channel_or_error = mPeerConnection->CreateDataChannelOrError("SLData", &init);
            if (data_channel_or_error.ok())
            {
                mDataChannel = std::move(data_channel_or_error.value());

                mDataChannel->RegisterObserver(this);
            }

            webrtc::AudioOptions audioOptions;
            audioOptions.auto_gain_control = true;
            audioOptions.echo_cancellation = true;
            audioOptions.noise_suppression = true;
            audioOptions.init_recording_on_send = false;

            mLocalStream = mPeerConnectionFactory->CreateLocalMediaStream("SLStream");

            webrtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
                mPeerConnectionFactory->CreateAudioTrack("SLAudio", mPeerConnectionFactory->CreateAudioSource(audioOptions).get()));
            audio_track->set_enabled(false);
            mLocalStream->AddTrack(audio_track);

            mPeerConnection->AddTrack(audio_track, {"SLStream"});

            auto senders = mPeerConnection->GetSenders();

            for (auto &sender : senders)
            {
                webrtc::RtpParameters      params;
                webrtc::RtpCodecParameters codecparam;
                codecparam.name                       = "opus";
                codecparam.kind                       = webrtc::MediaType::AUDIO;
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
                codecparam.kind                       = webrtc::MediaType::AUDIO;
                codecparam.clock_rate                 = 48000;
                codecparam.num_channels               = 2;
                codecparam.parameters["stereo"]       = "1";
                codecparam.parameters["sprop-stereo"] = "1";
                params.codecs.push_back(codecparam);
                receiver->SetParameters(params);
            }

            webrtc::PeerConnectionInterface::RTCOfferAnswerOptions offerOptions;
            mPeerConnection->CreateOffer(this, offerOptions);
        });

    return true;
}

bool LLWebRTCPeerConnectionImpl::shutdownConnection()
{
    terminate();
    return true;
}

void LLWebRTCPeerConnectionImpl::enableSenderTracks(bool enable)
{
    // set_enabled shouldn't be done on the worker thread.
    if (mPeerConnection)
    {
        auto senders = mPeerConnection->GetSenders();
        for (auto &sender : senders)
        {
            sender->track()->set_enabled(enable);
        }
    }
}

void LLWebRTCPeerConnectionImpl::enableReceiverTracks(bool enable)
{
    // set_enabled shouldn't be done on the worker thread
    if (mPeerConnection)
    {
        auto receivers = mPeerConnection->GetReceivers();
        for (auto &receiver : receivers)
        {
            receiver->track()->set_enabled(enable);
        }
    }
}

// Tell the peer connection that we've received a SDP answer from the sim.
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
                                                                             webrtc::scoped_refptr<webrtc::SetRemoteDescriptionObserverInterface>(this));
                                   }
                               });
}


//
// LLWebRTCAudioInterface implementation
//

void LLWebRTCPeerConnectionImpl::setMute(bool mute)
{
    EMicMuteState new_state = mute ? MUTE_MUTED : MUTE_UNMUTED;

    // even if mute hasn't changed, we still need to update the mute
    // state on the connections to handle cases where the 'Default' device
    // has changed in the OS (unplugged headset, etc.) which messes
    // with the mute state.

    bool force_reset = mMute == MUTE_INITIAL && mute;
    bool enable = !mute;
    mMute = new_state;

    mWebRTCImpl->PostSignalingTask(
        [this, force_reset, enable]()
        {
        if (mPeerConnection)
        {
            auto senders = mPeerConnection->GetSenders();

            RTC_LOG(LS_INFO) << __FUNCTION__ << (mMute ? "disabling" : "enabling") << " streams count " << senders.size();
            for (auto &sender : senders)
            {
                auto track = sender->track();
                if (track)
                {
                    if (force_reset)
                    {
                        // Force notify observers
                        // Was it disabled too early?
                        // Without this microphone icon in Win's taskbar will stay
                        track->set_enabled(true);
                    }
                    track->set_enabled(enable);
                }
            }
        }
    });
}

void LLWebRTCPeerConnectionImpl::resetMute()
{
    switch(mMute)
    {
    case MUTE_MUTED:
         setMute(true);
         break;
    case MUTE_UNMUTED:
         setMute(false);
         break;
    default:
        break;
    }
}

void LLWebRTCPeerConnectionImpl::setReceiveVolume(float volume)
{
    mWebRTCImpl->PostSignalingTask(
        [this, volume]()
        {
            if (mPeerConnection)
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
            }
        });
}

void LLWebRTCPeerConnectionImpl::setSendVolume(float volume)
{
    mWebRTCImpl->PostSignalingTask(
        [this, volume]()
        {
            if (mLocalStream)
            {
                for (auto &track : mLocalStream->GetAudioTracks())
                {
                    track->GetSource()->SetVolume(volume*5.0);
                }
            }
        });
}

//
// PeerConnectionObserver implementation.
//

void LLWebRTCPeerConnectionImpl::OnAddTrack(webrtc::scoped_refptr<webrtc::RtpReceiverInterface>                     receiver,
                                            const std::vector<webrtc::scoped_refptr<webrtc::MediaStreamInterface>> &streams)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << receiver->id();
    webrtc::RtpParameters      params;
    webrtc::RtpCodecParameters codecparam;
    codecparam.name                       = "opus";
    codecparam.kind                       = webrtc::MediaType::AUDIO;
    codecparam.clock_rate                 = 48000;
    codecparam.num_channels               = 2;
    codecparam.parameters["stereo"]       = "1";
    codecparam.parameters["sprop-stereo"] = "1";
    params.codecs.push_back(codecparam);
    receiver->SetParameters(params);
}

void LLWebRTCPeerConnectionImpl::OnRemoveTrack(webrtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << receiver->id();
}

void LLWebRTCPeerConnectionImpl::OnDataChannel(webrtc::scoped_refptr<webrtc::DataChannelInterface> channel)
{
    if (mDataChannel)
    {
        mDataChannel->UnregisterObserver();
    }
    mDataChannel = channel;
    channel->RegisterObserver(this);
}

void LLWebRTCPeerConnectionImpl::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state)
{
    LLWebRTCSignalingObserver::EIceGatheringState webrtc_new_state = LLWebRTCSignalingObserver::EIceGatheringState::ICE_GATHERING_NEW;
    switch (new_state)
    {
        case webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringNew:
            webrtc_new_state = LLWebRTCSignalingObserver::EIceGatheringState::ICE_GATHERING_NEW;
            break;
        case webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringGathering:
            webrtc_new_state = LLWebRTCSignalingObserver::EIceGatheringState::ICE_GATHERING_GATHERING;
            break;
        case webrtc::PeerConnectionInterface::IceGatheringState::kIceGatheringComplete:
            webrtc_new_state = LLWebRTCSignalingObserver::EIceGatheringState::ICE_GATHERING_COMPLETE;
            break;
        default:
            RTC_LOG(LS_ERROR) << __FUNCTION__ << " Bad Ice Gathering State" << new_state;
            webrtc_new_state = LLWebRTCSignalingObserver::EIceGatheringState::ICE_GATHERING_NEW;
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
            mWebRTCImpl->PostWorkerTask([this]() {
                for (auto &observer : mSignalingObserverList)
                {
                    observer->OnAudioEstablished(this);
                }
            });
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

// Convert an ICE candidate into a string appropriate for trickling
// to the Secondlife WebRTC server via the sim.
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

    if (candidate->candidate().type() == webrtc::IceCandidateType::kHost)
    {
        candidate_stream << "host";
    }
    else if (candidate->candidate().type() == webrtc::IceCandidateType::kSrflx)
    {
        candidate_stream << "srflx " <<
        "raddr " << candidate->candidate().related_address().ipaddr().ToString() << " " <<
        "rport " << candidate->candidate().related_address().PortAsString();
    }
    else if (candidate->candidate().type() == webrtc::IceCandidateType::kRelay)
    {
        candidate_stream << "relay " <<
        "raddr " << candidate->candidate().related_address().ipaddr().ToString() << " " <<
        "rport " << candidate->candidate().related_address().PortAsString();
    }
    else if (candidate->candidate().type() == webrtc::IceCandidateType::kPrflx)
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

// The webrtc library has a new ice candidate.
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
        // We've already received an answer SDP from the Secondlife WebRTC server
        // so simply tell observers about our new ice candidate.
        for (auto &observer : mSignalingObserverList)
        {
            LLWebRTCIceCandidate ice_candidate;
            ice_candidate.mCandidate  = iceCandidateToTrickleString(candidate);
            ice_candidate.mMLineIndex = candidate->sdp_mline_index();
            ice_candidate.mSdpMid     = candidate->sdp_mid();
            observer->OnIceCandidate(ice_candidate);
        }
    }
    else
    {
        // As we've not yet received our answer, cache the candidate.
        mCachedIceCandidates.push_back(
            webrtc::CreateIceCandidate(candidate->sdp_mid(),
                                       candidate->sdp_mline_index(),
                                       candidate->candidate()));
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
            opus_payload = std::to_string(payload_id);
            sdp_mangled_stream << "a=rtpmap:" << opus_payload << " opus/48000/2" << "\n";
        }
        else if (sdp_line.find("a=fmtp:" + opus_payload) == 0)
        {
            sdp_mangled_stream << sdp_line << "a=fmtp:" << opus_payload
            << " minptime=10;useinbandfec=1;stereo=1;sprop-stereo=1;maxplaybackrate=48000;sprop-maxplaybackrate=48000;sprop-maxcapturerate=48000\n";
        }
        else
        {
            sdp_mangled_stream << sdp_line << "\n";
        }
    }

    RTC_LOG(LS_INFO) << __FUNCTION__ << " Local SDP: " << sdp_mangled_stream.str();
    std::string mangled_sdp = sdp_mangled_stream.str();
    for (auto &observer : mSignalingObserverList)
    {
        observer->OnOfferAvailable(mangled_sdp);
    }

   mPeerConnection->SetLocalDescription(std::unique_ptr<webrtc::SessionDescriptionInterface>(
                                                     webrtc::CreateSessionDescription(webrtc::SdpType::kOffer, mangled_sdp)),
                                                 webrtc::scoped_refptr<webrtc::SetLocalDescriptionObserverInterface>(this));

}

void LLWebRTCPeerConnectionImpl::OnFailure(webrtc::RTCError error)
{
    RTC_LOG(LS_ERROR) << ToString(error.type()) << ": " << error.message();
    for (auto &observer : mSignalingObserverList)
    {
        observer->OnRenegotiationNeeded();
    }
}

//
// SetRemoteDescriptionObserverInterface implementation.
//
void LLWebRTCPeerConnectionImpl::OnSetRemoteDescriptionComplete(webrtc::RTCError error)
{
    // we've received an answer SDP from the sim.

    RTC_LOG(LS_INFO) << __FUNCTION__ << " " << mPeerConnection->signaling_state();
    if (!error.ok())
    {
        RTC_LOG(LS_ERROR) << ToString(error.type()) << ": " << error.message();
        for (auto &observer : mSignalingObserverList)
        {
            observer->OnRenegotiationNeeded();
        }
        return;
    }
    mAnswerReceived = true;

    // tell the observers about any cached ICE candidates.
    for (auto &observer : mSignalingObserverList)
    {
        for (auto &candidate : mCachedIceCandidates)
        {
            LLWebRTCIceCandidate ice_candidate;
            ice_candidate.mCandidate  = iceCandidateToTrickleString(candidate.get());
            ice_candidate.mMLineIndex = candidate->sdp_mline_index();
            ice_candidate.mSdpMid     = candidate->sdp_mid();
            observer->OnIceCandidate(ice_candidate);
        }
    }
    mCachedIceCandidates.clear();
    if (mPeerConnection)
    {
        OnIceGatheringChange(mPeerConnection->ice_gathering_state());
    }

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
    if (!mDataChannel)
    {
        return;
    }
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

//
// LLWebRTCDataInterface
//

void LLWebRTCPeerConnectionImpl::sendData(const std::string& data, bool binary)
{
    if (mDataChannel)
    {
        webrtc::CopyOnWriteBuffer cowBuffer(data.data(), data.length());
        webrtc::DataBuffer     buffer(cowBuffer, binary);
        mWebRTCImpl->PostNetworkTask([this, buffer]() {
                if (mDataChannel)
                {
                    mDataChannel->Send(buffer);
                }
            });
    }
}

void LLWebRTCPeerConnectionImpl::setDataObserver(LLWebRTCDataObserver* observer)
{
    mDataObserverList.emplace_back(observer);
}

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

LLWebRTCPeerConnectionInterface* newPeerConnection()
{
    return gWebRTCImpl->newPeerConnection();
}

void freePeerConnection(LLWebRTCPeerConnectionInterface* peer_connection)
{
    gWebRTCImpl->freePeerConnection(peer_connection);
}


void init(LLWebRTCLogCallback* logCallback)
{
    gWebRTCImpl = new LLWebRTCImpl(logCallback);
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
