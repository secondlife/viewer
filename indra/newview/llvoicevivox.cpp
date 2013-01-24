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
#include "llvoicevivox.h"

#include "llsdutil.h"

// Linden library includes
#include "llavatarnamecache.h"
#include "llvoavatarself.h"
#include "llbufferstream.h"
#include "llfile.h"
#ifdef LL_STANDALONE
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

#include "llviewernetwork.h"
#include "llnotificationsutil.h"

#include "stringize.h"

// for base64 decoding
#include "apr_base64.h"

#define USE_SESSION_GROUPS 0

const F32 VOLUME_SCALE_VIVOX = 0.01f;

const F32 SPEAKING_TIMEOUT = 1.f;

static const std::string VOICE_SERVER_TYPE = "Vivox";

// Don't retry connecting to the daemon more frequently than this:
const F32 CONNECT_THROTTLE_SECONDS = 1.0f;

// Don't send positional updates more frequently than this:
const F32 UPDATE_THROTTLE_SECONDS = 0.1f;

const F32 LOGIN_RETRY_SECONDS = 10.0f;
const int MAX_LOGIN_RETRIES = 12;

// Defines the maximum number of times(in a row) "stateJoiningSession" case for spatial channel is reached in stateMachine()
// which is treated as normal. If this number is exceeded we suspect there is a problem with connection
// to voice server (EXT-4313). When voice works correctly, there is from 1 to 15 times. 50 was chosen 
// to make sure we don't make mistake when slight connection problems happen- situation when connection to server is 
// blocked is VERY rare and it's better to sacrifice response time in this situation for the sake of stability.
const int MAX_NORMAL_JOINING_SPATIAL_NUM = 50;

// How often to check for expired voice fonts in seconds
const F32 VOICE_FONT_EXPIRY_INTERVAL = 10.f;
// Time of day at which Vivox expires voice font subscriptions.
// Used to replace the time portion of received expiry timestamps.
static const std::string VOICE_FONT_EXPIRY_TIME = "T05:00:00Z";

// Maximum length of capture buffer recordings in seconds.
const F32 CAPTURE_BUFFER_MAX_TIME = 10.f;


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

class LLVivoxVoiceAccountProvisionResponder :
	public LLHTTPClient::Responder
{
public:
	LLVivoxVoiceAccountProvisionResponder(int retries)
	{
		mRetries = retries;
	}

	virtual void error(U32 status, const std::string& reason)
	{
		if ( mRetries > 0 )
		{
			LL_WARNS("Voice") << "ProvisionVoiceAccountRequest returned an error, retrying.  status = " << status << ", reason = \"" << reason << "\"" << LL_ENDL;
			LLVivoxVoiceClient::getInstance()->requestVoiceAccountProvision(
				mRetries - 1);
		}
		else
		{
			LL_WARNS("Voice") << "ProvisionVoiceAccountRequest returned an error, too many retries (giving up).  status = " << status << ", reason = \"" << reason << "\"" << LL_ENDL;
			LLVivoxVoiceClient::getInstance()->giveUp();
		}
	}

	virtual void result(const LLSD& content)
	{

		std::string voice_sip_uri_hostname;
		std::string voice_account_server_uri;
		
		LL_DEBUGS("Voice") << "ProvisionVoiceAccountRequest response:" << ll_pretty_print_sd(content) << LL_ENDL;
		
		if(content.has("voice_sip_uri_hostname"))
			voice_sip_uri_hostname = content["voice_sip_uri_hostname"].asString();
		
		// this key is actually misnamed -- it will be an entire URI, not just a hostname.
		if(content.has("voice_account_server_name"))
			voice_account_server_uri = content["voice_account_server_name"].asString();
		
		LLVivoxVoiceClient::getInstance()->login(
			content["username"].asString(),
			content["password"].asString(),
			voice_sip_uri_hostname,
			voice_account_server_uri);

	}

private:
	int mRetries;
};



///////////////////////////////////////////////////////////////////////////////////////////////

class LLVivoxVoiceClientMuteListObserver : public LLMuteListObserver
{
	/* virtual */ void onChange()  { LLVivoxVoiceClient::getInstance()->muteListChanged();}
};

class LLVivoxVoiceClientFriendsObserver : public LLFriendObserver
{
public:
	/* virtual */ void changed(U32 mask) { LLVivoxVoiceClient::getInstance()->updateFriends(mask);}
};

static LLVivoxVoiceClientMuteListObserver mutelist_listener;
static bool sMuteListListener_listening = false;

static LLVivoxVoiceClientFriendsObserver *friendslist_listener = NULL;

///////////////////////////////////////////////////////////////////////////////////////////////

class LLVivoxVoiceClientCapResponder : public LLHTTPClient::Responder
{
public:
	LLVivoxVoiceClientCapResponder(LLVivoxVoiceClient::state requesting_state) : mRequestingState(requesting_state) {};

	virtual void error(U32 status, const std::string& reason);	// called with bad status codes
	virtual void result(const LLSD& content);

private:
	LLVivoxVoiceClient::state mRequestingState;  // state 
};

void LLVivoxVoiceClientCapResponder::error(U32 status, const std::string& reason)
{
	LL_WARNS("Voice") << "LLVivoxVoiceClientCapResponder::error("
		<< status << ": " << reason << ")"
		<< LL_ENDL;
	LLVivoxVoiceClient::getInstance()->sessionTerminate();
}

void LLVivoxVoiceClientCapResponder::result(const LLSD& content)
{
	LLSD::map_const_iterator iter;
	
	LL_DEBUGS("Voice") << "ParcelVoiceInfoRequest response:" << ll_pretty_print_sd(content) << LL_ENDL;

	std::string uri;
	std::string credentials;
	
	if ( content.has("voice_credentials") )
	{
		LLSD voice_credentials = content["voice_credentials"];
		if ( voice_credentials.has("channel_uri") )
		{
			uri = voice_credentials["channel_uri"].asString();
		}
		if ( voice_credentials.has("channel_credentials") )
		{
			credentials =
				voice_credentials["channel_credentials"].asString();
		}
	}
	
	// set the spatial channel.  If no voice credentials or uri are 
	// available, then we simply drop out of voice spatially.
	if(LLVivoxVoiceClient::getInstance()->parcelVoiceInfoReceived(mRequestingState))
	{
		LLVivoxVoiceClient::getInstance()->setSpatialChannel(uri, credentials);
	}
}

static LLProcessPtr sGatewayPtr;

static bool isGatewayRunning()
{
	return sGatewayPtr && sGatewayPtr->isRunning();
}

