 /** 
 * @file LLWebRTCVoiceClient.cpp
 * @brief Implementation of LLWebRTCVoiceClient class which is the interface to the voice client process.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
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
    const F32 VOLUME_SCALE_WEBRTC = 0.01f;

    const F32 SPEAKING_TIMEOUT = 1.f;
    const F32 SPEAKING_AUDIO_LEVEL = 0.05;

    static const std::string VOICE_SERVER_TYPE = "WebRTC";

    // Don't send positional updates more frequently than this:
    const F32 UPDATE_THROTTLE_SECONDS = 0.1f;

    // Timeout for connection to WebRTC 
    const F32 CONNECT_ATTEMPT_TIMEOUT = 300.0f;
    const F32 CONNECT_DNS_TIMEOUT = 5.0f;

    const F32 LOGOUT_ATTEMPT_TIMEOUT = 5.0f;
    
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
}

static int scale_mic_volume(float volume)
{
	// incoming volume has the range [0.0 ... 2.0], with 1.0 as the default.                                                
	// Map it to WebRTC levels as follows: 0.0 -> 30, 1.0 -> 50, 2.0 -> 70                                                   
	return 30 + (int)(volume * 20.0f);
}


///////////////////////////////////////////////////////////////////////////////////////////////

class LLWebRTCVoiceClientMuteListObserver : public LLMuteListObserver
{
	/* virtual */ void onChange()  { LLWebRTCVoiceClient::getInstance()->muteListChanged();}
};


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

static LLWebRTCVoiceClientMuteListObserver mutelist_listener;
static bool sMuteListListener_listening = false;


///////////////////////////////////////////////////////////////////////////////////////////////

bool LLWebRTCVoiceClient::sShuttingDown = false;
bool LLWebRTCVoiceClient::sConnected = false;
LLPumpIO *LLWebRTCVoiceClient::sPump = nullptr;

LLWebRTCVoiceClient::LLWebRTCVoiceClient() :
    mSessionTerminateRequested(false),
    mRelogRequested(false),
    mTerminateDaemon(false),
    mSpatialJoiningNum(0),

    mTuningMode(false),
    mTuningEnergy(0.0f),
    mTuningMicVolume(0),
    mTuningMicVolumeDirty(true),
    mTuningSpeakerVolume(50),  // Set to 50 so the user can hear himself when he sets his mic volume
    mTuningSpeakerVolumeDirty(true),
    mDevicesListUpdated(false),

    mAreaVoiceDisabled(false),
    mAudioSession(),  // TBD - should be NULL
    mAudioSessionChanged(false),
    mNextAudioSession(),

    mCurrentParcelLocalID(0),
    mConnectorEstablished(false),
    mAccountLoggedIn(false),
    mNumberOfAliases(0),
    mCommandCookie(0),
    mLoginRetryCount(0),

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
    mSpeakerMuteDirty(true),
    mMicVolume(0),
    mMicVolumeDirty(true),

    mVoiceEnabled(false),
    mWriteInProgress(false),

    mLipSyncEnabled(false),

    mVoiceFontsReceived(false),
    mVoiceFontsNew(false),
    mVoiceFontListDirty(false),

    mCaptureBufferMode(false),
    mCaptureBufferRecording(false),
    mCaptureBufferRecorded(false),
    mCaptureBufferPlaying(false),
    mShutdownComplete(true),
    mPlayRequestCount(0),

    mAvatarNameCacheConnection(),
    mIsInTuningMode(false),
    mIsInChannel(false),
    mIsJoiningSession(false),
    mIsWaitingForFonts(false),
    mIsLoggingIn(false),
    mIsLoggedIn(false),
    mIsProcessingChannels(false),
    mIsCoroutineActive(false),
    mWebRTCPump("WebRTCClientPump"),
    mWebRTCDeviceInterface(nullptr),
    mWebRTCSignalingInterface(nullptr),
    mWebRTCAudioInterface(nullptr)
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

void LLWebRTCVoiceClient::init(LLPumpIO *pump)
{
	// constructor will set up LLVoiceClient::getInstance()
	sPump = pump;

//     LLCoros::instance().launch("LLWebRTCVoiceClient::voiceControlCoro",
//         boost::bind(&LLWebRTCVoiceClient::voiceControlCoro, LLWebRTCVoiceClient::getInstance()));
    llwebrtc::init();

	mWebRTCDeviceInterface = llwebrtc::getDeviceInterface();
    mWebRTCDeviceInterface->setDevicesObserver(this);

	mWebRTCSignalingInterface = llwebrtc::getSignalingInterface();
    mWebRTCSignalingInterface->setSignalingObserver(this);
    
    mWebRTCDataInterface = llwebrtc::getDataInterface();
    mWebRTCDataInterface->setDataObserver(this);
}

void LLWebRTCVoiceClient::terminate()
{
    if (sShuttingDown)
    {
        return;
    }
    
	mRelogRequested = false;
    mVoiceEnabled   = false;
    llwebrtc::terminate();

    sShuttingDown = true;
    sPump = NULL;
}

//---------------------------------------------------

void LLWebRTCVoiceClient::cleanUp()
{
    LL_DEBUGS("Voice") << LL_ENDL;
    
	deleteAllSessions();
	deleteAllVoiceFonts();
	deleteVoiceFontTemplates();
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
// utility functions

bool LLWebRTCVoiceClient::writeString(const std::string &str)
{
	bool result = false;
    LL_DEBUGS("LowVoice") << "sending:\n" << str << LL_ENDL;

	if(sConnected)
	{
		apr_status_t err;
		apr_size_t size = (apr_size_t)str.size();
		apr_size_t written = size;
	
		//MARK: Turn this on to log outgoing XML
        // LL_DEBUGS("Voice") << "sending: " << str << LL_ENDL;

		// check return code - sockets will fail (broken, etc.)
		err = apr_socket_send(
				mSocket->getSocket(),
				(const char*)str.data(),
				&written);
		
		if(err == 0 && written == size)
		{
			// Success.
			result = true;
		}
		else if (err == 0 && written != size) {
			// Did a short write,  log it for now
			LL_WARNS("Voice") << ") short write on socket sending data to WebRTC daemon." << "Sent " << written << "bytes instead of " << size <<LL_ENDL;
		}
		else if(APR_STATUS_IS_EAGAIN(err))
		{
			char buf[MAX_STRING];
			LL_WARNS("Voice") << "EAGAIN error " << err << " (" << apr_strerror(err, buf, MAX_STRING) << ") sending data to WebRTC daemon." << LL_ENDL;
		}
		else
		{
			// Assume any socket error means something bad.  For now, just close the socket.
			char buf[MAX_STRING];
			LL_WARNS("Voice") << "apr error " << err << " ("<< apr_strerror(err, buf, MAX_STRING) << ") sending data to WebRTC daemon." << LL_ENDL;
			daemonDied();
		}
	}
		
	return result;
}


/////////////////////////////
// session control messages
void LLWebRTCVoiceClient::connectorCreate()
{

}

void LLWebRTCVoiceClient::connectorShutdown()
{

	mShutdownComplete = true;
}

void LLWebRTCVoiceClient::userAuthorized(const std::string& user_id, const LLUUID &agentID)
{

	mAccountDisplayName = user_id;

	LL_INFOS("Voice") << "name \"" << mAccountDisplayName << "\" , ID " << agentID << LL_ENDL;

	mAccountName = nameFromID(agentID);
}

void LLWebRTCVoiceClient::setLoginInfo(
	const std::string& account_name,
	const std::string& password,
	const std::string& channel_sdp)
{
	mRemoteChannelSDP = channel_sdp;
    mWebRTCSignalingInterface->AnswerAvailable(channel_sdp);

	if(mAccountLoggedIn)
	{
		// Already logged in.
		LL_WARNS("Voice") << "Called while already logged in." << LL_ENDL;
		
		// Don't process another login.
		return;
	}
	else if ( account_name != mAccountName )
	{
		LL_WARNS("Voice") << "Mismatched account name! " << account_name
                          << " instead of " << mAccountName << LL_ENDL;
	}
	else
	{
		mAccountPassword = password;
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

typedef enum e_voice_control_coro_state
{
    VOICE_STATE_ERROR = -1,
    VOICE_STATE_TP_WAIT = 0, // entry point
    VOICE_STATE_START_DAEMON,
    VOICE_STATE_PROVISION_ACCOUNT,
    VOICE_STATE_SESSION_PROVISION_WAIT,
    VOICE_STATE_START_SESSION,
	VOICE_STATE_WAIT_FOR_SESSION_START,
    VOICE_STATE_SESSION_RETRY,
    VOICE_STATE_SESSION_ESTABLISHED,
    VOICE_STATE_WAIT_FOR_CHANNEL,
    VOICE_STATE_DISCONNECT,
    VOICE_STATE_WAIT_FOR_EXIT,
} EVoiceControlCoroState;

void LLWebRTCVoiceClient::voiceControlCoro()
{
    int state = 0;
    try
    {
        // state is passed as a reference instead of being
        // a member due to unresolved issues with coroutine
        // surviving longer than LLWebRTCVoiceClient
        voiceControlStateMachine();
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
        LL_WARNS("Voice") << "voiceControlStateMachine crashed in state " << state << LL_ENDL;
        throw;
    }
}

void LLWebRTCVoiceClient::voiceControlStateMachine()
{
    if (sShuttingDown)
    {
        return;
    }

    LL_DEBUGS("Voice") << "starting" << LL_ENDL;
    mIsCoroutineActive = true;
    LLCoros::set_consuming(true);

    U32 retry = 0;

    setVoiceControlStateUnless(VOICE_STATE_TP_WAIT);

    do
    {
        if (sShuttingDown)
        {
            // WebRTC singleton performed the exit, logged out,
            // cleaned sockets, gateway and no longer cares
            // about state of coroutine, so just stop
            return;
        }

		processIceUpdates();

        switch (getVoiceControlState())
        {
            case VOICE_STATE_TP_WAIT:
                // starting point for voice
                if (gAgent.getTeleportState() != LLAgent::TELEPORT_NONE)
                {
                    LL_DEBUGS("Voice") << "Suspending voiceControlCoro() momentarily for teleport. Tuning: " << mTuningMode
                                       << ". Relog: " << mRelogRequested << LL_ENDL;
                    llcoro::suspendUntilTimeout(1.0);
                }
                else
                {
                    setVoiceControlStateUnless(VOICE_STATE_START_SESSION, VOICE_STATE_SESSION_RETRY);
                }
                break;

            case VOICE_STATE_START_SESSION:
                if (establishVoiceConnection() && getVoiceControlState() != VOICE_STATE_SESSION_RETRY)
                {
                    setVoiceControlStateUnless(VOICE_STATE_WAIT_FOR_SESSION_START, VOICE_STATE_SESSION_RETRY);
                }
                else
                {
                    setVoiceControlStateUnless(VOICE_STATE_SESSION_RETRY);
                }
                break;

            case VOICE_STATE_WAIT_FOR_SESSION_START:
            {
                llcoro::suspendUntilTimeout(1.0);
                std::string channel_sdp;
                {
                    LLMutexLock lock(&mVoiceStateMutex);
                    if (mVoiceControlState == VOICE_STATE_SESSION_RETRY)
                    {
                        break;
                    }
                    if (!mChannelSDP.empty())
                    {
                        mVoiceControlState = VOICE_STATE_PROVISION_ACCOUNT;
                    }
                }
                break;
            }
            case VOICE_STATE_PROVISION_ACCOUNT:
                if (!provisionVoiceAccount())
                {
                    setVoiceControlStateUnless(VOICE_STATE_SESSION_RETRY);
                }
                else
                {
                    setVoiceControlStateUnless(VOICE_STATE_SESSION_PROVISION_WAIT, VOICE_STATE_SESSION_RETRY);
                }
                break;
            case VOICE_STATE_SESSION_PROVISION_WAIT:
                llcoro::suspendUntilTimeout(1.0);
                break;

            case VOICE_STATE_SESSION_RETRY:
                giveUp();  // cleans sockets and session
                if (mRelogRequested)
                {
                    // We failed to connect, give it a bit time before retrying.
                    retry++;
                    F32 full_delay    = llmin(5.f * (F32) retry, 60.f);
                    F32 current_delay = 0.f;
                    LL_INFOS("Voice") << "Voice failed to establish session after " << retry << " tries. Will attempt to reconnect in "
                                      << full_delay << " seconds" << LL_ENDL;
                    while (current_delay < full_delay && !sShuttingDown)
                    {
                        // Assuming that a second has passed is not accurate,
                        // but we don't need accurancy here, just to make sure
                        // that some time passed and not to outlive voice itself
                        current_delay++;
                        llcoro::suspendUntilTimeout(1.f);
                    }
                }
                setVoiceControlStateUnless(VOICE_STATE_WAIT_FOR_EXIT);
                break;

            case VOICE_STATE_SESSION_ESTABLISHED:
            {
                if (mTuningMode)
                {
                    performMicTuning();
                }
                sessionEstablished();
                setVoiceControlStateUnless(VOICE_STATE_WAIT_FOR_CHANNEL, VOICE_STATE_SESSION_RETRY);
            }
            break;

            case VOICE_STATE_WAIT_FOR_CHANNEL:
            {
                if ((!waitForChannel()) || !mVoiceEnabled)  // todo: split into more states like login/fonts
                {
                    setVoiceControlStateUnless(VOICE_STATE_DISCONNECT, VOICE_STATE_SESSION_RETRY);
                }
                // on true, it's a retry, so let the state stand.
            }
            break;

            case VOICE_STATE_DISCONNECT:
                LL_DEBUGS("Voice") << "lost channel RelogRequested=" << mRelogRequested << LL_ENDL;
                breakVoiceConnection(true);
                retry = 0;  // Connected without issues
                setVoiceControlStateUnless(VOICE_STATE_WAIT_FOR_EXIT);
                break;

            case VOICE_STATE_WAIT_FOR_EXIT:
                if (mVoiceEnabled)
                {
                    LL_INFOS("Voice") << "will attempt to reconnect to voice" << LL_ENDL;
                    setVoiceControlStateUnless(VOICE_STATE_TP_WAIT);
                }
                else
                {
                    llcoro::suspendUntilTimeout(1.0);
                }
                break;

            default:
            {
                LL_WARNS("Voice") << "Unknown voice control state " << getVoiceControlState() << LL_ENDL;
                break;
            }
        }
    } while (true);
}

bool LLWebRTCVoiceClient::callbackEndDaemon(const LLSD& data)
{
    if (!sShuttingDown && mVoiceEnabled)
    {
        LL_WARNS("Voice") << "SLVoice terminated " << ll_stream_notation_sd(data) << LL_ENDL;
        terminateAudioSession(false);
        closeSocket();
        cleanUp();
        LLVoiceClient::getInstance()->setUserPTTState(false);
        gAgent.setVoiceConnected(false);
        mRelogRequested = true;
    }
    return false;
}

bool LLWebRTCVoiceClient::provisionVoiceAccount()
{
    LL_INFOS("Voice") << "Provisioning voice account." << LL_ENDL;

    while ((!gAgent.getRegion() || !gAgent.getRegion()->capabilitiesReceived()) && !sShuttingDown)
    {
        LL_DEBUGS("Voice") << "no capabilities for voice provisioning; waiting " << LL_ENDL;
        // *TODO* Pump a message for wake up.
        llcoro::suspend();
    }

    if (sShuttingDown)
    {
        return false;
    }

    std::string url = gAgent.getRegionCapability("ProvisionVoiceAccountRequest");

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

    LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpPost(
							      url,	
                                  LLCore::HttpRequest::DEFAULT_POLICY_ID,
                                  body,
                                  boost::bind(&LLWebRTCVoiceClient::OnVoiceAccountProvisioned, this, _1),
								  boost::bind(&LLWebRTCVoiceClient::OnVoiceAccountProvisionFailure, this, url, 3, body, _1));
    return true;
}

void LLWebRTCVoiceClient::OnVoiceAccountProvisioned(const LLSD& result)
{
    LLVoiceWebRTCStats::getInstance()->provisionAttemptEnd(true);
    std::string channelSDP;
    if (result.has("jsep") && 
		result["jsep"].has("type") && 
		result["jsep"]["type"] == "answer" && 
		result["jsep"].has("sdp"))
    {
        channelSDP = result["jsep"]["sdp"].asString();
    }
    std::string voiceAccountServerUri;
    std::string voiceUserName = gAgent.getID().asString();
    std::string voicePassword = "";  // no password for now.

    LL_DEBUGS("Voice") << "ProvisionVoiceAccountRequest response"
                       << " user " << (voiceUserName.empty() ? "not set" : "set") << " password "
                       << (voicePassword.empty() ? "not set" : "set") << " channel sdp " << channelSDP << LL_ENDL;

    setLoginInfo(voiceUserName, voicePassword, channelSDP);
}

void LLWebRTCVoiceClient::OnVoiceAccountProvisionFailure(std::string url, int retries, LLSD body, const LLSD& result)
{
    if (sShuttingDown)
	{
		return;
	}
    if (retries >= 0)
    {

		LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpPost(
									  url,
                                      LLCore::HttpRequest::DEFAULT_POLICY_ID,
                                      body,
                                      boost::bind(&LLWebRTCVoiceClient::OnVoiceAccountProvisioned, this, _1),
									  boost::bind(&LLWebRTCVoiceClient::OnVoiceAccountProvisionFailure, this, url, retries - 1, body, _1));
    }
    else
    {
        LL_WARNS("Voice") << "Unable to complete ice trickling voice account, retrying." << LL_ENDL;
    }
}

bool LLWebRTCVoiceClient::establishVoiceConnection()
{
    LL_INFOS("Voice") << "Ice Gathering voice account." << LL_ENDL;
    while ((!gAgent.getRegion() || !gAgent.getRegion()->capabilitiesReceived()) && !sShuttingDown)
    {
        LL_DEBUGS("Voice") << "no capabilities for voice provisioning; waiting " << LL_ENDL;
        // *TODO* Pump a message for wake up.
        llcoro::suspend();
        return false;
    }

    if (!mVoiceEnabled && mIsInitialized)
    {
        LL_WARNS("Voice") << "cannot establish connection; enabled "<<mVoiceEnabled<<" initialized "<<mIsInitialized<<LL_ENDL;
        return false;
    }

    if (sShuttingDown)
    {
        return false;
    }
    return mWebRTCSignalingInterface->initializeConnection();
}

bool LLWebRTCVoiceClient::breakVoiceConnection(bool corowait)
{

	LL_INFOS("Voice") << "Breaking voice account." << LL_ENDL;

    while ((!gAgent.getRegion() || !gAgent.getRegion()->capabilitiesReceived()) && !sShuttingDown)
    {
		LL_DEBUGS("Voice") << "no capabilities for voice breaking; waiting " << LL_ENDL;
		// *TODO* Pump a message for wake up.
		llcoro::suspend();
    }

    if (sShuttingDown)
    {
		return false;
    }

    std::string url = gAgent.getRegionCapability("ProvisionVoiceAccountRequest");

    LL_DEBUGS("Voice") << "region ready for voice break; url=" << url << LL_ENDL;

    LL_DEBUGS("Voice") << "sending ProvisionVoiceAccountRequest (breaking) (" << mCurrentRegionName << ", " << mCurrentParcelLocalID << ")" << LL_ENDL;

    LLCore::HttpRequest::policy_t               httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("parcelVoiceInfoRequest", httpPolicy));
    LLCore::HttpRequest::ptr_t                  httpRequest(new LLCore::HttpRequest);

    LLVoiceWebRTCStats::getInstance()->provisionAttemptStart();
    LLSD body;
    body["logout"] = TRUE;
    httpAdapter->postAndSuspend(httpRequest, url, body);
    mWebRTCSignalingInterface->shutdownConnection();
    return true;
}

bool LLWebRTCVoiceClient::loginToWebRTC()
{
    mRelogRequested = false;
    mIsLoggedIn = true;
    notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LOGGED_IN);

    // Set up the mute list observer if it hasn't been set up already.
    if ((!sMuteListListener_listening))
    {
        LLMuteList::getInstance()->addObserver(&mutelist_listener);
        sMuteListListener_listening = true;
    }

    // Set the initial state of mic mute, local speaker volume, etc.
    sendLocalAudioUpdates();
    mIsLoggingIn = false;

    return true;
}

void LLWebRTCVoiceClient::logoutOfWebRTC(bool wait)
{
    if (mIsLoggedIn)
    {
        mAccountPassword.clear();
        breakVoiceConnection(wait);
        // Ensure that we'll re-request provisioning before logging in again
        mIsLoggedIn = false;
    }
}

bool LLWebRTCVoiceClient::requestParcelVoiceInfo()
{
    //_INFOS("Voice") << "Requesting voice info for Parcel" << LL_ENDL;

    LLViewerRegion * region = gAgent.getRegion();
    if (region == NULL || !region->capabilitiesReceived())
    {
        LL_DEBUGS("Voice") << "ParcelVoiceInfoRequest capability not yet available, deferring" << LL_ENDL;
        return false;
    }

    // grab the cap.
    std::string url = gAgent.getRegion()->getCapability("ParcelVoiceInfoRequest");
    if (url.empty())
    {
        // Region dosn't have the cap. Stop probing.
        LL_DEBUGS("Voice") << "ParcelVoiceInfoRequest capability not available in this region" << LL_ENDL;
        return false;
    }
 
    // update the parcel
    checkParcelChanged(true);

    LL_DEBUGS("Voice") << "sending ParcelVoiceInfoRequest (" << mCurrentRegionName << ", " << mCurrentParcelLocalID << ")" << LL_ENDL;

    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("parcelVoiceInfoRequest", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);

    LLSD result = httpAdapter->postAndSuspend(httpRequest, url, LLSD());

    if (sShuttingDown)
    {
        return false;
    }

    LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
    LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

    if (mSessionTerminateRequested || (!mVoiceEnabled && mIsInitialized))
    {
        // if a terminate request has been received,
        // bail and go to the stateSessionTerminated
        // state.  If the cap request is still pending,
        // the responder will check to see if we've moved
        // to a new session and won't change any state.
        LL_DEBUGS("Voice") << "terminate requested " << mSessionTerminateRequested
                           << " enabled " << mVoiceEnabled
                           << " initialized " << mIsInitialized
                           << LL_ENDL;
        terminateAudioSession(true);
        return false;
    }

    if ((!status) || (mSessionTerminateRequested || (!mVoiceEnabled && mIsInitialized)))
    {
        if (mSessionTerminateRequested || (!mVoiceEnabled && mIsInitialized))
        {
            LL_WARNS("Voice") << "Session terminated." << LL_ENDL;
        }

        LL_WARNS("Voice") << "No voice on parcel" << LL_ENDL;
        sessionTerminate();
        return false;
    }

    std::string uri;
    std::string credentials;

    if (result.has("voice_credentials"))
    {
        LLSD voice_credentials = result["voice_credentials"];
        if (voice_credentials.has("channel_uri"))
        {
            LL_DEBUGS("Voice") << "got voice channel uri" << LL_ENDL;
            uri = voice_credentials["channel_uri"].asString();
        }
        else
        {
            LL_WARNS("Voice") << "No voice channel uri" << LL_ENDL;
        }
        
        if (voice_credentials.has("channel_credentials"))
        {
            LL_DEBUGS("Voice") << "got voice channel credentials" << LL_ENDL;
            credentials =
                voice_credentials["channel_credentials"].asString();
        }
        else
        {
            LLVoiceChannel* channel = LLVoiceChannel::getCurrentVoiceChannel();
            if (channel != NULL)
            {
                if (channel->getSessionName().empty() && channel->getSessionID().isNull())
                {
                    if (LLViewerParcelMgr::getInstance()->allowAgentVoice())
                    {
                        LL_WARNS("Voice") << "No channel credentials for default channel" << LL_ENDL;
                    }
                }
                else
                {
                    LL_WARNS("Voice") << "No voice channel credentials" << LL_ENDL;
                }
            }
        }
    }
    else
    {
        if (LLViewerParcelMgr::getInstance()->allowAgentVoice())
        {
            LL_WARNS("Voice") << "No voice credentials" << LL_ENDL;
        }
        else
        {
            LL_DEBUGS("Voice") << "No voice credentials" << LL_ENDL;
        }
    }

    // set the spatial channel.  If no voice credentials or uri are 
    // available, then we simply drop out of voice spatially.
    return !setSpatialChannel(uri, credentials);
}

bool LLWebRTCVoiceClient::addAndJoinSession(const sessionStatePtr_t &nextSession)
{
    mIsJoiningSession = true;

    sessionStatePtr_t oldSession = mAudioSession;

    LL_INFOS("Voice") << "Adding or joining voice session " << nextSession->mHandle << LL_ENDL;

    mAudioSession = nextSession;
    mAudioSessionChanged = true;
    if (!mAudioSession || !mAudioSession->mReconnect)
    {
        mNextAudioSession.reset();
    }

    notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINING);

    llcoro::suspend();

    if (sShuttingDown)
    {
        return false;
    }

    LLSD result;

    if (mSpatialJoiningNum == MAX_NORMAL_JOINING_SPATIAL_NUM)
    {
        // Notify observers to let them know there is problem with voice
        notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_VOICE_DISABLED);
        LL_WARNS() << "There seems to be problem with connection to voice server. Disabling voice chat abilities." << LL_ENDL;
    }
    
    // Increase mSpatialJoiningNum only for spatial sessions- it's normal to reach this case for
    // example for p2p many times while waiting for response, so it can't be used to detect errors
    if (mAudioSession && mAudioSession->mIsSpatial)
    {
        mSpatialJoiningNum++;
    }

    if (!mVoiceEnabled && mIsInitialized)
    {
        LL_DEBUGS("Voice") << "Voice no longer enabled. Exiting"
                           << " enabled " << mVoiceEnabled
                           << " initialized " << mIsInitialized
                           << LL_ENDL;
        mIsJoiningSession = false;
        // User bailed out during connect -- jump straight to teardown.
        terminateAudioSession(true);
        notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_VOICE_DISABLED);
        return false;
    }
    else if (mSessionTerminateRequested)
    {
        LL_DEBUGS("Voice") << "Terminate requested" << LL_ENDL;
        if (mAudioSession && !mAudioSession->mHandle.empty())
        {
            // Only allow direct exits from this state in p2p calls (for cancelling an invite).
            // Terminating a half-connected session on other types of calls seems to break something in the WebRTC gateway.
            if (mAudioSession->mIsP2P)
            {
                terminateAudioSession(true);
                mIsJoiningSession = false;
                notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL);
                return false;
            }
        }
    }

    LLSD timeoutResult(LLSDMap("session", "timeout"));

    // We are about to start a whole new session.  Anything that MIGHT still be in our 
    // maildrop is going to be stale and cause us much wailing and gnashing of teeth.  
    // Just flush it all out and start new.
    mWebRTCPump.discard();

	// add 'self' participant.
    addParticipantByID(gAgent.getID());
    // tell peers that this participant has joined.

    Json::FastWriter writer;
    Json::Value      root = getPositionAndVolumeUpdateJson(true);
    root["j"]             = true;
    std::string json_data = writer.write(root);
    mWebRTCDataInterface->sendData(json_data, false);

    notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINED);

    return true;
}

