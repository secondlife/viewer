 /** 
 * @file LLVivoxVoiceClient.cpp
 * @brief Implementation of LLVivoxVoiceClient class which is the interface to the voice client process.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
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

#include "llviewerprecompiledheaders.h"
#include <algorithm>
#include "llvoicevivox.h"

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
#include "llviewerregion.h"
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
#include "llviewerwindow.h"
#include "llviewercamera.h"
#include "llversioninfo.h"

#include "llviewernetwork.h"
#include "llnotificationsutil.h"

#include "llcorehttputil.h"
#include "lleventfilter.h"

#include "stringize.h"

// for base64 decoding
#include "apr_base64.h"

#define USE_SESSION_GROUPS 0
#define VX_NULL_POSITION -2147483648.0 /*The Silence*/

extern LLMenuBarGL* gMenuBarView;
extern void handle_voice_morphing_subscribe();

namespace {
    const F32 VOLUME_SCALE_VIVOX = 0.01f;

    const F32 SPEAKING_TIMEOUT = 1.f;

    static const std::string VOICE_SERVER_TYPE = "Vivox";

    // Don't retry connecting to the daemon more frequently than this:
    const F32 DAEMON_CONNECT_THROTTLE_SECONDS = 1.0f;
    const int DAEMON_CONNECT_RETRY_MAX = 3;

    // Don't send positional updates more frequently than this:
    const F32 UPDATE_THROTTLE_SECONDS = 0.5f;

    // Timeout for connection to Vivox 
    const F32 CONNECT_ATTEMPT_TIMEOUT = 300.0f;
    const F32 CONNECT_DNS_TIMEOUT = 5.0f;
    const int CONNECT_RETRY_MAX = 3;

    const F32 LOGIN_ATTEMPT_TIMEOUT = 30.0f;
    const F32 LOGOUT_ATTEMPT_TIMEOUT = 5.0f;
    const int LOGIN_RETRY_MAX = 3;

    const F32 PROVISION_RETRY_TIMEOUT = 2.0;
    const int PROVISION_RETRY_MAX = 5;

    // Cosine of a "trivially" small angle
    const F32 FOUR_DEGREES = 4.0f * (F_PI / 180.0f);
    const F32 MINUSCULE_ANGLE_COS = (F32) cos(0.5f * FOUR_DEGREES);

    const F32 SESSION_JOIN_TIMEOUT = 30.0f;

    // Defines the maximum number of times(in a row) "stateJoiningSession" case for spatial channel is reached in stateMachine()
    // which is treated as normal. The is the number of frames to wait for a channel join before giving up.  This was changed 
    // from the original count of 50 for two reason.  Modern PCs have higher frame rates and sometimes the SLVoice process 
    // backs up processing join requests.  There is a log statement that records when channel joins take longer than 100 frames.
    const int MAX_NORMAL_JOINING_SPATIAL_NUM = 1500;

    // How often to check for expired voice fonts in seconds
    const F32 VOICE_FONT_EXPIRY_INTERVAL = 10.f;
    // Time of day at which Vivox expires voice font subscriptions.
    // Used to replace the time portion of received expiry timestamps.
    static const std::string VOICE_FONT_EXPIRY_TIME = "T05:00:00Z";

    // Maximum length of capture buffer recordings in seconds.
    const F32 CAPTURE_BUFFER_MAX_TIME = 10.f;

    const int ERROR_VIVOX_OBJECT_NOT_FOUND = 1001;
    const int ERROR_VIVOX_NOT_LOGGED_IN = 1007;
}

static int scale_mic_volume(float volume)
{
	// incoming volume has the range [0.0 ... 2.0], with 1.0 as the default.                                                
	// Map it to Vivox levels as follows: 0.0 -> 30, 1.0 -> 50, 2.0 -> 70                                                   
	return 30 + (int)(volume * 20.0f);
}

static int scale_speaker_volume(float volume)
{
	// incoming volume has the range [0.0 ... 1.0], with 0.5 as the default.                                                
	// Map it to Vivox levels as follows: 0.0 -> 30, 0.5 -> 50, 1.0 -> 70                                                   
	return 30 + (int)(volume * 40.0f);
	
}


///////////////////////////////////////////////////////////////////////////////////////////////

class LLVivoxVoiceClientMuteListObserver : public LLMuteListObserver
{
	/* virtual */ void onChange()  { LLVivoxVoiceClient::getInstance()->muteListChanged();}
};


void LLVoiceVivoxStats::reset()
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

LLVoiceVivoxStats::LLVoiceVivoxStats()
{
    reset();
}

LLVoiceVivoxStats::~LLVoiceVivoxStats()
{
}

void LLVoiceVivoxStats::connectionAttemptStart()
{
    if (!mConnectAttempts)
    {
        mStartTime = LLTimer::getTotalTime();
        mConnectCycles++;
    }
    mConnectAttempts++;
}

void LLVoiceVivoxStats::connectionAttemptEnd(bool success)
{
    if ( success )
    {
        mConnectTime = (LLTimer::getTotalTime() - mStartTime) / USEC_PER_SEC;
    }
}

void LLVoiceVivoxStats::provisionAttemptStart()
{
    if (!mProvisionAttempts)
    {
        mStartTime = LLTimer::getTotalTime();
    }
    mProvisionAttempts++;
}

void LLVoiceVivoxStats::provisionAttemptEnd(bool success)
{
    if ( success )
    {
        mProvisionTime = (LLTimer::getTotalTime() - mStartTime) / USEC_PER_SEC;
    }
}

void LLVoiceVivoxStats::establishAttemptStart()
{
    if (!mEstablishAttempts)
    {
        mStartTime = LLTimer::getTotalTime();
    }
    mEstablishAttempts++;
}

void LLVoiceVivoxStats::establishAttemptEnd(bool success)
{
    if ( success )
    {
        mEstablishTime = (LLTimer::getTotalTime() - mStartTime) / USEC_PER_SEC;
    }
}

LLSD LLVoiceVivoxStats::read()
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

static LLVivoxVoiceClientMuteListObserver mutelist_listener;
static bool sMuteListListener_listening = false;

///////////////////////////////////////////////////////////////////////////////////////////////
static LLProcessPtr sGatewayPtr;
static LLEventStream sGatewayPump("VivoxDaemonPump", true);

static bool isGatewayRunning()
{
	return sGatewayPtr && sGatewayPtr->isRunning();
}

static void killGateway()
{
	if (sGatewayPtr)
	{
        LL_DEBUGS("Voice") << "SLVoice " << sGatewayPtr->getStatusString() << LL_ENDL;

		sGatewayPump.stopListening("VivoxDaemonPump");
		sGatewayPtr->kill(__FUNCTION__);
        sGatewayPtr=NULL;
	}
    else
    {
        LL_DEBUGS("Voice") << "no gateway" << LL_ENDL;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////

bool LLVivoxVoiceClient::sShuttingDown = false;
bool LLVivoxVoiceClient::sConnected = false;
LLPumpIO *LLVivoxVoiceClient::sPump = nullptr;

LLVivoxVoiceClient::LLVivoxVoiceClient() :
	mSessionTerminateRequested(false),
	mRelogRequested(false),
	mTerminateDaemon(false),
	mSpatialJoiningNum(0),

	mTuningMode(false),
	mTuningEnergy(0.0f),
	mTuningMicVolume(0),
	mTuningMicVolumeDirty(true),
	mTuningSpeakerVolume(50),     // Set to 50 so the user can hear himself when he sets his mic volume
	mTuningSpeakerVolumeDirty(true),
	mDevicesListUpdated(false),

	mAreaVoiceDisabled(false),
	mAudioSession(), // TBD - should be NULL
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

	mCaptureDeviceDirty(false),
	mRenderDeviceDirty(false),
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
	mVivoxPump("vivoxClientPump")
{
    sShuttingDown = false;
    sConnected = false;
    sPump = nullptr;

	mSpeakerVolume = scale_speaker_volume(0);

	mVoiceVersion.serverVersion = "";
	mVoiceVersion.serverType = VOICE_SERVER_TYPE;
	
	//  gMuteListp isn't set up at this point, so we defer this until later.
//	gMuteListp->addObserver(&mutelist_listener);

	
#if LL_DARWIN || LL_LINUX
		// HACK: THIS DOES NOT BELONG HERE
		// When the vivox daemon dies, the next write attempt on our socket generates a SIGPIPE, which kills us.
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

LLVivoxVoiceClient::~LLVivoxVoiceClient()
{
	if (mAvatarNameCacheConnection.connected())
	{
		mAvatarNameCacheConnection.disconnect();
	}
    sShuttingDown = true;
}

//---------------------------------------------------

void LLVivoxVoiceClient::init(LLPumpIO *pump)
{
	// constructor will set up LLVoiceClient::getInstance()
	sPump = pump;

//     LLCoros::instance().launch("LLVivoxVoiceClient::voiceControlCoro",
//         boost::bind(&LLVivoxVoiceClient::voiceControlCoro, LLVivoxVoiceClient::getInstance()));

}

void LLVivoxVoiceClient::terminate()
{
    if (sShuttingDown)
    {
        return;
    }

    // needs to be done manually here since we will not get another pass in 
    // coroutines... that mechanism is long since gone.
    if (mIsLoggedIn)
    {
        logoutOfVivox(false);
    }
    
	if(sConnected)
	{
        breakVoiceConnection(false);
        sConnected = false;
	}
	else
	{
		mRelogRequested = false;
		killGateway();
	}

    sShuttingDown = true;
    sPump = NULL;
}

//---------------------------------------------------

void LLVivoxVoiceClient::cleanUp()
{
    LL_DEBUGS("Voice") << LL_ENDL;
    
	deleteAllSessions();
	deleteAllVoiceFonts();
	deleteVoiceFontTemplates();
    LL_DEBUGS("Voice") << "exiting" << LL_ENDL;
}

//---------------------------------------------------

const LLVoiceVersionInfo& LLVivoxVoiceClient::getVersion()
{
	return mVoiceVersion;
}

//---------------------------------------------------

void LLVivoxVoiceClient::updateSettings()
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

bool LLVivoxVoiceClient::writeString(const std::string &str)
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
			LL_WARNS("Voice") << ") short write on socket sending data to vivox daemon." << "Sent " << written << "bytes instead of " << size <<LL_ENDL;
		}
		else if(APR_STATUS_IS_EAGAIN(err))
		{
			char buf[MAX_STRING];
			LL_WARNS("Voice") << "EAGAIN error " << err << " (" << apr_strerror(err, buf, MAX_STRING) << ") sending data to vivox daemon." << LL_ENDL;
		}
		else
		{
			// Assume any socket error means something bad.  For now, just close the socket.
			char buf[MAX_STRING];
			LL_WARNS("Voice") << "apr error " << err << " ("<< apr_strerror(err, buf, MAX_STRING) << ") sending data to vivox daemon." << LL_ENDL;
			daemonDied();
		}
	}
		
	return result;
}


/////////////////////////////
// session control messages
void LLVivoxVoiceClient::connectorCreate()
{
	std::ostringstream stream;
	std::string logdir = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "");
	
	// Transition to stateConnectorStarted when the connector handle comes back.
	std::string vivoxLogLevel = gSavedSettings.getString("VivoxDebugLevel");
    if ( vivoxLogLevel.empty() )
    {
        vivoxLogLevel = "0";
    }
    LL_DEBUGS("Voice") << "creating connector with log level " << vivoxLogLevel << LL_ENDL;
	
	stream 
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.Create.1\">"
		<< "<ClientName>V2 SDK</ClientName>"
		<< "<AccountManagementServer>" << mVoiceAccountServerURI << "</AccountManagementServer>"
		<< "<Mode>Normal</Mode>"
        << "<ConnectorHandle>" << LLVivoxSecurity::getInstance()->connectorHandle() << "</ConnectorHandle>"
		<< "<Logging>"
		<< "<Folder>" << logdir << "</Folder>"
		<< "<FileNamePrefix>Connector</FileNamePrefix>"
		<< "<FileNameSuffix>.log</FileNameSuffix>"
		<< "<LogLevel>" << vivoxLogLevel << "</LogLevel>"
		<< "</Logging>"
		<< "<Application>" << LLVersionInfo::instance().getChannel() << " " << LLVersionInfo::instance().getVersion() << "</Application>"
		//<< "<Application></Application>"  //Name can cause problems per vivox.
		<< "<MaxCalls>12</MaxCalls>"
		<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVivoxVoiceClient::connectorShutdown()
{
	if(mConnectorEstablished)
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.InitiateShutdown.1\">"
			<< "<ConnectorHandle>" << LLVivoxSecurity::getInstance()->connectorHandle() << "</ConnectorHandle>"
		<< "</Request>"
		<< "\n\n\n";
		
		mShutdownComplete = false;
		mConnectorEstablished = false;
		
		writeString(stream.str());
	}
	else
	{
		mShutdownComplete = true;
	}
}

void LLVivoxVoiceClient::userAuthorized(const std::string& user_id, const LLUUID &agentID)
{

	mAccountDisplayName = user_id;

	LL_INFOS("Voice") << "name \"" << mAccountDisplayName << "\" , ID " << agentID << LL_ENDL;

	mAccountName = nameFromID(agentID);
}

void LLVivoxVoiceClient::setLoginInfo(
	const std::string& account_name,
	const std::string& password,
	const std::string& voice_sip_uri_hostname,
	const std::string& voice_account_server_uri)
{
	mVoiceSIPURIHostName = voice_sip_uri_hostname;
	mVoiceAccountServerURI = voice_account_server_uri;

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

	std::string debugSIPURIHostName = gSavedSettings.getString("VivoxDebugSIPURIHostName");
	
	if( !debugSIPURIHostName.empty() )
	{
        LL_INFOS("Voice") << "Overriding account server based on VivoxDebugSIPURIHostName: "
                          << debugSIPURIHostName << LL_ENDL;
		mVoiceSIPURIHostName = debugSIPURIHostName;
	}
	
	if( mVoiceSIPURIHostName.empty() )
	{
		// we have an empty account server name
		// so we fall back to hardcoded defaults

		if(LLGridManager::getInstance()->isInProductionGrid())
		{
			// Use the release account server
			mVoiceSIPURIHostName = "bhr.vivox.com";
		}
		else
		{
			// Use the development account server
			mVoiceSIPURIHostName = "bhd.vivox.com";
		}
        LL_INFOS("Voice") << "Defaulting SIP URI host: "
                          << mVoiceSIPURIHostName << LL_ENDL;

	}
	
	std::string debugAccountServerURI = gSavedSettings.getString("VivoxDebugVoiceAccountServerURI");

	if( !debugAccountServerURI.empty() )
	{
        LL_INFOS("Voice") << "Overriding account server based on VivoxDebugVoiceAccountServerURI: "
                          << debugAccountServerURI << LL_ENDL;
		mVoiceAccountServerURI = debugAccountServerURI;
	}
	
	if( mVoiceAccountServerURI.empty() )
	{
		// If the account server URI isn't specified, construct it from the SIP URI hostname
		mVoiceAccountServerURI = "https://www." + mVoiceSIPURIHostName + "/api2/";		
        LL_INFOS("Voice") << "Inferring account server based on SIP URI Host name: "
                          << mVoiceAccountServerURI << LL_ENDL;
	}
}

void LLVivoxVoiceClient::idle(void* user_data)
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
    VOICE_STATE_DONE = 0,
    VOICE_STATE_TP_WAIT, // entry point
    VOICE_STATE_START_DAEMON,
    VOICE_STATE_PROVISION_ACCOUNT,
    VOICE_STATE_START_SESSION,
    VOICE_STATE_SESSION_RETRY,
    VOICE_STATE_SESSION_ESTABLISHED,
    VOICE_STATE_WAIT_FOR_CHANNEL,
    VOICE_STATE_DISCONNECT,
    VOICE_STATE_WAIT_FOR_EXIT,
} EVoiceControlCoroState;

