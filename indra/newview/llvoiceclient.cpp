 /** 
 * @file llvoiceclient.cpp
 * @brief Voice client delegation class implementation.
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"
#include "llvoiceclient.h"
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llvoicedw.h"
#include "llvoicevivox.h"
#include "llviewernetwork.h"
#include "llhttpnode.h"
#include "llnotificationsutil.h"
#include "llsdserialize.h"
#include "llui.h"

const F32 LLVoiceClient::OVERDRIVEN_POWER_LEVEL = 0.7f;

std::string LLVoiceClientStatusObserver::status2string(LLVoiceClientStatusObserver::EStatusType inStatus)
{
	std::string result = "UNKNOWN";
	
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
			CASE(BEGIN_ERROR_STATUS);
			CASE(ERROR_CHANNEL_FULL);
			CASE(ERROR_CHANNEL_LOCKED);
			CASE(ERROR_NOT_AVAILABLE);
			CASE(ERROR_UNKNOWN);
		default:
			break;
	}
	
#undef CASE
	
	return result;
}




///////////////////////////////////////////////////////////////////////////////////////////////

LLVoiceClient::LLVoiceClient()
{
	mVoiceModule = NULL;
}

//---------------------------------------------------
// Basic setup/shutdown

LLVoiceClient::~LLVoiceClient()
{
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
	if(voice_server == "diamondware")
	{
		mVoiceModule = (LLVoiceModuleInterface *)LLDiamondwareVoiceClient::getInstance();
	}
	else if(voice_server == "vivox")
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


void LLVoiceClient::terminate()
{
	if (mVoiceModule) mVoiceModule->terminate();
	mVoiceModule = NULL;
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
		return result;
	}
}

void LLVoiceClient::updateSettings()
{
	if (mVoiceModule) mVoiceModule->updateSettings();
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


BOOL LLVoiceClient::isSessionTextIMPossible(const LLUUID& id)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->isSessionTextIMPossible(id);
	}
	else
	{
		return FALSE;
	}	
}

BOOL LLVoiceClient::isSessionCallBackPossible(const LLUUID& id)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->isSessionCallBackPossible(id);
	}
	else
	{
		return FALSE;
	}	
}

BOOL LLVoiceClient::sendTextMessage(const LLUUID& participant_id, const std::string& message)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->sendTextMessage(participant_id, message);
	}
	else
	{
		return FALSE;
	}	
}

void LLVoiceClient::endUserIMSession(const LLUUID& participant_id)
{
	if (mVoiceModule) 
	{
		mVoiceModule->endUserIMSession(participant_id);
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
	if (mVoiceModule) mVoiceModule->setNonSpatialChannel(uri, credentials);
}

void LLVoiceClient::setSpatialChannel(
	const std::string &uri,
	const std::string &credentials)
{
	if (mVoiceModule) mVoiceModule->setSpatialChannel(uri, credentials);
}

void LLVoiceClient::leaveNonSpatialChannel()
{
	if (mVoiceModule) mVoiceModule->leaveNonSpatialChannel();
}

void LLVoiceClient::leaveChannel(void)
{
	if (mVoiceModule) mVoiceModule->leaveChannel();
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
	if (mVoiceModule) mVoiceModule->setVoiceEnabled(enabled);
}

void LLVoiceClient::setLipSyncEnabled(BOOL enabled)
{
	if (mVoiceModule) mVoiceModule->setLipSyncEnabled(enabled);
}

BOOL LLVoiceClient::lipSyncEnabled()
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
	if (mVoiceModule) mVoiceModule->setMuteMic(muted);
}


// ----------------------------------------------
// PTT

void LLVoiceClient::setUserPTTState(bool ptt)
{
	if (mVoiceModule) mVoiceModule->setUserPTTState(ptt);
}

bool LLVoiceClient::getUserPTTState()
{
	if (mVoiceModule) 
	{
		return mVoiceModule->getUserPTTState();
	}
	else
	{
		return false;
	}
}

void LLVoiceClient::setUsePTT(bool usePTT)
{
	if (mVoiceModule) mVoiceModule->setUsePTT(usePTT);
}

void LLVoiceClient::setPTTIsToggle(bool PTTIsToggle)
{
	if (mVoiceModule) mVoiceModule->setPTTIsToggle(PTTIsToggle);
}

bool LLVoiceClient::getPTTIsToggle()
{
	if (mVoiceModule) 
	{
		return mVoiceModule->getPTTIsToggle();
	}
	else {
		return false;
	}

}

void LLVoiceClient::inputUserControlState(bool down)
{
	if (mVoiceModule) mVoiceModule->inputUserControlState(down);	
}

void LLVoiceClient::toggleUserPTTState(void)
{
	if (mVoiceModule) mVoiceModule->toggleUserPTTState();
}

void LLVoiceClient::keyDown(KEY key, MASK mask)
{	
	if (mVoiceModule) mVoiceModule->keyDown(key, mask);
}
void LLVoiceClient::keyUp(KEY key, MASK mask)
{
	if (mVoiceModule) mVoiceModule->keyUp(key, mask);
}
void LLVoiceClient::middleMouseState(bool down)
{
	if (mVoiceModule) mVoiceModule->middleMouseState(down);
}


//-------------------------------------------
// nearby speaker accessors

BOOL LLVoiceClient::getVoiceEnabled(const LLUUID& id)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->getVoiceEnabled(id);
	} 
	else
	{
		return FALSE;
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

bool LLVoiceClient::isVoiceWorking()
{
	if (mVoiceModule) 
	{
		return mVoiceModule->isVoiceWorking();
	}
	return false;
}

BOOL LLVoiceClient::isParticipantAvatar(const LLUUID& id)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->isParticipantAvatar(id);
	}
	else
	{
		return FALSE;
	}
}

BOOL LLVoiceClient::isOnlineSIP(const LLUUID& id)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->isOnlineSIP(id);
	}
	else
	{
		return FALSE;
	}
}

BOOL LLVoiceClient::getIsSpeaking(const LLUUID& id)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->getIsSpeaking(id);
	}
	else
	{
		return FALSE;
	}
}

BOOL LLVoiceClient::getIsModeratorMuted(const LLUUID& id)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->getIsModeratorMuted(id);
	}
	else
	{
		return FALSE;
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

BOOL LLVoiceClient::getOnMuteList(const LLUUID& id)
{
	if (mVoiceModule) 
	{
		return mVoiceModule->getOnMuteList(id);
	}
	else
	{
		return FALSE;
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
	if (mVoiceModule) mVoiceModule->removeObserver(observer);
}

void LLVoiceClient::addObserver(LLFriendObserver* observer)
{
	if (mVoiceModule) mVoiceModule->addObserver(observer);
}

void LLVoiceClient::removeObserver(LLFriendObserver* observer)
{
	if (mVoiceModule) mVoiceModule->removeObserver(observer);
}

void LLVoiceClient::addObserver(LLVoiceClientParticipantObserver* observer)
{
	if (mVoiceModule) mVoiceModule->addObserver(observer);
}

void LLVoiceClient::removeObserver(LLVoiceClientParticipantObserver* observer)
{
	if (mVoiceModule) mVoiceModule->removeObserver(observer);
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


///////////////////
// version checking

class LLViewerRequiredVoiceVersion : public LLHTTPNode
{
	static BOOL sAlertedUser;
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
					//sAlertedUser = TRUE;
					LLNotificationsUtil::add("VoiceVersionMismatch");
					gSavedSettings.setBOOL("EnableVoiceChat", FALSE); // toggles listener
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
	save();
}

void LLSpeakerVolumeStorage::storeSpeakerVolume(const LLUUID& speaker_id, F32 volume)
{
	mSpeakersData[speaker_id] = volume;
}

S32 LLSpeakerVolumeStorage::getSpeakerVolume(const LLUUID& speaker_id)
{
	// Return value of -1 indicates no level is stored for this speaker
	S32 ret_val = -1;
	speaker_data_map_t::const_iterator it = mSpeakersData.find(speaker_id);
	
	if (it != mSpeakersData.end())
	{
		F32 f_val = it->second;
		// volume can amplify by as much as 4x!
		S32 ivol = (S32)(400.f * f_val * f_val);
		ret_val = llclamp(ivol, 0, 400);
	}
	return ret_val;
}

void LLSpeakerVolumeStorage::load()
{
	// load per-resident voice volume information
	std::string filename = gDirUtilp->getExpandedFilename(LL_PATH_PER_SL_ACCOUNT, SETTINGS_FILE_NAME);

	LLSD settings_llsd;
	llifstream file;
	file.open(filename);
	if (file.is_open())
	{
		LLSDSerialize::fromXML(settings_llsd, file);
	}

	for (LLSD::map_const_iterator iter = settings_llsd.beginMap();
		iter != settings_llsd.endMap(); ++iter)
	{
		mSpeakersData.insert(std::make_pair(LLUUID(iter->first), (F32)iter->second.asReal()));
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

		for(speaker_data_map_t::const_iterator iter = mSpeakersData.begin(); iter != mSpeakersData.end(); ++iter)
		{
			settings_llsd[iter->first.asString()] = iter->second;
		}

		llofstream file;
		file.open(filename);
		LLSDSerialize::toPrettyXML(settings_llsd, file);
	}
}

BOOL LLViewerRequiredVoiceVersion::sAlertedUser = FALSE;

LLHTTPRegistration<LLViewerParcelVoiceInfo>
gHTTPRegistrationMessageParcelVoiceInfo(
										"/message/ParcelVoiceInfo");

LLHTTPRegistration<LLViewerRequiredVoiceVersion>
gHTTPRegistrationMessageRequiredVoiceVersion(
											 "/message/RequiredVoiceVersion");