bool LLWebRTCVoiceClient::terminateAudioSession(bool wait)
{

    if (mAudioSession)
    {
        LL_INFOS("Voice") << "terminateAudioSession(" << wait << ") Terminating current voice session " << mAudioSession->mHandle << LL_ENDL;

        if (mIsLoggedIn)
        {
            if (!mAudioSession->mHandle.empty())
            {

#if RECORD_EVERYTHING
                // HACK: for testing only
                // Save looped recording
                std::string savepath("/tmp/WebRTCrecording");
                {
                    time_t now = time(NULL);
                    const size_t BUF_SIZE = 64;
                    char time_str[BUF_SIZE];	/* Flawfinder: ignore */

                    strftime(time_str, BUF_SIZE, "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
                    savepath += time_str;
                }
                recordingLoopSave(savepath);
#endif

                sessionMediaDisconnectSendMessage(mAudioSession);

                if (wait)
                {
                    LLSD result;
                    do
                    {
                        LLSD timeoutResult(LLSDMap("session", "timeout"));

                        result = llcoro::suspendUntilEventOnWithTimeout(mWebRTCPump, LOGOUT_ATTEMPT_TIMEOUT, timeoutResult);

                        if (sShuttingDown)
                        {
                            return false;
                        }

                        LL_DEBUGS("Voice") << "event=" << ll_stream_notation_sd(result) << LL_ENDL;
                        if (result.has("session"))
                        {
                            if (result.has("handle"))
                            {
                                if (result["handle"] != mAudioSession->mHandle)
                                {
                                    LL_WARNS("Voice") << "Message for session handle \"" << result["handle"] << "\" while waiting for \"" << mAudioSession->mHandle << "\"." << LL_ENDL;
                                    continue;
                                }
                            }

                            std::string message = result["session"].asString();
                            if (message == "removed" || message == "timeout")
                                break;
                        }
                    } while (true);

                }
            }
            else
            {
                LL_WARNS("Voice") << "called with no session handle" << LL_ENDL;
            }
        }
        else
        {
            LL_WARNS("Voice") << "Session " << mAudioSession->mHandle << " already terminated by logout." << LL_ENDL;
        }

        sessionStatePtr_t oldSession = mAudioSession;

        mAudioSession.reset();
        // We just notified status observers about this change.  Don't do it again.
        mAudioSessionChanged = false;

        // The old session may now need to be deleted.
        reapSession(oldSession);
    }
    else
    {
        LL_WARNS("Voice") << "terminateAudioSession(" << wait << ") with NULL mAudioSession" << LL_ENDL;
    }

    notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL);

    // Always reset the terminate request flag when we get here.
    // Some slower PCs have a race condition where they can switch to an incoming  P2P call faster than the state machine leaves 
    // the region chat.
    mSessionTerminateRequested = false;

    bool status=((mVoiceEnabled || !mIsInitialized) && !mRelogRequested  && !sShuttingDown);
    LL_DEBUGS("Voice") << "exiting"
                       << " VoiceEnabled " << mVoiceEnabled
                       << " IsInitialized " << mIsInitialized
                       << " RelogRequested " << mRelogRequested
                       << " ShuttingDown " << (sShuttingDown ? "TRUE" : "FALSE")
                       << " returning " << status
                       << LL_ENDL;
    return status;
}


typedef enum e_voice_wait_for_channel_state
{
    VOICE_CHANNEL_STATE_LOGIN = 0, // entry point
    VOICE_CHANNEL_STATE_START_CHANNEL_PROCESSING,
    VOICE_CHANNEL_STATE_PROCESS_CHANNEL,
    VOICE_CHANNEL_STATE_NEXT_CHANNEL_DELAY,
    VOICE_CHANNEL_STATE_NEXT_CHANNEL_CHECK
} EVoiceWaitForChannelState;

bool LLWebRTCVoiceClient::waitForChannel()
{
    LL_INFOS("Voice") << "Waiting for channel" << LL_ENDL;

    EVoiceWaitForChannelState state = VOICE_CHANNEL_STATE_LOGIN;

    do 
    {
        if (sShuttingDown)
        {
            // terminate() forcefully disconects voice, no need for cleanup
            return false;
        }

		if (getVoiceControlState() == VOICE_STATE_SESSION_RETRY) 
		{
            mIsProcessingChannels = false;
            return true;
		}

		processIceUpdates();
        switch (state)
        {
        case VOICE_CHANNEL_STATE_LOGIN:
            if (!loginToWebRTC())
            {
                return false;
            }
            state = VOICE_CHANNEL_STATE_START_CHANNEL_PROCESSING;
            break;

        case VOICE_CHANNEL_STATE_START_CHANNEL_PROCESSING:
            mIsProcessingChannels = true;
            llcoro::suspend();
            state = VOICE_CHANNEL_STATE_PROCESS_CHANNEL;
            break;

        case VOICE_CHANNEL_STATE_PROCESS_CHANNEL:
            if (mTuningMode)
            {
                performMicTuning();
            }
            else if (mCaptureBufferMode)
            {
                recordingAndPlaybackMode();
            }
            else if (checkParcelChanged() || (mNextAudioSession == NULL))
            {
                // the parcel is changed, or we have no pending audio sessions,
                // so try to request the parcel voice info
                // if we have the cap, we move to the appropriate state
                requestParcelVoiceInfo(); //suspends for http reply
            }
            else if (sessionNeedsRelog(mNextAudioSession))
            {
                LL_INFOS("Voice") << "Session requesting reprovision and login." << LL_ENDL;
                requestRelog();
                break;
            }
            else if (mNextAudioSession)
            {
                sessionStatePtr_t joinSession = mNextAudioSession;
                mNextAudioSession.reset();
                if (!runSession(joinSession)) //suspends
                {
                    LL_DEBUGS("Voice") << "runSession returned false; leaving inner loop" << LL_ENDL;
                    break;
                }
                else
                {
                    LL_DEBUGS("Voice")
                        << "runSession returned true to inner loop"
                        << " RelogRequested=" << mRelogRequested
                        << " VoiceEnabled=" << mVoiceEnabled
                        << LL_ENDL;
                }
            }

            state = VOICE_CHANNEL_STATE_NEXT_CHANNEL_DELAY;
            break;

        case VOICE_CHANNEL_STATE_NEXT_CHANNEL_DELAY:
            if (!mNextAudioSession)
            {
                llcoro::suspendUntilTimeout(1.0);
            }
            state = VOICE_CHANNEL_STATE_NEXT_CHANNEL_CHECK;
            break;

        case VOICE_CHANNEL_STATE_NEXT_CHANNEL_CHECK:
            if (mVoiceEnabled && !mRelogRequested)
            {
                state = VOICE_CHANNEL_STATE_START_CHANNEL_PROCESSING;
                break;
            }
            else
            {
                mIsProcessingChannels = false;
                LL_DEBUGS("Voice")
                    << "leaving inner waitForChannel loop"
                    << " RelogRequested=" << mRelogRequested
                    << " VoiceEnabled=" << mVoiceEnabled
                    << LL_ENDL;
                return !sShuttingDown;
            }
        }
    } while (true);
}

bool LLWebRTCVoiceClient::runSession(const sessionStatePtr_t &session)
{
    LL_INFOS("Voice") << "running new voice session " << session->mHandle << LL_ENDL;

    bool joined_session = addAndJoinSession(session);

    if (sShuttingDown)
    {
        return false;
    }

    if (!joined_session)
    {
        notifyStatusObservers(LLVoiceClientStatusObserver::ERROR_UNKNOWN);

        if (mSessionTerminateRequested)
        {
            LL_DEBUGS("Voice") << "runSession terminate requested " << LL_ENDL;
            terminateAudioSession(true);
        }
        // if a relog has been requested then addAndJoineSession 
        // failed in a spectacular way and we need to back out.
        // If this is not the case then we were simply trying to
        // make a call and the other party rejected it.  
        return !mRelogRequested;
    }

    notifyParticipantObservers();
    notifyVoiceFontObservers();

    LLSD timeoutEvent(LLSDMap("timeout", LLSD::Boolean(true)));

    mIsInChannel = true;
    mMuteMicDirty = true;

    while (!sShuttingDown
           && mVoiceEnabled
           && !mSessionTerminateRequested
           && !mTuningMode)
    {

        if (sShuttingDown)
        {
            return false;
        }
        if (getVoiceControlState() == VOICE_STATE_SESSION_RETRY) 
		{
			break; 
		}

        if (mSessionTerminateRequested)
        {
            break;
        }

        if (mAudioSession && mAudioSession->mParticipantsChanged)
        {
            mAudioSession->mParticipantsChanged = false;
            notifyParticipantObservers();
        }
        
        if (!inSpatialChannel())
        {
            // When in a non-spatial channel, never send positional updates.
            mSpatialCoordsDirty = false;
        }
        else
        {
            updatePosition();

            if (checkParcelChanged())
            {
                // *RIDER: I think I can just return here if the parcel has changed 
                // and grab the new voice channel from the outside loop.
                // 
                // if the parcel has changed, attempted to request the
                // cap for the parcel voice info.  If we can't request it
                // then we don't have the cap URL so we do nothing and will
                // recheck next time around
                if (requestParcelVoiceInfo()) // suspends
                {   // The parcel voice URI has changed.. break out and reconnect.
                    break;
                }

                if (sShuttingDown)
                {
                    return false;
                }
            }
            // Do the calculation that enforces the listener<->speaker tether (and also updates the real camera position)
            enforceTether();
        }
        sendPositionAndVolumeUpdate();

        // Do notifications for expiring Voice Fonts.
        if (mVoiceFontExpiryTimer.hasExpired())
        {
            expireVoiceFonts();
            mVoiceFontExpiryTimer.setTimerExpirySec(VOICE_FONT_EXPIRY_INTERVAL);
        }

        // send any requests to adjust mic and speaker settings if they have changed
        sendLocalAudioUpdates();

        mIsInitialized = true;
        LLSD result = llcoro::suspendUntilEventOnWithTimeout(mWebRTCPump, UPDATE_THROTTLE_SECONDS, timeoutEvent);

        if (sShuttingDown)
        {
            return false;
        }

        if (!result.has("timeout")) // logging the timeout event spams the log
        {
            LL_DEBUGS("Voice") << "event=" << ll_stream_notation_sd(result) << LL_ENDL;
        }
        if (result.has("session"))
        {   
            if (result.has("handle"))
            {
                if (!mAudioSession)
                {
                    LL_WARNS("Voice") << "Message for session handle \"" << result["handle"] << "\" while session is not initiated." << LL_ENDL;
                    continue;
                }
                if (result["handle"] != mAudioSession->mHandle)
                {
                    LL_WARNS("Voice") << "Message for session handle \"" << result["handle"] << "\" while waiting for \"" << mAudioSession->mHandle << "\"." << LL_ENDL;
                    continue;
                }
            }

            std::string message = result["session"];

            if (message == "removed")
            {
                LL_DEBUGS("Voice") << "session removed" << LL_ENDL;
                notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL);
                break;
            }
        }
        else if (result.has("login"))
        {
            std::string message = result["login"];
            if (message == "account_logout")
            {
                LL_DEBUGS("Voice") << "logged out" << LL_ENDL;
                mIsLoggedIn = false;
                mRelogRequested = true;
                break;
            }
        }
    }

    if (sShuttingDown)
    {
        return false;
    }

    mIsInChannel = false;
    LL_DEBUGS("Voice") << "terminating at end of runSession" << LL_ENDL;
    terminateAudioSession(true);

    return true;
}

