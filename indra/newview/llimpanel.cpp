/** 
 * @file llimpanel.cpp
 * @brief LLIMPanel class definition
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

#include "llimpanel.h"

#include "indra_constants.h"
#include "llfloaterreg.h"
#include "llfocusmgr.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llstring.h"
#include "message.h"
#include "lltextbox.h"

#include "llagent.h"
#include "llbutton.h"
#include "llbottomtray.h"
#include "llcallingcard.h"
#include "llchannelmanager.h"
#include "llchat.h"
#include "llchiclet.h"
#include "llconsole.h"
#include "llgroupactions.h"
#include "llfloater.h"
#include "llfloatercall.h"
#include "llavataractions.h"
#include "llimview.h"
#include "llinventory.h"
#include "llinventorymodel.h"
#include "llfloaterinventory.h"
#include "llfloaterchat.h"
#include "lliconctrl.h"
#include "llimview.h"                  // for LLIMModel to get other avatar id in chat
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llnotify.h"
#include "llpanelimcontrolpanel.h"
#include "llrecentpeople.h"
#include "llresmgr.h"
#include "lltrans.h"
#include "lltabcontainer.h"
#include "llviewertexteditor.h"
#include "llviewermessage.h"
#include "llviewerstats.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llviewerwindow.h"
#include "lllogchat.h"
#include "llweb.h"
#include "llhttpclient.h"
#include "llmutelist.h"
#include "llstylemap.h"
#include "llappviewer.h"

//
// Constants
//
const S32 LINE_HEIGHT = 16;
const S32 MIN_WIDTH = 200;
const S32 MIN_HEIGHT = 130;
const U32 DEFAULT_RETRIES_COUNT = 3;

//
// Statics
//
//
static std::string sTitleString = "Instant Message with [NAME]";
static std::string sTypingStartString = "[NAME]: ...";
static std::string sSessionStartString = "Starting session with [NAME] please wait.";

LLVoiceChannel::voice_channel_map_t LLVoiceChannel::sVoiceChannelMap;
LLVoiceChannel::voice_channel_map_uri_t LLVoiceChannel::sVoiceChannelURIMap;
LLVoiceChannel* LLVoiceChannel::sCurrentVoiceChannel = NULL;
LLVoiceChannel* LLVoiceChannel::sSuspendedVoiceChannel = NULL;

BOOL LLVoiceChannel::sSuspended = FALSE;



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
			LLNotifications::instance().add(
				"VoiceNotAllowed",
				channelp->getNotifyArgs());
		}
		else
		{
			LLNotifications::instance().add(
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
			LLNotifications::instance().add("VoiceChannelJoinFailed", mNotifyArgs);
			llwarns << "Received empty URI for channel " << mSessionName << llendl;
			deactivate();
		}
		else if (mCredentials.empty())
		{
			LLNotifications::instance().add("VoiceChannelJoinFailed", mNotifyArgs);
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
		LLNotifications::instance().add("VoiceLoginRetry");
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
			LLNotifications::instance().add("VoiceChannelDisconnected", mNotifyArgs);
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
		if (old_channel)
		{
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

	mState = state;
}

void LLVoiceChannel::toggleCallWindowIfNeeded(EState state)
{
	if (state == STATE_CONNECTED)
	{
		LLFloaterReg::showInstance("voice_call", mSessionID);
	}
	// By checking that current state is CONNECTED we make sure that the call window
	// has been shown, hence there's something to hide. This helps when user presses
	// the "End call" button right after initiating the call.
	// *TODO: move this check to LLFloaterCall?
	else if (state == STATE_HUNG_UP && mState == STATE_CONNECTED)
	{
		LLFloaterReg::hideInstance("voice_call", mSessionID);
	}
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
		LLNotificationPtr notification = LLNotifications::instance().add(notify, mNotifyArgs);
		// echo to im window
		gIMMgr->addMessage(mSessionID, LLUUID::null, SYSTEM_FROM, notification->getMessage());
	}

	LLVoiceChannel::handleError(status);
}

void LLVoiceChannelGroup::setState(EState state)
{
	// HACK: Open/close the call window if needed.
	toggleCallWindowIfNeeded(state);

	switch(state)
	{
	case STATE_RINGING:
		if ( !mIsRetrying )
		{
			gIMMgr->addSystemMessage(mSessionID, "ringing", mNotifyArgs);
		}

		mState = state;
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
		LLNotifications::instance().add(notify, mNotifyArgs);
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
	setURI(LLVoiceClient::getInstance()->sipURIFromID(other_user_id));
}

void LLVoiceChannelP2P::handleStatusChange(EStatusType type)
{
	// status updates
	switch(type)
	{
	case STATUS_LEFT_CHANNEL:
		if (callStarted() && !mIgnoreNextSessionLeave && !sSuspended)
		{
			if (mState == STATE_RINGING)
			{
				// other user declined call
				LLNotifications::instance().add("P2PCallDeclined", mNotifyArgs);
			}
			else
			{
				// other user hung up
				LLNotifications::instance().add("VoiceChannelDisconnectedP2P", mNotifyArgs);
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
		LLNotifications::instance().add("P2PCallNoAnswer", mNotifyArgs);
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
	// HACK: Open/close the call window if needed.
	toggleCallWindowIfNeeded(state);

	// you only "answer" voice invites in p2p mode
	// so provide a special purpose message here
	if (mReceivedCall && state == STATE_RINGING)
	{
		gIMMgr->addSystemMessage(mSessionID, "answering", mNotifyArgs);
		mState = state;
		return;
	}
	LLVoiceChannel::setState(state);
}


//
// LLFloaterIMPanel
//

LLFloaterIMPanel::LLFloaterIMPanel(const std::string& session_label,
								   const LLUUID& session_id,
								   const LLUUID& other_participant_id,
								   const std::vector<LLUUID>& ids,
								   EInstantMessage dialog)
:	LLFloater(session_id),
	mInputEditor(NULL),
	mHistoryEditor(NULL),
	mSessionUUID(session_id),
	mSessionLabel(session_label),
	mSessionInitialized(FALSE),
	mSessionStartMsgPos(0),
	mOtherParticipantUUID(other_participant_id),
	mDialog(dialog),
	mSessionInitialTargetIDs(ids),
	mTyping(FALSE),
	mOtherTyping(FALSE),
	mTypingLineStartIndex(0),
	mSentTypingState(TRUE),
	mNumUnreadMessages(0),
	mShowSpeakersOnConnect(TRUE),
	mAutoConnect(FALSE),
	mTextIMPossible(TRUE),
	mProfileButtonEnabled(TRUE),
	mCallBackEnabled(TRUE),
	mSpeakerPanel(NULL),
	mFirstKeystrokeTimer(),
	mLastKeystrokeTimer()
{
	std::string xml_filename;
	switch(mDialog)
	{
	case IM_SESSION_GROUP_START:
		mFactoryMap["active_speakers_panel"] = LLCallbackMap(createSpeakersPanel, this);
		xml_filename = "floater_instant_message_group.xml";
		break;
	case IM_SESSION_INVITE:
		mFactoryMap["active_speakers_panel"] = LLCallbackMap(createSpeakersPanel, this);
		if (gAgent.isInGroup(mSessionUUID))
		{
			xml_filename = "floater_instant_message_group.xml";
		}
		else // must be invite to ad hoc IM
		{
			xml_filename = "floater_instant_message_ad_hoc.xml";
		}
		break;
	case IM_SESSION_P2P_INVITE:
		xml_filename = "floater_instant_message.xml";
		break;
	case IM_SESSION_CONFERENCE_START:
		mFactoryMap["active_speakers_panel"] = LLCallbackMap(createSpeakersPanel, this);
		xml_filename = "floater_instant_message_ad_hoc.xml";
		break;
	// just received text from another user
	case IM_NOTHING_SPECIAL:

		xml_filename = "floater_instant_message.xml";
		
		mTextIMPossible = LLVoiceClient::getInstance()->isSessionTextIMPossible(mSessionUUID);
		mProfileButtonEnabled = LLVoiceClient::getInstance()->isParticipantAvatar(mSessionUUID);
		mCallBackEnabled = LLVoiceClient::getInstance()->isSessionCallBackPossible(mSessionUUID);
		break;
	default:
		llwarns << "Unknown session type" << llendl;
		xml_filename = "floater_instant_message.xml";
		break;
	}

	LLUICtrlFactory::getInstance()->buildFloater(this, xml_filename, NULL);

	setTitle(mSessionLabel);
	mInputEditor->setMaxTextLength(1023);
	// enable line history support for instant message bar
	mInputEditor->setEnableLineHistory(TRUE);

	if ( gSavedPerAccountSettings.getBOOL("LogShowHistory") )
	{
		LLLogChat::loadHistory(mSessionLabel,
				       &chatFromLogFile,
				       (void *)this);
	}

	if ( !mSessionInitialized )
	{
		if ( !LLIMModel::instance().sendStartSession(
				 mSessionUUID,
				 mOtherParticipantUUID,
				 mSessionInitialTargetIDs,
				 mDialog) )
		{
			//we don't need to need to wait for any responses
			//so we're already initialized
			mSessionInitialized = TRUE;
			mSessionStartMsgPos = 0;
		}
		else
		{
			//locally echo a little "starting session" message
			LLUIString session_start = sSessionStartString;

			session_start.setArg("[NAME]", getTitle());
			mSessionStartMsgPos = 
				mHistoryEditor->getWText().length();

			addHistoryLine(
				session_start,
				LLUIColorTable::instance().getColor("SystemChatColor"),
				false);
		}
	}
}


LLFloaterIMPanel::~LLFloaterIMPanel()
{
	//delete focus lost callback
	mFocusCallbackConnection.disconnect();
}

BOOL LLFloaterIMPanel::postBuild() 
{
	mCloseSignal.connect(boost::bind(&LLFloaterIMPanel::onClose, this));
	
	mVisibleSignal.connect(boost::bind(&LLFloaterIMPanel::onVisibilityChange, this, _2));
	
	mInputEditor = getChild<LLLineEditor>("chat_editor");
	mInputEditor->setFocusReceivedCallback( boost::bind(onInputEditorFocusReceived, _1, this) );
	mFocusCallbackConnection = mInputEditor->setFocusLostCallback( boost::bind(onInputEditorFocusLost, _1, this));
	mInputEditor->setKeystrokeCallback( onInputEditorKeystroke, this );
	mInputEditor->setCommitCallback( onCommitChat, this );
	mInputEditor->setCommitOnFocusLost( FALSE );
	mInputEditor->setRevertOnEsc( FALSE );
	mInputEditor->setReplaceNewlinesWithSpaces( FALSE );

	childSetAction("profile_callee_btn", onClickProfile, this);
	childSetAction("group_info_btn", onClickGroupInfo, this);

	childSetAction("start_call_btn", onClickStartCall, this);
	childSetAction("end_call_btn", onClickEndCall, this);
	childSetAction("send_btn", onClickSend, this);
	childSetAction("toggle_active_speakers_btn", onClickToggleActiveSpeakers, this);

	childSetAction("moderator_kick_speaker", onKickSpeaker, this);
	//LLButton* close_btn = getChild<LLButton>("close_btn");
	//close_btn->setClickedCallback(&LLFloaterIMPanel::onClickClose, this);

	mHistoryEditor = getChild<LLViewerTextEditor>("im_history");
	mHistoryEditor->setParseHTML(TRUE);
	mHistoryEditor->setParseHighlights(TRUE);

	if ( IM_SESSION_GROUP_START == mDialog )
	{
		childSetEnabled("profile_btn", FALSE);
	}
	
	if(!mProfileButtonEnabled)
	{
		childSetEnabled("profile_callee_btn", FALSE);
	}

	sTitleString = getString("title_string");
	sTypingStartString = getString("typing_start_string");
	sSessionStartString = getString("session_start_string");

	if (mSpeakerPanel)
	{
		mSpeakerPanel->refreshSpeakers();
	}

	if (mDialog == IM_NOTHING_SPECIAL)
	{
		childSetAction("mute_btn", onClickMuteVoice, this);
		childSetCommitCallback("speaker_volume", onVolumeChange, this);
	}

	setDefaultBtn("send_btn");
	return TRUE;
}

void* LLFloaterIMPanel::createSpeakersPanel(void* data)
{
	LLFloaterIMPanel* floaterp = (LLFloaterIMPanel*)data;
	LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(floaterp->mSessionUUID);
	floaterp->mSpeakerPanel = new LLPanelActiveSpeakers(speaker_mgr, TRUE);
	return floaterp->mSpeakerPanel;
}

//static 
void LLFloaterIMPanel::onClickMuteVoice(void* user_data)
{
	LLFloaterIMPanel* floaterp = (LLFloaterIMPanel*)user_data;
	if (floaterp)
	{
		BOOL is_muted = LLMuteList::getInstance()->isMuted(floaterp->mOtherParticipantUUID, LLMute::flagVoiceChat);

		LLMute mute(floaterp->mOtherParticipantUUID, floaterp->getTitle(), LLMute::AGENT);
		if (!is_muted)
		{
			LLMuteList::getInstance()->add(mute, LLMute::flagVoiceChat);
		}
		else
		{
			LLMuteList::getInstance()->remove(mute, LLMute::flagVoiceChat);
		}
	}
}

//static 
void LLFloaterIMPanel::onVolumeChange(LLUICtrl* source, void* user_data)
{
	LLFloaterIMPanel* floaterp = (LLFloaterIMPanel*)user_data;
	if (floaterp)
	{
		gVoiceClient->setUserVolume(floaterp->mOtherParticipantUUID, (F32)source->getValue().asReal());
	}
}


// virtual
void LLFloaterIMPanel::draw()
{	
	LLViewerRegion* region = gAgent.getRegion();
	
	BOOL enable_connect = (region && region->getCapability("ChatSessionRequest") != "")
					  && mSessionInitialized
					  && LLVoiceClient::voiceEnabled()
					  && mCallBackEnabled;

	// hide/show start call and end call buttons
	LLVoiceChannel* voice_channel = LLIMModel::getInstance()->getVoiceChannel(mSessionUUID);
	childSetVisible("end_call_btn", LLVoiceClient::voiceEnabled() && voice_channel->getState() >= LLVoiceChannel::STATE_CALL_STARTED);
	childSetVisible("start_call_btn", LLVoiceClient::voiceEnabled() && voice_channel->getState() < LLVoiceChannel::STATE_CALL_STARTED);
	childSetEnabled("start_call_btn", enable_connect);
	childSetEnabled("send_btn", !childGetValue("chat_editor").asString().empty());
	
	LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(mSessionUUID);
	LLPointer<LLSpeaker> self_speaker = speaker_mgr->findSpeaker(gAgent.getID());
	if(!mTextIMPossible)
	{
		mInputEditor->setEnabled(FALSE);
		mInputEditor->setLabel(getString("unavailable_text_label"));
	}
	else if (self_speaker.notNull() && self_speaker->mModeratorMutedText)
	{
		mInputEditor->setEnabled(FALSE);
		mInputEditor->setLabel(getString("muted_text_label"));
	}
	else
	{
		mInputEditor->setEnabled(TRUE);
		mInputEditor->setLabel(getString("default_text_label"));
	}

	if (mAutoConnect && enable_connect)
	{
		onClickStartCall(this);
		mAutoConnect = FALSE;
	}

	// show speakers window when voice first connects
	if (mShowSpeakersOnConnect && voice_channel->isActive())
	{
		childSetVisible("active_speakers_panel", TRUE);
		mShowSpeakersOnConnect = FALSE;
	}
	childSetValue("toggle_active_speakers_btn", childIsVisible("active_speakers_panel"));

	if (mTyping)
	{
		// Time out if user hasn't typed for a while.
		if (mLastKeystrokeTimer.getElapsedTimeF32() > LLAgent::TYPING_TIMEOUT_SECS)
		{
			setTyping(FALSE);
		}

		// If we are typing, and it's been a little while, send the
		// typing indicator
		if (!mSentTypingState
			&& mFirstKeystrokeTimer.getElapsedTimeF32() > 1.f)
		{
			sendTypingState(TRUE);
			mSentTypingState = TRUE;
		}
	}

	// use embedded panel if available
	if (mSpeakerPanel)
	{
		if (mSpeakerPanel->getVisible())
		{
			mSpeakerPanel->refreshSpeakers();
		}
	}
	else
	{
		// refresh volume and mute checkbox
		childSetVisible("speaker_volume", LLVoiceClient::voiceEnabled() && voice_channel->isActive());
		childSetValue("speaker_volume", gVoiceClient->getUserVolume(mOtherParticipantUUID));

		childSetValue("mute_btn", LLMuteList::getInstance()->isMuted(mOtherParticipantUUID, LLMute::flagVoiceChat));
		childSetVisible("mute_btn", LLVoiceClient::voiceEnabled() && voice_channel->isActive());
	}
	LLFloater::draw();
}

class LLSessionInviteResponder : public LLHTTPClient::Responder
{
public:
	LLSessionInviteResponder(const LLUUID& session_id)
	{
		mSessionID = session_id;
	}

	void error(U32 statusNum, const std::string& reason)
	{
		llinfos << "Error inviting all agents to session" << llendl;
		//throw something back to the viewer here?
	}

private:
	LLUUID mSessionID;
};

BOOL LLFloaterIMPanel::inviteToSession(const std::vector<LLUUID>& ids)
{
	LLViewerRegion* region = gAgent.getRegion();
	if (!region)
	{
		return FALSE;
	}
	
	S32 count = ids.size();

	if( isInviteAllowed() && (count > 0) )
	{
		llinfos << "LLFloaterIMPanel::inviteToSession() - inviting participants" << llendl;

		std::string url = region->getCapability("ChatSessionRequest");

		LLSD data;

		data["params"] = LLSD::emptyArray();
		for (int i = 0; i < count; i++)
		{
			data["params"].append(ids[i]);
		}

		data["method"] = "invite";
		data["session-id"] = mSessionUUID;
		LLHTTPClient::post(
			url,
			data,
			new LLSessionInviteResponder(
				mSessionUUID));		
	}
	else
	{
		llinfos << "LLFloaterIMPanel::inviteToSession -"
				<< " no need to invite agents for "
				<< mDialog << llendl;
		// successful add, because everyone that needed to get added
		// was added.
	}

	return TRUE;
}

void LLFloaterIMPanel::addHistoryLine(const std::string &utf8msg, const LLColor4& color, bool log_to_file, const LLUUID& source, const std::string& name)
{
	// start tab flashing when receiving im for background session from user
	if (source != LLUUID::null)
	{
		LLMultiFloater* hostp = getHost();
		if( !isInVisibleChain() 
			&& hostp 
			&& source != gAgent.getID())
		{
			hostp->setFloaterFlashing(this, TRUE);
		}
	}

	// Now we're adding the actual line of text, so erase the 
	// "Foo is typing..." text segment, and the optional timestamp
	// if it was present. JC
	removeTypingIndicator(NULL);

	// Actually add the line
	std::string timestring;
	bool prepend_newline = true;
	if (gSavedSettings.getBOOL("IMShowTimestamps"))
	{
		timestring = mHistoryEditor->appendTime(prepend_newline);
		prepend_newline = false;
	}

	std::string separator_string(": ");
	
	// 'name' is a sender name that we want to hotlink so that clicking on it opens a profile.
	if (!name.empty()) // If name exists, then add it to the front of the message.
	{
		// Don't hotlink any messages from the system (e.g. "Second Life:"), so just add those in plain text.
		if (name == SYSTEM_FROM)
		{
			mHistoryEditor->appendColoredText(name + separator_string, false, prepend_newline, color);
		}
		else
		{
			// Convert the name to a hotlink and add to message.
			mHistoryEditor->appendStyledText(name + separator_string, false, prepend_newline, LLStyleMap::instance().lookupAgent(source));
		}
		prepend_newline = false;
	}
	mHistoryEditor->appendColoredText(utf8msg, false, prepend_newline, color);
	S32 im_log_option =  gSavedPerAccountSettings.getS32("IMLogOptions");
	if (log_to_file && (im_log_option!=LOG_CHAT))
	{
		std::string histstr;
		if (gSavedPerAccountSettings.getBOOL("LogTimestamp"))
			histstr = LLLogChat::timestamp(gSavedPerAccountSettings.getBOOL("LogTimestampDate")) + name + separator_string + utf8msg;
		else
			histstr = name + separator_string + utf8msg;

		if(im_log_option==LOG_BOTH_TOGETHER)
		{
			LLLogChat::saveHistory(std::string("chat"),histstr);
		}
		else
		{
			LLLogChat::saveHistory(getTitle(),histstr);
		}
	}

	if (!isInVisibleChain())
	{
		mNumUnreadMessages++;
	}
}


void LLFloaterIMPanel::setInputFocus( BOOL b )
{
	mInputEditor->setFocus( b );
}


void LLFloaterIMPanel::selectAll()
{
	mInputEditor->selectAll();
}


void LLFloaterIMPanel::selectNone()
{
	mInputEditor->deselect();
}

BOOL LLFloaterIMPanel::handleKeyHere( KEY key, MASK mask )
{
	BOOL handled = FALSE;
	if( KEY_RETURN == key && mask == MASK_NONE)
	{
		sendMsg();
		handled = TRUE;
	}
	else if ( KEY_ESCAPE == key )
	{
		handled = TRUE;
		gFocusMgr.setKeyboardFocus(NULL);
	}

	// May need to call base class LLPanel::handleKeyHere if not handled
	// in order to tab between buttons.  JNC 1.2.2002
	return handled;
}

BOOL LLFloaterIMPanel::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								  EDragAndDropType cargo_type,
								  void* cargo_data,
								  EAcceptance* accept,
								  std::string& tooltip_msg)
{

	if (mDialog == IM_NOTHING_SPECIAL)
	{
		LLToolDragAndDrop::handleGiveDragAndDrop(mOtherParticipantUUID, mSessionUUID, drop,
												 cargo_type, cargo_data, accept);
	}
	
	// handle case for dropping calling cards (and folders of calling cards) onto invitation panel for invites
	else if (isInviteAllowed())
	{
		*accept = ACCEPT_NO;
		
		if (cargo_type == DAD_CALLINGCARD)
		{
			if (dropCallingCard((LLInventoryItem*)cargo_data, drop))
			{
				*accept = ACCEPT_YES_MULTI;
			}
		}
		else if (cargo_type == DAD_CATEGORY)
		{
			if (dropCategory((LLInventoryCategory*)cargo_data, drop))
			{
				*accept = ACCEPT_YES_MULTI;
			}
		}
	}
	return TRUE;
} 

BOOL LLFloaterIMPanel::dropCallingCard(LLInventoryItem* item, BOOL drop)
{
	BOOL rv = isInviteAllowed();
	if(rv && item && item->getCreatorUUID().notNull())
	{
		if(drop)
		{
			std::vector<LLUUID> ids;
			ids.push_back(item->getCreatorUUID());
			inviteToSession(ids);
		}
	}
	else
	{
		// set to false if creator uuid is null.
		rv = FALSE;
	}
	return rv;
}

BOOL LLFloaterIMPanel::dropCategory(LLInventoryCategory* category, BOOL drop)
{
	BOOL rv = isInviteAllowed();
	if(rv && category)
	{
		LLInventoryModel::cat_array_t cats;
		LLInventoryModel::item_array_t items;
		LLUniqueBuddyCollector buddies;
		gInventory.collectDescendentsIf(category->getUUID(),
										cats,
										items,
										LLInventoryModel::EXCLUDE_TRASH,
										buddies);
		S32 count = items.count();
		if(count == 0)
		{
			rv = FALSE;
		}
		else if(drop)
		{
			std::vector<LLUUID> ids;
			ids.reserve(count);
			for(S32 i = 0; i < count; ++i)
			{
				ids.push_back(items.get(i)->getCreatorUUID());
			}
			inviteToSession(ids);
		}
	}
	return rv;
}

BOOL LLFloaterIMPanel::isInviteAllowed() const
{

	return ( (IM_SESSION_CONFERENCE_START == mDialog) 
			 || (IM_SESSION_INVITE == mDialog) );
}


// static
void LLFloaterIMPanel::onTabClick(void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;
	self->setInputFocus(TRUE);
}


// static
void LLFloaterIMPanel::onClickProfile( void* userdata )
{
	//  Bring up the Profile window
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;
	
	if (self->getOtherParticipantID().notNull())
	{
		LLAvatarActions::showProfile(self->getOtherParticipantID());
	}
}

// static
void LLFloaterIMPanel::onClickGroupInfo( void* userdata )
{
	//  Bring up the Profile window
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;

	LLGroupActions::show(self->mSessionUUID);

}

// static
void LLFloaterIMPanel::onClickClose( void* userdata )
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;
	if(self)
	{
		self->closeFloater();
	}
}

// static
void LLFloaterIMPanel::onClickStartCall(void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;

	LLIMModel::getInstance()->getVoiceChannel(self->mSessionUUID)->activate();
}

// static
void LLFloaterIMPanel::onClickEndCall(void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;

	LLIMModel::getInstance()->getVoiceChannel(self->mSessionUUID)->deactivate();
}

// static
void LLFloaterIMPanel::onClickSend(void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*)userdata;
	self->sendMsg();
}

// static
void LLFloaterIMPanel::onClickToggleActiveSpeakers(void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*)userdata;

	self->childSetVisible("active_speakers_panel", !self->childIsVisible("active_speakers_panel"));
}

// static
void LLFloaterIMPanel::onCommitChat(LLUICtrl* caller, void* userdata)
{
	LLFloaterIMPanel* self= (LLFloaterIMPanel*) userdata;
	self->sendMsg();
}

// static
void LLFloaterIMPanel::onInputEditorFocusReceived( LLFocusableElement* caller, void* userdata )
{
	LLFloaterIMPanel* self= (LLFloaterIMPanel*) userdata;
	self->mHistoryEditor->setCursorAndScrollToEnd();
}

// static
void LLFloaterIMPanel::onInputEditorFocusLost(LLFocusableElement* caller, void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;
	self->setTyping(FALSE);
}

// static
void LLFloaterIMPanel::onInputEditorKeystroke(LLLineEditor* caller, void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*)userdata;
	std::string text = self->mInputEditor->getText();
	if (!text.empty())
	{
		self->setTyping(TRUE);
	}
	else
	{
		// Deleting all text counts as stopping typing.
		self->setTyping(FALSE);
	}
}

void LLFloaterIMPanel::onClose()
{
	setTyping(FALSE);

	LLIMModel::instance().sendLeaveSession(mSessionUUID, mOtherParticipantUUID);

	gIMMgr->removeSession(mSessionUUID);

	// *HACK hide the voice floater
	LLFloaterReg::hideInstance("voice_call", mSessionUUID);
}

void LLFloaterIMPanel::onVisibilityChange(const LLSD& new_visibility)
{
	if (new_visibility.asBoolean())
	{
		mNumUnreadMessages = 0;
	}
	
	LLVoiceChannel* voice_channel = LLIMModel::getInstance()->getVoiceChannel(mSessionUUID);
	if (new_visibility.asBoolean() && voice_channel->getState() == LLVoiceChannel::STATE_CONNECTED)
		LLFloaterReg::showInstance("voice_call", mSessionUUID);
	else
		LLFloaterReg::hideInstance("voice_call", mSessionUUID);
}

void LLFloaterIMPanel::sendMsg()
{
	if (!gAgent.isGodlike() 
		&& (mDialog == IM_NOTHING_SPECIAL)
		&& mOtherParticipantUUID.isNull())
	{
		llinfos << "Cannot send IM to everyone unless you're a god." << llendl;
		return;
	}

	if (mInputEditor)
	{
		LLWString text = mInputEditor->getConvertedText();
		if(!text.empty())
		{
			// Truncate and convert to UTF8 for transport
			std::string utf8_text = wstring_to_utf8str(text);
			utf8_text = utf8str_truncate(utf8_text, MAX_MSG_BUF_SIZE - 1);
			
			if ( mSessionInitialized )
			{
				LLIMModel::sendMessage(utf8_text,
								mSessionUUID,
								mOtherParticipantUUID,
								mDialog);

			}
			else
			{
				//queue up the message to send once the session is
				//initialized
				mQueuedMsgsForInit.append(utf8_text);
			}
		}

		LLViewerStats::getInstance()->incStat(LLViewerStats::ST_IM_COUNT);

		mInputEditor->setText(LLStringUtil::null);
	}

	// Don't need to actually send the typing stop message, the other
	// client will infer it from receiving the message.
	mTyping = FALSE;
	mSentTypingState = TRUE;
}

void LLFloaterIMPanel::processSessionUpdate(const LLSD& session_update)
{
	if (
		session_update.has("moderated_mode") &&
		session_update["moderated_mode"].has("voice") )
	{
		BOOL voice_moderated = session_update["moderated_mode"]["voice"];

		if (voice_moderated)
		{
			setTitle(mSessionLabel + std::string(" ") + getString("moderated_chat_label"));
		}
		else
		{
			setTitle(mSessionLabel);
		}


		//update the speakers dropdown too
		mSpeakerPanel->setVoiceModerationCtrlMode(voice_moderated);
	}
}

void LLFloaterIMPanel::sessionInitReplyReceived(const LLUUID& session_id)
{
	mSessionUUID = session_id;
	mSessionInitialized = TRUE;

	//we assume the history editor hasn't moved at all since
	//we added the starting session message
	//so, we count how many characters to remove
	S32 chars_to_remove = mHistoryEditor->getWText().length() -
		mSessionStartMsgPos;
	mHistoryEditor->removeTextFromEnd(chars_to_remove);

	//and now, send the queued msg
	LLSD::array_iterator iter;
	for ( iter = mQueuedMsgsForInit.beginArray();
		  iter != mQueuedMsgsForInit.endArray();
		  ++iter)
	{
		LLIMModel::sendMessage(
			iter->asString(),
			mSessionUUID,
			mOtherParticipantUUID,
			mDialog);
	}
}

void LLFloaterIMPanel::requestAutoConnect()
{
	mAutoConnect = TRUE;
}

void LLFloaterIMPanel::setTyping(BOOL typing)
{
	LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(mSessionUUID);
	if (typing)
	{
		// Every time you type something, reset this timer
		mLastKeystrokeTimer.reset();

		if (!mTyping)
		{
			// You just started typing.
			mFirstKeystrokeTimer.reset();

			// Will send typing state after a short delay.
			mSentTypingState = FALSE;
		}

		speaker_mgr->setSpeakerTyping(gAgent.getID(), TRUE);
	}
	else
	{
		if (mTyping)
		{
			// you just stopped typing, send state immediately
			sendTypingState(FALSE);
			mSentTypingState = TRUE;
		}
		speaker_mgr->setSpeakerTyping(gAgent.getID(), FALSE);
	}

	mTyping = typing;
}

void LLFloaterIMPanel::sendTypingState(BOOL typing)
{
	// Don't want to send typing indicators to multiple people, potentially too
	// much network traffic.  Only send in person-to-person IMs.
	if (mDialog != IM_NOTHING_SPECIAL) return;

	LLIMModel::instance().sendTypingState(mSessionUUID, mOtherParticipantUUID, typing);
}


void LLFloaterIMPanel::processIMTyping(const LLIMInfo* im_info, BOOL typing)
{
	if (typing)
	{
		// other user started typing
		addTypingIndicator(im_info->mName);
	}
	else
	{
		// other user stopped typing
		removeTypingIndicator(im_info);
	}
}


void LLFloaterIMPanel::addTypingIndicator(const std::string &name)
{
	// we may have lost a "stop-typing" packet, don't add it twice
	if (!mOtherTyping)
	{
		mTypingLineStartIndex = mHistoryEditor->getWText().length();
		LLUIString typing_start = sTypingStartString;
		typing_start.setArg("[NAME]", name);
		addHistoryLine(typing_start, LLUIColorTable::instance().getColor("SystemChatColor"), false);
		mOtherTypingName = name;
		mOtherTyping = TRUE;
	}
	// MBW -- XXX -- merge from release broke this (argument to this function changed from an LLIMInfo to a name)
	// Richard will fix.
//	mSpeakers->setSpeakerTyping(im_info->mFromID, TRUE);
}


void LLFloaterIMPanel::removeTypingIndicator(const LLIMInfo* im_info)
{
	if (mOtherTyping)
	{
		// Must do this first, otherwise addHistoryLine calls us again.
		mOtherTyping = FALSE;

		S32 chars_to_remove = mHistoryEditor->getWText().length() - mTypingLineStartIndex;
		mHistoryEditor->removeTextFromEnd(chars_to_remove);
		if (im_info)
		{
			LLIMModel::getInstance()->getSpeakerManager(mSessionUUID)->setSpeakerTyping(im_info->mFromID, FALSE);
		}
	}
}

//static
void LLFloaterIMPanel::chatFromLogFile(LLLogChat::ELogLineType type, std::string line, void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*)userdata;
	std::string message = line;
	S32 im_log_option =  gSavedPerAccountSettings.getS32("IMLogOptions");
	switch (type)
	{
	case LLLogChat::LOG_EMPTY:
		// add warning log enabled message
		if (im_log_option!=LOG_CHAT)
		{
			message = LLTrans::getString("IM_logging_string");
		}
		break;
	case LLLogChat::LOG_END:
		// add log end message
		if (im_log_option!=LOG_CHAT)
		{
			message = LLTrans::getString("IM_logging_string");
		}
		break;
	case LLLogChat::LOG_LINE:
		// just add normal lines from file
		break;
	default:
		// nothing
		break;
	}

	//self->addHistoryLine(line, LLColor4::grey, FALSE);
	self->mHistoryEditor->appendColoredText(message, false, true, LLUIColorTable::instance().getColor("ChatHistoryTextColor"));
}

void LLFloaterIMPanel::showSessionStartError(
	const std::string& error_string)
{
	LLSD args;
	args["REASON"] = LLTrans::getString(error_string);
	args["RECIPIENT"] = getTitle();

	LLSD payload;
	payload["session_id"] = mSessionUUID;

	LLNotifications::instance().add(
		"ChatterBoxSessionStartError",
		args,
		payload,
		onConfirmForceCloseError);
}

void LLFloaterIMPanel::showSessionEventError(
	const std::string& event_string,
	const std::string& error_string)
{
	LLSD args;
	args["REASON"] =
		LLTrans::getString(error_string);
	args["EVENT"] =
		LLTrans::getString(event_string);
	args["RECIPIENT"] = getTitle();

	LLNotifications::instance().add(
		"ChatterBoxSessionEventError",
		args);
}

void LLFloaterIMPanel::showSessionForceClose(
	const std::string& reason_string)
{
	LLSD args;

	args["NAME"] = getTitle();
	args["REASON"] = LLTrans::getString(reason_string);

	LLSD payload;
	payload["session_id"] = mSessionUUID;

	LLNotifications::instance().add(
		"ForceCloseChatterBoxSession",
		args,
		payload,
		LLFloaterIMPanel::onConfirmForceCloseError);

}

//static 
void LLFloaterIMPanel::onKickSpeaker(void* user_data)
{

}

bool LLFloaterIMPanel::onConfirmForceCloseError(const LLSD& notification, const LLSD& response)
{
	//only 1 option really
	LLUUID session_id = notification["payload"]["session_id"];

	if ( gIMMgr )
	{
		LLFloaterIMPanel* floaterp = gIMMgr->findFloaterBySession(
			session_id);

		if ( floaterp ) floaterp->closeFloater(FALSE);
	}
	return false;
}