static void killGateway()
{
	if (sGatewayPtr)
	{
		sGatewayPtr->kill();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////

LLVivoxVoiceClient::LLVivoxVoiceClient() :
	mState(stateDisabled),
	mSessionTerminateRequested(false),
	mRelogRequested(false),
	mConnected(false),
	mPump(NULL),
	mSpatialJoiningNum(0),

	mTuningMode(false),
	mTuningEnergy(0.0f),
	mTuningMicVolume(0),
	mTuningMicVolumeDirty(true),
	mTuningSpeakerVolume(0),
	mTuningSpeakerVolumeDirty(true),
	mTuningExitState(stateDisabled),

	mAreaVoiceDisabled(false),
	mAudioSession(NULL),
	mAudioSessionChanged(false),
	mNextAudioSession(NULL),

	mCurrentParcelLocalID(0),
	mNumberOfAliases(0),
	mCommandCookie(0),
	mLoginRetryCount(0),

	mBuddyListMapPopulated(false),
	mBlockRulesListReceived(false),
	mAutoAcceptRulesListReceived(false),

	mCaptureDeviceDirty(false),
	mRenderDeviceDirty(false),
	mSpatialCoordsDirty(false),

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
	mPlayRequestCount(0)
{	
	mSpeakerVolume = scale_speaker_volume(0);

	mVoiceVersion.serverVersion = "";
	mVoiceVersion.serverType = VOICE_SERVER_TYPE;
	
	//  gMuteListp isn't set up at this point, so we defer this until later.
//	gMuteListp->addObserver(&mutelist_listener);
	
	
#if LL_DARWIN || LL_LINUX || LL_SOLARIS
		// HACK: THIS DOES NOT BELONG HERE
		// When the vivox daemon dies, the next write attempt on our socket generates a SIGPIPE, which kills us.
		// This should cause us to ignore SIGPIPE and handle the error through proper channels.
		// This should really be set up elsewhere.  Where should it go?
		signal(SIGPIPE, SIG_IGN);
		
		// Since we're now launching the gateway with fork/exec instead of system(), we need to deal with zombie processes.
		// Ignoring SIGCHLD should prevent zombies from being created.  Alternately, we could use wait(), but I'd rather not do that.
		signal(SIGCHLD, SIG_IGN);
#endif

	// set up state machine
	setState(stateDisabled);
	
	gIdleCallbacks.addFunction(idle, this);
}

//---------------------------------------------------

LLVivoxVoiceClient::~LLVivoxVoiceClient()
{
}

//---------------------------------------------------

void LLVivoxVoiceClient::init(LLPumpIO *pump)
{
	// constructor will set up LLVoiceClient::getInstance()
	LLVivoxVoiceClient::getInstance()->mPump = pump;
}

void LLVivoxVoiceClient::terminate()
{
	if(mConnected)
	{
		logout();
		connectorShutdown();
		closeSocket();		// Need to do this now -- bad things happen if the destructor does it later.
		cleanUp();
	}
	else
	{
		killGateway();
	}
}

//---------------------------------------------------

void LLVivoxVoiceClient::cleanUp()
{
	deleteAllSessions();
	deleteAllBuddies();
	deleteAllVoiceFonts();
	deleteVoiceFontTemplates();
}

//---------------------------------------------------

const LLVoiceVersionInfo& LLVivoxVoiceClient::getVersion()
{
	return mVoiceVersion;
}

//---------------------------------------------------

void LLVivoxVoiceClient::updateSettings()
{
	setVoiceEnabled(gSavedSettings.getBOOL("EnableVoiceChat"));
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
	if(mConnected)
	{
		apr_status_t err;
		apr_size_t size = (apr_size_t)str.size();
		apr_size_t written = size;
	
		//MARK: Turn this on to log outgoing XML
//		LL_DEBUGS("Voice") << "sending: " << str << LL_ENDL;

		// check return code - sockets will fail (broken, etc.)
		err = apr_socket_send(
				mSocket->getSocket(),
				(const char*)str.data(),
				&written);
		
		if(err == 0)
		{
			// Success.
			result = true;
		}
		// TODO: handle partial writes (written is number of bytes written)
		// Need to set socket to non-blocking before this will work.
//		else if(APR_STATUS_IS_EAGAIN(err))
//		{
//			// 
//		}
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
	std::string logpath = gDirUtilp->getExpandedFilename(LL_PATH_LOGS, "");
	std::string loglevel = "0";
	
	// Transition to stateConnectorStarted when the connector handle comes back.
	setState(stateConnectorStarting);

	std::string savedLogLevel = gSavedSettings.getString("VivoxDebugLevel");
		
	if(savedLogLevel != "-1")
	{
		LL_DEBUGS("Voice") << "creating connector with logging enabled" << LL_ENDL;
		loglevel = "10";
	}
	
	stream 
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.Create.1\">"
		<< "<ClientName>V2 SDK</ClientName>"
		<< "<AccountManagementServer>" << mVoiceAccountServerURI << "</AccountManagementServer>"
		<< "<Mode>Normal</Mode>"
		<< "<Logging>"
			<< "<Folder>" << logpath << "</Folder>"
			<< "<FileNamePrefix>Connector</FileNamePrefix>"
			<< "<FileNameSuffix>.log</FileNameSuffix>"
			<< "<LogLevel>" << loglevel << "</LogLevel>"
		<< "</Logging>"
		<< "<Application>SecondLifeViewer.1</Application>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVivoxVoiceClient::connectorShutdown()
{
	setState(stateConnectorStopping);
	
	if(!mConnectorHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.InitiateShutdown.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
		<< "</Request>"
		<< "\n\n\n";
		
		mConnectorHandle.clear();
		
		writeString(stream.str());
	}
}

void LLVivoxVoiceClient::userAuthorized(const std::string& user_id, const LLUUID &agentID)
{

	mAccountDisplayName = user_id;

	LL_INFOS("Voice") << "name \"" << mAccountDisplayName << "\" , ID " << agentID << LL_ENDL;

	mAccountName = nameFromID(agentID);
}

void LLVivoxVoiceClient::requestVoiceAccountProvision(S32 retries)
{
	LLViewerRegion *region = gAgent.getRegion();
	
	if ( region && mVoiceEnabled )
	{
		std::string url = 
		region->getCapability("ProvisionVoiceAccountRequest");
		
		if ( url.empty() ) 
		{
			// we've not received the capability yet, so return.
			// the password will remain empty, so we'll remain in
			// stateIdle
			return;
		}
		
		LLHTTPClient::post(
						   url,
						   LLSD(),
						   new LLVivoxVoiceAccountProvisionResponder(retries));
		
		setState(stateConnectorStart);		
	}
}

void LLVivoxVoiceClient::login(
	const std::string& account_name,
	const std::string& password,
	const std::string& voice_sip_uri_hostname,
	const std::string& voice_account_server_uri)
{
	mVoiceSIPURIHostName = voice_sip_uri_hostname;
	mVoiceAccountServerURI = voice_account_server_uri;

	if(!mAccountHandle.empty())
	{
		// Already logged in.
		LL_WARNS("Voice") << "Called while already logged in." << LL_ENDL;
		
		// Don't process another login.
		return;
	}
	else if ( account_name != mAccountName )
	{
		//TODO: error?
		LL_WARNS("Voice") << "Wrong account name! " << account_name
				<< " instead of " << mAccountName << LL_ENDL;
	}
	else
	{
		mAccountPassword = password;
	}

	std::string debugSIPURIHostName = gSavedSettings.getString("VivoxDebugSIPURIHostName");
	
	if( !debugSIPURIHostName.empty() )
	{
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
	}
	
	std::string debugAccountServerURI = gSavedSettings.getString("VivoxDebugVoiceAccountServerURI");

	if( !debugAccountServerURI.empty() )
	{
		mVoiceAccountServerURI = debugAccountServerURI;
	}
	
	if( mVoiceAccountServerURI.empty() )
	{
		// If the account server URI isn't specified, construct it from the SIP URI hostname
		mVoiceAccountServerURI = "https://www." + mVoiceSIPURIHostName + "/api2/";		
	}
}

void LLVivoxVoiceClient::idle(void* user_data)
{
	LLVivoxVoiceClient* self = (LLVivoxVoiceClient*)user_data;
	self->stateMachine();
}

std::string LLVivoxVoiceClient::state2string(LLVivoxVoiceClient::state inState)
{
	std::string result = "UNKNOWN";
	
		// Prevent copy-paste errors when updating this list...
#define CASE(x)  case x:  result = #x;  break

	switch(inState)
	{
		CASE(stateDisableCleanup);
		CASE(stateDisabled);
		CASE(stateStart);
		CASE(stateDaemonLaunched);
		CASE(stateConnecting);
		CASE(stateConnected);
		CASE(stateIdle);
		CASE(stateMicTuningStart);
		CASE(stateMicTuningRunning);
		CASE(stateMicTuningStop);
		CASE(stateCaptureBufferPaused);
		CASE(stateCaptureBufferRecStart);
		CASE(stateCaptureBufferRecording);
		CASE(stateCaptureBufferPlayStart);
		CASE(stateCaptureBufferPlaying);
		CASE(stateConnectorStart);
		CASE(stateConnectorStarting);
		CASE(stateConnectorStarted);
		CASE(stateLoginRetry);
		CASE(stateLoginRetryWait);
		CASE(stateNeedsLogin);
		CASE(stateLoggingIn);
		CASE(stateLoggedIn);
		CASE(stateVoiceFontsWait);
		CASE(stateVoiceFontsReceived);
		CASE(stateCreatingSessionGroup);
		CASE(stateNoChannel);		
		CASE(stateRetrievingParcelVoiceInfo);
		CASE(stateJoiningSession);
		CASE(stateSessionJoined);
		CASE(stateRunning);
		CASE(stateLeavingSession);
		CASE(stateSessionTerminated);
		CASE(stateLoggingOut);
		CASE(stateLoggedOut);
		CASE(stateConnectorStopping);
		CASE(stateConnectorStopped);
		CASE(stateConnectorFailed);
		CASE(stateConnectorFailedWaiting);
		CASE(stateLoginFailed);
		CASE(stateLoginFailedWaiting);
		CASE(stateJoinSessionFailed);
		CASE(stateJoinSessionFailedWaiting);
		CASE(stateJail);
	}

#undef CASE
	
	return result;
}



void LLVivoxVoiceClient::setState(state inState)
{
	LL_DEBUGS("Voice") << "entering state " << state2string(inState) << LL_ENDL;
	
	mState = inState;
}

void LLVivoxVoiceClient::stateMachine()
{
	if(gDisconnected)
	{
		// The viewer has been disconnected from the sim.  Disable voice.
		setVoiceEnabled(false);
	}
	
	if(mVoiceEnabled)
	{
		updatePosition();
	}
	else if(mTuningMode)
	{
		// Tuning mode is special -- it needs to launch SLVoice even if voice is disabled.
	}
	else
	{
		if((getState() != stateDisabled) && (getState() != stateDisableCleanup))
		{
			// User turned off voice support.  Send the cleanup messages, close the socket, and reset.
			if(!mConnected)
			{
				// if voice was turned off after the daemon was launched but before we could connect to it, we may need to issue a kill.
				LL_INFOS("Voice") << "Disabling voice before connection to daemon, terminating." << LL_ENDL;
				killGateway();
			}
			
			logout();
			connectorShutdown();
			
			setState(stateDisableCleanup);
		}
	}
	

	switch(getState())
	{
		//MARK: stateDisableCleanup
		case stateDisableCleanup:
			// Clean up and reset everything.
			closeSocket();
			cleanUp();

			mAccountHandle.clear();
			mAccountPassword.clear();
			mVoiceAccountServerURI.clear();
			
			setState(stateDisabled);	
		break;
		
		//MARK: stateDisabled
		case stateDisabled:
			if(mTuningMode || (mVoiceEnabled && !mAccountName.empty()))
			{
				setState(stateStart);
			}
		break;
		
		//MARK: stateStart
		case stateStart:
			if(gSavedSettings.getBOOL("CmdLineDisableVoice"))
			{
				// Voice is locked out, we must not launch the vivox daemon.
				setState(stateJail);
			}
			else if(!isGatewayRunning())
			{
				if (true)           // production build, not test
				{
					// Launch the voice daemon
					
					// *FIX:Mani - Using the executable dir instead 
					// of mAppRODataDir, the working directory from which the app
					// is launched.
					//std::string exe_path = gDirUtilp->getAppRODataDir();
					std::string exe_path = gDirUtilp->getExecutableDir();
					exe_path += gDirUtilp->getDirDelimiter();
#if LL_WINDOWS
					exe_path += "SLVoice.exe";
#elif LL_DARWIN
					exe_path += "../Resources/SLVoice";
#else
					exe_path += "SLVoice";
#endif
					// See if the vivox executable exists
					llstat s;
					if (!LLFile::stat(exe_path, &s))
					{
						// vivox executable exists.  Build the command line and launch the daemon.
						LLProcess::Params params;
						params.executable = exe_path;
						// SLIM SDK: these arguments are no longer necessary.
//						std::string args = " -p tcp -h -c";
						std::string loglevel = gSavedSettings.getString("VivoxDebugLevel");
						if(loglevel.empty())
						{
							loglevel = "-1";	// turn logging off completely
						}

						params.args.add("-ll");
						params.args.add(loglevel);
						params.cwd = gDirUtilp->getAppRODataDir();
						sGatewayPtr = LLProcess::create(params);

						mDaemonHost = LLHost(gSavedSettings.getString("VivoxVoiceHost").c_str(), gSavedSettings.getU32("VivoxVoicePort"));
					}
					else
					{
						LL_INFOS("Voice") << exe_path << " not found." << LL_ENDL;
					}
				}
				else
				{
					// SLIM SDK: port changed from 44124 to 44125.
					// We can connect to a client gateway running on another host.  This is useful for testing.
					// To do this, launch the gateway on a nearby host like this:
					//  vivox-gw.exe -p tcp -i 0.0.0.0:44125
					// and put that host's IP address here.
					mDaemonHost = LLHost(gSavedSettings.getString("VivoxVoiceHost"), gSavedSettings.getU32("VivoxVoicePort"));
				}

				mUpdateTimer.start();
				mUpdateTimer.setTimerExpirySec(CONNECT_THROTTLE_SECONDS);

				setState(stateDaemonLaunched);
				
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
		break;

		//MARK: stateDaemonLaunched
		case stateDaemonLaunched:
			if(mUpdateTimer.hasExpired())
			{
				LL_DEBUGS("Voice") << "Connecting to vivox daemon:" << mDaemonHost << LL_ENDL;

				mUpdateTimer.setTimerExpirySec(CONNECT_THROTTLE_SECONDS);

				if(!mSocket)
				{
					mSocket = LLSocket::create(gAPRPoolp, LLSocket::STREAM_TCP);	
				}
				
				mConnected = mSocket->blockingConnect(mDaemonHost);
				if(mConnected)
				{
					setState(stateConnecting);
				}
				else
				{
					// If the connect failed, the socket may have been put into a bad state.  Delete it.
					closeSocket();
				}
			}
		break;

		//MARK: stateConnecting
		case stateConnecting:
		// Can't do this until we have the pump available.
		if(mPump)
		{
			// MBW -- Note to self: pumps and pipes examples in
			//  indra/test/io.cpp
			//  indra/test/llpipeutil.{cpp|h}

			// Attach the pumps and pipes
				
			LLPumpIO::chain_t readChain;

			readChain.push_back(LLIOPipe::ptr_t(new LLIOSocketReader(mSocket)));
			readChain.push_back(LLIOPipe::ptr_t(new LLVivoxProtocolParser()));

			mPump->addChain(readChain, NEVER_CHAIN_EXPIRY_SECS);

			setState(stateConnected);
		}

		break;
		
		//MARK: stateConnected
		case stateConnected:
			// Initial devices query
			getCaptureDevicesSendMessage();
			getRenderDevicesSendMessage();

			mLoginRetryCount = 0;

			setState(stateIdle);
		break;

		//MARK: stateIdle
		case stateIdle:
			// This is the idle state where we're connected to the daemon but haven't set up a connector yet.
			if(mTuningMode)
			{
				mTuningExitState = stateIdle;
				setState(stateMicTuningStart);
			}
			else if(!mVoiceEnabled)
			{
				// We never started up the connector.  This will shut down the daemon.
				setState(stateConnectorStopped);
			}
			else if(!mAccountName.empty())
			{
				if ( mAccountPassword.empty() )
				{
					requestVoiceAccountProvision();
				}
			}
		break;

		//MARK: stateMicTuningStart
		case stateMicTuningStart:
			if(mUpdateTimer.hasExpired())
			{
				if(mCaptureDeviceDirty || mRenderDeviceDirty)
				{
					// These can't be changed while in tuning mode.  Set them before starting.
					std::ostringstream stream;
					
					buildSetCaptureDevice(stream);
					buildSetRenderDevice(stream);

					if(!stream.str().empty())
					{
						writeString(stream.str());
					}				

					// This will come around again in the same state and start the capture, after the timer expires.
					mUpdateTimer.start();
					mUpdateTimer.setTimerExpirySec(UPDATE_THROTTLE_SECONDS);
				}
				else
				{
					// duration parameter is currently unused, per Mike S.
					tuningCaptureStartSendMessage(10000);

					setState(stateMicTuningRunning);
				}
			}
			
		break;
		
		//MARK: stateMicTuningRunning
		case stateMicTuningRunning:
			if(!mTuningMode || mCaptureDeviceDirty || mRenderDeviceDirty)
			{
				// All of these conditions make us leave tuning mode.
				setState(stateMicTuningStop);
			}
			else
			{
				// process mic/speaker volume changes
				if(mTuningMicVolumeDirty || mTuningSpeakerVolumeDirty)
				{
					std::ostringstream stream;
					
					if(mTuningMicVolumeDirty)
					{
						LL_INFOS("Voice") << "setting tuning mic level to " << mTuningMicVolume << LL_ENDL;
						stream
						<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.SetMicLevel.1\">"
						<< "<Level>" << mTuningMicVolume << "</Level>"
						<< "</Request>\n\n\n";
					}
					
					if(mTuningSpeakerVolumeDirty)
					{
						stream
						<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.SetSpeakerLevel.1\">"
						<< "<Level>" << mTuningSpeakerVolume << "</Level>"
						<< "</Request>\n\n\n";
					}
					
					mTuningMicVolumeDirty = false;
					mTuningSpeakerVolumeDirty = false;

					if(!stream.str().empty())
					{
						writeString(stream.str());
					}
				}
			}
		break;
		
		//MARK: stateMicTuningStop
		case stateMicTuningStop:
		{
			// transition out of mic tuning
			tuningCaptureStopSendMessage();
			
			setState(mTuningExitState);
			
			// if we exited just to change devices, this will keep us from re-entering too fast.
			mUpdateTimer.start();
			mUpdateTimer.setTimerExpirySec(UPDATE_THROTTLE_SECONDS);
			
		}
		break;

		//MARK: stateCaptureBufferPaused
		case stateCaptureBufferPaused:
			if (!mCaptureBufferMode)
			{
				// Leaving capture mode.

				mCaptureBufferRecording = false;
				mCaptureBufferRecorded = false;
				mCaptureBufferPlaying = false;

				// Return to stateNoChannel to trigger reconnection to a channel.
				setState(stateNoChannel);
			}
			else if (mCaptureBufferRecording)
			{
				setState(stateCaptureBufferRecStart);
			}
			else if (mCaptureBufferPlaying)
			{
				setState(stateCaptureBufferPlayStart);
			}
		break;

		//MARK: stateCaptureBufferRecStart
		case stateCaptureBufferRecStart:
			captureBufferRecordStartSendMessage();

			// Flag that something is recorded to allow playback.
			mCaptureBufferRecorded = true;

			// Start the timer, recording will be stopped when it expires.
			mCaptureTimer.start();
			mCaptureTimer.setTimerExpirySec(CAPTURE_BUFFER_MAX_TIME);

			// Update UI, should really use a separate callback.
			notifyVoiceFontObservers();

			setState(stateCaptureBufferRecording);
		break;

		//MARK: stateCaptureBufferRecording
		case stateCaptureBufferRecording:
			if (!mCaptureBufferMode || !mCaptureBufferRecording ||
				mCaptureBufferPlaying || mCaptureTimer.hasExpired())
			{
				// Stop recording
				captureBufferRecordStopSendMessage();
				mCaptureBufferRecording = false;

				// Update UI, should really use a separate callback.
				notifyVoiceFontObservers();

				setState(stateCaptureBufferPaused);
			}
		break;

		//MARK: stateCaptureBufferPlayStart
		case stateCaptureBufferPlayStart:
			captureBufferPlayStartSendMessage(mPreviewVoiceFont);

			// Store the voice font being previewed, so that we know to restart if it changes.
			mPreviewVoiceFontLast = mPreviewVoiceFont;

			// Update UI, should really use a separate callback.
			notifyVoiceFontObservers();

			setState(stateCaptureBufferPlaying);
		break;

		//MARK: stateCaptureBufferPlaying
		case stateCaptureBufferPlaying:
			if (mCaptureBufferPlaying && mPreviewVoiceFont != mPreviewVoiceFontLast)
			{
				// If the preview voice font changes, restart playing with the new font.
				setState(stateCaptureBufferPlayStart);
			}
			else if (!mCaptureBufferMode || !mCaptureBufferPlaying || mCaptureBufferRecording)
			{
				// Stop playing.
				captureBufferPlayStopSendMessage();
				mCaptureBufferPlaying = false;

				// Update UI, should really use a separate callback.
				notifyVoiceFontObservers();

				setState(stateCaptureBufferPaused);
			}
		break;

			//MARK: stateConnectorStart
		case stateConnectorStart:
			if(!mVoiceEnabled)
			{
				// We were never logged in.  This will shut down the connector.
				setState(stateLoggedOut);
			}
			else if(!mVoiceAccountServerURI.empty())
			{
				connectorCreate();
			}
		break;
		
		//MARK: stateConnectorStarting
		case stateConnectorStarting:	// waiting for connector handle
			// connectorCreateResponse() will transition from here to stateConnectorStarted.
		break;
		
		//MARK: stateConnectorStarted
		case stateConnectorStarted:		// connector handle received
			if(!mVoiceEnabled)
			{
				// We were never logged in.  This will shut down the connector.
				setState(stateLoggedOut);
			}
			else
			{
				// The connector is started.  Send a login message.
				setState(stateNeedsLogin);
			}
		break;
				
		//MARK: stateLoginRetry
		case stateLoginRetry:
			if(mLoginRetryCount == 0)
			{
				// First retry -- display a message to the user
				notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LOGIN_RETRY);
			}
			
			mLoginRetryCount++;
			
			if(mLoginRetryCount > MAX_LOGIN_RETRIES)
			{
				LL_WARNS("Voice") << "too many login retries, giving up." << LL_ENDL;
				setState(stateLoginFailed);
				LLSD args;
				std::stringstream errs;
				errs << mVoiceAccountServerURI << "\n:UDP: 3478, 3479, 5060, 5062, 12000-17000";
				args["HOSTID"] = errs.str();
				if (LLGridManager::getInstance()->isSystemGrid())
				{
					LLNotificationsUtil::add("NoVoiceConnect", args);	
				}
				else
				{
					LLNotificationsUtil::add("NoVoiceConnect-GIAB", args);	
				}				
			}
			else
			{
				LL_INFOS("Voice") << "will retry login in " << LOGIN_RETRY_SECONDS << " seconds." << LL_ENDL;
				mUpdateTimer.start();
				mUpdateTimer.setTimerExpirySec(LOGIN_RETRY_SECONDS);
				setState(stateLoginRetryWait);
			}
		break;
		
		//MARK: stateLoginRetryWait
		case stateLoginRetryWait:
			if(mUpdateTimer.hasExpired())
			{
				setState(stateNeedsLogin);
			}
		break;
		
		//MARK: stateNeedsLogin
		case stateNeedsLogin:
			if(!mAccountPassword.empty())
			{
				setState(stateLoggingIn);
				loginSendMessage();
			}		
		break;
		
		//MARK: stateLoggingIn
		case stateLoggingIn:			// waiting for account handle
			// loginResponse() will transition from here to stateLoggedIn.
		break;
		
		//MARK: stateLoggedIn
		case stateLoggedIn:				// account handle received

			notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LOGGED_IN);

			if (LLVoiceClient::instance().getVoiceEffectEnabled())
			{
				// Request the set of available voice fonts.
				setState(stateVoiceFontsWait);
				refreshVoiceEffectLists(true);
			}
			else
			{
				// If voice effects are disabled, pretend we've received them and carry on.
				setState(stateVoiceFontsReceived);
			}

			// request the current set of block rules (we'll need them when updating the friends list)
			accountListBlockRulesSendMessage();
			
			// request the current set of auto-accept rules
			accountListAutoAcceptRulesSendMessage();
			
			// Set up the mute list observer if it hasn't been set up already.
			if((!sMuteListListener_listening))
			{
				LLMuteList::getInstance()->addObserver(&mutelist_listener);
				sMuteListListener_listening = true;
			}

			// Set up the friends list observer if it hasn't been set up already.
			if(friendslist_listener == NULL)
			{
				friendslist_listener = new LLVivoxVoiceClientFriendsObserver;
				LLAvatarTracker::instance().addObserver(friendslist_listener);
			}
			
			// Set the initial state of mic mute, local speaker volume, etc.
			{
				std::ostringstream stream;
				
				buildLocalAudioUpdates(stream);
				
				if(!stream.str().empty())
				{
					writeString(stream.str());
				}
			}
		break;

		//MARK: stateVoiceFontsWait
		case stateVoiceFontsWait:		// Await voice font list
			// accountGetSessionFontsResponse() will transition from here to
			// stateVoiceFontsReceived, to ensure we have the voice font list
			// before attempting to create a session.
		break;
			
		//MARK: stateVoiceFontsReceived
		case stateVoiceFontsReceived:	// Voice font list received
			// Set up the timer to check for expiring voice fonts
			mVoiceFontExpiryTimer.start();
			mVoiceFontExpiryTimer.setTimerExpirySec(VOICE_FONT_EXPIRY_INTERVAL);

#if USE_SESSION_GROUPS			
			// create the main session group
			setState(stateCreatingSessionGroup);
			sessionGroupCreateSendMessage();
#else
			setState(stateNoChannel);				
#endif
		break;
		
		//MARK: stateCreatingSessionGroup
		case stateCreatingSessionGroup:
			if(mSessionTerminateRequested || !mVoiceEnabled)
			{
				// *TODO: Question: is this the right way out of this state
				setState(stateSessionTerminated);
			}
			else if(!mMainSessionGroupHandle.empty())
			{
				// Start looped recording (needed for "panic button" anti-griefing tool)
				recordingLoopStart();
				setState(stateNoChannel);	
			}
		break;
			
		//MARK: stateRetrievingParcelVoiceInfo
		case stateRetrievingParcelVoiceInfo: 
			// wait until parcel voice info is received.
			if(mSessionTerminateRequested || !mVoiceEnabled)
			{
				// if a terminate request has been received,
				// bail and go to the stateSessionTerminated
				// state.  If the cap request is still pending,
				// the responder will check to see if we've moved
				// to a new session and won't change any state.
				setState(stateSessionTerminated);
			}
			break;
			
					
		//MARK: stateNoChannel
		case stateNoChannel:
			LL_DEBUGS("Voice") << "State No Channel" << LL_ENDL;
			mSpatialJoiningNum = 0;
			// Do this here as well as inside sendPositionalUpdate().  
			// Otherwise, if you log in but don't join a proximal channel (such as when your login location has voice disabled), your friends list won't sync.
			sendFriendsListUpdates();
			
			if(mSessionTerminateRequested || !mVoiceEnabled)
			{
				// TODO: Question: Is this the right way out of this state?
				setState(stateSessionTerminated);
			}
			else if(mTuningMode)
			{
				mTuningExitState = stateNoChannel;
				setState(stateMicTuningStart);
			}
			else if(mCaptureBufferMode)
			{
				setState(stateCaptureBufferPaused);
			}
			else if(checkParcelChanged() || (mNextAudioSession == NULL))
			{
				// the parcel is changed, or we have no pending audio sessions,
				// so try to request the parcel voice info
				// if we have the cap, we move to the appropriate state
				if(requestParcelVoiceInfo())
				{
					setState(stateRetrievingParcelVoiceInfo);
				}
			}
			else if(sessionNeedsRelog(mNextAudioSession))
			{
				requestRelog();
				setState(stateSessionTerminated);
			}
			else if(mNextAudioSession)
			{				
				sessionState *oldSession = mAudioSession;

				mAudioSession = mNextAudioSession;
				mAudioSessionChanged = true;
				if(!mAudioSession->mReconnect)	
				{
					mNextAudioSession = NULL;
				}
				
				// The old session may now need to be deleted.
				reapSession(oldSession);
				
				if(!mAudioSession->mHandle.empty())
				{
					// Connect to a session by session handle

					sessionMediaConnectSendMessage(mAudioSession);
				}
				else
				{
					// Connect to a session by URI
					sessionCreateSendMessage(mAudioSession, true, false);
				}

				notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINING);
				setState(stateJoiningSession);
			}
		break;
			
		//MARK: stateJoiningSession
		case stateJoiningSession:		// waiting for session handle
			
			// If this is true we have problem with connection to voice server (EXT-4313).
			// See descriptions of mSpatialJoiningNum and MAX_NORMAL_JOINING_SPATIAL_NUM.
			if(mSpatialJoiningNum == MAX_NORMAL_JOINING_SPATIAL_NUM) 
		    {
				// Notify observers to let them know there is problem with voice
				notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_VOICE_DISABLED);
				llwarns << "There seems to be problem with connection to voice server. Disabling voice chat abilities." << llendl;
		    }
			
			// Increase mSpatialJoiningNum only for spatial sessions- it's normal to reach this case for
			// example for p2p many times while waiting for response, so it can't be used to detect errors
			if(mAudioSession && mAudioSession->mIsSpatial)
		    {
				
				mSpatialJoiningNum++;
		    }
			
			// joinedAudioSession() will transition from here to stateSessionJoined.
			if(!mVoiceEnabled)
			{
				// User bailed out during connect -- jump straight to teardown.
				setState(stateSessionTerminated);
			}
			else if(mSessionTerminateRequested)
			{
				if(mAudioSession && !mAudioSession->mHandle.empty())
				{
					// Only allow direct exits from this state in p2p calls (for cancelling an invite).
					// Terminating a half-connected session on other types of calls seems to break something in the vivox gateway.
					if(mAudioSession->mIsP2P)
					{
						sessionMediaDisconnectSendMessage(mAudioSession);
						setState(stateSessionTerminated);
					}
				}
			}
			break;
			
		//MARK: stateSessionJoined
		case stateSessionJoined:		// session handle received


			mSpatialJoiningNum = 0;
			// It appears that I need to wait for BOTH the SessionGroup.AddSession response and the SessionStateChangeEvent with state 4
			// before continuing from this state.  They can happen in either order, and if I don't wait for both, things can get stuck.
			// For now, the SessionGroup.AddSession response handler sets mSessionHandle and the SessionStateChangeEvent handler transitions to stateSessionJoined.
			// This is a cheap way to make sure both have happened before proceeding.
			if(mAudioSession && mAudioSession->mVoiceEnabled)
			{
				// Dirty state that may need to be sync'ed with the daemon.
				mMuteMicDirty = true;
				mSpeakerVolumeDirty = true;
				mSpatialCoordsDirty = true;
				
				setState(stateRunning);
				
				// Start the throttle timer
				mUpdateTimer.start();
				mUpdateTimer.setTimerExpirySec(UPDATE_THROTTLE_SECONDS);

				// Events that need to happen when a session is joined could go here.
				// Maybe send initial spatial data?
				notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_JOINED);

			}
			else if(!mVoiceEnabled)
			{
				// User bailed out during connect -- jump straight to teardown.
				setState(stateSessionTerminated);
			}
			else if(mSessionTerminateRequested)
			{
				// Only allow direct exits from this state in p2p calls (for cancelling an invite).
				// Terminating a half-connected session on other types of calls seems to break something in the vivox gateway.
				if(mAudioSession && mAudioSession->mIsP2P)
				{
					sessionMediaDisconnectSendMessage(mAudioSession);
					setState(stateSessionTerminated);
				}
			}	
		break;
		
		//MARK: stateRunning
		case stateRunning:				// steady state
			// Disabling voice or disconnect requested.
			if(!mVoiceEnabled || mSessionTerminateRequested)
			{
				leaveAudioSession();
			}
			else
			{
				
				if(!inSpatialChannel())
				{
					// When in a non-spatial channel, never send positional updates.
					mSpatialCoordsDirty = false;
				}
				else
				{
					if(checkParcelChanged())
					{
						// if the parcel has changed, attempted to request the
						// cap for the parcel voice info.  If we can't request it
						// then we don't have the cap URL so we do nothing and will
						// recheck next time around
						if(requestParcelVoiceInfo())
						{
							// we did get the cap, and we made the request,
							// so go wait for the response.
							setState(stateRetrievingParcelVoiceInfo);
						}
					}
					// Do the calculation that enforces the listener<->speaker tether (and also updates the real camera position)
					enforceTether();
					
				}
				
				// Do notifications for expiring Voice Fonts.
				if (mVoiceFontExpiryTimer.hasExpired())
				{
					expireVoiceFonts();
					mVoiceFontExpiryTimer.setTimerExpirySec(VOICE_FONT_EXPIRY_INTERVAL);
				}

				// Send an update only if the ptt or mute state has changed (which shouldn't be able to happen that often
				// -- the user can only click so fast) or every 10hz, whichever is sooner.
				// Sending for every volume update causes an excessive flood of messages whenever a volume slider is dragged.
				if((mAudioSession && mAudioSession->mMuteDirty) || mMuteMicDirty || mUpdateTimer.hasExpired())
				{
					mUpdateTimer.setTimerExpirySec(UPDATE_THROTTLE_SECONDS);
					sendPositionalUpdate();
				}
			}
		break;
		
		//MARK: stateLeavingSession
		case stateLeavingSession:		// waiting for terminate session response
			// The handler for the Session.Terminate response will transition from here to stateSessionTerminated.
		break;

		//MARK: stateSessionTerminated
		case stateSessionTerminated:
			
			// Must do this first, since it uses mAudioSession.
			notifyStatusObservers(LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL);
			
			if(mAudioSession)
			{
				sessionState *oldSession = mAudioSession;

				mAudioSession = NULL;
				// We just notified status observers about this change.  Don't do it again.
				mAudioSessionChanged = false;

				// The old session may now need to be deleted.
				reapSession(oldSession);
			}
			else
			{
				LL_WARNS("Voice") << "stateSessionTerminated with NULL mAudioSession" << LL_ENDL;
			}
	
			// Always reset the terminate request flag when we get here.
			mSessionTerminateRequested = false;

			if(mVoiceEnabled && !mRelogRequested)
			{				
				// Just leaving a channel, go back to stateNoChannel (the "logged in but have no channel" state).
				setState(stateNoChannel);
			}
			else
			{
				// Shutting down voice, continue with disconnecting.
				logout();
				
				// The state machine will take it from here
				mRelogRequested = false;
			}
			
		break;
		
		//MARK: stateLoggingOut
		case stateLoggingOut:			// waiting for logout response
			// The handler for the AccountLoginStateChangeEvent will transition from here to stateLoggedOut.
		break;
		
		//MARK: stateLoggedOut
		case stateLoggedOut:			// logout response received
			
			// Once we're logged out, these things are invalid.
			mAccountHandle.clear();
			cleanUp();

			if(mVoiceEnabled && !mRelogRequested)
			{
				// User was logged out, but wants to be logged in.  Send a new login request.
				setState(stateNeedsLogin);
			}
			else
			{
				// shut down the connector
				connectorShutdown();
			}
		break;
		
		//MARK: stateConnectorStopping
		case stateConnectorStopping:	// waiting for connector stop
			// The handler for the Connector.InitiateShutdown response will transition from here to stateConnectorStopped.
		break;

		//MARK: stateConnectorStopped
		case stateConnectorStopped:		// connector stop received
			setState(stateDisableCleanup);
		break;

		//MARK: stateConnectorFailed
		case stateConnectorFailed:
			setState(stateConnectorFailedWaiting);
		break;
		//MARK: stateConnectorFailedWaiting
		case stateConnectorFailedWaiting:
			if(!mVoiceEnabled)
			{
				setState(stateDisableCleanup);
			}
		break;

		//MARK: stateLoginFailed
		case stateLoginFailed:
			setState(stateLoginFailedWaiting);
		break;
		//MARK: stateLoginFailedWaiting
		case stateLoginFailedWaiting:
			if(!mVoiceEnabled)
			{
				setState(stateDisableCleanup);
			}
		break;

		//MARK: stateJoinSessionFailed
		case stateJoinSessionFailed:
			// Transition to error state.  Send out any notifications here.
			if(mAudioSession)
			{
				LL_WARNS("Voice") << "stateJoinSessionFailed: (" << mAudioSession->mErrorStatusCode << "): " << mAudioSession->mErrorStatusString << LL_ENDL;
			}
			else
			{
				LL_WARNS("Voice") << "stateJoinSessionFailed with no current session" << LL_ENDL;
			}
			
			notifyStatusObservers(LLVoiceClientStatusObserver::ERROR_UNKNOWN);
			setState(stateJoinSessionFailedWaiting);
		break;
		
		//MARK: stateJoinSessionFailedWaiting
		case stateJoinSessionFailedWaiting:
			// Joining a channel failed, either due to a failed channel name -> sip url lookup or an error from the join message.
			// Region crossings may leave this state and try the join again.
			if(mSessionTerminateRequested)
			{
				setState(stateSessionTerminated);
			}
		break;
		
		//MARK: stateJail
		case stateJail:
			// We have given up.  Do nothing.
		break;

	}
	
	if (mAudioSessionChanged)
	{
		mAudioSessionChanged = false;
		notifyParticipantObservers();
		notifyVoiceFontObservers();
	}
	else if (mAudioSession && mAudioSession->mParticipantsChanged)
	{
		mAudioSession->mParticipantsChanged = false;
		notifyParticipantObservers();
	}
}