void LLWebRTCVoiceClient::recordingAndPlaybackMode()
{
    LL_INFOS("Voice") << "In voice capture/playback mode." << LL_ENDL;

    while (true)
    {
        LLSD command;
        do
        {
            command = llcoro::suspendUntilEventOn(mWebRTCPump);
            LL_DEBUGS("Voice") << "event=" << ll_stream_notation_sd(command) << LL_ENDL;
        } while (!command.has("recplay"));

        if (command["recplay"].asString() == "quit")
        {
            mCaptureBufferMode = false;
            break;
        }
        else if (command["recplay"].asString() == "record")
        {
            voiceRecordBuffer();
        }
        else if (command["recplay"].asString() == "playback")
        {
            voicePlaybackBuffer();
        }
    }

    LL_INFOS("Voice") << "Leaving capture/playback mode." << LL_ENDL;
    mCaptureBufferRecording = false;
    mCaptureBufferRecorded = false;
    mCaptureBufferPlaying = false;

    return;
}

int LLWebRTCVoiceClient::voiceRecordBuffer()
{
    LLSD timeoutResult(LLSDMap("recplay", "stop")); 

    LL_INFOS("Voice") << "Recording voice buffer" << LL_ENDL;

    LLSD result;

    captureBufferRecordStartSendMessage();
    notifyVoiceFontObservers();

    do
    {
        result = llcoro::suspendUntilEventOnWithTimeout(mWebRTCPump, CAPTURE_BUFFER_MAX_TIME, timeoutResult);
        LL_DEBUGS("Voice") << "event=" << ll_stream_notation_sd(result) << LL_ENDL;
    } while (!result.has("recplay"));

    mCaptureBufferRecorded = true;

    captureBufferRecordStopSendMessage();
    mCaptureBufferRecording = false;

    // Update UI, should really use a separate callback.
    notifyVoiceFontObservers();

    return true;
    /*TODO expand return to move directly into play*/
}

int LLWebRTCVoiceClient::voicePlaybackBuffer()
{
    LLSD timeoutResult(LLSDMap("recplay", "stop"));

    LL_INFOS("Voice") << "Playing voice buffer" << LL_ENDL;

    LLSD result;

    do
    {
        captureBufferPlayStartSendMessage(mPreviewVoiceFont);

        // Store the voice font being previewed, so that we know to restart if it changes.
        mPreviewVoiceFontLast = mPreviewVoiceFont;

        do
        {
            // Update UI, should really use a separate callback.
            notifyVoiceFontObservers();

            result = llcoro::suspendUntilEventOnWithTimeout(mWebRTCPump, CAPTURE_BUFFER_MAX_TIME, timeoutResult);
            LL_DEBUGS("Voice") << "event=" << ll_stream_notation_sd(result) << LL_ENDL;
        } while (!result.has("recplay"));

        if (result["recplay"] == "playback")
            continue;   // restart playback... May be a font change.

        break;
    } while (true);

    // Stop playing.
    captureBufferPlayStopSendMessage();
    mCaptureBufferPlaying = false;

    // Update UI, should really use a separate callback.
    notifyVoiceFontObservers();

    return true;
}


bool LLWebRTCVoiceClient::performMicTuning()
{
    LL_INFOS("Voice") << "Entering voice tuning mode." << LL_ENDL;

    mIsInTuningMode = false;

    //---------------------------------------------------------------------
    return true;
}

//=========================================================================

void LLWebRTCVoiceClient::closeSocket(void)
{
	mSocket.reset();
    sConnected = false;
	mConnectorEstablished = false;
	mAccountLoggedIn = false;
}

void LLWebRTCVoiceClient::loginSendMessage()
{
	std::ostringstream stream;

	bool autoPostCrashDumps = gSavedSettings.getBOOL("WebRTCAutoPostCrashDumps");

	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.Login.1\">"
		<< "<ConnectorHandle>" << LLWebRTCSecurity::getInstance()->connectorHandle() << "</ConnectorHandle>"
		<< "<AccountName>" << mAccountName << "</AccountName>"
        << "<AccountPassword>" << mAccountPassword << "</AccountPassword>"
        << "<AccountHandle>" << LLWebRTCSecurity::getInstance()->accountHandle() << "</AccountHandle>"
		<< "<AudioSessionAnswerMode>VerifyAnswer</AudioSessionAnswerMode>"
		<< "<EnableBuddiesAndPresence>false</EnableBuddiesAndPresence>"
		<< "<EnablePresencePersistence>0</EnablePresencePersistence>"
		<< "<BuddyManagementMode>Application</BuddyManagementMode>"
		<< "<ParticipantPropertyFrequency>5</ParticipantPropertyFrequency>"
		<< (autoPostCrashDumps?"<AutopostCrashDumps>true</AutopostCrashDumps>":"")
	<< "</Request>\n\n\n";

    LL_INFOS("Voice") << "Attempting voice login" << LL_ENDL;
	writeString(stream.str());
}

void LLWebRTCVoiceClient::logout()
{
	// Ensure that we'll re-request provisioning before logging in again
	mAccountPassword.clear();
	
	logoutSendMessage();
}

void LLWebRTCVoiceClient::logoutSendMessage()
{
	if(mAccountLoggedIn)
	{
        LL_INFOS("Voice") << "Attempting voice logout" << LL_ENDL;
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.Logout.1\">"
			<< "<AccountHandle>" << LLWebRTCSecurity::getInstance()->accountHandle() << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		mAccountLoggedIn = false;

		writeString(stream.str());
	}
}

void LLWebRTCVoiceClient::sessionGroupCreateSendMessage()
{
	if(mAccountLoggedIn)
	{		
		std::ostringstream stream;

		LL_DEBUGS("Voice") << "creating session group" << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.Create.1\">"
			<< "<AccountHandle>" << LLWebRTCSecurity::getInstance()->accountHandle() << "</AccountHandle>"
			<< "<Type>Normal</Type>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLWebRTCVoiceClient::sessionCreateSendMessage(const sessionStatePtr_t &session, bool startAudio, bool startText)
{
	S32 font_index = getVoiceFontIndex(session->mVoiceFontID);
	LL_DEBUGS("Voice") << "Requesting create: " << session->mSIPURI
                       << " with voice font: " << session->mVoiceFontID << " (" << font_index << ")"
                       << LL_ENDL;

	session->mCreateInProgress = true;
	if(startAudio)
	{
		session->mMediaConnectInProgress = true;
	}

	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << session->mSIPURI << "\" action=\"Session.Create.1\">"
		<< "<AccountHandle>" << LLWebRTCSecurity::getInstance()->accountHandle() << "</AccountHandle>"
		<< "<URI>" << session->mSIPURI << "</URI>";

	static const std::string allowed_chars =
				"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
				"0123456789"
				"-._~";

	if(!session->mHash.empty())
	{
		stream
			<< "<Password>" << LLURI::escape(session->mHash, allowed_chars) << "</Password>"
			<< "<PasswordHashAlgorithm>SHA1UserName</PasswordHashAlgorithm>";
	}

	stream
		<< "<ConnectAudio>" << (startAudio?"true":"false") << "</ConnectAudio>"
		<< "<ConnectText>" << (startText?"true":"false") << "</ConnectText>"
		<< "<VoiceFontID>" << font_index << "</VoiceFontID>"
		<< "<Name>" << mChannelName << "</Name>"
	<< "</Request>\n\n\n";
	writeString(stream.str());
}

void LLWebRTCVoiceClient::sessionGroupAddSessionSendMessage(const sessionStatePtr_t &session, bool startAudio, bool startText)
{
	LL_DEBUGS("Voice") << "Requesting create: " << session->mSIPURI << LL_ENDL;

	S32 font_index = getVoiceFontIndex(session->mVoiceFontID);
	LL_DEBUGS("Voice") << "With voice font: " << session->mVoiceFontID << " (" << font_index << ")" << LL_ENDL;

	session->mCreateInProgress = true;
	if(startAudio)
	{
		session->mMediaConnectInProgress = true;
	}
	
	std::string password;
	if(!session->mHash.empty())
	{
		static const std::string allowed_chars =
					"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
					"0123456789"
					"-._~"
					;
		password = LLURI::escape(session->mHash, allowed_chars);
	}

	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << session->mSIPURI << "\" action=\"SessionGroup.AddSession.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
		<< "<URI>" << session->mSIPURI << "</URI>"
		<< "<Name>" << mChannelName << "</Name>"
		<< "<ConnectAudio>" << (startAudio?"true":"false") << "</ConnectAudio>"
		<< "<ConnectText>" << (startText?"true":"false") << "</ConnectText>"
		<< "<VoiceFontID>" << font_index << "</VoiceFontID>"
		<< "<Password>" << password << "</Password>"
		<< "<PasswordHashAlgorithm>SHA1UserName</PasswordHashAlgorithm>"
	<< "</Request>\n\n\n"
	;
	
	writeString(stream.str());
}

void LLWebRTCVoiceClient::sessionMediaConnectSendMessage(const sessionStatePtr_t &session)
{
	S32 font_index = getVoiceFontIndex(session->mVoiceFontID);
	LL_DEBUGS("Voice") << "Connecting audio to session handle: " << session->mHandle
                       << " with voice font: " << session->mVoiceFontID << " (" << font_index << ")"
                       << LL_ENDL;

	session->mMediaConnectInProgress = true;
	
	std::ostringstream stream;

	stream
	<< "<Request requestId=\"" << session->mHandle << "\" action=\"Session.MediaConnect.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
		<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
		<< "<VoiceFontID>" << font_index << "</VoiceFontID>"
		<< "<Media>Audio</Media>"
	<< "</Request>\n\n\n";

	writeString(stream.str());
}

void LLWebRTCVoiceClient::sessionTextConnectSendMessage(const sessionStatePtr_t &session)
{
	LL_DEBUGS("Voice") << "connecting text to session handle: " << session->mHandle << LL_ENDL;
	
	std::ostringstream stream;

	stream
	<< "<Request requestId=\"" << session->mHandle << "\" action=\"Session.TextConnect.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
		<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
	<< "</Request>\n\n\n";

	writeString(stream.str());
}

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
	if(mAudioSession)
	{
		LL_DEBUGS("Voice") << "leaving session: " << mAudioSession->mSIPURI << LL_ENDL;

		if(!mAudioSession->mHandle.empty())
		{

#if RECORD_EVERYTHING
			// HACK: for testing only
			// Save looped recording
			std::string savepath("/tmp/WebRTCrecording");
			{
				time_t now = time(NULL);
				const size_t BUF_SIZE = 64;
				char time_str[BUF_SIZE];	/* Flawfinder: ignore */
						
				strftime(time_str, BUF_SIZE, "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
				savepath += time_str;
			}
			recordingLoopSave(savepath);
#endif

			sessionMediaDisconnectSendMessage(mAudioSession);
		}
		else
		{
			LL_WARNS("Voice") << "called with no session handle" << LL_ENDL;	
		}
	}
	else
	{
		LL_WARNS("Voice") << "called with no active session" << LL_ENDL;
	}
    sessionTerminate();
}

void LLWebRTCVoiceClient::sessionTerminateSendMessage(const sessionStatePtr_t &session)
{
	std::ostringstream stream;

	sessionGroupTerminateSendMessage(session);
	return;
	/*
	LL_DEBUGS("Voice") << "Sending Session.Terminate with handle " << session->mHandle << LL_ENDL;	
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.Terminate.1\">"
		<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
	*/
}

void LLWebRTCVoiceClient::sessionGroupTerminateSendMessage(const sessionStatePtr_t &session)
{
	std::ostringstream stream;
	
	LL_DEBUGS("Voice") << "Sending SessionGroup.Terminate with handle " << session->mGroupHandle << LL_ENDL;	
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.Terminate.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLWebRTCVoiceClient::sessionMediaDisconnectSendMessage(const sessionStatePtr_t &session)
{
	std::ostringstream stream;
	sessionGroupTerminateSendMessage(session);
	return;
	/*
	LL_DEBUGS("Voice") << "Sending Session.MediaDisconnect with handle " << session->mHandle << LL_ENDL;	
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.MediaDisconnect.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
		<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
		<< "<Media>Audio</Media>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
	*/
	
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
    bool inTuningMode = mIsInTuningMode;
    if (inTuningMode)
    {
        tuningStop();
	}
    mWebRTCDeviceInterface->setCaptureDevice(name);
    if (inTuningMode)
    {
        tuningStart();
    }
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

void LLWebRTCVoiceClient::tuningRenderStartSendMessage(const std::string& name, bool loop)
{		
	mTuningAudioFile = name;
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.RenderAudioStart.1\">"
    << "<SoundFilePath>" << mTuningAudioFile << "</SoundFilePath>"
    << "<Loop>" << (loop?"1":"0") << "</Loop>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLWebRTCVoiceClient::tuningRenderStopSendMessage()
{
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.RenderAudioStop.1\">"
    << "<SoundFilePath>" << mTuningAudioFile << "</SoundFilePath>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLWebRTCVoiceClient::tuningCaptureStartSendMessage(int loop)
{
	LL_DEBUGS("Voice") << "sending CaptureAudioStart" << LL_ENDL;
	
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.CaptureAudioStart.1\">"
	<< "<Duration>-1</Duration>"
    << "<LoopToRenderDevice>" << loop << "</LoopToRenderDevice>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLWebRTCVoiceClient::tuningCaptureStopSendMessage()
{
	LL_DEBUGS("Voice") << "sending CaptureAudioStop" << LL_ENDL;
	
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.CaptureAudioStop.1\">"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());

	mTuningEnergy = 0.0f;
}

void LLWebRTCVoiceClient::tuningSetMicVolume(float volume)
{
	int scaled_volume = scale_mic_volume(volume);

	if(scaled_volume != mTuningMicVolume)
	{
		mTuningMicVolume = scaled_volume;
		mTuningMicVolumeDirty = true;
	}
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
    return mWebRTCDeviceInterface->getTuningAudioLevel();
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

void LLWebRTCVoiceClient::daemonDied()
{
	// The daemon died, so the connection is gone.  Reset everything and start over.
	LL_WARNS("Voice") << "Connection to WebRTC daemon lost.  Resetting state."<< LL_ENDL;

	//TODO: Try to relaunch the daemon
}

void LLWebRTCVoiceClient::giveUp()
{
	// All has failed.  Clean up and stop trying.
    LL_WARNS("Voice") << "Terminating Voice Service" << LL_ENDL;
	closeSocket();
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
        sendPositionAndVolumeUpdate();
    }
}

Json::Value LLWebRTCVoiceClient::getPositionAndVolumeUpdateJson(bool force)
{
    Json::Value root = Json::objectValue;

    if ((mSpatialCoordsDirty || force) && inSpatialChannel())
    {
        root["sp"]      = Json::objectValue;
        root["sp"]["x"] = (int) (mAvatarPosition[0] * 100);
        root["sp"]["y"] = (int) (mAvatarPosition[1] * 100);
        root["sp"]["z"] = (int) (mAvatarPosition[2] * 100);
        root["sh"]      = Json::objectValue;
        root["sh"]["x"] = (int) (mAvatarRot[0] * 100);
        root["sh"]["y"] = (int) (mAvatarRot[1] * 100);
        root["sh"]["z"] = (int) (mAvatarRot[2] * 100);
        root["sh"]["w"] = (int) (mAvatarRot[3] * 100);

        root["lp"]      = Json::objectValue;
        root["lp"]["x"] = (int) (mCameraPosition[0] * 100);
        root["lp"]["y"] = (int) (mCameraPosition[1] * 100);
        root["lp"]["z"] = (int) (mCameraPosition[2] * 100);
        root["lh"]      = Json::objectValue;
        root["lh"]["x"] = (int) (mCameraRot[0] * 100);
        root["lh"]["y"] = (int) (mCameraRot[1] * 100);
        root["lh"]["z"] = (int) (mCameraRot[2] * 100);
        root["lh"]["w"] = (int) (mCameraRot[3] * 100);

        mSpatialCoordsDirty = false;
    }

    F32 audio_level = 0.0;

    if (!mMuteMic)
    {
        audio_level = (F32) mWebRTCAudioInterface->getAudioLevel();
    }
    uint32_t uint_audio_level = mMuteMic ? 0 : (uint32_t) (audio_level * 128);
    if (force || (uint_audio_level != mAudioLevel))
    {
        root["p"]                         = uint_audio_level;
        mAudioLevel                       = uint_audio_level;
        participantStatePtr_t participant = findParticipantByID(gAgentID);
        if (participant)
        {
            participant->mPower      = audio_level;
            participant->mIsSpeaking = participant->mPower > SPEAKING_AUDIO_LEVEL;
        }
    }
    return root;
}

void LLWebRTCVoiceClient::sendPositionAndVolumeUpdate()
{	


    if (mWebRTCDataInterface && mWebRTCAudioInterface)
    {
        Json::Value root = getPositionAndVolumeUpdateJson(false);
        
        if (root.size() > 0)
        {
            
            Json::FastWriter writer;
            std::string json_data = writer.write(root);
            
            mWebRTCDataInterface->sendData(json_data, false);
        }
    }
	
	
	if(mAudioSession && (mAudioSession->mVolumeDirty || mAudioSession->mMuteDirty))
	{
		participantMap::iterator iter = mAudioSession->mParticipantsByURI.begin();

		mAudioSession->mVolumeDirty = false;
		mAudioSession->mMuteDirty = false;
		
		for(; iter != mAudioSession->mParticipantsByURI.end(); iter++)
		{
			participantStatePtr_t p(iter->second);
			
			if(p->mVolumeDirty)
			{
				// Can't set volume/mute for yourself
				if(!p->mIsSelf)
				{
					// scale from the range 0.0-1.0 to WebRTC volume in the range 0-100
					S32 volume = ll_round(p->mVolume / VOLUME_SCALE_WEBRTC);
					bool mute = p->mOnMuteList;
					
					if(mute)
					{
						// SetParticipantMuteForMe doesn't work in p2p sessions.
						// If we want the user to be muted, set their volume to 0 as well.
						// This isn't perfect, but it will at least reduce their volume to a minimum.
						volume = 0;
						// Mark the current volume level as set to prevent incoming events
						// changing it to 0, so that we can return to it when unmuting.
						p->mVolumeSet = true;
					}
					
					if(volume == 0)
					{
						mute = true;
					}

					LL_DEBUGS("Voice") << "Setting volume/mute for avatar " << p->mAvatarID << " to " << volume << (mute?"/true":"/false") << LL_ENDL;
				}
				
				p->mVolumeDirty = false;
			}
		}
	}
}

void LLWebRTCVoiceClient::sendLocalAudioUpdates()
{
}

/**
 * Because of the recurring voice cutout issues (SL-15072) we are going to try
 * to disable the automatic VAD (Voice Activity Detection) and set the associated
 * parameters directly. We will expose them via Debug Settings and that should
 * let us iterate on a collection of values that work for us. Hopefully! 
 *
 * From the WebRTC Docs:
 *
 * VadAuto: A flag indicating if the automatic VAD is enabled (1) or disabled (0)
 *
 * VadHangover: The time (in milliseconds) that it takes
 * for the VAD to switch back to silence from speech mode after the last speech
 * frame has been detected.
 *
 * VadNoiseFloor: A dimensionless value between 0 and 
 * 20000 (default 576) that controls the maximum level at which the noise floor
 * may be set at by the VAD's noise tracking. Too low of a value will make noise
 * tracking ineffective (A value of 0 disables noise tracking and the VAD then 
 * relies purely on the sensitivity property). Too high of a value will make 
 * long speech classifiable as noise.
 *
 * VadSensitivity: A dimensionless value between 0 and 
 * 100, indicating the 'sensitivity of the VAD'. Increasing this value corresponds
 * to decreasing the sensitivity of the VAD (i.e. '0' is most sensitive, 
 * while 100 is 'least sensitive')
 */
void LLWebRTCVoiceClient::setupVADParams(unsigned int vad_auto,
                                        unsigned int vad_hangover,
                                        unsigned int vad_noise_floor,
                                        unsigned int vad_sensitivity)
{
    std::ostringstream stream;

    LL_INFOS("Voice") << "Setting the automatic VAD to "
        << (vad_auto ? "True" : "False")
		<< " and discrete values to"
		<< " VadHangover = " << vad_hangover
		<< ", VadSensitivity = " << vad_sensitivity
		<< ", VadNoiseFloor = " << vad_noise_floor
        << LL_ENDL;

	// Create a request to set the VAD parameters:
	stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.SetVadProperties.1\">"
               << "<VadAuto>" << vad_auto << "</VadAuto>"
               << "<VadHangover>" << vad_hangover << "</VadHangover>"
               << "<VadSensitivity>" << vad_sensitivity << "</VadSensitivity>"
               << "<VadNoiseFloor>" << vad_noise_floor << "</VadNoiseFloor>"
           << "</Request>\n\n\n";

    if (!stream.str().empty())
    {
        writeString(stream.str());
    }
}

void LLWebRTCVoiceClient::onVADSettingsChange()
{
	// pick up the VAD variables (one of which was changed)
	unsigned int vad_auto = gSavedSettings.getU32("WebRTCVadAuto");
	unsigned int vad_hangover = gSavedSettings.getU32("WebRTCVadHangover");
	unsigned int vad_noise_floor = gSavedSettings.getU32("WebRTCVadNoiseFloor");
	unsigned int vad_sensitivity = gSavedSettings.getU32("WebRTCVadSensitivity");

	// build a VAD params change request and send it to SLVoice
	setupVADParams(vad_auto, vad_hangover, vad_noise_floor, vad_sensitivity);
}

/////////////////////////////
// WebRTC Signaling Handlers
void LLWebRTCVoiceClient::OnIceGatheringState(llwebrtc::LLWebRTCSignalingObserver::IceGatheringState state)
{
    LL_INFOS("Voice") << "Ice Gathering voice account. " << state << LL_ENDL;

	if (state == llwebrtc::LLWebRTCSignalingObserver::IceGatheringState::ICE_GATHERING_COMPLETE) 
	{
        LLMutexLock lock(&mVoiceStateMutex);
        mIceCompleted = true;
	}
}

void LLWebRTCVoiceClient::OnIceCandidate(const llwebrtc::LLWebRTCIceCandidate &candidate)
{
    LLMutexLock lock(&mVoiceStateMutex);
	mIceCandidates.push_back(candidate); 
}

void LLWebRTCVoiceClient::processIceUpdates()
{
    LL_INFOS("Voice") << "Ice Gathering voice account." << LL_ENDL;
    while ((!gAgent.getRegion() || !gAgent.getRegion()->capabilitiesReceived()) && !sShuttingDown)
    {
        LL_DEBUGS("Voice") << "no capabilities for voice provisioning; waiting " << LL_ENDL;
        // *TODO* Pump a message for wake up.
        llcoro::suspend();
    }

    if (sShuttingDown)
    {
        return;
    }

    std::string url = gAgent.getRegionCapability("VoiceSignalingRequest");

    LL_DEBUGS("Voice") << "region ready to complete voice signaling; url=" << url << LL_ENDL;

    LLCore::HttpRequest::policy_t               httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("voiceAccountProvision", httpPolicy));
    LLCore::HttpRequest::ptr_t                  httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t                  httpOpts = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions);

    LLSD body;
    {
        LLMutexLock lock(&mVoiceStateMutex);

        if (mIceCandidates.size())
        {
            LLSD body;

            for (auto &ice_candidate : mIceCandidates)
            {
                LLSD body_candidate;
                body_candidate["sdpMid"]        = ice_candidate.sdp_mid;
                body_candidate["sdpMLineIndex"] = ice_candidate.mline_index;
                body_candidate["candidate"]     = ice_candidate.candidate;
                body["candidates"].append(body_candidate);
            }
            mIceCandidates.clear();
        }
        else if (mIceCompleted)
        {
            LLSD body_candidate;
            body_candidate["completed"] = true;
            body["candidate"]           = body_candidate;
            mIceCompleted               = false;
        }
        else
        {
            return;
        }
    }
    LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpPost(url,
                                  LLCore::HttpRequest::DEFAULT_POLICY_ID,
                                  body,
                                  boost::bind(&LLWebRTCVoiceClient::onIceUpdateComplete, this, _1),
                                  boost::bind(&LLWebRTCVoiceClient::onIceUpdateError, this, 3, url, body, _1));
}

void LLWebRTCVoiceClient::onIceUpdateComplete(const LLSD& result)
{
    if (sShuttingDown)
    {
        return;
    }
}

void LLWebRTCVoiceClient::onIceUpdateError(int retries, std::string url, LLSD body, const LLSD& result)
{
    if (sShuttingDown)
    {
        return;
    }
    LLCore::HttpRequest::policy_t               httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("voiceAccountProvision", httpPolicy));

	if (retries >= 0)
    {
        LL_WARNS("Voice") << "Unable to complete ice trickling voice account, retrying." << LL_ENDL;
        LLCoreHttpUtil::HttpCoroutineAdapter::callbackHttpPost(url,
                                      LLCore::HttpRequest::DEFAULT_POLICY_ID,
                                      body,
                                      boost::bind(&LLWebRTCVoiceClient::onIceUpdateComplete, this, _1),
									  boost::bind(&LLWebRTCVoiceClient::onIceUpdateError, this, retries - 1, url, body, _1));
    }
    else 
	{
        LL_WARNS("Voice") << "Unable to complete ice trickling voice account, retrying." << LL_ENDL;
	}
}

void LLWebRTCVoiceClient::OnOfferAvailable(const std::string &sdp) 
{
    LL_INFOS("Voice") << "On Offer Available." << LL_ENDL;
    LLMutexLock lock(&mVoiceStateMutex);
    mChannelSDP = sdp;
}

void LLWebRTCVoiceClient::OnAudioEstablished(llwebrtc::LLWebRTCAudioInterface * audio_interface)
{
    LL_INFOS("Voice") << "On AudioEstablished." << LL_ENDL;
    mWebRTCAudioInterface = audio_interface;
    mWebRTCAudioInterface->setAudioObserver(this);
    float speaker_volume  = 0;
    audio_interface->setMute(true);
    {
        LLMutexLock lock(&mVoiceStateMutex);
        speaker_volume = mSpeakerVolume;
    }
	audio_interface->setSpeakerVolume(mSpeakerVolume);
    setVoiceControlStateUnless(VOICE_STATE_SESSION_ESTABLISHED, VOICE_STATE_SESSION_RETRY);
}

void LLWebRTCVoiceClient::OnDataReceived(const std::string& data, bool binary)
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
            participantStatePtr_t participant = findParticipantByID(agent_id);
            bool joined = voice_data[participant_id].get("j", Json::Value(false)).asBool();
            new_participant |= joined;
            if (!participant && joined)
            {
                participant = addParticipantByID(agent_id);
            }
			if (participant)
			{
                if(voice_data[participant_id].get("l", Json::Value(false)).asBool())
                {
                    removeParticipantByID(agent_id);
                }
                F32 energyRMS = (F32) (voice_data[participant_id].get("p", Json::Value(participant->mPower)).asInt()) / 128;
				// convert to decibles
                participant->mPower = energyRMS;
                /* WebRTC appears to have deprecated VAD, but it's still in the Audio Processing Module so maybe we
				   can use it at some point when we actually process frames. */
                participant->mIsSpeaking = participant->mPower > SPEAKING_AUDIO_LEVEL; 
			}
        }
        if (new_participant) 
		{
            Json::FastWriter writer;
            Json::Value      root = getPositionAndVolumeUpdateJson(true);
            std::string json_data = writer.write(root);
            mWebRTCDataInterface->sendData(json_data, false);
		}
    }
}