void LLVivoxVoiceClient::voiceControlCoro()
{
    int state = 0;
    try
    {
        // state is passed as a reference instead of being
        // a member due to unresolved issues with coroutine
        // surviving longer than LLVivoxVoiceClient
        voiceControlStateMachine(state);
    }
    catch (const LLCoros::Stop&)
    {
        LL_DEBUGS("LLVivoxVoiceClient") << "Received a shutdown exception" << LL_ENDL;
    }
    catch (const LLContinueError&)
    {
        LOG_UNHANDLED_EXCEPTION("LLVivoxVoiceClient");
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

void LLVivoxVoiceClient::voiceControlStateMachine(S32 &coro_state)
{
    if (sShuttingDown)
    {
        return;
    }

    LL_DEBUGS("Voice") << "starting" << LL_ENDL;
    mIsCoroutineActive = true;
    LLCoros::set_consuming(true);

    U32 retry = 0;

    coro_state = VOICE_STATE_TP_WAIT;

    do
    {
        if (sShuttingDown)
        {
            // Vivox singleton performed the exit, logged out,
            // cleaned sockets, gateway and no longer cares
            // about state of coroutine, so just stop
            return;
        }

        switch (coro_state)
        {
        case VOICE_STATE_TP_WAIT:
            // starting point for voice
            if (gAgent.getTeleportState() != LLAgent::TELEPORT_NONE)
            {
                LL_DEBUGS("Voice") << "Suspending voiceControlCoro() momentarily for teleport. Tuning: " << mTuningMode << ". Relog: " << mRelogRequested << LL_ENDL;
                llcoro::suspendUntilTimeout(1.0);
            }
            else
            {
                coro_state = VOICE_STATE_START_DAEMON;
            }
            break;

        case VOICE_STATE_START_DAEMON:
            LL_DEBUGS("Voice") << "Launching daemon" << LL_ENDL;
            LLVoiceVivoxStats::getInstance()->reset();
            if (startAndLaunchDaemon())
            {
                coro_state = VOICE_STATE_PROVISION_ACCOUNT;
            }
            else
            {
                coro_state = VOICE_STATE_SESSION_RETRY;
            }
            break;

        case VOICE_STATE_PROVISION_ACCOUNT:
            if (provisionVoiceAccount())
            {
                coro_state = VOICE_STATE_START_SESSION;
            }
            else
            {
                coro_state = VOICE_STATE_SESSION_RETRY;
            }
            break;

        case VOICE_STATE_START_SESSION:
            if (establishVoiceConnection())
            {
                coro_state = VOICE_STATE_SESSION_ESTABLISHED;
            }
            else
            {
                coro_state = VOICE_STATE_SESSION_RETRY;
            }
            break;

        case VOICE_STATE_SESSION_RETRY:
            giveUp(); // cleans sockets and session
            if (mRelogRequested)
            {
                // We failed to connect, give it a bit time before retrying.
                retry++;
                F32 full_delay = llmin(5.f * (F32)retry, 60.f);
                F32 current_delay = 0.f;
                LL_INFOS("Voice") << "Voice failed to establish session after " << retry
                                  << " tries. Will attempt to reconnect in " << full_delay
                                  << " seconds" << LL_ENDL;
                while (current_delay < full_delay && !sShuttingDown)
                {
                    // Assuming that a second has passed is not accurate,
                    // but we don't need accurancy here, just to make sure
                    // that some time passed and not to outlive voice itself
                    current_delay++;
                    llcoro::suspendUntilTimeout(1.f);
                }
                coro_state = VOICE_STATE_WAIT_FOR_EXIT;
            }
            else
            {
                coro_state = VOICE_STATE_DONE;
            }
            break;

        case VOICE_STATE_SESSION_ESTABLISHED:
            {
                // enable/disable the automatic VAD and explicitly set the initial values of 
                // the VAD variables ourselves when it is off - see SL-15072 for more details
                // note: we set the other parameters too even if the auto VAD is on which is ok
                unsigned int vad_auto = gSavedSettings.getU32("VivoxVadAuto");
                unsigned int vad_hangover = gSavedSettings.getU32("VivoxVadHangover");
                unsigned int vad_noise_floor = gSavedSettings.getU32("VivoxVadNoiseFloor");
                unsigned int vad_sensitivity = gSavedSettings.getU32("VivoxVadSensitivity");
                setupVADParams(vad_auto, vad_hangover, vad_noise_floor, vad_sensitivity);

                // watch for changes to the VAD settings via Debug Settings UI and act on them accordingly
                gSavedSettings.getControl("VivoxVadAuto")->getSignal()->connect(boost::bind(&LLVivoxVoiceClient::onVADSettingsChange, this));
                gSavedSettings.getControl("VivoxVadHangover")->getSignal()->connect(boost::bind(&LLVivoxVoiceClient::onVADSettingsChange, this));
                gSavedSettings.getControl("VivoxVadNoiseFloor")->getSignal()->connect(boost::bind(&LLVivoxVoiceClient::onVADSettingsChange, this));
                gSavedSettings.getControl("VivoxVadSensitivity")->getSignal()->connect(boost::bind(&LLVivoxVoiceClient::onVADSettingsChange, this));

                if (mTuningMode)
                {
                    performMicTuning();
                }

                coro_state = VOICE_STATE_WAIT_FOR_CHANNEL;
            }
            break;

        case VOICE_STATE_WAIT_FOR_CHANNEL:
            waitForChannel(); // todo: split into more states like login/fonts
            coro_state = VOICE_STATE_DISCONNECT;
            break;

        case VOICE_STATE_DISCONNECT:
            LL_DEBUGS("Voice") << "lost channel RelogRequested=" << mRelogRequested << LL_ENDL;
            endAndDisconnectSession();
            retry = 0; // Connected without issues
            coro_state = VOICE_STATE_WAIT_FOR_EXIT;
            break;

        case VOICE_STATE_WAIT_FOR_EXIT:
            if (isGatewayRunning())
            {
                LL_INFOS("Voice") << "waiting for SLVoice to exit" << LL_ENDL;
                llcoro::suspendUntilTimeout(1.0);
            }
            else if (mRelogRequested && mVoiceEnabled)
            {
                LL_INFOS("Voice") << "will attempt to reconnect to voice" << LL_ENDL;
                coro_state = VOICE_STATE_TP_WAIT;
            }
            else
            {
                coro_state = VOICE_STATE_DONE;
            }
            break;

        case VOICE_STATE_DONE:
            break;
        }
    } while (coro_state > 0);

    if (sShuttingDown)
    {
        // LLVivoxVoiceClient might be already dead
        return;
    }

    mIsCoroutineActive = false;
    LL_INFOS("Voice") << "exiting" << LL_ENDL;
}

bool LLVivoxVoiceClient::endAndDisconnectSession()
{
    LL_DEBUGS("Voice") << LL_ENDL;

    breakVoiceConnection(true);

    killGateway();

    return true;
}

bool LLVivoxVoiceClient::callbackEndDaemon(const LLSD& data)
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
    sGatewayPump.stopListening("VivoxDaemonPump");
    return false;
}

bool LLVivoxVoiceClient::startAndLaunchDaemon()
{
    //---------------------------------------------------------------------
    if (!voiceEnabled())
    {
        // Voice is locked out, we must not launch the vivox daemon.
        LL_WARNS("Voice") << "voice disabled; not starting daemon" << LL_ENDL;
        return false;
    }

    if (!isGatewayRunning())
    {
#ifndef VIVOXDAEMON_REMOTEHOST
        // Launch the voice daemon
#if LL_WINDOWS
        // On windows use exe (not work or RO) directory
        std::string exe_path = gDirUtilp->getExecutableDir();
        gDirUtilp->append(exe_path, "SLVoice.exe");
#elif LL_DARWIN
        // On MAC use resource directory
        std::string exe_path = gDirUtilp->getAppRODataDir();
        gDirUtilp->append(exe_path, "SLVoice");
#else
        std::string exe_path = gDirUtilp->getExecutableDir();
        gDirUtilp->append(exe_path, "SLVoice");
#endif
        // See if the vivox executable exists
        llstat s;
        if (!LLFile::stat(exe_path, &s))
        {
            // vivox executable exists.  Build the command line and launch the daemon.
            LLProcess::Params params;
            params.executable = exe_path;

            // VOICE-88: Cycle through [portbase..portbase+portrange) on
            // successive tries because attempting to relaunch (after manually
            // disabling and then re-enabling voice) with the same port can
            // cause SLVoice's bind() call to fail with EADDRINUSE. We expect
            // that eventually the OS will time out previous ports, which is
            // why we cycle instead of incrementing indefinitely.
            U32 portbase = gSavedSettings.getU32("VivoxVoicePort");
            static U32 portoffset = 0;
            static const U32 portrange = 100;
            std::string host(gSavedSettings.getString("VivoxVoiceHost"));
            U32 port = portbase + portoffset;
            portoffset = (portoffset + 1) % portrange;
            params.args.add("-i");
            params.args.add(STRINGIZE(host << ':' << port));

            std::string loglevel = gSavedSettings.getString("VivoxDebugLevel");
            if (loglevel.empty())
            {
                loglevel = "0";
            }
            params.args.add("-ll");
            params.args.add(loglevel);

            std::string log_folder = gSavedSettings.getString("VivoxLogDirectory");

            if (log_folder.empty())
            {
                log_folder = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "");
            }

            params.args.add("-lf");
            params.args.add(log_folder);

            // set log file basename and .log
            params.args.add("-lp");
            params.args.add("SLVoice");
            params.args.add("-ls");
            params.args.add(".log");

            // rotate any existing log
            std::string new_log = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "SLVoice.log");
            std::string old_log = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "SLVoice.old");
            if (gDirUtilp->fileExists(new_log))
            {
                LLFile::rename(new_log, old_log);
            }
            
            std::string shutdown_timeout = gSavedSettings.getString("VivoxShutdownTimeout");
            if (!shutdown_timeout.empty())
            {
                params.args.add("-st");
                params.args.add(shutdown_timeout);
            }
            params.cwd = gDirUtilp->getAppRODataDir();

#           ifdef VIVOX_HANDLE_ARGS
            params.args.add("-ah");
            params.args.add(LLVivoxSecurity::getInstance()->accountHandle());

            params.args.add("-ch");
            params.args.add(LLVivoxSecurity::getInstance()->connectorHandle());
#           endif // VIVOX_HANDLE_ARGS

            params.postend = sGatewayPump.getName();
            sGatewayPump.listen("VivoxDaemonPump", boost::bind(&LLVivoxVoiceClient::callbackEndDaemon, this, _1));

            LL_INFOS("Voice") << "Launching SLVoice" << LL_ENDL;
            LL_DEBUGS("Voice") << "SLVoice params " << params << LL_ENDL;

            sGatewayPtr = LLProcess::create(params);

            mDaemonHost = LLHost(host.c_str(), port);
        }
        else
        {
            LL_WARNS("Voice") << exe_path << " not found." << LL_ENDL;
            return false;
        }
#else
        // SLIM SDK: port changed from 44124 to 44125.
        // We can connect to a client gateway running on another host.  This is useful for testing.
        // To do this, launch the gateway on a nearby host like this:
        //  vivox-gw.exe -p tcp -i 0.0.0.0:44125
        // and put that host's IP address here.
        mDaemonHost = LLHost(gSavedSettings.getString("VivoxVoiceHost"), gSavedSettings.getU32("VivoxVoicePort"));
#endif

        // Dirty the states we'll need to sync with the daemon when it comes up.
        mMuteMicDirty = true;
        mMicVolumeDirty = true;
        mSpeakerVolumeDirty = true;
        mSpeakerMuteDirty = true;
        // These only need to be set if they're not default (i.e. empty string).
        mCaptureDeviceDirty = !mCaptureDevice.empty();
        mRenderDeviceDirty = !mRenderDevice.empty();

        mMainSessionGroupHandle.clear();
    }
    else
    {
        LL_DEBUGS("Voice") << " gateway running; not attempting to start" << LL_ENDL;
    }

    //---------------------------------------------------------------------
    llcoro::suspendUntilTimeout(UPDATE_THROTTLE_SECONDS);

    LL_DEBUGS("Voice") << "Connecting to vivox daemon:" << mDaemonHost << LL_ENDL;

    int retryCount(0);
    LLVoiceVivoxStats::getInstance()->reset();
    while (!sConnected && !sShuttingDown && retryCount++ <= DAEMON_CONNECT_RETRY_MAX)
    {
        LLVoiceVivoxStats::getInstance()->connectionAttemptStart();
        LL_DEBUGS("Voice") << "Attempting to connect to vivox daemon: " << mDaemonHost << LL_ENDL;
        closeSocket();
        if (!mSocket)
        {
            mSocket = LLSocket::create(gAPRPoolp, LLSocket::STREAM_TCP);
        }

        sConnected = mSocket->blockingConnect(mDaemonHost);
        LLVoiceVivoxStats::getInstance()->connectionAttemptEnd(sConnected);
        if (!sConnected)
        {
            llcoro::suspendUntilTimeout(DAEMON_CONNECT_THROTTLE_SECONDS);
        }
    }
    
    //---------------------------------------------------------------------
    if (sShuttingDown && !sConnected)
    {
        return false;
    }

    llcoro::suspendUntilTimeout(UPDATE_THROTTLE_SECONDS);

    while (!sPump && !sShuttingDown)
    {   // Can't use the pump until we have it available.
        llcoro::suspend();
    }

    if (sShuttingDown)
    {
        return false;
    }
    
    // MBW -- Note to self: pumps and pipes examples in
    //  indra/test/io.cpp
    //  indra/test/llpipeutil.{cpp|h}

    // Attach the pumps and pipes

    LLPumpIO::chain_t readChain;

    readChain.push_back(LLIOPipe::ptr_t(new LLIOSocketReader(mSocket)));
    readChain.push_back(LLIOPipe::ptr_t(new LLVivoxProtocolParser()));


    sPump->addChain(readChain, NEVER_CHAIN_EXPIRY_SECS);


    //---------------------------------------------------------------------
    llcoro::suspendUntilTimeout(UPDATE_THROTTLE_SECONDS);

    // Initial devices query
    getCaptureDevicesSendMessage();
    getRenderDevicesSendMessage();

    mLoginRetryCount = 0;

    return true;
}

bool LLVivoxVoiceClient::provisionVoiceAccount()
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

    LLCore::HttpRequest::policy_t httpPolicy(LLCore::HttpRequest::DEFAULT_POLICY_ID);
    LLCoreHttpUtil::HttpCoroutineAdapter::ptr_t
        httpAdapter(new LLCoreHttpUtil::HttpCoroutineAdapter("voiceAccountProvision", httpPolicy));
    LLCore::HttpRequest::ptr_t httpRequest(new LLCore::HttpRequest);
    LLCore::HttpOptions::ptr_t httpOpts = LLCore::HttpOptions::ptr_t(new LLCore::HttpOptions);
    int retryCount(0);

    LLSD result;
    bool provisioned = false;
    do
    {
        LLVoiceVivoxStats::getInstance()->provisionAttemptStart();
        result = httpAdapter->postAndSuspend(httpRequest, url, LLSD(), httpOpts);

        if (sShuttingDown)
        {
            return false;
        }

        LLSD httpResults = result[LLCoreHttpUtil::HttpCoroutineAdapter::HTTP_RESULTS];
        LLCore::HttpStatus status = LLCoreHttpUtil::HttpCoroutineAdapter::getStatusFromLLSD(httpResults);

        if (status == LLCore::HttpStatus(404))
        {
            F32 timeout = pow(PROVISION_RETRY_TIMEOUT, static_cast<float>(retryCount));
            LL_WARNS("Voice") << "Provision CAP 404.  Retrying in " << timeout << " seconds. Retries: " << (S32)retryCount << LL_ENDL;
            llcoro::suspendUntilTimeout(timeout);

            if (sShuttingDown)
            {
                return false;
            }
        }
        else if (!status)
        {
            LL_WARNS("Voice") << "Unable to provision voice account." << LL_ENDL;
            LLVoiceVivoxStats::getInstance()->provisionAttemptEnd(false);
            return false;
        }
        else
        {
            provisioned = true;
        }        
    } while (!provisioned && ++retryCount <= PROVISION_RETRY_MAX && !sShuttingDown);

    if (sShuttingDown && !provisioned)
    {
        return false;
    }

    LLVoiceVivoxStats::getInstance()->provisionAttemptEnd(provisioned);
    if (!provisioned)
    {
        LL_WARNS("Voice") << "Could not access voice provision cap after " << retryCount << " attempts." << LL_ENDL;
        return false;
    }

    std::string voiceSipUriHostname;
    std::string voiceAccountServerUri;
    std::string voiceUserName = result["username"].asString();
    std::string voicePassword = result["password"].asString();

    if (result.has("voice_sip_uri_hostname"))
    {
        voiceSipUriHostname = result["voice_sip_uri_hostname"].asString();
    }
    
    // this key is actually misnamed -- it will be an entire URI, not just a hostname.
    if (result.has("voice_account_server_name"))
    {
        voiceAccountServerUri = result["voice_account_server_name"].asString();
    }

    LL_DEBUGS("Voice") << "ProvisionVoiceAccountRequest response"
                       << " user " << (voiceUserName.empty() ? "not set" : "set")
                       << " password " << (voicePassword.empty() ? "not set" : "set")
                       << " sip uri " << voiceSipUriHostname
                       << " account uri " << voiceAccountServerUri
                       << LL_ENDL;

    setLoginInfo(voiceUserName, voicePassword, voiceSipUriHostname, voiceAccountServerUri);

    return true;
}

bool LLVivoxVoiceClient::establishVoiceConnection()
{
    if (!mVoiceEnabled && mIsInitialized)
    {
        LL_WARNS("Voice") << "cannot establish connection; enabled "<<mVoiceEnabled<<" initialized "<<mIsInitialized<<LL_ENDL;
        return false;
    }

    if (sShuttingDown)
    {
        return false;
    }
    
    LLSD result;
    bool connected(false);
    bool giving_up(false);
    int retries = 0;
    LL_INFOS("Voice") << "Requesting connection to voice service" << LL_ENDL;

    LLVoiceVivoxStats::getInstance()->establishAttemptStart();
    connectorCreate();
    do
    {
        result = llcoro::suspendUntilEventOn(mVivoxPump);
        LL_DEBUGS("Voice") << "event=" << ll_stream_notation_sd(result) << LL_ENDL;

        if (result.has("connector"))
        {
            connected = LLSD::Boolean(result["connector"]);
            LLVoiceVivoxStats::getInstance()->establishAttemptEnd(connected);
            if (!connected)
            {
                if (result.has("retry") && ++retries <= CONNECT_RETRY_MAX && !sShuttingDown)
                {
                    F32 timeout = LLSD::Real(result["retry"]);
                    timeout *= retries;
                    LL_INFOS("Voice") << "Retry connection to voice service in " << timeout << " seconds" << LL_ENDL;
                    llcoro::suspendUntilTimeout(timeout);

                    if (mVoiceEnabled) // user may have switched it off
                    {
                        // try again
                        LLVoiceVivoxStats::getInstance()->establishAttemptStart();
                        connectorCreate();
                    }
                    else
                    {
                        // stop if they've turned off voice
                        giving_up = true;
                    }
                }
                else
                {
                    giving_up=true;
                }
            }
        }
        LL_DEBUGS("Voice") << (connected ? "" : "not ") << "connected, "
                           << (giving_up ? "" : "not ") << "giving up"
                           << LL_ENDL;
    } while (!connected && !giving_up && !sShuttingDown);

    if (giving_up)
    {
        LLSD args;
        args["HOSTID"] = LLURI(mVoiceAccountServerURI).authority();
        LLNotificationsUtil::add("NoVoiceConnect", args);
    }

    return connected;
}

bool LLVivoxVoiceClient::breakVoiceConnection(bool corowait)
{
    LL_DEBUGS("Voice") << "( wait=" << corowait << ")" << LL_ENDL;
    bool retval(true);

    mShutdownComplete = false;
    connectorShutdown();

    if (corowait)
    {
        LLSD timeoutResult(LLSDMap("connector", "timeout"));

        LLSD result = llcoro::suspendUntilEventOnWithTimeout(mVivoxPump, LOGOUT_ATTEMPT_TIMEOUT, timeoutResult);
        LL_DEBUGS("Voice") << "event=" << ll_stream_notation_sd(result) << LL_ENDL;

        retval = result.has("connector");
    }
    else
    {
        mRelogRequested = false; //stop the control coro
        // If we are not doing a corowait then we must sleep until the connector has responded
        // otherwise we may very well close the socket too early.
#if LL_WINDOWS
        if (!mShutdownComplete)
        {
            // The situation that brings us here is a call from ::terminate()
            // At this point message system is already down so we can't wait for
            // the message, yet we need to receive "connector shutdown response".
            // Either wait a bit and emulate it or check gMessageSystem for specific message
            _sleep(1000);
            if (sConnected)
            {
                sConnected = false;
                LLSD vivoxevent(LLSDMap("connector", LLSD::Boolean(false)));
                mVivoxPump.post(vivoxevent);
            }
            mShutdownComplete = true;
        }
#endif
    }

    LL_DEBUGS("Voice") << "closing SLVoice socket" << LL_ENDL;
    closeSocket();		// Need to do this now -- bad things happen if the destructor does it later.
    cleanUp();
    sConnected = false;

    return retval;
}

