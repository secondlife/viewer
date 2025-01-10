 /**
 * @file llvoicewebrtc.cpp
 * @brief Implementation of LLWebRTCVoiceClient class which is the interface to the voice client process.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
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
#include <algorithm>
#include "llvoicewebrtc.h"

#include "llsdutil.h"

// Linden library includes
#include "llavatarnamecache.h"
#include "llvoavatarself.h"
#include "llbufferstream.h"
#include "llfile.h"
#include "llmenugl.h"
#ifdef LL_USESYSTEMLIBS
# include "expat.h"
#else
# include "expat/expat.h"
#endif
#include "llcallbacklist.h"
#include "llviewernetwork.h"        // for gGridChoice
#include "llbase64.h"
#include "llviewercontrol.h"
#include "llappviewer.h"    // for gDisconnected, gDisableVoice
#include "llprocess.h"

// Viewer includes
#include "llmutelist.h"  // to check for muted avatars
#include "llagent.h"
#include "llcachename.h"
#include "llimview.h" // for LLIMMgr
#include "llworld.h"
#include "llparcel.h"
#include "llviewerparcelmgr.h"
#include "llfirstuse.h"
#include "llspeakers.h"
#include "lltrans.h"
#include "llrand.h"
#include "llviewerwindow.h"
#include "llviewercamera.h"
#include "llversioninfo.h"

#include "llviewernetwork.h"
#include "llnotificationsutil.h"

#include "llcorehttputil.h"
#include "lleventfilter.h"

#include "stringize.h"

#include "llwebrtc.h"

// for base64 decoding
#include "apr_base64.h"

#include "boost/json.hpp"

const std::string WEBRTC_VOICE_SERVER_TYPE = "webrtc";

namespace {

    const F32 MAX_AUDIO_DIST      = 50.0f;
    const F32 VOLUME_SCALE_WEBRTC = 0.01f;
    const F32 LEVEL_SCALE_WEBRTC  = 0.008f;

    const F32 SPEAKING_AUDIO_LEVEL = 0.30;

    const uint32_t PEER_GAIN_CONVERSION_FACTOR = 220;

    static const std::string REPORTED_VOICE_SERVER_TYPE = "Secondlife WebRTC Gateway";

    // Don't send positional updates more frequently than this:
    const F32 UPDATE_THROTTLE_SECONDS = 0.1f;
    const F32 MAX_RETRY_WAIT_SECONDS  = 10.0f;

    // Cosine of a "trivially" small angle
    const F32 FOUR_DEGREES = 4.0f * (F_PI / 180.0f);
    const F32 MINUSCULE_ANGLE_COS = (F32) cos(0.5f * FOUR_DEGREES);

}  // namespace


///////////////////////////////////////////////////////////////////////////////////////////////

void LLVoiceWebRTCStats::reset()
{
    mStartTime = -1.0f;
    mConnectCycles = 0;
    mConnectTime = -1.0f;
    mConnectAttempts = 0;
    mProvisionTime = -1.0f;
    mProvisionAttempts = 0;
    mEstablishTime = -1.0f;
    mEstablishAttempts = 0;
}

LLVoiceWebRTCStats::LLVoiceWebRTCStats()
{
    reset();
}

LLVoiceWebRTCStats::~LLVoiceWebRTCStats()
{
}

void LLVoiceWebRTCStats::connectionAttemptStart()
{
    if (!mConnectAttempts)
    {
        mStartTime = LLTimer::getTotalTime();
        mConnectCycles++;
    }
    mConnectAttempts++;
}

void LLVoiceWebRTCStats::connectionAttemptEnd(bool success)
{
    if ( success )
    {
        mConnectTime = (LLTimer::getTotalTime() - mStartTime) / USEC_PER_SEC;
    }
}

void LLVoiceWebRTCStats::provisionAttemptStart()
{
    if (!mProvisionAttempts)
    {
        mStartTime = LLTimer::getTotalTime();
    }
    mProvisionAttempts++;
}

void LLVoiceWebRTCStats::provisionAttemptEnd(bool success)
{
    if ( success )
    {
        mProvisionTime = (LLTimer::getTotalTime() - mStartTime) / USEC_PER_SEC;
    }
}

void LLVoiceWebRTCStats::establishAttemptStart()
{
    if (!mEstablishAttempts)
    {
        mStartTime = LLTimer::getTotalTime();
    }
    mEstablishAttempts++;
}

void LLVoiceWebRTCStats::establishAttemptEnd(bool success)
{
    if ( success )
    {
        mEstablishTime = (LLTimer::getTotalTime() - mStartTime) / USEC_PER_SEC;
    }
}

LLSD LLVoiceWebRTCStats::read()
{
    LLSD stats(LLSD::emptyMap());

    stats["connect_cycles"] = LLSD::Integer(mConnectCycles);
    stats["connect_attempts"] = LLSD::Integer(mConnectAttempts);
    stats["connect_time"] = LLSD::Real(mConnectTime);

    stats["provision_attempts"] = LLSD::Integer(mProvisionAttempts);
    stats["provision_time"] = LLSD::Real(mProvisionTime);

    stats["establish_attempts"] = LLSD::Integer(mEstablishAttempts);
    stats["establish_time"] = LLSD::Real(mEstablishTime);

    return stats;
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool LLWebRTCVoiceClient::sShuttingDown = false;

LLWebRTCVoiceClient::LLWebRTCVoiceClient() :
    mHidden(false),
    mTuningMode(false),
    mTuningMicGain(0.0),
    mTuningSpeakerVolume(50),  // Set to 50 so the user can hear themselves when he sets his mic volume
    mDeviceSettingsAvailable(false),
    mDevicesListUpdated(false),

    mSpatialCoordsDirty(false),

    mMuteMic(false),

    mEarLocation(0),
    mMicGain(0.0),

    mVoiceEnabled(false),
    mProcessChannels(false),

    mAvatarNameCacheConnection(),
    mIsInTuningMode(false),
    mIsProcessingChannels(false),
    mIsTimerActive(false),
    mWebRTCPump("WebRTCClientPump"),
    mWebRTCDeviceInterface(nullptr)
{
    sShuttingDown = false;

    mSpeakerVolume = 0.0;

    mVoiceVersion.serverVersion = "";
    mVoiceVersion.voiceServerType = REPORTED_VOICE_SERVER_TYPE;
    mVoiceVersion.internalVoiceServerType = WEBRTC_VOICE_SERVER_TYPE;
    mVoiceVersion.minorVersion = 0;
    mVoiceVersion.majorVersion = 2;
    mVoiceVersion.mBuildVersion = "";
}

//---------------------------------------------------

LLWebRTCVoiceClient::~LLWebRTCVoiceClient()
{
}

void LLWebRTCVoiceClient::cleanupSingleton()
{
    if (mAvatarNameCacheConnection.connected())
    {
        mAvatarNameCacheConnection.disconnect();
    }

    sShuttingDown = true;
    if (mSession)
    {
        mSession->shutdownAllConnections();
    }
    if (mNextSession)
    {
        mNextSession->shutdownAllConnections();
    }
    cleanUp();
    stopTimer();
    sessionState::clearSessions();

    mStatusObservers.clear();
}

//---------------------------------------------------

void LLWebRTCVoiceClient::init(LLPumpIO* pump)
{
    // constructor will set up LLVoiceClient::getInstance()
    llwebrtc::init(this);

    mWebRTCDeviceInterface = llwebrtc::getDeviceInterface();
    mWebRTCDeviceInterface->setDevicesObserver(this);
    mMainQueue = LL::WorkQueue::getInstance("mainloop");
    refreshDeviceLists();
}

void LLWebRTCVoiceClient::terminate()
{
    if (sShuttingDown)
    {
        return;
    }

    mVoiceEnabled = false;
    llwebrtc::terminate();

    sShuttingDown = true;
}

//---------------------------------------------------

void LLWebRTCVoiceClient::cleanUp()
{
    mNextSession.reset();
    mSession.reset();
    mNeighboringRegions.clear();
    sessionState::for_each(boost::bind(predShutdownSession, _1));
    LL_DEBUGS("Voice") << "Exiting" << LL_ENDL;
}

void LLWebRTCVoiceClient::stopTimer()
{
    if (mIsTimerActive)
    {
        if (LLMuteList::instanceExists())
        {
            LLMuteList::getInstance()->removeObserver(this);
        }
        mIsTimerActive = false;
        LL::Timers::instance().cancel(mVoiceTimerHandle);
    }
}

void LLWebRTCVoiceClient::LogMessage(llwebrtc::LLWebRTCLogCallback::LogLevel level, const std::string& message)
{
    switch (level)
    {
    case llwebrtc::LLWebRTCLogCallback::LOG_LEVEL_VERBOSE:
        LL_DEBUGS("Voice") << message << LL_ENDL;
        break;
    case llwebrtc::LLWebRTCLogCallback::LOG_LEVEL_INFO:
        LL_INFOS("Voice") << message << LL_ENDL;
        break;
    case llwebrtc::LLWebRTCLogCallback::LOG_LEVEL_WARNING:
        LL_WARNS("Voice") << message << LL_ENDL;
        break;
    case llwebrtc::LLWebRTCLogCallback::LOG_LEVEL_ERROR:
        // use WARN so that we don't crash on a webrtc error.
        // webrtc will force a crash on a fatal error.
        LL_WARNS("Voice") << message << LL_ENDL;
        break;
    default:
        break;
    }
}

// --------------------------------------------------

const LLVoiceVersionInfo& LLWebRTCVoiceClient::getVersion()
{
    return mVoiceVersion;
}

//---------------------------------------------------

void LLWebRTCVoiceClient::updateSettings()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    setVoiceEnabled(LLVoiceClient::getInstance()->voiceEnabled());
    static LLCachedControl<S32> sVoiceEarLocation(gSavedSettings, "VoiceEarLocation");
    setEarLocation(sVoiceEarLocation);

    static LLCachedControl<std::string> sInputDevice(gSavedSettings, "VoiceInputAudioDevice");
    setCaptureDevice(sInputDevice);

    static LLCachedControl<std::string> sOutputDevice(gSavedSettings, "VoiceOutputAudioDevice");
    setRenderDevice(sOutputDevice);

    static LLCachedControl<F32> sMicLevel(gSavedSettings, "AudioLevelMic");
    setMicGain(sMicLevel);

    llwebrtc::LLWebRTCDeviceInterface::AudioConfig config;

    static LLCachedControl<bool> sEchoCancellation(gSavedSettings, "VoiceEchoCancellation", true);
    config.mEchoCancellation = sEchoCancellation;

    static LLCachedControl<bool> sAGC(gSavedSettings, "VoiceAutomaticGainControl", true);
    config.mAGC = sAGC;

    static LLCachedControl<U32> sNoiseSuppressionLevel(gSavedSettings,
                                                       "VoiceNoiseSuppressionLevel",
                                                       llwebrtc::LLWebRTCDeviceInterface::AudioConfig::ENoiseSuppressionLevel::NOISE_SUPPRESSION_LEVEL_VERY_HIGH);
    config.mNoiseSuppressionLevel = (llwebrtc::LLWebRTCDeviceInterface::AudioConfig::ENoiseSuppressionLevel) (U32)sNoiseSuppressionLevel;

    mWebRTCDeviceInterface->setAudioConfig(config);

}

// Observers
void LLWebRTCVoiceClient::addObserver(LLVoiceClientParticipantObserver *observer)
{
    mParticipantObservers.insert(observer);
}

void LLWebRTCVoiceClient::removeObserver(LLVoiceClientParticipantObserver *observer)
{
    mParticipantObservers.erase(observer);
}

void LLWebRTCVoiceClient::notifyParticipantObservers()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    for (observer_set_t::iterator it = mParticipantObservers.begin(); it != mParticipantObservers.end();)
    {
        LLVoiceClientParticipantObserver *observer = *it;
        observer->onParticipantsChanged();
        // In case onParticipantsChanged() deleted an entry.
        it = mParticipantObservers.upper_bound(observer);
    }
}

void LLWebRTCVoiceClient::addObserver(LLVoiceClientStatusObserver *observer)
{
    mStatusObservers.insert(observer);
}

void LLWebRTCVoiceClient::removeObserver(LLVoiceClientStatusObserver *observer)
{
    mStatusObservers.erase(observer);
}

void LLWebRTCVoiceClient::notifyStatusObservers(LLVoiceClientStatusObserver::EStatusType status)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    LL_DEBUGS("Voice") << "( " << LLVoiceClientStatusObserver::status2string(status) << " )"
                       << " mSession=" << mSession << LL_ENDL;

    bool in_spatial_channel = inSpatialChannel();
    LL_DEBUGS("Voice") << " " << LLVoiceClientStatusObserver::status2string(status) << ", session channelInfo "
                       << getAudioSessionChannelInfo() << ", proximal is " << in_spatial_channel << LL_ENDL;

    mIsProcessingChannels = status == LLVoiceClientStatusObserver::STATUS_JOINED;

    LLSD channelInfo = getAudioSessionChannelInfo();
    for (status_observer_set_t::iterator it = mStatusObservers.begin(); it != mStatusObservers.end();)
    {
        LLVoiceClientStatusObserver *observer = *it;
        observer->onChange(status, channelInfo, in_spatial_channel);
        // In case onError() deleted an entry.
        it = mStatusObservers.upper_bound(observer);
    }

    // skipped to avoid speak button blinking
    if (status != LLVoiceClientStatusObserver::STATUS_JOINING &&
        status != LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL &&
        status != LLVoiceClientStatusObserver::STATUS_VOICE_DISABLED)
    {
        bool voice_status = LLVoiceClient::getInstance()->voiceEnabled() && mIsProcessingChannels;

        gAgent.setVoiceConnected(voice_status);

        if (voice_status)
        {
            LLAppViewer::instance()->postToMainCoro([=]() { LLFirstUse::speak(true); });
        }
    }
}

void LLWebRTCVoiceClient::addObserver(LLFriendObserver *observer)
{
}

void LLWebRTCVoiceClient::removeObserver(LLFriendObserver *observer)
{
}

//---------------------------------------------------
// Primary voice loop.
// This voice loop is called every 100ms
// The loop does the following:
// * gates whether we do channel processing depending on
//   whether we're running a WebRTC voice channel or
//   one from another voice provider.
// * If in spatial voice, it determines whether we've changed
//   parcels, whether region/parcel voice settings have changed,
//   etc. and manages whether the voice channel needs to change.
// * calls the state machines for the sessions to negotiate
//   connection to various voice channels.
// * Sends updates to the voice server when this agent's
//   voice levels, or positions have changed.
void LLWebRTCVoiceClient::connectionTimer()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    try
    {
        bool voiceEnabled = mVoiceEnabled;

        if (!isAgentAvatarValid())
        {
            if (sShuttingDown)
            {
                cleanUp();
            }
            return;
        }

        LLViewerRegion* regionp = gAgent.getRegion();
        if (!regionp)
        {
            if (sShuttingDown)
            {
                cleanUp();
            }
            return;
        }

        if (!mProcessChannels)
        {
            // we've switched away from webrtc voice, so shut all channels down.
            // leave channel can be called again and again without adverse effects.
            // it merely tells channels to shut down if they're not already doing so.
            leaveChannel(false);
        }
        else if (inSpatialChannel())
        {
            bool useEstateVoice = true;
            // add session for region or parcel voice.
            if (!regionp || regionp->getRegionID().isNull())
            {
                // no region, no voice.
                return;
            }

            voiceEnabled = voiceEnabled && regionp->isVoiceEnabled();

            if (voiceEnabled)
            {
                LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
                // check to see if parcel changed.
                if (parcel && parcel->getLocalID() != INVALID_PARCEL_ID)
                {
                    // parcel voice
                    if (!parcel->getParcelFlagAllowVoice())
                    {
                        voiceEnabled = false;
                    }
                    else if (!parcel->getParcelFlagUseEstateVoiceChannel())
                    {
                        // use the parcel-specific voice channel.
                        S32         parcel_local_id = parcel->getLocalID();
                        std::string channelID = regionp->getRegionID().asString() + "-" + std::to_string(parcel->getLocalID());

                        useEstateVoice = false;
                        if (!inOrJoiningChannel(channelID))
                        {
                            startParcelSession(channelID, parcel_local_id);
                        }
                    }
                }
                if (voiceEnabled && useEstateVoice && !inEstateChannel())
                {
                    // estate voice
                    startEstateSession();
                }
            }
            if (!voiceEnabled)
            {
                // voice is disabled, so leave and disable PTT
                leaveChannel(true);
            }
            else
            {
                // we're in spatial voice, and voice is enabled, so determine positions in order
                // to send position updates.
                updatePosition();
            }
            LL::WorkQueue::postMaybe(mMainQueue,
                [=, this] {
                    if  (sShuttingDown)
                    {
                        return;
                    }
                    sessionState::processSessionStates();
                    if (mProcessChannels && voiceEnabled && !mHidden)
                    {
                        sendPositionUpdate(false);
                        updateOwnVolume();
                    }
            });
        }
    }
    catch (const LLContinueError&)
    {
        LOG_UNHANDLED_EXCEPTION("LLWebRTCVoiceClient");
    }
    catch (...)
    {
        // Ideally for Windows need to log SEH exception instead or to set SEH
        // handlers but bugsplat shows local variables for windows, which should
        // be enough
        LL_WARNS("Voice") << "voiceConnectionStateMachine crashed" << LL_ENDL;
        throw;
    }

    if (sShuttingDown)
    {
        cleanUp();
    }
}

// For spatial, determine which neighboring regions to connect to
// for cross-region voice.
void LLWebRTCVoiceClient::updateNeighboringRegions()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    static const std::vector<LLVector3d> neighbors {LLVector3d(0.0f, 1.0f, 0.0f),  LLVector3d(0.707f, 0.707f, 0.0f),
                                                    LLVector3d(1.0f, 0.0f, 0.0f),  LLVector3d(0.707f, -0.707f, 0.0f),
                                                    LLVector3d(0.0f, -1.0f, 0.0f), LLVector3d(-0.707f, -0.707f, 0.0f),
                                                    LLVector3d(-1.0f, 0.0f, 0.0f), LLVector3d(-0.707f, 0.707f, 0.0f)};

    // Estate voice requires connection to neighboring regions.
    mNeighboringRegions.clear();

    // add current region.
    mNeighboringRegions.insert(gAgent.getRegion()->getRegionID());

    // base off of speaker position as it'll move more slowly than camera position.
    // Once we have hysteresis, we may be able to track off of speaker and camera position at 50m
    // TODO: Add hysteresis so we don't flip-flop connections to neighbors
    LLVector3d speaker_pos = LLWebRTCVoiceClient::getInstance()->getSpeakerPosition();
    for (auto &neighbor_pos : neighbors)
    {
        // include every region within 100m (2*MAX_AUDIO_DIST) to deal witht he fact that the camera
        // can stray 50m away from the avatar.
        LLViewerRegion *neighbor = LLWorld::instance().getRegionFromPosGlobal(speaker_pos + 2 * MAX_AUDIO_DIST * neighbor_pos);
        if (neighbor && !neighbor->getRegionID().isNull())
        {
            mNeighboringRegions.insert(neighbor->getRegionID());
        }
    }
}

//=========================================================================
// shut down the current audio session to make room for the next one.
void LLWebRTCVoiceClient::leaveAudioSession()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    if(mSession)
    {
        LL_DEBUGS("Voice") << "leaving session: " << mSession->mChannelID << LL_ENDL;
        mSession->shutdownAllConnections();
    }
    else
    {
        LL_WARNS("Voice") << "called with no active session" << LL_ENDL;
    }
}

//=========================================================================
// Device Management
void LLWebRTCVoiceClient::clearCaptureDevices()
{
    LL_DEBUGS("Voice") << "called" << LL_ENDL;
    mDeviceSettingsAvailable = false;
    mCaptureDevices.clear();
}

void LLWebRTCVoiceClient::addCaptureDevice(const LLVoiceDevice& device)
{
    LL_INFOS("Voice") << "Voice Capture Device: '" << device.display_name << "' (" << device.full_name << ")" << LL_ENDL;
    mDeviceSettingsAvailable = false;
    mCaptureDevices.push_back(device);
}

LLVoiceDeviceList& LLWebRTCVoiceClient::getCaptureDevices()
{
    return mCaptureDevices;
}

void LLWebRTCVoiceClient::setCaptureDevice(const std::string& name)
{
    mWebRTCDeviceInterface->setCaptureDevice(name);
}

void LLWebRTCVoiceClient::setDevicesListUpdated(bool state)
{
    mDevicesListUpdated = state;
}

// the singleton 'this' pointer will outlive the work queue.
void LLWebRTCVoiceClient::OnDevicesChanged(const llwebrtc::LLWebRTCVoiceDeviceList& render_devices,
                                           const llwebrtc::LLWebRTCVoiceDeviceList& capture_devices)
{

    LL::WorkQueue::postMaybe(mMainQueue,
        [=, this]
        {
            OnDevicesChangedImpl(render_devices, capture_devices);
        });
}

void LLWebRTCVoiceClient::OnDevicesChangedImpl(const llwebrtc::LLWebRTCVoiceDeviceList &render_devices,
                                               const llwebrtc::LLWebRTCVoiceDeviceList &capture_devices)
{
    if (sShuttingDown)
    {
        return;
    }
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;
    std::string inputDevice = gSavedSettings.getString("VoiceInputAudioDevice");
    std::string outputDevice = gSavedSettings.getString("VoiceOutputAudioDevice");

    LL_DEBUGS("Voice") << "Setting devices to-input: '" << inputDevice << "' output: '" << outputDevice << "'" << LL_ENDL;
    clearRenderDevices();
    for (auto &device : render_devices)
    {
        addRenderDevice(LLVoiceDevice(device.mDisplayName, device.mID));
    }
    setRenderDevice(outputDevice);

    clearCaptureDevices();
    for (auto &device : capture_devices)
    {
        LL_DEBUGS("Voice") << "Checking capture device:'" << device.mID << "'" << LL_ENDL;

        addCaptureDevice(LLVoiceDevice(device.mDisplayName, device.mID));
    }
    setCaptureDevice(inputDevice);

    mDeviceSettingsAvailable = true;
    setDevicesListUpdated(true);
}

void LLWebRTCVoiceClient::clearRenderDevices()
{
    LL_DEBUGS("Voice") << "called" << LL_ENDL;
    mDeviceSettingsAvailable = false;
    mRenderDevices.clear();
}

void LLWebRTCVoiceClient::addRenderDevice(const LLVoiceDevice& device)
{
    LL_INFOS("Voice") << "Voice Render Device: '" << device.display_name << "' (" << device.full_name << ")" << LL_ENDL;
    mDeviceSettingsAvailable = false;
    mRenderDevices.push_back(device);
}

LLVoiceDeviceList& LLWebRTCVoiceClient::getRenderDevices()
{
    return mRenderDevices;
}

void LLWebRTCVoiceClient::setRenderDevice(const std::string& name)
{
    mWebRTCDeviceInterface->setRenderDevice(name);
}

bool LLWebRTCVoiceClient::isCaptureNoDevice()
{
    return mCaptureDevices.empty() || mWebRTCDeviceInterface->isCaptureNoDevice();
}

bool LLWebRTCVoiceClient::isRenderNoDevice()
{
    return mRenderDevices.empty() || mWebRTCDeviceInterface->isRenderNoDevice();
}

void LLWebRTCVoiceClient::tuningStart()
{
    if (!mIsInTuningMode)
    {
        mWebRTCDeviceInterface->setTuningMode(true);
        mIsInTuningMode = true;
    }
}

void LLWebRTCVoiceClient::tuningStop()
{
    if (mIsInTuningMode)
    {
        mWebRTCDeviceInterface->setTuningMode(false);
        mIsInTuningMode = false;
    }
}

bool LLWebRTCVoiceClient::inTuningMode()
{
    return mIsInTuningMode;
}

void LLWebRTCVoiceClient::tuningSetMicVolume(float volume)
{
    mTuningMicGain = volume;
}

void LLWebRTCVoiceClient::tuningSetSpeakerVolume(float volume)
{
    if (isRenderNoDevice())
    {
        volume = 0;
    }

    if (volume != mTuningSpeakerVolume)
    {
        mTuningSpeakerVolume = (int)volume;
    }
}

float LLWebRTCVoiceClient::getAudioLevel()
{
    if (isCaptureNoDevice())
    {
        return 0;
    }

    if (mIsInTuningMode)
    {
        return (1.0f - mWebRTCDeviceInterface->getTuningAudioLevel() * LEVEL_SCALE_WEBRTC) * mTuningMicGain / 2.1f;
    }

    return (1.0f - mWebRTCDeviceInterface->getPeerConnectionAudioLevel() * LEVEL_SCALE_WEBRTC) * mMicGain / 2.1f;
}

float LLWebRTCVoiceClient::tuningGetEnergy(void)
{
    return getAudioLevel();
}

bool LLWebRTCVoiceClient::deviceSettingsUpdated()
{
    bool updated = mDevicesListUpdated;
    mDevicesListUpdated = false;
    return updated;
}

void LLWebRTCVoiceClient::refreshDeviceLists(bool clearCurrentList)
{
    if (clearCurrentList)
    {
        clearCaptureDevices();
        clearRenderDevices();
    }
    mWebRTCDeviceInterface->refreshDevices();
}

void LLWebRTCVoiceClient::setHidden(bool hidden)
{
    mHidden = hidden;

    if (inSpatialChannel())
    {
        if (mHidden)
        {
            // get out of the channel entirely
            // mute the microphone.
            sessionState::for_each(boost::bind(predSetMuteMic, _1, true));
        }
        else
        {
            // and put it back
            sessionState::for_each(boost::bind(predSetMuteMic, _1, mMuteMic));
            updatePosition();
            sendPositionUpdate(true);
        }
    }
}

/////////////////////////////
// session control messages.
//
// these are called by the sessions to report
// status for a given channel.  By filtering
// on channel and region, these functions
// can send various notifications to
// other parts of the viewer, as well as
// managing housekeeping

// A connection to a channel was successfully established,
// so shut down the current session and move on to the next
// if one is available.
// if the current session is the one that was established,
// notify the observers.
void LLWebRTCVoiceClient::OnConnectionEstablished(const std::string &channelID, const LLUUID &regionID)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    if (gAgent.getRegion()->getRegionID() == regionID)
    {
        if (mNextSession && mNextSession->mChannelID == channelID)
        {
            if (mSession)
            {
                mSession->shutdownAllConnections();
            }
            mSession = mNextSession;
            mNextSession.reset();
        }

        if (mSession)
        {
            // Add ourselves as a participant.
            mSession->addParticipant(gAgentID, gAgent.getRegion()->getRegionID());
        }

        // The current session was established.
        if (mSession && mSession->mChannelID == channelID)
        {
            LLWebRTCVoiceClient::getInstance()->notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LOGGED_IN);

            // only set status to joined if asked to.  This will happen in the case where we're not
            // doing an ad-hoc based p2p session. Those sessions expect a STATUS_JOINED when the peer
            // has, in fact, joined, which we detect elsewhere.
            if (!mSession->mNotifyOnFirstJoin)
            {
                LLWebRTCVoiceClient::getInstance()->notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINED);
            }
        }
    }
}

void LLWebRTCVoiceClient::OnConnectionShutDown(const std::string &channelID, const LLUUID &regionID)
{
    if (mSession && (mSession->mChannelID == channelID))
    {
        if (gAgent.getRegion()->getRegionID() == regionID)
        {
            if (mSession && mSession->mChannelID == channelID)
            {
                LL_DEBUGS("Voice") << "Main WebRTC Connection Shut Down." << LL_ENDL;
            }
        }
        mSession->removeAllParticipants(regionID);
    }
}

void LLWebRTCVoiceClient::OnConnectionFailure(const std::string                       &channelID,
                                              const LLUUID                            &regionID,
                                              LLVoiceClientStatusObserver::EStatusType status_type)
{
    LL_DEBUGS("Voice") << "A connection failed.  channel:" << channelID << LL_ENDL;
    if (gAgent.getRegion()->getRegionID() == regionID)
    {
        if (mNextSession && mNextSession->mChannelID == channelID)
        {
            LLWebRTCVoiceClient::getInstance()->notifyStatusObservers(status_type);
        }
        else if (mSession && mSession->mChannelID == channelID)
        {
            LLWebRTCVoiceClient::getInstance()->notifyStatusObservers(status_type);
        }
    }
}

// -----------------------------------------------------------
// positional functionality.
void LLWebRTCVoiceClient::setEarLocation(S32 loc)
{
    if (mEarLocation != loc)
    {
        LL_DEBUGS("Voice") << "Setting mEarLocation to " << loc << LL_ENDL;

        mEarLocation        = loc;
        mSpatialCoordsDirty = true;
    }
}

void LLWebRTCVoiceClient::updatePosition(void)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    LLViewerRegion *region = gAgent.getRegion();
    if (region && isAgentAvatarValid())
    {
        // get the avatar position.
        LLVector3d   avatar_pos  = gAgentAvatarp->getPositionGlobal();
        LLQuaternion avatar_qrot = gAgentAvatarp->getRootJoint()->getWorldRotation();

        avatar_pos += LLVector3d(0.f, 0.f, 1.f);  // bump it up to head height

        LLVector3d   earPosition;
        LLQuaternion earRot;
        switch (mEarLocation)
        {
            case earLocCamera:
            default:
                earPosition = region->getPosGlobalFromRegion(LLViewerCamera::getInstance()->getOrigin());
                earRot      = LLViewerCamera::getInstance()->getQuaternion();
                break;

            case earLocAvatar:
                earPosition = mAvatarPosition;
                earRot      = mAvatarRot;
                break;

            case earLocMixed:
                earPosition = mAvatarPosition;
                earRot      = LLViewerCamera::getInstance()->getQuaternion();
                break;
        }
        setListenerPosition(earPosition,      // position
                            LLVector3::zero,  // velocity
                            earRot);          // rotation matrix

        setAvatarPosition(avatar_pos,       // position
                          LLVector3::zero,  // velocity
                          avatar_qrot);     // rotation matrix

        enforceTether();

        updateNeighboringRegions();

        // update own region id to be the region id avatar is currently in.
        LLWebRTCVoiceClient::participantStatePtr_t participant = findParticipantByID("Estate", gAgentID);
        if(participant)
        {
            participant->mRegion = gAgent.getRegion()->getRegionID();
        }
    }
}

void LLWebRTCVoiceClient::setListenerPosition(const LLVector3d &position, const LLVector3 &velocity, const LLQuaternion &rot)
{
    mListenerRequestedPosition = position;

    if (mListenerVelocity != velocity)
    {
        mListenerVelocity   = velocity;
        mSpatialCoordsDirty = true;
    }

    if (mListenerRot != rot)
    {
        mListenerRot        = rot;
        mSpatialCoordsDirty = true;
    }
}

void LLWebRTCVoiceClient::setAvatarPosition(const LLVector3d &position, const LLVector3 &velocity, const LLQuaternion &rot)
{
    if (dist_vec_squared(mAvatarPosition, position) > 0.01)
    {
        mAvatarPosition     = position;
        mSpatialCoordsDirty = true;
    }

    if (mAvatarVelocity != velocity)
    {
        mAvatarVelocity     = velocity;
        mSpatialCoordsDirty = true;
    }

    // If the two rotations are not exactly equal test their dot product
    // to get the cos of the angle between them.
    // If it is too small, don't update.
    F32 rot_cos_diff = llabs(dot(mAvatarRot, rot));
    if ((mAvatarRot != rot) && (rot_cos_diff < MINUSCULE_ANGLE_COS))
    {
        mAvatarRot          = rot;
        mSpatialCoordsDirty = true;
    }
}

// The listener (camera) must be within 50m of the
// avatar.  Enforce it on the client.
// This will also be enforced on the voice server
// based on position sent from the simulator to the
// voice server.
void LLWebRTCVoiceClient::enforceTether()
{
    LLVector3d tethered = mListenerRequestedPosition;

    // constrain 'tethered' to within 50m of mAvatarPosition.
    {
        LLVector3d camera_offset   = mListenerRequestedPosition - mAvatarPosition;
        F32        camera_distance = (F32) camera_offset.magVec();
        if (camera_distance > MAX_AUDIO_DIST)
        {
            tethered = mAvatarPosition + (MAX_AUDIO_DIST / camera_distance) * camera_offset;
        }
    }

    if (dist_vec_squared(mListenerPosition, tethered) > 0.01)
    {
        mListenerPosition   = tethered;
        mSpatialCoordsDirty = true;
    }
}

// We send our position via a WebRTC data channel to the WebRTC
// server for fine-grained, low latency updates.  On the server,
// these updates will be 'tethered' to the actual position of the avatar.
// Those updates are higher latency, however.
// This mechanism gives low latency spatial updates and server-enforced
// prevention of 'evesdropping' by sending camera updates beyond the
// standard 50m
void LLWebRTCVoiceClient::sendPositionUpdate(bool force)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    std::string      spatial_data;

    if (mSpatialCoordsDirty || force)
    {
        boost::json::object spatial;

        spatial["sp"] = {
            {"x", (int) (mAvatarPosition[0] * 100)},
            {"y", (int) (mAvatarPosition[1] * 100)},
            {"z", (int) (mAvatarPosition[2] * 100)}
        };
        spatial["sh"]  = {
            {"x", (int) (mAvatarRot[0] * 100)},
            {"y", (int) (mAvatarRot[1] * 100)},
            {"z", (int) (mAvatarRot[2] * 100)},
            {"w", (int) (mAvatarRot[3] * 100)}
        };

        spatial["lp"] = {
            {"x", (int) (mListenerPosition[0] * 100)},
            {"y", (int) (mListenerPosition[1] * 100)},
            {"z", (int) (mListenerPosition[2] * 100)}
        };

        spatial["lh"] = {
            {"x", (int) (mListenerRot[0] * 100)},
            {"y", (int) (mListenerRot[1] * 100)},
            {"z", (int) (mListenerRot[2] * 100)},
            {"w", (int) (mListenerRot[3] * 100)}};

        mSpatialCoordsDirty = false;
        spatial_data = boost::json::serialize(spatial);

        sessionState::for_each(boost::bind(predSendData, _1, spatial_data));
    }
}

// Update our own volume on our participant, so it'll show up
// in the UI.  This is done on all sessions, so switching
// sessions retains consistent volume levels.
void LLWebRTCVoiceClient::updateOwnVolume() {
    F32 audio_level = 0.0;
    if (!mMuteMic && !mTuningMode)
    {
        audio_level = getAudioLevel();
    }

    sessionState::for_each(boost::bind(predUpdateOwnVolume, _1, audio_level));
}

////////////////////////////////////
// Managing list of participants

// Provider-level participant management

bool LLWebRTCVoiceClient::isParticipantAvatar(const LLUUID &id)
{
    // WebRTC participants are always SL avatars.
    return true;
}

void LLWebRTCVoiceClient::getParticipantList(std::set<LLUUID> &participants)
{
    if (mProcessChannels && mSession)
    {
        for (participantUUIDMap::iterator iter = mSession->mParticipantsByUUID.begin();
            iter != mSession->mParticipantsByUUID.end();
            iter++)
        {
            participants.insert(iter->first);
        }
    }
}

bool LLWebRTCVoiceClient::isParticipant(const LLUUID &speaker_id)
{
    if (mProcessChannels && mSession)
    {
        return (mSession->mParticipantsByUUID.find(speaker_id) != mSession->mParticipantsByUUID.end());
    }
    return false;
}

// protected provider-level participant management.
LLWebRTCVoiceClient::participantStatePtr_t LLWebRTCVoiceClient::findParticipantByID(const std::string &channelID, const LLUUID &id)
{
    participantStatePtr_t result;
    LLWebRTCVoiceClient::sessionState::ptr_t session = sessionState::matchSessionByChannelID(channelID);

    if (session)
    {
        result = session->findParticipantByID(id);
    }

    return result;
}

LLWebRTCVoiceClient::participantStatePtr_t LLWebRTCVoiceClient::addParticipantByID(const std::string &channelID, const LLUUID &id, const LLUUID& region)
{
    participantStatePtr_t result;
    LLWebRTCVoiceClient::sessionState::ptr_t session = sessionState::matchSessionByChannelID(channelID);
    if (session)
    {
        result = session->addParticipant(id, region);
        if (session->mNotifyOnFirstJoin && (id != gAgentID))
        {
            notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINED);
        }
    }
    return result;
}

void LLWebRTCVoiceClient::removeParticipantByID(const std::string &channelID, const LLUUID &id, const LLUUID& region)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    participantStatePtr_t result;
    LLWebRTCVoiceClient::sessionState::ptr_t session = sessionState::matchSessionByChannelID(channelID);
    if (session)
    {
        participantStatePtr_t participant = session->findParticipantByID(id);
        if (participant && (participant->mRegion == region))
        {
            session->removeParticipant(participant);
        }
    }
}


//  participantState level participant management
LLWebRTCVoiceClient::participantState::participantState(const LLUUID& agent_id, const LLUUID& region) :
     mURI(agent_id.asString()),
     mAvatarID(agent_id),
     mIsSpeaking(false),
     mIsModeratorMuted(false),
     mLevel(0.f),
     mVolume(LLVoiceClient::VOLUME_DEFAULT),
     mRegion(region)
{
}

LLWebRTCVoiceClient::participantStatePtr_t LLWebRTCVoiceClient::sessionState::addParticipant(const LLUUID& agent_id, const LLUUID& region)
{

    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    participantStatePtr_t result;

    participantUUIDMap::iterator iter = mParticipantsByUUID.find(agent_id);

    if (iter != mParticipantsByUUID.end())
    {
        result = iter->second;
        result->mRegion = region;
    }

    if (!result)
    {
        // participant isn't already in one list or the other.
        result.reset(new participantState(agent_id, region));
        mParticipantsByUUID.insert(participantUUIDMap::value_type(agent_id, result));
        result->mAvatarID = agent_id;
    }

    LLWebRTCVoiceClient::getInstance()->lookupName(agent_id);

    LLSpeakerVolumeStorage::getInstance()->getSpeakerVolume(result->mAvatarID, result->mVolume);
    if (!LLWebRTCVoiceClient::sShuttingDown)
    {
        LLWebRTCVoiceClient::getInstance()->notifyParticipantObservers();
    }

    LL_DEBUGS("Voice") << "Participant \"" << result->mURI << "\" added." << LL_ENDL;

    return result;
}


// session-level participant management

LLWebRTCVoiceClient::participantStatePtr_t LLWebRTCVoiceClient::sessionState::findParticipantByID(const LLUUID& id)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    participantStatePtr_t result;
    participantUUIDMap::iterator iter = mParticipantsByUUID.find(id);

    if(iter != mParticipantsByUUID.end())
    {
        result = iter->second;
    }

    return result;
}

void LLWebRTCVoiceClient::sessionState::removeParticipant(const LLWebRTCVoiceClient::participantStatePtr_t &participant)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    if (participant)
    {
        LLUUID participantID = participant->mAvatarID;
        participantUUIDMap::iterator iter = mParticipantsByUUID.find(participant->mAvatarID);

        LL_DEBUGS("Voice") << "participant \"" << participant->mURI << "\" (" << participantID << ") removed." << LL_ENDL;

        if (iter == mParticipantsByUUID.end())
        {
            LL_WARNS("Voice") << "Internal error: participant ID " << participantID << " not in UUID map" << LL_ENDL;
        }
        else
        {
            mParticipantsByUUID.erase(iter);
            if (!LLWebRTCVoiceClient::sShuttingDown)
            {
                LLWebRTCVoiceClient::getInstance()->notifyParticipantObservers();
            }
        }
        if (mHangupOnLastLeave && (participantID != gAgentID) && (mParticipantsByUUID.size() <= 1) && LLWebRTCVoiceClient::instanceExists())
        {
            LLWebRTCVoiceClient::getInstance()->notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL);
        }
    }
}

void LLWebRTCVoiceClient::sessionState::removeAllParticipants(const LLUUID &region)
{
    std::vector<participantStatePtr_t> participantsToRemove;

    for (auto& participantEntry : mParticipantsByUUID)
    {
        if (region.isNull() || (participantEntry.second->mRegion == region))
        {
            participantsToRemove.push_back(participantEntry.second);
        }
    }
    for (auto& participant : participantsToRemove)
    {
        removeParticipant(participant);
    }
}

// Initiated the various types of sessions.
bool LLWebRTCVoiceClient::startEstateSession()
{
    leaveChannel(false);
    mNextSession = addSession("Estate", sessionState::ptr_t(new estateSessionState()));
    return true;
}

bool LLWebRTCVoiceClient::startParcelSession(const std::string &channelID, S32 parcelID)
{
    leaveChannel(false);
    mNextSession = addSession(channelID, sessionState::ptr_t(new parcelSessionState(channelID, parcelID)));
    return true;
}

bool LLWebRTCVoiceClient::startAdHocSession(const LLSD& channelInfo, bool notify_on_first_join, bool hangup_on_last_leave)
{
    leaveChannel(false);
    LL_WARNS("Voice") << "Start AdHoc Session " << channelInfo << LL_ENDL;
    std::string channelID = channelInfo["channel_uri"];
    std::string credentials = channelInfo["channel_credentials"];
    mNextSession = addSession(channelID,
                              sessionState::ptr_t(new adhocSessionState(channelID,
                                                                        credentials,
                                                                        notify_on_first_join,
                                                                        hangup_on_last_leave)));
    return true;
}

bool LLWebRTCVoiceClient::isVoiceWorking() const
{
    // webrtc is working if the coroutine is active in the case of
    // webrtc. WebRTC doesn't need to connect to a secondary process
    // or a login server to become active.
    return mIsTimerActive;
}

// Returns true if calling back the session URI after the session has closed is possible.
// Currently this will be false only for PSTN P2P calls.
bool LLWebRTCVoiceClient::isSessionCallBackPossible(const LLUUID &session_id)
{
    sessionStatePtr_t session(findP2PSession(session_id));
    return session && session->isCallbackPossible();
}

// Channel Management

bool LLWebRTCVoiceClient::setSpatialChannel(const LLSD &channelInfo)
{
    LL_INFOS("Voice") << "SetSpatialChannel " << channelInfo << LL_ENDL;
    LLViewerRegion *regionp = gAgent.getRegion();
    if (!regionp)
    {
        return false;
    }
    LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();

    // we don't really have credentials for a spatial channel in webrtc,
    // it's all handled by the sim.
    if (channelInfo.isMap() && channelInfo.has("channel_uri"))
    {
        bool allow_voice = !channelInfo["channel_uri"].asString().empty();
        if (parcel)
        {
            parcel->setParcelFlag(PF_ALLOW_VOICE_CHAT, allow_voice);
            parcel->setParcelFlag(PF_USE_ESTATE_VOICE_CHAN, channelInfo["channel_uri"].asUUID() == regionp->getRegionID());
        }
        else
        {
            regionp->setRegionFlag(REGION_FLAGS_ALLOW_VOICE, allow_voice);
        }
    }
    return true;
}

void LLWebRTCVoiceClient::leaveNonSpatialChannel()
{
    LL_DEBUGS("Voice") << "Request to leave non-spatial channel." << LL_ENDL;

    // make sure we're not simply rejoining the current session
    deleteSession(mNextSession);

    leaveChannel(true);
}

// determine whether we're processing channels, or whether
// another voice provider is.
void LLWebRTCVoiceClient::processChannels(bool process)
{
    mProcessChannels = process;
}

bool LLWebRTCVoiceClient::inProximalChannel()
{
    return inSpatialChannel();
}

bool LLWebRTCVoiceClient::inOrJoiningChannel(const std::string& channelID)
{
    return (mSession && mSession->mChannelID == channelID) || (mNextSession && mNextSession->mChannelID == channelID);
}

bool LLWebRTCVoiceClient::inEstateChannel()
{
    return (mSession && mSession->isEstate()) || (mNextSession && mNextSession->isEstate());
}

bool LLWebRTCVoiceClient::inSpatialChannel()
{
    bool result = true;

    if (mNextSession)
    {
        result = mNextSession->isSpatial();
    }
    else if(mSession)
    {
        result = mSession->isSpatial();
    }

    return result;
}

// retrieves information used to negotiate p2p, adhoc, and group
// channels
LLSD LLWebRTCVoiceClient::getAudioSessionChannelInfo()
{
    LLSD result;

    if (mSession)
    {
        result["voice_server_type"]   = WEBRTC_VOICE_SERVER_TYPE;
        result["channel_uri"]         = mSession->mChannelID;
    }

    return result;
}

void LLWebRTCVoiceClient::leaveChannel(bool stopTalking)
{
    if (mSession)
    {
        deleteSession(mSession);
    }

    if (mNextSession)
    {
        deleteSession(mNextSession);
    }

    // If voice was on, turn it off
    if (stopTalking && LLVoiceClient::getInstance()->getUserPTTState())
    {
        LLVoiceClient::getInstance()->setUserPTTState(false);
    }
}

bool LLWebRTCVoiceClient::isCurrentChannel(const LLSD &channelInfo)
{
    if (!mProcessChannels || (channelInfo["voice_server_type"].asString() != WEBRTC_VOICE_SERVER_TYPE))
    {
        return false;
    }

    sessionStatePtr_t session = mSession;
    if (!session)
    {
        session = mNextSession;
    }

    if (session)
    {
        if (!channelInfo["session_handle"].asString().empty())
        {
            return session->mHandle == channelInfo["session_handle"].asString();
        }
        return channelInfo["channel_uri"].asString() == session->mChannelID;
    }
    return false;
}

bool LLWebRTCVoiceClient::compareChannels(const LLSD &channelInfo1, const LLSD &channelInfo2)
{
    return (channelInfo1["voice_server_type"] == WEBRTC_VOICE_SERVER_TYPE) &&
           (channelInfo1["voice_server_type"] == channelInfo2["voice_server_type"]) &&
           (channelInfo1["sip_uri"] == channelInfo2["sip_uri"]);
}


//----------------------------------------------
// Audio muting, volume, gain, etc.

// we're muting the mic, so tell each session such
void LLWebRTCVoiceClient::setMuteMic(bool muted)
{
    mMuteMic = muted;
    // when you're hidden, your mic is always muted.
    if (!mHidden)
    {
        sessionState::for_each(boost::bind(predSetMuteMic, _1, muted));
    }
}

void LLWebRTCVoiceClient::predSetMuteMic(const LLWebRTCVoiceClient::sessionStatePtr_t &session, bool muted)
{
    participantStatePtr_t participant = session->findParticipantByID(gAgentID);
    if (participant)
    {
        participant->mLevel = 0.0;
    }
    session->setMuteMic(muted);
}

void LLWebRTCVoiceClient::setVoiceVolume(F32 volume)
{
    if (volume != mSpeakerVolume)
    {
        mSpeakerVolume  = volume;
        sessionState::for_each(boost::bind(predSetSpeakerVolume, _1, volume));
    }
}

void LLWebRTCVoiceClient::predSetSpeakerVolume(const LLWebRTCVoiceClient::sessionStatePtr_t &session, F32 volume)
{
    session->setSpeakerVolume(volume);
}

void LLWebRTCVoiceClient::setMicGain(F32 gain)
{
    if (gain != mMicGain)
    {
        mMicGain = gain;
        mWebRTCDeviceInterface->setPeerConnectionGain(gain);
    }
}

void LLWebRTCVoiceClient::setVoiceEnabled(bool enabled)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    LL_DEBUGS("Voice")
        << "( " << (enabled ? "enabled" : "disabled") << " )"
        << " was "<< (mVoiceEnabled ? "enabled" : "disabled")
        << " coro "<< (mIsTimerActive ? "active" : "inactive")
        << LL_ENDL;

    if (enabled != mVoiceEnabled)
    {
        // TODO: Refactor this so we don't call into LLVoiceChannel, but simply
        // use the status observer
        mVoiceEnabled = enabled;
        LLVoiceClientStatusObserver::EStatusType status;

        if (enabled)
        {
            LL_DEBUGS("Voice") << "enabling" << LL_ENDL;
            LLVoiceChannel::getCurrentVoiceChannel()->activate();
            status = LLVoiceClientStatusObserver::STATUS_VOICE_ENABLED;
            mSpatialCoordsDirty = true;
            updatePosition();
            if (!mIsTimerActive)
            {
                LL_DEBUGS("Voice") << "Starting" << LL_ENDL;
                mIsTimerActive = true;
                LLMuteList::getInstance()->addObserver(this);
                mVoiceTimerHandle = LL::Timers::instance().scheduleEvery([this]() { connectionTimer(); return false; }, UPDATE_THROTTLE_SECONDS);
            }
            else
            {
                LL_DEBUGS("Voice") << "coro should be active.. not launching" << LL_ENDL;
            }
        }
        else
        {
            // Turning voice off looses your current channel -- this makes sure the UI isn't out of sync when you re-enable it.
            LLVoiceChannel::getCurrentVoiceChannel()->deactivate();
            gAgent.setVoiceConnected(false);
            status = LLVoiceClientStatusObserver::STATUS_VOICE_DISABLED;
            cleanUp();
        }

        notifyStatusObservers(status);
    }
    else
    {
        LL_DEBUGS("Voice") << " no-op" << LL_ENDL;
    }
}


/////////////////////////////
// Accessors for data related to nearby speakers

std::string LLWebRTCVoiceClient::getDisplayName(const LLUUID& id)
{
    std::string result;
    if (mProcessChannels && mSession)
    {
        participantStatePtr_t participant(mSession->findParticipantByID(id));
        if (participant)
        {
            result = participant->mDisplayName;
        }
    }
    return result;
}

bool LLWebRTCVoiceClient::getIsSpeaking(const LLUUID& id)
{
    bool result = false;
    if (mProcessChannels && mSession)
    {
        participantStatePtr_t participant(mSession->findParticipantByID(id));
        if (participant)
        {
            result = participant->mIsSpeaking;
        }
    }
    return result;
}

// TODO: Need to pull muted status from the webrtc server
bool LLWebRTCVoiceClient::getIsModeratorMuted(const LLUUID& id)
{
    bool result = false;
    if (mProcessChannels && mSession)
    {
        participantStatePtr_t participant(mSession->findParticipantByID(id));
        if (participant)
        {
            result = participant->mIsModeratorMuted;
        }
    }
    return result;
}

F32 LLWebRTCVoiceClient::getCurrentPower(const LLUUID &id)
{
    F32 result = 0.0;
    if (!mProcessChannels || !mSession)
    {
        return result;
    }
    participantStatePtr_t participant(mSession->findParticipantByID(id));
    if (participant)
    {
        if (participant->mIsSpeaking)
        {
            result = participant->mLevel;
        }
    }
    return result;
}

// External accessors.
F32 LLWebRTCVoiceClient::getUserVolume(const LLUUID& id)
{
    // Minimum volume will be returned for users with voice disabled
    F32 result = LLVoiceClient::VOLUME_MIN;

    if (mSession)
    {
        participantStatePtr_t participant(mSession->findParticipantByID(id));
        if (participant)
        {
            result = participant->mVolume;
        }
    }

    return result;
}

void LLWebRTCVoiceClient::setUserVolume(const LLUUID& id, F32 volume)
{
    F32 clamped_volume = llclamp(volume, LLVoiceClient::VOLUME_MIN, LLVoiceClient::VOLUME_MAX);
    if(mSession)
    {
        participantStatePtr_t participant(mSession->findParticipantByID(id));
        if (participant && (participant->mAvatarID != gAgentID))
        {
            if (!is_approx_equal(volume, LLVoiceClient::VOLUME_DEFAULT))
            {
                // Store this volume setting for future sessions if it has been
                // changed from the default
                LLSpeakerVolumeStorage::getInstance()->storeSpeakerVolume(id, volume);
            }
            else
            {
                // Remove stored volume setting if it is returned to the default
                LLSpeakerVolumeStorage::getInstance()->removeSpeakerVolume(id);
            }

            participant->mVolume = clamped_volume;
        }
    }
    sessionState::for_each(boost::bind(predSetUserVolume, _1, id, clamped_volume));
}

// set volume level (gain level) for another user.
void LLWebRTCVoiceClient::predSetUserVolume(const LLWebRTCVoiceClient::sessionStatePtr_t &session, const LLUUID &id, F32 volume)
{
    session->setUserVolume(id, volume);
}

////////////////////////
///LLMuteListObserver
///

void LLWebRTCVoiceClient::onChange()
{
}

void LLWebRTCVoiceClient::onChangeDetailed(const LLMute& mute)
{
    if (mute.mType == LLMute::AGENT)
    {
        bool muted = ((mute.mFlags & LLMute::flagVoiceChat) == 0);
        sessionState::for_each(boost::bind(predSetUserMute, _1, mute.mID, muted));
    }
}

void LLWebRTCVoiceClient::predSetUserMute(const LLWebRTCVoiceClient::sessionStatePtr_t &session, const LLUUID &id, bool mute)
{
    session->setUserMute(id, mute);
}

//------------------------------------------------------------------------
// Sessions

std::map<std::string, LLWebRTCVoiceClient::sessionState::ptr_t> LLWebRTCVoiceClient::sessionState::sSessions;


LLWebRTCVoiceClient::sessionState::sessionState() :
    mHangupOnLastLeave(false),
    mNotifyOnFirstJoin(false),
    mMuted(false),
    mSpeakerVolume(1.0),
    mShuttingDown(false)
{
}
// ------------------------------------------------------------------
// Predicates, for calls to all sessions

void LLWebRTCVoiceClient::predUpdateOwnVolume(const LLWebRTCVoiceClient::sessionStatePtr_t &session, F32 audio_level)
{
    participantStatePtr_t participant = session->findParticipantByID(gAgentID);
    if (participant)
    {
        participant->mLevel = audio_level;
        // TODO: Add VAD for our own voice.
        participant->mIsSpeaking = audio_level > SPEAKING_AUDIO_LEVEL;
    }
}

void LLWebRTCVoiceClient::predSendData(const LLWebRTCVoiceClient::sessionStatePtr_t &session, const std::string &spatial_data)
{
    if (session->isSpatial() && !spatial_data.empty())
    {
        session->sendData(spatial_data);
    }
}

void LLWebRTCVoiceClient::sessionState::sendData(const std::string &data)
{
    for (auto &connection : mWebRTCConnections)
    {
        connection->sendData(data);
    }
}

void LLWebRTCVoiceClient::sessionState::setMuteMic(bool muted)
{
    mMuted = muted;
    for (auto &connection : mWebRTCConnections)
    {
        connection->setMuteMic(muted);
    }
}

void LLWebRTCVoiceClient::sessionState::setSpeakerVolume(F32 volume)
{
    mSpeakerVolume = volume;
    for (auto &connection : mWebRTCConnections)
    {
        connection->setSpeakerVolume(volume);
    }
}

void LLWebRTCVoiceClient::sessionState::setUserVolume(const LLUUID &id, F32 volume)
{
    if (mParticipantsByUUID.find(id) == mParticipantsByUUID.end())
    {
        return;
    }
    for (auto &connection : mWebRTCConnections)
    {
        connection->setUserVolume(id, volume);
    }
}

void LLWebRTCVoiceClient::sessionState::setUserMute(const LLUUID &id, bool mute)
{
    if (mParticipantsByUUID.find(id) == mParticipantsByUUID.end())
    {
        return;
    }
    for (auto &connection : mWebRTCConnections)
    {
        connection->setUserMute(id, mute);
    }
}
/*static*/
void LLWebRTCVoiceClient::sessionState::addSession(
    const std::string & channelID,
    LLWebRTCVoiceClient::sessionState::ptr_t& session)
{
    sSessions[channelID] = session;
}