void LLWebRTCVoiceClient::OnDataChannelReady() 
{

}


void LLWebRTCVoiceClient::OnRenegotiationNeeded()
{
    LL_INFOS("Voice") << "On Renegotiation Needed." << LL_ENDL;
    mRelogRequested = TRUE;
    mIsProcessingChannels = FALSE;
    notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LOGIN_RETRY);
    setVoiceControlStateUnless(VOICE_STATE_SESSION_RETRY);
}

/////////////////////////////
// Response/Event handlers

void LLWebRTCVoiceClient::connectorCreateResponse(int statusCode, std::string &statusString, std::string &connectorHandle, std::string &versionID)
{	
    LLSD result = LLSD::emptyMap();

	if(statusCode == 0)
	{
		// Connector created, move forward.
        if (connectorHandle == LLWebRTCSecurity::getInstance()->connectorHandle())
        {
            LL_INFOS("Voice") << "Voice connector succeeded, WebRTC SDK version is " << versionID << " connector handle " << connectorHandle << LL_ENDL;
            mVoiceVersion.serverVersion = versionID;
            mConnectorEstablished = true;
            mTerminateDaemon = false;

            result["connector"] = LLSD::Boolean(true);
        }
        else
        {
            // This shouldn't happen - we are somehow out of sync with SLVoice
            // or possibly there are two things trying to run SLVoice at once
            // or someone is trying to hack into it.
            LL_WARNS("Voice") << "Connector returned wrong handle "
                              << "(" << connectorHandle << ")"
                              << " expected (" << LLWebRTCSecurity::getInstance()->connectorHandle() << ")"
                              << LL_ENDL;
            result["connector"] = LLSD::Boolean(false);
            // Give up.
            mTerminateDaemon = true;
        }
	}
    else if (statusCode == 10028) // web request timeout prior to login
    {
        // this is usually fatal, but a long timeout might work
        result["connector"] = LLSD::Boolean(false);
        result["retry"] = LLSD::Real(CONNECT_ATTEMPT_TIMEOUT);
        
        LL_WARNS("Voice") << "Voice connection failed" << LL_ENDL;
    }
    else if (statusCode == 10006) // name resolution failure - a shorter retry may work
    {
        // some networks have slower DNS, but a short timeout might let it catch up
        result["connector"] = LLSD::Boolean(false);
        result["retry"] = LLSD::Real(CONNECT_DNS_TIMEOUT);
        
        LL_WARNS("Voice") << "Voice connection DNS lookup failed" << LL_ENDL;
    }
    else // unknown failure - give up
    {
        LL_WARNS("Voice") << "Voice connection failure ("<< statusCode << "): " << statusString << LL_ENDL;
        mTerminateDaemon = true;
        result["connector"] = LLSD::Boolean(false);
    }

    mWebRTCPump.post(result);
}

void LLWebRTCVoiceClient::loginResponse(int statusCode, std::string &statusString, std::string &accountHandle, int numberOfAliases)
{ 
    LLSD result = LLSD::emptyMap();

    LL_DEBUGS("Voice") << "Account.Login response (" << statusCode << "): " << statusString << LL_ENDL;
	
	// Status code of 20200 means "bad password".  We may want to special-case that at some point.
	
	if ( statusCode == HTTP_UNAUTHORIZED )
	{
		// Login failure which is probably caused by the delay after a user's password being updated.
		LL_INFOS("Voice") << "Account.Login response failure (" << statusCode << "): " << statusString << LL_ENDL;
        result["login"] = LLSD::String("retry");
	}
	else if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Account.Login response failure (" << statusCode << "): " << statusString << LL_ENDL;
        result["login"] = LLSD::String("failed");
	}
	else
	{
		// Login succeeded, move forward.
		mAccountLoggedIn = true;
		mNumberOfAliases = numberOfAliases;
        result["login"] = LLSD::String("response_ok");
	}

    mWebRTCPump.post(result);

}

void LLWebRTCVoiceClient::sessionCreateResponse(std::string &requestId, int statusCode, std::string &statusString, std::string &sessionHandle)
{	
    sessionStatePtr_t session(findSessionBeingCreatedByURI(requestId));
	
	if(session)
	{
		session->mCreateInProgress = false;
	}
	
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Session.Create response failure (" << statusCode << "): " << statusString << LL_ENDL;
		if(session)
		{
			session->mErrorStatusCode = statusCode;		
			session->mErrorStatusString = statusString;
			if(session == mAudioSession)
			{
                LLSD WebRTCevent(LLSDMap("handle", LLSD::String(sessionHandle))
                        ("session", "failed")
                        ("reason", LLSD::Integer(statusCode)));

                mWebRTCPump.post(WebRTCevent);
            }
			else
			{
				reapSession(session);
			}
		}
	}
	else
	{
		LL_INFOS("Voice") << "Session.Create response received (success), session handle is " << sessionHandle << LL_ENDL;
		if(session)
		{
			setSessionHandle(session, sessionHandle);
		}
        LLSD WebRTCevent(LLSDMap("handle", LLSD::String(sessionHandle))
                ("session", "created"));

        mWebRTCPump.post(WebRTCevent);
	}
}

void LLWebRTCVoiceClient::sessionGroupAddSessionResponse(std::string &requestId, int statusCode, std::string &statusString, std::string &sessionHandle)
{	
    sessionStatePtr_t session(findSessionBeingCreatedByURI(requestId));
	
	if(session)
	{
		session->mCreateInProgress = false;
	}
	
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "SessionGroup.AddSession response failure (" << statusCode << "): " << statusString << LL_ENDL;
		if(session)
		{
			session->mErrorStatusCode = statusCode;		
			session->mErrorStatusString = statusString;
			if(session == mAudioSession)
			{
                LLSD WebRTCevent(LLSDMap("handle", LLSD::String(sessionHandle))
                    ("session", "failed"));

                mWebRTCPump.post(WebRTCevent);
			}
			else
			{
				reapSession(session);
			}
		}
	}
	else
	{
		LL_DEBUGS("Voice") << "SessionGroup.AddSession response received (success), session handle is " << sessionHandle << LL_ENDL;
		if(session)
		{
			setSessionHandle(session, sessionHandle);
		}

        LLSD WebRTCevent(LLSDMap("handle", LLSD::String(sessionHandle))
            ("session", "added"));

        mWebRTCPump.post(WebRTCevent);

	}
}

void LLWebRTCVoiceClient::sessionConnectResponse(std::string &requestId, int statusCode, std::string &statusString)
{
    sessionStatePtr_t session(findSession(requestId));
	// 1026 is session already has media,  somehow mediaconnect was called twice on the same session.
	// set the session info to reflect that the user is already connected.
	if (statusCode == 1026)
	{
		session->mVoiceActive = true;
		session->mMediaConnectInProgress = false;
		session->mMediaStreamState = streamStateConnected;
		//session->mTextStreamState = streamStateConnected;
		session->mErrorStatusCode = 0;
	}
	else if (statusCode != 0)
	{
		LL_WARNS("Voice") << "Session.Connect response failure (" << statusCode << "): " << statusString << LL_ENDL;
		if (session)
		{
			session->mMediaConnectInProgress = false;
			session->mErrorStatusCode = statusCode;
			session->mErrorStatusString = statusString;
		}
	}
	else
	{
		LL_DEBUGS("Voice") << "Session.Connect response received (success)" << LL_ENDL;
	}
}

void LLWebRTCVoiceClient::logoutResponse(int statusCode, std::string &statusString)
{	
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Account.Logout response failure: " << statusString << LL_ENDL;
		// Should this ever fail?  do we care if it does?
	}
    LLSD WebRTCevent(LLSDMap("logout", LLSD::Boolean(true)));

    mWebRTCPump.post(WebRTCevent);
}

void LLWebRTCVoiceClient::connectorShutdownResponse(int statusCode, std::string &statusString)
{
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Connector.InitiateShutdown response failure: " << statusString << LL_ENDL;
		// Should this ever fail?  do we care if it does?
	}
	
	sConnected = false;
	mShutdownComplete = true;
	
    LLSD WebRTCevent(LLSDMap("connector", LLSD::Boolean(false)));

    mWebRTCPump.post(WebRTCevent);
}

void LLWebRTCVoiceClient::sessionAddedEvent(
		std::string &uriString, 
		std::string &alias, 
		std::string &sessionHandle, 
		std::string &sessionGroupHandle, 
		bool isChannel, 
		bool incoming,
		std::string &nameString,
		std::string &applicationString)
{
    sessionStatePtr_t session;

	LL_INFOS("Voice") << "session " << uriString << ", alias " << alias << ", name " << nameString << " handle " << sessionHandle << LL_ENDL;
	
	session = addSession(uriString, sessionHandle);
	if(session)
	{
		session->mGroupHandle = sessionGroupHandle;
		session->mIsChannel = isChannel;
		session->mIncoming = incoming;
		session->mAlias = alias;
			
		// Generate a caller UUID -- don't need to do this for channels
		if(!session->mIsChannel)
		{
			if(IDFromName(session->mSIPURI, session->mCallerID))
			{
				// Normal URI(base64-encoded UUID) 
			}
			else if(!session->mAlias.empty() && IDFromName(session->mAlias, session->mCallerID))
			{
				// Wrong URI, but an alias is available.  Stash the incoming URI as an alternate
				session->mAlternateSIPURI = session->mSIPURI;
				
				// and generate a proper URI from the ID.
				setSessionURI(session, session->mCallerID.asString());
			}
			else
			{
				LL_INFOS("Voice") << "Could not generate caller id from uri, using hash of uri " << session->mSIPURI << LL_ENDL;
				session->mCallerID.generate(session->mSIPURI);
				session->mSynthesizedCallerID = true;
				
				// Can't look up the name in this case -- we have to extract it from the URI.
                std::string namePortion = nameString;
				
				// Some incoming names may be separated with an underscore instead of a space.  Fix this.
				LLStringUtil::replaceChar(namePortion, '_', ' ');
				
				// Act like we just finished resolving the name (this stores it in all the right places)
				avatarNameResolved(session->mCallerID, namePortion);
			}
		
			LL_INFOS("Voice") << "caller ID: " << session->mCallerID << LL_ENDL;

			if(!session->mSynthesizedCallerID)
			{
				// If we got here, we don't have a proper name.  Initiate a lookup.
				lookupName(session->mCallerID);
			}
		}
	}
}

void LLWebRTCVoiceClient::sessionGroupAddedEvent(std::string &sessionGroupHandle)
{
	LL_DEBUGS("Voice") << "handle " << sessionGroupHandle << LL_ENDL;
	
#if  USE_SESSION_GROUPS
	if(mMainSessionGroupHandle.empty())
	{
		// This is the first (i.e. "main") session group.  Save its handle.
		mMainSessionGroupHandle = sessionGroupHandle;
	}
	else
	{
		LL_DEBUGS("Voice") << "Already had a session group handle " << mMainSessionGroupHandle << LL_ENDL;
	}
#endif
}

