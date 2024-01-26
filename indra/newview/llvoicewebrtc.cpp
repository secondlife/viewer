 /** 
 * @file LLWebRTCVoiceClient.cpp
 * @brief Implementation of LLWebRTCVoiceClient class which is the interface to the voice client process.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2023, Linden Research, Inc.
 * ne
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
#include "llviewernetwork.h"		// for gGridChoice
#include "llbase64.h"
#include "llviewercontrol.h"
#include "llappviewer.h"	// for gDisconnected, gDisableVoice
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

#include "json/reader.h"
#include "json/writer.h"

#define USE_SESSION_GROUPS 0
#define VX_NULL_POSITION -2147483648.0 /*The Silence*/

extern LLMenuBarGL* gMenuBarView;
extern void handle_voice_morphing_subscribe();

namespace {

    const F32 MAX_AUDIO_DIST      = 50.0f;
    const F32 VOLUME_SCALE_WEBRTC = 0.01f;
    const F32 LEVEL_SCALE_WEBRTC  = 0.008f;

    const F32 SPEAKING_TIMEOUT = 1.f;
    const F32 SPEAKING_AUDIO_LEVEL = 0.40;

    static const std::string VOICE_SERVER_TYPE = "WebRTC";

    // Don't send positional updates more frequently than this:
    const F32 UPDATE_THROTTLE_SECONDS = 0.1f;

    // Timeout for connection to WebRTC 
    const F32 CONNECT_ATTEMPT_TIMEOUT = 300.0f;
    const F32 CONNECT_DNS_TIMEOUT = 5.0f;

    const F32 LOGOUT_ATTEMPT_TIMEOUT = 5.0f;
    const S32 PROVISION_WAIT_TIMEOUT_SEC = 5;
    
    // Cosine of a "trivially" small angle
    const F32 FOUR_DEGREES = 4.0f * (F_PI / 180.0f);
    const F32 MINUSCULE_ANGLE_COS = (F32) cos(0.5f * FOUR_DEGREES);

    // Defines the maximum number of times(in a row) "stateJoiningSession" case for spatial channel is reached in stateMachine()
    // which is treated as normal. The is the number of frames to wait for a channel join before giving up.  This was changed 
    // from the original count of 50 for two reason.  Modern PCs have higher frame rates and sometimes the SLVoice process 
    // backs up processing join requests.  There is a log statement that records when channel joins take longer than 100 frames.
    const int MAX_NORMAL_JOINING_SPATIAL_NUM = 1500;

    // How often to check for expired voice fonts in seconds
    const F32 VOICE_FONT_EXPIRY_INTERVAL = 10.f;
    // Time of day at which WebRTC expires voice font subscriptions.
    // Used to replace the time portion of received expiry timestamps.
    static const std::string VOICE_FONT_EXPIRY_TIME = "T05:00:00Z";

    // Maximum length of capture buffer recordings in seconds.
    const F32 CAPTURE_BUFFER_MAX_TIME = 10.f;
}  // namespace
float LLWebRTCVoiceClient::getAudioLevel()
{
    if (mIsInTuningMode)
    {
        return (1.0 - mWebRTCDeviceInterface->getTuningAudioLevel() * LEVEL_SCALE_WEBRTC) * mTuningMicGain / 2.1;
    }
    else
    {
        return (1.0 - mWebRTCDeviceInterface->getPeerAudioLevel() * LEVEL_SCALE_WEBRTC) * mMicGain / 2.1;
    }
}

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
bool LLWebRTCVoiceClient::sConnected = false;
LLPumpIO *LLWebRTCVoiceClient::sPump = nullptr;

LLWebRTCVoiceClient::LLWebRTCVoiceClient() :
    mSessionTerminateRequested(false),
    mRelogRequested(false),
    mSpatialJoiningNum(0),

    mTuningMode(false),
    mTuningMicGain(0.0),
    mTuningSpeakerVolume(50),  // Set to 50 so the user can hear himself when he sets his mic volume
    mTuningSpeakerVolumeDirty(true),
    mDevicesListUpdated(false),

    mAreaVoiceDisabled(false),
    mSession(),  // TBD - should be NULL
    mSessionChanged(false),
    mNextSession(),

    mCurrentParcelLocalID(0),

    mBuddyListMapPopulated(false),
    mBlockRulesListReceived(false),
    mAutoAcceptRulesListReceived(false),

    mSpatialCoordsDirty(false),
    mIsInitialized(false),

    mMuteMic(false),
    mMuteMicDirty(false),
    mFriendsListDirty(true),

    mEarLocation(0),
    mSpeakerVolumeDirty(true),
    mMicGain(0.0),

    mVoiceEnabled(false),

    mLipSyncEnabled(false),

    mShutdownComplete(true),
    mPlayRequestCount(0),

    mAvatarNameCacheConnection(),
    mIsInTuningMode(false),
    mIsJoiningSession(false),
    mIsWaitingForFonts(false),
    mIsLoggingIn(false),
    mIsProcessingChannels(false),
    mIsCoroutineActive(false),
    mWebRTCPump("WebRTCClientPump"),
    mWebRTCDeviceInterface(nullptr)
{
    sShuttingDown = false;
    sConnected = false;
    sPump = nullptr;

	mSpeakerVolume = 0.0;

	mVoiceVersion.serverVersion = "";
	mVoiceVersion.serverType = VOICE_SERVER_TYPE;
	
	//  gMuteListp isn't set up at this point, so we defer this until later.
//	gMuteListp->addObserver(&mutelist_listener);

	
#if LL_DARWIN || LL_LINUX
		// HACK: THIS DOES NOT BELONG HERE
		// When the WebRTC daemon dies, the next write attempt on our socket generates a SIGPIPE, which kills us.
		// This should cause us to ignore SIGPIPE and handle the error through proper channels.
		// This should really be set up elsewhere.  Where should it go?
		signal(SIGPIPE, SIG_IGN);
		
		// Since we're now launching the gateway with fork/exec instead of system(), we need to deal with zombie processes.
		// Ignoring SIGCHLD should prevent zombies from being created.  Alternately, we could use wait(), but I'd rather not do that.
		signal(SIGCHLD, SIG_IGN);
#endif


	gIdleCallbacks.addFunction(idle, this);
}

//---------------------------------------------------

LLWebRTCVoiceClient::~LLWebRTCVoiceClient()
{
    if (mAvatarNameCacheConnection.connected())
    {
        mAvatarNameCacheConnection.disconnect();
    }
    sShuttingDown = true;
}

//---------------------------------------------------

void LLWebRTCVoiceClient::init(LLPumpIO* pump)
{
    // constructor will set up LLVoiceClient::getInstance()
    sPump = pump;

    //     LLCoros::instance().launch("LLWebRTCVoiceClient::voiceConnectionCoro",
    //         boost::bind(&LLWebRTCVoiceClient::voiceConnectionCoro, LLWebRTCVoiceClient::getInstance()));
    llwebrtc::init();

    mWebRTCDeviceInterface = llwebrtc::getDeviceInterface();
    mWebRTCDeviceInterface->setDevicesObserver(this);
}

void LLWebRTCVoiceClient::terminate()
{
    if (sShuttingDown)
    {
        return;
    }

    mRelogRequested = false;
    mVoiceEnabled = false;
    llwebrtc::terminate();

    sShuttingDown = true;
    sPump = NULL;
}

//---------------------------------------------------

void LLWebRTCVoiceClient::cleanUp()
{
    mNextSession.reset();
    mSession.reset();
    mNeighboringRegions.clear();
    sessionState::for_each(boost::bind(predShutdownSession, _1));
    LL_DEBUGS("Voice") << "exiting" << LL_ENDL;
}

//---------------------------------------------------

const LLVoiceVersionInfo& LLWebRTCVoiceClient::getVersion()
{
    return mVoiceVersion;
}

//---------------------------------------------------

void LLWebRTCVoiceClient::updateSettings()
{
    setVoiceEnabled(voiceEnabled());
    setEarLocation(gSavedSettings.getS32("VoiceEarLocation"));

    std::string inputDevice = gSavedSettings.getString("VoiceInputAudioDevice");
    setCaptureDevice(inputDevice);
    std::string outputDevice = gSavedSettings.getString("VoiceOutputAudioDevice");
    setRenderDevice(outputDevice);
    F32 mic_level = gSavedSettings.getF32("AudioLevelMic");
    setMicGain(mic_level);
    setLipSyncEnabled(gSavedSettings.getBOOL("LipSyncEnabled"));
}


/////////////////////////////
// session control messages

void LLWebRTCVoiceClient::OnConnectionFailure(const std::string& channelID, const LLUUID& regionID)
{
    if (gAgent.getRegion()->getRegionID() == regionID)
    {
        if (mNextSession && mNextSession->mChannelID == channelID)
        {
            LLWebRTCVoiceClient::getInstance()->notifyStatusObservers(LLVoiceClientStatusObserver::ERROR_UNKNOWN);
        }
        else if (mSession && mSession->mChannelID == channelID)
        {
            LLWebRTCVoiceClient::getInstance()->notifyStatusObservers(LLVoiceClientStatusObserver::ERROR_UNKNOWN);
        }
    }
}

void LLWebRTCVoiceClient::OnConnectionEstablished(const std::string& channelID, const LLUUID& regionID)
{
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
        
        if (mSession && mSession->mChannelID == channelID)
        {
            LLWebRTCVoiceClient::getInstance()->notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINED);
            LLWebRTCVoiceClient::getInstance()->notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LOGGED_IN);
        }
    }
}

void LLWebRTCVoiceClient::OnConnectionShutDown(const std::string &channelID, const LLUUID &regionID)
{
    if (gAgent.getRegion()->getRegionID() == regionID)
    {
        if (mSession && mSession->mChannelID == channelID)
        {
            LLWebRTCVoiceClient::getInstance()->notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL);
        }
    }
}

void LLWebRTCVoiceClient::idle(void* user_data)
{
}

//=========================================================================
// the following are methods to support the coroutine implementation of the 
// voice connection and processing.  They should only be called in the context 
// of a coroutine.
// 
// 

