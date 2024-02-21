 /** 
 * @file llvoiceclient.cpp
 * @brief Voice client delegation class implementation.
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
#include "llvoiceclient.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llvoicevivox.h"
#include "llviewernetwork.h"
#include "llcommandhandler.h"
#include "llhttpnode.h"
#include "llnotificationsutil.h"
#include "llsdserialize.h"
#include "llui.h"
#include "llkeyboard.h"
#include "llagent.h"
#include "lluiusage.h"

const F32 LLVoiceClient::OVERDRIVEN_POWER_LEVEL = 0.7f;

const F32 LLVoiceClient::VOLUME_MIN = 0.f;
const F32 LLVoiceClient::VOLUME_DEFAULT = 0.5f;
const F32 LLVoiceClient::VOLUME_MAX = 1.0f;


// Support for secondlife:///app/voice SLapps
class LLVoiceHandler : public LLCommandHandler
{
public:
	// requests will be throttled from a non-trusted browser
	LLVoiceHandler() : LLCommandHandler("voice", UNTRUSTED_THROTTLE) {}

	bool handle(const LLSD& params, const LLSD& query_map, const std::string& grid, LLMediaCtrl* web)
	{
		if (params[0].asString() == "effects")
		{
			LLVoiceEffectInterface* effect_interface = LLVoiceClient::instance().getVoiceEffectInterface();
			// If the voice client doesn't support voice effects, we can't handle effects SLapps
			if (!effect_interface)
			{
				return false;
			}

			// Support secondlife:///app/voice/effects/refresh to update the voice effect list with new effects
			if (params[1].asString() == "refresh")
			{
				effect_interface->refreshVoiceEffectLists(false);
				return true;
			}
		}
		return false;
	}
};
LLVoiceHandler gVoiceHandler;



std::string LLVoiceClientStatusObserver::status2string(LLVoiceClientStatusObserver::EStatusType inStatus)
{
	std::string result = "UNTRANSLATED";
	
	// Prevent copy-paste errors when updating this list...
#define CASE(x)  case x:  result = #x;  break
	
	switch(inStatus)
	{
			CASE(STATUS_LOGIN_RETRY);
			CASE(STATUS_LOGGED_IN);
			CASE(STATUS_JOINING);
			CASE(STATUS_JOINED);
			CASE(STATUS_LEFT_CHANNEL);
			CASE(STATUS_VOICE_DISABLED);
			CASE(STATUS_VOICE_ENABLED);
			CASE(BEGIN_ERROR_STATUS);
			CASE(ERROR_CHANNEL_FULL);
			CASE(ERROR_CHANNEL_LOCKED);
			CASE(ERROR_NOT_AVAILABLE);
			CASE(ERROR_UNKNOWN);
		default:
            {
                std::ostringstream stream;
                stream << "UNKNOWN(" << (int)inStatus << ")";
                result = stream.str();
            }
			break;
	}
	
#undef CASE
	
	return result;
}



///////////////////////////////////////////////////////////////////////////////////////////////

LLVoiceClient::LLVoiceClient(LLPumpIO *pump)
	:
	mVoiceModule(NULL),
	m_servicePump(NULL),
	mVoiceEffectEnabled(LLCachedControl<bool>(gSavedSettings, "VoiceMorphingEnabled", true)),
	mVoiceEffectDefault(LLCachedControl<std::string>(gSavedPerAccountSettings, "VoiceEffectDefault", "00000000-0000-0000-0000-000000000000")),
	mPTTDirty(true),
	mPTT(true),
	mUsePTT(true),
	mPTTMouseButton(0),
	mPTTKey(0),
	mPTTIsToggle(false),
	mUserPTTState(false),
	mMuteMic(false),
	mDisableMic(false)
{
	updateSettings();
	init(pump);
}

//---------------------------------------------------
// Basic setup/shutdown

LLVoiceClient::~LLVoiceClient()
{
    llassert(!mVoiceModule);
}

void LLVoiceClient::init(LLPumpIO *pump)
{
	// Initialize all of the voice modules
	m_servicePump = pump;
}

void LLVoiceClient::userAuthorized(const std::string& user_id, const LLUUID &agentID)
{
	// In the future, we should change this to allow voice module registration
	// with a table lookup of sorts.
	std::string voice_server = gSavedSettings.getString("VoiceServerType");
	LL_DEBUGS("Voice") << "voice server type " << voice_server << LL_ENDL;
	if(voice_server == "vivox")
	{
		mVoiceModule = (LLVoiceModuleInterface *)LLVivoxVoiceClient::getInstance();
	}
	else
	{
		mVoiceModule = NULL;
		return; 
	}
	mVoiceModule->init(m_servicePump);	
	mVoiceModule->userAuthorized(user_id, agentID);
}

void LLVoiceClient::setHidden(bool hidden)
{
    if (mVoiceModule)
    {
        mVoiceModule->setHidden(hidden);
    }
}

void LLVoiceClient::terminate()
{
	if (mVoiceModule) mVoiceModule->terminate();
	mVoiceModule = NULL;
    m_servicePump = NULL;

    // Shutdown speaker volume storage before LLSingletonBase::deleteAll() does it
    if (LLSpeakerVolumeStorage::instanceExists())
    {
        LLSpeakerVolumeStorage::deleteSingleton();
    }
}

const LLVoiceVersionInfo LLVoiceClient::getVersion()
{
	if (mVoiceModule) 
	{
		return mVoiceModule->getVersion();
	}
	else
	{
		LLVoiceVersionInfo result;
		result.serverVersion = std::string();
		result.serverType = std::string();
		result.mBuildVersion = std::string();
		return result;
	}
}

void LLVoiceClient::updateSettings()
{
	setUsePTT(gSavedSettings.getBOOL("PTTCurrentlyEnabled"));
	setPTTIsToggle(gSavedSettings.getBOOL("PushToTalkToggle"));
	mDisableMic = gSavedSettings.getBOOL("VoiceDisableMic");

	updateMicMuteLogic();

	if (mVoiceModule)
    {
        mVoiceModule->updateSettings();
    }
}

//--------------------------------------------------
// tuning

void LLVoiceClient::tuningStart()
{
	if (mVoiceModule) mVoiceModule->tuningStart();
}

void LLVoiceClient::tuningStop()
{
	if (mVoiceModule) mVoiceModule->tuningStop();
}

bool LLVoiceClient::inTuningMode()
{
	if (mVoiceModule) 
	{
		return mVoiceModule->inTuningMode();
	}
	else
	{
		return false;
	}
}

void LLVoiceClient::tuningSetMicVolume(float volume)
{
	if (mVoiceModule) mVoiceModule->tuningSetMicVolume(volume);
}

void LLVoiceClient::tuningSetSpeakerVolume(float volume)
{
	if (mVoiceModule) mVoiceModule->tuningSetSpeakerVolume(volume);
}

float LLVoiceClient::tuningGetEnergy(void)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->tuningGetEnergy();
	}
	else
	{
		return 0.0;
	}
}


//------------------------------------------------
// devices

bool LLVoiceClient::deviceSettingsAvailable()
{
	if (mVoiceModule) 
	{
		return mVoiceModule->deviceSettingsAvailable();
	}
	else
	{
		return false;
	}
}

bool LLVoiceClient::deviceSettingsUpdated()
{
	if (mVoiceModule)
	{
		return mVoiceModule->deviceSettingsUpdated();
	}
	else
	{
		return false;
	}
}

void LLVoiceClient::refreshDeviceLists(bool clearCurrentList)
{
	if (mVoiceModule) mVoiceModule->refreshDeviceLists(clearCurrentList);
}

void LLVoiceClient::setCaptureDevice(const std::string& name)
{
	if (mVoiceModule) mVoiceModule->setCaptureDevice(name);
	
}

void LLVoiceClient::setRenderDevice(const std::string& name)
{
	if (mVoiceModule) mVoiceModule->setRenderDevice(name);	
}

const LLVoiceDeviceList& LLVoiceClient::getCaptureDevices()
{
	static LLVoiceDeviceList nullCaptureDevices;
	if (mVoiceModule) 
	{
		return mVoiceModule->getCaptureDevices();
	}
	else
	{
		return nullCaptureDevices;
	}
}


const LLVoiceDeviceList& LLVoiceClient::getRenderDevices()
{
	static LLVoiceDeviceList nullRenderDevices;	
	if (mVoiceModule) 
	{
		return mVoiceModule->getRenderDevices();
	}
	else
	{
		return nullRenderDevices;
	}
}


//--------------------------------------------------
// participants

void LLVoiceClient::getParticipantList(std::set<LLUUID> &participants)
{
	if (mVoiceModule) 
	{
	  mVoiceModule->getParticipantList(participants);
	}
	else
	{
	  participants = std::set<LLUUID>();
	}
}

bool LLVoiceClient::isParticipant(const LLUUID &speaker_id)
{
  if(mVoiceModule)
    {
      return mVoiceModule->isParticipant(speaker_id);
    }
  return false;
}


//--------------------------------------------------
// text chat


bool LLVoiceClient::isSessionTextIMPossible(const LLUUID& id)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->isSessionTextIMPossible(id);
	}
	else
	{
		return false;
	}	
}

bool LLVoiceClient::isSessionCallBackPossible(const LLUUID& id)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->isSessionCallBackPossible(id);
	}
	else
	{
		return false;
	}	
}

/* obsolete
bool LLVoiceClient::sendTextMessage(const LLUUID& participant_id, const std::string& message)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->sendTextMessage(participant_id, message);
	}
	else
	{
		return false;
	}	
}
*/