LLWebRTCVoiceClient::sessionState::~sessionState()
{
    LL_DEBUGS("Voice") << "Destroying session CHANNEL=" << mChannelID << LL_ENDL;

    if (!mShuttingDown)
    {
        shutdownAllConnections();
    }
    mWebRTCConnections.clear();

    removeAllParticipants();
}

/*static*/
LLWebRTCVoiceClient::sessionState::ptr_t LLWebRTCVoiceClient::sessionState::matchSessionByChannelID(const std::string& channel_id)
{
    sessionStatePtr_t result;

    // *TODO: My kingdom for a lambda!
    std::map<std::string, ptr_t>::iterator it = sSessions.find(channel_id);
    if (it != sSessions.end())
    {
        result = (*it).second;
    }
    return result;
}

void LLWebRTCVoiceClient::sessionState::for_each(sessionFunc_t func)
{
    std::for_each(sSessions.begin(), sSessions.end(), boost::bind(for_eachPredicate, _1, func));
}

void LLWebRTCVoiceClient::sessionState::reapEmptySessions()
{
    std::map<std::string, ptr_t>::iterator iter;
    for (iter = sSessions.begin(); iter != sSessions.end();)
    {
        if (iter->second->isEmpty())
        {
            iter = sSessions.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

/*static*/
void LLWebRTCVoiceClient::sessionState::for_eachPredicate(const std::pair<std::string, LLWebRTCVoiceClient::sessionState::wptr_t> &a, sessionFunc_t func)
{
    ptr_t aLock(a.second.lock());

    if (aLock)
        func(aLock);
    else
    {
        LL_WARNS("Voice") << "Stale handle in session map!" << LL_ENDL;
    }
}

LLWebRTCVoiceClient::sessionStatePtr_t LLWebRTCVoiceClient::addSession(const std::string &channel_id, sessionState::ptr_t session)
{
    sessionStatePtr_t existingSession = sessionState::matchSessionByChannelID(channel_id);
    if (!existingSession)
    {
        // No existing session found.

        LL_DEBUGS("Voice") << "adding new session with channel: " << channel_id << LL_ENDL;
        session->setMuteMic(mMuteMic);
        session->setSpeakerVolume(mSpeakerVolume);

        sessionState::addSession(channel_id, session);
        return session;
    }
    else
    {
        // Found an existing session
        LL_DEBUGS("Voice") << "Attempting to add already-existing session " << channel_id << LL_ENDL;
        existingSession->revive();

        return existingSession;
    }
}

void LLWebRTCVoiceClient::sessionState::clearSessions()
{
    sSessions.clear();
}

LLWebRTCVoiceClient::sessionStatePtr_t LLWebRTCVoiceClient::findP2PSession(const LLUUID &agent_id)
{
    sessionStatePtr_t result = sessionState::matchSessionByChannelID(agent_id.asString());
    if (result && !result->isSpatial())
    {
        return result;
    }

    result.reset();
    return result;
}

void LLWebRTCVoiceClient::sessionState::shutdownAllConnections()
{
    mShuttingDown = true;
    for (auto &&connection : mWebRTCConnections)
    {
        connection->shutDown();
    }
}

// in case we drop into a session (spatial, etc.) right after
// telling the session to shut down, revive it so it reconnects.
void LLWebRTCVoiceClient::sessionState::revive()
{
    mShuttingDown = false;
}

//=========================================================================
// the following are methods to support the coroutine implementation of the
// voice connection and processing.  They should only be called in the context
// of a coroutine.
//
//

void LLWebRTCVoiceClient::sessionState::processSessionStates()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    auto iter = sSessions.begin();
    while (iter != sSessions.end())
    {
        if (!iter->second->processConnectionStates() && iter->second->mShuttingDown)
        {
            // if the connections associated with a session are gone,
            // and this session is shutting down, remove it.
            iter = sSessions.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}

// process the states on each connection associated with a session.
bool LLWebRTCVoiceClient::sessionState::processConnectionStates()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    std::list<connectionPtr_t>::iterator iter = mWebRTCConnections.begin();
    while (iter != mWebRTCConnections.end())
    {
        if (!iter->get()->connectionStateMachine())
        {
            // if the state machine returns false, the connection is shut down
            // so delete it.
            iter = mWebRTCConnections.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
    return !mWebRTCConnections.empty();
}

// processing of spatial voice connection states requires special handling.
// as neighboring regions need to be started up or shut down depending
// on our location.
bool LLWebRTCVoiceClient::estateSessionState::processConnectionStates()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    if (!mShuttingDown)
    {
        // Estate voice requires connection to neighboring regions.
        std::set<LLUUID> neighbor_ids = LLWebRTCVoiceClient::getInstance()->getNeighboringRegions();

        for (auto &connection : mWebRTCConnections)
        {
            std::shared_ptr<LLVoiceWebRTCSpatialConnection> spatialConnection =
                std::static_pointer_cast<LLVoiceWebRTCSpatialConnection>(connection);

            LLUUID regionID = spatialConnection.get()->getRegionID();

            if (neighbor_ids.find(regionID) == neighbor_ids.end())
            {
                // shut down connections to neighbors that are too far away.
                spatialConnection.get()->shutDown();
            }
            if (!spatialConnection.get()->isShuttingDown())
            {
                neighbor_ids.erase(regionID);
            }
        }

        // add new connections for new neighbors
        for (auto &neighbor : neighbor_ids)
        {
            connectionPtr_t connection(new LLVoiceWebRTCSpatialConnection(neighbor, INVALID_PARCEL_ID, mChannelID));

            mWebRTCConnections.push_back(connection);
            connection->setMuteMic(mMuted);
            connection->setSpeakerVolume(mSpeakerVolume);
        }
    }
    return LLWebRTCVoiceClient::sessionState::processConnectionStates();
}

// Various session state constructors.

LLWebRTCVoiceClient::estateSessionState::estateSessionState()
{
    mHangupOnLastLeave = false;
    mNotifyOnFirstJoin = false;
    mChannelID         = "Estate";
    LLUUID region_id   = gAgent.getRegion()->getRegionID();

    mWebRTCConnections.emplace_back(new LLVoiceWebRTCSpatialConnection(region_id, INVALID_PARCEL_ID, "Estate"));
}

LLWebRTCVoiceClient::parcelSessionState::parcelSessionState(const std::string &channelID, S32 parcel_local_id)
{
    mHangupOnLastLeave = false;
    mNotifyOnFirstJoin = false;
    LLUUID region_id   = gAgent.getRegion()->getRegionID();
    mChannelID         = channelID;
    mWebRTCConnections.emplace_back(new LLVoiceWebRTCSpatialConnection(region_id, parcel_local_id, channelID));
}

LLWebRTCVoiceClient::adhocSessionState::adhocSessionState(const std::string &channelID,
                                                          const std::string &credentials,
                                                          bool notify_on_first_join,
                                                          bool hangup_on_last_leave) :
    mCredentials(credentials)
{
    mHangupOnLastLeave = hangup_on_last_leave;
    mNotifyOnFirstJoin = notify_on_first_join;
    LLUUID region_id   = gAgent.getRegion()->getRegionID();
    mChannelID         = channelID;
    mWebRTCConnections.emplace_back(new LLVoiceWebRTCAdHocConnection(region_id, channelID, credentials));
}

void LLWebRTCVoiceClient::predShutdownSession(const LLWebRTCVoiceClient::sessionStatePtr_t& session)
{
    session->shutdownAllConnections();
}

void LLWebRTCVoiceClient::deleteSession(const sessionStatePtr_t &session)
{
    if (!session)
    {
        return;
    }

    // At this point, the session should be unhooked from all lists and all state should be consistent.
    session->shutdownAllConnections();
    // If this is the current audio session, clean up the pointer which will soon be dangling.
    bool deleteAudioSession = mSession == session;
    bool deleteNextAudioSession = mNextSession == session;
    if (deleteAudioSession)
    {
        mSession.reset();
    }

    // ditto for the next audio session
    if (deleteNextAudioSession)
    {
        mNextSession.reset();
    }
}


// Name resolution
void LLWebRTCVoiceClient::lookupName(const LLUUID &id)
{
    if (mAvatarNameCacheConnection.connected())
    {
        mAvatarNameCacheConnection.disconnect();
    }
    mAvatarNameCacheConnection = LLAvatarNameCache::get(id, boost::bind(&LLWebRTCVoiceClient::onAvatarNameCache, this, _1, _2));
}

void LLWebRTCVoiceClient::onAvatarNameCache(const LLUUID& agent_id,
                                           const LLAvatarName& av_name)
{
    mAvatarNameCacheConnection.disconnect();
    std::string display_name = av_name.getDisplayName();
    avatarNameResolved(agent_id, display_name);
}

void LLWebRTCVoiceClient::predAvatarNameResolution(const LLWebRTCVoiceClient::sessionStatePtr_t &session, LLUUID id, std::string name)
{
    participantStatePtr_t participant(session->findParticipantByID(id));
    if (participant)
    {
        // Found -- fill in the name
        participant->mDisplayName = name;
        // and post a "participants updated" message to listeners later.
        LLWebRTCVoiceClient::getInstance()->notifyParticipantObservers();
    }
}

void LLWebRTCVoiceClient::avatarNameResolved(const LLUUID &id, const std::string &name)
{
    sessionState::for_each(boost::bind(predAvatarNameResolution, _1, id, name));
}

// Leftover from vivox PTSN
std::string LLWebRTCVoiceClient::sipURIFromID(const LLUUID& id) const
{
    return id.asString();
}

LLSD LLWebRTCVoiceClient::getP2PChannelInfoTemplate(const LLUUID& id) const
{
    return LLSD();
}


/////////////////////////////
// LLVoiceWebRTCConnection
// These connections manage state transitions, negotiating webrtc connections,
// and other such things for a single connection to a Secondlife WebRTC server.
// Multiple of these connections may be active at once, in the case of
// cross-region voice, or when a new connection is being created before the old
// has a chance to shut down.
LLVoiceWebRTCConnection::LLVoiceWebRTCConnection(const LLUUID &regionID, const std::string &channelID) :
    mWebRTCAudioInterface(nullptr),
    mWebRTCDataInterface(nullptr),
    mVoiceConnectionState(VOICE_STATE_START_SESSION),
    mCurrentStatus(LLVoiceClientStatusObserver::STATUS_VOICE_ENABLED),
    mMuted(true),
    mShutDown(false),
    mIceCompleted(false),
    mSpeakerVolume(0.0),
    mOutstandingRequests(0),
    mChannelID(channelID),
    mRegionID(regionID),
    mPrimary(true),
    mRetryWaitPeriod(0)
{

    // retries wait a short period...randomize it so
    // all clients don't try to reconnect at once.
    mRetryWaitSecs = static_cast<F32>(rand()) / static_cast<F32>(RAND_MAX) + 0.5f;

    mWebRTCPeerConnectionInterface = llwebrtc::newPeerConnection();
    mWebRTCPeerConnectionInterface->setSignalingObserver(this);
    mMainQueue = LL::WorkQueue::getInstance("mainloop");
}

LLVoiceWebRTCConnection::~LLVoiceWebRTCConnection()
{
    if (LLWebRTCVoiceClient::isShuttingDown())
    {
        // peer connection and observers will be cleaned up
        // by llwebrtc::terminate() on shutdown.
        return;
    }
    mWebRTCPeerConnectionInterface->unsetSignalingObserver(this);
    llwebrtc::freePeerConnection(mWebRTCPeerConnectionInterface);
}


// ICE (Interactive Connectivity Establishment)
// When WebRTC tries to negotiate a connection to the Secondlife WebRTC Server,
// the negotiation will result in a few updates about the best path
// to which to connect.
// The Secondlife servers are configured for ICE trickling, where, after a session is partially
// negotiated, updates about the best connectivity paths may trickle in.  These need to be
// sent to the Secondlife WebRTC server via the simulator so that both sides have a clear
// view of the network environment.

// callback from llwebrtc
void LLVoiceWebRTCConnection::OnIceGatheringState(llwebrtc::LLWebRTCSignalingObserver::EIceGatheringState state)
{
    LL::WorkQueue::postMaybe(mMainQueue,
        [=, this] {
            LL_DEBUGS("Voice") << "Ice Gathering voice account. " << state << LL_ENDL;

            switch (state)
            {
                case llwebrtc::LLWebRTCSignalingObserver::EIceGatheringState::ICE_GATHERING_COMPLETE:
                {
                    mIceCompleted = true;
                    break;
                }
                case llwebrtc::LLWebRTCSignalingObserver::EIceGatheringState::ICE_GATHERING_NEW:
                {
                    mIceCompleted = false;
                }
                default:
                    break;
            }
        });
}

// callback from llwebrtc
void LLVoiceWebRTCConnection::OnIceCandidate(const llwebrtc::LLWebRTCIceCandidate& candidate)
{
    LL::WorkQueue::postMaybe(mMainQueue, [=, this] { mIceCandidates.push_back(candidate); });
}

void LLVoiceWebRTCConnection::processIceUpdates()
{
    mOutstandingRequests++;

    LLCoros::getInstance()->launch("LLVoiceWebRTCConnection::processIceUpdatesCoro",
                                   boost::bind(&LLVoiceWebRTCConnection::processIceUpdatesCoro, this->shared_from_this()));
}

// Ice candidates may be streamed in before or after the SDP offer is available (see below)
// This function determines whether candidates are available to send to the Secondlife WebRTC
// server via the simulator.  If so, and there are no more candidates, this code
// will make the cap call to the server sending up the ICE candidates.
void LLVoiceWebRTCConnection::processIceUpdatesCoro(connectionPtr_t connection)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    if (connection->mShutDown || LLWebRTCVoiceClient::isShuttingDown())
    {
        connection->mOutstandingRequests--;
        return;
    }

    bool iceCompleted = false;
    LLSD body;
    if (!connection->mIceCandidates.empty() || connection->mIceCompleted)
    {
        LLViewerRegion *regionp = LLWorld::instance().getRegionFromID(connection->mRegionID);
        if (!regionp || !regionp->capabilitiesReceived())
        {
            LL_DEBUGS("Voice") << "no capabilities for ice gathering; waiting " << LL_ENDL;
            connection->mOutstandingRequests--;
            return;
        }

        std::string url = regionp->getCapability("VoiceSignalingRequest");
        if (url.empty())
        {
            connection->mOutstandingRequests--;
            return;
        }

        LL_DEBUGS("Voice") << "region ready to complete voice signaling; url=" << url << LL_ENDL;
        if (!connection->mIceCandidates.empty())
        {
            LLSD candidates = LLSD::emptyArray();
            for (auto &ice_candidate : connection->mIceCandidates)
            {
                LLSD body_candidate;
                body_candidate["sdpMid"]        = ice_candidate.mSdpMid;
                body_candidate["sdpMLineIndex"] = ice_candidate.mMLineIndex;
                body_candidate["candidate"]     = ice_candidate.mCandidate;
                candidates.append(body_candidate);
            }
            body["candidates"] = candidates;
            connection->mIceCandidates.clear();
        }
        else if (connection->mIceCompleted)
        {
            LLSD body_candidate;
            body_candidate["completed"] = true;
            body["candidate"]           = body_candidate;
            iceCompleted                = connection->mIceCompleted;
            connection->mIceCompleted   = false;
        }

        body["viewer_session"]    = connection->mViewerSession;
        body["voice_server_type"] = WEBRTC_VOICE_SERVER_TYPE;

        LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(
            new LLCoreHttpUtil::HttpCoroutineAdapter("LLVoiceWebRTCAdHocConnection::processIceUpdatesCoro",
                                                        LLCore::HttpRequest::DEFAULT_POLICY_ID));
        LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
        LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

        httpOpts->setWantHeaders(true);

        LLSD result = httpAdapter->postAndSuspend(httpRequest, url, body, httpOpts);

        if (LLWebRTCVoiceClient::isShuttingDown())
        {
            connection->mOutstandingRequests--;
            return;
        }

        LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
        LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

        if (!status)
        {
            // couldn't trickle the candidates, so restart the session.
            connection->setVoiceConnectionState(VOICE_STATE_SESSION_RETRY);
        }
    }
    connection->mOutstandingRequests--;
}


// An 'Offer' comes in the form of a SDP (Session Description Protocol)
// which contains all sorts of info about the session, from network paths
// to the type of session (audio, video) to characteristics (the encoder type.)
// This SDP also serves as the 'ticket' to the server, security-wise.
// The Offer is retrieved from the WebRTC library on the client,
// and is passed to the simulator via a CAP, which then passes
// it on to the Secondlife WebRTC server.

//
// The LLWebRTCVoiceConnection object will not be deleted
// before the webrtc connection itself is shut down, so
// we shouldn't be getting this callback on a nonexistant
// this pointer.

// callback from llwebrtc
void LLVoiceWebRTCConnection::OnOfferAvailable(const std::string &sdp)
{
    LL::WorkQueue::postMaybe(mMainQueue,
        [=, this] {
            if (mShutDown)
            {
                return;
            }
            LL_DEBUGS("Voice") << "On Offer Available." << LL_ENDL;
            mChannelSDP = sdp;
            if (mVoiceConnectionState == VOICE_STATE_WAIT_FOR_SESSION_START)
            {
                mVoiceConnectionState = VOICE_STATE_REQUEST_CONNECTION;
            }
        });
}


//
// The LLWebRTCVoiceConnection object will not be deleted
// before the webrtc connection itself is shut down, so
// we shouldn't be getting this callback on a nonexistant
// this pointer.
// nor should audio_interface be invalid if the LLWebRTCVoiceConnection
// is shut down.

// callback from llwebrtc
void LLVoiceWebRTCConnection::OnAudioEstablished(llwebrtc::LLWebRTCAudioInterface* audio_interface)
{
    LL::WorkQueue::postMaybe(mMainQueue,
        [=, this] {
            if (mShutDown)
            {
                return;
            }
            LL_DEBUGS("Voice") << "On AudioEstablished." << LL_ENDL;
            mWebRTCAudioInterface = audio_interface;
            setVoiceConnectionState(VOICE_STATE_SESSION_ESTABLISHED);
        });
}


//
// The LLWebRTCVoiceConnection object will not be deleted
// before the webrtc connection itself is shut down, so
// we shouldn't be getting this callback on a nonexistant
// this pointer.

// callback from llwebrtc
void LLVoiceWebRTCConnection::OnRenegotiationNeeded()
{
    LL::WorkQueue::postMaybe(mMainQueue,
        [=, this] {
            LL_DEBUGS("Voice") << "Voice channel requires renegotiation." << LL_ENDL;
            if (!mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_SESSION_RETRY);
            }
            mCurrentStatus = LLVoiceClientStatusObserver::ERROR_UNKNOWN;
        });
}

// callback from llwebrtc
void LLVoiceWebRTCConnection::OnPeerConnectionClosed()
{
    LL::WorkQueue::postMaybe(mMainQueue,
        [=, this] {
            LL_DEBUGS("Voice") << "Peer connection has closed." << LL_ENDL;
            if (mVoiceConnectionState == VOICE_STATE_WAIT_FOR_CLOSE)
            {
                setVoiceConnectionState(VOICE_STATE_CLOSED);
                mOutstandingRequests--;
            }
            else if (LLWebRTCVoiceClient::isShuttingDown())
            {
                // disconnect was initialized by llwebrtc::terminate() instead of connectionStateMachine
                LL_INFOS("Voice") << "Peer connection has closed, but state is " << mVoiceConnectionState << LL_ENDL;
                setVoiceConnectionState(VOICE_STATE_CLOSED);
            }
        });
}

void LLVoiceWebRTCConnection::setMuteMic(bool muted)
{
    mMuted = muted;
    if (mWebRTCAudioInterface)
    {
        mWebRTCAudioInterface->setMute(muted);
    }
}

void LLVoiceWebRTCConnection::setSpeakerVolume(F32 volume)
{
    mSpeakerVolume = volume;
    if (mWebRTCAudioInterface)
    {
        mWebRTCAudioInterface->setReceiveVolume(volume);
    }
}

void LLVoiceWebRTCConnection::setUserVolume(const LLUUID& id, F32 volume)
{
    boost::json::object root      = { { "ug", { { id.asString(), (uint32_t)(volume * PEER_GAIN_CONVERSION_FACTOR) } } } };
    std::string json_data = boost::json::serialize(root);
    if (mWebRTCDataInterface)
    {
        mWebRTCDataInterface->sendData(json_data, false);
    }
}

void LLVoiceWebRTCConnection::setUserMute(const LLUUID& id, bool mute)
{
    boost::json::object root      = { { "m", { { id.asString(), mute } } } };
    std::string         json_data = boost::json::serialize(root);
    if (mWebRTCDataInterface)
    {
        mWebRTCDataInterface->sendData(json_data, false);
    }
}


// Send data to the Secondlife WebRTC server via the webrtc
// data channel.
void LLVoiceWebRTCConnection::sendData(const std::string &data)
{
    if (getVoiceConnectionState() == VOICE_STATE_SESSION_UP && mWebRTCDataInterface)
    {
        mWebRTCDataInterface->sendData(data, false);
    }
}

// Tell the simulator that we're shutting down a voice connection.
// The simulator will pass this on to the Secondlife WebRTC server.
void LLVoiceWebRTCConnection::breakVoiceConnectionCoro(connectionPtr_t connection)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    LL_DEBUGS("Voice") << "Disconnecting voice." << LL_ENDL;
    if (connection->mWebRTCDataInterface)
    {
        connection->mWebRTCDataInterface->unsetDataObserver(connection.get());
        connection->mWebRTCDataInterface = nullptr;
    }
    connection->mWebRTCAudioInterface   = nullptr;
    LLViewerRegion *regionp = LLWorld::instance().getRegionFromID(connection->mRegionID);
    if (!regionp || !regionp->capabilitiesReceived())
    {
        LL_DEBUGS("Voice") << "no capabilities for voice provisioning; waiting " << LL_ENDL;
        // fine, don't be polite and ask the janus server to break the connection.
        // just fall through and drop the connection.
        connection->setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
        connection->mOutstandingRequests--;
        return;
    }

    std::string url = regionp->getCapability("ProvisionVoiceAccountRequest");
    if (url.empty())
    {
        // and go on to drop the connection here, too.
        connection->setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
        connection->mOutstandingRequests--;
        return;
    }

    LL_DEBUGS("Voice") << "region ready for voice break; url=" << url << LL_ENDL;

    LLVoiceWebRTCStats::getInstance()->provisionAttemptStart();
    LLSD body;
    body["logout"]         = true;
    body["viewer_session"] = connection->mViewerSession;
    body["voice_server_type"] = WEBRTC_VOICE_SERVER_TYPE;

    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(
        new LLCoreHttpUtil::HttpCoroutineAdapter("LLVoiceWebRTCAdHocConnection::breakVoiceConnectionCoro",
                                                 LLCore::HttpRequest::DEFAULT_POLICY_ID));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setWantHeaders(true);

    // tell the server to shut down the connection as a courtesy.
    // shutdownConnection will drop the WebRTC connection which will
    // also shut things down.
    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, body, httpOpts);

    connection->mOutstandingRequests--;

    if (connection->getVoiceConnectionState() == VOICE_STATE_WAIT_FOR_EXIT ||
        !(connection->getVoiceConnectionState() & VOICE_STATE_SESSION_STOPPING))
    {
        // drop the connection if we either somehow got set back to a running/starting state
        // or we completed the call in the wait-for-exit state
        connection->setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
    }
}