void LLWebRTCVoiceClient::sessionState::processSessionStates()
{

    auto iter = mSessions.begin();
    while (iter != mSessions.end())
    {
        if (!iter->second->processConnectionStates())
        {
            iter = mSessions.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}

bool LLWebRTCVoiceClient::sessionState::processConnectionStates()
{
    std::list<connectionPtr_t>::iterator iter = mWebRTCConnections.begin();
    while (iter != mWebRTCConnections.end())
    {
        if (!iter->get()->connectionStateMachine())
        {
            iter = mWebRTCConnections.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
    return !mWebRTCConnections.empty();
}


//////////////////////////
// LLWebRTCVoiceClient::estateSessionState

LLWebRTCVoiceClient::estateSessionState::estateSessionState()
{
    mChannelID       = "Estate";
    LLUUID region_id = gAgent.getRegion()->getRegionID();

    mWebRTCConnections.emplace_back(new LLVoiceWebRTCSpatialConnection(region_id, INVALID_PARCEL_ID, "Estate"));
}

LLWebRTCVoiceClient::parcelSessionState::parcelSessionState(const std::string &channelID, S32 parcel_local_id)
{
    LLUUID region_id = gAgent.getRegion()->getRegionID();
    mChannelID       = channelID;
    mWebRTCConnections.emplace_back(new LLVoiceWebRTCSpatialConnection(region_id, parcel_local_id, channelID));
}

LLWebRTCVoiceClient::adhocSessionState::adhocSessionState(const std::string &channelID, const std::string& credentials) :
    mCredentials(credentials)
{
    LLUUID region_id = gAgent.getRegion()->getRegionID();
    mChannelID = channelID;
    mWebRTCConnections.emplace_back(new LLVoiceWebRTCAdHocConnection(region_id, channelID, credentials));
}

bool LLWebRTCVoiceClient::estateSessionState::processConnectionStates()
{

    // Estate voice requires connection to neighboring regions.
    std::set<LLUUID> neighbor_ids = LLWebRTCVoiceClient::getInstance()->getNeighboringRegions();

    for (auto& connection : mWebRTCConnections)
    {
        boost::shared_ptr<LLVoiceWebRTCSpatialConnection> spatialConnection =
            boost::static_pointer_cast<LLVoiceWebRTCSpatialConnection>(connection);

        LLUUID regionID = spatialConnection.get()->getRegionID();
     
        if (neighbor_ids.find(regionID) == neighbor_ids.end())
        {
            // shut down connections to neighbors that are too far away.
            spatialConnection.get()->shutDown();
        }
        neighbor_ids.erase(regionID);
    }

    // add new connections for new neighbors
    for (auto &neighbor : neighbor_ids)
    {
        connectionPtr_t connection = mWebRTCConnections.emplace_back(new LLVoiceWebRTCSpatialConnection(neighbor, INVALID_PARCEL_ID, mChannelID));
        connection->setMicGain(mMicGain);
        connection->setMuteMic(mMuted);
        connection->setSpeakerVolume(mSpeakerVolume);
    }
    return LLWebRTCVoiceClient::sessionState::processConnectionStates();
}

void LLWebRTCVoiceClient::voiceConnectionCoro()
{
    LL_DEBUGS("Voice") << "starting" << LL_ENDL;
    mIsCoroutineActive = true;
    LLCoros::set_consuming(true);
    try
    {
        while (!sShuttingDown)
        {
            llcoro::suspendUntilTimeout(UPDATE_THROTTLE_SECONDS);
            bool voiceEnabled = mVoiceEnabled;

            if (!isAgentAvatarValid())
            {
                continue;
            }

            if (inSpatialChannel())
            {
                // add session for region or parcel voice.
                LLViewerRegion *regionp = gAgent.getRegion();
                if (!regionp || regionp->getRegionID().isNull())
                {
                    continue;
                }

                updatePosition();

                voiceEnabled = voiceEnabled && regionp->isVoiceEnabled();

                if (voiceEnabled)
                {
                    LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
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
                            S32 parcel_local_id = parcel->getLocalID();
                            std::string channelID = regionp->getRegionID().asString() + "-" + std::to_string(parcel->getLocalID());

                            if (!inOrJoiningChannel(channelID))
                            {
                                startParcelSession(channelID, parcel_local_id);
                            }
                        }
                        else 
                        {
                            // parcel using estate voice
                            if (!inEstateChannel())
                            {
                                startEstateSession();
                            }
                        }
                    }
                    else
                    {
                        // estate voice
                        if (!inEstateChannel())
                        {
                            startEstateSession();
                        }
                    }
                }
                else
                {
                    // voice is disabled, so leave and disable PTT
                    leaveChannel(true);
                }
            }
            
            sessionState::processSessionStates();
            if (voiceEnabled)
            {
                sendPositionUpdate(true);
                updateOwnVolume();
            }
        }
    }
    catch (const LLCoros::Stop&)
    {
        LL_DEBUGS("LLWebRTCVoiceClient") << "Received a shutdown exception" << LL_ENDL;
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

    cleanUp();
}


//=========================================================================

void LLWebRTCVoiceClient::sessionTerminate()
{
	mSessionTerminateRequested = true;
}

void LLWebRTCVoiceClient::requestRelog()
{
	mSessionTerminateRequested = true;
	mRelogRequested = true;
}


void LLWebRTCVoiceClient::leaveAudioSession()
{
	if(mSession)
	{
		LL_DEBUGS("Voice") << "leaving session: " << mSession->mChannelID << LL_ENDL;
	}
	else
	{
		LL_WARNS("Voice") << "called with no active session" << LL_ENDL;
	}
    sessionTerminate();
}

void LLWebRTCVoiceClient::clearCaptureDevices()
{
	LL_DEBUGS("Voice") << "called" << LL_ENDL;
	mCaptureDevices.clear();
}

void LLWebRTCVoiceClient::addCaptureDevice(const LLVoiceDevice& device)
{
	LL_DEBUGS("Voice") << "display: '" << device.display_name << "' device: '" << device.full_name << "'" << LL_ENDL;
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

void LLWebRTCVoiceClient::OnDevicesChanged(const llwebrtc::LLWebRTCVoiceDeviceList &render_devices,
                                           const llwebrtc::LLWebRTCVoiceDeviceList &capture_devices)
{
    clearRenderDevices();
    for (auto &device : render_devices)
    {
        addRenderDevice(LLVoiceDevice(device.display_name, device.id));
    }
    clearCaptureDevices();
    for (auto &device : capture_devices)
    {
        addCaptureDevice(LLVoiceDevice(device.display_name, device.id));
    }
    setDevicesListUpdated(true);
}

void LLWebRTCVoiceClient::clearRenderDevices()
{	
	LL_DEBUGS("Voice") << "called" << LL_ENDL;
	mRenderDevices.clear();
}

void LLWebRTCVoiceClient::addRenderDevice(const LLVoiceDevice& device)
{
	LL_DEBUGS("Voice") << "display: '" << device.display_name << "' device: '" << device.full_name << "'" << LL_ENDL;
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
    mTuningMicGain      = volume;
}

void LLWebRTCVoiceClient::tuningSetSpeakerVolume(float volume)
{	

	if (volume != mTuningSpeakerVolume)
	{
        mTuningSpeakerVolume      = volume;
		mTuningSpeakerVolumeDirty = true;
	}
}


float LLWebRTCVoiceClient::tuningGetEnergy(void)
{
    return getAudioLevel();
}

bool LLWebRTCVoiceClient::deviceSettingsAvailable()
{
	bool result = true;
	
	if(mRenderDevices.empty() || mCaptureDevices.empty())
		result = false;
	
	return result;
}
bool LLWebRTCVoiceClient::deviceSettingsUpdated()
{
    bool updated = mDevicesListUpdated;
	mDevicesListUpdated = false;
	return updated;		
}

void LLWebRTCVoiceClient::refreshDeviceLists(bool clearCurrentList)
{
	if(clearCurrentList)
	{
		clearCaptureDevices();
		clearRenderDevices();
	}
    mWebRTCDeviceInterface->refreshDevices();
}

void LLWebRTCVoiceClient::giveUp()
{
	// All has failed.  Clean up and stop trying.
    LL_WARNS("Voice") << "Terminating Voice Service" << LL_ENDL;
	cleanUp();
}

void LLWebRTCVoiceClient::setHidden(bool hidden)
{
    mHidden = hidden;

    if (mHidden && inSpatialChannel())
    {
        // get out of the channel entirely 
        leaveAudioSession();
    }
    else
    {
        sendPositionUpdate(true);
    }
}

void LLWebRTCVoiceClient::sendPositionUpdate(bool force)
{
    Json::FastWriter writer;
    std::string      spatial_data;

    if (mSpatialCoordsDirty || force)
    {
        Json::Value   spatial = Json::objectValue;

        spatial["sp"]   = Json::objectValue;
        spatial["sp"]["x"] = (int) (mAvatarPosition[0] * 100);
        spatial["sp"]["y"] = (int) (mAvatarPosition[1] * 100);
        spatial["sp"]["z"] = (int) (mAvatarPosition[2] * 100);
        spatial["sh"]      = Json::objectValue;
        spatial["sh"]["x"] = (int) (mAvatarRot[0] * 100);
        spatial["sh"]["y"] = (int) (mAvatarRot[1] * 100);
        spatial["sh"]["z"] = (int) (mAvatarRot[2] * 100);
        spatial["sh"]["w"] = (int) (mAvatarRot[3] * 100);

        spatial["lp"]   = Json::objectValue;
        spatial["lp"]["x"] = (int) (mListenerPosition[0] * 100);
        spatial["lp"]["y"] = (int) (mListenerPosition[1] * 100);
        spatial["lp"]["z"] = (int) (mListenerPosition[2] * 100);
        spatial["lh"]      = Json::objectValue;
        spatial["lh"]["x"] = (int) (mListenerRot[0] * 100);
        spatial["lh"]["y"] = (int) (mListenerRot[1] * 100);
        spatial["lh"]["z"] = (int) (mListenerRot[2] * 100);
        spatial["lh"]["w"] = (int) (mListenerRot[3] * 100);

        mSpatialCoordsDirty = false;
        spatial_data = writer.write(spatial);

        sessionState::for_each(boost::bind(predSendData, _1, spatial_data));
    }
}

void LLWebRTCVoiceClient::updateOwnVolume() { 
    F32 audio_level = 0.0;
    if (!mMuteMic && !mTuningMode)
    {
        audio_level = getAudioLevel();
    }

    sessionState::for_each(boost::bind(predUpdateOwnVolume, _1, audio_level)); 
}

void LLWebRTCVoiceClient::predUpdateOwnVolume(const LLWebRTCVoiceClient::sessionStatePtr_t &session, F32 audio_level)
{
    participantStatePtr_t participant = session->findParticipant(gAgentID.asString());
    if (participant)
    {
        participant->mLevel      = audio_level;
        participant->mIsSpeaking = audio_level > SPEAKING_AUDIO_LEVEL;
    }
}

void LLWebRTCVoiceClient::predSendData(const LLWebRTCVoiceClient::sessionStatePtr_t& session, const std::string& spatial_data)
{
    if (session->isSpatial() && !spatial_data.empty())
    {
        session->sendData(spatial_data);
    }
}

void LLWebRTCVoiceClient::sessionState::sendData(const std::string &data)
{
    for (auto& connection : mWebRTCConnections)
    {
        connection->sendData(data);
    }
}

void LLWebRTCVoiceClient::sessionState::setMuteMic(bool muted)
{
    mMuted = muted;
    for (auto& connection : mWebRTCConnections)
    {
        connection->setMuteMic(muted);
    }
}

void LLWebRTCVoiceClient::sessionState::setMicGain(F32 gain)
{
    mMicGain = gain;
    for (auto& connection : mWebRTCConnections)
    {
        connection->setMicGain(gain);
    }
}

void LLWebRTCVoiceClient::sessionState::setSpeakerVolume(F32 volume)
{
    mSpeakerVolume = volume;
    for (auto& connection : mWebRTCConnections)
    {
        connection->setSpeakerVolume(volume);
    }
}

void LLWebRTCVoiceClient::sendLocalAudioUpdates()
{
}

void LLWebRTCVoiceClient::reapSession(const sessionStatePtr_t &session)
{
	if(session)
	{		
		if(session == mSession)
		{
			LL_DEBUGS("Voice") << "NOT deleting session " << session->mChannelID << " (it's the current session)" << LL_ENDL;
		}
		else if(session == mNextSession)
		{
            LL_DEBUGS("Voice") << "NOT deleting session " << session->mChannelID << " (it's the next session)" << LL_ENDL;
		}
		else
		{
			// We don't have a reason to keep tracking this session, so just delete it.
            LL_DEBUGS("Voice") << "deleting session " << session->mChannelID << LL_ENDL;
			deleteSession(session);
		}	
	}
	else
	{
//		LL_DEBUGS("Voice") << "session is NULL" << LL_ENDL;
	}
}


void LLWebRTCVoiceClient::muteListChanged()
{
	// The user's mute list has been updated.  Go through the current participant list and sync it with the mute list.
	if(mSession)
	{
		participantMap::iterator iter = mSession->mParticipantsByURI.begin();
		
		for(; iter != mSession->mParticipantsByURI.end(); iter++)
		{
			participantStatePtr_t p(iter->second);
			
			// Check to see if this participant is on the mute list already
			if(p->updateMuteState())
				mSession->mVolumeDirty = true;
		}
	}
}

/////////////////////////////
// Managing list of participants
LLWebRTCVoiceClient::participantState::participantState(const LLUUID& agent_id) : 
	 mURI(agent_id.asString()),
     mAvatarID(agent_id),
	 mPTT(false), 
	 mIsSpeaking(false), 
	 mIsModeratorMuted(false), 
	 mLastSpokeTimestamp(0.f), 
	 mLevel(0.f), 
	 mVolume(LLVoiceClient::VOLUME_DEFAULT), 
	 mUserVolume(0),
	 mOnMuteList(false), 
	 mVolumeSet(false),
	 mVolumeDirty(false), 
	 mAvatarIDValid(false),
	 mIsSelf(false)
{
}

LLWebRTCVoiceClient::participantStatePtr_t LLWebRTCVoiceClient::sessionState::addParticipant(const LLUUID& agent_id)
{
    participantStatePtr_t result;
	
    participantUUIDMap::iterator iter = mParticipantsByUUID.find(agent_id);


	if (iter != mParticipantsByUUID.end())
	{
		result = iter->second;
	}
		
	if(!result)
	{
		// participant isn't already in one list or the other.
		result.reset(new participantState(agent_id));
        mParticipantsByURI.insert(participantMap::value_type(agent_id.asString(), result));
        mParticipantsByUUID.insert(participantUUIDMap::value_type(agent_id, result));
		mParticipantsChanged = true;

		result->mAvatarIDValid = true;
        result->mAvatarID      = agent_id;
		
        if(result->updateMuteState())
        {
	        mMuteDirty = true;
        }

		if (LLSpeakerVolumeStorage::getInstance()->getSpeakerVolume(result->mAvatarID, result->mVolume))
		{
			result->mVolumeDirty = true;
			mVolumeDirty = true;
		}
		
		LL_DEBUGS("Voice") << "participant \"" << result->mURI << "\" added." << LL_ENDL;
	}
	
	return result;
}

bool LLWebRTCVoiceClient::participantState::updateMuteState()
{
	bool result = false;

	bool isMuted = LLMuteList::getInstance()->isMuted(mAvatarID, LLMute::flagVoiceChat);
	if(mOnMuteList != isMuted)
	{
	    mOnMuteList = isMuted;
	    mVolumeDirty = true;
	    result = true;
	}
	return result;
}

bool LLWebRTCVoiceClient::participantState::isAvatar()
{
	return mAvatarIDValid;
}

void LLWebRTCVoiceClient::sessionState::removeParticipant(const LLWebRTCVoiceClient::participantStatePtr_t &participant)
{
	if(participant)
	{
		participantMap::iterator iter = mParticipantsByURI.find(participant->mURI);
		participantUUIDMap::iterator iter2 = mParticipantsByUUID.find(participant->mAvatarID);
		
		LL_DEBUGS("Voice") << "participant \"" << participant->mURI <<  "\" (" << participant->mAvatarID << ") removed." << LL_ENDL;
		
		if(iter == mParticipantsByURI.end())
		{
			LL_WARNS("Voice") << "Internal error: participant " << participant->mURI << " not in URI map" << LL_ENDL;
		}
		else if(iter2 == mParticipantsByUUID.end())
		{
			LL_WARNS("Voice") << "Internal error: participant ID " << participant->mAvatarID << " not in UUID map" << LL_ENDL;
		}
		else if(iter->second != iter2->second)
		{
			LL_WARNS("Voice") << "Internal error: participant mismatch!" << LL_ENDL;
		}
		else
		{
			mParticipantsByURI.erase(iter);
			mParticipantsByUUID.erase(iter2);

            mParticipantsChanged = true;
		}
	}
}

void LLWebRTCVoiceClient::sessionState::removeAllParticipants()
{
	LL_DEBUGS("Voice") << "called" << LL_ENDL;

	while(!mParticipantsByURI.empty())
	{
		removeParticipant(mParticipantsByURI.begin()->second);
	}
	
	if(!mParticipantsByUUID.empty())
	{
		LL_WARNS("Voice") << "Internal error: empty URI map, non-empty UUID map" << LL_ENDL;
	}
}


void LLWebRTCVoiceClient::getParticipantList(std::set<LLUUID> &participants)
{
	if(mSession)
	{
		for(participantUUIDMap::iterator iter = mSession->mParticipantsByUUID.begin();
			iter != mSession->mParticipantsByUUID.end(); 
			iter++)
		{
			participants.insert(iter->first);
		}
	}
}

bool LLWebRTCVoiceClient::isParticipant(const LLUUID &speaker_id)
{
  if(mSession)
    {
      return (mSession->mParticipantsByUUID.find(speaker_id) != mSession->mParticipantsByUUID.end());
    }
  return false;
}

LLWebRTCVoiceClient::participantStatePtr_t LLWebRTCVoiceClient::sessionState::findParticipant(const std::string &uri)
{
    participantStatePtr_t result;
	
	participantMap::iterator iter = mParticipantsByURI.find(uri);

	if(iter != mParticipantsByURI.end())
	{
		result = iter->second;
	}
		
	return result;
}

LLWebRTCVoiceClient::participantStatePtr_t LLWebRTCVoiceClient::sessionState::findParticipantByID(const LLUUID& id)
{
    participantStatePtr_t result;
	participantUUIDMap::iterator iter = mParticipantsByUUID.find(id);

	if(iter != mParticipantsByUUID.end())
	{
		result = iter->second;
	}

	return result;
}

LLWebRTCVoiceClient::participantStatePtr_t LLWebRTCVoiceClient::findParticipantByID(const std::string& channelID, const LLUUID& id)
{
    participantStatePtr_t result;
    auto session = sessionState::matchSessionByChannelID(channelID);
	
	if (session)
	{
        result = session->findParticipantByID(id);
	}
	
	return result;
}

LLWebRTCVoiceClient::participantStatePtr_t LLWebRTCVoiceClient::addParticipantByID(const std::string& channelID, const LLUUID &id)
{ 
	participantStatePtr_t result;
    auto session = sessionState::matchSessionByChannelID(channelID);
    if (session)
    {
        result = session->addParticipant(id);
    }
    return result;
}

void LLWebRTCVoiceClient::removeParticipantByID(const std::string &channelID, const LLUUID &id)
{
    participantStatePtr_t result;
    auto session = sessionState::matchSessionByChannelID(channelID);
    if (session)
    {
        participantStatePtr_t participant = session->findParticipantByID(id);
        if (participant)
        {
            session->removeParticipant(participant);
        }
    }
}

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

bool LLWebRTCVoiceClient::startAdHocSession(const std::string &channelID, const std::string &credentials)
{
    leaveChannel(false);
    mNextSession = addSession(channelID, sessionState::ptr_t(new adhocSessionState(channelID, credentials)));
    return true;
}

void LLWebRTCVoiceClient::joinSession(const sessionStatePtr_t &session)
{
	mNextSession = session;

    if (mSession)
    {
        // If we're already in a channel, or if we're joining one, terminate
        // so we can rejoin with the new session data.
        sessionTerminate();
    }
}

void LLWebRTCVoiceClient::callUser(const LLUUID &uuid) 
{
}

void LLWebRTCVoiceClient::endUserIMSession(const LLUUID &uuid)
{

}
bool LLWebRTCVoiceClient::isValidChannel(std::string &channelID)
{
  return(findP2PSession(LLUUID(channelID)) != NULL);
	
}
bool LLWebRTCVoiceClient::answerInvite(std::string &channelID)
{	
	return false;
}

bool LLWebRTCVoiceClient::isVoiceWorking() const
{

    //Added stateSessionTerminated state to avoid problems with call in parcels with disabled voice (EXT-4758)
    // Condition with joining spatial num was added to take into account possible problems with connection to voice
    // server(EXT-4313). See bug descriptions and comments for MAX_NORMAL_JOINING_SPATIAL_NUM for more info.
    return (mSpatialJoiningNum < MAX_NORMAL_JOINING_SPATIAL_NUM) && mIsProcessingChannels;
//  return (mSpatialJoiningNum < MAX_NORMAL_JOINING_SPATIAL_NUM) && (stateLoggedIn <= mState) && (mState <= stateSessionTerminated);
}

// Returns true if the indicated participant in the current audio session is really an SL avatar.
// Currently this will be false only for PSTN callers into group chats, and PSTN p2p calls.
BOOL LLWebRTCVoiceClient::isParticipantAvatar(const LLUUID &id)
{
	BOOL result = TRUE; 
	return result;
}

// Returns true if calling back the session URI after the session has closed is possible.
// Currently this will be false only for PSTN P2P calls.		
BOOL LLWebRTCVoiceClient::isSessionCallBackPossible(const LLUUID &session_id)
{
	BOOL result = TRUE; 
    sessionStatePtr_t session(findP2PSession(session_id));
	
	if(session != NULL)
	{
		result = session->isCallBackPossible();
	}
	
	return result;
}

// Returns true if the session can accept text IM's.
// Currently this will be false only for PSTN P2P calls.
BOOL LLWebRTCVoiceClient::isSessionTextIMPossible(const LLUUID &session_id)
{
	bool result = TRUE;
    sessionStatePtr_t session(findP2PSession(session_id));
	
	if(session != NULL)
	{
		result = session->isTextIMPossible();
	}
	
	return result;
}
		

void LLWebRTCVoiceClient::declineInvite(std::string &sessionHandle)
{
}

void LLWebRTCVoiceClient::leaveNonSpatialChannel()
{
    LL_DEBUGS("Voice") << "Request to leave spacial channel." << LL_ENDL;
	
	// Make sure we don't rejoin the current session.	
    sessionStatePtr_t oldNextSession(mNextSession);
	mNextSession.reset();
	
	// Most likely this will still be the current session at this point, but check it anyway.
	reapSession(oldNextSession);
	
	sessionTerminate();
}

std::string LLWebRTCVoiceClient::getCurrentChannel()
{
	return getAudioSessionURI();
}

bool LLWebRTCVoiceClient::inProximalChannel()
{
	return inSpatialChannel();
}

std::string LLWebRTCVoiceClient::nameFromAvatar(LLVOAvatar *avatar)
{
	std::string result;
	if(avatar)
	{
		result = nameFromID(avatar->getID());
	}	
	return result;
}

std::string LLWebRTCVoiceClient::nameFromID(const LLUUID &uuid)
{
	std::string result;
	
	if (uuid.isNull()) {
		//WebRTC, the uuid emtpy look for the mURIString and return that instead.
		//result.assign(uuid.mURIStringName);
		LLStringUtil::replaceChar(result, '_', ' ');
		return result;
	}
	// Prepending this apparently prevents conflicts with reserved names inside the WebRTC code.
	result = "x";
	
	// Base64 encode and replace the pieces of base64 that are less compatible 
	// with e-mail local-parts.
	// See RFC-4648 "Base 64 Encoding with URL and Filename Safe Alphabet"
	result += LLBase64::encode(uuid.mData, UUID_BYTES);
	LLStringUtil::replaceChar(result, '+', '-');
	LLStringUtil::replaceChar(result, '/', '_');
	
	// If you need to transform a GUID to this form on the Mac OS X command line, this will do so:
	// echo -n x && (echo e669132a-6c43-4ee1-a78d-6c82fff59f32 |xxd -r -p |openssl base64|tr '/+' '_-')
	
	// The reverse transform can be done with:
	// echo 'x5mkTKmxDTuGnjWyC__WfMg==' |cut -b 2- -|tr '_-' '/+' |openssl base64 -d|xxd -p
	
	return result;
}

bool LLWebRTCVoiceClient::IDFromName(const std::string inName, LLUUID &uuid)
{
	bool result = false;
	
	// SLIM SDK: The "name" may actually be a SIP URI such as: "sip:xFnPP04IpREWNkuw1cOXlhw==@bhr.WebRTC.com"
	// If it is, convert to a bare name before doing the transform.
	std::string name;
	
	// Doesn't look like a SIP URI, assume it's an actual name.
	if(name.empty())
		name = inName;

	// This will only work if the name is of the proper form.
	// As an example, the account name for Monroe Linden (UUID 1673cfd3-8229-4445-8d92-ec3570e5e587) is:
	// "xFnPP04IpREWNkuw1cOXlhw=="
	
	if((name.size() == 25) && (name[0] == 'x') && (name[23] == '=') && (name[24] == '='))
	{
		// The name appears to have the right form.

		// Reverse the transforms done by nameFromID
		std::string temp = name;
		LLStringUtil::replaceChar(temp, '-', '+');
		LLStringUtil::replaceChar(temp, '_', '/');

		U8 rawuuid[UUID_BYTES + 1]; 
		int len = apr_base64_decode_binary(rawuuid, temp.c_str() + 1);
		if(len == UUID_BYTES)
		{
			// The decode succeeded.  Stuff the bits into the result's UUID
			memcpy(uuid.mData, rawuuid, UUID_BYTES);
			result = true;
		}
	} 
	
	if(!result)
	{
		// WebRTC:  not a standard account name, just copy the URI name mURIString field
		// and hope for the best.  bpj
		uuid.setNull();  // WebRTC, set the uuid field to nulls
	}
	
	return result;
}

std::string LLWebRTCVoiceClient::displayNameFromAvatar(LLVOAvatar *avatar)
{
	return avatar->getFullname();
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

std::string LLWebRTCVoiceClient::getAudioSessionURI()
{
	std::string result;
	
	if(mSession)
		result = mSession->mChannelID;
		
	return result;
}

/////////////////////////////
// Sending updates of current state

void LLWebRTCVoiceClient::enforceTether()
{
	LLVector3d tethered	= mListenerRequestedPosition;

	// constrain 'tethered' to within 50m of mAvatarPosition.
	{
		LLVector3d camera_offset = mListenerRequestedPosition - mAvatarPosition;
		F32 camera_distance = (F32)camera_offset.magVec();
		if(camera_distance > MAX_AUDIO_DIST)
		{
			tethered = mAvatarPosition + 
				(MAX_AUDIO_DIST / camera_distance) * camera_offset;
		}
	}
	
	if(dist_vec_squared(mListenerPosition, tethered) > 0.01)
	{
        mListenerPosition   = tethered;
		mSpatialCoordsDirty = true;
	}
}

void LLWebRTCVoiceClient::updateNeighboringRegions()
{
    static const std::vector<LLVector3d> neighbors {LLVector3d(0.0f, 1.0f, 0.0f),  LLVector3d(0.707f, 0.707f, 0.0f),
                                                    LLVector3d(1.0f, 0.0f, 0.0f),  LLVector3d(0.707f, -0.707f, 0.0f),
                                                    LLVector3d(0.0f, -1.0f, 0.0f), LLVector3d(-0.707f, -0.707f, 0.0f),
                                                    LLVector3d(-1.0f, 0.0f, 0.0f), LLVector3d(-0.707f, 0.707f, 0.0f)};

    // Estate voice requires connection to neighboring regions.
    mNeighboringRegions.clear();

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

void LLWebRTCVoiceClient::updatePosition(void)
{
	LLViewerRegion *region = gAgent.getRegion();
	if(region && isAgentAvatarValid())
	{
        LLVector3d avatar_pos = gAgentAvatarp->getPositionGlobal();
        LLQuaternion avatar_qrot = gAgentAvatarp->getRootJoint()->getWorldRotation();

		avatar_pos += LLVector3d(0.f, 0.f, 1.f); // bump it up to head height
		
		// TODO: If camera and avatar velocity are actually used by the voice system, we could compute them here...
		// They're currently always set to zero.
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
		
		setAvatarPosition(
			avatar_pos,			// position
			LLVector3::zero, 	// velocity
			avatar_qrot);		// rotation matrix

        enforceTether();

        updateNeighboringRegions();
	}
}

void LLWebRTCVoiceClient::setListenerPosition(const LLVector3d &position, const LLVector3 &velocity, const LLQuaternion &rot)
{

	mListenerRequestedPosition = position;
	
	if(mListenerVelocity != velocity)
	{
		mListenerVelocity = velocity;
		mSpatialCoordsDirty = true;
	}
	
	if(mListenerRot != rot)
	{
		mListenerRot = rot;
		mSpatialCoordsDirty = true;
	}
}

void LLWebRTCVoiceClient::setAvatarPosition(const LLVector3d &position, const LLVector3 &velocity, const LLQuaternion &rot)
{
	if(dist_vec_squared(mAvatarPosition, position) > 0.01)
	{
		mAvatarPosition = position;
		mSpatialCoordsDirty = true;
	}
	
	if(mAvatarVelocity != velocity)
	{
		mAvatarVelocity = velocity;
		mSpatialCoordsDirty = true;
	}
	
    // If the two rotations are not exactly equal test their dot product
    // to get the cos of the angle between them.
    // If it is too small, don't update.
    F32 rot_cos_diff = llabs(dot(mAvatarRot, rot));
    if ((mAvatarRot != rot) && (rot_cos_diff < MINUSCULE_ANGLE_COS))
	{   
		mAvatarRot = rot;
		mSpatialCoordsDirty = true;
	}
}

bool LLWebRTCVoiceClient::channelFromRegion(LLViewerRegion *region, std::string &name)
{
	bool result = false;
	
	if(region)
	{
		name = region->getName();
	}
	
	if(!name.empty())
		result = true;
	
	return result;
}

void LLWebRTCVoiceClient::leaveChannel(bool stopTalking)
{
    mChannelName.clear();

   if (mSession)
    {
        // If we're already in a channel, or if we're joining one, terminate
        // so we can rejoin with the new session data.
        sessionTerminate();
        notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL);
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

void LLWebRTCVoiceClient::setMuteMic(bool muted)
{

    mMuteMic = muted;
    sessionState::for_each(boost::bind(predSetMuteMic, _1, muted));
}

void LLWebRTCVoiceClient::predSetMuteMic(const LLWebRTCVoiceClient::sessionStatePtr_t &session, bool muted)
{
    participantStatePtr_t participant = session->findParticipant(gAgentID.asString());
    if (participant)
    {
        participant->mLevel = 0.0;
    }
    session->setMuteMic(muted);
}

void LLWebRTCVoiceClient::predSetSpeakerVolume(const LLWebRTCVoiceClient::sessionStatePtr_t &session, F32 volume)
{
    session->setSpeakerVolume(volume);
}

void LLWebRTCVoiceClient::predSetMicGain(const LLWebRTCVoiceClient::sessionStatePtr_t &session, F32 gain)
{
    session->setMicGain(gain);
}

void LLWebRTCVoiceClient::setVoiceEnabled(bool enabled)
{
    LL_DEBUGS("Voice")
        << "( " << (enabled ? "enabled" : "disabled") << " )"
        << " was "<< (mVoiceEnabled ? "enabled" : "disabled")
        << " coro "<< (mIsCoroutineActive ? "active" : "inactive")
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
            if (!mIsCoroutineActive)
            {
                LLCoros::instance().launch("LLWebRTCVoiceClient::voiceConnectionCoro",
                    boost::bind(&LLWebRTCVoiceClient::voiceConnectionCoro, LLWebRTCVoiceClient::getInstance()));
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

bool LLWebRTCVoiceClient::voiceEnabled()
{
    return gSavedSettings.getBOOL("EnableVoiceChat") &&
          !gSavedSettings.getBOOL("CmdLineDisableVoice") &&
          !gNonInteractive;
}

void LLWebRTCVoiceClient::setLipSyncEnabled(BOOL enabled)
{
	mLipSyncEnabled = enabled;
}

BOOL LLWebRTCVoiceClient::lipSyncEnabled()
{
	   
	if ( mVoiceEnabled )
	{
		return mLipSyncEnabled;
	}
	else
	{
		return FALSE;
	}
}


void LLWebRTCVoiceClient::setEarLocation(S32 loc)
{
	if(mEarLocation != loc)
	{
		LL_DEBUGS("Voice") << "Setting mEarLocation to " << loc << LL_ENDL;
		
		mEarLocation = loc;
		mSpatialCoordsDirty = true;
	}
}

void LLWebRTCVoiceClient::setVoiceVolume(F32 volume)
{
	if (volume != mSpeakerVolume)
	{
        {
            mSpeakerVolume      = volume;
            mSpeakerVolumeDirty = true;
        }
        sessionState::for_each(boost::bind(predSetSpeakerVolume, _1, volume));
	}
}

void LLWebRTCVoiceClient::setMicGain(F32 gain)
{	
	if (gain != mMicGain)
    {
        mMicGain = gain;
        sessionState::for_each(boost::bind(predSetMicGain, _1, gain));
    }
}

/////////////////////////////
// Accessors for data related to nearby speakers
BOOL LLWebRTCVoiceClient::getVoiceEnabled(const LLUUID& id)
{
	BOOL result = FALSE;
    if (!mSession)
    {
        return FALSE;
    }
    participantStatePtr_t participant(mSession->findParticipant(id.asString()));
	if(participant)
	{
		// I'm not sure what the semantics of this should be.
		// For now, if we have any data about the user that came through the chat channel, assume they're voice-enabled.
		result = TRUE;
	}
	
	return result;
}

std::string LLWebRTCVoiceClient::getDisplayName(const LLUUID& id)
{
	std::string result;
    if (!mSession)
    {
        return result;
    }
    participantStatePtr_t participant(mSession->findParticipant(id.asString()));
	if(participant)
	{
		result = participant->mDisplayName;
	}
	
	return result;
}



BOOL LLWebRTCVoiceClient::getIsSpeaking(const LLUUID& id)
{
	BOOL result = FALSE;
    if (!mSession)
    {
        return result;
    }
    participantStatePtr_t participant(mSession->findParticipant(id.asString()));
	if(participant)
	{
		result = participant->mIsSpeaking;
	}
	
	return result;
}

BOOL LLWebRTCVoiceClient::getIsModeratorMuted(const LLUUID& id)
{
	BOOL result = FALSE;
    if (!mSession)
    {
        return result;
    }
    participantStatePtr_t participant(mSession->findParticipant(id.asString()));
	if(participant)
	{
		result = participant->mIsModeratorMuted;
	}
	
	return result;
}

F32 LLWebRTCVoiceClient::getCurrentPower(const LLUUID &id)
{
    F32 result = 0;
    if (!mSession)
    {
        return result;
    }
    participantStatePtr_t participant(mSession->findParticipant(id.asString()));
	if (participant)
	{
		result = participant->mLevel;
	}
	return result;
}

BOOL LLWebRTCVoiceClient::getUsingPTT(const LLUUID& id)
{
	BOOL result = FALSE;
    if (!mSession)
    {
        return result;
    }
    participantStatePtr_t participant(mSession->findParticipant(id.asString()));
	if(participant)
	{
		// I'm not sure what the semantics of this should be.
		// Does "using PTT" mean they're configured with a push-to-talk button?
		// For now, we know there's no PTT mechanism in place, so nobody is using it.
	}
	
	return result;
}

BOOL LLWebRTCVoiceClient::getOnMuteList(const LLUUID& id)
{
	BOOL result = FALSE;
	
    participantStatePtr_t participant(mSession->findParticipant(id.asString()));
	if(participant)
	{
		result = participant->mOnMuteList;
	}

	return result;
}

// External accessors.
F32 LLWebRTCVoiceClient::getUserVolume(const LLUUID& id)
{
    // Minimum volume will be returned for users with voice disabled
    F32 result = LLVoiceClient::VOLUME_MIN;
	
    participantStatePtr_t participant(mSession->findParticipant(id.asString()));
    if(participant)
	{
		result = participant->mVolume;

		// Enable this when debugging voice slider issues.  It's way to spammy even for debug-level logging.
		// LL_DEBUGS("Voice") << "mVolume = " << result <<  " for " << id << LL_ENDL;
	}

	return result;
}

void LLWebRTCVoiceClient::setUserVolume(const LLUUID& id, F32 volume)
{
	if(mSession)
	{
        participantStatePtr_t participant(mSession->findParticipant(id.asString()));
		if (participant && !participant->mIsSelf)
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

			participant->mVolume = llclamp(volume, LLVoiceClient::VOLUME_MIN, LLVoiceClient::VOLUME_MAX);
			participant->mVolumeDirty = true;
			mSession->mVolumeDirty = true;
		}
	}
}

std::string LLWebRTCVoiceClient::getGroupID(const LLUUID& id)
{
	std::string result;

    participantStatePtr_t participant(mSession->findParticipant(id.asString()));
	if(participant)
	{
		result = participant->mGroupID;
	}
	
	return result;
}

BOOL LLWebRTCVoiceClient::getAreaVoiceDisabled()
{
	return mAreaVoiceDisabled;
}

//------------------------------------------------------------------------
std::map<std::string, LLWebRTCVoiceClient::sessionState::ptr_t> LLWebRTCVoiceClient::sessionState::mSessions;


LLWebRTCVoiceClient::sessionState::sessionState() :
    mErrorStatusCode(0),
    mVolumeDirty(false),
    mMuteDirty(false),
    mParticipantsChanged(false),
    mShuttingDown(false)
{
}

/*static*/
void LLWebRTCVoiceClient::sessionState::addSession(
    const std::string                       &channelID,
    LLWebRTCVoiceClient::sessionState::ptr_t& session)
{
    session->addParticipant(gAgentID);
    mSessions[channelID] = session;
}

LLWebRTCVoiceClient::sessionState::~sessionState()
{
    LL_INFOS("Voice") << "Destroying session CHANNEL=" << mChannelID << LL_ENDL;

	removeAllParticipants();
}

bool LLWebRTCVoiceClient::sessionState::isCallBackPossible()
{
	// This may change to be explicitly specified by WebRTC in the future...
	// Currently, only PSTN P2P calls cannot be returned.
	// Conveniently, this is also the only case where we synthesize a caller UUID.
	return false;
}

bool LLWebRTCVoiceClient::sessionState::isTextIMPossible()
{
	// This may change to be explicitly specified by WebRTC in the future...
	return false;
}

/*static*/
LLWebRTCVoiceClient::sessionState::ptr_t LLWebRTCVoiceClient::sessionState::matchSessionByChannelID(const std::string& channel_id)
{
    sessionStatePtr_t result;

    // *TODO: My kingdom for a lambda!
    std::map<std::string, ptr_t>::iterator it = mSessions.find(channel_id);
    if (it != mSessions.end())
    {
        result = (*it).second;
    }
    return result;
}

void LLWebRTCVoiceClient::sessionState::for_each(sessionFunc_t func)
{
    std::for_each(mSessions.begin(), mSessions.end(), boost::bind(for_eachPredicate, _1, func));
}

void LLWebRTCVoiceClient::sessionState::reapEmptySessions()
{
    std::map<std::string, ptr_t>::iterator iter;
    for (iter = mSessions.begin(); iter != mSessions.end();)
    {
        if (iter->second->isEmpty())
        {
            iter = mSessions.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}


bool LLWebRTCVoiceClient::sessionState::testByCreatingURI(const LLWebRTCVoiceClient::sessionState::wptr_t &a, std::string uri)
{
    ptr_t aLock(a.lock());

    return aLock ? (aLock->mChannelID == LLUUID(uri)) : false;
}

bool LLWebRTCVoiceClient::sessionState::testByCallerId(const LLWebRTCVoiceClient::sessionState::wptr_t &a, LLUUID participantId)
{
    ptr_t aLock(a.lock());

    return aLock ? ((aLock->mCallerID == participantId) || (aLock->mIMSessionID == participantId)) : false;
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


LLWebRTCVoiceClient::sessionStatePtr_t LLWebRTCVoiceClient::addSession(const std::string &channel_id, sessionState::ptr_t session)
{
    sessionStatePtr_t existingSession = sessionState::matchSessionByChannelID(channel_id);
	if (!existingSession)
	{
		// No existing session found.
		
		LL_DEBUGS("Voice") << "adding new session: CHANNEL " << channel_id << LL_ENDL;
        session->setMuteMic(mMuteMic);
        session->setMicGain(mMicGain);
        session->setSpeakerVolume(mSpeakerVolume);

		if (LLVoiceClient::instance().getVoiceEffectEnabled())
		{
            session->mVoiceFontID = LLVoiceClient::instance().getVoiceEffectDefault();
		}

        sessionState::addSession(channel_id, session);
        return session;
	}
	else
	{
		// Found an existing session
		LL_DEBUGS("Voice") << "Attempting to add already-existing session " << channel_id << LL_ENDL;
        return existingSession;
	}	
}

void LLWebRTCVoiceClient::predShutdownSession(const LLWebRTCVoiceClient::sessionStatePtr_t& session)
{ 
    session->shutdownAllConnections();
}

void LLWebRTCVoiceClient::deleteSession(const sessionStatePtr_t &session)
{
	// At this point, the session should be unhooked from all lists and all state should be consistent.
    session->shutdownAllConnections();
	// If this is the current audio session, clean up the pointer which will soon be dangling.
    bool deleteAudioSession = mSession == session;
    bool deleteNextAudioSession = mNextSession == session;
    if (deleteAudioSession)
	{
		mSession.reset();
		mSessionChanged = true;
	}

	// ditto for the next audio session
    if (deleteNextAudioSession)
	{
		mNextSession.reset();
	}
}

void LLWebRTCVoiceClient::addObserver(LLVoiceClientParticipantObserver* observer)
{
	mParticipantObservers.insert(observer);
}

void LLWebRTCVoiceClient::removeObserver(LLVoiceClientParticipantObserver* observer)
{
	mParticipantObservers.erase(observer);
}

void LLWebRTCVoiceClient::notifyParticipantObservers()
{
	for (observer_set_t::iterator it = mParticipantObservers.begin();
		it != mParticipantObservers.end();
		)
	{
		LLVoiceClientParticipantObserver* observer = *it;
		observer->onParticipantsChanged();
		// In case onParticipantsChanged() deleted an entry.
		it = mParticipantObservers.upper_bound(observer);
	}
}

void LLWebRTCVoiceClient::addObserver(LLVoiceClientStatusObserver* observer)
{
	mStatusObservers.insert(observer);
}

void LLWebRTCVoiceClient::removeObserver(LLVoiceClientStatusObserver* observer)
{
	mStatusObservers.erase(observer);
}

void LLWebRTCVoiceClient::notifyStatusObservers(LLVoiceClientStatusObserver::EStatusType status)
{
    LL_DEBUGS("Voice") << "( " << LLVoiceClientStatusObserver::status2string(status) << " )"
                       << " mSession=" << mSession
                       << LL_ENDL;

	if(mSession)
	{
		if(status == LLVoiceClientStatusObserver::ERROR_UNKNOWN)
		{
			switch(mSession->mErrorStatusCode)
			{
				case 20713:		status = LLVoiceClientStatusObserver::ERROR_CHANNEL_FULL; 		break;
				case 20714:		status = LLVoiceClientStatusObserver::ERROR_CHANNEL_LOCKED; 	break;
				case 20715:
					//invalid channel, we may be using a set of poorly cached
					//info
					status = LLVoiceClientStatusObserver::ERROR_NOT_AVAILABLE;
					break;
				case 1009:
					//invalid username and password
					status = LLVoiceClientStatusObserver::ERROR_NOT_AVAILABLE;
					break;
			}

			// Reset the error code to make sure it won't be reused later by accident.
			mSession->mErrorStatusCode = 0;
		}
		else if(status == LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL)
		{
			switch(mSession->mErrorStatusCode)
			{
				case HTTP_NOT_FOUND:	// NOT_FOUND
				// *TODO: Should this be 503?
				case 480:	// TEMPORARILY_UNAVAILABLE
				case HTTP_REQUEST_TIME_OUT:	// REQUEST_TIMEOUT
					// call failed because other user was not available
					// treat this as an error case
					status = LLVoiceClientStatusObserver::ERROR_NOT_AVAILABLE;

					// Reset the error code to make sure it won't be reused later by accident.
					mSession->mErrorStatusCode = 0;
				break;
			}
		}
	}
		
	LL_DEBUGS("Voice") 
		<< " " << LLVoiceClientStatusObserver::status2string(status)  
		<< ", session URI " << getAudioSessionURI() 
		<< ", proximal is " << inSpatialChannel()
        << LL_ENDL;

    mIsProcessingChannels = status == LLVoiceClientStatusObserver::STATUS_LOGGED_IN;

	for (status_observer_set_t::iterator it = mStatusObservers.begin();
		it != mStatusObservers.end();
		)
	{
		LLVoiceClientStatusObserver* observer = *it;
		observer->onChange(status, getAudioSessionURI(), inSpatialChannel());
		// In case onError() deleted an entry.
		it = mStatusObservers.upper_bound(observer);
	}

	// skipped to avoid speak button blinking
	if (   status != LLVoiceClientStatusObserver::STATUS_JOINING
		&& status != LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL
		&& status != LLVoiceClientStatusObserver::STATUS_VOICE_DISABLED)
	{
		bool voice_status = LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking();

		gAgent.setVoiceConnected(voice_status);

		if (voice_status)
		{
			LLFirstUse::speak(true);
		}
	}
}

void LLWebRTCVoiceClient::addObserver(LLFriendObserver* observer)
{
	mFriendObservers.insert(observer);
}

void LLWebRTCVoiceClient::removeObserver(LLFriendObserver* observer)
{
	mFriendObservers.erase(observer);
}

void LLWebRTCVoiceClient::notifyFriendObservers()
{
	for (friend_observer_set_t::iterator it = mFriendObservers.begin();
		it != mFriendObservers.end();
		)
	{
		LLFriendObserver* observer = *it;
		it++;
		// The only friend-related thing we notify on is online/offline transitions.
		observer->changed(LLFriendObserver::ONLINE);
	}
}

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
        // and post a "participants updated" message to listeners later.
        session->mParticipantsChanged = true;
    }

    // Check whether this is a p2p session whose caller name just resolved
    if (session->mCallerID == id)
    {
        // this session's "caller ID" just resolved.  Fill in the name.
        session->mName = name;
    }
}

void LLWebRTCVoiceClient::avatarNameResolved(const LLUUID &id, const std::string &name)
{
    sessionState::for_each(boost::bind(predAvatarNameResolution, _1, id, name));
}


std::string LLWebRTCVoiceClient::sipURIFromID(const LLUUID& id) { return id.asString(); }


/////////////////////////////
// LLVoiceWebRTCConnection

LLVoiceWebRTCConnection::LLVoiceWebRTCConnection(const LLUUID &regionID, const std::string &channelID) :
    mWebRTCAudioInterface(nullptr),
    mWebRTCDataInterface(nullptr),
    mVoiceConnectionState(VOICE_STATE_START_SESSION),
    mMuted(true),
    mShutDown(false),
    mTrickling(false),
    mIceCompleted(false),
    mSpeakerVolume(0.0),
    mMicGain(0.0),
    mOutstandingRequests(0),
    mChannelID(channelID),
    mRegionID(regionID)
{
    mWebRTCPeerConnection = llwebrtc::newPeerConnection();
    mWebRTCPeerConnection->setSignalingObserver(this);
}

LLVoiceWebRTCConnection::~LLVoiceWebRTCConnection()
{
    if (LLWebRTCVoiceClient::isShuttingDown())
    {
        // peer connection and observers will be cleaned up
        // by llwebrtc::terminate() on shutdown.
        return;
    }
    llwebrtc::freePeerConnection(mWebRTCPeerConnection);
    mWebRTCPeerConnection = nullptr;
}

void LLVoiceWebRTCConnection::OnIceGatheringState(llwebrtc::LLWebRTCSignalingObserver::IceGatheringState state)
{
    LL_INFOS("Voice") << "Ice Gathering voice account. " << state << LL_ENDL;

    switch (state)
    {
        case llwebrtc::LLWebRTCSignalingObserver::IceGatheringState::ICE_GATHERING_COMPLETE:
        {
            LLMutexLock lock(&mVoiceStateMutex);
            mIceCompleted = true;
            break;
        }
        case llwebrtc::LLWebRTCSignalingObserver::IceGatheringState::ICE_GATHERING_NEW:
        {
            LLMutexLock lock(&mVoiceStateMutex);
            mIceCompleted = false;
        }
        default:
            break;
    }
}

void LLVoiceWebRTCConnection::OnIceCandidate(const llwebrtc::LLWebRTCIceCandidate &candidate)
{
    LLMutexLock lock(&mVoiceStateMutex);
    mIceCandidates.push_back(candidate);
}

void LLVoiceWebRTCConnection::onIceUpdateComplete(bool ice_completed, const LLSD &result)
{
    if (LLWebRTCVoiceClient::isShuttingDown())
    {
        return;
    }
    mTrickling = false;
    mOutstandingRequests--;
}

void LLVoiceWebRTCConnection::onIceUpdateError(int retries, std::string url, LLSD body, bool ice_completed, const LLSD &result)
{
    if (LLWebRTCVoiceClient::isShuttingDown())
    {
        return;
    }
    LLCore::HttpRequest::policy_t               httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("voiceAccountProvision", httpPolicy));

    if (retries >= 0)
    {
        LL_WARNS("Voice") << "Unable to complete ice trickling voice account, retrying.  " << result << LL_ENDL;
        LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpPost(
            url,
            LLCore::HttpRequest::DEFAULT_POLICY_ID,
            body,
            boost::bind(&LLVoiceWebRTCConnection::onIceUpdateComplete, this, ice_completed, _1),
            boost::bind(&LLVoiceWebRTCConnection::onIceUpdateError, this, retries - 1, url, body, ice_completed, _1));
        return;
    }

    LL_WARNS("Voice") << "Unable to complete ice trickling voice account, restarting connection.  " << result << LL_ENDL;
    if (!mShutDown)
    {
        setVoiceConnectionState(VOICE_STATE_SESSION_RETRY);
    }
    mTrickling = false;

    mOutstandingRequests--;
}

void LLVoiceWebRTCConnection::OnOfferAvailable(const std::string &sdp)
{
    LL_INFOS("Voice") << "On Offer Available." << LL_ENDL;
    LLMutexLock lock(&mVoiceStateMutex);
    mChannelSDP = sdp;
    if (mVoiceConnectionState == VOICE_STATE_WAIT_FOR_SESSION_START)
    {
        mVoiceConnectionState = VOICE_STATE_REQUEST_CONNECTION;
    }
}

void LLVoiceWebRTCConnection::OnAudioEstablished(llwebrtc::LLWebRTCAudioInterface *audio_interface)
{
    LL_INFOS("Voice") << "On AudioEstablished." << LL_ENDL;
    mWebRTCAudioInterface = audio_interface;
    setVoiceConnectionState(VOICE_STATE_SESSION_ESTABLISHED);
}

void LLVoiceWebRTCConnection::OnRenegotiationNeeded()
{
    LL_INFOS("Voice") << "On Renegotiation Needed." << LL_ENDL;
    if (!mShutDown)
    {
        setVoiceConnectionState(VOICE_STATE_SESSION_RETRY);
    }
}

void LLVoiceWebRTCConnection::OnPeerShutDown()
{
    setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
    mOutstandingRequests--;
}

void LLVoiceWebRTCConnection::processIceUpdates()
{
    if (mShutDown || LLWebRTCVoiceClient::isShuttingDown())
    {
        return;
    }

    bool iceCompleted = false;
    LLSD body;
    {
        if (!mTrickling)
        {
            if (!mIceCandidates.empty() || mIceCompleted)
            {
                LLViewerRegion *regionp = LLWorld::instance().getRegionFromID(mRegionID);
                if (!regionp || !regionp->capabilitiesReceived())
                {
                LL_DEBUGS("Voice") << "no capabilities for ice gathering; waiting " << LL_ENDL;
                return;
                }

                std::string url = regionp->getCapability("VoiceSignalingRequest");
                if (url.empty())
                {
                return;
                }

                LL_DEBUGS("Voice") << "region ready to complete voice signaling; url=" << url << LL_ENDL;
                if (!mIceCandidates.empty())
                {
                LLSD candidates = LLSD::emptyArray();
                for (auto &ice_candidate : mIceCandidates)
                {
                        LLSD body_candidate;
                        body_candidate["sdpMid"]        = ice_candidate.sdp_mid;
                        body_candidate["sdpMLineIndex"] = ice_candidate.mline_index;
                        body_candidate["candidate"]     = ice_candidate.candidate;
                        candidates.append(body_candidate);
                }
                body["candidates"] = candidates;
                mIceCandidates.clear();
                }
                else if (mIceCompleted)
                {
                LLSD body_candidate;
                body_candidate["completed"] = true;
                body["candidate"]           = body_candidate;
                iceCompleted                = mIceCompleted;
                mIceCompleted               = false;
                }

                body["viewer_session"] = mViewerSession;

                LLCore::HttpRequest::policy_t               httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
                LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(
                    new LLCoreHttpUtil::HttpCoroutineAdapter("voiceAccountProvision", httpPolicy));
                LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
                LLCore::HttpOptions::ptr_t httpOpts = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions);
                LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpPost(
                    url,
                    LLCore::HttpRequest::DEFAULT_POLICY_ID,
                    body,
                    boost::bind(&LLVoiceWebRTCSpatialConnection::onIceUpdateComplete, this, iceCompleted, _1),
                    boost::bind(&LLVoiceWebRTCSpatialConnection::onIceUpdateError, this, 3, url, body, iceCompleted, _1));
                mOutstandingRequests++;
                mTrickling = true;
            }
        }
    }
}


void LLVoiceWebRTCConnection::setMuteMic(bool muted)
{
    mMuted = muted;
    if (mWebRTCAudioInterface)
    {
        mWebRTCAudioInterface->setMute(muted);
    }
}

void LLVoiceWebRTCConnection::setMicGain(F32 gain)
{
    mMicGain = gain;
    if (mWebRTCAudioInterface)
    {
        mWebRTCAudioInterface->setSendVolume(gain);
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

void LLVoiceWebRTCConnection::sendData(const std::string &data)
{
    if (getVoiceConnectionState() == VOICE_STATE_SESSION_UP && mWebRTCDataInterface)
    {
        mWebRTCDataInterface->sendData(data, false);
    }
}

bool LLVoiceWebRTCConnection::breakVoiceConnection(bool corowait)
{
    LL_INFOS("Voice") << "Disconnecting voice." << LL_ENDL;
    if (mWebRTCDataInterface)
    {
        mWebRTCDataInterface->unsetDataObserver(this);
        mWebRTCDataInterface = nullptr;
    }
    mWebRTCAudioInterface   = nullptr;
    LLViewerRegion *regionp = LLWorld::instance().getRegionFromID(mRegionID);
    if (!regionp || !regionp->capabilitiesReceived())
    {
        LL_DEBUGS("Voice") << "no capabilities for voice provisioning; waiting " << LL_ENDL;
        setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
        return false;
    }

    std::string url = regionp->getCapability("ProvisionVoiceAccountRequest");
    if (url.empty())
    {
        return false;
    }

    LL_DEBUGS("Voice") << "region ready for voice break; url=" << url << LL_ENDL;

    LLCore::HttpRequest::policy_t               httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("parcelVoiceInfoRequest", httpPolicy));
    LLCore::HttpRequest::ptr_t                  httpRequest(new LLCore::HttpRequest);

    LLVoiceWebRTCStats::getInstance()->provisionAttemptStart();
    LLSD body;
    body["logout"]         = TRUE;
    body["viewer_session"] = mViewerSession;

    LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpPost(
        url,
        LLCore::HttpRequest::DEFAULT_POLICY_ID,
        body,
        boost::bind(&LLVoiceWebRTCSpatialConnection::OnVoiceDisconnectionRequestSuccess, this, _1),
        boost::bind(&LLVoiceWebRTCSpatialConnection::OnVoiceDisconnectionRequestFailure, this, url, 3, body, _1));
    setVoiceConnectionState(VOICE_STATE_WAIT_FOR_EXIT);
    mOutstandingRequests++;
    return true;
}

void LLVoiceWebRTCConnection::OnVoiceDisconnectionRequestSuccess(const LLSD &result)
{
    if (LLWebRTCVoiceClient::isShuttingDown())
    {
        return;
    }

    if (mWebRTCPeerConnection)
    {
        mOutstandingRequests++;
        mWebRTCPeerConnection->shutdownConnection();
    }
    else
    {
        setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
    }
    mOutstandingRequests--;
}

void LLVoiceWebRTCConnection::OnVoiceDisconnectionRequestFailure(std::string url, int retries, LLSD body, const LLSD &result)
{
    if (LLWebRTCVoiceClient::isShuttingDown())
    {
        return;
    }
    if (retries >= 0)
    {
        LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpPost(
            url,
            LLCore::HttpRequest::DEFAULT_POLICY_ID,
            body,
            boost::bind(&LLVoiceWebRTCSpatialConnection::OnVoiceDisconnectionRequestSuccess, this, _1),
            boost::bind(&LLVoiceWebRTCSpatialConnection::OnVoiceDisconnectionRequestFailure, this, url, retries - 1, body, _1));
        return;
    }
    if (mWebRTCPeerConnection)
    {
        mOutstandingRequests++;
        mWebRTCPeerConnection->shutdownConnection();
    }
    else
    {
        setVoiceConnectionState(VOICE_STATE_SESSION_EXIT);
    }
    mOutstandingRequests--;
}


void LLVoiceWebRTCConnection::OnVoiceConnectionRequestSuccess(const LLSD &result)
{
    if (LLWebRTCVoiceClient::isShuttingDown())
    {
        return;
    }
    LLVoiceWebRTCStats::getInstance()->provisionAttemptEnd(true);

    if (result.has("viewer_session") && result.has("jsep") && result["jsep"].has("type") && result["jsep"]["type"] == "answer" &&
        result["jsep"].has("sdp"))
    {
        mRemoteChannelSDP = result["jsep"]["sdp"].asString();
        mViewerSession    = result["viewer_session"];
    }
    else
    {
        setVoiceConnectionState(VOICE_STATE_SESSION_RETRY);
        mOutstandingRequests--;
        return;
    }

    LL_DEBUGS("Voice") << "ProvisionVoiceAccountRequest response"
                       << " channel sdp " << mRemoteChannelSDP << LL_ENDL;

    mWebRTCPeerConnection->AnswerAvailable(mRemoteChannelSDP);
    mOutstandingRequests--;
}

void LLVoiceWebRTCConnection::OnVoiceConnectionRequestFailure(std::string url, int retries, LLSD body, const LLSD &result)
{
    if (LLWebRTCVoiceClient::isShuttingDown())
    {
        return;
    }
    if (retries >= 0)
    {
        LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpPost(
            url,
            LLCore::HttpRequest::DEFAULT_POLICY_ID,
            body,
            boost::bind(&LLVoiceWebRTCConnection::OnVoiceConnectionRequestSuccess, this, _1),
            boost::bind(&LLVoiceWebRTCConnection::OnVoiceConnectionRequestFailure, this, url, retries - 1, body, _1));
        return;
    }
    LL_WARNS("Voice") << "Unable to connect voice." << body << " RESULT: " << result << LL_ENDL;
    setVoiceConnectionState(VOICE_STATE_SESSION_RETRY);
    mOutstandingRequests--;
}

bool LLVoiceWebRTCConnection::connectionStateMachine()
{
    processIceUpdates();

    switch (getVoiceConnectionState())
    {
        case VOICE_STATE_START_SESSION:
        {
            if (mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_DISCONNECT);
                break;
            }
            mTrickling    = false;
            mIceCompleted = false;
            setVoiceConnectionState(VOICE_STATE_WAIT_FOR_SESSION_START);
            if (!mWebRTCPeerConnection->initializeConnection())
            {
                setVoiceConnectionState(VOICE_STATE_SESSION_RETRY);
            }
            break;
        }
        case VOICE_STATE_WAIT_FOR_SESSION_START:
        {
            if (mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_DISCONNECT);
            }
            break;
        }
        case VOICE_STATE_REQUEST_CONNECTION:
            if (mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_DISCONNECT);
                break;
            }
            if (!requestVoiceConnection())
            {
                setVoiceConnectionState(VOICE_STATE_SESSION_RETRY);
            }
            else
            {
                setVoiceConnectionState(VOICE_STATE_CONNECTION_WAIT);
            }
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
            mWebRTCAudioInterface->setMute(mMuted);
            mWebRTCAudioInterface->setReceiveVolume(mSpeakerVolume);
            mWebRTCAudioInterface->setSendVolume(mMicGain);
            LLWebRTCVoiceClient::getInstance()->OnConnectionEstablished(mChannelID, mRegionID);
            setVoiceConnectionState(VOICE_STATE_SESSION_UP);
        }
        break;
        case VOICE_STATE_SESSION_UP:
        {
            if (mShutDown)
            {
                setVoiceConnectionState(VOICE_STATE_DISCONNECT);
            }
            break;
        }

        case VOICE_STATE_SESSION_RETRY:
            LLWebRTCVoiceClient::getInstance()->OnConnectionFailure(mChannelID, mRegionID);
            setVoiceConnectionState(VOICE_STATE_DISCONNECT);
            break;

        case VOICE_STATE_DISCONNECT:
            breakVoiceConnection(true);
            break;

        case VOICE_STATE_WAIT_FOR_EXIT:
            break;
        case VOICE_STATE_SESSION_EXIT:
        {
            {
                LLMutexLock lock(&mVoiceStateMutex);
                if (!mShutDown)
                {
                    mVoiceConnectionState = VOICE_STATE_START_SESSION;
                }
                else
                {
                    if (mOutstandingRequests <= 0)
                    {
                        LLWebRTCVoiceClient::getInstance()->OnConnectionShutDown(mChannelID, mRegionID);
                        return false;
                    }
                }
            }
            break;
        }

        default:
        {
            LL_WARNS("Voice") << "Unknown voice control state " << getVoiceConnectionState() << LL_ENDL;
            return false;
        }
    }
    return true;
}

void LLVoiceWebRTCConnection::OnDataReceived(const std::string &data, bool binary)
{
    // incoming data will be a json structure (if it's not binary.)  We may pack
    // binary for size reasons.  Most of the keys in the json objects are
    // single or double characters for size reasons.
    // The primary element is:
    // An object where each key is an agent id.  (in the future, we may allow
    // integer indices into an agentid list, populated on join commands.  For size.
    // Each key will point to a json object with keys identifying what's updated.
    // 'p'  - audio source power (level/volume) (int8 as int)
    // 'j'  - join - object of join data (TBD) (true for now)
    // 'l'  - boolean, always true if exists.

    if (binary)
    {
        LL_WARNS("Voice") << "Binary data received from data channel." << LL_ENDL;
        return;
    }

    Json::Reader reader;
    Json::Value  voice_data;
    if (reader.parse(data, voice_data, false))  // don't collect comments
    {
        if (!voice_data.isObject())
        {
            LL_WARNS("Voice") << "Expected object from data channel:" << data << LL_ENDL;
            return;
        }
        bool new_participant = false;
        for (auto &participant_id : voice_data.getMemberNames())
        {
            LLUUID agent_id(participant_id);
            if (agent_id.isNull())
            {
                LL_WARNS("Voice") << "Bad participant ID from data channel (" << participant_id << "):" << data << LL_ENDL;
                continue;
            }

            LLWebRTCVoiceClient::participantStatePtr_t participant =
                LLWebRTCVoiceClient::getInstance()->findParticipantByID(mChannelID, agent_id);
            bool joined  = false;
            bool primary = false;
            if (voice_data[participant_id].isMember("j"))
            {
                joined  = true;
                primary = voice_data[participant_id]["j"].get("p", Json::Value(false)).asBool();
            }

            new_participant |= joined;
            if (!participant && joined && primary)
            {
                participant = LLWebRTCVoiceClient::getInstance()->addParticipantByID(mChannelID, agent_id);
            }
            if (participant)
            {
                if (voice_data[participant_id].get("l", Json::Value(false)).asBool())
                {
                if (agent_id != gAgentID)
                {
                        LLWebRTCVoiceClient::getInstance()->removeParticipantByID(mChannelID, agent_id);
                }
                }
                else
                {
                F32 level = (F32) (voice_data[participant_id].get("p", Json::Value(participant->mLevel)).asInt()) / 128;
                // convert to decibles
                participant->mLevel = level;
                /* WebRTC appears to have deprecated VAD, but it's still in the Audio Processing Module so maybe we
                   can use it at some point when we actually process frames. */
                participant->mIsSpeaking = participant->mLevel > SPEAKING_AUDIO_LEVEL;
                }
            }
        }
    }
}

void LLVoiceWebRTCConnection::OnDataChannelReady(llwebrtc::LLWebRTCDataInterface *data_interface)
{
    if (data_interface)
    {
        mWebRTCDataInterface = data_interface;
        mWebRTCDataInterface->setDataObserver(this);

        Json::FastWriter writer;
        Json::Value      root     = Json::objectValue;
        Json::Value      join_obj = Json::objectValue;
        LLUUID           regionID = gAgent.getRegion()->getRegionID();
        if (regionID == mRegionID)
        {
            join_obj["p"] = true;
        }
        root["j"]             = join_obj;
        std::string json_data = writer.write(root);
        mWebRTCDataInterface->sendData(json_data, false);
    }
}

/////////////////////////////
// WebRTC Spatial Connection

LLVoiceWebRTCSpatialConnection::LLVoiceWebRTCSpatialConnection(const LLUUID &regionID, S32 parcelLocalID, const std::string &channelID) :
    LLVoiceWebRTCConnection(regionID, channelID),
    mParcelLocalID(parcelLocalID)
{
}

LLVoiceWebRTCSpatialConnection::~LLVoiceWebRTCSpatialConnection() 
{
    if (LLWebRTCVoiceClient::isShuttingDown())
    {
        // peer connection and observers will be cleaned up
        // by llwebrtc::terminate() on shutdown.
        return;
    }
    assert(mOutstandingRequests == 0);
    mWebRTCPeerConnection->unsetSignalingObserver(this);
}


bool LLVoiceWebRTCSpatialConnection::requestVoiceConnection()
{
    LLViewerRegion *regionp = LLWorld::instance().getRegionFromID(mRegionID);

    LL_INFOS("Voice") << "Requesting voice connection." << LL_ENDL;
    if (!regionp || !regionp->capabilitiesReceived())
    {
        LL_DEBUGS("Voice") << "no capabilities for voice provisioning; waiting " << LL_ENDL;
        return false;
    }

    std::string url = regionp->getCapability("ProvisionVoiceAccountRequest");
    if (url.empty())
    {
        return false;
    }

    LL_DEBUGS("Voice") << "region ready for voice provisioning; url=" << url << LL_ENDL;

    LLVoiceWebRTCStats::getInstance()->provisionAttemptStart();
    LLSD body;
    LLSD jsep;
    jsep["type"] = "offer";
    {
        LLMutexLock lock(&mVoiceStateMutex);
        jsep["sdp"] = mChannelSDP;
    }
    body["jsep"] = jsep;
    if (mParcelLocalID != INVALID_PARCEL_ID)
    {
        body["parcel_local_id"] = mParcelLocalID;
    }

    LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpPost(
        url,
        LLCore::HttpRequest::DEFAULT_POLICY_ID,
        body,
        boost::bind(&LLVoiceWebRTCSpatialConnection::OnVoiceConnectionRequestSuccess, this, _1),
        boost::bind(&LLVoiceWebRTCSpatialConnection::OnVoiceConnectionRequestFailure, this, url, 3, body, _1));
    mOutstandingRequests++;
    return true;
}

void LLVoiceWebRTCSpatialConnection::setMuteMic(bool muted)
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
            // always mute to regions the agent isn't on, to prevent echo.
            mWebRTCAudioInterface->setMute(true);
        }
    }
}

/////////////////////////////
// WebRTC Spatial Connection

LLVoiceWebRTCAdHocConnection::LLVoiceWebRTCAdHocConnection(const LLUUID &regionID, const std::string& channelID, const std::string& credentials) :
    LLVoiceWebRTCConnection(regionID, channelID),
    mCredentials(credentials)
{
}

LLVoiceWebRTCAdHocConnection::~LLVoiceWebRTCAdHocConnection()
{
    if (LLWebRTCVoiceClient::isShuttingDown())
    {
        // peer connection and observers will be cleaned up
        // by llwebrtc::terminate() on shutdown.
        return;
    }
    assert(mOutstandingRequests == 0);
    mWebRTCPeerConnection->unsetSignalingObserver(this);
}


bool LLVoiceWebRTCAdHocConnection::requestVoiceConnection()
{
    LLViewerRegion *regionp = LLWorld::instance().getRegionFromID(mRegionID);

    LL_INFOS("Voice") << "Requesting voice connection." << LL_ENDL;
    if (!regionp || !regionp->capabilitiesReceived())
    {
        LL_DEBUGS("Voice") << "no capabilities for voice provisioning; waiting " << LL_ENDL;
        return false;
    }

    std::string url = regionp->getCapability("ProvisionVoiceAccountRequest");
    if (url.empty())
    {
        return false;
    }

    LL_DEBUGS("Voice") << "region ready for voice provisioning; url=" << url << LL_ENDL;

    LLVoiceWebRTCStats::getInstance()->provisionAttemptStart();
    LLSD body;
    LLSD jsep;
    jsep["type"] = "offer";
    {
        LLMutexLock lock(&mVoiceStateMutex);
        jsep["sdp"] = mChannelSDP;
    }
    body["jsep"] = jsep;
    body["credentials"] = mCredentials;
    body["channel"] = mChannelID;
    body["channel_type"] = "multiagent";

    LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpPost(
        url,
        LLCore::HttpRequest::DEFAULT_POLICY_ID,
        body,
        boost::bind(&LLVoiceWebRTCAdHocConnection::OnVoiceConnectionRequestSuccess, this, _1),
        boost::bind(&LLVoiceWebRTCAdHocConnection::OnVoiceConnectionRequestFailure, this, url, 3, body, _1));
    mOutstandingRequests++;
    return true;
}