void LLVoiceClient::endUserIMSession(const LLUUID& participant_id)
{
	if (mVoiceModule) 
	{
		// mVoiceModule->endUserIMSession(participant_id);  // A SLim leftover
	}
}

//----------------------------------------------
// channels

bool LLVoiceClient::inProximalChannel()
{
	if (mVoiceModule) 
	{
		return mVoiceModule->inProximalChannel();
	}
	else
	{
		return false;
	}
}

void LLVoiceClient::setNonSpatialChannel(
	const std::string &uri,
	const std::string &credentials)
{
	if (mVoiceModule)
    {
        mVoiceModule->setNonSpatialChannel(uri, credentials);
    }
}

void LLVoiceClient::setSpatialChannel(
	const std::string &uri,
	const std::string &credentials)
{
	if (mVoiceModule)
    {
        mVoiceModule->setSpatialChannel(uri, credentials);
    }
}

void LLVoiceClient::leaveNonSpatialChannel()
{
	if (mVoiceModule)
    {
        mVoiceModule->leaveNonSpatialChannel();
    }
}

void LLVoiceClient::leaveChannel(void)
{
	if (mVoiceModule)
    {
        mVoiceModule->leaveChannel();
    }
}

std::string LLVoiceClient::getCurrentChannel()
{
	if (mVoiceModule) 
	{
		return mVoiceModule->getCurrentChannel();
	}
	else
	{
		return std::string();
	}
}