// Tell the simulator to tell the Secondlife WebRTC server that we want a voice
// connection. The SDP is sent up as part of this, and the simulator will respond
// with an 'answer' which is in the form of another SDP.  The webrtc library
// will use the offer and answer to negotiate the session.
void LLVoiceWebRTCSpatialConnection::requestVoiceConnection()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    LLViewerRegion *regionp = LLWorld::instance().getRegionFromID(mRegionID);

    LL_DEBUGS("Voice") << "Requesting voice connection." << LL_ENDL;
    if (!regionp || !regionp->capabilitiesReceived())
    {
        LL_DEBUGS("Voice") << "no capabilities for voice provisioning; waiting " << LL_ENDL;

        // try again.
        setVoiceConnectionState(VOICE_STATE_REQUEST_CONNECTION);
        mOutstandingRequests--;
        return;
    }

    std::string url = regionp->getCapability("ProvisionVoiceAccountRequest");
    if (url.empty())
    {
        setVoiceConnectionState(VOICE_STATE_SESSION_RETRY);
        mOutstandingRequests--;
        return;
    }

    LL_DEBUGS("Voice") << "region ready for voice provisioning; url=" << url << LL_ENDL;

    LLVoiceWebRTCStats::getInstance()->provisionAttemptStart();
    LLSD body;
    LLSD jsep;
    jsep["type"] = "offer";
    jsep["sdp"] = mChannelSDP;
    body["jsep"] = jsep;
    if (mParcelLocalID != INVALID_PARCEL_ID)
    {
        body["parcel_local_id"] = mParcelLocalID;
    }
    body["channel_type"]      = "local";
    body["voice_server_type"] = WEBRTC_VOICE_SERVER_TYPE;
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(
        new LLCoreHttpUtil::HttpCoroutineAdapter("LLVoiceWebRTCAdHocConnection::requestVoiceConnection",
                                                 LLCore::HttpRequest::DEFAULT_POLICY_ID));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setWantHeaders(true);
    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, body, httpOpts);

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (status)
    {
        OnVoiceConnectionRequestSuccess(result);
    }
    else
    {
        switch (status.getType())
        {
            case HTTP_CONFLICT:
                mCurrentStatus = LLVoiceClientStatusObserver::ERROR_CHANNEL_FULL;
                break;
            case HTTP_UNAUTHORIZED:
                mCurrentStatus = LLVoiceClientStatusObserver::ERROR_CHANNEL_LOCKED;
                break;
            default:
                mCurrentStatus = LLVoiceClientStatusObserver::ERROR_UNKNOWN;
                break;
        }
        setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
    }
    mOutstandingRequests--;
}

