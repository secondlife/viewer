/** 
 * @file llvoicechannel.cpp
 * @brief Voice Channel related classes
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

#include "llagent.h"
#include "llfloaterreg.h"
#include "llimview.h"
#include "llnotifications.h"
#include "llnotificationsutil.h"
#include "llpanel.h"
#include "llrecentpeople.h"
#include "llviewercontrol.h"
#include "llvoicechannel.h"


LLVoiceChannel::voice_channel_map_t LLVoiceChannel::sVoiceChannelMap;
LLVoiceChannel::voice_channel_map_uri_t LLVoiceChannel::sVoiceChannelURIMap;
LLVoiceChannel* LLVoiceChannel::sCurrentVoiceChannel = NULL;
LLVoiceChannel* LLVoiceChannel::sSuspendedVoiceChannel = NULL;
LLVoiceChannel::channel_changed_signal_t LLVoiceChannel::sCurrentVoiceChannelChangedSignal;

BOOL LLVoiceChannel::sSuspended = FALSE;

//
// Constants
//
const U32 DEFAULT_RETRIES_COUNT = 3;


class LLVoiceCallCapResponder : public LLHTTPClient::Responder
{
public:
	LLVoiceCallCapResponder(const LLUUID& session_id) : mSessionID(session_id) {};

	// called with bad status codes
	virtual void errorWithContent(U32 status, const std::string& reason, const LLSD& content);
	virtual void result(const LLSD& content);

private:
	LLUUID mSessionID;
};


void LLVoiceCallCapResponder::errorWithContent(U32 status, const std::string& reason, const LLSD& content)
{
	LL_WARNS("Voice") << "LLVoiceCallCapResponder error [status:"
		<< status << "]: " << content << LL_ENDL;
	LLVoiceChannel* channelp = LLVoiceChannel::getChannelByID(mSessionID);
	if ( channelp )
	{
		if ( 403 == status )
		{
			//403 == no ability
			LLNotificationsUtil::add(
				"VoiceNotAllowed",
				channelp->getNotifyArgs());
		}
		else
		{
			LLNotificationsUtil::add(
				"VoiceCallGenericError",
				channelp->getNotifyArgs());
		}
		channelp->deactivate();
	}
}

void LLVoiceCallCapResponder::result(const LLSD& content)
{
	LLVoiceChannel* channelp = LLVoiceChannel::getChannelByID(mSessionID);
	if (channelp)
	{
		//*TODO: DEBUG SPAM
		LLSD::map_const_iterator iter;
		for(iter = content.beginMap(); iter != content.endMap(); ++iter)
		{
			LL_DEBUGS("Voice") << "LLVoiceCallCapResponder::result got " 
				<< iter->first << LL_ENDL;
		}

		channelp->setChannelInfo(
			content["voice_credentials"]["channel_uri"].asString(),
			content["voice_credentials"]["channel_credentials"].asString());
	}
}

//
// LLVoiceChannel
//
LLVoiceChannel::LLVoiceChannel(const LLUUID& session_id, const std::string& session_name) : 
	mSessionID(session_id), 
	mState(STATE_NO_CHANNEL_INFO), 
	mSessionName(session_name),
	mCallDirection(OUTGOING_CALL),
	mIgnoreNextSessionLeave(FALSE),
	mCallEndedByAgent(false)
{
	mNotifyArgs["VOICE_CHANNEL_NAME"] = mSessionName;

	if (!sVoiceChannelMap.insert(std::make_pair(session_id, this)).second)
	{
		// a voice channel already exists for this session id, so this instance will be orphaned
		// the end result should simply be the failure to make voice calls
		LL_WARNS("Voice") << "Duplicate voice channels registered for session_id " << session_id << LL_ENDL;
	}
}

LLVoiceChannel::~LLVoiceChannel()
{
	// Must check instance exists here, the singleton MAY have already been destroyed.
	if(LLVoiceClient::instanceExists())
	{
		LLVoiceClient::getInstance()->removeObserver(this);
	}
	
	sVoiceChannelMap.erase(mSessionID);
	sVoiceChannelURIMap.erase(mURI);
}

void LLVoiceChannel::setChannelInfo(
	const std::string& uri,
	const std::string& credentials)
{
	setURI(uri);

	mCredentials = credentials;

	if (mState == STATE_NO_CHANNEL_INFO)
	{
		if (mURI.empty())
		{
			LLNotificationsUtil::add("VoiceChannelJoinFailed", mNotifyArgs);
			LL_WARNS("Voice") << "Received empty URI for channel " << mSessionName << LL_ENDL;
			deactivate();
		}
		else if (mCredentials.empty())
		{
			LLNotificationsUtil::add("VoiceChannelJoinFailed", mNotifyArgs);
			LL_WARNS("Voice") << "Received empty credentials for channel " << mSessionName << LL_ENDL;
			deactivate();
		}
		else
		{
			setState(STATE_READY);

			// if we are supposed to be active, reconnect
			// this will happen on initial connect, as we request credentials on first use
			if (sCurrentVoiceChannel == this)
			{
				// just in case we got new channel info while active
				// should move over to new channel
				activate();
			}
		}
	}
}

void LLVoiceChannel::onChange(EStatusType type, const std::string &channelURI, bool proximal)
{
	if (channelURI != mURI)
	{
		return;
	}

	if (type < BEGIN_ERROR_STATUS)
	{
		handleStatusChange(type);
	}
	else
	{
		handleError(type);
	}
}

void LLVoiceChannel::handleStatusChange(EStatusType type)
{
	// status updates
	switch(type)
	{
	case STATUS_LOGIN_RETRY:
		//mLoginNotificationHandle = LLNotifyBox::showXml("VoiceLoginRetry")->getHandle();
		LLNotificationsUtil::add("VoiceLoginRetry");
		break;
	case STATUS_LOGGED_IN:
		//if (!mLoginNotificationHandle.isDead())
		//{
		//	LLNotifyBox* notifyp = (LLNotifyBox*)mLoginNotificationHandle.get();
		//	if (notifyp)
		//	{
		//		notifyp->close();
		//	}
		//	mLoginNotificationHandle.markDead();
		//}
		break;
	case STATUS_LEFT_CHANNEL:
		if (callStarted() && !mIgnoreNextSessionLeave && !sSuspended)
		{
			// if forceably removed from channel
			// update the UI and revert to default channel
			deactivate();
		}
		mIgnoreNextSessionLeave = FALSE;
		break;
	case STATUS_JOINING:
		if (callStarted())
		{
			setState(STATE_RINGING);
		}
		break;
	case STATUS_JOINED:
		if (callStarted())
		{
			setState(STATE_CONNECTED);
		}
	default:
		break;
	}
}

// default behavior is to just deactivate channel
// derived classes provide specific error messages
void LLVoiceChannel::handleError(EStatusType type)
{
	deactivate();
	setState(STATE_ERROR);
}

BOOL LLVoiceChannel::isActive()
{ 
	// only considered active when currently bound channel matches what our channel
	return callStarted() && LLVoiceClient::getInstance()->getCurrentChannel() == mURI; 
}

BOOL LLVoiceChannel::callStarted()
{
	return mState >= STATE_CALL_STARTED;
}

void LLVoiceChannel::deactivate()
{
	if (mState >= STATE_RINGING)
	{
		// ignore session leave event
		mIgnoreNextSessionLeave = TRUE;
	}

	if (callStarted())
	{
		setState(STATE_HUNG_UP);
		
		//Default mic is OFF when leaving voice calls
		if (gSavedSettings.getBOOL("AutoDisengageMic") && 
			sCurrentVoiceChannel == this &&
			LLVoiceClient::getInstance()->getUserPTTState())
		{
			gSavedSettings.setBOOL("PTTCurrentlyEnabled", true);
			LLVoiceClient::getInstance()->inputUserControlState(true);
		}
	}
	LLVoiceClient::getInstance()->removeObserver(this);
	
	if (sCurrentVoiceChannel == this)
	{
		// default channel is proximal channel
		sCurrentVoiceChannel = LLVoiceChannelProximal::getInstance();
		sCurrentVoiceChannel->activate();
	}
}

void LLVoiceChannel::activate()
{
	if (callStarted())
	{
		return;
	}

	// deactivate old channel and mark ourselves as the active one
	if (sCurrentVoiceChannel != this)
	{
		// mark as current before deactivating the old channel to prevent
		// activating the proximal channel between IM calls
		LLVoiceChannel* old_channel = sCurrentVoiceChannel;
		sCurrentVoiceChannel = this;
		mCallDialogPayload["old_channel_name"] = "";
		if (old_channel)
		{
			mCallDialogPayload["old_channel_name"] = old_channel->getSessionName();
			old_channel->deactivate();
		}
	}

	if (mState == STATE_NO_CHANNEL_INFO)
	{
		// responsible for setting status to active
		getChannelInfo();
	}
	else
	{
		setState(STATE_CALL_STARTED);
	}
	
	LLVoiceClient::getInstance()->addObserver(this);
	
	//do not send earlier, channel should be initialized, should not be in STATE_NO_CHANNEL_INFO state
	sCurrentVoiceChannelChangedSignal(this->mSessionID);
}

void LLVoiceChannel::getChannelInfo()
{
	// pretend we have everything we need
	if (sCurrentVoiceChannel == this)
	{
		setState(STATE_CALL_STARTED);
	}
}

//static 
LLVoiceChannel* LLVoiceChannel::getChannelByID(const LLUUID& session_id)
{
	voice_channel_map_t::iterator found_it = sVoiceChannelMap.find(session_id);
	if (found_it == sVoiceChannelMap.end())
	{
		return NULL;
	}
	else
	{
		return found_it->second;
	}
}

//static 
LLVoiceChannel* LLVoiceChannel::getChannelByURI(std::string uri)
{
	voice_channel_map_uri_t::iterator found_it = sVoiceChannelURIMap.find(uri);
	if (found_it == sVoiceChannelURIMap.end())
	{
		return NULL;
	}
	else
	{
		return found_it->second;
	}
}

LLVoiceChannel* LLVoiceChannel::getCurrentVoiceChannel()
{
	return sCurrentVoiceChannel;
}

void LLVoiceChannel::updateSessionID(const LLUUID& new_session_id)
{
	sVoiceChannelMap.erase(sVoiceChannelMap.find(mSessionID));
	mSessionID = new_session_id;
	sVoiceChannelMap.insert(std::make_pair(mSessionID, this));
}

void LLVoiceChannel::setURI(std::string uri)
{
	sVoiceChannelURIMap.erase(mURI);
	mURI = uri;
	sVoiceChannelURIMap.insert(std::make_pair(mURI, this));
}

void LLVoiceChannel::setState(EState state)
{
	switch(state)
	{
	case STATE_RINGING:
		//TODO: remove or redirect this call status notification
//		LLCallInfoDialog::show("ringing", mNotifyArgs);
		break;
	case STATE_CONNECTED:
		//TODO: remove or redirect this call status notification
//		LLCallInfoDialog::show("connected", mNotifyArgs);
		break;
	case STATE_HUNG_UP:
		//TODO: remove or redirect this call status notification
//		LLCallInfoDialog::show("hang_up", mNotifyArgs);
		break;
	default:
		break;
	}

	doSetState(state);
}

void LLVoiceChannel::doSetState(const EState& new_state)
{
	EState old_state = mState;
	mState = new_state;

	if (!mStateChangedCallback.empty())
		mStateChangedCallback(old_state, mState, mCallDirection, mCallEndedByAgent);
}

//static
void LLVoiceChannel::initClass()
{
	sCurrentVoiceChannel = LLVoiceChannelProximal::getInstance();
}

//static 
void LLVoiceChannel::suspend()
{
	if (!sSuspended)
	{
		sSuspendedVoiceChannel = sCurrentVoiceChannel;
		sSuspended = TRUE;
	}
}

//static 
void LLVoiceChannel::resume()
{
	if (sSuspended)
	{
		if (LLVoiceClient::getInstance()->voiceEnabled())
		{
			if (sSuspendedVoiceChannel)
			{
				sSuspendedVoiceChannel->activate();
			}
			else
			{
				LLVoiceChannelProximal::getInstance()->activate();
			}
		}
		sSuspended = FALSE;
	}
}

boost::signals2::connection LLVoiceChannel::setCurrentVoiceChannelChangedCallback(channel_changed_callback_t cb, bool at_front)
{
	if (at_front)
	{
		return sCurrentVoiceChannelChangedSignal.connect(cb,  boost::signals2::at_front);
	}
	else
	{
		return sCurrentVoiceChannelChangedSignal.connect(cb);
	}
}

//
// LLVoiceChannelGroup
//

LLVoiceChannelGroup::LLVoiceChannelGroup(const LLUUID& session_id, const std::string& session_name) : 
	LLVoiceChannel(session_id, session_name)
{
	mRetries = DEFAULT_RETRIES_COUNT;
	mIsRetrying = FALSE;
}

void LLVoiceChannelGroup::deactivate()
{
	if (callStarted())
	{
		LLVoiceClient::getInstance()->leaveNonSpatialChannel();
	}
	LLVoiceChannel::deactivate();
}

void LLVoiceChannelGroup::activate()
{
	if (callStarted()) return;

	LLVoiceChannel::activate();

	if (callStarted())
	{
		// we have the channel info, just need to use it now
		LLVoiceClient::getInstance()->setNonSpatialChannel(
			mURI,
			mCredentials);

		if (!gAgent.isInGroup(mSessionID)) // ad-hoc channel
		{
			LLIMModel::LLIMSession* session = LLIMModel::getInstance()->findIMSession(mSessionID);
			// Adding ad-hoc call participants to Recent People List.
			// If it's an outgoing ad-hoc, we can use mInitialTargetIDs that holds IDs of people we
			// called(both online and offline) as source to get people for recent (STORM-210).
			if (session->isOutgoingAdHoc())
			{
				for (uuid_vec_t::iterator it = session->mInitialTargetIDs.begin();
					it!=session->mInitialTargetIDs.end();++it)
				{
					const LLUUID id = *it;
					LLRecentPeople::instance().add(id);
				}
			}
			// If this ad-hoc is incoming then trying to get ids of people from mInitialTargetIDs
			// would lead to EXT-8246. So in this case we get them from speakers list.
			else
			{
				LLIMModel::addSpeakersToRecent(mSessionID);
			}
		}

		//Mic default state is OFF on initiating/joining Ad-Hoc/Group calls
		if (LLVoiceClient::getInstance()->getUserPTTState() && LLVoiceClient::getInstance()->getPTTIsToggle())
		{
			LLVoiceClient::getInstance()->inputUserControlState(true);
		}
		
	}
}

void LLVoiceChannelGroup::getChannelInfo()
{
	LLViewerRegion* region = gAgent.getRegion();
	if (region)
	{
		std::string url = region->getCapability("ChatSessionRequest");
		LLSD data;
		data["method"] = "call";
		data["session-id"] = mSessionID;
		LLHTTPClient::post(url,
						   data,
						   new LLVoiceCallCapResponder(mSessionID));
	}
}

void LLVoiceChannelGroup::setChannelInfo(
	const std::string& uri,
	const std::string& credentials)
{
	setURI(uri);

	mCredentials = credentials;

	if (mState == STATE_NO_CHANNEL_INFO)
	{
		if(!mURI.empty() && !mCredentials.empty())
		{
			setState(STATE_READY);

			// if we are supposed to be active, reconnect
			// this will happen on initial connect, as we request credentials on first use
			if (sCurrentVoiceChannel == this)
			{
				// just in case we got new channel info while active
				// should move over to new channel
				activate();
			}
		}
		else
		{
			//*TODO: notify user
			LL_WARNS("Voice") << "Received invalid credentials for channel " << mSessionName << LL_ENDL;
			deactivate();
		}
	}
	else if ( mIsRetrying )
	{
		// we have the channel info, just need to use it now
		LLVoiceClient::getInstance()->setNonSpatialChannel(
			mURI,
			mCredentials);
	}
}

void LLVoiceChannelGroup::handleStatusChange(EStatusType type)
{
	// status updates
	switch(type)
	{
	case STATUS_JOINED:
		mRetries = 3;
		mIsRetrying = FALSE;
	default:
		break;
	}

	LLVoiceChannel::handleStatusChange(type);
}

void LLVoiceChannelGroup::handleError(EStatusType status)
{
	std::string notify;
	switch(status)
	{
	case ERROR_CHANNEL_LOCKED:
	case ERROR_CHANNEL_FULL:
		notify = "VoiceChannelFull";
		break;
	case ERROR_NOT_AVAILABLE:
		//clear URI and credentials
		//set the state to be no info
		//and activate
		if ( mRetries > 0 )
		{
			mRetries--;
			mIsRetrying = TRUE;
			mIgnoreNextSessionLeave = TRUE;

			getChannelInfo();
			return;
		}
		else
		{
			notify = "VoiceChannelJoinFailed";
			mRetries = DEFAULT_RETRIES_COUNT;
			mIsRetrying = FALSE;
		}

		break;

	case ERROR_UNKNOWN:
	default:
		break;
	}

	// notification
	if (!notify.empty())
	{
		LLNotificationPtr notification = LLNotificationsUtil::add(notify, mNotifyArgs);
		// echo to im window
		gIMMgr->addMessage(mSessionID, LLUUID::null, SYSTEM_FROM, notification->getMessage());
	}

	LLVoiceChannel::handleError(status);
}

void LLVoiceChannelGroup::setState(EState state)
{
	switch(state)
	{
	case STATE_RINGING:
		if ( !mIsRetrying )
		{
			//TODO: remove or redirect this call status notification
//			LLCallInfoDialog::show("ringing", mNotifyArgs);
		}

		doSetState(state);
		break;
	default:
		LLVoiceChannel::setState(state);
	}
}

//
// LLVoiceChannelProximal
//
LLVoiceChannelProximal::LLVoiceChannelProximal() : 
	LLVoiceChannel(LLUUID::null, LLStringUtil::null)
{
}

BOOL LLVoiceChannelProximal::isActive()
{
	return callStarted() && LLVoiceClient::getInstance()->inProximalChannel(); 
}

void LLVoiceChannelProximal::activate()
{
	if (callStarted()) return;

	if((LLVoiceChannel::sCurrentVoiceChannel != this) && (LLVoiceChannel::getState() == STATE_CONNECTED))
	{
		// we're connected to a non-spatial channel, so disconnect.
		LLVoiceClient::getInstance()->leaveNonSpatialChannel();	
	}
	LLVoiceChannel::activate();
	
}

void LLVoiceChannelProximal::onChange(EStatusType type, const std::string &channelURI, bool proximal)
{
	if (!proximal)
	{
		return;
	}

	if (type < BEGIN_ERROR_STATUS)
	{
		handleStatusChange(type);
	}
	else
	{
		handleError(type);
	}
}

void LLVoiceChannelProximal::handleStatusChange(EStatusType status)
{
	// status updates
	switch(status)
	{
	case STATUS_LEFT_CHANNEL:
		// do not notify user when leaving proximal channel
		return;
	case STATUS_VOICE_DISABLED:
		//skip showing "Voice not available at your current location" when agent voice is disabled (EXT-4749)
		if(LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking())
		{
			//TODO: remove or redirect this call status notification
//			LLCallInfoDialog::show("unavailable", mNotifyArgs);
		}
		return;
	default:
		break;
	}
	LLVoiceChannel::handleStatusChange(status);
}


void LLVoiceChannelProximal::handleError(EStatusType status)
{
	std::string notify;
	switch(status)
	{
	  case ERROR_CHANNEL_LOCKED:
	  case ERROR_CHANNEL_FULL:
		notify = "ProximalVoiceChannelFull";
		break;
	  default:
		 break;
	}

	// notification
	if (!notify.empty())
	{
		LLNotificationsUtil::add(notify, mNotifyArgs);
	}

	LLVoiceChannel::handleError(status);
}

void LLVoiceChannelProximal::deactivate()
{
	if (callStarted())
	{
		setState(STATE_HUNG_UP);
	}
}


//
// LLVoiceChannelP2P
//
LLVoiceChannelP2P::LLVoiceChannelP2P(const LLUUID& session_id, const std::string& session_name, const LLUUID& other_user_id) : 
		LLVoiceChannelGroup(session_id, session_name), 
		mOtherUserID(other_user_id),
		mReceivedCall(FALSE)
{
	// make sure URI reflects encoded version of other user's agent id
	// *NOTE: in case of Avaline call generated SIP URL will be incorrect.
	// But it will be overridden in LLVoiceChannelP2P::setSessionHandle() called when agent accepts call
	setURI(LLVoiceClient::getInstance()->sipURIFromID(other_user_id));
}

void LLVoiceChannelP2P::handleStatusChange(EStatusType type)
{
	LL_INFOS("Voice") << "P2P CALL CHANNEL STATUS CHANGE: incoming=" << int(mReceivedCall) << " newstatus=" << LLVoiceClientStatusObserver::status2string(type) << " (mState=" << mState << ")" << LL_ENDL;

	// status updates
	switch(type)
	{
	case STATUS_LEFT_CHANNEL:
		if (callStarted() && !mIgnoreNextSessionLeave && !sSuspended)
		{
			// *TODO: use it to show DECLINE voice notification
			if (mState == STATE_RINGING)
			{
				// other user declined call
				LLNotificationsUtil::add("P2PCallDeclined", mNotifyArgs);
			}
			else
			{
				// other user hung up, so we didn't end the call				
				mCallEndedByAgent = false;			
			}
			deactivate();
		}
		mIgnoreNextSessionLeave = FALSE;
		return;
	case STATUS_JOINING:
		// because we join session we expect to process session leave event in the future. EXT-7371
		// may be this should be done in the LLVoiceChannel::handleStatusChange.
		mIgnoreNextSessionLeave = FALSE;
		break;

	default:
		break;
	}

	LLVoiceChannel::handleStatusChange(type);
}

void LLVoiceChannelP2P::handleError(EStatusType type)
{
	switch(type)
	{
	case ERROR_NOT_AVAILABLE:
		LLNotificationsUtil::add("P2PCallNoAnswer", mNotifyArgs);
		break;
	default:
		break;
	}

	LLVoiceChannel::handleError(type);
}

void LLVoiceChannelP2P::activate()
{
	if (callStarted()) return;

	//call will be counted as ended by user unless this variable is changed in handleStatusChange()
	mCallEndedByAgent = true;

	LLVoiceChannel::activate();

	if (callStarted())
	{
		// no session handle yet, we're starting the call
		if (mSessionHandle.empty())
		{
			mReceivedCall = FALSE;
			LLVoiceClient::getInstance()->callUser(mOtherUserID);
		}
		// otherwise answering the call
		else
		{
			if (!LLVoiceClient::getInstance()->answerInvite(mSessionHandle))
			{
				mCallEndedByAgent = false;
				mSessionHandle.clear();
				handleError(ERROR_UNKNOWN);
				return;
			}
			// using the session handle invalidates it.  Clear it out here so we can't reuse it by accident.
			mSessionHandle.clear();
		}

		// Add the party to the list of people with which we've recently interacted.
		addToTheRecentPeopleList();

		//Default mic is ON on initiating/joining P2P calls
		if (!LLVoiceClient::getInstance()->getUserPTTState() && LLVoiceClient::getInstance()->getPTTIsToggle())
		{
			LLVoiceClient::getInstance()->inputUserControlState(true);
		}
	}
}

void LLVoiceChannelP2P::getChannelInfo()
{
	// pretend we have everything we need, since P2P doesn't use channel info
	if (sCurrentVoiceChannel == this)
	{
		setState(STATE_CALL_STARTED);
	}
}

// receiving session from other user who initiated call
void LLVoiceChannelP2P::setSessionHandle(const std::string& handle, const std::string &inURI)
{ 
	BOOL needs_activate = FALSE;
	if (callStarted())
	{
		// defer to lower agent id when already active
		if (mOtherUserID < gAgent.getID())
		{
			// pretend we haven't started the call yet, so we can connect to this session instead
			deactivate();
			needs_activate = TRUE;
		}
		else
		{
			// we are active and have priority, invite the other user again
			// under the assumption they will join this new session
			mSessionHandle.clear();
			LLVoiceClient::getInstance()->callUser(mOtherUserID);
			return;
		}
	}

	mSessionHandle = handle;

	// The URI of a p2p session should always be the other end's SIP URI.
	if(!inURI.empty())
	{
		setURI(inURI);
	}
	else
	{
		LL_WARNS("Voice") << "incoming SIP URL is not provided. Channel may not work properly." << LL_ENDL;
		// In the case of an incoming AvaLine call, the generated URI will be different from the
		// original one. This is because the P2P URI is based on avatar UUID but Avaline is not.
		// See LLVoiceClient::sessionAddedEvent()
		setURI(LLVoiceClient::getInstance()->sipURIFromID(mOtherUserID));
	}
	
	mReceivedCall = TRUE;

	if (needs_activate)
	{
		activate();
	}
}

void LLVoiceChannelP2P::setState(EState state)
{
	LL_INFOS("Voice") << "P2P CALL STATE CHANGE: incoming=" << int(mReceivedCall) << " oldstate=" << mState << " newstate=" << state << LL_ENDL;

	if (mReceivedCall) // incoming call
	{
		// you only "answer" voice invites in p2p mode
		// so provide a special purpose message here
		if (mReceivedCall && state == STATE_RINGING)
		{
			//TODO: remove or redirect this call status notification
//			LLCallInfoDialog::show("answering", mNotifyArgs);
			doSetState(state);
			return;
		}
	}

	LLVoiceChannel::setState(state);
}

void LLVoiceChannelP2P::addToTheRecentPeopleList()
{
	bool avaline_call = LLIMModel::getInstance()->findIMSession(mSessionID)->isAvalineSessionType();
	
	if (avaline_call)
	{
		LLSD call_data;
		std::string call_number = LLVoiceChannel::getSessionName();
		
		call_data["avaline_call"]	= true;
		call_data["session_id"]		= mSessionID;
		call_data["call_number"]	= call_number;
		call_data["date"]			= LLDate::now();
		
		LLRecentPeople::instance().add(mOtherUserID, call_data);
	}
	else
	{
		LLRecentPeople::instance().add(mOtherUserID);
	}
}