//---------------------------------------
// invitations

void LLVoiceClient::callUser(const LLUUID &uuid)
{
	if (mVoiceModule) mVoiceModule->callUser(uuid);
}

bool LLVoiceClient::isValidChannel(std::string &session_handle)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->isValidChannel(session_handle);
	}
	else
	{
		return false;
	}
}

bool LLVoiceClient::answerInvite(std::string &channelHandle)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->answerInvite(channelHandle);
	}
	else
	{
		return false;
	}
}

void LLVoiceClient::declineInvite(std::string &channelHandle)
{
	if (mVoiceModule) mVoiceModule->declineInvite(channelHandle);
}


//------------------------------------------
// Volume/gain


void LLVoiceClient::setVoiceVolume(F32 volume)
{
	if (mVoiceModule) mVoiceModule->setVoiceVolume(volume);
}

void LLVoiceClient::setMicGain(F32 volume)
{
	if (mVoiceModule) mVoiceModule->setMicGain(volume);
}


//------------------------------------------
// enable/disable voice features

bool LLVoiceClient::voiceEnabled()
{
	if (mVoiceModule) 
	{
		return mVoiceModule->voiceEnabled();
	}
	else
	{
		return false;
	}
}

void LLVoiceClient::setVoiceEnabled(bool enabled)
{
	if (mVoiceModule)
    {
        mVoiceModule->setVoiceEnabled(enabled);
    }
}

void LLVoiceClient::updateMicMuteLogic()
{
	// If not configured to use PTT, the mic should be open (otherwise the user will be unable to speak).
	bool new_mic_mute = false;
	
	if(mUsePTT)
	{
		// If configured to use PTT, track the user state.
		new_mic_mute = !mUserPTTState;
	}

	if(mMuteMic || mDisableMic)
	{
		// Either of these always overrides any other PTT setting.
		new_mic_mute = true;
	}
	
	if (mVoiceModule) mVoiceModule->setMuteMic(new_mic_mute);
}

void LLVoiceClient::setLipSyncEnabled(bool enabled)
{
	if (mVoiceModule) mVoiceModule->setLipSyncEnabled(enabled);
}