void LLVoiceWebRTCConnection::OnVoiceConnectionRequestSuccess(const LLSD &result)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    if (LLWebRTCVoiceClient::isShuttingDown())
    {
        return;
    }
    LLVoiceWebRTCStats::getInstance()->provisionAttemptEnd(true);

    if (result.has("viewer_session") &&
        result.has("jsep") &&
        result["jsep"].has("type") &&
        result["jsep"]["type"] == "answer" &&
        result["jsep"].has("sdp"))
    {
        mRemoteChannelSDP = result["jsep"]["sdp"].asString();
        mViewerSession    = result["viewer_session"];
    }
    else
    {
        LL_WARNS("Voice") << "Invalid voice provision request result:" << result << LL_ENDL;
        setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
        return;
    }

    LL_DEBUGS("Voice") << "ProvisionVoiceAccountRequest response"
                       << " channel sdp " << mRemoteChannelSDP << LL_ENDL;
    mWebRTCPeerConnectionInterface->AnswerAvailable(mRemoteChannelSDP);
}

static llwebrtc::LLWebRTCPeerConnectionInterface::InitOptions getConnectionOptions()
{
    llwebrtc::LLWebRTCPeerConnectionInterface::InitOptions options;
    llwebrtc::LLWebRTCPeerConnectionInterface::InitOptions::IceServers servers;

    // TODO: Pull these from login
    std::string grid = LLGridManager::getInstance()->getGridLoginID();
    std::transform(grid.begin(), grid.end(), grid.begin(), [](unsigned char c){ return std::tolower(c); });
    int num_servers = 2;
    if (grid == "agni")
    {
        num_servers = 3;
    }
    for (int i=1; i <= num_servers; i++)
    {
        servers.mUrls.push_back(llformat("stun:stun%d.%s.secondlife.io:3478", i, grid.c_str()));
    }
    options.mServers.push_back(servers);
    return options;
}