void LLWebRTCVoiceClient::joinedAudioSession(const sessionStatePtr_t &session)
{
	LL_DEBUGS("Voice") << "Joined Audio Session" << LL_ENDL;
	if(mAudioSession != session)
	{
        sessionStatePtr_t oldSession = mAudioSession;

		mAudioSession = session;
		mAudioSessionChanged = true;

		// The old session may now need to be deleted.
		reapSession(oldSession);
	}
	
	// This is the session we're joining.
	if(mIsJoiningSession)
	{
        LLSD WebRTCevent(LLSDMap("handle", LLSD::String(session->mHandle))
                ("session", "joined"));

        mWebRTCPump.post(WebRTCevent);
		
		if(!session->mIsChannel)
		{
			// this is a p2p session.  Make sure the other end is added as a participant.
            participantStatePtr_t participant(session->addParticipant(LLUUID(session->mSIPURI)));
			if(participant)
			{
				if(participant->mAvatarIDValid)
				{
					lookupName(participant->mAvatarID);
				}
				else if(!session->mName.empty())
				{
					participant->mDisplayName = session->mName;
					avatarNameResolved(participant->mAvatarID, session->mName);
				}
				
				// TODO: Question: Do we need to set up mAvatarID/mAvatarIDValid here?
				LL_INFOS("Voice") << "added caller as participant \"" << participant->mAccountName 
						<< "\" (" << participant->mAvatarID << ")"<< LL_ENDL;
			}
		}
	}
}

void LLWebRTCVoiceClient::sessionRemovedEvent(
	std::string &sessionHandle, 
	std::string &sessionGroupHandle)
{
	LL_INFOS("Voice") << "handle " << sessionHandle << LL_ENDL;
	
    sessionStatePtr_t session(findSession(sessionHandle));
	if(session)
	{
		leftAudioSession(session);

		// This message invalidates the session's handle.  Set it to empty.
        clearSessionHandle(session);
		
		// This also means that the session's session group is now empty.
		// Terminate the session group so it doesn't leak.
		sessionGroupTerminateSendMessage(session);
		
		// Reset the media state (we now have no info)
		session->mMediaStreamState = streamStateUnknown;
		//session->mTextStreamState = streamStateUnknown;
		
		// Conditionally delete the session
		reapSession(session);
	}
	else
	{
		// Already reaped this session.
		LL_DEBUGS("Voice") << "unknown session " << sessionHandle << " removed" << LL_ENDL;
	}

}

void LLWebRTCVoiceClient::reapSession(const sessionStatePtr_t &session)
{
	if(session)
	{
		
		if(session->mCreateInProgress)
		{
			LL_DEBUGS("Voice") << "NOT deleting session " << session->mSIPURI << " (create in progress)" << LL_ENDL;
		}
		else if(session->mMediaConnectInProgress)
		{
			LL_DEBUGS("Voice") << "NOT deleting session " << session->mSIPURI << " (connect in progress)" << LL_ENDL;
		}
		else if(session == mAudioSession)
		{
			LL_DEBUGS("Voice") << "NOT deleting session " << session->mSIPURI << " (it's the current session)" << LL_ENDL;
		}
		else if(session == mNextAudioSession)
		{
			LL_DEBUGS("Voice") << "NOT deleting session " << session->mSIPURI << " (it's the next session)" << LL_ENDL;
		}
		else
		{
			// We don't have a reason to keep tracking this session, so just delete it.
			LL_DEBUGS("Voice") << "deleting session " << session->mSIPURI << LL_ENDL;
			deleteSession(session);
		}	
	}
	else
	{
//		LL_DEBUGS("Voice") << "session is NULL" << LL_ENDL;
	}
}

// Returns true if the session seems to indicate we've moved to a region on a different voice server
bool LLWebRTCVoiceClient::sessionNeedsRelog(const sessionStatePtr_t &session)
{
	bool result = false;
	
	if(session)
	{
		// Only make this check for spatial channels (so it won't happen for group or p2p calls)
		if(session->mIsSpatial)
		{	
			std::string::size_type atsign;
			
			atsign = session->mSIPURI.find("@");
			
			if(atsign != std::string::npos)
			{
				std::string urihost = session->mSIPURI.substr(atsign + 1);
			}
		}
	}
	
	return result;
}

void LLWebRTCVoiceClient::leftAudioSession(const sessionStatePtr_t &session)
{
    if (mAudioSession == session)
    {
        LLSD WebRTCevent(LLSDMap("handle", LLSD::String(session->mHandle))
            ("session", "removed"));

        mWebRTCPump.post(WebRTCevent);
    }
}

void LLWebRTCVoiceClient::accountLoginStateChangeEvent(
		std::string &accountHandle, 
		int statusCode, 
		std::string &statusString, 
		int state)
{
    LLSD levent = LLSD::emptyMap();

	/*
		According to Mike S., status codes for this event are:
		login_state_logged_out=0,
        login_state_logged_in = 1,
        login_state_logging_in = 2,
        login_state_logging_out = 3,
        login_state_resetting = 4,
        login_state_error=100	
	*/
	
	LL_DEBUGS("Voice") << "state change event: " << state << LL_ENDL;
	switch(state)
	{
		case 1:
            levent["login"] = LLSD::String("account_login");

            mWebRTCPump.post(levent);
            break;
        case 2:
            break;

        case 3:
            levent["login"] = LLSD::String("account_loggingOut");

            mWebRTCPump.post(levent);
            break;

        case 4:
            break;

        case 100:
            LL_WARNS("Voice") << "account state event error" << LL_ENDL;
            break;

        case 0:
            levent["login"] = LLSD::String("account_logout");

            mWebRTCPump.post(levent);
            break;
    		
        default:
			//Used to be a commented out warning
			LL_WARNS("Voice") << "unknown account state event: " << state << LL_ENDL;
	    	break;
	}
}

void LLWebRTCVoiceClient::mediaCompletionEvent(std::string &sessionGroupHandle, std::string &mediaCompletionType)
{
    LLSD result;

	if (mediaCompletionType == "AuxBufferAudioCapture")
	{
		mCaptureBufferRecording = false;
        result["recplay"] = "end";
	}
	else if (mediaCompletionType == "AuxBufferAudioRender")
	{
		// Ignore all but the last stop event
		if (--mPlayRequestCount <= 0)
		{
			mCaptureBufferPlaying = false;
            result["recplay"] = "end";
//          result["recplay"] = "done";
        }
	}
	else
	{
		LL_WARNS("Voice") << "Unknown MediaCompletionType: " << mediaCompletionType << LL_ENDL;
	}

    if (!result.isUndefined())
        mWebRTCPump.post(result);
}

void LLWebRTCVoiceClient::mediaStreamUpdatedEvent(
	std::string &sessionHandle, 
	std::string &sessionGroupHandle, 
	int statusCode, 
	std::string &statusString, 
	int state, 
	bool incoming)
{
    sessionStatePtr_t session(findSession(sessionHandle));
	
	LL_DEBUGS("Voice") << "session " << sessionHandle << ", status code " << statusCode << ", string \"" << statusString << "\"" << LL_ENDL;
	
	if(session)
	{
		// We know about this session
		
		// Save the state for later use
		session->mMediaStreamState = state;
		
		switch(statusCode)
		{
			case 0:
			case HTTP_OK:
				// generic success
				// Don't change the saved error code (it may have been set elsewhere)
			break;
			default:
				// save the status code for later
				session->mErrorStatusCode = statusCode;
			break;
		}

		switch(state)
		{
		case streamStateDisconnecting:
		case streamStateIdle:
				// Standard "left audio session", WebRTC state 'disconnected'
				session->mVoiceActive = false;
				session->mMediaConnectInProgress = false;
				leftAudioSession(session);
			break;

			case streamStateConnected:
				session->mVoiceActive = true;
				session->mMediaConnectInProgress = false;
				joinedAudioSession(session);
			case streamStateConnecting: // do nothing, but prevents a warning getting into the logs.  
			break;
			
			case streamStateRinging:
				if(incoming)
				{
					// Send the voice chat invite to the GUI layer
					// TODO: Question: Should we correlate with the mute list here?
					session->mIMSessionID = LLIMMgr::computeSessionID(IM_SESSION_P2P_INVITE, session->mCallerID);
					session->mVoiceInvitePending = true;
					if(session->mName.empty())
					{
						lookupName(session->mCallerID);
					}
					else
					{
						// Act like we just finished resolving the name
						avatarNameResolved(session->mCallerID, session->mName);
					}
				}
			break;
			
			default:
				LL_WARNS("Voice") << "unknown state " << state << LL_ENDL;
			break;
			
		}
		
	}
	else
	{
		// session disconnectintg and disconnected events arriving after we have already left the session.
		LL_DEBUGS("Voice") << "session " << sessionHandle << " not found"<< LL_ENDL;
	}
}

void LLWebRTCVoiceClient::participantAddedEvent(
		std::string &sessionHandle, 
		std::string &sessionGroupHandle, 
		std::string &uriString, 
		std::string &alias, 
		std::string &nameString, 
		std::string &displayNameString, 
		int participantType)
{
    sessionStatePtr_t session(findSession(sessionHandle));
	if(session)
	{
        participantStatePtr_t participant(session->addParticipant(LLUUID(uriString)));
		if(participant)
		{
			participant->mAccountName = nameString;

			LL_DEBUGS("Voice") << "added participant \"" << participant->mAccountName 
					<< "\" (" << participant->mAvatarID << ")"<< LL_ENDL;

			if(participant->mAvatarIDValid)
			{
				// Initiate a lookup
				lookupName(participant->mAvatarID);
			}
			else
			{
				// If we don't have a valid avatar UUID, we need to fill in the display name to make the active speakers floater work.
				std::string namePortion = displayNameString;

				if(namePortion.empty())
				{
					// Problems with both of the above, fall back to the account name
					namePortion = nameString;
				}
				
				// Set the display name (which is a hint to the active speakers window not to do its own lookup)
				participant->mDisplayName = namePortion;
				avatarNameResolved(participant->mAvatarID, namePortion);
			}
		}
	}
}

void LLWebRTCVoiceClient::participantRemovedEvent(
		std::string &sessionHandle, 
		std::string &sessionGroupHandle, 
		std::string &uriString, 
		std::string &alias, 
		std::string &nameString)
{
    sessionStatePtr_t session(findSession(sessionHandle));
	if(session)
	{
        participantStatePtr_t participant(session->findParticipant(uriString));
		if(participant)
		{
			session->removeParticipant(participant);
		}
		else
		{
			LL_DEBUGS("Voice") << "unknown participant " << uriString << LL_ENDL;
		}
	}
	else
	{
		// a late arriving event on a session we have already left.
		LL_DEBUGS("Voice") << "unknown session " << sessionHandle << LL_ENDL;
	}
}

void LLWebRTCVoiceClient::messageEvent(
		std::string &sessionHandle, 
		std::string &uriString, 
		std::string &alias, 
		std::string &messageHeader, 
		std::string &messageBody,
		std::string &applicationString)
{
	LL_DEBUGS("Voice") << "Message event, session " << sessionHandle << " from " << uriString << LL_ENDL;
//	LL_DEBUGS("Voice") << "    header " << messageHeader << ", body: \n" << messageBody << LL_ENDL;

    LL_INFOS("Voice") << "WebRTC raw message:" << std::endl << messageBody << LL_ENDL;

	if(messageHeader.find(HTTP_CONTENT_TEXT_HTML) != std::string::npos)
	{
		std::string message;

		{
			const std::string startMarker = "<body";
			const std::string startMarker2 = ">";
			const std::string endMarker = "</body>";
			const std::string startSpan = "<span";
			const std::string endSpan = "</span>";
			std::string::size_type start;
			std::string::size_type end;
			
			// Default to displaying the raw string, so the message gets through.
			message = messageBody;

			// Find the actual message text within the XML fragment
			start = messageBody.find(startMarker);
			start = messageBody.find(startMarker2, start);
			end = messageBody.find(endMarker);

			if(start != std::string::npos)
			{
				start += startMarker2.size();
				
				if(end != std::string::npos)
					end -= start;
					
				message.assign(messageBody, start, end);
			}
			else 
			{
				// Didn't find a <body>, try looking for a <span> instead.
				start = messageBody.find(startSpan);
				start = messageBody.find(startMarker2, start);
				end = messageBody.find(endSpan);
				
				if(start != std::string::npos)
				{
					start += startMarker2.size();
					
					if(end != std::string::npos)
						end -= start;
					
					message.assign(messageBody, start, end);
				}			
			}
		}	
		
//		LL_DEBUGS("Voice") << "    raw message = \n" << message << LL_ENDL;

		// strip formatting tags
		{
			std::string::size_type start;
			std::string::size_type end;
			
			while((start = message.find('<')) != std::string::npos)
			{
				if((end = message.find('>', start + 1)) != std::string::npos)
				{
					// Strip out the tag
					message.erase(start, (end + 1) - start);
				}
				else
				{
					// Avoid an infinite loop
					break;
				}
			}
		}
		
		// Decode ampersand-escaped chars
		{
			std::string::size_type mark = 0;

			// The text may contain text encoded with &lt;, &gt;, and &amp;
			mark = 0;
			while((mark = message.find("&lt;", mark)) != std::string::npos)
			{
				message.replace(mark, 4, "<");
				mark += 1;
			}
			
			mark = 0;
			while((mark = message.find("&gt;", mark)) != std::string::npos)
			{
				message.replace(mark, 4, ">");
				mark += 1;
			}
			
			mark = 0;
			while((mark = message.find("&amp;", mark)) != std::string::npos)
			{
				message.replace(mark, 5, "&");
				mark += 1;
			}
		}
		
		// strip leading/trailing whitespace (since we always seem to get a couple newlines)
		LLStringUtil::trim(message);
		
//		LL_DEBUGS("Voice") << "    stripped message = \n" << message << LL_ENDL;
		
        sessionStatePtr_t session(findSession(sessionHandle));
		if(session)
		{
			bool is_do_not_disturb = gAgent.isDoNotDisturb();
			bool is_muted = LLMuteList::getInstance()->isMuted(session->mCallerID, session->mName, LLMute::flagTextChat);
			bool is_linden = LLMuteList::isLinden(session->mName);
			LLChat chat;

			chat.mMuted = is_muted && !is_linden;
			
			if(!chat.mMuted)
			{
				chat.mFromID = session->mCallerID;
				chat.mFromName = session->mName;
				chat.mSourceType = CHAT_SOURCE_AGENT;

				if(is_do_not_disturb && !is_linden)
				{
					// TODO: Question: Return do not disturb mode response here?  Or maybe when session is started instead?
				}
				
				LL_DEBUGS("Voice") << "adding message, name " << session->mName << " session " << session->mIMSessionID << ", target " << session->mCallerID << LL_ENDL;
				LLIMMgr::getInstance()->addMessage(session->mIMSessionID,
						session->mCallerID,
						session->mName.c_str(),
						message.c_str(),
						false,
						LLStringUtil::null,		// default arg
						IM_NOTHING_SPECIAL,		// default arg
						0,						// default arg
						LLUUID::null,			// default arg
						LLVector3::zero);		// default arg
			}
		}		
	}
}

void LLWebRTCVoiceClient::sessionNotificationEvent(std::string &sessionHandle, std::string &uriString, std::string &notificationType)
{
    sessionStatePtr_t session(findSession(sessionHandle));
	
	if(session)
	{
		participantStatePtr_t participant(session->findParticipant(uriString));
		if(participant)
		{
			if (!stricmp(notificationType.c_str(), "Typing"))
			{
				// Other end started typing
				// TODO: The proper way to add a typing notification seems to be LLIMMgr::processIMTypingStart().
				// It requires some info for the message, which we don't have here.
			}
			else if (!stricmp(notificationType.c_str(), "NotTyping"))
			{
				// Other end stopped typing
				// TODO: The proper way to remove a typing notification seems to be LLIMMgr::processIMTypingStop().
				// It requires some info for the message, which we don't have here.
			}
			else
			{
				LL_DEBUGS("Voice") << "Unknown notification type " << notificationType << "for participant " << uriString << " in session " << session->mSIPURI << LL_ENDL;
			}
		}
		else
		{
			LL_DEBUGS("Voice") << "Unknown participant " << uriString << " in session " << session->mSIPURI << LL_ENDL;
		}
	}
	else
	{
		LL_DEBUGS("Voice") << "Unknown session handle " << sessionHandle << LL_ENDL;
	}
}

void LLWebRTCVoiceClient::voiceServiceConnectionStateChangedEvent(int statusCode, std::string &statusString, std::string &build_id)
{
	// We don't generally need to process this. However, one occurence is when we first connect, and so it is the
	// earliest opportunity to learn what we're connected to.
	if (statusCode)
	{
		LL_WARNS("Voice") << "VoiceServiceConnectionStateChangedEvent statusCode: " << statusCode <<
			"statusString: " << statusString << LL_ENDL;
		return;
	}
	if (build_id.empty())
	{
		return;
	}
	mVoiceVersion.mBuildVersion = build_id;
}

void LLWebRTCVoiceClient::auxAudioPropertiesEvent(F32 energy)
{
	LL_DEBUGS("VoiceEnergy") << "got energy " << energy << LL_ENDL;
	mTuningEnergy = energy;
}