bool LLVoiceClient::lipSyncEnabled()
{
	if (mVoiceModule) 
	{
		return mVoiceModule->lipSyncEnabled();
	}
	else
	{
		return false;
	}
}

void LLVoiceClient::setMuteMic(bool muted)
{
	mMuteMic = muted;
	updateMicMuteLogic();
	mMicroChangedSignal();
}


// ----------------------------------------------
// PTT

void LLVoiceClient::setUserPTTState(bool ptt)
{
	if (ptt)
	{
		LLUIUsage::instance().logCommand("Agent.EnableMicrophone");
	}
	mUserPTTState = ptt;
	updateMicMuteLogic();
	mMicroChangedSignal();
}

bool LLVoiceClient::getUserPTTState()
{
	return mUserPTTState;
}

void LLVoiceClient::setUsePTT(bool usePTT)
{
	if(usePTT && !mUsePTT)
	{
		// When the user turns on PTT, reset the current state.
		mUserPTTState = false;
	}
	mUsePTT = usePTT;
	
	updateMicMuteLogic();
}

void LLVoiceClient::setPTTIsToggle(bool PTTIsToggle)
{
	if(!PTTIsToggle && mPTTIsToggle)
	{
		// When the user turns off toggle, reset the current state.
		mUserPTTState = false;
	}
	
	mPTTIsToggle = PTTIsToggle;

	updateMicMuteLogic();
}

bool LLVoiceClient::getPTTIsToggle()
{
	return mPTTIsToggle;
}

void LLVoiceClient::inputUserControlState(bool down)
{
	if(mPTTIsToggle)
	{
		if(down) // toggle open-mic state on 'down'                                                        
		{
			toggleUserPTTState();
		}
	}
	else // set open-mic state as an absolute                                                                  
	{
		setUserPTTState(down);
	}
}

void LLVoiceClient::toggleUserPTTState(void)
{
	setUserPTTState(!getUserPTTState());
}


//-------------------------------------------
// nearby speaker accessors

bool LLVoiceClient::getVoiceEnabled(const LLUUID& id)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->getVoiceEnabled(id);
	} 
	else
	{
		return false;
	}
}

std::string LLVoiceClient::getDisplayName(const LLUUID& id)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->getDisplayName(id);
	}
	else
	{
	  return std::string();
	}
}

bool LLVoiceClient::isVoiceWorking() const
{
	if (mVoiceModule) 
	{
		return mVoiceModule->isVoiceWorking();
	}
	return false;
}

bool LLVoiceClient::isParticipantAvatar(const LLUUID& id)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->isParticipantAvatar(id);
	}
	else
	{
		return false;
	}
}

bool LLVoiceClient::isOnlineSIP(const LLUUID& id)
{
		return false;
}

bool LLVoiceClient::getIsSpeaking(const LLUUID& id)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->getIsSpeaking(id);
	}
	else
	{
		return false;
	}
}

bool LLVoiceClient::getIsModeratorMuted(const LLUUID& id)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->getIsModeratorMuted(id);
	}
	else
	{
		return false;
	}
}

F32 LLVoiceClient::getCurrentPower(const LLUUID& id)
{		
	if (mVoiceModule) 
	{
		return mVoiceModule->getCurrentPower(id);
	}
	else
	{
		return 0.0;
	}
}

bool LLVoiceClient::getOnMuteList(const LLUUID& id)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->getOnMuteList(id);
	}
	else
	{
		return false;
	}
}

F32 LLVoiceClient::getUserVolume(const LLUUID& id)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->getUserVolume(id);
	}
	else
	{
		return 0.0;
	}
}

void LLVoiceClient::setUserVolume(const LLUUID& id, F32 volume)
{
	if (mVoiceModule) mVoiceModule->setUserVolume(id, volume);
}

//--------------------------------------------------
// status observers

void LLVoiceClient::addObserver(LLVoiceClientStatusObserver* observer)
{
	if (mVoiceModule) mVoiceModule->addObserver(observer);
}

void LLVoiceClient::removeObserver(LLVoiceClientStatusObserver* observer)
{
    if (mVoiceModule)
    {
        mVoiceModule->removeObserver(observer);
    }
}

void LLVoiceClient::addObserver(LLFriendObserver* observer)
{
	if (mVoiceModule) mVoiceModule->addObserver(observer);
}

void LLVoiceClient::removeObserver(LLFriendObserver* observer)
{
    if (mVoiceModule)
    {
        mVoiceModule->removeObserver(observer);
    }
}