// Primary state machine for negotiating a single voice connection to the
// Secondlife WebRTC server.
bool LLVoiceWebRTCConnection::connectionStateMachine()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    if (!mShutDown)
    {
        processIceUpdates();
    }

    switch (getVoiceConnectionState())
    {
        case VOICE_STATE_START_SESSION:
        {
            LL_PROFILE_ZONE_NAMED_CATEGORY_VOICE("VOICE_STATE_START_SESSION")
            if (mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
                break;
            }
            mIceCompleted = false;
            setVoiceConnectionState(VOICE_STATE_WAIT_FOR_SESSION_START);

            // tell the webrtc library that we want a connection.  The library will
            // respond with an offer on a separate thread, which will cause
            // the session state to change.
            if (!mWebRTCPeerConnectionInterface->initializeConnection(getConnectionOptions()))
            {
                setVoiceConnectionState(VOICE_STATE_SESSION_RETRY);
            }
            break;
        }

        case VOICE_STATE_WAIT_FOR_SESSION_START:
        {
            if (mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
            }
            break;
        }

        case VOICE_STATE_REQUEST_CONNECTION:
            if (mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
                break;
            }
            // Ask the sim to ask the Secondlife WebRTC server for a connection to
            // a given voice channel.  On completion, we'll move on to the
            // VOICE_STATE_SESSION_ESTABLISHED via a callback on a webrtc thread.
            setVoiceConnectionState(VOICE_STATE_CONNECTION_WAIT);
            mOutstandingRequests++;
            LLCoros::getInstance()->launch("LLVoiceWebRTCConnection::requestVoiceConnectionCoro",
                                           boost::bind(&LLVoiceWebRTCConnection::requestVoiceConnectionCoro, this->shared_from_this()));
            break;

        case VOICE_STATE_CONNECTION_WAIT:
            if (mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_DISCONNECT);
            }
            break;

        case VOICE_STATE_SESSION_ESTABLISHED:
        {
            if (mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_DISCONNECT);
                break;
            }
            // update the peer connection with the various characteristics of
            // this connection.
            mWebRTCAudioInterface->setMute(mMuted);
            mWebRTCAudioInterface->setReceiveVolume(mSpeakerVolume);
            LLWebRTCVoiceClient::getInstance()->OnConnectionEstablished(mChannelID, mRegionID);
            setVoiceConnectionState(VOICE_STATE_WAIT_FOR_DATA_CHANNEL);
            break;
        }

        case VOICE_STATE_WAIT_FOR_DATA_CHANNEL:
        {
            if (mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_DISCONNECT);
                break;
            }
            if (mWebRTCDataInterface) // the interface will be set when the session is negotiated.
            {
                sendJoin();  // tell the Secondlife WebRTC server that we're here via the data channel.
                setVoiceConnectionState(VOICE_STATE_SESSION_UP);
                if (isSpatial())
                {
                    LLWebRTCVoiceClient::getInstance()->updatePosition();
                    LLWebRTCVoiceClient::getInstance()->sendPositionUpdate(true);
                }
            }
            break;
        }

        case VOICE_STATE_SESSION_UP:
        {
            mRetryWaitPeriod = 0;
            mRetryWaitSecs   = (F32)((F32)rand() / (F32)RAND_MAX) + 0.5f;

            // we'll stay here as long as the session remains up.
            if (mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_DISCONNECT);
            }
            else
            {
                if (isSpatial() && gAgent.getRegion())
                {
                    bool primary = (mRegionID == gAgent.getRegion()->getRegionID());
                    if (primary != mPrimary)
                    {
                        mPrimary = primary;
                        sendJoin();
                    }
                }
            }
            break;
        }

        case VOICE_STATE_SESSION_RETRY:
            // only retry ever 'n' seconds
            if (mRetryWaitPeriod++ * UPDATE_THROTTLE_SECONDS > mRetryWaitSecs)
            {
                // something went wrong, so notify that the connection has failed.
                LLWebRTCVoiceClient::getInstance()->OnConnectionFailure(mChannelID, mRegionID, mCurrentStatus);
                setVoiceConnectionState(VOICE_STATE_DISCONNECT);
                mRetryWaitPeriod = 0;
                if (mRetryWaitSecs < MAX_RETRY_WAIT_SECONDS)
                {
                    // back off the retry period, and do it by a small random
                    // bit so all clients don't reconnect at once.
                    mRetryWaitSecs += static_cast<F32>(rand()) / static_cast<F32>(RAND_MAX) + 0.5f;
                    mRetryWaitPeriod = 0;
                }
            }
            break;

        case VOICE_STATE_DISCONNECT:
            if (!LLWebRTCVoiceClient::isShuttingDown())
            {
                mOutstandingRequests++;
                setVoiceConnectionState(VOICE_STATE_WAIT_FOR_EXIT);
                LLCoros::getInstance()->launch("LLVoiceWebRTCConnection::breakVoiceConnectionCoro",
                                               boost::bind(&LLVoiceWebRTCConnection::breakVoiceConnectionCoro, this->shared_from_this()));
            }
            else
            {
                // llwebrtc::terminate() is already shuting down the connection.
                setVoiceConnectionState(VOICE_STATE_WAIT_FOR_CLOSE);
            }
            break;

        case VOICE_STATE_WAIT_FOR_EXIT:
            break;

        case VOICE_STATE_SESSION_EXIT:
        setVoiceConnectionState(VOICE_STATE_WAIT_FOR_CLOSE);
        mOutstandingRequests++;
        if (!LLWebRTCVoiceClient::isShuttingDown())
        {
            mWebRTCPeerConnectionInterface->shutdownConnection();
        }
        // else was already posted by llwebrtc::terminate().
        break;

        case VOICE_STATE_WAIT_FOR_CLOSE:
            break;

        case VOICE_STATE_CLOSED:
            if (!mShutDown)
            {
                mVoiceConnectionState = VOICE_STATE_START_SESSION;
            }
            else
            {
                // if we still have outstanding http or webrtc calls, wait for them to
                // complete so we don't delete objects while they still may be used.
                if (mOutstandingRequests <= 0)
                {
                    LLWebRTCVoiceClient::getInstance()->OnConnectionShutDown(mChannelID, mRegionID);
                    return false;
                }
            }
            break;

        default:
        {
            LL_WARNS("Voice") << "Unknown voice control state " << getVoiceConnectionState() << LL_ENDL;
            return false;
        }
    }
    return true;
}