void LLVivoxVoiceClient::closeSocket(void)
{
	mSocket.reset();
	mConnected = false;	
	mConnectorHandle.clear();
	mAccountHandle.clear();
}

void LLVivoxVoiceClient::loginSendMessage()
{
	std::ostringstream stream;

	bool autoPostCrashDumps = gSavedSettings.getBOOL("VivoxAutoPostCrashDumps");

	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.Login.1\">"
		<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
		<< "<AccountName>" << mAccountName << "</AccountName>"
		<< "<AccountPassword>" << mAccountPassword << "</AccountPassword>"
		<< "<AudioSessionAnswerMode>VerifyAnswer</AudioSessionAnswerMode>"
		<< "<EnableBuddiesAndPresence>true</EnableBuddiesAndPresence>"
		<< "<BuddyManagementMode>Application</BuddyManagementMode>"
		<< "<ParticipantPropertyFrequency>5</ParticipantPropertyFrequency>"
		<< (autoPostCrashDumps?"<AutopostCrashDumps>true</AutopostCrashDumps>":"")
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVivoxVoiceClient::logout()
{
	// Ensure that we'll re-request provisioning before logging in again
	mAccountPassword.clear();
	mVoiceAccountServerURI.clear();
	
	setState(stateLoggingOut);
	logoutSendMessage();
}

void LLVivoxVoiceClient::logoutSendMessage()
{
	if(!mAccountHandle.empty())
	{
		std::ostringstream stream;
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.Logout.1\">"
			<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		mAccountHandle.clear();

		writeString(stream.str());
	}
}

void LLVivoxVoiceClient::accountListBlockRulesSendMessage()
{
	if(!mAccountHandle.empty())
	{		
		std::ostringstream stream;

		LL_DEBUGS("Voice") << "requesting block rules" << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.ListBlockRules.1\">"
			<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLVivoxVoiceClient::accountListAutoAcceptRulesSendMessage()
{
	if(!mAccountHandle.empty())
	{		
		std::ostringstream stream;

		LL_DEBUGS("Voice") << "requesting auto-accept rules" << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.ListAutoAcceptRules.1\">"
			<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLVivoxVoiceClient::sessionGroupCreateSendMessage()
{
	if(!mAccountHandle.empty())
	{		
		std::ostringstream stream;

		LL_DEBUGS("Voice") << "creating session group" << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.Create.1\">"
			<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
			<< "<Type>Normal</Type>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLVivoxVoiceClient::sessionCreateSendMessage(sessionState *session, bool startAudio, bool startText)
{
	LL_DEBUGS("Voice") << "Requesting create: " << session->mSIPURI << LL_ENDL;

	S32 font_index = getVoiceFontIndex(session->mVoiceFontID);
	LL_DEBUGS("Voice") << "With voice font: " << session->mVoiceFontID << " (" << font_index << ")" << LL_ENDL;

	session->mCreateInProgress = true;
	if(startAudio)
	{
		session->mMediaConnectInProgress = true;
	}

	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << session->mSIPURI << "\" action=\"Session.Create.1\">"
		<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
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

void LLVivoxVoiceClient::sessionGroupAddSessionSendMessage(sessionState *session, bool startAudio, bool startText)
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

void LLVivoxVoiceClient::sessionMediaConnectSendMessage(sessionState *session)
{
	LL_DEBUGS("Voice") << "Connecting audio to session handle: " << session->mHandle << LL_ENDL;

	S32 font_index = getVoiceFontIndex(session->mVoiceFontID);
	LL_DEBUGS("Voice") << "With voice font: " << session->mVoiceFontID << " (" << font_index << ")" << LL_ENDL;

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

void LLVivoxVoiceClient::sessionTextConnectSendMessage(sessionState *session)
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

		switch(getState())
		{
			case stateNoChannel:
				// In this case, we want to pretend the join failed so our state machine doesn't get stuck.
				// Skip the join failed transition state so we don't send out error notifications.
				setState(stateJoinSessionFailedWaiting);
			break;
			case stateJoiningSession:
			case stateSessionJoined:
			case stateRunning:
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
					setState(stateLeavingSession);
				}
				else
				{
					LL_WARNS("Voice") << "called with no session handle" << LL_ENDL;	
					setState(stateSessionTerminated);
				}
			break;
			case stateJoinSessionFailed:
			case stateJoinSessionFailedWaiting:
				setState(stateSessionTerminated);
			break;
			
			default:
				LL_WARNS("Voice") << "called from unknown state" << LL_ENDL;
			break;
		}
	}
	else
	{
		LL_WARNS("Voice") << "called with no active session" << LL_ENDL;
		setState(stateSessionTerminated);
	}
}

void LLVivoxVoiceClient::sessionTerminateSendMessage(sessionState *session)
{
	std::ostringstream stream;
	
	LL_DEBUGS("Voice") << "Sending Session.Terminate with handle " << session->mHandle << LL_ENDL;	
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.Terminate.1\">"
		<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVivoxVoiceClient::sessionGroupTerminateSendMessage(sessionState *session)
{
	std::ostringstream stream;
	
	LL_DEBUGS("Voice") << "Sending SessionGroup.Terminate with handle " << session->mGroupHandle << LL_ENDL;	
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"SessionGroup.Terminate.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
}

void LLVivoxVoiceClient::sessionMediaDisconnectSendMessage(sessionState *session)
{
	std::ostringstream stream;
	
	LL_DEBUGS("Voice") << "Sending Session.MediaDisconnect with handle " << session->mHandle << LL_ENDL;	
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.MediaDisconnect.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
		<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
		<< "<Media>Audio</Media>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
	
}

void LLVivoxVoiceClient::sessionTextDisconnectSendMessage(sessionState *session)
{
	std::ostringstream stream;
	
	LL_DEBUGS("Voice") << "Sending Session.TextDisconnect with handle " << session->mHandle << LL_ENDL;	
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.TextDisconnect.1\">"
		<< "<SessionGroupHandle>" << session->mGroupHandle << "</SessionGroupHandle>"
		<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
	<< "</Request>\n\n\n";
	
	writeString(stream.str());
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

void LLVivoxVoiceClient::addCaptureDevice(const std::string& name)
{
	LL_DEBUGS("Voice") << name << LL_ENDL;

	mCaptureDevices.push_back(name);
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

void LLVivoxVoiceClient::clearRenderDevices()
{	
	LL_DEBUGS("Voice") << "called" << LL_ENDL;
	mRenderDevices.clear();
}

void LLVivoxVoiceClient::addRenderDevice(const std::string& name)
{
	LL_DEBUGS("Voice") << name << LL_ENDL;
	mRenderDevices.push_back(name);
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
	mTuningMode = true;
	LL_DEBUGS("Voice") << "Starting tuning" << LL_ENDL;
	if(getState() >= stateNoChannel)
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
	bool result = false;
	switch(getState())
	{
	case stateMicTuningRunning:
		result = true;
		break;
	default:
		break;
	}
	return result;
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

void LLVivoxVoiceClient::tuningCaptureStartSendMessage(int duration)
{
	LL_DEBUGS("Voice") << "sending CaptureAudioStart" << LL_ENDL;
	
	std::ostringstream stream;
	stream
	<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.CaptureAudioStart.1\">"
    << "<Duration>" << duration << "</Duration>"
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
	
	if(!mConnected)
		result = false;
	
	if(mRenderDevices.empty())
		result = false;
	
	return result;
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

	// Try to relaunch the daemon
	setState(stateDisableCleanup);
}

void LLVivoxVoiceClient::giveUp()
{
	// All has failed.  Clean up and stop trying.
	closeSocket();
	cleanUp();
	
	setState(stateJail);
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

void LLVivoxVoiceClient::sendPositionalUpdate(void)
{	
	std::ostringstream stream;
	
	if(mSpatialCoordsDirty)
	{
		LLVector3 l, u, a, vel;
		LLVector3d pos;

		mSpatialCoordsDirty = false;
		
		// Always send both speaker and listener positions together.
		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.Set3DPosition.1\">"		
			<< "<SessionHandle>" << getAudioSessionHandle() << "</SessionHandle>";
		
		stream << "<SpeakerPosition>";

//		LL_DEBUGS("Voice") << "Sending speaker position " << mAvatarPosition << LL_ENDL;
		l = mAvatarRot.getLeftRow();
		u = mAvatarRot.getUpRow();
		a = mAvatarRot.getFwdRow();
		pos = mAvatarPosition;
		vel = mAvatarVelocity;

		// SLIM SDK: the old SDK was doing a transform on the passed coordinates that the new one doesn't do anymore.
		// The old transform is replicated by this function.
		oldSDKTransform(l, u, a, pos, vel);
		
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
			<< "</LeftOrientation>";

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
				earRot = mAvatarRot;
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

//		LL_DEBUGS("Voice") << "Sending listener position " << earPosition << LL_ENDL;
		
		oldSDKTransform(l, u, a, pos, vel);
		
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
			<< "</LeftOrientation>";


		stream << "</ListenerPosition>";

		stream << "</Request>\n\n\n";
	}	
	
	if(mAudioSession && (mAudioSession->mVolumeDirty || mAudioSession->mMuteDirty))
	{
		participantMap::iterator iter = mAudioSession->mParticipantsByURI.begin();

		mAudioSession->mVolumeDirty = false;
		mAudioSession->mMuteDirty = false;
		
		for(; iter != mAudioSession->mParticipantsByURI.end(); iter++)
		{
			participantState *p = iter->second;
			
			if(p->mVolumeDirty)
			{
				// Can't set volume/mute for yourself
				if(!p->mIsSelf)
				{
					// scale from the range 0.0-1.0 to vivox volume in the range 0-100
					S32 volume = llround(p->mVolume / VOLUME_SCALE_VIVOX);
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
			
	buildLocalAudioUpdates(stream);
	
	if(!stream.str().empty())
	{
		writeString(stream.str());
	}
	
	// Friends list updates can be huge, especially on the first voice login of an account with lots of friends.
	// Batching them all together can choke SLVoice, so send them in separate writes.
	sendFriendsListUpdates();
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

void LLVivoxVoiceClient::buildLocalAudioUpdates(std::ostringstream &stream)
{
	buildSetCaptureDevice(stream);

	buildSetRenderDevice(stream);

	if(mMuteMicDirty)
	{
		mMuteMicDirty = false;

		// Send a local mute command.
		
		LL_DEBUGS("Voice") << "Sending MuteLocalMic command with parameter " << (mMuteMic?"true":"false") << LL_ENDL;

		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.MuteLocalMic.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>" << (mMuteMic?"true":"false") << "</Value>"
			<< "</Request>\n\n\n";
		
	}

	if(mSpeakerMuteDirty)
	{
	  const char *muteval = ((mSpeakerVolume <= scale_speaker_volume(0))?"true":"false");

		mSpeakerMuteDirty = false;

		LL_INFOS("Voice") << "Setting speaker mute to " << muteval  << LL_ENDL;
		
		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.MuteLocalSpeaker.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>" << muteval << "</Value>"
			<< "</Request>\n\n\n";	
		
	}
	
	if(mSpeakerVolumeDirty)
	{
		mSpeakerVolumeDirty = false;

		LL_INFOS("Voice") << "Setting speaker volume to " << mSpeakerVolume  << LL_ENDL;

		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.SetLocalSpeakerVolume.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>" << mSpeakerVolume << "</Value>"
			<< "</Request>\n\n\n";
			
	}
	
	if(mMicVolumeDirty)
	{
		mMicVolumeDirty = false;

		LL_INFOS("Voice") << "Setting mic volume to " << mMicVolume  << LL_ENDL;

		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.SetLocalMicVolume.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>" << mMicVolume << "</Value>"
			<< "</Request>\n\n\n";				
	}

	
}

void LLVivoxVoiceClient::checkFriend(const LLUUID& id)
{
	buddyListEntry *buddy = findBuddy(id);

	// Make sure we don't add a name before it's been looked up.
	LLAvatarName av_name;
	if(LLAvatarNameCache::get(id, &av_name))
	{
		// *NOTE: For now, we feed legacy names to Vivox because I don't know
		// if their service can support a mix of new and old clients with
		// different sorts of names.
		std::string name = av_name.getLegacyName();

		const LLRelationship* relationInfo = LLAvatarTracker::instance().getBuddyInfo(id);
		bool canSeeMeOnline = false;
		if(relationInfo && relationInfo->isRightGrantedTo(LLRelationship::GRANT_ONLINE_STATUS))
			canSeeMeOnline = true;
		
		// When we get here, mNeedsSend is true and mInSLFriends is false.  Change them as necessary.
		
		if(buddy)
		{
			// This buddy is already in both lists.

			if(name != buddy->mDisplayName)
			{
				// The buddy is in the list with the wrong name.  Update it with the correct name.
				LL_WARNS("Voice") << "Buddy " << id << " has wrong name (\"" << buddy->mDisplayName << "\" should be \"" << name << "\"), updating."<< LL_ENDL;
				buddy->mDisplayName = name;
				buddy->mNeedsNameUpdate = true;		// This will cause the buddy to be resent.
			}
		}
		else
		{
			// This buddy was not in the vivox list, needs to be added.
			buddy = addBuddy(sipURIFromID(id), name);
			buddy->mUUID = id;
		}
		
		// In all the above cases, the buddy is in the SL friends list (which is how we got here).
		buddy->mInSLFriends = true;
		buddy->mCanSeeMeOnline = canSeeMeOnline;
		buddy->mNameResolved = true;
		
	}
	else
	{
		// This name hasn't been looked up yet.  Don't do anything with this buddy list entry until it has.
		if(buddy)
		{
			buddy->mNameResolved = false;
		}
		
		// Initiate a lookup.
		// The "lookup completed" callback will ensure that the friends list is rechecked after it completes.
		lookupName(id);
	}
}

void LLVivoxVoiceClient::clearAllLists()
{
	// FOR TESTING ONLY
	
	// This will send the necessary commands to delete ALL buddies, autoaccept rules, and block rules SLVoice tells us about.
	buddyListMap::iterator buddy_it;
	for(buddy_it = mBuddyListMap.begin(); buddy_it != mBuddyListMap.end();)
	{
		buddyListEntry *buddy = buddy_it->second;
		buddy_it++;
		
		std::ostringstream stream;

		if(buddy->mInVivoxBuddies)
		{
			// delete this entry from the vivox buddy list
			buddy->mInVivoxBuddies = false;
			LL_DEBUGS("Voice") << "delete " << buddy->mURI << " (" << buddy->mDisplayName << ")" << LL_ENDL;
			stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.BuddyDelete.1\">"
				<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
				<< "<BuddyURI>" << buddy->mURI << "</BuddyURI>"
				<< "</Request>\n\n\n";		
		}

		if(buddy->mHasBlockListEntry)
		{
			// Delete the associated block list entry (so the block list doesn't fill up with junk)
			buddy->mHasBlockListEntry = false;
			stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.DeleteBlockRule.1\">"
				<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
				<< "<BlockMask>" << buddy->mURI << "</BlockMask>"
				<< "</Request>\n\n\n";								
		}
		if(buddy->mHasAutoAcceptListEntry)
		{
			// Delete the associated auto-accept list entry (so the auto-accept list doesn't fill up with junk)
			buddy->mHasAutoAcceptListEntry = false;
			stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.DeleteAutoAcceptRule.1\">"
				<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
				<< "<AutoAcceptMask>" << buddy->mURI << "</AutoAcceptMask>"
				<< "</Request>\n\n\n";
		}

		writeString(stream.str());

	}
}

void LLVivoxVoiceClient::sendFriendsListUpdates()
{
	if(mBuddyListMapPopulated && mBlockRulesListReceived && mAutoAcceptRulesListReceived && mFriendsListDirty)
	{
		mFriendsListDirty = false;
		
		if(0)
		{
			// FOR TESTING ONLY -- clear all buddy list, block list, and auto-accept list entries.
			clearAllLists();
			return;
		}
		
		LL_INFOS("Voice") << "Checking vivox buddy list against friends list..." << LL_ENDL;
		
		buddyListMap::iterator buddy_it;
		for(buddy_it = mBuddyListMap.begin(); buddy_it != mBuddyListMap.end(); buddy_it++)
		{
			// reset the temp flags in the local buddy list
			buddy_it->second->mInSLFriends = false;
		}
		
		// correlate with the friends list
		{
			LLCollectAllBuddies collect;
			LLAvatarTracker::instance().applyFunctor(collect);
			LLCollectAllBuddies::buddy_map_t::const_iterator it = collect.mOnline.begin();
			LLCollectAllBuddies::buddy_map_t::const_iterator end = collect.mOnline.end();
			
			for ( ; it != end; ++it)
			{
				checkFriend(it->second);
			}
			it = collect.mOffline.begin();
			end = collect.mOffline.end();
			for ( ; it != end; ++it)
			{
				checkFriend(it->second);
			}
		}
				
		LL_INFOS("Voice") << "Sending friend list updates..." << LL_ENDL;

		for(buddy_it = mBuddyListMap.begin(); buddy_it != mBuddyListMap.end();)
		{
			buddyListEntry *buddy = buddy_it->second;
			buddy_it++;
			
			// Ignore entries that aren't resolved yet.
			if(buddy->mNameResolved)
			{
				std::ostringstream stream;

				if(buddy->mInSLFriends && (!buddy->mInVivoxBuddies || buddy->mNeedsNameUpdate))
				{					
					if(mNumberOfAliases > 0)
					{
						// Add (or update) this entry in the vivox buddy list
						buddy->mInVivoxBuddies = true;
						buddy->mNeedsNameUpdate = false;
						LL_DEBUGS("Voice") << "add/update " << buddy->mURI << " (" << buddy->mDisplayName << ")" << LL_ENDL;
						stream 
							<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.BuddySet.1\">"
								<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
								<< "<BuddyURI>" << buddy->mURI << "</BuddyURI>"
								<< "<DisplayName>" << buddy->mDisplayName << "</DisplayName>"
								<< "<BuddyData></BuddyData>"	// Without this, SLVoice doesn't seem to parse the command.
								<< "<GroupID>0</GroupID>"
							<< "</Request>\n\n\n";	
					}
				}
				else if(!buddy->mInSLFriends)
				{
					// This entry no longer exists in your SL friends list.  Remove all traces of it from the Vivox buddy list.
 					if(buddy->mInVivoxBuddies)
					{
						// delete this entry from the vivox buddy list
						buddy->mInVivoxBuddies = false;
						LL_DEBUGS("Voice") << "delete " << buddy->mURI << " (" << buddy->mDisplayName << ")" << LL_ENDL;
						stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.BuddyDelete.1\">"
							<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
							<< "<BuddyURI>" << buddy->mURI << "</BuddyURI>"
							<< "</Request>\n\n\n";		
					}

					if(buddy->mHasBlockListEntry)
					{
						// Delete the associated block list entry, if any
						buddy->mHasBlockListEntry = false;
						stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.DeleteBlockRule.1\">"
							<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
							<< "<BlockMask>" << buddy->mURI << "</BlockMask>"
							<< "</Request>\n\n\n";								
					}
					if(buddy->mHasAutoAcceptListEntry)
					{
						// Delete the associated auto-accept list entry, if any
						buddy->mHasAutoAcceptListEntry = false;
						stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.DeleteAutoAcceptRule.1\">"
							<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
							<< "<AutoAcceptMask>" << buddy->mURI << "</AutoAcceptMask>"
							<< "</Request>\n\n\n";
					}
				}
				
				if(buddy->mInSLFriends)
				{

					if(buddy->mCanSeeMeOnline)
					{
						// Buddy should not be blocked.

						// If this buddy doesn't already have either a block or autoaccept list entry, we'll update their status when we receive a SubscriptionEvent.
						
						// If the buddy has a block list entry, delete it.
						if(buddy->mHasBlockListEntry)
						{
							buddy->mHasBlockListEntry = false;
							stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.DeleteBlockRule.1\">"
								<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
								<< "<BlockMask>" << buddy->mURI << "</BlockMask>"
								<< "</Request>\n\n\n";		
							
							
							// If we just deleted a block list entry, add an auto-accept entry.
							if(!buddy->mHasAutoAcceptListEntry)
							{
								buddy->mHasAutoAcceptListEntry = true;								
								stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.CreateAutoAcceptRule.1\">"
									<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
									<< "<AutoAcceptMask>" << buddy->mURI << "</AutoAcceptMask>"
									<< "<AutoAddAsBuddy>0</AutoAddAsBuddy>"
									<< "</Request>\n\n\n";
							}
						}
					}
					else
					{
						// Buddy should be blocked.
						
						// If this buddy doesn't already have either a block or autoaccept list entry, we'll update their status when we receive a SubscriptionEvent.

						// If this buddy has an autoaccept entry, delete it
						if(buddy->mHasAutoAcceptListEntry)
						{
							buddy->mHasAutoAcceptListEntry = false;
							stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.DeleteAutoAcceptRule.1\">"
								<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
								<< "<AutoAcceptMask>" << buddy->mURI << "</AutoAcceptMask>"
								<< "</Request>\n\n\n";
						
							// If we just deleted an auto-accept entry, add a block list entry.
							if(!buddy->mHasBlockListEntry)
							{
								buddy->mHasBlockListEntry = true;
								stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.CreateBlockRule.1\">"
									<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
									<< "<BlockMask>" << buddy->mURI << "</BlockMask>"
									<< "<PresenceOnly>1</PresenceOnly>"
									<< "</Request>\n\n\n";								
							}
						}
					}

					if(!buddy->mInSLFriends && !buddy->mInVivoxBuddies)
					{
						// Delete this entry from the local buddy list.  This should NOT invalidate the iterator,
						// since it has already been incremented to the next entry.
						deleteBuddy(buddy->mURI);
					}

				}
				writeString(stream.str());
			}
		}
	}
}

/////////////////////////////
// Response/Event handlers

void LLVivoxVoiceClient::connectorCreateResponse(int statusCode, std::string &statusString, std::string &connectorHandle, std::string &versionID)
{	
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Connector.Create response failure: " << statusString << LL_ENDL;
		setState(stateConnectorFailed);
		LLSD args;
		std::stringstream errs;
		errs << mVoiceAccountServerURI << "\n:UDP: 3478, 3479, 5060, 5062, 12000-17000";
		args["HOSTID"] = errs.str();
		if (LLGridManager::getInstance()->isSystemGrid())
		{
			LLNotificationsUtil::add("NoVoiceConnect", args);	
		}
		else
		{
			LLNotificationsUtil::add("NoVoiceConnect-GIAB", args);	
		}
	}
	else
	{
		// Connector created, move forward.
		LL_INFOS("Voice") << "Connector.Create succeeded, Vivox SDK version is " << versionID << LL_ENDL;
		mVoiceVersion.serverVersion = versionID;
		mConnectorHandle = connectorHandle;
		if(getState() == stateConnectorStarting)
		{
			setState(stateConnectorStarted);
		}
	}
}

void LLVivoxVoiceClient::loginResponse(int statusCode, std::string &statusString, std::string &accountHandle, int numberOfAliases)
{ 
	LL_DEBUGS("Voice") << "Account.Login response (" << statusCode << "): " << statusString << LL_ENDL;
	
	// Status code of 20200 means "bad password".  We may want to special-case that at some point.
	
	if ( statusCode == 401 )
	{
		// Login failure which is probably caused by the delay after a user's password being updated.
		LL_INFOS("Voice") << "Account.Login response failure (" << statusCode << "): " << statusString << LL_ENDL;
		setState(stateLoginRetry);
	}
	else if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Account.Login response failure (" << statusCode << "): " << statusString << LL_ENDL;
		setState(stateLoginFailed);
	}
	else
	{
		// Login succeeded, move forward.
		mAccountHandle = accountHandle;
		mNumberOfAliases = numberOfAliases;
		// This needs to wait until the AccountLoginStateChangeEvent is received.
//		if(getState() == stateLoggingIn)
//		{
//			setState(stateLoggedIn);
//		}
	}
}

void LLVivoxVoiceClient::sessionCreateResponse(std::string &requestId, int statusCode, std::string &statusString, std::string &sessionHandle)
{	
	sessionState *session = findSessionBeingCreatedByURI(requestId);
	
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
				setState(stateJoinSessionFailed);
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
	}
}

void LLVivoxVoiceClient::sessionGroupAddSessionResponse(std::string &requestId, int statusCode, std::string &statusString, std::string &sessionHandle)
{	
	sessionState *session = findSessionBeingCreatedByURI(requestId);
	
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
				setState(stateJoinSessionFailed);
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
	}
}

void LLVivoxVoiceClient::sessionConnectResponse(std::string &requestId, int statusCode, std::string &statusString)
{
	sessionState *session = findSession(requestId);
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Session.Connect response failure (" << statusCode << "): " << statusString << LL_ENDL;
		if(session)
		{
			session->mMediaConnectInProgress = false;
			session->mErrorStatusCode = statusCode;		
			session->mErrorStatusString = statusString;
			if(session == mAudioSession)
				setState(stateJoinSessionFailed);
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
}

void LLVivoxVoiceClient::connectorShutdownResponse(int statusCode, std::string &statusString)
{
	if(statusCode != 0)
	{
		LL_WARNS("Voice") << "Connector.InitiateShutdown response failure: " << statusString << LL_ENDL;
		// Should this ever fail?  do we care if it does?
	}
	
	mConnected = false;
	
	if(getState() == stateConnectorStopping)
	{
		setState(stateConnectorStopped);
	}
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
	sessionState *session = NULL;

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
	
#if USE_SESSION_GROUPS
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

void LLVivoxVoiceClient::joinedAudioSession(sessionState *session)
{
	LL_DEBUGS("Voice") << "Joined Audio Session" << LL_ENDL;
	if(mAudioSession != session)
	{
		sessionState *oldSession = mAudioSession;

		mAudioSession = session;
		mAudioSessionChanged = true;

		// The old session may now need to be deleted.
		reapSession(oldSession);
	}
	
	// This is the session we're joining.
	if(getState() == stateJoiningSession)
	{
		setState(stateSessionJoined);
		
		// SLIM SDK: we don't always receive a participant state change for ourselves when joining a channel now.
		// Add the current user as a participant here.
		participantState *participant = session->addParticipant(sipURIFromName(mAccountName));
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
			participantState *participant = session->addParticipant(session->mSIPURI);
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
	
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		leftAudioSession(session);

		// This message invalidates the session's handle.  Set it to empty.
		setSessionHandle(session);
		
		// This also means that the session's session group is now empty.
		// Terminate the session group so it doesn't leak.
		sessionGroupTerminateSendMessage(session);
		
		// Reset the media state (we now have no info)
		session->mMediaStreamState = streamStateUnknown;
		session->mTextStreamState = streamStateUnknown;
		
		// Conditionally delete the session
		reapSession(session);
	}
	else
	{
		LL_WARNS("Voice") << "unknown session " << sessionHandle << " removed" << LL_ENDL;
	}
}

void LLVivoxVoiceClient::reapSession(sessionState *session)
{
	if(session)
	{
		if(!session->mHandle.empty())
		{
			LL_DEBUGS("Voice") << "NOT deleting session " << session->mSIPURI << " (non-null session handle)" << LL_ENDL;
		}
		else if(session->mCreateInProgress)
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
			// TODO: Question: Should we check for queued text messages here?
			// We don't have a reason to keep tracking this session, so just delete it.
			LL_DEBUGS("Voice") << "deleting session " << session->mSIPURI << LL_ENDL;
			deleteSession(session);
			session = NULL;
		}	
	}
	else
	{
//		LL_DEBUGS("Voice") << "session is NULL" << LL_ENDL;
	}
}

// Returns true if the session seems to indicate we've moved to a region on a different voice server
bool LLVivoxVoiceClient::sessionNeedsRelog(sessionState *session)
{
	bool result = false;
	
	if(session != NULL)
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

void LLVivoxVoiceClient::leftAudioSession(
	sessionState *session)
{
	if(mAudioSession == session)
	{
		switch(getState())
		{
			case stateJoiningSession:
			case stateSessionJoined:
			case stateRunning:
			case stateLeavingSession:
			case stateJoinSessionFailed:
			case stateJoinSessionFailedWaiting:
				// normal transition
				LL_DEBUGS("Voice") << "left session " << session->mHandle << " in state " << state2string(getState()) << LL_ENDL;
				setState(stateSessionTerminated);
			break;
			
			case stateSessionTerminated:
				// this will happen sometimes -- there are cases where we send the terminate and then go straight to this state.
				LL_WARNS("Voice") << "left session " << session->mHandle << " in state " << state2string(getState()) << LL_ENDL;
			break;
			
			default:
				LL_WARNS("Voice") << "unexpected SessionStateChangeEvent (left session) in state " << state2string(getState()) << LL_ENDL;
				setState(stateSessionTerminated);
			break;
		}
	}
}

void LLVivoxVoiceClient::accountLoginStateChangeEvent(
		std::string &accountHandle, 
		int statusCode, 
		std::string &statusString, 
		int state)
{
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
		if(getState() == stateLoggingIn)
		{
			setState(stateLoggedIn);
		}
		break;

		case 3:
			// The user is in the process of logging out.
			setState(stateLoggingOut);
		break;

		case 0:
			// The user has been logged out.  
			setState(stateLoggedOut);
		break;
		
		default:
			//Used to be a commented out warning
			LL_DEBUGS("Voice") << "unknown state: " << state << LL_ENDL;
		break;
	}
}

void LLVivoxVoiceClient::mediaCompletionEvent(std::string &sessionGroupHandle, std::string &mediaCompletionType)
{
	if (mediaCompletionType == "AuxBufferAudioCapture")
	{
		mCaptureBufferRecording = false;
	}
	else if (mediaCompletionType == "AuxBufferAudioRender")
	{
		// Ignore all but the last stop event
		if (--mPlayRequestCount <= 0)
		{
			mCaptureBufferPlaying = false;
		}
	}
	else
	{
		LL_DEBUGS("Voice") << "Unknown MediaCompletionType: " << mediaCompletionType << LL_ENDL;
	}
}

void LLVivoxVoiceClient::mediaStreamUpdatedEvent(
	std::string &sessionHandle, 
	std::string &sessionGroupHandle, 
	int statusCode, 
	std::string &statusString, 
	int state, 
	bool incoming)
{
	sessionState *session = findSession(sessionHandle);
	
	LL_DEBUGS("Voice") << "session " << sessionHandle << ", status code " << statusCode << ", string \"" << statusString << "\"" << LL_ENDL;
	
	if(session)
	{
		// We know about this session
		
		// Save the state for later use
		session->mMediaStreamState = state;
		
		switch(statusCode)
		{
			case 0:
			case 200:
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
			case streamStateIdle:
				// Standard "left audio session"
				session->mVoiceEnabled = false;
				session->mMediaConnectInProgress = false;
				leftAudioSession(session);
			break;

			case streamStateConnected:
				session->mVoiceEnabled = true;
				session->mMediaConnectInProgress = false;
				joinedAudioSession(session);
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
		LL_WARNS("Voice") << "session " << sessionHandle << "not found"<< LL_ENDL;
	}
}

void LLVivoxVoiceClient::textStreamUpdatedEvent(
	std::string &sessionHandle, 
	std::string &sessionGroupHandle, 
	bool enabled,
	int state, 
	bool incoming)
{
	sessionState *session = findSession(sessionHandle);
	
	if(session)
	{
		// Save the state for later use
		session->mTextStreamState = state;
		
		// We know about this session
		switch(state)
		{
			case 0:	// We see this when the text stream closes
				LL_DEBUGS("Voice") << "stream closed" << LL_ENDL;
			break;
			
			case 1:	// We see this on an incoming call from the Connector
				// Try to send any text messages queued for this session.
				sendQueuedTextMessages(session);

				// Send the text chat invite to the GUI layer
				// TODO: Question: Should we correlate with the mute list here?
				session->mTextInvitePending = true;
				if(session->mName.empty())
				{
					lookupName(session->mCallerID);
				}
				else
				{
					// Act like we just finished resolving the name
					avatarNameResolved(session->mCallerID, session->mName);
				}
			break;

			default:
				LL_WARNS("Voice") << "unknown state " << state << LL_ENDL;
			break;
			
		}
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
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		participantState *participant = session->addParticipant(uriString);
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
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		participantState *participant = session->findParticipant(uriString);
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
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		participantState *participant = session->findParticipant(uriString);
		
		if(participant)
		{
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
			 
			 So, we have to call LLSpeakerMgr::update() here. In any case it is better than call it                                                
			 in LLCallFloater::draw()                                                                                                              
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
		LL_INFOS("Voice") << "unknown session " << sessionHandle << LL_ENDL;
	}
}

void LLVivoxVoiceClient::buddyPresenceEvent(
		std::string &uriString, 
		std::string &alias, 
		std::string &statusString,
		std::string &applicationString)
{
	buddyListEntry *buddy = findBuddy(uriString);
	
	if(buddy)
	{
		LL_DEBUGS("Voice") << "Presence event for " << buddy->mDisplayName << " status \"" << statusString << "\", application \"" << applicationString << "\""<< LL_ENDL;
		LL_DEBUGS("Voice") << "before: mOnlineSL = " << (buddy->mOnlineSL?"true":"false") << ", mOnlineSLim = " << (buddy->mOnlineSLim?"true":"false") << LL_ENDL;

		if(applicationString.empty())
		{
			// This presence event is from a client that doesn't set up the Application string.  Do things the old-skool way.
			// NOTE: this will be needed to support people who aren't on the 3010-class SDK yet.

			if ( stricmp("Unknown", statusString.c_str())== 0) 
			{
				// User went offline with a non-SLim-enabled viewer.
				buddy->mOnlineSL = false;
			}
			else if ( stricmp("Online", statusString.c_str())== 0) 
			{
				// User came online with a non-SLim-enabled viewer.
				buddy->mOnlineSL = true;
			}
			else
			{
				// If the user is online through SLim, their status will be "Online-slc", "Away", or something else.
				// NOTE: we should never see this unless someone is running an OLD version of SLim -- the versions that should be in use now all set the application string.
				buddy->mOnlineSLim = true;
			} 
		}
		else if(applicationString.find("SecondLifeViewer") != std::string::npos)
		{
			// This presence event is from a viewer that sets the application string
			if ( stricmp("Unknown", statusString.c_str())== 0) 
			{
				// Viewer says they're offline
				buddy->mOnlineSL = false;
			}
			else
			{
				// Viewer says they're online
				buddy->mOnlineSL = true;
			}
		}
		else
		{
			// This presence event is from something which is NOT the SL viewer (assume it's SLim).
			if ( stricmp("Unknown", statusString.c_str())== 0) 
			{
				// SLim says they're offline
				buddy->mOnlineSLim = false;
			}
			else
			{
				// SLim says they're online
				buddy->mOnlineSLim = true;
			}
		} 

		LL_DEBUGS("Voice") << "after: mOnlineSL = " << (buddy->mOnlineSL?"true":"false") << ", mOnlineSLim = " << (buddy->mOnlineSLim?"true":"false") << LL_ENDL;
		
		// HACK -- increment the internal change serial number in the LLRelationship (without changing the actual status), so the UI notices the change.
		LLAvatarTracker::instance().setBuddyOnline(buddy->mUUID,LLAvatarTracker::instance().isBuddyOnline(buddy->mUUID));

		notifyFriendObservers();
	}
	else
	{
		LL_DEBUGS("Voice") << "Presence for unknown buddy " << uriString << LL_ENDL;
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
	
	if(messageHeader.find("text/html") != std::string::npos)
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
		
		sessionState *session = findSession(sessionHandle);
		if(session)
		{
			bool is_busy = gAgent.getBusy();
			bool is_muted = LLMuteList::getInstance()->isMuted(session->mCallerID, session->mName, LLMute::flagTextChat);
			bool is_linden = LLMuteList::getInstance()->isLinden(session->mName);
			LLChat chat;

			chat.mMuted = is_muted && !is_linden;
			
			if(!chat.mMuted)
			{
				chat.mFromID = session->mCallerID;
				chat.mFromName = session->mName;
				chat.mSourceType = CHAT_SOURCE_AGENT;

				if(is_busy && !is_linden)
				{
					// TODO: Question: Return busy mode response here?  Or maybe when session is started instead?
				}
				
				LL_DEBUGS("Voice") << "adding message, name " << session->mName << " session " << session->mIMSessionID << ", target " << session->mCallerID << LL_ENDL;
				LLIMMgr::getInstance()->addMessage(session->mIMSessionID,
						session->mCallerID,
						session->mName.c_str(),
						message.c_str(),
						LLStringUtil::null,		// default arg
						IM_NOTHING_SPECIAL,		// default arg
						0,						// default arg
						LLUUID::null,			// default arg
						LLVector3::zero,		// default arg
						true);					// prepend name and make it a link to the user's profile

			}
		}		
	}
}

void LLVivoxVoiceClient::sessionNotificationEvent(std::string &sessionHandle, std::string &uriString, std::string &notificationType)
{
	sessionState *session = findSession(sessionHandle);
	
	if(session)
	{
		participantState *participant = session->findParticipant(uriString);
		if(participant)
		{
			if (!stricmp(notificationType.c_str(), "Typing"))
			{
				// Other end started typing
				// TODO: The proper way to add a typing notification seems to be LLIMMgr::processIMTypingStart().
				// It requires an LLIMInfo for the message, which we don't have here.
			}
			else if (!stricmp(notificationType.c_str(), "NotTyping"))
			{
				// Other end stopped typing
				// TODO: The proper way to remove a typing notification seems to be LLIMMgr::processIMTypingStop().
				// It requires an LLIMInfo for the message, which we don't have here.
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

void LLVivoxVoiceClient::subscriptionEvent(std::string &buddyURI, std::string &subscriptionHandle, std::string &alias, std::string &displayName, std::string &applicationString, std::string &subscriptionType)
{
	buddyListEntry *buddy = findBuddy(buddyURI);
	
	if(!buddy)
	{
		// Couldn't find buddy by URI, try converting the alias...
		if(!alias.empty())
		{
			LLUUID id;
			if(IDFromName(alias, id))
			{
				buddy = findBuddy(id);
			}
		}
	}
	
	if(buddy)
	{
		std::ostringstream stream;
		
		if(buddy->mCanSeeMeOnline)
		{
			// Sending the response will create an auto-accept rule
			buddy->mHasAutoAcceptListEntry = true;
		}
		else
		{
			// Sending the response will create a block rule
			buddy->mHasBlockListEntry = true;
		}
		
		if(buddy->mInSLFriends)
		{
			buddy->mInVivoxBuddies = true;
		}
		
		stream
			<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.SendSubscriptionReply.1\">"
				<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
				<< "<BuddyURI>" << buddy->mURI << "</BuddyURI>"
				<< "<RuleType>" << (buddy->mCanSeeMeOnline?"Allow":"Hide") << "</RuleType>"
				<< "<AutoAccept>"<< (buddy->mInSLFriends?"1":"0")<< "</AutoAccept>"
				<< "<SubscriptionHandle>" << subscriptionHandle << "</SubscriptionHandle>"
			<< "</Request>"
			<< "\n\n\n";
			
		writeString(stream.str());
	}
}

void LLVivoxVoiceClient::auxAudioPropertiesEvent(F32 energy)
{
	LL_DEBUGS("Voice") << "got energy " << energy << LL_ENDL;
	mTuningEnergy = energy;
}

void LLVivoxVoiceClient::buddyListChanged()
{
	// This is called after we receive a BuddyAndGroupListChangedEvent.
	mBuddyListMapPopulated = true;
	mFriendsListDirty = true;
}

void LLVivoxVoiceClient::muteListChanged()
{
	// The user's mute list has been updated.  Go through the current participant list and sync it with the mute list.
	if(mAudioSession)
	{
		participantMap::iterator iter = mAudioSession->mParticipantsByURI.begin();
		
		for(; iter != mAudioSession->mParticipantsByURI.end(); iter++)
		{
			participantState *p = iter->second;
			
			// Check to see if this participant is on the mute list already
			if(p->updateMuteState())
				mAudioSession->mVolumeDirty = true;
		}
	}
}

void LLVivoxVoiceClient::updateFriends(U32 mask)
{
	if(mask & (LLFriendObserver::ADD | LLFriendObserver::REMOVE | LLFriendObserver::POWERS))
	{
		// Just resend the whole friend list to the daemon
		mFriendsListDirty = true;
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

LLVivoxVoiceClient::participantState *LLVivoxVoiceClient::sessionState::addParticipant(const std::string &uri)
{
	participantState *result = NULL;
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
		result = new participantState(useAlternateURI?mSIPURI:uri);
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

void LLVivoxVoiceClient::sessionState::removeParticipant(LLVivoxVoiceClient::participantState *participant)
{
	if(participant)
	{
		participantMap::iterator iter = mParticipantsByURI.find(participant->mURI);
		participantUUIDMap::iterator iter2 = mParticipantsByUUID.find(participant->mAvatarID);
		
		LL_DEBUGS("Voice") << "participant \"" << participant->mURI <<  "\" (" << participant->mAvatarID << ") removed." << LL_ENDL;
		
		if(iter == mParticipantsByURI.end())
		{
			LL_ERRS("Voice") << "Internal error: participant " << participant->mURI << " not in URI map" << LL_ENDL;
		}
		else if(iter2 == mParticipantsByUUID.end())
		{
			LL_ERRS("Voice") << "Internal error: participant ID " << participant->mAvatarID << " not in UUID map" << LL_ENDL;
		}
		else if(iter->second != iter2->second)
		{
			LL_ERRS("Voice") << "Internal error: participant mismatch!" << LL_ENDL;
		}
		else
		{
			mParticipantsByURI.erase(iter);
			mParticipantsByUUID.erase(iter2);
			
			delete participant;
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
		LL_ERRS("Voice") << "Internal error: empty URI map, non-empty UUID map" << LL_ENDL;
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


LLVivoxVoiceClient::participantState *LLVivoxVoiceClient::sessionState::findParticipant(const std::string &uri)
{
	participantState *result = NULL;
	
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

LLVivoxVoiceClient::participantState* LLVivoxVoiceClient::sessionState::findParticipantByID(const LLUUID& id)
{
	participantState * result = NULL;
	participantUUIDMap::iterator iter = mParticipantsByUUID.find(id);

	if(iter != mParticipantsByUUID.end())
	{
		result = iter->second;
	}

	return result;
}

LLVivoxVoiceClient::participantState* LLVivoxVoiceClient::findParticipantByID(const LLUUID& id)
{
	participantState * result = NULL;
	
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

bool LLVivoxVoiceClient::parcelVoiceInfoReceived(state requesting_state)
{
	// pop back to the state we were in when the parcel changed and we managed to 
	// do the request.
	if(getState() == stateRetrievingParcelVoiceInfo)
	{
		setState(requesting_state);
		return true;
	}
	else
	{
		// we've dropped out of stateRetrievingParcelVoiceInfo
		// before we received the cap result, due to a terminate
		// or transition to a non-voice channel.  Don't switch channels.
		return false;
	}
}


bool LLVivoxVoiceClient::requestParcelVoiceInfo()
{
	LLViewerRegion * region = gAgent.getRegion();
	if (region == NULL || !region->capabilitiesReceived())
	{
		// we don't have the cap yet, so return false so the caller can try again later.

		LL_DEBUGS("Voice") << "ParcelVoiceInfoRequest capability not yet available, deferring" << LL_ENDL;
		return false;
	}

	// grab the cap.
	std::string url = gAgent.getRegion()->getCapability("ParcelVoiceInfoRequest");
	if (url.empty())
	{
		// Region dosn't have the cap. Stop probing.
		LL_DEBUGS("Voice") << "ParcelVoiceInfoRequest capability not available in this region" << LL_ENDL;
		setState(stateDisableCleanup);
		return false;
	}
	else 
	{
		// if we've already retrieved the cap from the region, go ahead and make the request,
		// and return true so we can go into the state that waits for the response.
		checkParcelChanged(true);
		LLSD data;
		LL_DEBUGS("Voice") << "sending ParcelVoiceInfoRequest (" << mCurrentRegionName << ", " << mCurrentParcelLocalID << ")" << LL_ENDL;
		
		LLHTTPClient::post(
						url,
						data,
						new LLVivoxVoiceClientCapResponder(getState()));
		return true;
	}
}

void LLVivoxVoiceClient::switchChannel(
	std::string uri,
	bool spatial,
	bool no_reconnect,
	bool is_p2p,
	std::string hash)
{
	bool needsSwitch = false;
	
	LL_DEBUGS("Voice") 
		<< "called in state " << state2string(getState()) 
		<< " with uri \"" << uri << "\"" 
		<< (spatial?", spatial is true":", spatial is false")
		<< LL_ENDL;
	
	switch(getState())
	{
		case stateJoinSessionFailed:
		case stateJoinSessionFailedWaiting:
		case stateNoChannel:
		case stateRetrievingParcelVoiceInfo:
			// Always switch to the new URI from these states.
			needsSwitch = true;
		break;

		default:
			if(mSessionTerminateRequested)
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
						LL_WARNS("Voice") << "No current audio session." << LL_ENDL;
					}
				}
			}
		break;
	}
	
	if(needsSwitch)
	{
		if(uri.empty())
		{
			// Leave any channel we may be in
			LL_DEBUGS("Voice") << "leaving channel" << LL_ENDL;

			sessionState *oldSession = mNextAudioSession;
			mNextAudioSession = NULL;

			// The old session may now need to be deleted.
			reapSession(oldSession);

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
		
		if(getState() >= stateRetrievingParcelVoiceInfo)
		{
			// If we're already in a channel, or if we're joining one, terminate
			// so we can rejoin with the new session data.
			sessionTerminate();
		}
	}
}

void LLVivoxVoiceClient::joinSession(sessionState *session)
{
	mNextAudioSession = session;
	
	if(getState() <= stateNoChannel)
	{
		// We're already set up to join a channel, just needed to fill in the session handle
	}
	else
	{
		// State machine will come around and rejoin if uri/handle is not empty.
		sessionTerminate();
	}
}

void LLVivoxVoiceClient::setNonSpatialChannel(
	const std::string &uri,
	const std::string &credentials)
{
	switchChannel(uri, false, false, false, credentials);
}

void LLVivoxVoiceClient::setSpatialChannel(
	const std::string &uri,
	const std::string &credentials)
{
	mSpatialSessionURI = uri;
	mSpatialSessionCredentials = credentials;
	mAreaVoiceDisabled = mSpatialSessionURI.empty();

	LL_DEBUGS("Voice") << "got spatial channel uri: \"" << uri << "\"" << LL_ENDL;
	
	if((mAudioSession && !(mAudioSession->mIsSpatial)) || (mNextAudioSession && !(mNextAudioSession->mIsSpatial)))
	{
		// User is in a non-spatial chat or joining a non-spatial chat.  Don't switch channels.
		LL_INFOS("Voice") << "in non-spatial chat, not switching channels" << LL_ENDL;
	}
	else
	{
		switchChannel(mSpatialSessionURI, true, false, false, mSpatialSessionCredentials);
	}
}

void LLVivoxVoiceClient::callUser(const LLUUID &uuid)
{
	std::string userURI = sipURIFromID(uuid);

	switchChannel(userURI, false, true, true);
}

LLVivoxVoiceClient::sessionState* LLVivoxVoiceClient::startUserIMSession(const LLUUID &uuid)
{
	// Figure out if a session with the user already exists
	sessionState *session = findSession(uuid);
	if(!session)
	{
		// No session with user, need to start one.
		std::string uri = sipURIFromID(uuid);
		session = addSession(uri);

		llassert(session);
		if (!session) return NULL;

		session->mIsSpatial = false;
		session->mReconnect = false;	
		session->mIsP2P = true;
		session->mCallerID = uuid;
	}
	
	if(session->mHandle.empty())
	  {
	    // Session isn't active -- start it up.
	    sessionCreateSendMessage(session, false, true);
	  }
	else
	  {	
	    // Session is already active -- start up text.
	    sessionTextConnectSendMessage(session);
	  }
	
	return session;
}

BOOL LLVivoxVoiceClient::sendTextMessage(const LLUUID& participant_id, const std::string& message)
{
	bool result = false;

	// Attempt to locate the indicated session
	sessionState *session = startUserIMSession(participant_id);
	if(session)
	{
		// found the session, attempt to send the message
		session->mTextMsgQueue.push(message);
		
		// Try to send queued messages (will do nothing if the session is not open yet)
		sendQueuedTextMessages(session);

		// The message is queued, so we succeed.
		result = true;
	}	
	else
	{
		LL_DEBUGS("Voice") << "Session not found for participant ID " << participant_id << LL_ENDL;
	}
	
	return result;
}

void LLVivoxVoiceClient::sendQueuedTextMessages(sessionState *session)
{
	if(session->mTextStreamState == 1)
	{
		if(!session->mTextMsgQueue.empty())
		{
			std::ostringstream stream;
			
			while(!session->mTextMsgQueue.empty())
			{
				std::string message = session->mTextMsgQueue.front();
				session->mTextMsgQueue.pop();
				stream
				<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Session.SendMessage.1\">"
					<< "<SessionHandle>" << session->mHandle << "</SessionHandle>"
					<< "<MessageHeader>text/HTML</MessageHeader>"
					<< "<MessageBody>" << message << "</MessageBody>"
				<< "</Request>"
				<< "\n\n\n";
			}		
			writeString(stream.str());
		}
	}
	else
	{
		// Session isn't connected yet, defer until later.
	}
}

void LLVivoxVoiceClient::endUserIMSession(const LLUUID &uuid)
{
	// Figure out if a session with the user exists
	sessionState *session = findSession(uuid);
	if(session)
	{
		// found the session
		if(!session->mHandle.empty())
		{
			sessionTextDisconnectSendMessage(session);
		}
	}	
	else
	{
		LL_DEBUGS("Voice") << "Session not found for participant ID " << uuid << LL_ENDL;
	}
}
bool LLVivoxVoiceClient::isValidChannel(std::string &sessionHandle)
{
  return(findSession(sessionHandle) != NULL);
	
}
bool LLVivoxVoiceClient::answerInvite(std::string &sessionHandle)
{
	// this is only ever used to answer incoming p2p call invites.
	
	sessionState *session = findSession(sessionHandle);
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

BOOL LLVivoxVoiceClient::isOnlineSIP(const LLUUID &id)
{
	bool result = false;
	buddyListEntry *buddy = findBuddy(id);
	if(buddy)
	{
		result = buddy->mOnlineSLim;
		LL_DEBUGS("Voice") << "Buddy " << buddy->mDisplayName << " is SIP " << (result?"online":"offline") << LL_ENDL;
	}

	if(!result)
	{
		// This user isn't on the buddy list or doesn't show online status through the buddy list, but could be a participant in an existing session if they initiated a text IM.
		sessionState *session = findSession(id);
		if(session && !session->mHandle.empty())
		{
			if((session->mTextStreamState != streamStateUnknown) || (session->mMediaStreamState > streamStateIdle))
			{
				LL_DEBUGS("Voice") << "Open session with " << id << " found, returning SIP online state" << LL_ENDL;
				// we have a p2p text session open with this user, so by definition they're online.
				result = true;
			}
		}
	}
	
	return result;
}

bool LLVivoxVoiceClient::isVoiceWorking() const
{
  //Added stateSessionTerminated state to avoid problems with call in parcels with disabled voice (EXT-4758)
  // Condition with joining spatial num was added to take into account possible problems with connection to voice
  // server(EXT-4313). See bug descriptions and comments for MAX_NORMAL_JOINING_SPATIAL_NUM for more info.
  return (mSpatialJoiningNum < MAX_NORMAL_JOINING_SPATIAL_NUM) && (stateLoggedIn <= mState) && (mState <= stateSessionTerminated);
}

// Returns true if the indicated participant in the current audio session is really an SL avatar.
// Currently this will be false only for PSTN callers into group chats, and PSTN p2p calls.
BOOL LLVivoxVoiceClient::isParticipantAvatar(const LLUUID &id)
{
	BOOL result = TRUE; 
	sessionState *session = findSession(id);
	
	if(session != NULL)
	{
		// this is a p2p session with the indicated caller, or the session with the specified UUID.
		if(session->mSynthesizedCallerID)
			result = FALSE;
	}
	else
	{
		// Didn't find a matching session -- check the current audio session for a matching participant
		if(mAudioSession != NULL)
		{
			participantState *participant = findParticipantByID(id);
			if(participant != NULL)
			{
				result = participant->isAvatar();
			}
		}
	}
	
	return result;
}

// Returns true if calling back the session URI after the session has closed is possible.
// Currently this will be false only for PSTN P2P calls.		
BOOL LLVivoxVoiceClient::isSessionCallBackPossible(const LLUUID &session_id)
{
	BOOL result = TRUE; 
	sessionState *session = findSession(session_id);
	
	if(session != NULL)
	{
		result = session->isCallBackPossible();
	}
	
	return result;
}

// Returns true if the session can accepte text IM's.
// Currently this will be false only for PSTN P2P calls.
BOOL LLVivoxVoiceClient::isSessionTextIMPossible(const LLUUID &session_id)
{
	bool result = TRUE; 
	sessionState *session = findSession(session_id);
	
	if(session != NULL)
	{
		result = session->isTextIMPossible();
	}
	
	return result;
}
		

void LLVivoxVoiceClient::declineInvite(std::string &sessionHandle)
{
	sessionState *session = findSession(sessionHandle);
	if(session)
	{
		sessionMediaDisconnectSendMessage(session);
	}
}

void LLVivoxVoiceClient::leaveNonSpatialChannel()
{
	LL_DEBUGS("Voice") 
		<< "called in state " << state2string(getState()) 
		<< LL_ENDL;
	
	// Make sure we don't rejoin the current session.	
	sessionState *oldNextSession = mNextAudioSession;
	mNextAudioSession = NULL;
	
	// Most likely this will still be the current session at this point, but check it anyway.
	reapSession(oldNextSession);
	
	verifySessionState();
	
	sessionTerminate();
}

std::string LLVivoxVoiceClient::getCurrentChannel()
{
	std::string result;
	
	if((getState() == stateRunning) && !mSessionTerminateRequested)
	{
		result = getAudioSessionURI();
	}
	
	return result;
}

bool LLVivoxVoiceClient::inProximalChannel()
{
	bool result = false;
	
	if((getState() == stateRunning) && !mSessionTerminateRequested)
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
	
	// If you need to transform a GUID to this form on the Mac OS X command line, this will do so:
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
		result = mAudioSession->mIsSpatial;
		
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
		rot = gAgentAvatarp->getRootJoint()->getWorldRotation().getMatrix3();
		pos = gAgentAvatarp->getPositionGlobal();

		// TODO: Can we get the head offset from outside the LLVOAvatar?
		//			pos += LLVector3d(mHeadOffset);
		pos += LLVector3d(0.f, 0.f, 1.f);
		
		LLVivoxVoiceClient::getInstance()->setAvatarPosition(
															 pos,				// position
															 LLVector3::zero, 	// velocity
															 rot);				// rotation matrix
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

void LLVivoxVoiceClient::setAvatarPosition(const LLVector3d &position, const LLVector3 &velocity, const LLMatrix3 &rot)
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
	
	if(mAvatarRot != rot)
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
	if(getState() == stateRunning)
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
	if (enabled != mVoiceEnabled)
	{
		// TODO: Refactor this so we don't call into LLVoiceChannel, but simply
		// use the status observer
		mVoiceEnabled = enabled;
		LLVoiceClientStatusObserver::EStatusType status;
		
		
		if (enabled)
		{
			LLVoiceChannel::getCurrentVoiceChannel()->activate();
			status = LLVoiceClientStatusObserver::STATUS_VOICE_ENABLED;
		}
		else
		{
			// Turning voice off looses your current channel -- this makes sure the UI isn't out of sync when you re-enable it.
			LLVoiceChannel::getCurrentVoiceChannel()->deactivate();
			status = LLVoiceClientStatusObserver::STATUS_VOICE_DISABLED;
		}
		notifyStatusObservers(status);		
	}
}

bool LLVivoxVoiceClient::voiceEnabled()
{
	return gSavedSettings.getBOOL("EnableVoiceChat") && !gSavedSettings.getBOOL("CmdLineDisableVoice");
}

void LLVivoxVoiceClient::setLipSyncEnabled(BOOL enabled)
{
	mLipSyncEnabled = enabled;
}

BOOL LLVivoxVoiceClient::lipSyncEnabled()
{
	   
	if ( mVoiceEnabled && stateDisabled != getState() )
	{
		return mLipSyncEnabled;
	}
	else
	{
		return FALSE;
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
BOOL LLVivoxVoiceClient::getVoiceEnabled(const LLUUID& id)
{
	BOOL result = FALSE;
	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		// I'm not sure what the semantics of this should be.
		// For now, if we have any data about the user that came through the chat channel, assume they're voice-enabled.
		result = TRUE;
	}
	
	return result;
}

std::string LLVivoxVoiceClient::getDisplayName(const LLUUID& id)
{
	std::string result;
	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		result = participant->mDisplayName;
	}
	
	return result;
}



BOOL LLVivoxVoiceClient::getIsSpeaking(const LLUUID& id)
{
	BOOL result = FALSE;

	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		if (participant->mSpeakingTimeout.getElapsedTimeF32() > SPEAKING_TIMEOUT)
		{
			participant->mIsSpeaking = FALSE;
		}
		result = participant->mIsSpeaking;
	}
	
	return result;
}

BOOL LLVivoxVoiceClient::getIsModeratorMuted(const LLUUID& id)
{
	BOOL result = FALSE;

	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		result = participant->mIsModeratorMuted;
	}
	
	return result;
}

F32 LLVivoxVoiceClient::getCurrentPower(const LLUUID& id)
{		
	F32 result = 0;
	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		result = participant->mPower;
	}
	
	return result;
}



BOOL LLVivoxVoiceClient::getUsingPTT(const LLUUID& id)
{
	BOOL result = FALSE;

	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		// I'm not sure what the semantics of this should be.
		// Does "using PTT" mean they're configured with a push-to-talk button?
		// For now, we know there's no PTT mechanism in place, so nobody is using it.
	}
	
	return result;
}

BOOL LLVivoxVoiceClient::getOnMuteList(const LLUUID& id)
{
	BOOL result = FALSE;
	
	participantState *participant = findParticipantByID(id);
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
	
        participantState *participant = findParticipantByID(id);
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
		participantState *participant = findParticipantByID(id);
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

	participantState *participant = findParticipantByID(id);
	if(participant)
	{
		result = participant->mGroupID;
	}
	
	return result;
}

BOOL LLVivoxVoiceClient::getAreaVoiceDisabled()
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

LLVivoxVoiceClient::sessionState::sessionState() :
        mErrorStatusCode(0),
	mMediaStreamState(streamStateUnknown),
	mTextStreamState(streamStateUnknown),
	mCreateInProgress(false),
	mMediaConnectInProgress(false),
	mVoiceInvitePending(false),
	mTextInvitePending(false),
	mSynthesizedCallerID(false),
	mIsChannel(false),
	mIsSpatial(false),
	mIsP2P(false),
	mIncoming(false),
	mVoiceEnabled(false),
	mReconnect(false),
	mVolumeDirty(false),
	mMuteDirty(false),
	mParticipantsChanged(false)
{
}

LLVivoxVoiceClient::sessionState::~sessionState()
{
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


LLVivoxVoiceClient::sessionIterator LLVivoxVoiceClient::sessionsBegin(void)
{
	return mSessions.begin();
}

LLVivoxVoiceClient::sessionIterator LLVivoxVoiceClient::sessionsEnd(void)
{
	return mSessions.end();
}


LLVivoxVoiceClient::sessionState *LLVivoxVoiceClient::findSession(const std::string &handle)
{
	sessionState *result = NULL;
	sessionMap::iterator iter = mSessionsByHandle.find(handle);
	if(iter != mSessionsByHandle.end())
	{
		result = iter->second;
	}
	
	return result;
}

LLVivoxVoiceClient::sessionState *LLVivoxVoiceClient::findSessionBeingCreatedByURI(const std::string &uri)
{	
	sessionState *result = NULL;
	for(sessionIterator iter = sessionsBegin(); iter != sessionsEnd(); iter++)
	{
		sessionState *session = *iter;
		if(session->mCreateInProgress && (session->mSIPURI == uri))
		{
			result = session;
			break;
		}
	}
	
	return result;
}

LLVivoxVoiceClient::sessionState *LLVivoxVoiceClient::findSession(const LLUUID &participant_id)
{
	sessionState *result = NULL;
	
	for(sessionIterator iter = sessionsBegin(); iter != sessionsEnd(); iter++)
	{
		sessionState *session = *iter;
		if((session->mCallerID == participant_id) || (session->mIMSessionID == participant_id))
		{
			result = session;
			break;
		}
	}
	
	return result;
}

LLVivoxVoiceClient::sessionState *LLVivoxVoiceClient::addSession(const std::string &uri, const std::string &handle)
{
	sessionState *result = NULL;
	
	if(handle.empty())
	{
		// No handle supplied.
		// Check whether there's already a session with this URI
		for(sessionIterator iter = sessionsBegin(); iter != sessionsEnd(); iter++)
		{
			sessionState *s = *iter;
			if((s->mSIPURI == uri) || (s->mAlternateSIPURI == uri))
			{
				// TODO: I need to think about this logic... it's possible that this case should raise an internal error.
				result = s;
				break;
			}
		}
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
		
		LL_DEBUGS("Voice") << "adding new session: handle " << handle << " URI " << uri << LL_ENDL;
		result = new sessionState();
		result->mSIPURI = uri;
		result->mHandle = handle;

		if (LLVoiceClient::instance().getVoiceEffectEnabled())
		{
			result->mVoiceFontID = LLVoiceClient::instance().getVoiceEffectDefault();
		}

		mSessions.insert(result);

		if(!result->mHandle.empty())
		{
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

void LLVivoxVoiceClient::setSessionHandle(sessionState *session, const std::string &handle)
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
				LL_ERRS("Voice") << "Internal error: session mismatch!" << LL_ENDL;
			}

			mSessionsByHandle.erase(iter);
		}
		else
		{
			LL_ERRS("Voice") << "Internal error: session handle not found in map!" << LL_ENDL;
		}
	}
			
	session->mHandle = handle;

	if(!handle.empty())
	{
		mSessionsByHandle.insert(sessionMap::value_type(session->mHandle, session));
	}

	verifySessionState();
}

void LLVivoxVoiceClient::setSessionURI(sessionState *session, const std::string &uri)
{
	// There used to be a map of session URIs to sessions, which made this complex....
	session->mSIPURI = uri;

	verifySessionState();
}

void LLVivoxVoiceClient::deleteSession(sessionState *session)
{
	// Remove the session from the handle map
	if(!session->mHandle.empty())
	{
		sessionMap::iterator iter = mSessionsByHandle.find(session->mHandle);
		if(iter != mSessionsByHandle.end())
		{
			if(iter->second != session)
			{
				LL_ERRS("Voice") << "Internal error: session mismatch" << LL_ENDL;
			}
			mSessionsByHandle.erase(iter);
		}
	}

	// Remove the session from the URI map
	mSessions.erase(session);
	
	// At this point, the session should be unhooked from all lists and all state should be consistent.
	verifySessionState();

	// If this is the current audio session, clean up the pointer which will soon be dangling.
	if(mAudioSession == session)
	{
		mAudioSession = NULL;
		mAudioSessionChanged = true;
	}

	// ditto for the next audio session
	if(mNextAudioSession == session)
	{
		mNextAudioSession = NULL;
	}

	// delete the session
	delete session;
}

void LLVivoxVoiceClient::deleteAllSessions()
{
	LL_DEBUGS("Voice") << "called" << LL_ENDL;

	while(!mSessions.empty())
	{
		deleteSession(*(sessionsBegin()));
	}
	
	if(!mSessionsByHandle.empty())
	{
		LL_ERRS("Voice") << "Internal error: empty session map, non-empty handle map" << LL_ENDL;
	}
}

void LLVivoxVoiceClient::verifySessionState(void)
{
	// This is mostly intended for debugging problems with session state management.
	LL_DEBUGS("Voice") << "Total session count: " << mSessions.size() << " , session handle map size: " << mSessionsByHandle.size() << LL_ENDL;

	for(sessionIterator iter = sessionsBegin(); iter != sessionsEnd(); iter++)
	{
		sessionState *session = *iter;

		LL_DEBUGS("Voice") << "session " << session << ": handle " << session->mHandle << ", URI " << session->mSIPURI << LL_ENDL;
		
		if(!session->mHandle.empty())
		{
			// every session with a non-empty handle needs to be in the handle map
			sessionMap::iterator i2 = mSessionsByHandle.find(session->mHandle);
			if(i2 == mSessionsByHandle.end())
			{
				LL_ERRS("Voice") << "internal error (handle " << session->mHandle << " not found in session map)" << LL_ENDL;
			}
			else
			{
				if(i2->second != session)
				{
					LL_ERRS("Voice") << "internal error (handle " << session->mHandle << " in session map points to another session)" << LL_ENDL;
				}
			}
		}
	}
		
	// check that every entry in the handle map points to a valid session in the session set
	for(sessionMap::iterator iter = mSessionsByHandle.begin(); iter != mSessionsByHandle.end(); iter++)
	{
		sessionState *session = iter->second;
		sessionIterator i2 = mSessions.find(session);
		if(i2 == mSessions.end())
		{
			LL_ERRS("Voice") << "internal error (session for handle " << session->mHandle << " not found in session map)" << LL_ENDL;
		}
		else
		{
			if(session->mHandle != (*i2)->mHandle)
			{
				LL_ERRS("Voice") << "internal error (session for handle " << session->mHandle << " points to session with different handle " << (*i2)->mHandle << ")" << LL_ENDL;
			}
		}
	}
}

LLVivoxVoiceClient::buddyListEntry::buddyListEntry(const std::string &uri) :
	mURI(uri)
{
	mOnlineSL = false;
	mOnlineSLim = false;
	mCanSeeMeOnline = true;
	mHasBlockListEntry = false;
	mHasAutoAcceptListEntry = false;
	mNameResolved = false;
	mInVivoxBuddies = false;
	mInSLFriends = false;
	mNeedsNameUpdate = false;
}

void LLVivoxVoiceClient::processBuddyListEntry(const std::string &uri, const std::string &displayName)
{
	buddyListEntry *buddy = addBuddy(uri, displayName);
	buddy->mInVivoxBuddies = true;	
}

LLVivoxVoiceClient::buddyListEntry *LLVivoxVoiceClient::addBuddy(const std::string &uri)
{
	std::string empty;
	buddyListEntry *buddy = addBuddy(uri, empty);
	if(buddy->mDisplayName.empty())
	{
		buddy->mNameResolved = false;
	}
	return buddy;
}

LLVivoxVoiceClient::buddyListEntry *LLVivoxVoiceClient::addBuddy(const std::string &uri, const std::string &displayName)
{
	buddyListEntry *result = NULL;
	buddyListMap::iterator iter = mBuddyListMap.find(uri);
	
	if(iter != mBuddyListMap.end())
	{
		// Found a matching buddy already in the map.
		LL_DEBUGS("Voice") << "adding existing buddy " << uri << LL_ENDL;
		result = iter->second;
	}

	if(!result)
	{
		// participant isn't already in one list or the other.
		LL_DEBUGS("Voice") << "adding new buddy " << uri << LL_ENDL;
		result = new buddyListEntry(uri);
		result->mDisplayName = displayName;

		if(IDFromName(uri, result->mUUID)) 
		{
			// Extracted UUID from name successfully.
		}
		else
		{
			LL_DEBUGS("Voice") << "Couldn't find ID for buddy " << uri << " (\"" << displayName << "\")" << LL_ENDL;
		}

		mBuddyListMap.insert(buddyListMap::value_type(result->mURI, result));
	}
	
	return result;
}

LLVivoxVoiceClient::buddyListEntry *LLVivoxVoiceClient::findBuddy(const std::string &uri)
{
	buddyListEntry *result = NULL;
	buddyListMap::iterator iter = mBuddyListMap.find(uri);
	if(iter != mBuddyListMap.end())
	{
		result = iter->second;
	}
	
	return result;
}

LLVivoxVoiceClient::buddyListEntry *LLVivoxVoiceClient::findBuddy(const LLUUID &id)
{
	buddyListEntry *result = NULL;
	buddyListMap::iterator iter;

	for(iter = mBuddyListMap.begin(); iter != mBuddyListMap.end(); iter++)
	{
		if(iter->second->mUUID == id)
		{
			result = iter->second;
			break;
		}
	}
	
	return result;
}

LLVivoxVoiceClient::buddyListEntry *LLVivoxVoiceClient::findBuddyByDisplayName(const std::string &name)
{
	buddyListEntry *result = NULL;
	buddyListMap::iterator iter;

	for(iter = mBuddyListMap.begin(); iter != mBuddyListMap.end(); iter++)
	{
		if(iter->second->mDisplayName == name)
		{
			result = iter->second;
			break;
		}
	}
	
	return result;
}

void LLVivoxVoiceClient::deleteBuddy(const std::string &uri)
{
	buddyListMap::iterator iter = mBuddyListMap.find(uri);
	if(iter != mBuddyListMap.end())
	{
		LL_DEBUGS("Voice") << "deleting buddy " << uri << LL_ENDL;
		buddyListEntry *buddy = iter->second;
		mBuddyListMap.erase(iter);
		delete buddy;
	}
	else
	{
		LL_DEBUGS("Voice") << "attempt to delete nonexistent buddy " << uri << LL_ENDL;
	}
	
}

void LLVivoxVoiceClient::deleteAllBuddies(void)
{
	while(!mBuddyListMap.empty())
	{
		deleteBuddy(mBuddyListMap.begin()->first);
	}
	
	// Don't want to correlate with friends list when we've emptied the buddy list.
	mBuddyListMapPopulated = false;
	
	// Don't want to correlate with friends list when we've reset the block rules.
	mBlockRulesListReceived = false;
	mAutoAcceptRulesListReceived = false;
}

void LLVivoxVoiceClient::deleteAllBlockRules(void)
{
	// Clear the block list entry flags from all local buddy list entries
	buddyListMap::iterator buddy_it;
	for(buddy_it = mBuddyListMap.begin(); buddy_it != mBuddyListMap.end(); buddy_it++)
	{
		buddy_it->second->mHasBlockListEntry = false;
	}
}

void LLVivoxVoiceClient::deleteAllAutoAcceptRules(void)
{
	// Clear the auto-accept list entry flags from all local buddy list entries
	buddyListMap::iterator buddy_it;
	for(buddy_it = mBuddyListMap.begin(); buddy_it != mBuddyListMap.end(); buddy_it++)
	{
		buddy_it->second->mHasAutoAcceptListEntry = false;
	}
}

void LLVivoxVoiceClient::addBlockRule(const std::string &blockMask, const std::string &presenceOnly)
{
	buddyListEntry *buddy = NULL;

	// blockMask is the SIP URI of a friends list entry
	buddyListMap::iterator iter = mBuddyListMap.find(blockMask);
	if(iter != mBuddyListMap.end())
	{
		LL_DEBUGS("Voice") << "block list entry for " << blockMask << LL_ENDL;
		buddy = iter->second;
	}

	if(buddy == NULL)
	{
		LL_DEBUGS("Voice") << "block list entry for unknown buddy " << blockMask << LL_ENDL;
		buddy = addBuddy(blockMask);
	}
	
	if(buddy != NULL)
	{
		buddy->mHasBlockListEntry = true;
	}
}

void LLVivoxVoiceClient::addAutoAcceptRule(const std::string &autoAcceptMask, const std::string &autoAddAsBuddy)
{
	buddyListEntry *buddy = NULL;

	// blockMask is the SIP URI of a friends list entry
	buddyListMap::iterator iter = mBuddyListMap.find(autoAcceptMask);
	if(iter != mBuddyListMap.end())
	{
		LL_DEBUGS("Voice") << "auto-accept list entry for " << autoAcceptMask << LL_ENDL;
		buddy = iter->second;
	}

	if(buddy == NULL)
	{
		LL_DEBUGS("Voice") << "auto-accept list entry for unknown buddy " << autoAcceptMask << LL_ENDL;
		buddy = addBuddy(autoAcceptMask);
	}

	if(buddy != NULL)
	{
		buddy->mHasAutoAcceptListEntry = true;
	}
}

void LLVivoxVoiceClient::accountListBlockRulesResponse(int statusCode, const std::string &statusString)
{
	// Block list entries were updated via addBlockRule() during parsing.  Just flag that we're done.
	mBlockRulesListReceived = true;
}

void LLVivoxVoiceClient::accountListAutoAcceptRulesResponse(int statusCode, const std::string &statusString)
{
	// Block list entries were updated via addBlockRule() during parsing.  Just flag that we're done.
	mAutoAcceptRulesListReceived = true;
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
				case 404:	// NOT_FOUND
				case 480:	// TEMPORARILY_UNAVAILABLE
				case 408:	// REQUEST_TIMEOUT
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
		<< (inSpatialChannel()?", proximal is true":", proximal is false")
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
		&& status != LLVoiceClientStatusObserver::STATUS_LEFT_CHANNEL)
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
	LLAvatarNameCache::get(id,
		boost::bind(&LLVivoxVoiceClient::onAvatarNameCache,
			this, _1, _2));
}

void LLVivoxVoiceClient::onAvatarNameCache(const LLUUID& agent_id,
										   const LLAvatarName& av_name)
{
	// For Vivox, we use the legacy name because I'm uncertain whether or
	// not their service can tolerate switching to Username or Display Name
	std::string legacy_name = av_name.getLegacyName();
	avatarNameResolved(agent_id, legacy_name);	
}

void LLVivoxVoiceClient::avatarNameResolved(const LLUUID &id, const std::string &name)
{
	// If the avatar whose name just resolved is on our friends list, resync the friends list.
	if(LLAvatarTracker::instance().getBuddyInfo(id) != NULL)
	{
		mFriendsListDirty = true;
	}
	// Iterate over all sessions.
	for(sessionIterator iter = sessionsBegin(); iter != sessionsEnd(); iter++)
	{
		sessionState *session = *iter;
		// Check for this user as a participant in this session
		participantState *participant = session->findParticipantByID(id);
		if(participant)
		{
			// Found -- fill in the name
			participant->mAccountName = name;
			// and post a "participants updated" message to listeners later.
			session->mParticipantsChanged = true;
		}
		
		// Check whether this is a p2p session whose caller name just resolved
		if(session->mCallerID == id)
		{
			// this session's "caller ID" just resolved.  Fill in the name.
			session->mName = name;
			if(session->mTextInvitePending)
			{
				session->mTextInvitePending = false;

				// We don't need to call LLIMMgr::getInstance()->addP2PSession() here.  The first incoming message will create the panel.				
			}
			if(session->mVoiceInvitePending)
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
		LL_DEBUGS("Voice") << "Expired " << (template_font ? "Template " : "")
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

		LL_DEBUGS("Voice") << (template_font ? "Template " : "")
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
			LL_DEBUGS("Voice") << "Unknown voice font type: " << font_type << LL_ENDL;
		}
		if (font_status < VOICE_FONT_STATUS_NONE || font_status >= VOICE_FONT_STATUS_UNKNOWN)
		{
			LL_DEBUGS("Voice") << "Unknown voice font status: " << font_status << LL_ENDL;
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
			LL_DEBUGS("Voice") << "Voice Font " << voice_font->mName << " will expire soon." << LL_ENDL;
			will_expire = true;
			warning_timer.stop();
		}
	}

	LLSD args;
	args["URL"] = LLTrans::getString("voice_morphing_url");

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
		S32 seconds = gSavedSettings.getS32("VoiceEffectExpiryWarningTime");
		args["INTERVAL"] = llformat("%d", seconds / SEC_PER_DAY);

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
			LL_DEBUGS("Voice") << "Removing " << id << " from the voice font list." << LL_ENDL;
			mVoiceFontList.erase(list_iter++);
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
			LL_DEBUGS("Voice") << "Selected voice font " << id << " is not available." << LL_ENDL;
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
			LL_DEBUGS("Voice") << "Selected voice font template " << id << " is not available." << LL_ENDL;
		}
	}
	return result;
}

void LLVivoxVoiceClient::accountGetSessionFontsSendMessage()
{
	if(!mAccountHandle.empty())
	{
		std::ostringstream stream;

		LL_DEBUGS("Voice") << "Requesting voice font list." << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.GetSessionFonts.1\">"
		<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLVivoxVoiceClient::accountGetTemplateFontsSendMessage()
{
	if(!mAccountHandle.empty())
	{
		std::ostringstream stream;

		LL_DEBUGS("Voice") << "Requesting voice font template list." << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Account.GetTemplateFonts.1\">"
		<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLVivoxVoiceClient::sessionSetVoiceFontSendMessage(sessionState *session)
{
	S32 font_index = getVoiceFontIndex(session->mVoiceFontID);
	LL_DEBUGS("Voice") << "Requesting voice font: " << session->mVoiceFontID << " (" << font_index << "), session handle: " << session->mHandle << LL_ENDL;

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
	// Voice font list entries were updated via addVoiceFont() during parsing.
	if(getState() == stateVoiceFontsWait)
	{
		setState(stateVoiceFontsReceived);
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

void LLVivoxVoiceClient::notifyVoiceFontObservers()
{
	LL_DEBUGS("Voice") << "Notifying voice effect observers. Lists changed: " << mVoiceFontListDirty << LL_ENDL;

	for (voice_font_observer_set_t::iterator it = mVoiceFontObservers.begin();
		 it != mVoiceFontObservers.end();
		 )
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
		if(mVoiceFontsReceived)
		{
			LLNotificationsUtil::add("VoiceEffectsNew");
		}
		mVoiceFontsNew = false;
	}
}

void LLVivoxVoiceClient::enablePreviewBuffer(bool enable)
{
	mCaptureBufferMode = enable;
	if(mCaptureBufferMode && getState() >= stateNoChannel)
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
}

void LLVivoxVoiceClient::stopPreviewBuffer()
{
	mCaptureBufferRecording = false;
	mCaptureBufferPlaying = false;
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
{	if(!mAccountHandle.empty())
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
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>false</Value>"
		<< "</Request>\n\n\n";

		// Dirty the mute mic state so that it will get reset when we finishing previewing
		mMuteMicDirty = true;

		writeString(stream.str());
	}
}

void LLVivoxVoiceClient::captureBufferRecordStopSendMessage()
{
	if(!mAccountHandle.empty())
	{
		std::ostringstream stream;

		LL_DEBUGS("Voice") << "Stopping audio capture to buffer." << LL_ENDL;

		// Mute the mic. Mic mute state was dirtied at recording start, so will be reset when finished previewing.
		stream << "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Connector.MuteLocalMic.1\">"
			<< "<ConnectorHandle>" << mConnectorHandle << "</ConnectorHandle>"
			<< "<Value>true</Value>"
		<< "</Request>\n\n\n";

		// Stop capture
		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.CaptureAudioStop.1\">"
			<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLVivoxVoiceClient::captureBufferPlayStartSendMessage(const LLUUID& voice_font_id)
{
	if(!mAccountHandle.empty())
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
			<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
			<< "<TemplateFontID>" << font_index << "</TemplateFontID>"
			<< "<FontDelta />"
		<< "</Request>"
		<< "\n\n\n";

		writeString(stream.str());
	}
}

void LLVivoxVoiceClient::captureBufferPlayStopSendMessage()
{
	if(!mAccountHandle.empty())
	{
		std::ostringstream stream;

		LL_DEBUGS("Voice") << "Stopping audio buffer playback." << LL_ENDL;

		stream
		<< "<Request requestId=\"" << mCommandCookie++ << "\" action=\"Aux.RenderAudioStop.1\">"
			<< "<AccountHandle>" << mAccountHandle << "</AccountHandle>"
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
	squelchDebugOutput = false;
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

static LLFastTimer::DeclareTimer FTM_VIVOX_PROCESS("Vivox Process");

// virtual
LLIOPipe::EStatus LLVivoxProtocolParser::process_impl(
													  const LLChannelDescriptors& channels,
													  buffer_ptr_t& buffer,
													  bool& eos,
													  LLSD& context,
													  LLPumpIO* pump)
{
	LLFastTimer t(FTM_VIVOX_PROCESS);
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
		
		// If this message isn't set to be squelched, output the raw XML received.
		if(!squelchDebugOutput)
		{
			LL_DEBUGS("Voice") << "parsing: " << mInput.substr(start, delim - start) << LL_ENDL;
		}
		
		start = delim + 3;
	}
	
	if(start != 0)
		mInput = mInput.substr(start);
	
	LL_DEBUGS("VivoxProtocolParser") << "at end, mInput is: " << mInput << LL_ENDL;
	
	if(!LLVivoxVoiceClient::getInstance()->mConnected)
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
			else if (!stricmp("Buddies", tag))
			{
				LLVivoxVoiceClient::getInstance()->deleteAllBuddies();
			}
			else if (!stricmp("BlockRules", tag))
			{
				LLVivoxVoiceClient::getInstance()->deleteAllBlockRules();
			}
			else if (!stricmp("AutoAcceptRules", tag))
			{
				LLVivoxVoiceClient::getInstance()->deleteAllAutoAcceptRules();
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
		else if (!stricmp("CaptureDevice", tag))
		{
			LLVivoxVoiceClient::getInstance()->addCaptureDevice(deviceString);
		}
		else if (!stricmp("RenderDevice", tag))
		{
			LLVivoxVoiceClient::getInstance()->addRenderDevice(deviceString);
		}
		else if (!stricmp("Buddy", tag))
		{
			LLVivoxVoiceClient::getInstance()->processBuddyListEntry(uriString, displayNameString);
		}
		else if (!stricmp("BlockRule", tag))
		{
			LLVivoxVoiceClient::getInstance()->addBlockRule(blockMask, presenceOnly);
		}
		else if (!stricmp("BlockMask", tag))
			blockMask = string;
		else if (!stricmp("PresenceOnly", tag))
			presenceOnly = string;
		else if (!stricmp("AutoAcceptRule", tag))
		{
			LLVivoxVoiceClient::getInstance()->addAutoAcceptRule(autoAcceptMask, autoAddAsBuddy);
		}
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
		if (!stricmp(eventTypeCstr, "AccountLoginStateChangeEvent"))
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
		else if (!stricmp(eventTypeCstr, "TextStreamUpdatedEvent"))
		{
			/*
			 <Event type="TextStreamUpdatedEvent">
			 <SessionGroupHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==_sg1</SessionGroupHandle>
			 <SessionHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==1</SessionHandle>
			 <Enabled>true</Enabled>
			 <State>1</State>
			 <Incoming>true</Incoming>
			 </Event>
			 */
			LLVivoxVoiceClient::getInstance()->textStreamUpdatedEvent(sessionHandle, sessionGroupHandle, enabled, state, incoming);
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
			LLVivoxVoiceClient::getInstance()->participantRemovedEvent(sessionHandle, sessionGroupHandle, uriString, alias, nameString);
		}
		else if (!stricmp(eventTypeCstr, "ParticipantUpdatedEvent"))
		{
			/*
			 <Event type="ParticipantUpdatedEvent">
			 <SessionGroupHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==_sg0</SessionGroupHandle>
			 <SessionHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==0</SessionHandle>
			 <ParticipantUri>sip:xFnPP04IpREWNkuw1cOXlhw==@bhr.vivox.com</ParticipantUri>
			 <IsModeratorMuted>false</IsModeratorMuted>
			 <IsSpeaking>true</IsSpeaking>
			 <Volume>44</Volume>
			 <Energy>0.0879437</Energy>
			 </Event>
			 */
			
			// These happen so often that logging them is pretty useless.
			squelchDebugOutput = true;
			
			LLVivoxVoiceClient::getInstance()->participantUpdatedEvent(sessionHandle, sessionGroupHandle, uriString, alias, isModeratorMuted, isSpeaking, volume, energy);
		}
		else if (!stricmp(eventTypeCstr, "AuxAudioPropertiesEvent"))
		{
			// These are really spammy in tuning mode
			squelchDebugOutput = true;

			LLVivoxVoiceClient::getInstance()->auxAudioPropertiesEvent(energy);
		}
		else if (!stricmp(eventTypeCstr, "BuddyPresenceEvent"))
		{
			LLVivoxVoiceClient::getInstance()->buddyPresenceEvent(uriString, alias, statusString, applicationString);
		}
		else if (!stricmp(eventTypeCstr, "BuddyAndGroupListChangedEvent"))
		{
			// The buddy list was updated during parsing.
			// Need to recheck against the friends list.
			LLVivoxVoiceClient::getInstance()->buddyListChanged();
		}
		else if (!stricmp(eventTypeCstr, "BuddyChangedEvent"))
		{
			/*
			 <Event type="BuddyChangedEvent">
			 <AccountHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==</AccountHandle>
			 <BuddyURI>sip:x9fFHFZjOTN6OESF1DUPrZQ==@bhr.vivox.com</BuddyURI>
			 <DisplayName>Monroe Tester</DisplayName>
			 <BuddyData />
			 <GroupID>0</GroupID>
			 <ChangeType>Set</ChangeType>
			 </Event>
			 */		
			// TODO: Question: Do we need to process this at all?
		}
		else if (!stricmp(eventTypeCstr, "MessageEvent"))  
		{
			LLVivoxVoiceClient::getInstance()->messageEvent(sessionHandle, uriString, alias, messageHeader, messageBody, applicationString);
		}
		else if (!stricmp(eventTypeCstr, "SessionNotificationEvent"))  
		{
			LLVivoxVoiceClient::getInstance()->sessionNotificationEvent(sessionHandle, uriString, notificationType);
		}
		else if (!stricmp(eventTypeCstr, "SubscriptionEvent"))  
		{
			LLVivoxVoiceClient::getInstance()->subscriptionEvent(uriString, subscriptionHandle, alias, displayNameString, applicationString, subscriptionType);
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
			/*
			 <Event type="SessionGroupRemovedEvent">
			 <SessionGroupHandle>c1_m1000xFnPP04IpREWNkuw1cOXlhw==_sg0</SessionGroupHandle>
			 </Event>
			 */
			// We don't need to process this, but we also shouldn't warn on it, since that confuses people.
		}
		else
		{
			LL_WARNS("VivoxProtocolParser") << "Unknown event type " << eventTypeString << LL_ENDL;
		}
	}
	else
	{
		const char *actionCstr = actionString.c_str();
		if (!stricmp(actionCstr, "Connector.Create.1"))
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
		else if (!stricmp(actionCstr, "Account.ListBlockRules.1"))
		{
			LLVivoxVoiceClient::getInstance()->accountListBlockRulesResponse(statusCode, statusString);						
		}
		else if (!stricmp(actionCstr, "Account.ListAutoAcceptRules.1"))
		{
			LLVivoxVoiceClient::getInstance()->accountListAutoAcceptRulesResponse(statusCode, statusString);						
		}
		else if (!stricmp(actionCstr, "Session.Set3DPosition.1"))
		{
			// We don't need to process these, but they're so spammy we don't want to log them.
			squelchDebugOutput = true;
		}
		else if (!stricmp(actionCstr, "Account.GetSessionFonts.1"))
		{
			LLVivoxVoiceClient::getInstance()->accountGetSessionFontsResponse(statusCode, statusString);
		}
		else if (!stricmp(actionCstr, "Account.GetTemplateFonts.1"))
		{
			LLVivoxVoiceClient::getInstance()->accountGetTemplateFontsResponse(statusCode, statusString);
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