void LLVoiceClient::addObserver(LLVoiceClientParticipantObserver* observer)
{
	if (mVoiceModule) mVoiceModule->addObserver(observer);
}

void LLVoiceClient::removeObserver(LLVoiceClientParticipantObserver* observer)
{
    if (mVoiceModule)
    {
        mVoiceModule->removeObserver(observer);
    }
}

std::string LLVoiceClient::sipURIFromID(const LLUUID &id)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->sipURIFromID(id);
	}
	else
	{
		return std::string();
	}
}

LLVoiceEffectInterface* LLVoiceClient::getVoiceEffectInterface() const
{
	return getVoiceEffectEnabled() ? dynamic_cast<LLVoiceEffectInterface*>(mVoiceModule) : NULL;
}

///////////////////
// version checking

class LLViewerRequiredVoiceVersion : public LLHTTPNode
{
	static bool sAlertedUser;
	virtual void post(
					  LLHTTPNode::ResponsePtr response,
					  const LLSD& context,
					  const LLSD& input) const
	{
		//You received this messsage (most likely on region cross or
		//teleport)
		if ( input.has("body") && input["body"].has("major_version") )
		{
			int major_voice_version =
			input["body"]["major_version"].asInteger();
			// 			int minor_voice_version =
			// 				input["body"]["minor_version"].asInteger();
			LLVoiceVersionInfo versionInfo = LLVoiceClient::getInstance()->getVersion();
			
			if (major_voice_version > 1)
			{
				if (!sAlertedUser)
				{
					//sAlertedUser = true;
					LLNotificationsUtil::add("VoiceVersionMismatch");
					gSavedSettings.setBOOL("EnableVoiceChat", false); // toggles listener
				}
			}
		}
	}
};

class LLViewerParcelVoiceInfo : public LLHTTPNode
{
	virtual void post(
					  LLHTTPNode::ResponsePtr response,
					  const LLSD& context,
					  const LLSD& input) const
	{
		//the parcel you are in has changed something about its
		//voice information
		
		//this is a misnomer, as it can also be when you are not in
		//a parcel at all.  Should really be something like
		//LLViewerVoiceInfoChanged.....
		if ( input.has("body") )
		{
			LLSD body = input["body"];
			
			//body has "region_name" (str), "parcel_local_id"(int),
			//"voice_credentials" (map).
			
			//body["voice_credentials"] has "channel_uri" (str),
			//body["voice_credentials"] has "channel_credentials" (str)
			
			//if we really wanted to be extra careful,
			//we'd check the supplied
			//local parcel id to make sure it's for the same parcel
			//we believe we're in
			if ( body.has("voice_credentials") )
			{
				LLSD voice_credentials = body["voice_credentials"];
				std::string uri;
				std::string credentials;
				
				if ( voice_credentials.has("channel_uri") )
				{
					uri = voice_credentials["channel_uri"].asString();
				}
				if ( voice_credentials.has("channel_credentials") )
				{
					credentials =
					voice_credentials["channel_credentials"].asString();
				}
				
				LLVoiceClient::getInstance()->setSpatialChannel(uri, credentials);
			}
		}
	}
};

const std::string LLSpeakerVolumeStorage::SETTINGS_FILE_NAME = "volume_settings.xml";

LLSpeakerVolumeStorage::LLSpeakerVolumeStorage()
{
	load();
}

LLSpeakerVolumeStorage::~LLSpeakerVolumeStorage()
{
}

//virtual
void LLSpeakerVolumeStorage::cleanupSingleton()
{
    save();
}

void LLSpeakerVolumeStorage::storeSpeakerVolume(const LLUUID& speaker_id, F32 volume)
{
	if ((volume >= LLVoiceClient::VOLUME_MIN) && (volume <= LLVoiceClient::VOLUME_MAX))
	{
		mSpeakersData[speaker_id] = volume;

		// Enable this when debugging voice slider issues.  It's way to spammy even for debug-level logging.
		// LL_DEBUGS("Voice") << "Stored volume = " << volume <<  " for " << id << LL_ENDL;
	}
	else
	{
		LL_WARNS("Voice") << "Attempted to store out of range volume " << volume << " for " << speaker_id << LL_ENDL;
		llassert(0);
	}
}

bool LLSpeakerVolumeStorage::getSpeakerVolume(const LLUUID& speaker_id, F32& volume)
{
	speaker_data_map_t::const_iterator it = mSpeakersData.find(speaker_id);
	
	if (it != mSpeakersData.end())
	{
		volume = it->second;

		// Enable this when debugging voice slider issues.  It's way to spammy even for debug-level logging.
		// LL_DEBUGS("Voice") << "Retrieved stored volume = " << volume <<  " for " << id << LL_ENDL;

		return true;
	}

	return false;
}