void LLWebRTCVoiceClient::muteListChanged()
{
	// The user's mute list has been updated.  Go through the current participant list and sync it with the mute list.
	if(mAudioSession)
	{
		participantMap::iterator iter = mAudioSession->mParticipantsByURI.begin();
		
		for(; iter != mAudioSession->mParticipantsByURI.end(); iter++)
		{
			participantStatePtr_t p(iter->second);
			
			// Check to see if this participant is on the mute list already
			if(p->updateMuteState())
				mAudioSession->mVolumeDirty = true;
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
	 mPower(0.f), 
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
		mParticipantsChanged = true;

		result->mAvatarIDValid = true;
        result->mAvatarID      = agent_id;
		
        if(result->updateMuteState())
        {
	        mMuteDirty = true;
        }
		
		mParticipantsByUUID.insert(participantUUIDMap::value_type(result->mAvatarID, result));

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

/*static*/
void LLWebRTCVoiceClient::sessionState::VerifySessions()
{
    std::set<wptr_t>::iterator it = mSession.begin();
    while (it != mSession.end())
    {
        if ((*it).expired())
        {
            LL_WARNS("Voice") << "Expired session found! removing" << LL_ENDL;
            it = mSession.erase(it);
        }
        else
            ++it;
    }
}


void LLWebRTCVoiceClient::getParticipantList(std::set<LLUUID> &participants)
{
	if(mAudioSession)
	{
		for(participantUUIDMap::iterator iter = mAudioSession->mParticipantsByUUID.begin();
			iter != mAudioSession->mParticipantsByUUID.end(); 
			iter++)
		{
			participants.insert(iter->first);
		}
	}
}

bool LLWebRTCVoiceClient::isParticipant(const LLUUID &speaker_id)
{
  if(mAudioSession)
    {
      return (mAudioSession->mParticipantsByUUID.find(speaker_id) != mAudioSession->mParticipantsByUUID.end());
    }
  return false;
}


LLWebRTCVoiceClient::participantStatePtr_t LLWebRTCVoiceClient::sessionState::findParticipant(const std::string &uri)
{
    participantStatePtr_t result;
	
	participantMap::iterator iter = mParticipantsByURI.find(uri);

	if(iter == mParticipantsByURI.end())
	{
		if(!mAlternateSIPURI.empty() && (uri == mAlternateSIPURI))
		{
			// This is a p2p session (probably with the SLIM client) with an alternate URI for the other participant.
			// Look up the other URI
			iter = mParticipantsByURI.find(mSIPURI);
		}
	}

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

LLWebRTCVoiceClient::participantStatePtr_t LLWebRTCVoiceClient::findParticipantByID(const LLUUID& id)
{
    participantStatePtr_t result;
	
	if(mAudioSession)
	{
		result = mAudioSession->findParticipantByID(id);
	}
	
	return result;
}

LLWebRTCVoiceClient::participantStatePtr_t LLWebRTCVoiceClient::addParticipantByID(const LLUUID &id)
{ 
	participantStatePtr_t result;
    if (mAudioSession)
    {
        result = mAudioSession->addParticipant(id);
    }
    return result;
}

void LLWebRTCVoiceClient::removeParticipantByID(const LLUUID &id)
{
    participantStatePtr_t result;
    if (mAudioSession)
    {
        participantStatePtr_t participant = mAudioSession->findParticipantByID(id);
        if (participant)
        {
            mAudioSession->removeParticipant(participant);
        }
    }
}


// Check for parcel boundary crossing
bool LLWebRTCVoiceClient::checkParcelChanged(bool update)
{
	LLViewerRegion *region = gAgent.getRegion();
	LLParcel *parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
	
	if(region && parcel)
	{
		S32 parcelLocalID = parcel->getLocalID();
		std::string regionName = region->getName();
		
		//			LL_DEBUGS("Voice") << "Region name = \"" << regionName << "\", parcel local ID = " << parcelLocalID << ", cap URI = \"" << capURI << "\"" << LL_ENDL;
		
		// The region name starts out empty and gets filled in later.  
		// Also, the cap gets filled in a short time after the region cross, but a little too late for our purposes.
		// If either is empty, wait for the next time around.
		if(!regionName.empty())
		{
			if((parcelLocalID != mCurrentParcelLocalID) || (regionName != mCurrentRegionName))
			{
				// We have changed parcels.  Initiate a parcel channel lookup.
				if (update)
				{
					mCurrentParcelLocalID = parcelLocalID;
					mCurrentRegionName = regionName;
				}
				return true;
			}
		}
	}
	return false;
}

bool LLWebRTCVoiceClient::switchChannel(
	std::string uri,
	bool spatial,
	bool no_reconnect,
	bool is_p2p,
	std::string hash)
{
	bool needsSwitch = !mIsInChannel;
	
    if (mIsInChannel)
    {
        if (mSessionTerminateRequested)
        {
            // If a terminate has been requested, we need to compare against where the URI we're already headed to.
            if(mNextAudioSession)
            {
                if(mNextAudioSession->mSIPURI != uri)
                    needsSwitch = true;
            }
            else
            {
                // mNextAudioSession is null -- this probably means we're on our way back to spatial.
                if(!uri.empty())
                {
                    // We do want to process a switch in this case.
                    needsSwitch = true;
                }
            }
        }
        else
        {
            // Otherwise, compare against the URI we're in now.
            if(mAudioSession)
            {
                if(mAudioSession->mSIPURI != uri)
                {
                    needsSwitch = true;
                }
            }
            else
            {
                if(!uri.empty())
                {
                    // mAudioSession is null -- it's not clear what case would cause this.
                    // For now, log it as a warning and see if it ever crops up.
                    LL_WARNS("Voice") << "No current audio session... Forcing switch" << LL_ENDL;
                    needsSwitch = true;
                }
            }
        }
    }

    if(needsSwitch)
	{
		if(uri.empty())
		{
			// Leave any channel we may be in
			LL_DEBUGS("Voice") << "leaving channel" << LL_ENDL;

            sessionStatePtr_t oldSession = mNextAudioSession;
			mNextAudioSession.reset();

			// The old session may now need to be deleted.
			reapSession(oldSession);

            // If voice was on, turn it off
            if (LLVoiceClient::getInstance()->getUserPTTState())
            {
                LLVoiceClient::getInstance()->setUserPTTState(false);
            }

			notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_VOICE_DISABLED);
		}
		else
		{
			LL_DEBUGS("Voice") << "switching to channel " << uri << LL_ENDL;

			mNextAudioSession = addSession(uri);
			mNextAudioSession->mHash = hash;
			mNextAudioSession->mIsSpatial = spatial;
			mNextAudioSession->mReconnect = !no_reconnect;
			mNextAudioSession->mIsP2P = is_p2p;
		}
		
        if (mIsInChannel)
		{
			// If we're already in a channel, or if we're joining one, terminate
			// so we can rejoin with the new session data.
			sessionTerminate();
		}
	}

    return needsSwitch;
}

void LLWebRTCVoiceClient::joinSession(const sessionStatePtr_t &session)
{
	mNextAudioSession = session;

    if (mIsInChannel)
    {
        // If we're already in a channel, or if we're joining one, terminate
        // so we can rejoin with the new session data.
        sessionTerminate();
    }
}

void LLWebRTCVoiceClient::setNonSpatialChannel(
	const std::string &uri,
	const std::string &credentials)
{
	switchChannel(uri, false, false, false, credentials);
}

bool LLWebRTCVoiceClient::setSpatialChannel(
	const std::string &uri,
	const std::string &credentials)
{
	mSpatialSessionURI = uri;
	mSpatialSessionCredentials = credentials;
	mAreaVoiceDisabled = mSpatialSessionURI.empty();

	LL_DEBUGS("Voice") << "got spatial channel uri: \"" << uri << "\"" << LL_ENDL;
	
	if((mIsInChannel && mAudioSession && !(mAudioSession->mIsSpatial)) || (mNextAudioSession && !(mNextAudioSession->mIsSpatial)))
	{
		// User is in a non-spatial chat or joining a non-spatial chat.  Don't switch channels.
		LL_INFOS("Voice") << "in non-spatial chat, not switching channels" << LL_ENDL;
        return false;
	}
	else
	{
		return switchChannel(mSpatialSessionURI, true, false, false, mSpatialSessionCredentials);
	}
}

void LLWebRTCVoiceClient::callUser(const LLUUID &uuid)
{
	switchChannel(uuid.asString(), false, true, true);
}



void LLWebRTCVoiceClient::endUserIMSession(const LLUUID &uuid)
{

}
bool LLWebRTCVoiceClient::isValidChannel(std::string &sessionHandle)
{
  return(findSession(sessionHandle) != NULL);
	
}
bool LLWebRTCVoiceClient::answerInvite(std::string &sessionHandle)
{
	// this is only ever used to answer incoming p2p call invites.
	
    sessionStatePtr_t session(findSession(sessionHandle));
	if(session)
	{
		session->mIsSpatial = false;
		session->mReconnect = false;	
		session->mIsP2P = true;

		joinSession(session);
		return true;
	}
	
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
    sessionStatePtr_t session(findSession(id));
	
	if(session)
	{
		// this is a p2p session with the indicated caller, or the session with the specified UUID.
		if(session->mSynthesizedCallerID)
			result = FALSE;
	}
	else
	{
		// Didn't find a matching session -- check the current audio session for a matching participant
		if(mAudioSession)
		{
            participantStatePtr_t participant(findParticipantByID(id));
			if(participant)
			{
				result = participant->isAvatar();
			}
		}
	}
	
	return result;
}

// Returns true if calling back the session URI after the session has closed is possible.
// Currently this will be false only for PSTN P2P calls.		
BOOL LLWebRTCVoiceClient::isSessionCallBackPossible(const LLUUID &session_id)
{
	BOOL result = TRUE; 
    sessionStatePtr_t session(findSession(session_id));
	
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
    sessionStatePtr_t session(findSession(session_id));
	
	if(session != NULL)
	{
		result = session->isTextIMPossible();
	}
	
	return result;
}
		

void LLWebRTCVoiceClient::declineInvite(std::string &sessionHandle)
{
    sessionStatePtr_t session(findSession(sessionHandle));
	if(session)
	{
		sessionMediaDisconnectSendMessage(session);
	}
}

void LLWebRTCVoiceClient::leaveNonSpatialChannel()
{
    LL_DEBUGS("Voice") << "Request to leave spacial channel." << LL_ENDL;
	
	// Make sure we don't rejoin the current session.	
    sessionStatePtr_t oldNextSession(mNextAudioSession);
	mNextAudioSession.reset();
	
	// Most likely this will still be the current session at this point, but check it anyway.
	reapSession(oldNextSession);
	
	verifySessionState();
	
	sessionTerminate();
}

std::string LLWebRTCVoiceClient::getCurrentChannel()
{
	std::string result;
	
    if (mIsInChannel && !mSessionTerminateRequested)
	{
		result = getAudioSessionURI();
	}
	
	return result;
}

bool LLWebRTCVoiceClient::inProximalChannel()
{
	bool result = false;
	
    if (mIsInChannel && !mSessionTerminateRequested)
	{
		result = inSpatialChannel();
	}
	
	return result;
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

bool LLWebRTCVoiceClient::inSpatialChannel(void)
{
	bool result = false;
	
	if(mAudioSession)
    {
		result = mAudioSession->mIsSpatial;
    }
    
	return result;
}

std::string LLWebRTCVoiceClient::getAudioSessionURI()
{
	std::string result;
	
	if(mAudioSession)
		result = mAudioSession->mSIPURI;
		
	return result;
}

std::string LLWebRTCVoiceClient::getAudioSessionHandle()
{
	std::string result;
	
	if(mAudioSession)
		result = mAudioSession->mHandle;
		
	return result;
}


/////////////////////////////
// Sending updates of current state

void LLWebRTCVoiceClient::enforceTether(void)
{
	LLVector3d tethered	= mCameraRequestedPosition;

	// constrain 'tethered' to within 50m of mAvatarPosition.
	{
		F32 max_dist = 50.0f;
		LLVector3d camera_offset = mCameraRequestedPosition - mAvatarPosition;
		F32 camera_distance = (F32)camera_offset.magVec();
		if(camera_distance > max_dist)
		{
			tethered = mAvatarPosition + 
				(max_dist / camera_distance) * camera_offset;
		}
	}
	
	if(dist_vec_squared(mCameraPosition, tethered) > 0.01)
	{
		mCameraPosition = tethered;
		mSpatialCoordsDirty = true;
	}
}

void LLWebRTCVoiceClient::updatePosition(void)
{

	LLViewerRegion *region = gAgent.getRegion();
	if(region && isAgentAvatarValid())
	{
		LLVector3d pos;
        LLQuaternion qrot;
		
		// TODO: If camera and avatar velocity are actually used by the voice system, we could compute them here...
		// They're currently always set to zero.
		
		// Send the current camera position to the voice code
		pos = gAgent.getRegion()->getPosGlobalFromRegion(LLViewerCamera::getInstance()->getOrigin());
		
		LLWebRTCVoiceClient::getInstance()->setCameraPosition(
															 pos,				// position
															 LLVector3::zero, 	// velocity
                                                            LLViewerCamera::getInstance()->getQuaternion()); // rotation matrix
		
		// Send the current avatar position to the voice code
        qrot = gAgentAvatarp->getRootJoint()->getWorldRotation();
		pos = gAgentAvatarp->getPositionGlobal();

		// TODO: Can we get the head offset from outside the LLVOAvatar?
		//			pos += LLVector3d(mHeadOffset);
		pos += LLVector3d(0.f, 0.f, 1.f);
		
		LLWebRTCVoiceClient::getInstance()->setAvatarPosition(
															 pos,				// position
															 LLVector3::zero, 	// velocity
															 qrot);				// rotation matrix
	}
}

void LLWebRTCVoiceClient::setCameraPosition(const LLVector3d &position, const LLVector3 &velocity, const LLQuaternion &rot)
{
	mCameraRequestedPosition = position;
	
	if(mCameraVelocity != velocity)
	{
		mCameraVelocity = velocity;
		mSpatialCoordsDirty = true;
	}
	
	if(mCameraRot != rot)
	{
		mCameraRot = rot;
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

void LLWebRTCVoiceClient::leaveChannel(void)
{
    if (mIsInChannel)
	{
		LL_DEBUGS("Voice") << "leaving channel for teleport/logout" << LL_ENDL;
		mChannelName.clear();
		sessionTerminate();
	}
}

void LLWebRTCVoiceClient::setMuteMic(bool muted)
{
    participantStatePtr_t participant = findParticipantByID(gAgentID);
    if (participant)
	{
        participant->mPower = 0.0;
	}
    if (mWebRTCAudioInterface)
	{
        mWebRTCAudioInterface->setMute(muted);
	}
    mMuteMic = muted;
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

            if (!mIsCoroutineActive)
            {
                LLCoros::instance().launch("LLWebRTCVoiceClient::voiceControlCoro",
                    boost::bind(&LLWebRTCVoiceClient::voiceControlCoro, LLWebRTCVoiceClient::getInstance()));
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
            LLMutexLock lock(&mVoiceStateMutex);
            int         min_volume = 0.0;
            if ((volume == min_volume) || (mSpeakerVolume == min_volume))
            {
                mSpeakerMuteDirty = true;
            }

            mSpeakerVolume      = volume;
            mSpeakerVolumeDirty = true;
        }
        if (mWebRTCAudioInterface)
        {
            mWebRTCAudioInterface->setSpeakerVolume(volume);
        }
	}
}

void LLWebRTCVoiceClient::setMicGain(F32 volume)
{
	int scaled_volume = scale_mic_volume(volume);
	
	if(scaled_volume != mMicVolume)
	{
		mMicVolume = scaled_volume;
		mMicVolumeDirty = true;
	}
}

/////////////////////////////
// Accessors for data related to nearby speakers
BOOL LLWebRTCVoiceClient::getVoiceEnabled(const LLUUID& id)
{
	BOOL result = FALSE;
    participantStatePtr_t participant(findParticipantByID(id));
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
    participantStatePtr_t participant(findParticipantByID(id));
	if(participant)
	{
		result = participant->mDisplayName;
	}
	
	return result;
}



BOOL LLWebRTCVoiceClient::getIsSpeaking(const LLUUID& id)
{
	BOOL result = FALSE;

    participantStatePtr_t participant(findParticipantByID(id));
	if(participant)
	{
		result = participant->mIsSpeaking;
	}
	
	return result;
}

BOOL LLWebRTCVoiceClient::getIsModeratorMuted(const LLUUID& id)
{
	BOOL result = FALSE;

    participantStatePtr_t participant(findParticipantByID(id));
	if(participant)
	{
		result = participant->mIsModeratorMuted;
	}
	
	return result;
}

F32 LLWebRTCVoiceClient::getCurrentPower(const LLUUID &id)
{
    F32 result = 0;
	participantStatePtr_t participant(findParticipantByID(id));
	if (participant)
	{
		result = participant->mPower * 2.0;
	}
	return result;
}

BOOL LLWebRTCVoiceClient::getUsingPTT(const LLUUID& id)
{
	BOOL result = FALSE;

    participantStatePtr_t participant(findParticipantByID(id));
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
	
    participantStatePtr_t participant(findParticipantByID(id));
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
	
    participantStatePtr_t participant(findParticipantByID(id));
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
	if(mAudioSession)
	{
        participantStatePtr_t participant(findParticipantByID(id));
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
			mAudioSession->mVolumeDirty = true;
		}
	}
}

std::string LLWebRTCVoiceClient::getGroupID(const LLUUID& id)
{
	std::string result;

    participantStatePtr_t participant(findParticipantByID(id));
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

void LLWebRTCVoiceClient::recordingLoopStart(int seconds, int deltaFramesPerControlFrame)
{
//	LL_DEBUGS("Voice") << "sending SessionGroup.ControlRecording (Start)" << LL_ENDL;
	
	if(!mMainSessionGroupHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.ControlRecording.1\">"
		<< "<SessionGroupHandle>" << mMainSessionGroupHandle << "</SessionGroupHandle>"
		<< "<RecordingControlType>Start</RecordingControlType>" 
		<< "<DeltaFramesPerControlFrame>" << deltaFramesPerControlFrame << "</DeltaFramesPerControlFrame>"
		<< "<Filename>" << "" << "</Filename>"
		<< "<EnableAudioRecordingEvents>false</EnableAudioRecordingEvents>"
		<< "<LoopModeDurationSeconds>" << seconds << "</LoopModeDurationSeconds>"
		<< "</Request>\n\n\n";


		writeString(stream.str());
	}
}

void LLWebRTCVoiceClient::recordingLoopSave(const std::string& filename)
{
//	LL_DEBUGS("Voice") << "sending SessionGroup.ControlRecording (Flush)" << LL_ENDL;

	if(mAudioSession != NULL && !mAudioSession->mGroupHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.ControlRecording.1\">"
		<< "<SessionGroupHandle>" << mMainSessionGroupHandle << "</SessionGroupHandle>"
		<< "<RecordingControlType>Flush</RecordingControlType>" 
		<< "<Filename>" << filename << "</Filename>"
		<< "</Request>\n\n\n";

		writeString(stream.str());
	}
}

void LLWebRTCVoiceClient::recordingStop()
{
//	LL_DEBUGS("Voice") << "sending SessionGroup.ControlRecording (Stop)" << LL_ENDL;

	if(mAudioSession != NULL && !mAudioSession->mGroupHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.ControlRecording.1\">"
		<< "<SessionGroupHandle>" << mMainSessionGroupHandle << "</SessionGroupHandle>"
		<< "<RecordingControlType>Stop</RecordingControlType>" 
		<< "</Request>\n\n\n";

		writeString(stream.str());
	}
}

void LLWebRTCVoiceClient::filePlaybackStart(const std::string& filename)
{
//	LL_DEBUGS("Voice") << "sending SessionGroup.ControlPlayback (Start)" << LL_ENDL;

	if(mAudioSession != NULL && !mAudioSession->mGroupHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.ControlPlayback.1\">"
		<< "<SessionGroupHandle>" << mMainSessionGroupHandle << "</SessionGroupHandle>"
		<< "<RecordingControlType>Start</RecordingControlType>" 
		<< "<Filename>" << filename << "</Filename>"
		<< "</Request>\n\n\n";

		writeString(stream.str());
	}
}

void LLWebRTCVoiceClient::filePlaybackStop()
{
//	LL_DEBUGS("Voice") << "sending SessionGroup.ControlPlayback (Stop)" << LL_ENDL;

	if(mAudioSession != NULL && !mAudioSession->mGroupHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.ControlPlayback.1\">"
		<< "<SessionGroupHandle>" << mMainSessionGroupHandle << "</SessionGroupHandle>"
		<< "<RecordingControlType>Stop</RecordingControlType>" 
		<< "</Request>\n\n\n";

		writeString(stream.str());
	}
}

void LLWebRTCVoiceClient::filePlaybackSetPaused(bool paused)
{
	// TODO: Implement once WebRTC gives me a sample
}

void LLWebRTCVoiceClient::filePlaybackSetMode(bool vox, float speed)
{
	// TODO: Implement once WebRTC gives me a sample
}

//------------------------------------------------------------------------
std::set<LLWebRTCVoiceClient::sessionState::wptr_t> LLWebRTCVoiceClient::sessionState::mSession;


LLWebRTCVoiceClient::sessionState::sessionState() :
    mErrorStatusCode(0),
    mMediaStreamState(streamStateUnknown),
    mCreateInProgress(false),
    mMediaConnectInProgress(false),
    mVoiceInvitePending(false),
    mTextInvitePending(false),
    mSynthesizedCallerID(false),
    mIsChannel(false),
    mIsSpatial(false),
    mIsP2P(false),
    mIncoming(false),
    mVoiceActive(false),
    mReconnect(false),
    mVolumeDirty(false),
    mMuteDirty(false),
    mParticipantsChanged(false)
{
}

/*static*/
LLWebRTCVoiceClient::sessionState::ptr_t LLWebRTCVoiceClient::sessionState::createSession()
{
    sessionState::ptr_t ptr(new sessionState());

    std::pair<std::set<wptr_t>::iterator, bool>  result = mSession.insert(ptr);

    if (result.second)
        ptr->mMyIterator = result.first;

    return ptr;
}

LLWebRTCVoiceClient::sessionState::~sessionState()
{
    LL_INFOS("Voice") << "Destroying session handle=" << mHandle << " SIP=" << mSIPURI << LL_ENDL;
    if (mMyIterator != mSession.end())
        mSession.erase(mMyIterator);

	removeAllParticipants();
}

bool LLWebRTCVoiceClient::sessionState::isCallBackPossible()
{
	// This may change to be explicitly specified by WebRTC in the future...
	// Currently, only PSTN P2P calls cannot be returned.
	// Conveniently, this is also the only case where we synthesize a caller UUID.
	return !mSynthesizedCallerID;
}

bool LLWebRTCVoiceClient::sessionState::isTextIMPossible()
{
	// This may change to be explicitly specified by WebRTC in the future...
	return !mSynthesizedCallerID;
}


/*static*/ 
LLWebRTCVoiceClient::sessionState::ptr_t LLWebRTCVoiceClient::sessionState::matchSessionByHandle(const std::string &handle)
{
    sessionStatePtr_t result;

    // *TODO: My kingdom for a lambda!
    std::set<wptr_t>::iterator it = std::find_if(mSession.begin(), mSession.end(), boost::bind(testByHandle, _1, handle));

    if (it != mSession.end())
        result = (*it).lock();

    return result;
}

/*static*/ 
LLWebRTCVoiceClient::sessionState::ptr_t LLWebRTCVoiceClient::sessionState::matchCreatingSessionByURI(const std::string &uri)
{
    sessionStatePtr_t result;

    // *TODO: My kingdom for a lambda!
    std::set<wptr_t>::iterator it = std::find_if(mSession.begin(), mSession.end(), boost::bind(testByCreatingURI, _1, uri));

    if (it != mSession.end())
        result = (*it).lock();

    return result;
}

/*static*/
LLWebRTCVoiceClient::sessionState::ptr_t LLWebRTCVoiceClient::sessionState::matchSessionByURI(const std::string &uri)
{
    sessionStatePtr_t result;

    // *TODO: My kingdom for a lambda!
    std::set<wptr_t>::iterator it = std::find_if(mSession.begin(), mSession.end(), boost::bind(testBySIPOrAlterateURI, _1, uri));

    if (it != mSession.end())
        result = (*it).lock();

    return result;
}

/*static*/
LLWebRTCVoiceClient::sessionState::ptr_t LLWebRTCVoiceClient::sessionState::matchSessionByParticipant(const LLUUID &participant_id)
{
    sessionStatePtr_t result;

    // *TODO: My kingdom for a lambda!
    std::set<wptr_t>::iterator it = std::find_if(mSession.begin(), mSession.end(), boost::bind(testByCallerId, _1, participant_id));

    if (it != mSession.end())
        result = (*it).lock();

    return result;
}

void LLWebRTCVoiceClient::sessionState::for_each(sessionFunc_t func)
{
    std::for_each(mSession.begin(), mSession.end(), boost::bind(for_eachPredicate, _1, func));
}

// simple test predicates.  
// *TODO: These should be made into lambdas when we can pull the trigger on newer C++ features.
bool LLWebRTCVoiceClient::sessionState::testByHandle(const LLWebRTCVoiceClient::sessionState::wptr_t &a, std::string handle)
{
    ptr_t aLock(a.lock());

    return aLock ? aLock->mHandle == handle : false;
}

bool LLWebRTCVoiceClient::sessionState::testByCreatingURI(const LLWebRTCVoiceClient::sessionState::wptr_t &a, std::string uri)
{
    ptr_t aLock(a.lock());

    return aLock ? (aLock->mCreateInProgress && (aLock->mSIPURI == uri)) : false;
}

bool LLWebRTCVoiceClient::sessionState::testBySIPOrAlterateURI(const LLWebRTCVoiceClient::sessionState::wptr_t &a, std::string uri)
{
    ptr_t aLock(a.lock());

    return aLock ? ((aLock->mSIPURI == uri) || (aLock->mAlternateSIPURI == uri)) : false;
}


bool LLWebRTCVoiceClient::sessionState::testByCallerId(const LLWebRTCVoiceClient::sessionState::wptr_t &a, LLUUID participantId)
{
    ptr_t aLock(a.lock());

    return aLock ? ((aLock->mCallerID == participantId) || (aLock->mIMSessionID == participantId)) : false;
}

/*static*/
void LLWebRTCVoiceClient::sessionState::for_eachPredicate(const LLWebRTCVoiceClient::sessionState::wptr_t &a, sessionFunc_t func)
{
    ptr_t aLock(a.lock());

    if (aLock)
        func(aLock);
    else
    {
        LL_WARNS("Voice") << "Stale handle in session map!" << LL_ENDL;
    }
}

void LLWebRTCVoiceClient::sessionEstablished()
{
	addSession(gAgent.getRegion()->getRegionID().asString()); 
}

LLWebRTCVoiceClient::sessionStatePtr_t LLWebRTCVoiceClient::findSession(const std::string &handle)
{
    sessionStatePtr_t result;
	sessionMap::iterator iter = mSessionsByHandle.find(handle);
	if(iter != mSessionsByHandle.end())
	{
		result = iter->second;
	}
	
	return result;
}

LLWebRTCVoiceClient::sessionStatePtr_t LLWebRTCVoiceClient::findSessionBeingCreatedByURI(const std::string &uri)
{	
    sessionStatePtr_t result = sessionState::matchCreatingSessionByURI(uri);
	
	return result;
}

LLWebRTCVoiceClient::sessionStatePtr_t LLWebRTCVoiceClient::findSession(const LLUUID &participant_id)
{
    sessionStatePtr_t result = sessionState::matchSessionByParticipant(participant_id);
	
	return result;
}

LLWebRTCVoiceClient::sessionStatePtr_t LLWebRTCVoiceClient::addSession(const std::string &uri, const std::string &handle)
{
    sessionStatePtr_t result;
	
	if(handle.empty())
	{
        // No handle supplied.
        // Check whether there's already a session with this URI
        result = sessionState::matchSessionByURI(uri);
	}
	else // (!handle.empty())
	{
		// Check for an existing session with this handle
		sessionMap::iterator iter = mSessionsByHandle.find(handle);
		
		if(iter != mSessionsByHandle.end())
		{
			result = iter->second;
		}
	}

	if(!result)
	{
		// No existing session found.
		
		LL_DEBUGS("Voice") << "adding new session: handle \"" << handle << "\" URI " << uri << LL_ENDL;
        result = sessionState::createSession();
		result->mSIPURI = uri;
		result->mHandle = handle;

		if (LLVoiceClient::instance().getVoiceEffectEnabled())
		{
			result->mVoiceFontID = LLVoiceClient::instance().getVoiceEffectDefault();
		}

		if(!result->mHandle.empty())
		{
            // *TODO: Rider: This concerns me.  There is a path (via switchChannel) where 
            // we do not track the session.  In theory this means that we could end up with 
            // a mAuidoSession that does not match the session tracked in mSessionsByHandle
			mSessionsByHandle.insert(sessionMap::value_type(result->mHandle, result));
		}
	}
	else
	{
		// Found an existing session
		
		if(uri != result->mSIPURI)
		{
			// TODO: Should this be an internal error?
			LL_DEBUGS("Voice") << "changing uri from " << result->mSIPURI << " to " << uri << LL_ENDL;
			setSessionURI(result, uri);
		}

		if(handle != result->mHandle)
		{
			if(handle.empty())
			{
				// There's at least one race condition where where addSession was clearing an existing session handle, which caused things to break.
				LL_DEBUGS("Voice") << "NOT clearing handle " << result->mHandle << LL_ENDL;
			}
			else
			{
				// TODO: Should this be an internal error?
				LL_DEBUGS("Voice") << "changing handle from " << result->mHandle << " to " << handle << LL_ENDL;
				setSessionHandle(result, handle);
			}
		}
		
		LL_DEBUGS("Voice") << "returning existing session: handle " << handle << " URI " << uri << LL_ENDL;
	}

	verifySessionState();
		
	return result;
}

void LLWebRTCVoiceClient::clearSessionHandle(const sessionStatePtr_t &session)
{
    if (session)
    {
        if (!session->mHandle.empty())
        {
            sessionMap::iterator iter = mSessionsByHandle.find(session->mHandle);
            if (iter != mSessionsByHandle.end())
            {
                mSessionsByHandle.erase(iter);
            }
        }
        else
        {
            LL_WARNS("Voice") << "Session has empty handle!" << LL_ENDL;
        }
    }
    else
    {
        LL_WARNS("Voice") << "Attempt to clear NULL session!" << LL_ENDL;
    }

}

void LLWebRTCVoiceClient::setSessionHandle(const sessionStatePtr_t &session, const std::string &handle)
{
	// Have to remove the session from the handle-indexed map before changing the handle, or things will break badly.
	
	if(!session->mHandle.empty())
	{
		// Remove session from the map if it should have been there.
		sessionMap::iterator iter = mSessionsByHandle.find(session->mHandle);
		if(iter != mSessionsByHandle.end())
		{
			if(iter->second != session)
			{
				LL_WARNS("Voice") << "Internal error: session mismatch! Session may have been duplicated. Removing version in map." << LL_ENDL;
			}

			mSessionsByHandle.erase(iter);
		}
		else
		{
            LL_WARNS("Voice") << "Attempt to remove session with handle " << session->mHandle << " not found in map!" << LL_ENDL;
		}
	}
			
	session->mHandle = handle;

	if(!handle.empty())
	{
		mSessionsByHandle.insert(sessionMap::value_type(session->mHandle, session));
	}

	verifySessionState();
}

void LLWebRTCVoiceClient::setSessionURI(const sessionStatePtr_t &session, const std::string &uri)
{
	// There used to be a map of session URIs to sessions, which made this complex....
	session->mSIPURI = uri;

	verifySessionState();
}

void LLWebRTCVoiceClient::deleteSession(const sessionStatePtr_t &session)
{
	// Remove the session from the handle map
	if(!session->mHandle.empty())
	{
		sessionMap::iterator iter = mSessionsByHandle.find(session->mHandle);
		if(iter != mSessionsByHandle.end())
		{
			if(iter->second != session)
			{
				LL_WARNS("Voice") << "Internal error: session mismatch, removing session in map." << LL_ENDL;
			}
			mSessionsByHandle.erase(iter);
		}
	}

	// At this point, the session should be unhooked from all lists and all state should be consistent.
	verifySessionState();

	// If this is the current audio session, clean up the pointer which will soon be dangling.
	if(mAudioSession == session)
	{
		mAudioSession.reset();
		mAudioSessionChanged = true;
	}

	// ditto for the next audio session
	if(mNextAudioSession == session)
	{
		mNextAudioSession.reset();
	}

}

void LLWebRTCVoiceClient::deleteAllSessions()
{
	LL_DEBUGS("Voice") << LL_ENDL;

    while (!mSessionsByHandle.empty())
	{
        const sessionStatePtr_t session = mSessionsByHandle.begin()->second;
        deleteSession(session);
	}
	
}

void LLWebRTCVoiceClient::verifySessionState(void)
{
    LL_DEBUGS("Voice") << "Sessions in handle map=" << mSessionsByHandle.size() << LL_ENDL;
    sessionState::VerifySessions();
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
                       << " mAudioSession=" << mAudioSession
                       << LL_ENDL;

	if(mAudioSession)
	{
		if(status == LLVoiceClientStatusObserver::ERROR_UNKNOWN)
		{
			switch(mAudioSession->mErrorStatusCode)
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
			mAudioSession->mErrorStatusCode = 0;
		}
		else if(status == LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL)
		{
			switch(mAudioSession->mErrorStatusCode)
			{
				case HTTP_NOT_FOUND:	// NOT_FOUND
				// *TODO: Should this be 503?
				case 480:	// TEMPORARILY_UNAVAILABLE
				case HTTP_REQUEST_TIME_OUT:	// REQUEST_TIMEOUT
					// call failed because other user was not available
					// treat this as an error case
					status = LLVoiceClientStatusObserver::ERROR_NOT_AVAILABLE;

					// Reset the error code to make sure it won't be reused later by accident.
					mAudioSession->mErrorStatusCode = 0;
				break;
			}
		}
	}
		
	LL_DEBUGS("Voice") 
		<< " " << LLVoiceClientStatusObserver::status2string(status)  
		<< ", session URI " << getAudioSessionURI() 
		<< ", proximal is " << inSpatialChannel()
        << LL_ENDL;

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
        participant->mAccountName = name;
        // and post a "participants updated" message to listeners later.
        session->mParticipantsChanged = true;
    }

    // Check whether this is a p2p session whose caller name just resolved
    if (session->mCallerID == id)
    {
        // this session's "caller ID" just resolved.  Fill in the name.
        session->mName = name;
        if (session->mTextInvitePending)
        {
            session->mTextInvitePending = false;

            // We don't need to call LLIMMgr::getInstance()->addP2PSession() here.  The first incoming message will create the panel.				
        }
        if (session->mVoiceInvitePending)
        {
            session->mVoiceInvitePending = false;

            LLIMMgr::getInstance()->inviteToSession(
                session->mIMSessionID,
                session->mName,
                session->mCallerID,
                session->mName,
                IM_SESSION_P2P_INVITE,
                LLIMMgr::INVITATION_TYPE_VOICE,
                session->mHandle,
                session->mSIPURI);
        }

    }
}

void LLWebRTCVoiceClient::avatarNameResolved(const LLUUID &id, const std::string &name)
{
    sessionState::for_each(boost::bind(predAvatarNameResolution, _1, id, name));
}

bool LLWebRTCVoiceClient::setVoiceEffect(const LLUUID& id)
{
	if (!mAudioSession)
	{
		return false;
	}

	if (!id.isNull())
	{
		if (mVoiceFontMap.empty())
		{
			LL_DEBUGS("Voice") << "Voice fonts not available." << LL_ENDL;
			return false;
		}
		else if (mVoiceFontMap.find(id) == mVoiceFontMap.end())
		{
			LL_DEBUGS("Voice") << "Invalid voice font " << id << LL_ENDL;
			return false;
		}
	}

	// *TODO: Check for expired fonts?
	mAudioSession->mVoiceFontID = id;

	// *TODO: Separate voice font defaults for spatial chat and IM?
	gSavedPerAccountSettings.setString("VoiceEffectDefault", id.asString());

	sessionSetVoiceFontSendMessage(mAudioSession);
	notifyVoiceFontObservers();

	return true;
}

const LLUUID LLWebRTCVoiceClient::getVoiceEffect()
{
	return mAudioSession ? mAudioSession->mVoiceFontID : LLUUID::null;
}

LLSD LLWebRTCVoiceClient::getVoiceEffectProperties(const LLUUID& id)
{
	LLSD sd;

	voice_font_map_t::iterator iter = mVoiceFontMap.find(id);
	if (iter != mVoiceFontMap.end())
	{
		sd["template_only"] = false;
	}
	else
	{
		// Voice effect is not in the voice font map, see if there is a template
		iter = mVoiceFontTemplateMap.find(id);
		if (iter == mVoiceFontTemplateMap.end())
		{
			LL_WARNS("Voice") << "Voice effect " << id << "not found." << LL_ENDL;
			return sd;
		}
		sd["template_only"] = true;
	}

	voiceFontEntry *font = iter->second;
	sd["name"] = font->mName;
	sd["expiry_date"] = font->mExpirationDate;
	sd["is_new"] = font->mIsNew;

	return sd;
}

LLWebRTCVoiceClient::voiceFontEntry::voiceFontEntry(LLUUID& id) :
	mID(id),
	mFontIndex(0),
	mFontType(VOICE_FONT_TYPE_NONE),
	mFontStatus(VOICE_FONT_STATUS_NONE),
	mIsNew(false)
{
	mExpiryTimer.stop();
	mExpiryWarningTimer.stop();
}

LLWebRTCVoiceClient::voiceFontEntry::~voiceFontEntry()
{
}

void LLWebRTCVoiceClient::refreshVoiceEffectLists(bool clear_lists)
{
	if (clear_lists)
	{
		mVoiceFontsReceived = false;
		deleteAllVoiceFonts();
		deleteVoiceFontTemplates();
	}

	accountGetSessionFontsSendMessage();
	accountGetTemplateFontsSendMessage();
}

const voice_effect_list_t& LLWebRTCVoiceClient::getVoiceEffectList() const
{
	return mVoiceFontList;
}

const voice_effect_list_t& LLWebRTCVoiceClient::getVoiceEffectTemplateList() const
{
	return mVoiceFontTemplateList;
}

void LLWebRTCVoiceClient::addVoiceFont(const S32 font_index,
								 const std::string &name,
								 const std::string &description,
								 const LLDate &expiration_date,
								 bool has_expired,
								 const S32 font_type,
								 const S32 font_status,
								 const bool template_font)
{
	// WebRTC SessionFontIDs are not guaranteed to remain the same between
	// sessions or grids so use a UUID for the name.

	// If received name is not a UUID, fudge one by hashing the name and type.
	LLUUID font_id;
	if (LLUUID::validate(name))
	{
		font_id = LLUUID(name);
	}
	else
	{
		font_id.generate(STRINGIZE(font_type << ":" << name));
	}

	voiceFontEntry *font = NULL;

	voice_font_map_t& font_map = template_font ? mVoiceFontTemplateMap : mVoiceFontMap;
	voice_effect_list_t& font_list = template_font ? mVoiceFontTemplateList : mVoiceFontList;

	// Check whether we've seen this font before.
	voice_font_map_t::iterator iter = font_map.find(font_id);
	bool new_font = (iter == font_map.end());

	// Override the has_expired flag if we have passed the expiration_date as a double check.
	if (expiration_date.secondsSinceEpoch() < (LLDate::now().secondsSinceEpoch() + VOICE_FONT_EXPIRY_INTERVAL))
	{
		has_expired = true;
	}

	if (has_expired)
	{
		LL_DEBUGS("VoiceFont") << "Expired " << (template_font ? "Template " : "")
		<< expiration_date.asString() << " " << font_id
		<< " (" << font_index << ") " << name << LL_ENDL;

		// Remove existing session fonts that have expired since we last saw them.
		if (!new_font && !template_font)
		{
			deleteVoiceFont(font_id);
		}
		return;
	}

	if (new_font)
	{
		// If it is a new font create a new entry.
		font = new voiceFontEntry(font_id);
	}
	else
	{
		// Not a new font, update the existing entry
		font = iter->second;
	}

	if (font)
	{
		font->mFontIndex = font_index;
		// Use the description for the human readable name if available, as the
		// "name" may be a UUID.
		font->mName = description.empty() ? name : description;
		font->mFontType = font_type;
		font->mFontStatus = font_status;

		// If the font is new or the expiration date has changed the expiry timers need updating.
		if (!template_font && (new_font || font->mExpirationDate != expiration_date))
		{
			font->mExpirationDate = expiration_date;

			// Set the expiry timer to trigger a notification when the voice font can no longer be used.
			font->mExpiryTimer.start();
			font->mExpiryTimer.setExpiryAt(expiration_date.secondsSinceEpoch() - VOICE_FONT_EXPIRY_INTERVAL);

			// Set the warning timer to some interval before actual expiry.
			S32 warning_time = gSavedSettings.getS32("VoiceEffectExpiryWarningTime");
			if (warning_time != 0)
			{
				font->mExpiryWarningTimer.start();
				F64 expiry_time = (expiration_date.secondsSinceEpoch() - (F64)warning_time);
				font->mExpiryWarningTimer.setExpiryAt(expiry_time - VOICE_FONT_EXPIRY_INTERVAL);
			}
			else
			{
				// Disable the warning timer.
				font->mExpiryWarningTimer.stop();
			}

			 // Only flag new session fonts after the first time we have fetched the list.
			if (mVoiceFontsReceived)
			{
				font->mIsNew = true;
				mVoiceFontsNew = true;
			}
		}

		LL_DEBUGS("VoiceFont") << (template_font ? "Template " : "")
			<< font->mExpirationDate.asString() << " " << font->mID
			<< " (" << font->mFontIndex << ") " << name << LL_ENDL;

		if (new_font)
		{
			font_map.insert(voice_font_map_t::value_type(font->mID, font));
			font_list.insert(voice_effect_list_t::value_type(font->mName, font->mID));
		}

		mVoiceFontListDirty = true;

		// Debugging stuff

		if (font_type < VOICE_FONT_TYPE_NONE || font_type >= VOICE_FONT_TYPE_UNKNOWN)
		{
			LL_WARNS("VoiceFont") << "Unknown voice font type: " << font_type << LL_ENDL;
		}
		if (font_status < VOICE_FONT_STATUS_NONE || font_status >= VOICE_FONT_STATUS_UNKNOWN)
		{
			LL_WARNS("VoiceFont") << "Unknown voice font status: " << font_status << LL_ENDL;
		}
	}
}

void LLWebRTCVoiceClient::expireVoiceFonts()
{
	// *TODO: If we are selling voice fonts in packs, there are probably
	// going to be a number of fonts with the same expiration time, so would
	// be more efficient to just keep a list of expiration times rather
	// than checking each font individually.

	bool have_expired = false;
	bool will_expire = false;
	bool expired_in_use = false;

	LLUUID current_effect = LLVoiceClient::instance().getVoiceEffectDefault();

	voice_font_map_t::iterator iter;
	for (iter = mVoiceFontMap.begin(); iter != mVoiceFontMap.end(); ++iter)
	{
		voiceFontEntry* voice_font = iter->second;
		LLFrameTimer& expiry_timer  = voice_font->mExpiryTimer;
		LLFrameTimer& warning_timer = voice_font->mExpiryWarningTimer;

		// Check for expired voice fonts
		if (expiry_timer.getStarted() && expiry_timer.hasExpired())
		{
			// Check whether it is the active voice font
			if (voice_font->mID == current_effect)
			{
				// Reset to no voice effect.
				setVoiceEffect(LLUUID::null);
				expired_in_use = true;
			}

			LL_DEBUGS("Voice") << "Voice Font " << voice_font->mName << " has expired." << LL_ENDL;
			deleteVoiceFont(voice_font->mID);
			have_expired = true;
		}

		// Check for voice fonts that will expire in less that the warning time
		if (warning_timer.getStarted() && warning_timer.hasExpired())
		{
			LL_DEBUGS("VoiceFont") << "Voice Font " << voice_font->mName << " will expire soon." << LL_ENDL;
			will_expire = true;
			warning_timer.stop();
		}
	}

	LLSD args;
	args["URL"] = LLTrans::getString("voice_morphing_url");
	args["PREMIUM_URL"] = LLTrans::getString("premium_voice_morphing_url");

	// Give a notification if any voice fonts have expired.
	if (have_expired)
	{
		if (expired_in_use)
		{
			LLNotificationsUtil::add("VoiceEffectsExpiredInUse", args);
		}
		else
		{
			LLNotificationsUtil::add("VoiceEffectsExpired", args);
		}

		// Refresh voice font lists in the UI.
		notifyVoiceFontObservers();
	}

	// Give a warning notification if any voice fonts are due to expire.
	if (will_expire)
	{
		S32Seconds seconds(gSavedSettings.getS32("VoiceEffectExpiryWarningTime"));
		args["INTERVAL"] = llformat("%d", LLUnit<S32, LLUnits::Days>(seconds).value());

		LLNotificationsUtil::add("VoiceEffectsWillExpire", args);
	}
}

void LLWebRTCVoiceClient::deleteVoiceFont(const LLUUID& id)
{
	// Remove the entry from the voice font list.
	voice_effect_list_t::iterator list_iter = mVoiceFontList.begin();
	while (list_iter != mVoiceFontList.end())
	{
		if (list_iter->second == id)
		{
			LL_DEBUGS("VoiceFont") << "Removing " << id << " from the voice font list." << LL_ENDL;
            list_iter = mVoiceFontList.erase(list_iter);
			mVoiceFontListDirty = true;
		}
		else
		{
			++list_iter;
		}
	}

	// Find the entry in the voice font map and erase its data.
	voice_font_map_t::iterator map_iter = mVoiceFontMap.find(id);
	if (map_iter != mVoiceFontMap.end())
	{
		delete map_iter->second;
	}

	// Remove the entry from the voice font map.
	mVoiceFontMap.erase(map_iter);
}

void LLWebRTCVoiceClient::deleteAllVoiceFonts()
{
	mVoiceFontList.clear();

	voice_font_map_t::iterator iter;
	for (iter = mVoiceFontMap.begin(); iter != mVoiceFontMap.end(); ++iter)
	{
		delete iter->second;
	}
	mVoiceFontMap.clear();
}

void LLWebRTCVoiceClient::deleteVoiceFontTemplates()
{
	mVoiceFontTemplateList.clear();

	voice_font_map_t::iterator iter;
	for (iter = mVoiceFontTemplateMap.begin(); iter != mVoiceFontTemplateMap.end(); ++iter)
	{
		delete iter->second;
	}
	mVoiceFontTemplateMap.clear();
}

S32 LLWebRTCVoiceClient::getVoiceFontIndex(const LLUUID& id) const
{
	S32 result = 0;
	if (!id.isNull())
	{
		voice_font_map_t::const_iterator it = mVoiceFontMap.find(id);
		if (it != mVoiceFontMap.end())
		{
			result = it->second->mFontIndex;
		}
		else
		{
			LL_WARNS("VoiceFont") << "Selected voice font " << id << " is not available." << LL_ENDL;
		}
	}
	return result;
}

S32 LLWebRTCVoiceClient::getVoiceFontTemplateIndex(const LLUUID& id) const
{
	S32 result = 0;
	if (!id.isNull())
	{
		voice_font_map_t::const_iterator it = mVoiceFontTemplateMap.find(id);
		if (it != mVoiceFontTemplateMap.end())
		{
			result = it->second->mFontIndex;
		}
		else
		{
			LL_WARNS("VoiceFont") << "Selected voice font template " << id << " is not available." << LL_ENDL;
		}
	}
	return result;
}

void LLWebRTCVoiceClient::accountGetSessionFontsSendMessage()
{
	if(mAccountLoggedIn)
	{
		std::ostringstream stream;

		LL_DEBUGS("VoiceFont") << "Requesting voice font list." << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.GetSessionFonts.1\">"
		<< "<AccountHandle>" << LLWebRTCSecurity::getInstance()->accountHandle() << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLWebRTCVoiceClient::accountGetTemplateFontsSendMessage()
{
	if(mAccountLoggedIn)
	{
		std::ostringstream stream;

		LL_DEBUGS("VoiceFont") << "Requesting voice font template list." << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.GetTemplateFonts.1\">"
		<< "<AccountHandle>" << LLWebRTCSecurity::getInstance()->accountHandle() << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLWebRTCVoiceClient::sessionSetVoiceFontSendMessage(const sessionStatePtr_t &session)
{
	S32 font_index = getVoiceFontIndex(session->mVoiceFontID);
	LL_DEBUGS("VoiceFont") << "Requesting voice font: " << session->mVoiceFontID << " (" << font_index << "), session handle: " << session->mHandle << LL_ENDL;

	std::ostringstream stream;

	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.SetVoiceFont.1\">"
	<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
	<< "<SessionFontID>" << font_index << "</SessionFontID>"
	<< "</Request>\n\n\n";

	writeString(stream.str());
}

void LLWebRTCVoiceClient::accountGetSessionFontsResponse(int statusCode, const std::string &statusString)
{
    if (mIsWaitingForFonts)
    {
        // *TODO: We seem to get multiple events of this type.  Should figure a way to advance only after
        // receiving the last one.
        LLSD result(LLSDMap("voice_fonts", LLSD::Boolean(true)));

        mWebRTCPump.post(result);
    }
	notifyVoiceFontObservers();
	mVoiceFontsReceived = true;
}

void LLWebRTCVoiceClient::accountGetTemplateFontsResponse(int statusCode, const std::string &statusString)
{
	// Voice font list entries were updated via addVoiceFont() during parsing.
	notifyVoiceFontObservers();
}
void LLWebRTCVoiceClient::addObserver(LLVoiceEffectObserver* observer)
{
	mVoiceFontObservers.insert(observer);
}

void LLWebRTCVoiceClient::removeObserver(LLVoiceEffectObserver* observer)
{
	mVoiceFontObservers.erase(observer);
}

// method checks the item in VoiceMorphing menu for appropriate current voice font
bool LLWebRTCVoiceClient::onCheckVoiceEffect(const std::string& voice_effect_name)
{
	LLVoiceEffectInterface * effect_interfacep = LLVoiceClient::instance().getVoiceEffectInterface();
	if (NULL != effect_interfacep)
	{
		const LLUUID& currect_voice_effect_id = effect_interfacep->getVoiceEffect();

		if (currect_voice_effect_id.isNull())
		{
			if (voice_effect_name == "NoVoiceMorphing")
			{
				return true;
			}
		}
		else
		{
			const LLSD& voice_effect_props = effect_interfacep->getVoiceEffectProperties(currect_voice_effect_id);
			if (voice_effect_props["name"].asString() == voice_effect_name)
			{
				return true;
			}
		}
	}

	return false;
}

// method changes voice font for selected VoiceMorphing menu item
void LLWebRTCVoiceClient::onClickVoiceEffect(const std::string& voice_effect_name)
{
	LLVoiceEffectInterface * effect_interfacep = LLVoiceClient::instance().getVoiceEffectInterface();
	if (NULL != effect_interfacep)
	{
		if (voice_effect_name == "NoVoiceMorphing")
		{
			effect_interfacep->setVoiceEffect(LLUUID());
			return;
		}
		const voice_effect_list_t& effect_list = effect_interfacep->getVoiceEffectList();
		if (!effect_list.empty())
		{
			for (voice_effect_list_t::const_iterator it = effect_list.begin(); it != effect_list.end(); ++it)
			{
				if (voice_effect_name == it->first)
				{
					effect_interfacep->setVoiceEffect(it->second);
					return;
				}
			}
		}
	}
}

// it updates VoiceMorphing menu items in accordance with purchased properties 
void LLWebRTCVoiceClient::updateVoiceMorphingMenu()
{
	if (mVoiceFontListDirty)
	{
		LLVoiceEffectInterface * effect_interfacep = LLVoiceClient::instance().getVoiceEffectInterface();
		if (effect_interfacep)
		{
			const voice_effect_list_t& effect_list = effect_interfacep->getVoiceEffectList();
			if (!effect_list.empty())
			{
				LLMenuGL * voice_morphing_menup = gMenuBarView->findChildMenuByName("VoiceMorphing", TRUE);

				if (NULL != voice_morphing_menup)
				{
					S32 items = voice_morphing_menup->getItemCount();
					if (items > 0)
					{
						voice_morphing_menup->erase(1, items - 3, false);

						S32 pos = 1;
						for (voice_effect_list_t::const_iterator it = effect_list.begin(); it != effect_list.end(); ++it)
						{
							LLMenuItemCheckGL::Params p;
							p.name = it->first;
							p.label = it->first;
							p.on_check.function(boost::bind(&LLWebRTCVoiceClient::onCheckVoiceEffect, this, it->first));
							p.on_click.function(boost::bind(&LLWebRTCVoiceClient::onClickVoiceEffect, this, it->first));
							LLMenuItemCheckGL * voice_effect_itemp = LLUICtrlFactory::create<LLMenuItemCheckGL>(p);
							voice_morphing_menup->insert(pos++, voice_effect_itemp, false);
						}

						voice_morphing_menup->needsArrange();
					}
				}
			}
		}
	}
}
void LLWebRTCVoiceClient::notifyVoiceFontObservers()
{
    LL_DEBUGS("VoiceFont") << "Notifying voice effect observers. Lists changed: " << mVoiceFontListDirty << LL_ENDL;

    updateVoiceMorphingMenu();

    for (voice_font_observer_set_t::iterator it = mVoiceFontObservers.begin();
            it != mVoiceFontObservers.end();)
    {
        LLVoiceEffectObserver* observer = *it;
        observer->onVoiceEffectChanged(mVoiceFontListDirty);
        // In case onVoiceEffectChanged() deleted an entry.
        it = mVoiceFontObservers.upper_bound(observer);
    }
    mVoiceFontListDirty = false;

	// If new Voice Fonts have been added notify the user.
    if (mVoiceFontsNew)
    {
        if (mVoiceFontsReceived)
        {
            LLNotificationsUtil::add("VoiceEffectsNew");
        }
        mVoiceFontsNew = false;
    }
}

void LLWebRTCVoiceClient::enablePreviewBuffer(bool enable)
{
    LLSD result;
    mCaptureBufferMode = enable;

    if (enable)
        result["recplay"] = "start";
    else
        result["recplay"] = "quit";

    mWebRTCPump.post(result);

	if(mCaptureBufferMode && mIsInChannel)
	{
		LL_DEBUGS("Voice") << "no channel" << LL_ENDL;
		sessionTerminate();
	}
}

void LLWebRTCVoiceClient::recordPreviewBuffer()
{
	if (!mCaptureBufferMode)
	{
		LL_DEBUGS("Voice") << "Not in voice effect preview mode, cannot start recording." << LL_ENDL;
		mCaptureBufferRecording = false;
		return;
	}

	mCaptureBufferRecording = true;

    LLSD result(LLSDMap("recplay", "record"));
    mWebRTCPump.post(result);
}

void LLWebRTCVoiceClient::playPreviewBuffer(const LLUUID& effect_id)
{
	if (!mCaptureBufferMode)
	{
		LL_DEBUGS("Voice") << "Not in voice effect preview mode, no buffer to play." << LL_ENDL;
		mCaptureBufferRecording = false;
		return;
	}

	if (!mCaptureBufferRecorded)
	{
		// Can't play until we have something recorded!
		mCaptureBufferPlaying = false;
		return;
	}

	mPreviewVoiceFont = effect_id;
	mCaptureBufferPlaying = true;

    LLSD result(LLSDMap("recplay", "playback"));
    mWebRTCPump.post(result);
}

void LLWebRTCVoiceClient::stopPreviewBuffer()
{
	mCaptureBufferRecording = false;
	mCaptureBufferPlaying = false;

    LLSD result(LLSDMap("recplay", "quit"));
    mWebRTCPump.post(result);
}

bool LLWebRTCVoiceClient::isPreviewRecording()
{
	return (mCaptureBufferMode && mCaptureBufferRecording);
}

bool LLWebRTCVoiceClient::isPreviewPlaying()
{
	return (mCaptureBufferMode && mCaptureBufferPlaying);
}

void LLWebRTCVoiceClient::captureBufferRecordStartSendMessage()
{	if(mAccountLoggedIn)
	{
		std::ostringstream stream;

		LL_DEBUGS("Voice") << "Starting audio capture to buffer." << LL_ENDL;

		// Start capture
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.StartBufferCapture.1\">"
		<< "</Request>"
		<< "\n\n\n";

		// Unmute the mic
		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.MuteLocalMic.1\">"
			<< "<ConnectorHandle>" << LLWebRTCSecurity::getInstance()->connectorHandle() << "</ConnectorHandle>"
			<< "<Value>false</Value>"
		<< "</Request>\n\n\n";

		// Dirty the mute mic state so that it will get reset when we finishing previewing
		mMuteMicDirty = true;

		writeString(stream.str());
	}
}

void LLWebRTCVoiceClient::captureBufferRecordStopSendMessage()
{
	if(mAccountLoggedIn)
	{
		std::ostringstream stream;

		LL_DEBUGS("Voice") << "Stopping audio capture to buffer." << LL_ENDL;

		// Mute the mic. Mic mute state was dirtied at recording start, so will be reset when finished previewing.
		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.MuteLocalMic.1\">"
			<< "<ConnectorHandle>" << LLWebRTCSecurity::getInstance()->connectorHandle() << "</ConnectorHandle>"
			<< "<Value>true</Value>"
		<< "</Request>\n\n\n";

		// Stop capture
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.CaptureAudioStop.1\">"
			<< "<AccountHandle>" << LLWebRTCSecurity::getInstance()->accountHandle() << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLWebRTCVoiceClient::captureBufferPlayStartSendMessage(const LLUUID& voice_font_id)
{
	if(mAccountLoggedIn)
	{
		// Track how may play requests are sent, so we know how many stop events to
		// expect before play actually stops.
		++mPlayRequestCount;

		std::ostringstream stream;

		LL_DEBUGS("Voice") << "Starting audio buffer playback." << LL_ENDL;

		S32 font_index = getVoiceFontTemplateIndex(voice_font_id);
		LL_DEBUGS("Voice") << "With voice font: " << voice_font_id << " (" << font_index << ")" << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.PlayAudioBuffer.1\">"
			<< "<AccountHandle>" << LLWebRTCSecurity::getInstance()->accountHandle() << "</AccountHandle>"
			<< "<TemplateFontID>" << font_index << "</TemplateFontID>"
			<< "<FontDelta />"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLWebRTCVoiceClient::captureBufferPlayStopSendMessage()
{
	if(mAccountLoggedIn)
	{
		std::ostringstream stream;

		LL_DEBUGS("Voice") << "Stopping audio buffer playback." << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.RenderAudioStop.1\">"
			<< "<AccountHandle>" << LLWebRTCSecurity::getInstance()->accountHandle() << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

std::string LLWebRTCVoiceClient::sipURIFromID(const LLUUID& id) { return id.asString(); }


LLWebRTCSecurity::LLWebRTCSecurity()
{
    // This size is an arbitrary choice; WebRTC does not care
    // Use a multiple of three so that there is no '=' padding in the base64 (purely an esthetic choice)
    #define WebRTC_TOKEN_BYTES 9
    U8  random_value[WebRTC_TOKEN_BYTES];

    for (int b = 0; b < WebRTC_TOKEN_BYTES; b++)
    {
        random_value[b] = ll_rand() & 0xff;
    }
    mConnectorHandle = LLBase64::encode(random_value, WebRTC_TOKEN_BYTES);
    
    for (int b = 0; b < WebRTC_TOKEN_BYTES; b++)
    {
        random_value[b] = ll_rand() & 0xff;
    }
    mAccountHandle = LLBase64::encode(random_value, WebRTC_TOKEN_BYTES);
}

LLWebRTCSecurity::~LLWebRTCSecurity()
{
}