// Data has been received on the webrtc data channel
// incoming data will be a json structure (if it's not binary.)  We may pack
// binary for size reasons.  Most of the keys in the json objects are
// single or double characters for size reasons.
// The primary element is:
// An object where each key is an agent id.  (in the future, we may allow
// integer indices into an agentid list, populated on join commands.  For size.
// Each key will point to a json object with keys identifying what's updated.
// 'p'  - audio source power (level/volume) (int8 as int)
// 'j'  - object of join data (currently only a boolean 'p' marking a primary participant)
// 'l'  - boolean, always true if exists.
// 'v'  - boolean - voice activity has been detected.

// llwebrtc callback
void LLVoiceWebRTCConnection::OnDataReceived(const std::string& data, bool binary)
{
    LL::WorkQueue::postMaybe(mMainQueue, [=, this] { LLVoiceWebRTCConnection::OnDataReceivedImpl(data, binary); });
}

//
// The LLWebRTCVoiceConnection object will not be deleted
// before the webrtc connection itself is shut down, so
// we shouldn't be getting this callback on a nonexistant
// this pointer.
void LLVoiceWebRTCConnection::OnDataReceivedImpl(const std::string &data, bool binary)
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    if (mShutDown)
    {
        return;
    }

    if (binary)
    {
        LL_WARNS("Voice") << "Binary data received from data channel." << LL_ENDL;
        return;
    }

    boost::system::error_code ec;
    boost::json::value voice_data_parsed = boost::json::parse(data, ec);
    if (!ec)  // don't collect comments
    {
        if (!voice_data_parsed.is_object())
        {
            LL_WARNS("Voice") << "Expected object from data channel:" << data << LL_ENDL;
            return;
        }
        boost::json::object voice_data = voice_data_parsed.as_object();
        bool new_participant = false;
        boost::json::object mute;
        boost::json::object user_gain;
        for (auto &participant_elem : voice_data)
        {
            boost::json::string participant_id(participant_elem.key());
            LLUUID agent_id(participant_id.c_str());
            if (agent_id.isNull())
            {
               // probably a test client.
               continue;
            }

            if (!participant_elem.value().is_object())
            {
                continue;
            }

            boost::json::object participant_obj = participant_elem.value().as_object();

            LLWebRTCVoiceClient::participantStatePtr_t participant =
                LLWebRTCVoiceClient::getInstance()->findParticipantByID(mChannelID, agent_id);
            bool joined  = false;
            bool primary = false;  // we ignore any 'joins' reported about participants
                                   // that come from voice servers that aren't their primary
                                   // voice server.  This will happen with cross-region voice
                                   // where a participant on a neighboring region may be
                                   // connected to multiple servers.  We don't want to
                                   // add new identical participants from all of those servers.
            if (participant_obj.contains("j") &&
                participant_obj["j"].is_object())
            {
                // a new participant has announced that they're joining.
                joined  = true;
                if (participant_elem.value().as_object()["j"].as_object().contains("p") &&
                    participant_elem.value().as_object()["j"].as_object()["p"].is_bool())
                {
                    primary = participant_elem.value().as_object()["j"].as_object()["p"].as_bool();
                }

                // track incoming participants that are muted so we can mute their connections (or set their volume)
                bool isMuted = LLMuteList::getInstance()->isMuted(agent_id, LLMute::flagVoiceChat);
                if (isMuted)
                {
                    mute[participant_id] = true;
                }
                F32 volume;
                if(LLSpeakerVolumeStorage::getInstance()->getSpeakerVolume(agent_id, volume))
                {
                    user_gain[participant_id] = (uint32_t)(volume * 200);
                }
            }

            new_participant |= joined;
            if (!participant && joined && (primary || !isSpatial()))
            {
                participant = LLWebRTCVoiceClient::getInstance()->addParticipantByID(mChannelID, agent_id, mRegionID);
            }

            if (participant)
            {
                if (participant_obj.contains("l") && participant_obj["l"].is_bool() && participant_obj["l"].as_bool())
                {
                    // an existing participant is leaving.
                    if (agent_id != gAgentID)
                    {
                        LLWebRTCVoiceClient::getInstance()->removeParticipantByID(mChannelID, agent_id, mRegionID);
                    }
                }
                else
                {
                    // we got a 'power' update.
                    if (participant_obj.contains("p") && participant_obj["p"].is_number())
                    {
                        // server sends up power as an integer which is level * 128 to save
                        // character count.
                        participant->mLevel = (F32)participant_obj["p"].as_int64()/128.0f;
                    }

                    if (participant_obj.contains("v") && participant_obj["v"].is_bool())
                    {
                        participant->mIsSpeaking = participant_obj["v"].as_bool();
                    }

                    if (participant_obj.contains("m") && participant_obj["m"].is_bool())
                    {
                        participant->mIsModeratorMuted = participant_obj["m"].as_bool();
                    }
                }
            }
        }

        // tell the simulator to set the mute and volume data for this
        // participant, if there are any updates.
        boost::json::object root;
        if (mute.size() > 0)
        {
            root["m"] = mute;
        }
        if (user_gain.size() > 0)
        {
            root["ug"] = user_gain;
        }
        if (root.size() > 0)
        {
            std::string json_data = boost::json::serialize(root);
            mWebRTCDataInterface->sendData(json_data, false);
        }
    }
}