void LLSpeakerVolumeStorage::removeSpeakerVolume(const LLUUID& speaker_id)
{
	mSpeakersData.erase(speaker_id);

	// Enable this when debugging voice slider issues.  It's way to spammy even for debug-level logging.
	// LL_DEBUGS("Voice") << "Removing stored volume for  " << id << LL_ENDL;
}

/* static */ F32 LLSpeakerVolumeStorage::transformFromLegacyVolume(F32 volume_in)
{
	// Convert to linear-logarithmic [0.0..1.0] with 0.5 = 0dB
	// from legacy characteristic composed of two square-curves
	// that intersect at volume_in = 0.5, volume_out = 0.56

	F32 volume_out = 0.f;
	volume_in = llclamp(volume_in, 0.f, 1.0f);

	if (volume_in <= 0.5f)
	{
		volume_out = volume_in * volume_in * 4.f * 0.56f;
	}
	else
	{
		volume_out = (1.f - 0.56f) * (4.f * volume_in * volume_in - 1.f) / 3.f + 0.56f;
	}

	return volume_out;
}

/* static */ F32 LLSpeakerVolumeStorage::transformToLegacyVolume(F32 volume_in)
{
	// Convert from linear-logarithmic [0.0..1.0] with 0.5 = 0dB
	// to legacy characteristic composed of two square-curves
	// that intersect at volume_in = 0.56, volume_out = 0.5

	F32 volume_out = 0.f;
	volume_in = llclamp(volume_in, 0.f, 1.0f);

	if (volume_in <= 0.56f)
	{
		volume_out = sqrt(volume_in / (4.f * 0.56f));
	}
	else
	{
		volume_out = sqrt((3.f * (volume_in - 0.56f) / (1.f - 0.56f) + 1.f) / 4.f);
	}

	return volume_out;
}

void LLSpeakerVolumeStorage::load()
{
	// load per-resident voice volume information
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, SETTINGS_FILE_NAME);

	LL_INFOS("Voice") << "Loading stored speaker volumes from: " << filename << LL_ENDL;

	LLSD settings_llsd;
	llifstream file;
	file.open(filename.c_str());
	if (file.is_open())
	{
		if (LLSDParser::PARSE_FAILURE == LLSDSerialize::fromXML(settings_llsd, file))
        {
            LL_WARNS("Voice") << "failed to parse " << filename << LL_ENDL;
            
        }
            
	}

	for (LLSD::map_const_iterator iter = settings_llsd.beginMap();
		iter != settings_llsd.endMap(); ++iter)
	{
		// Maintain compatibility with 1.23 non-linear saved volume levels
		F32 volume = transformFromLegacyVolume((F32)iter->second.asReal());

		storeSpeakerVolume(LLUUID(iter->first), volume);
	}
}

void LLSpeakerVolumeStorage::save()
{
	// If we quit from the login screen we will not have an SL account
	// name.  Don't try to save, otherwise we'll dump a file in
	// C:\Program Files\SecondLife\ or similar. JC
	std::string user_dir = gDirUtilp->getLindenUserDir();
	if (!user_dir.empty())
	{
		std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, SETTINGS_FILE_NAME);
		LLSD settings_llsd;

		LL_INFOS("Voice") << "Saving stored speaker volumes to: " << filename << LL_ENDL;

		for(speaker_data_map_t::const_iterator iter = mSpeakersData.begin(); iter != mSpeakersData.end(); ++iter)
		{
			// Maintain compatibility with 1.23 non-linear saved volume levels
			F32 volume = transformToLegacyVolume(iter->second);

			settings_llsd[iter->first.asString()] = volume;
		}

		llofstream file;
		file.open(filename.c_str());
		LLSDSerialize::toPrettyXML(settings_llsd, file);
	}
}

bool LLViewerRequiredVoiceVersion::sAlertedUser = false;

LLHTTPRegistration<LLViewerParcelVoiceInfo>
gHTTPRegistrationMessageParcelVoiceInfo(
										"/message/ParcelVoiceInfo");

LLHTTPRegistration<LLViewerRequiredVoiceVersion>
gHTTPRegistrationMessageRequiredVoiceVersion(
											 "/message/RequiredVoiceVersion");