bool LLVivoxVoiceClient::loginToVivox()
{
    LLSD timeoutResult(LLSDMap("login", "timeout"));

    int loginRetryCount(0);

    bool response_ok(false);
    bool account_login(false);
    bool send_login(true);

    do 
    {
        mIsLoggingIn = true;
        if (send_login)
        {
            loginSendMessage();
            send_login = false;
        }
        
        LLSD result = llcoro::suspendUntilEventOnWithTimeout(mVivoxPump, LOGIN_ATTEMPT_TIMEOUT, timeoutResult);

        if (sShuttingDown)
        {
            return false;
        }

        LL_DEBUGS("Voice") << "event=" << ll_stream_notation_sd(result) << LL_ENDL;

        if (result.has("login"))
        {
            std::string loginresp = result["login"];

            if (((loginresp == "retry") || (loginresp == "timeout")) && !sShuttingDown)
            {
                LL_WARNS("Voice") << "login failed with status '" << loginresp << "' "
                                  << " count " << loginRetryCount << "/" << LOGIN_RETRY_MAX
                                  << LL_ENDL;
                if (++loginRetryCount > LOGIN_RETRY_MAX)
                {
                    // We've run out of retries - tell the user
                    LL_WARNS("Voice") << "too many login retries (" << loginRetryCount << "); giving up." << LL_ENDL;
                    LLSD args;
                    args["HOSTID"] = LLURI(mVoiceAccountServerURI).authority();
                    mTerminateDaemon = true;
                    LLNotificationsUtil::add("NoVoiceConnect", args);

                    mIsLoggingIn = false;
                    return false;
                }
                response_ok = false;
                account_login = false;
                send_login = true;

                // an exponential backoff gets too long too quickly; stretch it out, but not too much
                F32 timeout = loginRetryCount * LOGIN_ATTEMPT_TIMEOUT;

                // tell the user there is a problem
                LL_WARNS("Voice") << "login " << loginresp << " will retry login in " << timeout << " seconds." << LL_ENDL;
                
                if (!sShuttingDown)
                {
                    // Todo: this is way to long, viewer can get stuck waiting during shutdown
                    // either make it listen to pump or split in smaller waits with checks for shutdown
                    llcoro::suspendUntilTimeout(timeout);
                }
            }
            else if (loginresp == "failed")
            {
                mIsLoggingIn = false;
                return false;
            }
            else if (loginresp == "response_ok")
            {
                response_ok = true;
            }
            else if (loginresp == "account_login")
            {
                account_login = true;
            }
            else if (sShuttingDown)
            {
                mIsLoggingIn = false;
                return false;
            }
        }

    } while ((!response_ok || !account_login) && !sShuttingDown);

    if (sShuttingDown)
    {
        return false;
    }

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

void LLVivoxVoiceClient::logoutOfVivox(bool wait)
{
    if (mIsLoggedIn)
    {
        // Ensure that we'll re-request provisioning before logging in again
        mAccountPassword.clear();
        mVoiceAccountServerURI.clear();

        logoutSendMessage();

        if (wait)
        {
            LLSD timeoutResult(LLSDMap("logout", "timeout"));
            LLSD result;

            do
            {
                LL_DEBUGS("Voice")
                    << "waiting for logout response on "
                    << mVivoxPump.getName()
                    << LL_ENDL;

                result = llcoro::suspendUntilEventOnWithTimeout(mVivoxPump, LOGOUT_ATTEMPT_TIMEOUT, timeoutResult);

                if (sShuttingDown)
                {
                    break;
                }

                LL_DEBUGS("Voice") << "event=" << ll_stream_notation_sd(result) << LL_ENDL;
                // Don't get confused by prior queued events -- note that it's
                // very important that mVivoxPump is an LLEventMailDrop, which
                // does queue events.
            } while (! result["logout"]);
        }
        else
        {
            LL_DEBUGS("Voice") << "not waiting for logout" << LL_ENDL;
        }

        mIsLoggedIn = false;
    }
}


bool LLVivoxVoiceClient::retrieveVoiceFonts()
{
    // Request the set of available voice fonts.
    refreshVoiceEffectLists(true);

    mIsWaitingForFonts = true;
    LLSD result;
    do 
    {
        result = llcoro::suspendUntilEventOn(mVivoxPump);

        LL_DEBUGS("Voice") << "event=" << ll_stream_notation_sd(result) << LL_ENDL;
        if (result.has("voice_fonts"))
            break;
    } while (true);
    mIsWaitingForFonts = false;

    mVoiceFontExpiryTimer.start();
    mVoiceFontExpiryTimer.setTimerExpirySec(VOICE_FONT_EXPIRY_INTERVAL);

    return result["voice_fonts"].asBoolean();
}

bool LLVivoxVoiceClient::requestParcelVoiceInfo()
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

bool LLVivoxVoiceClient::addAndJoinSession(const sessionStatePtr_t &nextSession)
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

    // The old session may now need to be deleted.
    reapSession(oldSession);

    if (mAudioSession)
    {
        if (!mAudioSession->mHandle.empty())
        {
            // Connect to a session by session handle

            sessionMediaConnectSendMessage(mAudioSession);
        }
        else
        {
            // Connect to a session by URI
            sessionCreateSendMessage(mAudioSession, true, false);
        }
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
            // Terminating a half-connected session on other types of calls seems to break something in the vivox gateway.
            if (mAudioSession->mIsP2P)
            {
                terminateAudioSession(true);
                mIsJoiningSession = false;
                notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL);
                return false;
            }
        }
    }

    bool added(true);
    bool joined(false);

    LLSD timeoutResult(LLSDMap("session", "timeout"));

    // We are about to start a whole new session.  Anything that MIGHT still be in our 
    // maildrop is going to be stale and cause us much wailing and gnashing of teeth.  
    // Just flush it all out and start new.
    mVivoxPump.discard();

    // It appears that I need to wait for BOTH the SessionGroup.AddSession response and the SessionStateChangeEvent with state 4
    // before continuing from this state.  They can happen in either order, and if I don't wait for both, things can get stuck.
    // For now, the SessionGroup.AddSession response handler sets mSessionHandle and the SessionStateChangeEvent handler transitions to stateSessionJoined.
    // This is a cheap way to make sure both have happened before proceeding.
    do
    {
        result = llcoro::suspendUntilEventOnWithTimeout(mVivoxPump, SESSION_JOIN_TIMEOUT, timeoutResult);

        if (sShuttingDown)
        {
            return false;
        }

        LL_INFOS("Voice") << "event=" << ll_stream_notation_sd(result) << LL_ENDL;
        if (result.has("session"))
        {
            if (!mAudioSession)
            {
                LL_WARNS("Voice") << "Message for session handle \"" << result["handle"] << "\" while session is not initialized." << LL_ENDL;
                continue;
            }
            if (result.has("handle") && result["handle"] != mAudioSession->mHandle)
            {
                LL_WARNS("Voice") << "Message for session handle \"" << result["handle"] << "\" while waiting for \"" << mAudioSession->mHandle << "\"." << LL_ENDL;
                continue;
            }

            std::string message = result["session"].asString();

            if ((message == "added") || (message == "created"))
            {
                added = true;
            }
            else if (message == "joined")
            {
                joined = true;
            }
            else if ((message == "failed") || (message == "removed") || (message == "timeout"))
            {   // we will get a removed message if a voice call is declined.
                
                if (message == "failed") 
                {
                    int reason = result["reason"].asInteger();
                    LL_WARNS("Voice") << "Add and join failed for reason " << reason << LL_ENDL;
                    
                    if (   (reason == ERROR_VIVOX_NOT_LOGGED_IN)
                        || (reason == ERROR_VIVOX_OBJECT_NOT_FOUND))
                    {
                        LL_DEBUGS("Voice") << "Requesting reprovision and login." << LL_ENDL;
                        requestRelog();
                    }                    
                }
                else
                {
                    LL_WARNS("Voice") << "session '" << message << "' "
                                      << LL_ENDL;
                }
                    
                notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL);
                mIsJoiningSession = false;
                return false;
            }
        }
    } while (!added || !joined);

    mIsJoiningSession = false;

    if (mSpatialJoiningNum > 100)
    {
        LL_WARNS("Voice") << "There seems to be problem with connecting to a voice channel. Frames to join were " << mSpatialJoiningNum << LL_ENDL;
    }

    mSpatialJoiningNum = 0;

    // Events that need to happen when a session is joined could go here.
    // send an initial positional information immediately upon joining.
    // 
    // do an initial update for position and the camera position, then send a 
    // positional update.
    updatePosition();
    enforceTether();

    // Dirty state that may need to be sync'ed with the daemon.
    mMuteMicDirty = true;
    mSpeakerVolumeDirty = true;
    mSpatialCoordsDirty = true;

    sendPositionAndVolumeUpdate();

    notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINED);

    return true;
}

bool LLVivoxVoiceClient::terminateAudioSession(bool wait)
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
                std::string savepath("/tmp/vivoxrecording");
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

                        result = llcoro::suspendUntilEventOnWithTimeout(mVivoxPump, LOGOUT_ATTEMPT_TIMEOUT, timeoutResult);

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
                       << " ShuttingDown " << (sShuttingDown ? "true" : "false")
                       << " returning " << status
                       << LL_ENDL;
    return status;
}


typedef enum e_voice_wait_for_channel_state
{
    VOICE_CHANNEL_STATE_LOGIN = 0, // entry point
    VOICE_CHANNEL_STATE_CHECK_EFFECTS,
    VOICE_CHANNEL_STATE_START_CHANNEL_PROCESSING,
    VOICE_CHANNEL_STATE_PROCESS_CHANNEL,
    VOICE_CHANNEL_STATE_NEXT_CHANNEL_DELAY,
    VOICE_CHANNEL_STATE_NEXT_CHANNEL_CHECK,
    VOICE_CHANNEL_STATE_LOGOUT,
    VOICE_CHANNEL_STATE_RELOG,
    VOICE_CHANNEL_STATE_DONE,
} EVoiceWaitForChannelState;

bool LLVivoxVoiceClient::waitForChannel()
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

        switch (state)
        {
        case VOICE_CHANNEL_STATE_LOGIN:
            if (!loginToVivox())
            {
                return false;
            }
            state = VOICE_CHANNEL_STATE_CHECK_EFFECTS;
            break;

        case VOICE_CHANNEL_STATE_CHECK_EFFECTS:
            if (LLVoiceClient::instance().getVoiceEffectEnabled())
            {
                retrieveVoiceFonts();

                if (sShuttingDown)
                {
                    return false;
                }

                // Request the set of available voice fonts.
                refreshVoiceEffectLists(false);
            }

#if USE_SESSION_GROUPS
            // Rider: This code is completely unchanged from the original state machine
            // It does not seem to be in active use... but I'd rather not rip it out.
            // create the main session group
            setState(stateCreatingSessionGroup);
            sessionGroupCreateSendMessage();
#endif

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
                state = VOICE_CHANNEL_STATE_LOGOUT;
                break;
            }

        case VOICE_CHANNEL_STATE_LOGOUT:
            logoutOfVivox(true /*bool wait*/);
            if (mRelogRequested)
            {
                state = VOICE_CHANNEL_STATE_RELOG;
            }
            else
            {
                state = VOICE_CHANNEL_STATE_DONE;
            }
            break;

        case VOICE_CHANNEL_STATE_RELOG:
            LL_DEBUGS("Voice") << "Relog Requested, restarting provisioning" << LL_ENDL;
            if (!provisionVoiceAccount())
            {
                if (sShuttingDown)
                {
                    return false;
                }
                LL_WARNS("Voice") << "provisioning voice failed; giving up" << LL_ENDL;
                giveUp();
                return false;
            }
            if (mVoiceEnabled && mRelogRequested && isGatewayRunning())
            {
                state = VOICE_CHANNEL_STATE_LOGIN;
            }
            else
            {
                state = VOICE_CHANNEL_STATE_DONE;
            }
            break;
        case VOICE_CHANNEL_STATE_DONE:
            LL_DEBUGS("Voice")
                << "exiting"
                << " RelogRequested=" << mRelogRequested
                << " VoiceEnabled=" << mVoiceEnabled
                << LL_ENDL;
            return !sShuttingDown;
        }
    } while (true);
}