//
// The LLWebRTCVoiceConnection object will not be deleted
// before the webrtc connection itself is shut down, so
// we shouldn't be getting this callback on a nonexistant
// this pointer.
// nor should data_interface be invalid if the LLWebRTCVoiceConnection
// is shut down.

// llwebrtc callback
void LLVoiceWebRTCConnection::OnDataChannelReady(llwebrtc::LLWebRTCDataInterface *data_interface)
{
    LL::WorkQueue::postMaybe(mMainQueue,
        [=, this] {
            if (mShutDown)
            {
                return;
            }

            // OnDataChannelReady may be called multiple times in a single connection attempt
            // so don't double-set the observer.
            if (!mWebRTCDataInterface && data_interface)
            {
                mWebRTCDataInterface = data_interface;
                mWebRTCDataInterface->setDataObserver(this);
            }
        });
}

// tell the Secondlife WebRTC server that
// we're joining and whether we're
// joining a server associated with the
// the region we currently occupy or not (primary)
// The WebRTC voice server will pass this info
// to peers.
void LLVoiceWebRTCConnection::sendJoin()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;


    boost::json::object root;
    boost::json::object join_obj;
    if (mPrimary)
    {
        join_obj["p"] = true;
    }
    root["j"]             = join_obj;
    std::string json_data = boost::json::serialize(root);
    mWebRTCDataInterface->sendData(json_data, false);
}

