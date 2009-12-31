/** 
 * @file llvoicechannel.cpp
 * @brief Voice Channel related classes
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
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
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

	virtual void error(U32 status, const std::string& reason);	// called with bad status codes
	virtual void result(const LLSD& content);

private:
	LLUUID mSessionID;
};


void LLVoiceCallCapResponder::error(U32 status, const std::string& reason)
{
	llwarns << "LLVoiceCallCapResponder::error("
		<< status << ": " << reason << ")"
		<< llendl;
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
			llinfos << "LLVoiceCallCapResponder::result got " 
				<< iter->first << llendl;
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
	mIgnoreNextSessionLeave(FALSE)
{
	mNotifyArgs["VOICE_CHANNEL_NAME"] = mSessionName;

	if (!sVoiceChannelMap.insert(std::make_pair(session_id, this)).second)
	{
		// a voice channel already exists for this session id, so this instance will be orphaned
		// the end result should simply be the failure to make voice calls
		llwarns << "Duplicate voice channels registered for session_id " << session_id << llendl;
	}

	LLVoiceClient::getInstance()->addObserver(this);
}

LLVoiceChannel::~LLVoiceChannel()
{
	// Don't use LLVoiceClient::getInstance() here -- this can get called during atexit() time and that singleton MAY have already been destroyed.
	if(gVoiceClient)
	{
		gVoiceClient->removeObserver(this);
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
			llwarns << "Received empty URI for channel " << mSessionName << llendl;
			deactivate();
		}
		else if (mCredentials.empty())
		{
			LLNotificationsUtil::add("VoiceChannelJoinFailed", mNotifyArgs);
			llwarns << "Received empty credentials for channel " << mSessionName << llendl;
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
		// mute the microphone if required when returning to the proximal channel
		if (gSavedSettings.getBOOL("AutoDisengageMic") && sCurrentVoiceChannel == this)
		{
			gSavedSettings.setBOOL("PTTCurrentlyEnabled", true);
		}
	}

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
		gIMMgr->addSystemMessage(mSessionID, "ringing", mNotifyArgs);
		break;
	case STATE_CONNECTED:
		gIMMgr->addSystemMessage(mSessionID, "connected", mNotifyArgs);
		break;
	case STATE_HUNG_UP:
		gIMMgr->addSystemMessage(mSessionID, "hang_up", mNotifyArgs);
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
		mStateChangedCallback(old_state, mState, mCallDirection);
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
		if (gVoiceClient->voiceEnabled())
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

#if 0 // *TODO
		if (!gAgent.isInGroup(mSessionID)) // ad-hoc channel
		{
			// Add the party to the list of people with which we've recently interacted.
			for (/*people in the chat*/)
				LLRecentPeople::instance().add(buddy_id);
		}
#endif
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
			llwarns << "Received invalid credentials for channel " << mSessionName << llendl;
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
			gIMMgr->addSystemMessage(mSessionID, "ringing", mNotifyArgs);
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
	activate();
}

BOOL LLVoiceChannelProximal::isActive()
{
	return callStarted() && LLVoiceClient::getInstance()->inProximalChannel(); 
}

void LLVoiceChannelProximal::activate()
{
	if (callStarted()) return;

	LLVoiceChannel::activate();

	if (callStarted())
	{
		// this implicitly puts you back in the spatial channel
		LLVoiceClient::getInstance()->leaveNonSpatialChannel();
	}
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
		 gIMMgr->addSystemMessage(LLUUID::null, "unavailable", mNotifyArgs);
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
	llinfos << "P2P CALL CHANNEL STATUS CHANGE: incoming=" << int(mReceivedCall) << " newstatus=" << LLVoiceClientStatusObserver::status2string(type) << " (mState=" << mState << ")" << llendl;

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
				// other user hung up				
			}
			deactivate();
		}
		mIgnoreNextSessionLeave = FALSE;
		return;
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
			LLVoiceClient::getInstance()->answerInvite(mSessionHandle);
			
			// using the session handle invalidates it.  Clear it out here so we can't reuse it by accident.
			mSessionHandle.clear();
		}

		// Add the party to the list of people with which we've recently interacted.
		LLRecentPeople::instance().add(mOtherUserID);
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
		// In case of incoming AvaLine call generated URI will be differ from original one.
		// This is because Avatar-2-Avatar URI is based on avatar UUID but Avaline is not.
		// See LLVoiceClient::sessionAddedEvent() -> setUUIDFromStringHash()
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
	llinfos << "P2P CALL STATE CHANGE: incoming=" << int(mReceivedCall) << " oldstate=" << mState << " newstate=" << state << llendl;

	if (mReceivedCall) // incoming call
	{
		// you only "answer" voice invites in p2p mode
		// so provide a special purpose message here
		if (mReceivedCall && state == STATE_RINGING)
		{
			gIMMgr->addSystemMessage(mSessionID, "answering", mNotifyArgs);
			doSetState(state);
			return;
		}
	}

	LLVoiceChannel::setState(state);
}