bool LLVivoxVoiceClient::runSession(const sessionStatePtr_t &session)
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
           && isGatewayRunning()
           && !mSessionTerminateRequested
           && !mTuningMode)
    {
        sendCaptureAndRenderDevices(); // suspends

        if (sShuttingDown)
        {
            return false;
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
        LLSD result = llcoro::suspendUntilEventOnWithTimeout(mVivoxPump, UPDATE_THROTTLE_SECONDS, timeoutEvent);

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

void LLVivoxVoiceClient::sendCaptureAndRenderDevices()
{
    if (mCaptureDeviceDirty || mRenderDeviceDirty)
    {
        std::ostringstream stream;

        buildSetCaptureDevice(stream);
        buildSetRenderDevice(stream);

        if (!stream.str().empty())
        {
            writeString(stream.str());
        }

        llcoro::suspendUntilTimeout(UPDATE_THROTTLE_SECONDS);
    }
}

void LLVivoxVoiceClient::recordingAndPlaybackMode()
{
    LL_INFOS("Voice") << "In voice capture/playback mode." << LL_ENDL;

    while (true)
    {
        LLSD command;
        do
        {
            command = llcoro::suspendUntilEventOn(mVivoxPump);
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

int LLVivoxVoiceClient::voiceRecordBuffer()
{
    LLSD timeoutResult(LLSDMap("recplay", "stop")); 

    LL_INFOS("Voice") << "Recording voice buffer" << LL_ENDL;

    LLSD result;

    captureBufferRecordStartSendMessage();
    notifyVoiceFontObservers();

    do
    {
        result = llcoro::suspendUntilEventOnWithTimeout(mVivoxPump, CAPTURE_BUFFER_MAX_TIME, timeoutResult);
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

int LLVivoxVoiceClient::voicePlaybackBuffer()
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

            result = llcoro::suspendUntilEventOnWithTimeout(mVivoxPump, CAPTURE_BUFFER_MAX_TIME, timeoutResult);
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


bool LLVivoxVoiceClient::performMicTuning()
{
    LL_INFOS("Voice") << "Entering voice tuning mode." << LL_ENDL;

    mIsInTuningMode = true;
    llcoro::suspend();

    while (mTuningMode && !sShuttingDown)
    {

        if (mCaptureDeviceDirty || mRenderDeviceDirty)
        {
            // These can't be changed while in tuning mode.  Set them before starting.
            std::ostringstream stream;

            buildSetCaptureDevice(stream);
            buildSetRenderDevice(stream);

            if (!stream.str().empty())
            {
                writeString(stream.str());
            }

            llcoro::suspendUntilTimeout(UPDATE_THROTTLE_SECONDS);
        }

        // loop mic back to render device.
        //setMuteMic(0);						// make sure the mic is not muted
        std::ostringstream stream;

        stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.MuteLocalMic.1\">"
            << "<ConnectorHandle>" << LLVivoxSecurity::getInstance()->connectorHandle() << "</ConnectorHandle>"
            << "<Value>false</Value>"
            << "</Request>\n\n\n";

        // Dirty the mute mic state so that it will get reset when we finishing previewing
        mMuteMicDirty = true;
        mTuningSpeakerVolumeDirty = true;

        writeString(stream.str());
        tuningCaptureStartSendMessage(1);  // 1-loop, zero, don't loop

        //---------------------------------------------------------------------
        if (!sShuttingDown)
        {
            llcoro::suspend();
        }

        while (mTuningMode && !mCaptureDeviceDirty && !mRenderDeviceDirty && !sShuttingDown)
        {
            // process mic/speaker volume changes
            if (mTuningMicVolumeDirty || mTuningSpeakerVolumeDirty)
            {
                std::ostringstream stream;

                if (mTuningMicVolumeDirty)
                {
                    LL_INFOS("Voice") << "setting tuning mic level to " << mTuningMicVolume << LL_ENDL;
                    stream
                        << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.SetMicLevel.1\">"
                        << "<Level>" << mTuningMicVolume << "</Level>"
                        << "</Request>\n\n\n";
                }

                if (mTuningSpeakerVolumeDirty)
                {
                    LL_INFOS("Voice") << "setting tuning speaker level to " << mTuningSpeakerVolume << LL_ENDL;
                    stream
                        << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.SetSpeakerLevel.1\">"
                        << "<Level>" << mTuningSpeakerVolume << "</Level>"
                        << "</Request>\n\n\n";
                }

                mTuningMicVolumeDirty = false;
                mTuningSpeakerVolumeDirty = false;

                if (!stream.str().empty())
                {
                    writeString(stream.str());
                }
            }
            llcoro::suspend();
        }

        //---------------------------------------------------------------------

        // transition out of mic tuning
        tuningCaptureStopSendMessage();
        if ((mCaptureDeviceDirty || mRenderDeviceDirty) && !sShuttingDown)
        {
            llcoro::suspendUntilTimeout(UPDATE_THROTTLE_SECONDS);
        }
    }

    mIsInTuningMode = false;

    //---------------------------------------------------------------------
    return true;
}

//=========================================================================

void LLVivoxVoiceClient::closeSocket(void)
{
	mSocket.reset();
    sConnected = false;
	mConnectorEstablished = false;
	mAccountLoggedIn = false;
}

void LLVivoxVoiceClient::loginSendMessage()
{
	std::ostringstream stream;

	bool autoPostCrashDumps = gSavedSettings.getBOOL("VivoxAutoPostCrashDumps");

	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.Login.1\">"
		<< "<ConnectorHandle>" << LLVivoxSecurity::getInstance()->connectorHandle() << "</ConnectorHandle>"
		<< "<AccountName>" << mAccountName << "</AccountName>"
        << "<AccountPassword>" << mAccountPassword << "</AccountPassword>"
        << "<AccountHandle>" << LLVivoxSecurity::getInstance()->accountHandle() << "</AccountHandle>"
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

void LLVivoxVoiceClient::logout()
{
	// Ensure that we'll re-request provisioning before logging in again
	mAccountPassword.clear();
	mVoiceAccountServerURI.clear();
	
	logoutSendMessage();
}

void LLVivoxVoiceClient::logoutSendMessage()
{
	if(mAccountLoggedIn)
	{
        LL_INFOS("Voice") << "Attempting voice logout" << LL_ENDL;
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.Logout.1\">"
			<< "<AccountHandle>" << LLVivoxSecurity::getInstance()->accountHandle() << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		mAccountLoggedIn = false;

		writeString(stream.str());
	}
}

void LLVivoxVoiceClient::sessionGroupCreateSendMessage()
{
	if(mAccountLoggedIn)
	{		
		std::ostringstream stream;

		LL_DEBUGS("Voice") << "creating session group" << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.Create.1\">"
			<< "<AccountHandle>" << LLVivoxSecurity::getInstance()->accountHandle() << "</AccountHandle>"
			<< "<Type>Normal</Type>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLVivoxVoiceClient::sessionCreateSendMessage(const sessionStatePtr_t &session, bool startAudio, bool startText)
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
		<< "<AccountHandle>" << LLVivoxSecurity::getInstance()->accountHandle() << "</AccountHandle>"
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

void LLVivoxVoiceClient::sessionGroupAddSessionSendMessage(const sessionStatePtr_t &session, bool startAudio, bool startText)
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

void LLVivoxVoiceClient::sessionMediaConnectSendMessage(const sessionStatePtr_t &session)
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

void LLVivoxVoiceClient::sessionTextConnectSendMessage(const sessionStatePtr_t &session)
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

void LLVivoxVoiceClient::sessionTerminate()
{
	mSessionTerminateRequested = true;
}

void LLVivoxVoiceClient::requestRelog()
{
	mSessionTerminateRequested = true;
	mRelogRequested = true;
}


void LLVivoxVoiceClient::leaveAudioSession()
{
	if(mAudioSession)
	{
		LL_DEBUGS("Voice") << "leaving session: " << mAudioSession->mSIPURI << LL_ENDL;

		if(!mAudioSession->mHandle.empty())
		{

#if RECORD_EVERYTHING
			// HACK: for testing only
			// Save looped recording
			std::string savepath("/tmp/vivoxrecording");
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

void LLVivoxVoiceClient::sessionTerminateSendMessage(const sessionStatePtr_t &session)
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

void LLVivoxVoiceClient::sessionGroupTerminateSendMessage(const sessionStatePtr_t &session)
{
	std::ostringstream stream;
	
	LL_DEBUGS("Voice") << "Sending SessionGroup.Terminate with handle " << session->mGroupHandle << LL_ENDL;	
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.Terminate.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVivoxVoiceClient::sessionMediaDisconnectSendMessage(const sessionStatePtr_t &session)
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


void LLVivoxVoiceClient::getCaptureDevicesSendMessage()
{
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.GetCaptureDevices.1\">"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVivoxVoiceClient::getRenderDevicesSendMessage()
{
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.GetRenderDevices.1\">"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVivoxVoiceClient::clearCaptureDevices()
{
	LL_DEBUGS("Voice") << "called" << LL_ENDL;
	mCaptureDevices.clear();
}

void LLVivoxVoiceClient::addCaptureDevice(const LLVoiceDevice& device)
{
	LL_DEBUGS("Voice") << "display: '" << device.display_name << "' device: '" << device.full_name << "'" << LL_ENDL;
    mCaptureDevices.push_back(device);
}

LLVoiceDeviceList& LLVivoxVoiceClient::getCaptureDevices()
{
	return mCaptureDevices;
}

void LLVivoxVoiceClient::setCaptureDevice(const std::string& name)
{
	if(name == "Default")
	{
		if(!mCaptureDevice.empty())
		{
			mCaptureDevice.clear();
			mCaptureDeviceDirty = true;	
		}
	}
	else
	{
		if(mCaptureDevice != name)
		{
			mCaptureDevice = name;
			mCaptureDeviceDirty = true;	
		}
	}
}
void LLVivoxVoiceClient::setDevicesListUpdated(bool state)
{
	mDevicesListUpdated = state;
}

void LLVivoxVoiceClient::clearRenderDevices()
{	
	LL_DEBUGS("Voice") << "called" << LL_ENDL;
	mRenderDevices.clear();
}

void LLVivoxVoiceClient::addRenderDevice(const LLVoiceDevice& device)
{
	LL_DEBUGS("Voice") << "display: '" << device.display_name << "' device: '" << device.full_name << "'" << LL_ENDL;
    mRenderDevices.push_back(device);
}

LLVoiceDeviceList& LLVivoxVoiceClient::getRenderDevices()
{
	return mRenderDevices;
}

void LLVivoxVoiceClient::setRenderDevice(const std::string& name)
{
	if(name == "Default")
	{
		if(!mRenderDevice.empty())
		{
			mRenderDevice.clear();
			mRenderDeviceDirty = true;	
		}
	}
	else
	{
		if(mRenderDevice != name)
		{
			mRenderDevice = name;
			mRenderDeviceDirty = true;	
		}
	}
	
}

void LLVivoxVoiceClient::tuningStart()
{
    LL_DEBUGS("Voice") << "Starting tuning" << LL_ENDL;
    mTuningMode = true;
    if (!mIsCoroutineActive)
    {
        LLCoros::instance().launch("LLVivoxVoiceClient::voiceControlCoro",
            boost::bind(&LLVivoxVoiceClient::voiceControlCoro, LLVivoxVoiceClient::getInstance()));
    }
    else if (mIsInChannel)
	{
		LL_DEBUGS("Voice") << "no channel" << LL_ENDL;
		sessionTerminate();
	}
}

void LLVivoxVoiceClient::tuningStop()
{
	mTuningMode = false;
}

bool LLVivoxVoiceClient::inTuningMode()
{
    return mIsInTuningMode;
}

void LLVivoxVoiceClient::tuningRenderStartSendMessage(const std::string& name, bool loop)
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

void LLVivoxVoiceClient::tuningRenderStopSendMessage()
{
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.RenderAudioStop.1\">"
    << "<SoundFilePath>" << mTuningAudioFile << "</SoundFilePath>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVivoxVoiceClient::tuningCaptureStartSendMessage(int loop)
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

void LLVivoxVoiceClient::tuningCaptureStopSendMessage()
{
	LL_DEBUGS("Voice") << "sending CaptureAudioStop" << LL_ENDL;
	
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.CaptureAudioStop.1\">"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());

	mTuningEnergy = 0.0f;
}

void LLVivoxVoiceClient::tuningSetMicVolume(float volume)
{
	int scaled_volume = scale_mic_volume(volume);

	if(scaled_volume != mTuningMicVolume)
	{
		mTuningMicVolume = scaled_volume;
		mTuningMicVolumeDirty = true;
	}
}

void LLVivoxVoiceClient::tuningSetSpeakerVolume(float volume)
{
	int scaled_volume = scale_speaker_volume(volume);	

	if(scaled_volume != mTuningSpeakerVolume)
	{
		mTuningSpeakerVolume = scaled_volume;
		mTuningSpeakerVolumeDirty = true;
	}
}
				
float LLVivoxVoiceClient::tuningGetEnergy(void)
{
	return mTuningEnergy;
}

bool LLVivoxVoiceClient::deviceSettingsAvailable()
{
	bool result = true;
	
	if(!sConnected)
		result = false;
	
	if(mRenderDevices.empty())
		result = false;
	
	return result;
}
bool LLVivoxVoiceClient::deviceSettingsUpdated()
{
    bool updated = mDevicesListUpdated;
	if (mDevicesListUpdated)
	{
		// a hot swap event or a polling of the audio devices has been parsed since the last redraw of the input and output device panel.
		mDevicesListUpdated = false; // toggle the setting
	}
	return updated;		
}

void LLVivoxVoiceClient::refreshDeviceLists(bool clearCurrentList)
{
	if(clearCurrentList)
	{
		clearCaptureDevices();
		clearRenderDevices();
	}
	getCaptureDevicesSendMessage();
	getRenderDevicesSendMessage();
}

void LLVivoxVoiceClient::daemonDied()
{
	// The daemon died, so the connection is gone.  Reset everything and start over.
	LL_WARNS("Voice") << "Connection to vivox daemon lost.  Resetting state."<< LL_ENDL;

	//TODO: Try to relaunch the daemon
}

void LLVivoxVoiceClient::giveUp()
{
	// All has failed.  Clean up and stop trying.
    LL_WARNS("Voice") << "Terminating Voice Service" << LL_ENDL;
	closeSocket();
	cleanUp();
}

static void oldSDKTransform (LLVector3 &left, LLVector3 &up, LLVector3 &at, LLVector3d &pos, LLVector3 &vel)
{
	F32 nat[3], nup[3], nl[3]; // the new at, up, left vectors and the  new position and velocity
//	F32 nvel[3]; 
	F64 npos[3];
	
	// The original XML command was sent like this:
	/*
			<< "<Position>"
				<< "<X>" << pos[VX] << "</X>"
				<< "<Y>" << pos[VZ] << "</Y>"
				<< "<Z>" << pos[VY] << "</Z>"
			<< "</Position>"
			<< "<Velocity>"
				<< "<X>" << mAvatarVelocity[VX] << "</X>"
				<< "<Y>" << mAvatarVelocity[VZ] << "</Y>"
				<< "<Z>" << mAvatarVelocity[VY] << "</Z>"
			<< "</Velocity>"
			<< "<AtOrientation>"
				<< "<X>" << l.mV[VX] << "</X>"
				<< "<Y>" << u.mV[VX] << "</Y>"
				<< "<Z>" << a.mV[VX] << "</Z>"
			<< "</AtOrientation>"
			<< "<UpOrientation>"
				<< "<X>" << l.mV[VZ] << "</X>"
				<< "<Y>" << u.mV[VY] << "</Y>"
				<< "<Z>" << a.mV[VZ] << "</Z>"
			<< "</UpOrientation>"
			<< "<LeftOrientation>"
				<< "<X>" << l.mV [VY] << "</X>"
				<< "<Y>" << u.mV [VZ] << "</Y>"
				<< "<Z>" << a.mV [VY] << "</Z>"
			<< "</LeftOrientation>";
	*/

#if 1
	// This was the original transform done when building the XML command
	nat[0] = left.mV[VX];
	nat[1] = up.mV[VX];
	nat[2] = at.mV[VX];

	nup[0] = left.mV[VZ];
	nup[1] = up.mV[VY];
	nup[2] = at.mV[VZ];

	nl[0] = left.mV[VY];
	nl[1] = up.mV[VZ];
	nl[2] = at.mV[VY];

	npos[0] = pos.mdV[VX];
	npos[1] = pos.mdV[VZ];
	npos[2] = pos.mdV[VY];

//	nvel[0] = vel.mV[VX];
//	nvel[1] = vel.mV[VZ];
//	nvel[2] = vel.mV[VY];

	for(int i=0;i<3;++i) {
		at.mV[i] = nat[i];
		up.mV[i] = nup[i];
		left.mV[i] = nl[i];
		pos.mdV[i] = npos[i];
	}
	
	// This was the original transform done in the SDK
	nat[0] = at.mV[2];
	nat[1] = 0; // y component of at vector is always 0, this was up[2]
	nat[2] = -1 * left.mV[2];

	// We override whatever the application gives us
	nup[0] = 0; // x component of up vector is always 0
	nup[1] = 1; // y component of up vector is always 1
	nup[2] = 0; // z component of up vector is always 0

	nl[0] = at.mV[0];
	nl[1] = 0;  // y component of left vector is always zero, this was up[0]
	nl[2] = -1 * left.mV[0];

	npos[2] = pos.mdV[2] * -1.0;
	npos[1] = pos.mdV[1];
	npos[0] = pos.mdV[0];

	for(int i=0;i<3;++i) {
		at.mV[i] = nat[i];
		up.mV[i] = nup[i];
		left.mV[i] = nl[i];
		pos.mdV[i] = npos[i];
	}
#else
	// This is the compose of the two transforms (at least, that's what I'm trying for)
	nat[0] = at.mV[VX];
	nat[1] = 0; // y component of at vector is always 0, this was up[2]
	nat[2] = -1 * up.mV[VZ];

	// We override whatever the application gives us
	nup[0] = 0; // x component of up vector is always 0
	nup[1] = 1; // y component of up vector is always 1
	nup[2] = 0; // z component of up vector is always 0

	nl[0] = left.mV[VX];
	nl[1] = 0;  // y component of left vector is always zero, this was up[0]
	nl[2] = -1 * left.mV[VY];

	npos[0] = pos.mdV[VX];
	npos[1] = pos.mdV[VZ];
	npos[2] = pos.mdV[VY] * -1.0;

	nvel[0] = vel.mV[VX];
	nvel[1] = vel.mV[VZ];
	nvel[2] = vel.mV[VY];

	for(int i=0;i<3;++i) {
		at.mV[i] = nat[i];
		up.mV[i] = nup[i];
		left.mV[i] = nl[i];
		pos.mdV[i] = npos[i];
	}
	
#endif
}

void LLVivoxVoiceClient::setHidden(bool hidden)
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

void LLVivoxVoiceClient::sendPositionAndVolumeUpdate(void)
{	
	std::ostringstream stream;
	
	if (mSpatialCoordsDirty && inSpatialChannel())
	{
		LLVector3 l, u, a, vel;
		LLVector3d pos;

		mSpatialCoordsDirty = false;
		
		// Always send both speaker and listener positions together.
		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.Set3DPosition.1\">"		
			<< "<SessionHandle>" << getAudioSessionHandle() << "</SessionHandle>";
		
		stream << "<SpeakerPosition>";

        LLMatrix3 avatarRot = mAvatarRot.getMatrix3();

//		LL_DEBUGS("Voice") << "Sending speaker position " << mAvatarPosition << LL_ENDL;
		l = avatarRot.getLeftRow();
		u = avatarRot.getUpRow();
		a = avatarRot.getFwdRow();

        pos = mAvatarPosition;
		vel = mAvatarVelocity;

		// SLIM SDK: the old SDK was doing a transform on the passed coordinates that the new one doesn't do anymore.
		// The old transform is replicated by this function.
		oldSDKTransform(l, u, a, pos, vel);
        
        if (mHidden)
        {
            for (int i=0;i<3;++i)
            {
                pos.mdV[i] = VX_NULL_POSITION;
            }
        }
		
		stream
			<< "<Position>"
				<< "<X>" << pos.mdV[VX] << "</X>"
				<< "<Y>" << pos.mdV[VY] << "</Y>"
				<< "<Z>" << pos.mdV[VZ] << "</Z>"
			<< "</Position>"
			<< "<Velocity>"
				<< "<X>" << vel.mV[VX] << "</X>"
				<< "<Y>" << vel.mV[VY] << "</Y>"
				<< "<Z>" << vel.mV[VZ] << "</Z>"
			<< "</Velocity>"
			<< "<AtOrientation>"
				<< "<X>" << a.mV[VX] << "</X>"
				<< "<Y>" << a.mV[VY] << "</Y>"
				<< "<Z>" << a.mV[VZ] << "</Z>"
			<< "</AtOrientation>"
			<< "<UpOrientation>"
				<< "<X>" << u.mV[VX] << "</X>"
				<< "<Y>" << u.mV[VY] << "</Y>"
				<< "<Z>" << u.mV[VZ] << "</Z>"
			<< "</UpOrientation>"
  			<< "<LeftOrientation>"
  				<< "<X>" << l.mV [VX] << "</X>"
  				<< "<Y>" << l.mV [VY] << "</Y>"
  				<< "<Z>" << l.mV [VZ] << "</Z>"
  			<< "</LeftOrientation>"
            ;
        
		stream << "</SpeakerPosition>";

		stream << "<ListenerPosition>";

		LLVector3d	earPosition;
		LLVector3	earVelocity;
		LLMatrix3	earRot;
		
		switch(mEarLocation)
		{
			case earLocCamera:
			default:
				earPosition = mCameraPosition;
				earVelocity = mCameraVelocity;
				earRot = mCameraRot;
			break;
			
			case earLocAvatar:
				earPosition = mAvatarPosition;
				earVelocity = mAvatarVelocity;
				earRot = avatarRot;
			break;
			
			case earLocMixed:
				earPosition = mAvatarPosition;
				earVelocity = mAvatarVelocity;
				earRot = mCameraRot;
			break;
		}

		l = earRot.getLeftRow();
		u = earRot.getUpRow();
		a = earRot.getFwdRow();

        pos = earPosition;
		vel = earVelocity;

		
		oldSDKTransform(l, u, a, pos, vel);
		
        if (mHidden)
        {
            for (int i=0;i<3;++i)
            {
                pos.mdV[i] = VX_NULL_POSITION;
            }
        }
        
		stream
			<< "<Position>"
				<< "<X>" << pos.mdV[VX] << "</X>"
				<< "<Y>" << pos.mdV[VY] << "</Y>"
				<< "<Z>" << pos.mdV[VZ] << "</Z>"
			<< "</Position>"
			<< "<Velocity>"
				<< "<X>" << vel.mV[VX] << "</X>"
				<< "<Y>" << vel.mV[VY] << "</Y>"
				<< "<Z>" << vel.mV[VZ] << "</Z>"
			<< "</Velocity>"
			<< "<AtOrientation>"
				<< "<X>" << a.mV[VX] << "</X>"
				<< "<Y>" << a.mV[VY] << "</Y>"
				<< "<Z>" << a.mV[VZ] << "</Z>"
			<< "</AtOrientation>"
			<< "<UpOrientation>"
				<< "<X>" << u.mV[VX] << "</X>"
				<< "<Y>" << u.mV[VY] << "</Y>"
				<< "<Z>" << u.mV[VZ] << "</Z>"
			<< "</UpOrientation>"
  			<< "<LeftOrientation>"
  				<< "<X>" << l.mV [VX] << "</X>"
  				<< "<Y>" << l.mV [VY] << "</Y>"
  				<< "<Z>" << l.mV [VZ] << "</Z>"
  			<< "</LeftOrientation>"
            ;

		stream << "</ListenerPosition>";

		stream << "<ReqDispositionType>1</ReqDispositionType>";  //do not generate responses for update requests
		stream << "</Request>\n\n\n";
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
					// scale from the range 0.0-1.0 to vivox volume in the range 0-100
					S32 volume = ll_round(p->mVolume / VOLUME_SCALE_VIVOX);
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
					
					// SLIM SDK: Send both volume and mute commands.
					
					// Send a "volume for me" command for the user.
					stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.SetParticipantVolumeForMe.1\">"
						<< "<SessionHandle>" << getAudioSessionHandle() << "</SessionHandle>"
						<< "<ParticipantURI>" << p->mURI << "</ParticipantURI>"
						<< "<Volume>" << volume << "</Volume>"
						<< "</Request>\n\n\n";

					if(!mAudioSession->mIsP2P)
					  {
					    // Send a "mute for me" command for the user
					    // Doesn't work in P2P sessions
					    stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.SetParticipantMuteForMe.1\">"
					      << "<SessionHandle>" << getAudioSessionHandle() << "</SessionHandle>"
					      << "<ParticipantURI>" << p->mURI << "</ParticipantURI>"
					      << "<Mute>" << (mute?"1":"0") << "</Mute>"
					      << "<Scope>Audio</Scope>"
					      << "</Request>\n\n\n";
					    }
				}
				
				p->mVolumeDirty = false;
			}
		}
	}

    std::string update(stream.str());
	if(!update.empty())
	{
        LL_DEBUGS("VoiceUpdate") << "sending update " << update << LL_ENDL;
		writeString(update);
	}

}

void LLVivoxVoiceClient::buildSetCaptureDevice(std::ostringstream &stream)
{
	if(mCaptureDeviceDirty)
	{
		LL_DEBUGS("Voice") << "Setting input device = \"" << mCaptureDevice << "\"" << LL_ENDL;
	
		stream 
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.SetCaptureDevice.1\">"
			<< "<CaptureDeviceSpecifier>" << mCaptureDevice << "</CaptureDeviceSpecifier>"
		<< "</Request>"
		<< "\n\n\n";
		
		mCaptureDeviceDirty = false;
	}
}

void LLVivoxVoiceClient::buildSetRenderDevice(std::ostringstream &stream)
{
	if(mRenderDeviceDirty)
	{
		LL_DEBUGS("Voice") << "Setting output device = \"" << mRenderDevice << "\"" << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.SetRenderDevice.1\">"
			<< "<RenderDeviceSpecifier>" << mRenderDevice << "</RenderDeviceSpecifier>"
		<< "</Request>"
		<< "\n\n\n";
		mRenderDeviceDirty = false;
	}
}

void LLVivoxVoiceClient::sendLocalAudioUpdates()
{
	// Check all of the dirty states and then send messages to those needing to be changed.
	// Tuningmode hands its own mute settings.
	std::ostringstream stream;

	if (mMuteMicDirty && !mTuningMode)
	{
		mMuteMicDirty = false;

		// Send a local mute command.

		LL_INFOS("Voice") << "Sending MuteLocalMic command with parameter " << (mMuteMic ? "true" : "false") << LL_ENDL;

		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.MuteLocalMic.1\">"
			<< "<ConnectorHandle>" << LLVivoxSecurity::getInstance()->connectorHandle() << "</ConnectorHandle>"
			<< "<Value>" << (mMuteMic ? "true" : "false") << "</Value>"
			<< "</Request>\n\n\n";

	}

	if (mSpeakerMuteDirty && !mTuningMode)
	{
		const char *muteval = ((mSpeakerVolume <= scale_speaker_volume(0)) ? "true" : "false");

		mSpeakerMuteDirty = false;

		LL_INFOS("Voice") << "Setting speaker mute to " << muteval << LL_ENDL;

		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.MuteLocalSpeaker.1\">"
			<< "<ConnectorHandle>" << LLVivoxSecurity::getInstance()->connectorHandle() << "</ConnectorHandle>"
			<< "<Value>" << muteval << "</Value>"
			<< "</Request>\n\n\n";

	}

	if (mSpeakerVolumeDirty)
	{
		mSpeakerVolumeDirty = false;

		LL_INFOS("Voice") << "Setting speaker volume to " << mSpeakerVolume << LL_ENDL;

		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.SetLocalSpeakerVolume.1\">"
			<< "<ConnectorHandle>" << LLVivoxSecurity::getInstance()->connectorHandle() << "</ConnectorHandle>"
			<< "<Value>" << mSpeakerVolume << "</Value>"
			<< "</Request>\n\n\n";

	}

	if (mMicVolumeDirty)
	{
		mMicVolumeDirty = false;

		LL_INFOS("Voice") << "Setting mic volume to " << mMicVolume << LL_ENDL;

		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.SetLocalMicVolume.1\">"
			<< "<ConnectorHandle>" << LLVivoxSecurity::getInstance()->connectorHandle() << "</ConnectorHandle>"
			<< "<Value>" << mMicVolume << "</Value>"
			<< "</Request>\n\n\n";
	}


	if (!stream.str().empty())
	{
		writeString(stream.str());
	}
}

/**
 * Because of the recurring voice cutout issues (SL-15072) we are going to try
 * to disable the automatic VAD (Voice Activity Detection) and set the associated
 * parameters directly. We will expose them via Debug Settings and that should
 * let us iterate on a collection of values that work for us. Hopefully! 
 *
 * From the VIVOX Docs:
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
void LLVivoxVoiceClient::setupVADParams(unsigned int vad_auto,
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

void LLVivoxVoiceClient::onVADSettingsChange()
{
	// pick up the VAD variables (one of which was changed)
	unsigned int vad_auto = gSavedSettings.getU32("VivoxVadAuto");
	unsigned int vad_hangover = gSavedSettings.getU32("VivoxVadHangover");
	unsigned int vad_noise_floor = gSavedSettings.getU32("VivoxVadNoiseFloor");
	unsigned int vad_sensitivity = gSavedSettings.getU32("VivoxVadSensitivity");

	// build a VAD params change request and send it to SLVoice
	setupVADParams(vad_auto, vad_hangover, vad_noise_floor, vad_sensitivity);
}

/////////////////////////////
// Response/Event handlers

void LLVivoxVoiceClient::connectorCreateResponse(int statusCode, std::string &statusString, std::string &connectorHandle, std::string &versionID)
{	
    LLSD result = LLSD::emptyMap();

	if(statusCode == 0)
	{
		// Connector created, move forward.
        if (connectorHandle == LLVivoxSecurity::getInstance()->connectorHandle())
        {
            LL_INFOS("Voice") << "Voice connector succeeded, Vivox SDK version is " << versionID << " connector handle " << connectorHandle << LL_ENDL;
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
                              << " expected (" << LLVivoxSecurity::getInstance()->connectorHandle() << ")"
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

    mVivoxPump.post(result);
}

void LLVivoxVoiceClient::loginResponse(int statusCode, std::string &statusString, std::string &accountHandle, int numberOfAliases)
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

    mVivoxPump.post(result);

}

void LLVivoxVoiceClient::sessionCreateResponse(std::string &requestId, int statusCode, std::string &statusString, std::string &sessionHandle)
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
                LLSD vivoxevent(LLSDMap("handle", LLSD::String(sessionHandle))
                        ("session", "failed")
                        ("reason", LLSD::Integer(statusCode)));

                mVivoxPump.post(vivoxevent);
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
        LLSD vivoxevent(LLSDMap("handle", LLSD::String(sessionHandle))
                ("session", "created"));

        mVivoxPump.post(vivoxevent);
	}
}

void LLVivoxVoiceClient::sessionGroupAddSessionResponse(std::string &requestId, int statusCode, std::string &statusString, std::string &sessionHandle)
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
                LLSD vivoxevent(LLSDMap("handle", LLSD::String(sessionHandle))
                    ("session", "failed"));

                mVivoxPump.post(vivoxevent);
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

        LLSD vivoxevent(LLSDMap("handle", LLSD::String(sessionHandle))
            ("session", "added"));

        mVivoxPump.post(vivoxevent);

	}
}

void LLVivoxVoiceClient::sessionConnectResponse(std::string &requestId, int statusCode, std::string &statusString)
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

void LLVivoxVoiceClient::logoutResponse(int statusCode, std::string &statusString)
{	
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Account.Logout response failure: " << statusString << LL_ENDL;
		// Should this ever fail?  do we care if it does?
	}
    LLSD vivoxevent(LLSDMap("logout", LLSD::Boolean(true)));

    mVivoxPump.post(vivoxevent);
}

void LLVivoxVoiceClient::connectorShutdownResponse(int statusCode, std::string &statusString)
{
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Connector.InitiateShutdown response failure: " << statusString << LL_ENDL;
		// Should this ever fail?  do we care if it does?
	}
	
	sConnected = false;
	mShutdownComplete = true;
	
    LLSD vivoxevent(LLSDMap("connector", LLSD::Boolean(false)));

    mVivoxPump.post(vivoxevent);
}

void LLVivoxVoiceClient::sessionAddedEvent(
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
				setSessionURI(session, sipURIFromID(session->mCallerID));
			}
			else
			{
				LL_INFOS("Voice") << "Could not generate caller id from uri, using hash of uri " << session->mSIPURI << LL_ENDL;
				session->mCallerID.generate(session->mSIPURI);
				session->mSynthesizedCallerID = true;
				
				// Can't look up the name in this case -- we have to extract it from the URI.
				std::string namePortion = nameFromsipURI(session->mSIPURI);
				if(namePortion.empty())
				{
					// Didn't seem to be a SIP URI, just use the whole provided name.
					namePortion = nameString;
				}
				
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

void LLVivoxVoiceClient::sessionGroupAddedEvent(std::string &sessionGroupHandle)
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

void LLVivoxVoiceClient::joinedAudioSession(const sessionStatePtr_t &session)
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
        LLSD vivoxevent(LLSDMap("handle", LLSD::String(session->mHandle))
                ("session", "joined"));

        mVivoxPump.post(vivoxevent);

		// Add the current user as a participant here.
        participantStatePtr_t participant(session->addParticipant(sipURIFromName(mAccountName)));
		if(participant)
		{
			participant->mIsSelf = true;
			lookupName(participant->mAvatarID);

			LL_INFOS("Voice") << "added self as participant \"" << participant->mAccountName 
					<< "\" (" << participant->mAvatarID << ")"<< LL_ENDL;
		}
		
		if(!session->mIsChannel)
		{
			// this is a p2p session.  Make sure the other end is added as a participant.
            participantStatePtr_t participant(session->addParticipant(session->mSIPURI));
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

void LLVivoxVoiceClient::sessionRemovedEvent(
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

void LLVivoxVoiceClient::reapSession(const sessionStatePtr_t &session)
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
bool LLVivoxVoiceClient::sessionNeedsRelog(const sessionStatePtr_t &session)
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
				if(stricmp(urihost.c_str(), mVoiceSIPURIHostName.c_str()))
				{
					// The hostname in this URI is different from what we expect.  This probably means we need to relog.
					
					// We could make a ProvisionVoiceAccountRequest and compare the result with the current values of
					// mVoiceSIPURIHostName and mVoiceAccountServerURI to be really sure, but this is a pretty good indicator.
					
					result = true;
				}
			}
		}
	}
	
	return result;
}

void LLVivoxVoiceClient::leftAudioSession(const sessionStatePtr_t &session)
{
    if (mAudioSession == session)
    {
        LLSD vivoxevent(LLSDMap("handle", LLSD::String(session->mHandle))
            ("session", "removed"));

        mVivoxPump.post(vivoxevent);
    }
}

void LLVivoxVoiceClient::accountLoginStateChangeEvent(
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

            mVivoxPump.post(levent);
            break;
        case 2:
            break;

        case 3:
            levent["login"] = LLSD::String("account_loggingOut");

            mVivoxPump.post(levent);
            break;

        case 4:
            break;

        case 100:
            LL_WARNS("Voice") << "account state event error" << LL_ENDL;
            break;

        case 0:
            levent["login"] = LLSD::String("account_logout");

            mVivoxPump.post(levent);
            break;
    		
        default:
			//Used to be a commented out warning
			LL_WARNS("Voice") << "unknown account state event: " << state << LL_ENDL;
	    	break;
	}
}

void LLVivoxVoiceClient::mediaCompletionEvent(std::string &sessionGroupHandle, std::string &mediaCompletionType)
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
        mVivoxPump.post(result);
}

void LLVivoxVoiceClient::mediaStreamUpdatedEvent(
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
				// Standard "left audio session", Vivox state 'disconnected'
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

void LLVivoxVoiceClient::participantAddedEvent(
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
        participantStatePtr_t participant(session->addParticipant(uriString));
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
				std::string namePortion = nameFromsipURI(uriString);
				if(namePortion.empty())
				{
					// Problem with the SIP URI, fall back to the display name
					namePortion = displayNameString;
				}
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

void LLVivoxVoiceClient::participantRemovedEvent(
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


void LLVivoxVoiceClient::participantUpdatedEvent(
		std::string &sessionHandle, 
		std::string &sessionGroupHandle, 
		std::string &uriString, 
		std::string &alias, 
		bool isModeratorMuted, 
		bool isSpeaking, 
		int volume, 
		F32 energy)
{
    sessionStatePtr_t session(findSession(sessionHandle));
	if(session)
	{
		participantStatePtr_t participant(session->findParticipant(uriString));
		
		if(participant)
		{
            //LL_INFOS("Voice") << "Participant Update for " << participant->mDisplayName << LL_ENDL;

			participant->mIsSpeaking = isSpeaking;
			participant->mIsModeratorMuted = isModeratorMuted;

			// SLIM SDK: convert range: ensure that energy is set to zero if is_speaking is false
			if (isSpeaking)
			{
				participant->mSpeakingTimeout.reset();
				participant->mPower = energy;
			}
			else
			{
				participant->mPower = 0.0f;
			}

			// Ignore incoming volume level if it has been explicitly set, or there
			//  is a volume or mute change pending.
			if ( !participant->mVolumeSet && !participant->mVolumeDirty)
			{
				participant->mVolume = (F32)volume * VOLUME_SCALE_VIVOX;
			}
			
			// *HACK: mantipov: added while working on EXT-3544                                                                                   
			/*                                                                                                                                    
			 Sometimes LLVoiceClient::participantUpdatedEvent callback is called BEFORE                                                            
			 LLViewerChatterBoxSessionAgentListUpdates::post() sometimes AFTER.                                                                    
			 
			 participantUpdatedEvent updates voice participant state in particular participantState::mIsModeratorMuted                             
			 Originally we wanted to update session Speaker Manager to fire LLSpeakerVoiceModerationEvent to fix the EXT-3544 bug.                 
			 Calling of the LLSpeakerMgr::update() method was added into LLIMMgr::processAgentListUpdates.                                         
			 
			 But in case participantUpdatedEvent() is called after LLViewerChatterBoxSessionAgentListUpdates::post()                               
			 voice participant mIsModeratorMuted is changed after speakers are updated in Speaker Manager                                          
			 and event is not fired.                                                                                                               
			 
			 So, we have to call LLSpeakerMgr::update() here. 
			 */
			LLVoiceChannel* voice_cnl = LLVoiceChannel::getCurrentVoiceChannel();
			
			// ignore session ID of local chat                                                                                                    
			if (voice_cnl && voice_cnl->getSessionID().notNull())
			{
				LLSpeakerMgr* speaker_manager = LLIMModel::getInstance()->getSpeakerManager(voice_cnl->getSessionID());
				if (speaker_manager)
				{
					speaker_manager->update(true);

					// also initialize voice moderate_mode depend on Agent's participant. See EXT-6937.
					// *TODO: remove once a way to request the current voice channel moderation mode is implemented.
					if (gAgent.getID() == participant->mAvatarID)
					{
						speaker_manager->initVoiceModerateMode();
					}
				}
			}
		}
		else
		{
			LL_WARNS("Voice") << "unknown participant: " << uriString << LL_ENDL;
		}
	}
	else
	{
		LL_DEBUGS("Voice") << "unknown session " << sessionHandle << LL_ENDL;
	}
}

void LLVivoxVoiceClient::messageEvent(
		std::string &sessionHandle, 
		std::string &uriString, 
		std::string &alias, 
		std::string &messageHeader, 
		std::string &messageBody,
		std::string &applicationString)
{
	LL_DEBUGS("Voice") << "Message event, session " << sessionHandle << " from " << uriString << LL_ENDL;
//	LL_DEBUGS("Voice") << "    header " << messageHeader << ", body: \n" << messageBody << LL_ENDL;

    LL_INFOS("Voice") << "Vivox raw message:" << std::endl << messageBody << LL_ENDL;

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

void LLVivoxVoiceClient::sessionNotificationEvent(std::string &sessionHandle, std::string &uriString, std::string &notificationType)
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

void LLVivoxVoiceClient::voiceServiceConnectionStateChangedEvent(int statusCode, std::string &statusString, std::string &build_id)
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

void LLVivoxVoiceClient::auxAudioPropertiesEvent(F32 energy)
{
	LL_DEBUGS("VoiceEnergy") << "got energy " << energy << LL_ENDL;
	mTuningEnergy = energy;
}

void LLVivoxVoiceClient::muteListChanged()
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
LLVivoxVoiceClient::participantState::participantState(const std::string &uri) : 
	 mURI(uri), 
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

LLVivoxVoiceClient::participantStatePtr_t LLVivoxVoiceClient::sessionState::addParticipant(const std::string &uri)
{
    participantStatePtr_t result;
	bool useAlternateURI = false;
	
	// Note: this is mostly the body of LLVivoxVoiceClient::sessionState::findParticipant(), but since we need to know if it
	// matched the alternate SIP URI (so we can add it properly), we need to reproduce it here.
	{
		participantMap::iterator iter = mParticipantsByURI.find(uri);

		if(iter == mParticipantsByURI.end())
		{
			if(!mAlternateSIPURI.empty() && (uri == mAlternateSIPURI))
			{
				// This is a p2p session (probably with the SLIM client) with an alternate URI for the other participant.
				// Use mSIPURI instead, since it will be properly encoded.
				iter = mParticipantsByURI.find(mSIPURI);
				useAlternateURI = true;
			}
		}

		if(iter != mParticipantsByURI.end())
		{
			result = iter->second;
		}
	}
		
	if(!result)
	{
		// participant isn't already in one list or the other.
		result.reset(new participantState(useAlternateURI?mSIPURI:uri));
		mParticipantsByURI.insert(participantMap::value_type(result->mURI, result));
		mParticipantsChanged = true;
		
		// Try to do a reverse transform on the URI to get the GUID back.
		{
			LLUUID id;
			if(LLVivoxVoiceClient::getInstance()->IDFromName(result->mURI, id))
			{
				result->mAvatarIDValid = true;
				result->mAvatarID = id;
			}
			else
			{
				// Create a UUID by hashing the URI, but do NOT set mAvatarIDValid.
				// This indicates that the ID will not be in the name cache.
				result->mAvatarID.generate(uri);
			}
		}
		
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

bool LLVivoxVoiceClient::participantState::updateMuteState()
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

bool LLVivoxVoiceClient::participantState::isAvatar()
{
	return mAvatarIDValid;
}

void LLVivoxVoiceClient::sessionState::removeParticipant(const LLVivoxVoiceClient::participantStatePtr_t &participant)
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

void LLVivoxVoiceClient::sessionState::removeAllParticipants()
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
void LLVivoxVoiceClient::sessionState::VerifySessions()
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


void LLVivoxVoiceClient::getParticipantList(std::set<LLUUID> &participants)
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

bool LLVivoxVoiceClient::isParticipant(const LLUUID &speaker_id)
{
  if(mAudioSession)
    {
      return (mAudioSession->mParticipantsByUUID.find(speaker_id) != mAudioSession->mParticipantsByUUID.end());
    }
  return false;
}


LLVivoxVoiceClient::participantStatePtr_t LLVivoxVoiceClient::sessionState::findParticipant(const std::string &uri)
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

LLVivoxVoiceClient::participantStatePtr_t LLVivoxVoiceClient::sessionState::findParticipantByID(const LLUUID& id)
{
    participantStatePtr_t result;
	participantUUIDMap::iterator iter = mParticipantsByUUID.find(id);

	if(iter != mParticipantsByUUID.end())
	{
		result = iter->second;
	}

	return result;
}

LLVivoxVoiceClient::participantStatePtr_t LLVivoxVoiceClient::findParticipantByID(const LLUUID& id)
{
    participantStatePtr_t result;
	
	if(mAudioSession)
	{
		result = mAudioSession->findParticipantByID(id);
	}
	
	return result;
}



// Check for parcel boundary crossing
bool LLVivoxVoiceClient::checkParcelChanged(bool update)
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

bool LLVivoxVoiceClient::switchChannel(
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

void LLVivoxVoiceClient::joinSession(const sessionStatePtr_t &session)
{
	mNextAudioSession = session;

    if (mIsInChannel)
    {
        // If we're already in a channel, or if we're joining one, terminate
        // so we can rejoin with the new session data.
        sessionTerminate();
    }
}

void LLVivoxVoiceClient::setNonSpatialChannel(
	const std::string &uri,
	const std::string &credentials)
{
	switchChannel(uri, false, false, false, credentials);
}

bool LLVivoxVoiceClient::setSpatialChannel(
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

void LLVivoxVoiceClient::callUser(const LLUUID &uuid)
{
	std::string userURI = sipURIFromID(uuid);

	switchChannel(userURI, false, true, true);
}

#if 0
// Vivox text IMs are not in use.
LLVivoxVoiceClient::sessionStatePtr_t LLVivoxVoiceClient::startUserIMSession(const LLUUID &uuid)
{
	// Figure out if a session with the user already exists
    sessionStatePtr_t session(findSession(uuid));
	if(!session)
	{
		// No session with user, need to start one.
		std::string uri = sipURIFromID(uuid);
		session = addSession(uri);

		llassert(session);
		if (!session)
            return session;

		session->mIsSpatial = false;
		session->mReconnect = false;	
		session->mIsP2P = true;
		session->mCallerID = uuid;
	}
	
	if(session->mHandle.empty())
	  {
	    // Session isn't active -- start it up.
	    sessionCreateSendMessage(session, false, false);
	  }
	else
	  {	
	    // Session is already active -- start up text.
	    sessionTextConnectSendMessage(session);
	  }
	
	return session;
}
#endif

void LLVivoxVoiceClient::endUserIMSession(const LLUUID &uuid)
{
#if 0
    // Vivox text IMs are not in use.
    
    // Figure out if a session with the user exists
    sessionStatePtr_t session(findSession(uuid));
	if(session)
	{
		// found the session
		if(!session->mHandle.empty())
		{
			// sessionTextDisconnectSendMessage(session);  // a SLim leftover,  not used any more.
		}
	}	
	else
	{
		LL_DEBUGS("Voice") << "Session not found for participant ID " << uuid << LL_ENDL;
	}
#endif
}
bool LLVivoxVoiceClient::isValidChannel(std::string &sessionHandle)
{
  return(findSession(sessionHandle) != NULL);
	
}
bool LLVivoxVoiceClient::answerInvite(std::string &sessionHandle)
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

bool LLVivoxVoiceClient::isVoiceWorking() const
{

    //Added stateSessionTerminated state to avoid problems with call in parcels with disabled voice (EXT-4758)
    // Condition with joining spatial num was added to take into account possible problems with connection to voice
    // server(EXT-4313). See bug descriptions and comments for MAX_NORMAL_JOINING_SPATIAL_NUM for more info.
    return (mSpatialJoiningNum < MAX_NORMAL_JOINING_SPATIAL_NUM) && mIsProcessingChannels;
//  return (mSpatialJoiningNum < MAX_NORMAL_JOINING_SPATIAL_NUM) && (stateLoggedIn <= mState) && (mState <= stateSessionTerminated);
}

// Returns true if the indicated participant in the current audio session is really an SL avatar.
// Currently this will be false only for PSTN callers into group chats, and PSTN p2p calls.
bool LLVivoxVoiceClient::isParticipantAvatar(const LLUUID &id)
{
	bool result = true; 
    sessionStatePtr_t session(findSession(id));
	
	if(session)
	{
		// this is a p2p session with the indicated caller, or the session with the specified UUID.
		if(session->mSynthesizedCallerID)
			result = false;
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
bool LLVivoxVoiceClient::isSessionCallBackPossible(const LLUUID &session_id)
{
	bool result = true; 
    sessionStatePtr_t session(findSession(session_id));
	
	if(session != NULL)
	{
		result = session->isCallBackPossible();
	}
	
	return result;
}

// Returns true if the session can accept text IM's.
// Currently this will be false only for PSTN P2P calls.
bool LLVivoxVoiceClient::isSessionTextIMPossible(const LLUUID &session_id)
{
	bool result = true;
    sessionStatePtr_t session(findSession(session_id));
	
	if(session != NULL)
	{
		result = session->isTextIMPossible();
	}
	
	return result;
}
		

void LLVivoxVoiceClient::declineInvite(std::string &sessionHandle)
{
    sessionStatePtr_t session(findSession(sessionHandle));
	if(session)
	{
		sessionMediaDisconnectSendMessage(session);
	}
}

void LLVivoxVoiceClient::leaveNonSpatialChannel()
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

std::string LLVivoxVoiceClient::getCurrentChannel()
{
	std::string result;
	
    if (mIsInChannel && !mSessionTerminateRequested)
	{
		result = getAudioSessionURI();
	}
	
	return result;
}

bool LLVivoxVoiceClient::inProximalChannel()
{
	bool result = false;
	
    if (mIsInChannel && !mSessionTerminateRequested)
	{
		result = inSpatialChannel();
	}
	
	return result;
}

std::string LLVivoxVoiceClient::sipURIFromID(const LLUUID &id)
{
	std::string result;
	result = "sip:";
	result += nameFromID(id);
	result += "@";
	result += mVoiceSIPURIHostName;
	
	return result;
}

std::string LLVivoxVoiceClient::sipURIFromAvatar(LLVOAvatar *avatar)
{
	std::string result;
	if(avatar)
	{
		result = "sip:";
		result += nameFromID(avatar->getID());
		result += "@";
		result += mVoiceSIPURIHostName;
	}
	
	return result;
}

std::string LLVivoxVoiceClient::nameFromAvatar(LLVOAvatar *avatar)
{
	std::string result;
	if(avatar)
	{
		result = nameFromID(avatar->getID());
	}	
	return result;
}

std::string LLVivoxVoiceClient::nameFromID(const LLUUID &uuid)
{
	std::string result;
	
	if (uuid.isNull()) {
		//VIVOX, the uuid emtpy look for the mURIString and return that instead.
		//result.assign(uuid.mURIStringName);
		LLStringUtil::replaceChar(result, '_', ' ');
		return result;
	}
	// Prepending this apparently prevents conflicts with reserved names inside the vivox code.
	result = "x";
	
	// Base64 encode and replace the pieces of base64 that are less compatible 
	// with e-mail local-parts.
	// See RFC-4648 "Base 64 Encoding with URL and Filename Safe Alphabet"
	result += LLBase64::encode(uuid.mData, UUID_BYTES);
	LLStringUtil::replaceChar(result, '+', '-');
	LLStringUtil::replaceChar(result, '/', '_');
	
	// If you need to transform a GUID to this form on the macOS command line, this will do so:
	// echo -n x && (echo e669132a-6c43-4ee1-a78d-6c82fff59f32 |xxd -r -p |openssl base64|tr '/+' '_-')
	
	// The reverse transform can be done with:
	// echo 'x5mkTKmxDTuGnjWyC__WfMg==' |cut -b 2- -|tr '_-' '/+' |openssl base64 -d|xxd -p
	
	return result;
}

bool LLVivoxVoiceClient::IDFromName(const std::string inName, LLUUID &uuid)
{
	bool result = false;
	
	// SLIM SDK: The "name" may actually be a SIP URI such as: "sip:xFnPP04IpREWNkuw1cOXlhw==@bhr.vivox.com"
	// If it is, convert to a bare name before doing the transform.
	std::string name = nameFromsipURI(inName);
	
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
		// VIVOX:  not a standard account name, just copy the URI name mURIString field
		// and hope for the best.  bpj
		uuid.setNull();  // VIVOX, set the uuid field to nulls
	}
	
	return result;
}

std::string LLVivoxVoiceClient::displayNameFromAvatar(LLVOAvatar *avatar)
{
	return avatar->getFullname();
}

std::string LLVivoxVoiceClient::sipURIFromName(std::string &name)
{
	std::string result;
	result = "sip:";
	result += name;
	result += "@";
	result += mVoiceSIPURIHostName;

//	LLStringUtil::toLower(result);

	return result;
}

std::string LLVivoxVoiceClient::nameFromsipURI(const std::string &uri)
{
	std::string result;

	std::string::size_type sipOffset, atOffset;
	sipOffset = uri.find("sip:");
	atOffset = uri.find("@");
	if((sipOffset != std::string::npos) && (atOffset != std::string::npos))
	{
		result = uri.substr(sipOffset + 4, atOffset - (sipOffset + 4));
	}
	
	return result;
}

bool LLVivoxVoiceClient::inSpatialChannel(void)
{
	bool result = false;
	
	if(mAudioSession)
    {
		result = mAudioSession->mIsSpatial;
    }
    
	return result;
}

std::string LLVivoxVoiceClient::getAudioSessionURI()
{
	std::string result;
	
	if(mAudioSession)
		result = mAudioSession->mSIPURI;
		
	return result;
}

std::string LLVivoxVoiceClient::getAudioSessionHandle()
{
	std::string result;
	
	if(mAudioSession)
		result = mAudioSession->mHandle;
		
	return result;
}


/////////////////////////////
// Sending updates of current state

void LLVivoxVoiceClient::enforceTether(void)
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

void LLVivoxVoiceClient::updatePosition(void)
{

	LLViewerRegion *region = gAgent.getRegion();
	if(region && isAgentAvatarValid())
	{
		LLMatrix3 rot;
		LLVector3d pos;
        LLQuaternion qrot;
		
		// TODO: If camera and avatar velocity are actually used by the voice system, we could compute them here...
		// They're currently always set to zero.
		
		// Send the current camera position to the voice code
		rot.setRows(LLViewerCamera::getInstance()->getAtAxis(), LLViewerCamera::getInstance()->getLeftAxis (),  LLViewerCamera::getInstance()->getUpAxis());		
		pos = gAgent.getRegion()->getPosGlobalFromRegion(LLViewerCamera::getInstance()->getOrigin());
		
		LLVivoxVoiceClient::getInstance()->setCameraPosition(
															 pos,				// position
															 LLVector3::zero, 	// velocity
															 rot);				// rotation matrix
		
		// Send the current avatar position to the voice code
        qrot = gAgentAvatarp->getRootJoint()->getWorldRotation();
		pos = gAgentAvatarp->getPositionGlobal();

		// TODO: Can we get the head offset from outside the LLVOAvatar?
		//			pos += LLVector3d(mHeadOffset);
		pos += LLVector3d(0.f, 0.f, 1.f);
		
		LLVivoxVoiceClient::getInstance()->setAvatarPosition(
															 pos,				// position
															 LLVector3::zero, 	// velocity
															 qrot);				// rotation matrix
	}
}

void LLVivoxVoiceClient::setCameraPosition(const LLVector3d &position, const LLVector3 &velocity, const LLMatrix3 &rot)
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

void LLVivoxVoiceClient::setAvatarPosition(const LLVector3d &position, const LLVector3 &velocity, const LLQuaternion &rot)
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

bool LLVivoxVoiceClient::channelFromRegion(LLViewerRegion *region, std::string &name)
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

void LLVivoxVoiceClient::leaveChannel(void)
{
    if (mIsInChannel)
	{
		LL_DEBUGS("Voice") << "leaving channel for teleport/logout" << LL_ENDL;
		mChannelName.clear();
		sessionTerminate();
	}
}

void LLVivoxVoiceClient::setMuteMic(bool muted)
{
	if(mMuteMic != muted)
	{
		mMuteMic = muted;
		mMuteMicDirty = true;
	}
}

void LLVivoxVoiceClient::setVoiceEnabled(bool enabled)
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
                LLCoros::instance().launch("LLVivoxVoiceClient::voiceControlCoro",
                    boost::bind(&LLVivoxVoiceClient::voiceControlCoro, LLVivoxVoiceClient::getInstance()));
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

bool LLVivoxVoiceClient::voiceEnabled()
{
    return gSavedSettings.getBOOL("EnableVoiceChat") &&
          !gSavedSettings.getBOOL("CmdLineDisableVoice") &&
          !gNonInteractive;
}

void LLVivoxVoiceClient::setLipSyncEnabled(bool enabled)
{
	mLipSyncEnabled = enabled;
}

bool LLVivoxVoiceClient::lipSyncEnabled()
{
	   
	if ( mVoiceEnabled )
	{
		return mLipSyncEnabled;
	}
	else
	{
		return false;
	}
}


void LLVivoxVoiceClient::setEarLocation(S32 loc)
{
	if(mEarLocation != loc)
	{
		LL_DEBUGS("Voice") << "Setting mEarLocation to " << loc << LL_ENDL;
		
		mEarLocation = loc;
		mSpatialCoordsDirty = true;
	}
}

void LLVivoxVoiceClient::setVoiceVolume(F32 volume)
{
	int scaled_volume = scale_speaker_volume(volume);	

	if(scaled_volume != mSpeakerVolume)
	{
	  int min_volume = scale_speaker_volume(0);
		if((scaled_volume == min_volume) || (mSpeakerVolume == min_volume))
		{
			mSpeakerMuteDirty = true;
		}

		mSpeakerVolume = scaled_volume;
		mSpeakerVolumeDirty = true;
	}
}

void LLVivoxVoiceClient::setMicGain(F32 volume)
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
bool LLVivoxVoiceClient::getVoiceEnabled(const LLUUID& id)
{
	bool result = false;
    participantStatePtr_t participant(findParticipantByID(id));
	if(participant)
	{
		// I'm not sure what the semantics of this should be.
		// For now, if we have any data about the user that came through the chat channel, assume they're voice-enabled.
		result = true;
	}
	
	return result;
}

std::string LLVivoxVoiceClient::getDisplayName(const LLUUID& id)
{
	std::string result;
    participantStatePtr_t participant(findParticipantByID(id));
	if(participant)
	{
		result = participant->mDisplayName;
	}
	
	return result;
}



bool LLVivoxVoiceClient::getIsSpeaking(const LLUUID& id)
{
	bool result = false;

    participantStatePtr_t participant(findParticipantByID(id));
	if(participant)
	{
		if (participant->mSpeakingTimeout.getElapsedTimeF32() > SPEAKING_TIMEOUT)
		{
			participant->mIsSpeaking = false;
		}
		result = participant->mIsSpeaking;
	}
	
	return result;
}

bool LLVivoxVoiceClient::getIsModeratorMuted(const LLUUID& id)
{
	bool result = false;

    participantStatePtr_t participant(findParticipantByID(id));
	if(participant)
	{
		result = participant->mIsModeratorMuted;
	}
	
	return result;
}

F32 LLVivoxVoiceClient::getCurrentPower(const LLUUID& id)
{		
	F32 result = 0;
    participantStatePtr_t participant(findParticipantByID(id));
	if(participant)
	{
		result = participant->mPower;
	}
	
	return result;
}



bool LLVivoxVoiceClient::getUsingPTT(const LLUUID& id)
{
	bool result = false;

    participantStatePtr_t participant(findParticipantByID(id));
	if(participant)
	{
		// I'm not sure what the semantics of this should be.
		// Does "using PTT" mean they're configured with a push-to-talk button?
		// For now, we know there's no PTT mechanism in place, so nobody is using it.
	}
	
	return result;
}

bool LLVivoxVoiceClient::getOnMuteList(const LLUUID& id)
{
	bool result = false;
	
    participantStatePtr_t participant(findParticipantByID(id));
	if(participant)
	{
		result = participant->mOnMuteList;
	}

	return result;
}

// External accessors.
F32 LLVivoxVoiceClient::getUserVolume(const LLUUID& id)
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

void LLVivoxVoiceClient::setUserVolume(const LLUUID& id, F32 volume)
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

std::string LLVivoxVoiceClient::getGroupID(const LLUUID& id)
{
	std::string result;

    participantStatePtr_t participant(findParticipantByID(id));
	if(participant)
	{
		result = participant->mGroupID;
	}
	
	return result;
}

bool LLVivoxVoiceClient::getAreaVoiceDisabled()
{
	return mAreaVoiceDisabled;
}

void LLVivoxVoiceClient::recordingLoopStart(int seconds, int deltaFramesPerControlFrame)
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

void LLVivoxVoiceClient::recordingLoopSave(const std::string& filename)
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

void LLVivoxVoiceClient::recordingStop()
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

void LLVivoxVoiceClient::filePlaybackStart(const std::string& filename)
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

void LLVivoxVoiceClient::filePlaybackStop()
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

void LLVivoxVoiceClient::filePlaybackSetPaused(bool paused)
{
	// TODO: Implement once Vivox gives me a sample
}

void LLVivoxVoiceClient::filePlaybackSetMode(bool vox, float speed)
{
	// TODO: Implement once Vivox gives me a sample
}

//------------------------------------------------------------------------
std::set<LLVivoxVoiceClient::sessionState::wptr_t> LLVivoxVoiceClient::sessionState::mSession;


LLVivoxVoiceClient::sessionState::sessionState() :
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
LLVivoxVoiceClient::sessionState::ptr_t LLVivoxVoiceClient::sessionState::createSession()
{
    sessionState::ptr_t ptr(new sessionState());

    std::pair<std::set<wptr_t>::iterator, bool>  result = mSession.insert(ptr);

    if (result.second)
        ptr->mMyIterator = result.first;

    return ptr;
}

LLVivoxVoiceClient::sessionState::~sessionState()
{
    LL_INFOS("Voice") << "Destroying session handle=" << mHandle << " SIP=" << mSIPURI << LL_ENDL;
    if (mMyIterator != mSession.end())
        mSession.erase(mMyIterator);

	removeAllParticipants();
}

bool LLVivoxVoiceClient::sessionState::isCallBackPossible()
{
	// This may change to be explicitly specified by vivox in the future...
	// Currently, only PSTN P2P calls cannot be returned.
	// Conveniently, this is also the only case where we synthesize a caller UUID.
	return !mSynthesizedCallerID;
}

bool LLVivoxVoiceClient::sessionState::isTextIMPossible()
{
	// This may change to be explicitly specified by vivox in the future...
	return !mSynthesizedCallerID;
}


/*static*/ 
LLVivoxVoiceClient::sessionState::ptr_t LLVivoxVoiceClient::sessionState::matchSessionByHandle(const std::string &handle)
{
    sessionStatePtr_t result;

    // *TODO: My kingdom for a lambda!
    std::set<wptr_t>::iterator it = std::find_if(mSession.begin(), mSession.end(), boost::bind(testByHandle, _1, handle));

    if (it != mSession.end())
        result = (*it).lock();

    return result;
}

/*static*/ 
LLVivoxVoiceClient::sessionState::ptr_t LLVivoxVoiceClient::sessionState::matchCreatingSessionByURI(const std::string &uri)
{
    sessionStatePtr_t result;

    // *TODO: My kingdom for a lambda!
    std::set<wptr_t>::iterator it = std::find_if(mSession.begin(), mSession.end(), boost::bind(testByCreatingURI, _1, uri));

    if (it != mSession.end())
        result = (*it).lock();

    return result;
}

/*static*/
LLVivoxVoiceClient::sessionState::ptr_t LLVivoxVoiceClient::sessionState::matchSessionByURI(const std::string &uri)
{
    sessionStatePtr_t result;

    // *TODO: My kingdom for a lambda!
    std::set<wptr_t>::iterator it = std::find_if(mSession.begin(), mSession.end(), boost::bind(testBySIPOrAlterateURI, _1, uri));

    if (it != mSession.end())
        result = (*it).lock();

    return result;
}

/*static*/
LLVivoxVoiceClient::sessionState::ptr_t LLVivoxVoiceClient::sessionState::matchSessionByParticipant(const LLUUID &participant_id)
{
    sessionStatePtr_t result;

    // *TODO: My kingdom for a lambda!
    std::set<wptr_t>::iterator it = std::find_if(mSession.begin(), mSession.end(), boost::bind(testByCallerId, _1, participant_id));

    if (it != mSession.end())
        result = (*it).lock();

    return result;
}

void LLVivoxVoiceClient::sessionState::for_each(sessionFunc_t func)
{
    std::for_each(mSession.begin(), mSession.end(), boost::bind(for_eachPredicate, _1, func));
}

// simple test predicates.  
// *TODO: These should be made into lambdas when we can pull the trigger on newer C++ features.
bool LLVivoxVoiceClient::sessionState::testByHandle(const LLVivoxVoiceClient::sessionState::wptr_t &a, std::string handle)
{
    ptr_t aLock(a.lock());

    return aLock ? aLock->mHandle == handle : false;
}

bool LLVivoxVoiceClient::sessionState::testByCreatingURI(const LLVivoxVoiceClient::sessionState::wptr_t &a, std::string uri)
{
    ptr_t aLock(a.lock());

    return aLock ? (aLock->mCreateInProgress && (aLock->mSIPURI == uri)) : false;
}

bool LLVivoxVoiceClient::sessionState::testBySIPOrAlterateURI(const LLVivoxVoiceClient::sessionState::wptr_t &a, std::string uri)
{
    ptr_t aLock(a.lock());

    return aLock ? ((aLock->mSIPURI == uri) || (aLock->mAlternateSIPURI == uri)) : false;
}


bool LLVivoxVoiceClient::sessionState::testByCallerId(const LLVivoxVoiceClient::sessionState::wptr_t &a, LLUUID participantId)
{
    ptr_t aLock(a.lock());

    return aLock ? ((aLock->mCallerID == participantId) || (aLock->mIMSessionID == participantId)) : false;
}

/*static*/
void LLVivoxVoiceClient::sessionState::for_eachPredicate(const LLVivoxVoiceClient::sessionState::wptr_t &a, sessionFunc_t func)
{
    ptr_t aLock(a.lock());

    if (aLock)
        func(aLock);
    else
    {
        LL_WARNS("Voice") << "Stale handle in session map!" << LL_ENDL;
    }
}



LLVivoxVoiceClient::sessionStatePtr_t LLVivoxVoiceClient::findSession(const std::string &handle)
{
    sessionStatePtr_t result;
	sessionMap::iterator iter = mSessionsByHandle.find(handle);
	if(iter != mSessionsByHandle.end())
	{
		result = iter->second;
	}
	
	return result;
}

LLVivoxVoiceClient::sessionStatePtr_t LLVivoxVoiceClient::findSessionBeingCreatedByURI(const std::string &uri)
{	
    sessionStatePtr_t result = sessionState::matchCreatingSessionByURI(uri);
	
	return result;
}

LLVivoxVoiceClient::sessionStatePtr_t LLVivoxVoiceClient::findSession(const LLUUID &participant_id)
{
    sessionStatePtr_t result = sessionState::matchSessionByParticipant(participant_id);
	
	return result;
}

LLVivoxVoiceClient::sessionStatePtr_t LLVivoxVoiceClient::addSession(const std::string &uri, const std::string &handle)
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

void LLVivoxVoiceClient::clearSessionHandle(const sessionStatePtr_t &session)
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

void LLVivoxVoiceClient::setSessionHandle(const sessionStatePtr_t &session, const std::string &handle)
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

void LLVivoxVoiceClient::setSessionURI(const sessionStatePtr_t &session, const std::string &uri)
{
	// There used to be a map of session URIs to sessions, which made this complex....
	session->mSIPURI = uri;

	verifySessionState();
}

void LLVivoxVoiceClient::deleteSession(const sessionStatePtr_t &session)
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

void LLVivoxVoiceClient::deleteAllSessions()
{
	LL_DEBUGS("Voice") << LL_ENDL;

    while (!mSessionsByHandle.empty())
	{
        const sessionStatePtr_t session = mSessionsByHandle.begin()->second;
        deleteSession(session);
	}
	
}

void LLVivoxVoiceClient::verifySessionState(void)
{
    LL_DEBUGS("Voice") << "Sessions in handle map=" << mSessionsByHandle.size() << LL_ENDL;
    sessionState::VerifySessions();
}

void LLVivoxVoiceClient::addObserver(LLVoiceClientParticipantObserver* observer)
{
	mParticipantObservers.insert(observer);
}

void LLVivoxVoiceClient::removeObserver(LLVoiceClientParticipantObserver* observer)
{
	mParticipantObservers.erase(observer);
}

void LLVivoxVoiceClient::notifyParticipantObservers()
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

void LLVivoxVoiceClient::addObserver(LLVoiceClientStatusObserver* observer)
{
	mStatusObservers.insert(observer);
}

void LLVivoxVoiceClient::removeObserver(LLVoiceClientStatusObserver* observer)
{
	mStatusObservers.erase(observer);
}

void LLVivoxVoiceClient::notifyStatusObservers(LLVoiceClientStatusObserver::EStatusType status)
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

void LLVivoxVoiceClient::addObserver(LLFriendObserver* observer)
{
	mFriendObservers.insert(observer);
}

void LLVivoxVoiceClient::removeObserver(LLFriendObserver* observer)
{
	mFriendObservers.erase(observer);
}

void LLVivoxVoiceClient::notifyFriendObservers()
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

void LLVivoxVoiceClient::lookupName(const LLUUID &id)
{
	if (mAvatarNameCacheConnection.connected())
	{
		mAvatarNameCacheConnection.disconnect();
	}
	mAvatarNameCacheConnection = LLAvatarNameCache::get(id, boost::bind(&LLVivoxVoiceClient::onAvatarNameCache, this, _1, _2));
}

void LLVivoxVoiceClient::onAvatarNameCache(const LLUUID& agent_id,
										   const LLAvatarName& av_name)
{
	mAvatarNameCacheConnection.disconnect();
	std::string display_name = av_name.getDisplayName();
	avatarNameResolved(agent_id, display_name);
}

void LLVivoxVoiceClient::predAvatarNameResolution(const LLVivoxVoiceClient::sessionStatePtr_t &session, LLUUID id, std::string name)
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

void LLVivoxVoiceClient::avatarNameResolved(const LLUUID &id, const std::string &name)
{
    sessionState::for_each(boost::bind(predAvatarNameResolution, _1, id, name));
}

bool LLVivoxVoiceClient::setVoiceEffect(const LLUUID& id)
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

const LLUUID LLVivoxVoiceClient::getVoiceEffect()
{
	return mAudioSession ? mAudioSession->mVoiceFontID : LLUUID::null;
}

LLSD LLVivoxVoiceClient::getVoiceEffectProperties(const LLUUID& id)
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

LLVivoxVoiceClient::voiceFontEntry::voiceFontEntry(LLUUID& id) :
	mID(id),
	mFontIndex(0),
	mFontType(VOICE_FONT_TYPE_NONE),
	mFontStatus(VOICE_FONT_STATUS_NONE),
	mIsNew(false)
{
	mExpiryTimer.stop();
	mExpiryWarningTimer.stop();
}

LLVivoxVoiceClient::voiceFontEntry::~voiceFontEntry()
{
}

void LLVivoxVoiceClient::refreshVoiceEffectLists(bool clear_lists)
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

const voice_effect_list_t& LLVivoxVoiceClient::getVoiceEffectList() const
{
	return mVoiceFontList;
}

const voice_effect_list_t& LLVivoxVoiceClient::getVoiceEffectTemplateList() const
{
	return mVoiceFontTemplateList;
}

void LLVivoxVoiceClient::addVoiceFont(const S32 font_index,
								 const std::string &name,
								 const std::string &description,
								 const LLDate &expiration_date,
								 bool has_expired,
								 const S32 font_type,
								 const S32 font_status,
								 const bool template_font)
{
	// Vivox SessionFontIDs are not guaranteed to remain the same between
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

void LLVivoxVoiceClient::expireVoiceFonts()
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

void LLVivoxVoiceClient::deleteVoiceFont(const LLUUID& id)
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

void LLVivoxVoiceClient::deleteAllVoiceFonts()
{
	mVoiceFontList.clear();

	voice_font_map_t::iterator iter;
	for (iter = mVoiceFontMap.begin(); iter != mVoiceFontMap.end(); ++iter)
	{
		delete iter->second;
	}
	mVoiceFontMap.clear();
}

void LLVivoxVoiceClient::deleteVoiceFontTemplates()
{
	mVoiceFontTemplateList.clear();

	voice_font_map_t::iterator iter;
	for (iter = mVoiceFontTemplateMap.begin(); iter != mVoiceFontTemplateMap.end(); ++iter)
	{
		delete iter->second;
	}
	mVoiceFontTemplateMap.clear();
}

S32 LLVivoxVoiceClient::getVoiceFontIndex(const LLUUID& id) const
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

S32 LLVivoxVoiceClient::getVoiceFontTemplateIndex(const LLUUID& id) const
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

void LLVivoxVoiceClient::accountGetSessionFontsSendMessage()
{
	if(mAccountLoggedIn)
	{
		std::ostringstream stream;

		LL_DEBUGS("VoiceFont") << "Requesting voice font list." << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.GetSessionFonts.1\">"
		<< "<AccountHandle>" << LLVivoxSecurity::getInstance()->accountHandle() << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLVivoxVoiceClient::accountGetTemplateFontsSendMessage()
{
	if(mAccountLoggedIn)
	{
		std::ostringstream stream;

		LL_DEBUGS("VoiceFont") << "Requesting voice font template list." << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.GetTemplateFonts.1\">"
		<< "<AccountHandle>" << LLVivoxSecurity::getInstance()->accountHandle() << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLVivoxVoiceClient::sessionSetVoiceFontSendMessage(const sessionStatePtr_t &session)
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

void LLVivoxVoiceClient::accountGetSessionFontsResponse(int statusCode, const std::string &statusString)
{
    if (mIsWaitingForFonts)
    {
        // *TODO: We seem to get multiple events of this type.  Should figure a way to advance only after
        // receiving the last one.
        LLSD result(LLSDMap("voice_fonts", LLSD::Boolean(true)));

        mVivoxPump.post(result);
    }
	notifyVoiceFontObservers();
	mVoiceFontsReceived = true;
}

void LLVivoxVoiceClient::accountGetTemplateFontsResponse(int statusCode, const std::string &statusString)
{
	// Voice font list entries were updated via addVoiceFont() during parsing.
	notifyVoiceFontObservers();
}
void LLVivoxVoiceClient::addObserver(LLVoiceEffectObserver* observer)
{
	mVoiceFontObservers.insert(observer);
}

void LLVivoxVoiceClient::removeObserver(LLVoiceEffectObserver* observer)
{
	mVoiceFontObservers.erase(observer);
}

// method checks the item in VoiceMorphing menu for appropriate current voice font
bool LLVivoxVoiceClient::onCheckVoiceEffect(const std::string& voice_effect_name)
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
void LLVivoxVoiceClient::onClickVoiceEffect(const std::string& voice_effect_name)
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
void LLVivoxVoiceClient::updateVoiceMorphingMenu()
{
	if (mVoiceFontListDirty)
	{
		LLVoiceEffectInterface * effect_interfacep = LLVoiceClient::instance().getVoiceEffectInterface();
		if (effect_interfacep)
		{
			const voice_effect_list_t& effect_list = effect_interfacep->getVoiceEffectList();
			if (!effect_list.empty())
			{
				LLMenuGL * voice_morphing_menup = gMenuBarView->findChildMenuByName("VoiceMorphing", true);

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
							p.on_check.function(boost::bind(&LLVivoxVoiceClient::onCheckVoiceEffect, this, it->first));
							p.on_click.function(boost::bind(&LLVivoxVoiceClient::onClickVoiceEffect, this, it->first));
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
void LLVivoxVoiceClient::notifyVoiceFontObservers()
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

void LLVivoxVoiceClient::enablePreviewBuffer(bool enable)
{
    LLSD result;
    mCaptureBufferMode = enable;

    if (enable)
        result["recplay"] = "start";
    else
        result["recplay"] = "quit";

    mVivoxPump.post(result);

	if(mCaptureBufferMode && mIsInChannel)
	{
		LL_DEBUGS("Voice") << "no channel" << LL_ENDL;
		sessionTerminate();
	}
}

void LLVivoxVoiceClient::recordPreviewBuffer()
{
	if (!mCaptureBufferMode)
	{
		LL_DEBUGS("Voice") << "Not in voice effect preview mode, cannot start recording." << LL_ENDL;
		mCaptureBufferRecording = false;
		return;
	}

	mCaptureBufferRecording = true;

    LLSD result(LLSDMap("recplay", "record"));
    mVivoxPump.post(result);
}

void LLVivoxVoiceClient::playPreviewBuffer(const LLUUID& effect_id)
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
    mVivoxPump.post(result);
}

void LLVivoxVoiceClient::stopPreviewBuffer()
{
	mCaptureBufferRecording = false;
	mCaptureBufferPlaying = false;

    LLSD result(LLSDMap("recplay", "quit"));
    mVivoxPump.post(result);
}

bool LLVivoxVoiceClient::isPreviewRecording()
{
	return (mCaptureBufferMode && mCaptureBufferRecording);
}

bool LLVivoxVoiceClient::isPreviewPlaying()
{
	return (mCaptureBufferMode && mCaptureBufferPlaying);
}

void LLVivoxVoiceClient::captureBufferRecordStartSendMessage()
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
			<< "<ConnectorHandle>" << LLVivoxSecurity::getInstance()->connectorHandle() << "</ConnectorHandle>"
			<< "<Value>false</Value>"
		<< "</Request>\n\n\n";

		// Dirty the mute mic state so that it will get reset when we finishing previewing
		mMuteMicDirty = true;

		writeString(stream.str());
	}
}

void LLVivoxVoiceClient::captureBufferRecordStopSendMessage()
{
	if(mAccountLoggedIn)
	{
		std::ostringstream stream;

		LL_DEBUGS("Voice") << "Stopping audio capture to buffer." << LL_ENDL;

		// Mute the mic. Mic mute state was dirtied at recording start, so will be reset when finished previewing.
		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.MuteLocalMic.1\">"
			<< "<ConnectorHandle>" << LLVivoxSecurity::getInstance()->connectorHandle() << "</ConnectorHandle>"
			<< "<Value>true</Value>"
		<< "</Request>\n\n\n";

		// Stop capture
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.CaptureAudioStop.1\">"
			<< "<AccountHandle>" << LLVivoxSecurity::getInstance()->accountHandle() << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLVivoxVoiceClient::captureBufferPlayStartSendMessage(const LLUUID& voice_font_id)
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
			<< "<AccountHandle>" << LLVivoxSecurity::getInstance()->accountHandle() << "</AccountHandle>"
			<< "<TemplateFontID>" << font_index << "</TemplateFontID>"
			<< "<FontDelta />"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLVivoxVoiceClient::captureBufferPlayStopSendMessage()
{
	if(mAccountLoggedIn)
	{
		std::ostringstream stream;

		LL_DEBUGS("Voice") << "Stopping audio buffer playback." << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.RenderAudioStop.1\">"
			<< "<AccountHandle>" << LLVivoxSecurity::getInstance()->accountHandle() << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

LLVivoxProtocolParser::LLVivoxProtocolParser()
{
	parser = XML_ParserCreate(NULL);
	
	reset();
}

void LLVivoxProtocolParser::reset()
{
	responseDepth = 0;
	ignoringTags = false;
	accumulateText = false;
	energy = 0.f;
	hasText = false;
	hasAudio = false;
	hasVideo = false;
	terminated = false;
	ignoreDepth = 0;
	isChannel = false;
	incoming = false;
	enabled = false;
	isEvent = false;
	isLocallyMuted = false;
	isModeratorMuted = false;
	isSpeaking = false;
	participantType = 0;
	returnCode = -1;
	state = 0;
	statusCode = 0;
	volume = 0;
	textBuffer.clear();
	alias.clear();
	numberOfAliases = 0;
	applicationString.clear();
}

//virtual 
LLVivoxProtocolParser::~LLVivoxProtocolParser()
{
	if (parser)
		XML_ParserFree(parser);
}

static LLTrace::BlockTimerStatHandle FTM_VIVOX_PROCESS("Vivox Process");

// virtual
LLIOPipe::EStatus LLVivoxProtocolParser::process_impl(
													  const LLChannelDescriptors& channels,
													  buffer_ptr_t& buffer,
													  bool& eos,
													  LLSD& context,
													  LLPumpIO* pump)
{
	LL_RECORD_BLOCK_TIME(FTM_VIVOX_PROCESS);
	LLBufferStream istr(channels, buffer.get());
	std::ostringstream ostr;
	while (istr.good())
	{
		char buf[1024];
		istr.read(buf, sizeof(buf));
		mInput.append(buf, istr.gcount());
	}
	
	// Look for input delimiter(s) in the input buffer.  If one is found, send the message to the xml parser.
	int start = 0;
	int delim;
	while((delim = mInput.find("\n\n\n", start)) != std::string::npos)
	{	
		
		// Reset internal state of the LLVivoxProtocolParser (no effect on the expat parser)
		reset();
		
		XML_ParserReset(parser, NULL);
		XML_SetElementHandler(parser, ExpatStartTag, ExpatEndTag);
		XML_SetCharacterDataHandler(parser, ExpatCharHandler);
		XML_SetUserData(parser, this);	
		XML_Parse(parser, mInput.data() + start, delim - start, false);
		
        LL_DEBUGS("VivoxProtocolParser") << "parsing: " << mInput.substr(start, delim - start) << LL_ENDL;
		start = delim + 3;
	}
	
	if(start != 0)
		mInput = mInput.substr(start);
	
	LL_DEBUGS("VivoxProtocolParser") << "at end, mInput is: " << mInput << LL_ENDL;
	
	if(!LLVivoxVoiceClient::sConnected)
	{
		// If voice has been disabled, we just want to close the socket.  This does so.
		LL_INFOS("Voice") << "returning STATUS_STOP" << LL_ENDL;
		return STATUS_STOP;
	}
	
	return STATUS_OK;
}

void XMLCALL LLVivoxProtocolParser::ExpatStartTag(void *data, const char *el, const char **attr)
{
	if (data)
	{
		LLVivoxProtocolParser	*object = (LLVivoxProtocolParser*)data;
		object->StartTag(el, attr);
	}
}

// --------------------------------------------------------------------------------

void XMLCALL LLVivoxProtocolParser::ExpatEndTag(void *data, const char *el)
{
	if (data)
	{
		LLVivoxProtocolParser	*object = (LLVivoxProtocolParser*)data;
		object->EndTag(el);
	}
}

// --------------------------------------------------------------------------------

void XMLCALL LLVivoxProtocolParser::ExpatCharHandler(void *data, const XML_Char *s, int len)
{
	if (data)
	{
		LLVivoxProtocolParser	*object = (LLVivoxProtocolParser*)data;
		object->CharData(s, len);
	}
}

// --------------------------------------------------------------------------------


void LLVivoxProtocolParser::StartTag(const char *tag, const char **attr)
{
	// Reset the text accumulator. We shouldn't have strings that are inturrupted by new tags
	textBuffer.clear();
	// only accumulate text if we're not ignoring tags.
	accumulateText = !ignoringTags;
	
	if (responseDepth == 0)
	{	
		isEvent = !stricmp("Event", tag);
		
		if (!stricmp("Response", tag) || isEvent)
		{
			// Grab the attributes
			while (*attr)
			{
				const char	*key = *attr++;
				const char	*value = *attr++;
				
				if (!stricmp("requestId", key))
				{
					requestId = value;
				}
				else if (!stricmp("action", key))
				{
					actionString = value;
				}
				else if (!stricmp("type", key))
				{
					eventTypeString = value;
				}
			}
		}
		LL_DEBUGS("VivoxProtocolParser") << tag << " (" << responseDepth << ")"  << LL_ENDL;
	}
	else
	{
		if (ignoringTags)
		{
			LL_DEBUGS("VivoxProtocolParser") << "ignoring tag " << tag << " (depth = " << responseDepth << ")" << LL_ENDL;
		}
		else
		{
			LL_DEBUGS("VivoxProtocolParser") << tag << " (" << responseDepth << ")"  << LL_ENDL;
			
			// Ignore the InputXml stuff so we don't get confused
			if (!stricmp("InputXml", tag))
			{
				ignoringTags = true;
				ignoreDepth = responseDepth;
				accumulateText = false;
				
				LL_DEBUGS("VivoxProtocolParser") << "starting ignore, ignoreDepth is " << ignoreDepth << LL_ENDL;
			}
			else if (!stricmp("CaptureDevices", tag))
			{
				LLVivoxVoiceClient::getInstance()->clearCaptureDevices();
			}			
			else if (!stricmp("RenderDevices", tag))
			{
				LLVivoxVoiceClient::getInstance()->clearRenderDevices();
			}
			else if (!stricmp("CaptureDevice", tag))
			{
				deviceString.clear();
			}
			else if (!stricmp("RenderDevice", tag))
			{
				deviceString.clear();
			}			
			else if (!stricmp("SessionFont", tag))
			{
				id = 0;
				nameString.clear();
				descriptionString.clear();
				expirationDate = LLDate();
				hasExpired = false;
				fontType = 0;
				fontStatus = 0;
			}
			else if (!stricmp("TemplateFont", tag))
			{
				id = 0;
				nameString.clear();
				descriptionString.clear();
				expirationDate = LLDate();
				hasExpired = false;
				fontType = 0;
				fontStatus = 0;
			}
			else if (!stricmp("MediaCompletionType", tag))
			{
				mediaCompletionType.clear();
			}
		}
	}
	responseDepth++;
}

// --------------------------------------------------------------------------------

void LLVivoxProtocolParser::EndTag(const char *tag)
{
	const std::string& string = textBuffer;
	
	responseDepth--;
	
	if (ignoringTags)
	{
		if (ignoreDepth == responseDepth)
		{
			LL_DEBUGS("VivoxProtocolParser") << "end of ignore" << LL_ENDL;
			ignoringTags = false;
		}
		else
		{
			LL_DEBUGS("VivoxProtocolParser") << "ignoring tag " << tag << " (depth = " << responseDepth << ")" << LL_ENDL;
		}
	}
	
	if (!ignoringTags)
	{
		LL_DEBUGS("VivoxProtocolParser") << "processing tag " << tag << " (depth = " << responseDepth << ")" << LL_ENDL;
		
		// Closing a tag. Finalize the text we've accumulated and reset
		if (!stricmp("ReturnCode", tag))
			returnCode = strtol(string.c_str(), NULL, 10);
		else if (!stricmp("SessionHandle", tag))
			sessionHandle = string;
		else if (!stricmp("SessionGroupHandle", tag))
			sessionGroupHandle = string;
		else if (!stricmp("StatusCode", tag))
			statusCode = strtol(string.c_str(), NULL, 10);
		else if (!stricmp("StatusString", tag))
			statusString = string;
		else if (!stricmp("ParticipantURI", tag))
			uriString = string;
		else if (!stricmp("Volume", tag))
			volume = strtol(string.c_str(), NULL, 10);
		else if (!stricmp("Energy", tag))
			energy = (F32)strtod(string.c_str(), NULL);
		else if (!stricmp("IsModeratorMuted", tag))
			isModeratorMuted = !stricmp(string.c_str(), "true");
		else if (!stricmp("IsSpeaking", tag))
			isSpeaking = !stricmp(string.c_str(), "true");
		else if (!stricmp("Alias", tag))
			alias = string;
		else if (!stricmp("NumberOfAliases", tag))
			numberOfAliases = strtol(string.c_str(), NULL, 10);
		else if (!stricmp("Application", tag))
			applicationString = string;
		else if (!stricmp("ConnectorHandle", tag))
			connectorHandle = string;
		else if (!stricmp("VersionID", tag))
			versionID = string;
		else if (!stricmp("Version", tag))
			mBuildID = string;
		else if (!stricmp("AccountHandle", tag))
			accountHandle = string;
		else if (!stricmp("State", tag))
			state = strtol(string.c_str(), NULL, 10);
		else if (!stricmp("URI", tag))
			uriString = string;
		else if (!stricmp("IsChannel", tag))
			isChannel = !stricmp(string.c_str(), "true");
		else if (!stricmp("Incoming", tag))
			incoming = !stricmp(string.c_str(), "true");
		else if (!stricmp("Enabled", tag))
			enabled = !stricmp(string.c_str(), "true");
		else if (!stricmp("Name", tag))
			nameString = string;
		else if (!stricmp("AudioMedia", tag))
			audioMediaString = string;
		else if (!stricmp("ChannelName", tag))
			nameString = string;
		else if (!stricmp("DisplayName", tag))
			displayNameString = string;
		else if (!stricmp("Device", tag))
			deviceString = string;		
		else if (!stricmp("AccountName", tag))
			nameString = string;
		else if (!stricmp("ParticipantType", tag))
			participantType = strtol(string.c_str(), NULL, 10);
		else if (!stricmp("IsLocallyMuted", tag))
			isLocallyMuted = !stricmp(string.c_str(), "true");
		else if (!stricmp("MicEnergy", tag))
			energy = (F32)strtod(string.c_str(), NULL);
		else if (!stricmp("ChannelName", tag))
			nameString = string;
		else if (!stricmp("ChannelURI", tag))
			uriString = string;
		else if (!stricmp("BuddyURI", tag))
			uriString = string;
		else if (!stricmp("Presence", tag))
			statusString = string;
		else if (!stricmp("CaptureDevices", tag))
		{
			LLVivoxVoiceClient::getInstance()->setDevicesListUpdated(true);
		}
		else if (!stricmp("RenderDevices", tag))
		{
			LLVivoxVoiceClient::getInstance()->setDevicesListUpdated(true);
		}
		else if (!stricmp("CaptureDevice", tag))
		{
			LLVivoxVoiceClient::getInstance()->addCaptureDevice(LLVoiceDevice(displayNameString, deviceString));
		}
		else if (!stricmp("RenderDevice", tag))
		{
			LLVivoxVoiceClient::getInstance()->addRenderDevice(LLVoiceDevice(displayNameString, deviceString));
		}
		else if (!stricmp("BlockMask", tag))
			blockMask = string;
		else if (!stricmp("PresenceOnly", tag))
			presenceOnly = string;
		else if (!stricmp("AutoAcceptMask", tag))
			autoAcceptMask = string;
		else if (!stricmp("AutoAddAsBuddy", tag))
			autoAddAsBuddy = string;
		else if (!stricmp("MessageHeader", tag))
			messageHeader = string;
		else if (!stricmp("MessageBody", tag))
			messageBody = string;
		else if (!stricmp("NotificationType", tag))
			notificationType = string;
		else if (!stricmp("HasText", tag))
			hasText = !stricmp(string.c_str(), "true");
		else if (!stricmp("HasAudio", tag))
			hasAudio = !stricmp(string.c_str(), "true");
		else if (!stricmp("HasVideo", tag))
			hasVideo = !stricmp(string.c_str(), "true");
		else if (!stricmp("Terminated", tag))
			terminated = !stricmp(string.c_str(), "true");
		else if (!stricmp("SubscriptionHandle", tag))
			subscriptionHandle = string;
		else if (!stricmp("SubscriptionType", tag))
			subscriptionType = string;
		else if (!stricmp("SessionFont", tag))
		{
			LLVivoxVoiceClient::getInstance()->addVoiceFont(id, nameString, descriptionString, expirationDate, hasExpired, fontType, fontStatus, false);
		}
		else if (!stricmp("TemplateFont", tag))
		{
			LLVivoxVoiceClient::getInstance()->addVoiceFont(id, nameString, descriptionString, expirationDate, hasExpired, fontType, fontStatus, true);
		}
		else if (!stricmp("ID", tag))
		{
			id = strtol(string.c_str(), NULL, 10);
		}
		else if (!stricmp("Description", tag))
		{
			descriptionString = string;
		}
		else if (!stricmp("ExpirationDate", tag))
		{
			expirationDate = expiryTimeStampToLLDate(string);
		}
		else if (!stricmp("Expired", tag))
		{
			hasExpired = !stricmp(string.c_str(), "1");
		}
		else if (!stricmp("Type", tag))
		{
			fontType = strtol(string.c_str(), NULL, 10);
		}
		else if (!stricmp("Status", tag))
		{
			fontStatus = strtol(string.c_str(), NULL, 10);
		}
		else if (!stricmp("MediaCompletionType", tag))
		{
			mediaCompletionType = string;;
		}

		textBuffer.clear();
		accumulateText= false;
		
		if (responseDepth == 0)
		{
			// We finished all of the XML, process the data
			processResponse(tag);
		}
	}
}

// --------------------------------------------------------------------------------

void LLVivoxProtocolParser::CharData(const char *buffer, int length)
{
	/*
	 This method is called for anything that isn't a tag, which can be text you
	 want that lies between tags, and a lot of stuff you don't want like file formatting
	 (tabs, spaces, CR/LF, etc).
	 
	 Only copy text if we are in accumulate mode...
	 */
	if (accumulateText)
		textBuffer.append(buffer, length);
}

// --------------------------------------------------------------------------------

LLDate LLVivoxProtocolParser::expiryTimeStampToLLDate(const std::string& vivox_ts)
{
	// *HACK: Vivox reports the time incorrectly. LLDate also only parses a
	// subset of valid ISO 8601 dates (only handles Z, not offsets).
	// So just use the date portion and fix the time here.
	std::string time_stamp = vivox_ts.substr(0, 10);
	time_stamp += VOICE_FONT_EXPIRY_TIME;

	LL_DEBUGS("VivoxProtocolParser") << "Vivox timestamp " << vivox_ts << " modified to: " << time_stamp << LL_ENDL;

	return LLDate(time_stamp);
}

// --------------------------------------------------------------------------------

void LLVivoxProtocolParser::processResponse(std::string tag)
{
	LL_DEBUGS("VivoxProtocolParser") << tag << LL_ENDL;
	
	// SLIM SDK: the SDK now returns a statusCode of "200" (OK) for success.  This is a change vs. previous SDKs.
	// According to Mike S., "The actual API convention is that responses with return codes of 0 are successful, regardless of the status code returned",
	// so I believe this will give correct behavior.
	
	if(returnCode == 0)
		statusCode = 0;
	
	if (isEvent)
	{
		const char *eventTypeCstr = eventTypeString.c_str();
        LL_DEBUGS("LowVoice") << eventTypeCstr << LL_ENDL;

		if (!stricmp(eventTypeCstr, "ParticipantUpdatedEvent"))
		{
			// These happen so often that logging them is pretty useless.
            LL_DEBUGS("LowVoice") << "Updated Params: " << sessionHandle << ", " << sessionGroupHandle << ", " << uriString << ", " << alias << ", " << isModeratorMuted << ", " << isSpeaking << ", " << volume << ", " << energy << LL_ENDL;
            LLVivoxVoiceClient::getInstance()->participantUpdatedEvent(sessionHandle, sessionGroupHandle, uriString, alias, isModeratorMuted, isSpeaking, volume, energy);
		}
		else if (!stricmp(eventTypeCstr, "AccountLoginStateChangeEvent"))
		{
			LLVivoxVoiceClient::getInstance()->accountLoginStateChangeEvent(accountHandle, statusCode, statusString, state);
		}
		else if (!stricmp(eventTypeCstr, "SessionAddedEvent"))
		{
			/*
			 <Event type="SessionAddedEvent">
			 <SessionGroupHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==_sg0</SessionGroupHandle>
			 <SessionHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==0</SessionHandle>
			 <Uri>sip:confctl-1408789@bhr.vivox.com</Uri>
			 <IsChannel>true</IsChannel>
			 <Incoming>false</Incoming>
			 <ChannelName />
			 </Event>
			 */
			LLVivoxVoiceClient::getInstance()->sessionAddedEvent(uriString, alias, sessionHandle, sessionGroupHandle, isChannel, incoming, nameString, applicationString);
		}
		else if (!stricmp(eventTypeCstr, "SessionRemovedEvent"))
		{
			LLVivoxVoiceClient::getInstance()->sessionRemovedEvent(sessionHandle, sessionGroupHandle);
		}
		else if (!stricmp(eventTypeCstr, "SessionGroupUpdatedEvent"))
		{
			//nothng useful to process for this event, but we should not WARN that we have received it.
		}
		else if (!stricmp(eventTypeCstr, "SessionGroupAddedEvent"))
		{
			LLVivoxVoiceClient::getInstance()->sessionGroupAddedEvent(sessionGroupHandle);
		}
		else if (!stricmp(eventTypeCstr, "MediaStreamUpdatedEvent"))
		{
			/*
			 <Event type="MediaStreamUpdatedEvent">
			 <SessionGroupHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==_sg0</SessionGroupHandle>
			 <SessionHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==0</SessionHandle>
			 <StatusCode>200</StatusCode>
			 <StatusString>OK</StatusString>
			 <State>2</State>
			 <Incoming>false</Incoming>
			 </Event>
			 */
			LLVivoxVoiceClient::getInstance()->mediaStreamUpdatedEvent(sessionHandle, sessionGroupHandle, statusCode, statusString, state, incoming);
		}
		else if (!stricmp(eventTypeCstr, "MediaCompletionEvent"))
		{
			/*
			<Event type="MediaCompletionEvent">
			<SessionGroupHandle />
			<MediaCompletionType>AuxBufferAudioCapture</MediaCompletionType>
			</Event>
			*/
			LLVivoxVoiceClient::getInstance()->mediaCompletionEvent(sessionGroupHandle, mediaCompletionType);
		}
		else if (!stricmp(eventTypeCstr, "ParticipantAddedEvent"))
		{
			/* 
			 <Event type="ParticipantAddedEvent">
			 <SessionGroupHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==_sg4</SessionGroupHandle>
			 <SessionHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==4</SessionHandle>
			 <ParticipantUri>sip:xI5auBZ60SJWIk606-1JGRQ==@bhr.vivox.com</ParticipantUri>
			 <AccountName>xI5auBZ60SJWIk606-1JGRQ==</AccountName>
			 <DisplayName />
			 <ParticipantType>0</ParticipantType>
			 </Event>
			 */
            LL_DEBUGS("LowVoice") << "Added Params: " << sessionHandle << ", " << sessionGroupHandle << ", " << uriString << ", " << alias << ", " << nameString << ", " << displayNameString << ", " << participantType << LL_ENDL;
			LLVivoxVoiceClient::getInstance()->participantAddedEvent(sessionHandle, sessionGroupHandle, uriString, alias, nameString, displayNameString, participantType);
		}
		else if (!stricmp(eventTypeCstr, "ParticipantRemovedEvent"))
		{
			/*
			 <Event type="ParticipantRemovedEvent">
			 <SessionGroupHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==_sg4</SessionGroupHandle>
			 <SessionHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==4</SessionHandle>
			 <ParticipantUri>sip:xtx7YNV-3SGiG7rA1fo5Ndw==@bhr.vivox.com</ParticipantUri>
			 <AccountName>xtx7YNV-3SGiG7rA1fo5Ndw==</AccountName>
			 </Event>
			 */
            LL_DEBUGS("LowVoice") << "Removed params:" << sessionHandle << ", " << sessionGroupHandle << ", " << uriString << ", " << alias << ", " << nameString << LL_ENDL;

			LLVivoxVoiceClient::getInstance()->participantRemovedEvent(sessionHandle, sessionGroupHandle, uriString, alias, nameString);
		}
		else if (!stricmp(eventTypeCstr, "AuxAudioPropertiesEvent"))
		{
			// These are really spammy in tuning mode
			LLVivoxVoiceClient::getInstance()->auxAudioPropertiesEvent(energy);
		}
		else if (!stricmp(eventTypeCstr, "MessageEvent"))  
		{
			//TODO:  This probably is not received any more, it was used to support SLim clients
			LLVivoxVoiceClient::getInstance()->messageEvent(sessionHandle, uriString, alias, messageHeader, messageBody, applicationString);
		}
		else if (!stricmp(eventTypeCstr, "SessionNotificationEvent"))  
		{
			//TODO:  This probably is not received any more, it was used to support SLim clients
			LLVivoxVoiceClient::getInstance()->sessionNotificationEvent(sessionHandle, uriString, notificationType);
		}
		else if (!stricmp(eventTypeCstr, "SessionUpdatedEvent"))
		{
			/*
			 <Event type="SessionUpdatedEvent">
			 <SessionGroupHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==_sg0</SessionGroupHandle>
			 <SessionHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==0</SessionHandle>
			 <Uri>sip:confctl-9@bhd.vivox.com</Uri>
			 <IsMuted>0</IsMuted>
			 <Volume>50</Volume>
			 <TransmitEnabled>1</TransmitEnabled>
			 <IsFocused>0</IsFocused>
			 <SpeakerPosition><Position><X>0</X><Y>0</Y><Z>0</Z></Position></SpeakerPosition>
			 <SessionFontID>0</SessionFontID>
			 </Event>
			 */
			// We don't need to process this, but we also shouldn't warn on it, since that confuses people.
		}
		else if (!stricmp(eventTypeCstr, "SessionGroupRemovedEvent"))
		{
			// We don't need to process this, but we also shouldn't warn on it, since that confuses people.
		}
		else if (!stricmp(eventTypeCstr, "VoiceServiceConnectionStateChangedEvent"))
		{
			LLVivoxVoiceClient::getInstance()->voiceServiceConnectionStateChangedEvent(statusCode, statusString, mBuildID);
		}
		else if (!stricmp(eventTypeCstr, "AudioDeviceHotSwapEvent"))
		{
			/*
			<Event type = "AudioDeviceHotSwapEvent">
			<EventType>RenderDeviceChanged< / EventType>
			<RelevantDevice>
			<Device>Speakers(Turtle Beach P11 Headset)< / Device>
			<DisplayName>Speakers(Turtle Beach P11 Headset)< / DisplayName>
			<Type>SpecificDevice< / Type>
			< / RelevantDevice>
			< / Event>
			*/
			// an audio device was removed or added, fetch and update the local list of audio devices.
			LLVivoxVoiceClient::getInstance()->getCaptureDevicesSendMessage();
			LLVivoxVoiceClient::getInstance()->getRenderDevicesSendMessage();
		}
		else
		{
			LL_WARNS("VivoxProtocolParser") << "Unknown event type " << eventTypeString << LL_ENDL;
		}
	}
	else
	{
		const char *actionCstr = actionString.c_str();
        LL_DEBUGS("LowVoice") << actionCstr << LL_ENDL;

		if (!stricmp(actionCstr, "Session.Set3DPosition.1"))
		{
			// We don't need to process these
		}
		else if (!stricmp(actionCstr, "Connector.Create.1"))
		{
			LLVivoxVoiceClient::getInstance()->connectorCreateResponse(statusCode, statusString, connectorHandle, versionID);
		}
		else if (!stricmp(actionCstr, "Account.Login.1"))
		{
			LLVivoxVoiceClient::getInstance()->loginResponse(statusCode, statusString, accountHandle, numberOfAliases);
		}
		else if (!stricmp(actionCstr, "Session.Create.1"))
		{
			LLVivoxVoiceClient::getInstance()->sessionCreateResponse(requestId, statusCode, statusString, sessionHandle);			
		}
		else if (!stricmp(actionCstr, "SessionGroup.AddSession.1"))
		{
			LLVivoxVoiceClient::getInstance()->sessionGroupAddSessionResponse(requestId, statusCode, statusString, sessionHandle);			
		}
		else if (!stricmp(actionCstr, "Session.Connect.1"))
		{
			LLVivoxVoiceClient::getInstance()->sessionConnectResponse(requestId, statusCode, statusString);			
		}
		else if (!stricmp(actionCstr, "Account.Logout.1"))
		{
			LLVivoxVoiceClient::getInstance()->logoutResponse(statusCode, statusString);			
		}
		else if (!stricmp(actionCstr, "Connector.InitiateShutdown.1"))
		{
			LLVivoxVoiceClient::getInstance()->connectorShutdownResponse(statusCode, statusString);			
		}
		else if (!stricmp(actionCstr, "Account.GetSessionFonts.1"))
		{
			LLVivoxVoiceClient::getInstance()->accountGetSessionFontsResponse(statusCode, statusString);
		}
		else if (!stricmp(actionCstr, "Account.GetTemplateFonts.1"))
		{
			LLVivoxVoiceClient::getInstance()->accountGetTemplateFontsResponse(statusCode, statusString);
		}
		else if (!stricmp(actionCstr, "Aux.SetVadProperties.1"))
		{
			// both values of statusCode (old and more recent) indicate valid requests
			if (statusCode != 0 && statusCode != 200)
			{
				LL_WARNS("Voice") << "Aux.SetVadProperties.1 request failed: "
					<< "statusCode: " << statusCode
					<< " and "
					<< "statusString: " << statusString
					<< LL_ENDL;
			}
		}
		/*
		 else if (!stricmp(actionCstr, "Account.ChannelGetList.1"))
		 {
		 LLVoiceClient::getInstance()->channelGetListResponse(statusCode, statusString);
		 }
		 else if (!stricmp(actionCstr, "Connector.AccountCreate.1"))
		 {
		 
		 }
		 else if (!stricmp(actionCstr, "Connector.MuteLocalMic.1"))
		 {
		 
		 }
		 else if (!stricmp(actionCstr, "Connector.MuteLocalSpeaker.1"))
		 {
		 
		 }
		 else if (!stricmp(actionCstr, "Connector.SetLocalMicVolume.1"))
		 {
		 
		 }
		 else if (!stricmp(actionCstr, "Connector.SetLocalSpeakerVolume.1"))
		 {
		 
		 }
		 else if (!stricmp(actionCstr, "Session.ListenerSetPosition.1"))
		 {
		 
		 }
		 else if (!stricmp(actionCstr, "Session.SpeakerSetPosition.1"))
		 {
		 
		 }
		 else if (!stricmp(actionCstr, "Session.AudioSourceSetPosition.1"))
		 {
		 
		 }
		 else if (!stricmp(actionCstr, "Session.GetChannelParticipants.1"))
		 {
		 
		 }
		 else if (!stricmp(actionCstr, "Account.ChannelCreate.1"))
		 {
		 
		 }
		 else if (!stricmp(actionCstr, "Account.ChannelUpdate.1"))
		 {
		 
		 }
		 else if (!stricmp(actionCstr, "Account.ChannelDelete.1"))
		 {
		 
		 }
		 else if (!stricmp(actionCstr, "Account.ChannelCreateAndInvite.1"))
		 {
		 
		 }
		 else if (!stricmp(actionCstr, "Account.ChannelFolderCreate.1"))
		 {
		 
		 }
		 else if (!stricmp(actionCstr, "Account.ChannelFolderUpdate.1"))
		 {
		 
		 }
		 else if (!stricmp(actionCstr, "Account.ChannelFolderDelete.1"))
		 {
		 
		 }
		 else if (!stricmp(actionCstr, "Account.ChannelAddModerator.1"))
		 {
		 
		 }
		 else if (!stricmp(actionCstr, "Account.ChannelDeleteModerator.1"))
		 {
		 
		 }
		 */
	}
}

LLVivoxSecurity::LLVivoxSecurity()
{
    // This size is an arbitrary choice; Vivox does not care
    // Use a multiple of three so that there is no '=' padding in the base64 (purely an esthetic choice)
    #define VIVOX_TOKEN_BYTES 9
    U8  random_value[VIVOX_TOKEN_BYTES];

    for (int b = 0; b < VIVOX_TOKEN_BYTES; b++)
    {
        random_value[b] = ll_rand() & 0xff;
    }
    mConnectorHandle = LLBase64::encode(random_value, VIVOX_TOKEN_BYTES);
    
    for (int b = 0; b < VIVOX_TOKEN_BYTES; b++)
    {
        random_value[b] = ll_rand() & 0xff;
    }
    mAccountHandle = LLBase64::encode(random_value, VIVOX_TOKEN_BYTES);
}

LLVivoxSecurity::~LLVivoxSecurity()
{
}