/////////////////////////////
// WebRTC Spatial Connection

LLVoiceWebRTCSpatialConnection::LLVoiceWebRTCSpatialConnection(const LLUUID &regionID,
                                                               S32 parcelLocalID,
                                                               const std::string &channelID) :
    LLVoiceWebRTCConnection(regionID, channelID),
    mParcelLocalID(parcelLocalID)
{
    if (gAgent.getRegion())
    {
        mPrimary = (regionID == gAgent.getRegion()->getRegionID());
    }
}

LLVoiceWebRTCSpatialConnection::~LLVoiceWebRTCSpatialConnection()
{
}

void LLVoiceWebRTCSpatialConnection::setMuteMic(bool muted)
{
    if (mMuted != muted)
    {
        mMuted = muted;
        if (mWebRTCAudioInterface)
        {
            LLViewerRegion *regionp = gAgent.getRegion();
            if (regionp && mRegionID == regionp->getRegionID())
            {
                mWebRTCAudioInterface->setMute(muted);
            }
            else
            {
                // Always mute this agent with respect to neighboring regions.
                // Peers don't want to hear this agent from multiple regions
                // as that'll echo.
                mWebRTCAudioInterface->setMute(true);
            }
        }
    }
}

/////////////////////////////
// WebRTC Spatial Connection

LLVoiceWebRTCAdHocConnection::LLVoiceWebRTCAdHocConnection(const LLUUID &regionID,
                                                           const std::string& channelID,
                                                           const std::string& credentials) :
    LLVoiceWebRTCConnection(regionID, channelID),
    mCredentials(credentials)
{
}

LLVoiceWebRTCAdHocConnection::~LLVoiceWebRTCAdHocConnection()
{
}

// Add-hoc connections require a different channel type
// as they go to a different set of Secondlife WebRTC servers.
// They also require credentials for the given channels.
// So, we have a separate requestVoiceConnection call.
void LLVoiceWebRTCAdHocConnection::requestVoiceConnection()
{
    LL_PROFILE_ZONE_SCOPED_CATEGORY_VOICE;

    LLViewerRegion *regionp = LLWorld::instance().getRegionFromID(mRegionID);

    LL_DEBUGS("Voice") << "Requesting voice connection." << LL_ENDL;
    if (!regionp || !regionp->capabilitiesReceived())
    {
        LL_DEBUGS("Voice") << "no capabilities for voice provisioning; retrying " << LL_ENDL;
        // try again.
        setVoiceConnectionState(VOICE_STATE_REQUEST_CONNECTION);
        mOutstandingRequests--;
        return;
    }

    std::string url = regionp->getCapability("ProvisionVoiceAccountRequest");
    if (url.empty())
    {
        setVoiceConnectionState(VOICE_STATE_SESSION_RETRY);
        mOutstandingRequests--;
        return;
    }

    LLVoiceWebRTCStats::getInstance()->provisionAttemptStart();
    LLSD body;
    LLSD jsep;
    jsep["type"] = "offer";
    {
        jsep["sdp"] = mChannelSDP;
    }
    body["jsep"] = jsep;
    body["credentials"] = mCredentials;
    body["channel"] = mChannelID;
    body["channel_type"] = "multiagent";
    body["voice_server_type"] = WEBRTC_VOICE_SERVER_TYPE;

    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(
        new LLCoreHttpUtil::HttpCoroutineAdapter("LLVoiceWebRTCAdHocConnection::requestVoiceConnection",
                                                 LLCore::HttpRequest::DEFAULT_POLICY_ID));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts(new LLCore::HttpOptions);

    httpOpts->setWantHeaders(true);

    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, body, httpOpts);

    LLSD               httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status      = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (!status)
    {
        switch (status.getType())
        {
            case HTTP_CONFLICT:
                mCurrentStatus = LLVoiceClientStatusObserver::ERROR_CHANNEL_FULL;
                break;
            case HTTP_UNAUTHORIZED:
                mCurrentStatus = LLVoiceClientStatusObserver::ERROR_CHANNEL_LOCKED;
                break;
            default:
                mCurrentStatus = LLVoiceClientStatusObserver::ERROR_UNKNOWN;
                break;
        }
        setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
    }
    else
    {
        OnVoiceConnectionRequestSuccess(result);
    }
    mOutstandingRequests--;
}
