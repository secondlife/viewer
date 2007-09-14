/** 
 * @file llimpanel.cpp
 * @brief LLIMPanel class definition
 *
 * Copyright (c) 2001-$CurrentYear$, Linden Research, Inc.
 * $License$
 */

#include "llviewerprecompiledheaders.h"

#include "llimpanel.h"

#include "indra_constants.h"
#include "llfocusmgr.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llstring.h"
#include "message.h"
#include "lltextbox.h"

#include "llagent.h"
#include "llbutton.h"
#include "llcallingcard.h"
#include "llchat.h"
#include "llconsole.h"
#include "llfloater.h"
#include "llinventory.h"
#include "llinventorymodel.h"
#include "llinventoryview.h"
#include "llfloateravatarinfo.h"
#include "llfloaterchat.h"
#include "llkeyboard.h"
#include "lllineeditor.h"
#include "llresmgr.h"
#include "lltabcontainer.h"
#include "llimview.h"
#include "llviewertexteditor.h"
#include "llviewermessage.h"
#include "llviewerstats.h"
#include "viewer.h"
#include "llvieweruictrlfactory.h"
#include "lllogchat.h"
#include "llfloaterhtml.h"
#include "llweb.h"
#include "llhttpclient.h"
#include "llfloateractivespeakers.h" // LLSpeakerMgr
#include "llfloatergroupinfo.h"
#include "llsdutil.h"
#include "llnotify.h"
#include "llmutelist.h"

//
// Constants
//
const S32 LINE_HEIGHT = 16;
const S32 MIN_WIDTH = 200;
const S32 MIN_HEIGHT = 130;

//
// Statics
//
//
static LLString sTitleString = "Instant Message with [NAME]";
static LLString sTypingStartString = "[NAME]: ...";
static LLString sSessionStartString = "Starting session with [NAME] please wait.";

LLVoiceChannel::voice_channel_map_t LLVoiceChannel::sVoiceChannelMap;
LLVoiceChannel::voice_channel_map_uri_t LLVoiceChannel::sVoiceChannelURIMap;
LLVoiceChannel* LLVoiceChannel::sCurrentVoiceChannel = NULL;

void session_starter_helper(const LLUUID& temp_session_id,
							const LLUUID& other_participant_id,
							EInstantMessage im_type)
{
	LLMessageSystem *msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_ImprovedInstantMessage);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());

	msg->nextBlockFast(_PREHASH_MessageBlock);
	msg->addBOOLFast(_PREHASH_FromGroup, FALSE);
	msg->addUUIDFast(_PREHASH_ToAgentID, other_participant_id);
	msg->addU8Fast(_PREHASH_Offline, IM_ONLINE);
	msg->addU8Fast(_PREHASH_Dialog, im_type);
	msg->addUUIDFast(_PREHASH_ID, temp_session_id);
	msg->addU32Fast(_PREHASH_Timestamp, NO_TIMESTAMP); // no timestamp necessary

	std::string name;
	gAgent.buildFullname(name);

	msg->addStringFast(_PREHASH_FromAgentName, name);
	msg->addStringFast(_PREHASH_Message, LLString::null);
	msg->addU32Fast(_PREHASH_ParentEstateID, 0);
	msg->addUUIDFast(_PREHASH_RegionID, LLUUID::null);
	msg->addVector3Fast(_PREHASH_Position, gAgent.getPositionAgent());
}

// Returns true if any messages were sent, false otherwise.
// Is sort of equivalent to "does the server need to do anything?"
bool send_start_session_messages(const LLUUID& temp_session_id,
								 const LLUUID& other_participant_id,
								 const LLDynamicArray<LLUUID>& ids,
								 EInstantMessage dialog)
{
	if ( (dialog == IM_SESSION_GROUP_START) ||
		 (dialog == IM_SESSION_CONFERENCE_START) )
	{
		S32 count = ids.size();
		S32 bucket_size = UUID_BYTES * count;
		U8* bucket;
		U8* pos;

		session_starter_helper(temp_session_id,
							   other_participant_id,
							   dialog);

		switch(dialog)
		{
		case IM_SESSION_GROUP_START:
			gMessageSystem->addBinaryDataFast(_PREHASH_BinaryBucket,
											  EMPTY_BINARY_BUCKET,
											  EMPTY_BINARY_BUCKET_SIZE);
			break;
		case IM_SESSION_CONFERENCE_START:
			bucket = new U8[bucket_size];
			pos = bucket;

			// *FIX: this could suffer from endian issues
			for(S32 i = 0; i < count; ++i)
			{
				memcpy(pos, &(ids.get(i)), UUID_BYTES);
				pos += UUID_BYTES;
			}
			gMessageSystem->addBinaryDataFast(_PREHASH_BinaryBucket,
											  bucket,
											  bucket_size);
			delete[] bucket;

			break;
		default:
			break;
		}
		gAgent.sendReliableMessage();

		return true;
	}

	return false;
}

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
	if (channelp)
	{
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
LLVoiceChannel::LLVoiceChannel(const LLUUID& session_id, const LLString& session_name) : 
	mSessionID(session_id), 
	mState(STATE_NO_CHANNEL_INFO), 
	mSessionName(session_name),
	mIgnoreNextSessionLeave(FALSE)
{
	mNotifyArgs["[VOICE_CHANNEL_NAME]"] = mSessionName;

	if (!sVoiceChannelMap.insert(std::make_pair(session_id, this)).second)
	{
		// a voice channel already exists for this session id, so this instance will be orphaned
		// the end result should simply be the failure to make voice calls
		llwarns << "Duplicate voice channels registered for session_id " << session_id << llendl;
	}

	LLVoiceClient::getInstance()->addStatusObserver(this);
}

LLVoiceChannel::~LLVoiceChannel()
{
	// CANNOT do this here, since it will crash on quit in the LLVoiceChannelProximal singleton destructor.
	// Do it in all other subclass destructors instead.
	// deactivate();
	
	// Don't use LLVoiceClient::getInstance() here -- this can get called during atexit() time and that singleton MAY have already been destroyed.
	if(gVoiceClient)
	{
		gVoiceClient->removeStatusObserver(this);
	}
	
	sVoiceChannelMap.erase(mSessionID);
	sVoiceChannelURIMap.erase(mURI);
}

void LLVoiceChannel::setChannelInfo(
	const LLString& uri,
	const LLString& credentials)
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
		mLoginNotificationHandle = LLNotifyBox::showXml("VoiceLoginRetry")->getHandle();
		break;
	case STATUS_LOGGED_IN:
		if (!mLoginNotificationHandle.isDead())
		{
			LLNotifyBox* notifyp = (LLNotifyBox*)LLPanel::getPanelByHandle(mLoginNotificationHandle);
			if (notifyp)
			{
				notifyp->close();
			}
			mLoginNotificationHandle.markDead();
		}
		break;
	case STATUS_LEFT_CHANNEL:
		if (callStarted() && !mIgnoreNextSessionLeave)
		{
			// if forceably removed from channel
			// update the UI and revert to default channel
			LLNotifyBox::showXml("VoiceChannelDisconnected", mNotifyArgs);
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
		if (sCurrentVoiceChannel)
		{
			sCurrentVoiceChannel->deactivate();
		}
		sCurrentVoiceChannel = this;
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
LLVoiceChannel* LLVoiceChannel::getChannelByURI(LLString uri)
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

void LLVoiceChannel::setURI(LLString uri)
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


//static
void LLVoiceChannel::initClass()
{
	sCurrentVoiceChannel = LLVoiceChannelProximal::getInstance();
}

//
// LLVoiceChannelGroup
//

LLVoiceChannelGroup::LLVoiceChannelGroup(const LLUUID& session_id, const LLString& session_name) : 
	LLVoiceChannel(session_id, session_name)
{
}

LLVoiceChannelGroup::~LLVoiceChannelGroup()
{
	deactivate();
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

void LLVoiceChannelGroup::handleError(EStatusType status)
{
	std::string notify;
	switch(status)
	{
	  case ERROR_CHANNEL_LOCKED:
	  case ERROR_CHANNEL_FULL:
		notify = "VoiceChannelFull";
		break;
	  case ERROR_UNKNOWN:
		break;
	  default:
		break;
	}

	// notification
	if (!notify.empty())
	{
		LLNotifyBox::showXml(notify, mNotifyArgs);
		// echo to im window
		gIMMgr->addMessage(mSessionID, LLUUID::null, SYSTEM_FROM, LLNotifyBox::getTemplateMessage(notify, mNotifyArgs).c_str());
	}
	
	LLVoiceChannel::handleError(status);
}

//
// LLVoiceChannelProximal
//
LLVoiceChannelProximal::LLVoiceChannelProximal() : 
	LLVoiceChannel(LLUUID::null, LLString::null)
{
	activate();
}

LLVoiceChannelProximal::~LLVoiceChannelProximal()
{
	// DO NOT call deactivate() here, since this will only happen at atexit() time.	
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
		LLNotifyBox::showXml(notify, mNotifyArgs);
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
LLVoiceChannelP2P::LLVoiceChannelP2P(const LLUUID& session_id, const LLString& session_name, const LLUUID& other_user_id) : 
		LLVoiceChannelGroup(session_id, session_name), 
		mOtherUserID(other_user_id)
{
	// make sure URI reflects encoded version of other user's agent id
	setURI(LLVoiceClient::getInstance()->sipURIFromID(other_user_id));
}

LLVoiceChannelP2P::~LLVoiceChannelP2P() 
{
	deactivate();
}

void LLVoiceChannelP2P::handleStatusChange(EStatusType type)
{
	// status updates
	switch(type)
	{
	case STATUS_LEFT_CHANNEL:
		if (callStarted() && !mIgnoreNextSessionLeave)
		{
			if (mState == STATE_RINGING)
			{
				// other user declined call
				LLNotifyBox::showXml("P2PCallDeclined", mNotifyArgs);
			}
			else
			{
				// other user hung up
				LLNotifyBox::showXml("VoiceChannelDisconnectedP2P", mNotifyArgs);
			}
			deactivate();
		}
		mIgnoreNextSessionLeave = FALSE;
		return;
	default:
		break;
	}

	LLVoiceChannelGroup::handleStatusChange(type);
}

void LLVoiceChannelP2P::handleError(EStatusType type)
{
	switch(type)
	{
	case ERROR_NOT_AVAILABLE:
		LLNotifyBox::showXml("P2PCallNoAnswer", mNotifyArgs);
		break;
	default:
		break;
	}

	LLVoiceChannelGroup::handleError(type);
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
			LLVoiceClient::getInstance()->callUser(mOtherUserID);
		}
		// otherwise answering the call
		else
		{
			LLVoiceClient::getInstance()->answerInvite(mSessionHandle, mOtherUserID);
			// using the session handle invalidates it.  Clear it out here so we can't reuse it by accident.
			mSessionHandle.clear();
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
void LLVoiceChannelP2P::setSessionHandle(const LLString& handle)
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
	setURI(LLVoiceClient::getInstance()->sipURIFromID(mOtherUserID));
	
	if (needs_activate)
	{
		activate();
	}
}

//
// LLFloaterIMPanel
//
LLFloaterIMPanel::LLFloaterIMPanel(
	const std::string& name,
	const LLRect& rect,
	const std::string& session_label,
	const LLUUID& session_id,
	const LLUUID& other_participant_id,
	EInstantMessage dialog) :
	LLFloater(name, rect, session_label),
	mInputEditor(NULL),
	mHistoryEditor(NULL),
	mSessionUUID(session_id),
	mVoiceChannel(NULL),
	mSessionInitialized(FALSE),

	mOtherParticipantUUID(other_participant_id),
	mDialog(dialog),
	mTyping(FALSE),
	mOtherTyping(FALSE),
	mTypingLineStartIndex(0),
	mSentTypingState(TRUE),
	mShowSpeakersOnConnect(TRUE),
	mAutoConnect(FALSE),
	mSpeakerPanel(NULL),
	mFirstKeystrokeTimer(),
	mLastKeystrokeTimer()
{
	init(session_label);
}

LLFloaterIMPanel::LLFloaterIMPanel(
	const std::string& name,
	const LLRect& rect,
	const std::string& session_label,
	const LLUUID& session_id,
	const LLUUID& other_participant_id,
	const LLDynamicArray<LLUUID>& ids,
	EInstantMessage dialog) :
	LLFloater(name, rect, session_label),
	mInputEditor(NULL),
	mHistoryEditor(NULL),
	mSessionUUID(session_id),
	mVoiceChannel(NULL),
	mSessionInitialized(FALSE),
	mOtherParticipantUUID(other_participant_id),
	mDialog(dialog),
	mTyping(FALSE),
	mOtherTyping(FALSE),
	mTypingLineStartIndex(0),
	mSentTypingState(TRUE),
	mShowSpeakersOnConnect(TRUE),
	mAutoConnect(FALSE),
	mSpeakers(NULL),
	mSpeakerPanel(NULL),
	mFirstKeystrokeTimer(),
	mLastKeystrokeTimer()
{
	mSessionInitialTargetIDs = ids;
	init(session_label);
}


void LLFloaterIMPanel::init(const LLString& session_label)
{
	LLString xml_filename;
	switch(mDialog)
	{
	case IM_SESSION_GROUP_START:
		mFactoryMap["active_speakers_panel"] = LLCallbackMap(createSpeakersPanel, this);
		xml_filename = "floater_instant_message_group.xml";
		mVoiceChannel = new LLVoiceChannelGroup(mSessionUUID, session_label);
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
		mVoiceChannel = new LLVoiceChannelGroup(mSessionUUID, session_label);
		break;
	case IM_SESSION_P2P_INVITE:
		xml_filename = "floater_instant_message.xml";
		mVoiceChannel = new LLVoiceChannelP2P(mSessionUUID, session_label, mOtherParticipantUUID);
		break;
	case IM_SESSION_CONFERENCE_START:
		mFactoryMap["active_speakers_panel"] = LLCallbackMap(createSpeakersPanel, this);
		xml_filename = "floater_instant_message_ad_hoc.xml";
		mVoiceChannel = new LLVoiceChannelGroup(mSessionUUID, session_label);
		break;
	// just received text from another user
	case IM_NOTHING_SPECIAL:
		xml_filename = "floater_instant_message.xml";
		mVoiceChannel = new LLVoiceChannelP2P(mSessionUUID, session_label, mOtherParticipantUUID);
		break;
	default:
		llwarns << "Unknown session type" << llendl;
		xml_filename = "floater_instant_message.xml";
		break;
	}

	mSpeakers = new LLIMSpeakerMgr(mVoiceChannel);

	gUICtrlFactory->buildFloater(this,
								xml_filename,
								&getFactoryMap(),
								FALSE);

	setLabel(session_label);
	setTitle(session_label);
	mInputEditor->setMaxTextLength(1023);
	// enable line history support for instant message bar
	mInputEditor->setEnableLineHistory(TRUE);

	if ( gSavedPerAccountSettings.getBOOL("LogShowHistory") )
	{
		LLLogChat::loadHistory(session_label,
				       &chatFromLogFile,
				       (void *)this);
	}

	if ( !mSessionInitialized )
	{
		if ( !send_start_session_messages(
				 mSessionUUID,
				 mOtherParticipantUUID,
				 mSessionInitialTargetIDs,
				 mDialog) )
		{
			//we don't need to need to wait for any responses
			//so we're already initialized
			mSessionInitialized = TRUE;
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
				gSavedSettings.getColor4("SystemChatColor"),
				false);
		}
	}
}


LLFloaterIMPanel::~LLFloaterIMPanel()
{
	delete mSpeakers;
	mSpeakers = NULL;

	//kicks you out of the voice channel if it is currently active

	// HAVE to do this here -- if it happens in the LLVoiceChannel destructor it will call the wrong version (since the object's partially deconstructed at that point).
	mVoiceChannel->deactivate();
	
	delete mVoiceChannel;
	mVoiceChannel = NULL;
}

BOOL LLFloaterIMPanel::postBuild() 
{
	requires("chat_editor", WIDGET_TYPE_LINE_EDITOR);
	requires("im_history", WIDGET_TYPE_TEXT_EDITOR);
	requires("live_help_dialog", WIDGET_TYPE_TEXT_BOX);
	requires("title_string", WIDGET_TYPE_TEXT_BOX);
	requires("typing_start_string", WIDGET_TYPE_TEXT_BOX);
	requires("session_start_string", WIDGET_TYPE_TEXT_BOX);

	if (checkRequirements())
	{
		mInputEditor = LLUICtrlFactory::getLineEditorByName(this, "chat_editor");
		mInputEditor->setFocusReceivedCallback( onInputEditorFocusReceived );
		mInputEditor->setFocusLostCallback( onInputEditorFocusLost );
		mInputEditor->setKeystrokeCallback( onInputEditorKeystroke );
		mInputEditor->setCommitCallback( onCommitChat );
		mInputEditor->setCallbackUserData(this);
		mInputEditor->setCommitOnFocusLost( FALSE );
		mInputEditor->setRevertOnEsc( FALSE );

		childSetAction("profile_callee_btn", onClickProfile, this);
		childSetAction("group_info_btn", onClickGroupInfo, this);

		childSetAction("start_call_btn", onClickStartCall, this);
		childSetAction("end_call_btn", onClickEndCall, this);
		childSetAction("send_btn", onClickSend, this);
		childSetAction("toggle_active_speakers_btn", onClickToggleActiveSpeakers, this);
		childSetAction("offer_tp_btn", onClickOfferTeleport, this);

		//LLButton* close_btn = LLUICtrlFactory::getButtonByName(this, "close_btn");
		//close_btn->setClickedCallback(&LLFloaterIMPanel::onClickClose, this);

		mHistoryEditor = LLViewerUICtrlFactory::getViewerTextEditorByName(this, "im_history");
		mHistoryEditor->setParseHTML(TRUE);

		if ( IM_SESSION_GROUP_START == mDialog )
		{
			childSetEnabled("profile_btn", FALSE);
		}
		LLTextBox* title = LLUICtrlFactory::getTextBoxByName(this, "title_string");
		sTitleString = title->getText();
		
		LLTextBox* typing_start = LLUICtrlFactory::getTextBoxByName(this, "typing_start_string");
				
		sTypingStartString = typing_start->getText();

		LLTextBox* session_start = LLUICtrlFactory::getTextBoxByName(
			this,
			"session_start_string");
		sSessionStartString = session_start->getText();
		if (mSpeakerPanel)
		{
			mSpeakerPanel->refreshSpeakers();
		}

		if (mDialog == IM_NOTHING_SPECIAL)
		{
			childSetCommitCallback("mute_btn", onClickMuteVoice, this);
			childSetCommitCallback("speaker_volume", onVolumeChange, this);
		}

		setDefaultBtn("send_btn");
		return TRUE;
	}

	return FALSE;
}

void* LLFloaterIMPanel::createSpeakersPanel(void* data)
{
	LLFloaterIMPanel* floaterp = (LLFloaterIMPanel*)data;
	floaterp->mSpeakerPanel = new LLPanelActiveSpeakers(floaterp->mSpeakers, TRUE);
	return floaterp->mSpeakerPanel;
}

//static 
void LLFloaterIMPanel::onClickMuteVoice(LLUICtrl* source, void* user_data)
{
	LLFloaterIMPanel* floaterp = (LLFloaterIMPanel*)user_data;
	if (floaterp)
	{
		BOOL is_muted = gMuteListp->isMuted(floaterp->mOtherParticipantUUID, LLMute::flagVoiceChat);

		LLMute mute(floaterp->mOtherParticipantUUID, floaterp->getTitle(), LLMute::AGENT);
		if (!is_muted)
		{
			gMuteListp->add(mute, LLMute::flagVoiceChat);
		}
		else
		{
			gMuteListp->remove(mute, LLMute::flagVoiceChat);
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
					  && LLVoiceClient::voiceEnabled();

	// hide/show start call and end call buttons
	childSetVisible("end_call_btn", mVoiceChannel->getState() >= LLVoiceChannel::STATE_CALL_STARTED);
	childSetVisible("start_call_btn", mVoiceChannel->getState() < LLVoiceChannel::STATE_CALL_STARTED);
	childSetEnabled("start_call_btn", enable_connect);
	childSetEnabled("send_btn", !childGetValue("chat_editor").asString().empty());

	const LLRelationship* info = NULL;
	info = LLAvatarTracker::instance().getBuddyInfo(mOtherParticipantUUID);
	if (info)
	{
		childSetEnabled("offer_tp_btn", info->isOnline());
	}

	if (mAutoConnect && enable_connect)
	{
		onClickStartCall(this);
		mAutoConnect = FALSE;
	}

	// show speakers window when voice first connects
	if (mShowSpeakersOnConnect && mVoiceChannel->isActive())
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

	if (mSpeakerPanel)
	{
		mSpeakerPanel->refreshSpeakers();
	}
	else
	{
		// refresh volume and mute checkbox
		childSetEnabled("speaker_volume", mVoiceChannel->isActive());
		childSetValue("speaker_volume", gVoiceClient->getUserVolume(mOtherParticipantUUID));

		childSetValue("mute_btn", gMuteListp->isMuted(mOtherParticipantUUID, LLMute::flagVoiceChat));
		childSetEnabled("mute_btn", mVoiceChannel->isActive());
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

BOOL LLFloaterIMPanel::inviteToSession(const LLDynamicArray<LLUUID>& ids)
{
	LLViewerRegion* region = gAgent.getRegion();
	if (!region)
	{
		return FALSE;
	}
	
	S32 count = ids.count();

	if( isInviteAllowed() && (count > 0) )
	{
		llinfos << "LLFloaterIMPanel::inviteToSession() - inviting participants" << llendl;

		std::string url = region->getCapability("ChatSessionRequest");

		LLSD data;

		data["params"] = LLSD::emptyArray();
		for (int i = 0; i < count; i++)
		{
			data["params"].append(ids.get(i));
		}

		data["method"] = "invite";
		data["session-id"] = mSessionUUID;
		LLHTTPClient::post(
			url,
			data,
			new LLSessionInviteResponder(mSessionUUID));
		
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

void LLFloaterIMPanel::addHistoryLine(const LLUUID& source, const std::string &utf8msg, const LLColor4& color, bool log_to_file)
{
	addHistoryLine(utf8msg, color, log_to_file);
	mSpeakers->speakerChatted(source);
	mSpeakers->setSpeakerTyping(source, FALSE);
}

void LLFloaterIMPanel::addHistoryLine(const std::string &utf8msg, const LLColor4& color, bool log_to_file)
{
	LLMultiFloater* hostp = getHost();
	if( !getVisible() && hostp && log_to_file)
	{
		// Only flash for logged ("real") messages
		LLTabContainer* parent = (LLTabContainer*) getParent();
		parent->setTabPanelFlashing( this, TRUE );
	}

	// Now we're adding the actual line of text, so erase the 
	// "Foo is typing..." text segment, and the optional timestamp
	// if it was present. JC
	removeTypingIndicator(NULL);

	// Actually add the line
	LLString timestring;
	bool prepend_newline = true;
	if (gSavedSettings.getBOOL("IMShowTimestamps"))
	{
		timestring = mHistoryEditor->appendTime(prepend_newline);
		prepend_newline = false;
	}
	mHistoryEditor->appendColoredText(utf8msg, false, prepend_newline, color);
	
	if (log_to_file
		&& gSavedPerAccountSettings.getBOOL("LogInstantMessages") ) 
	{
		LLString histstr;
		if (gSavedPerAccountSettings.getBOOL("IMLogTimestamp"))
			histstr = LLLogChat::timestamp(gSavedPerAccountSettings.getBOOL("LogTimestampDate")) + utf8msg;
		else
			histstr = utf8msg;

		LLLogChat::saveHistory(getTitle(),histstr);
	}
}


void LLFloaterIMPanel::setVisible(BOOL b)
{
	LLPanel::setVisible(b);

	LLMultiFloater* hostp = getHost();
	if( b && hostp )
	{
		LLTabContainer* parent = (LLTabContainer*) getParent();

		// When this tab is displayed, you can stop flashing.
		parent->setTabPanelFlashing( this, FALSE );

		/* Don't change containing floater title - leave it "Instant Message" JC
		LLUIString title = sTitleString;
		title.setArg("[NAME]", mSessionLabel);
		hostp->setTitle( title );
		*/
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


BOOL LLFloaterIMPanel::handleKeyHere( KEY key, MASK mask, BOOL called_from_parent )
{
	BOOL handled = FALSE;
	if( getVisible() && mEnabled && !called_from_parent && gFocusMgr.childHasKeyboardFocus(this))
	{
		if( KEY_RETURN == key && mask == MASK_NONE)
		{
			sendMsg();
			handled = TRUE;

			// Close talk panels on hitting return
			// but not shift-return or control-return
			if ( !gSavedSettings.getBOOL("PinTalkViewOpen") && !(mask & MASK_CONTROL) && !(mask & MASK_SHIFT) )
			{
				gIMMgr->toggle(NULL);
			}
		}
		else if ( KEY_ESCAPE == key )
		{
			handled = TRUE;
			gFocusMgr.setKeyboardFocus(NULL, NULL);

			// Close talk panel with escape
			if( !gSavedSettings.getBOOL("PinTalkViewOpen") )
			{
				gIMMgr->toggle(NULL);
			}
		}
	}

	// May need to call base class LLPanel::handleKeyHere if not handled
	// in order to tab between buttons.  JNC 1.2.2002
	return handled;
}

BOOL LLFloaterIMPanel::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								  EDragAndDropType cargo_type,
								  void* cargo_data,
								  EAcceptance* accept,
								  LLString& tooltip_msg)
{
	BOOL accepted = FALSE;
	switch(cargo_type)
	{
	case DAD_CALLINGCARD:
		accepted = dropCallingCard((LLInventoryItem*)cargo_data, drop);
		break;
	case DAD_CATEGORY:
		accepted = dropCategory((LLInventoryCategory*)cargo_data, drop);
		break;
	default:
		break;
	}
	if (accepted)
	{
		*accept = ACCEPT_YES_MULTI;
	}
	else
	{
		*accept = ACCEPT_NO;
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
			LLDynamicArray<LLUUID> ids;
			ids.put(item->getCreatorUUID());
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
			LLDynamicArray<LLUUID> ids;
			for(S32 i = 0; i < count; ++i)
			{
				ids.put(items.get(i)->getCreatorUUID());
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
void LLFloaterIMPanel::onClickOfferTeleport(void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;

	handle_lure(self->mOtherParticipantUUID);
}

// static
void LLFloaterIMPanel::onClickProfile( void* userdata )
{
	//  Bring up the Profile window
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;
	
	if (self->mOtherParticipantUUID.notNull())
	{
		LLFloaterAvatarInfo::showFromDirectory(self->getOtherParticipantID());
	}
}

// static
void LLFloaterIMPanel::onClickGroupInfo( void* userdata )
{
	//  Bring up the Profile window
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;

	LLFloaterGroupInfo::showFromUUID(self->mSessionUUID);
}

// static
void LLFloaterIMPanel::onClickClose( void* userdata )
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;
	if(self)
	{
		self->close();
	}
}

// static
void LLFloaterIMPanel::onClickStartCall(void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;

	self->mVoiceChannel->activate();
}

// static
void LLFloaterIMPanel::onClickEndCall(void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;

	self->getVoiceChannel()->deactivate();
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
void LLFloaterIMPanel::onInputEditorFocusReceived( LLUICtrl* caller, void* userdata )
{
	LLFloaterIMPanel* self= (LLFloaterIMPanel*) userdata;
	self->mHistoryEditor->setCursorAndScrollToEnd();
}

// static
void LLFloaterIMPanel::onInputEditorFocusLost(LLUICtrl* caller, void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*) userdata;
	self->setTyping(FALSE);
}

// static
void LLFloaterIMPanel::onInputEditorKeystroke(LLLineEditor* caller, void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*)userdata;
	LLString text = self->mInputEditor->getText();
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

void LLFloaterIMPanel::onClose(bool app_quitting)
{
	setTyping(FALSE);

	if(mSessionUUID.notNull())
	{
		std::string name;
		gAgent.buildFullname(name);
		pack_instant_message(
			gMessageSystem,
			gAgent.getID(),
			FALSE,
			gAgent.getSessionID(),
			mOtherParticipantUUID,
			name.c_str(), 
			"",
			IM_ONLINE,
			IM_SESSION_LEAVE,
			mSessionUUID);
		gAgent.sendReliableMessage();
	}
	gIMMgr->removeSession(mSessionUUID);

	destroy();
}

void deliver_message(const std::string& utf8_text,
					 const LLUUID& im_session_id,
					 const LLUUID& other_participant_id,
					 EInstantMessage dialog)
{
	std::string name;
	gAgent.buildFullname(name);

	const LLRelationship* info = NULL;
	info = LLAvatarTracker::instance().getBuddyInfo(other_participant_id);
	U8 offline = (!info || info->isOnline()) ? IM_ONLINE : IM_OFFLINE;

	// default to IM_SESSION_SEND unless it's nothing special - in
	// which case it's probably an IM to everyone.
	U8 new_dialog = dialog;

	if ( dialog != IM_NOTHING_SPECIAL )
	{
		new_dialog = IM_SESSION_SEND;
	}

	pack_instant_message(
		gMessageSystem,
		gAgent.getID(),
		FALSE,
		gAgent.getSessionID(),
		other_participant_id,
		name.c_str(),
		utf8_text.c_str(),
		offline,
		(EInstantMessage)new_dialog,
		im_session_id);
	gAgent.sendReliableMessage();
}

void LLFloaterIMPanel::sendMsg()
{
	LLWString text = mInputEditor->getWText();
	LLWString::trim(text);
	if (!gAgent.isGodlike() 
		&& (mDialog == IM_NOTHING_SPECIAL)
		&& mOtherParticipantUUID.isNull())
	{
		llinfos << "Cannot send IM to everyone unless you're a god." << llendl;
		return;
	}
	if(text.length() > 0)
	{
		// Truncate and convert to UTF8 for transport
		std::string utf8_text = wstring_to_utf8str(text);
		utf8_text = utf8str_truncate(utf8_text, MAX_MSG_BUF_SIZE - 1);

		if ( mSessionInitialized )
		{
			deliver_message(utf8_text,
							mSessionUUID,
							mOtherParticipantUUID,
							mDialog);

			// local echo
			if((mDialog == IM_NOTHING_SPECIAL) && 
			   (mOtherParticipantUUID.notNull()))
			{
				std::string history_echo;
				gAgent.buildFullname(history_echo);

				// Look for IRC-style emotes here.
				char tmpstr[5];		/* Flawfinder: ignore */
				strncpy(tmpstr,
						utf8_text.substr(0,4).c_str(),
						sizeof(tmpstr) -1);		/* Flawfinder: ignore */
				tmpstr[sizeof(tmpstr) -1] = '\0';
				if (!strncmp(tmpstr, "/me ", 4) || 
					!strncmp(tmpstr, "/me'", 4))
				{
					utf8_text.replace(0,3,"");
				}
				else
				{
					history_echo += ": ";
				}
				history_echo += utf8_text;

				BOOL other_was_typing = mOtherTyping;

				addHistoryLine(gAgent.getID(), history_echo);

				if (other_was_typing) 
				{
					addTypingIndicator(mOtherTypingName);
				}

			}
		}
		else
		{
			//queue up the message to send once the session is
			//initialized
			mQueuedMsgsForInit.append(utf8_text);
		}

		gViewerStats->incStat(LLViewerStats::ST_IM_COUNT);
	}
	mInputEditor->setText("");

	// Don't need to actually send the typing stop message, the other
	// client will infer it from receiving the message.
	mTyping = FALSE;
	mSentTypingState = TRUE;
}

void LLFloaterIMPanel::updateSpeakersList(LLSD speaker_updates)
{ 
	mSpeakers->processSpeakerListUpdate(speaker_updates); 
}

void LLFloaterIMPanel::setSpeakersListFromMap(LLSD speaker_map)
{
	mSpeakers->processSpeakerMap(speaker_map);
}

void LLFloaterIMPanel::setSpeakersList(LLSD speaker_list)
{
	mSpeakers->processSpeakerList(speaker_list);
}

void LLFloaterIMPanel::sessionInitReplyReceived(const LLUUID& session_id)
{
	mSessionUUID = session_id;
	mVoiceChannel->updateSessionID(session_id);
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
		deliver_message(
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

		mSpeakers->setSpeakerTyping(gAgent.getID(), TRUE);
	}
	else
	{
		if (mTyping)
		{
			// you just stopped typing, send state immediately
			sendTypingState(FALSE);
			mSentTypingState = TRUE;
		}
		mSpeakers->setSpeakerTyping(gAgent.getID(), FALSE);
	}

	mTyping = typing;
}

void LLFloaterIMPanel::sendTypingState(BOOL typing)
{
	// Don't want to send typing indicators to multiple people, potentially too
	// much network traffic.  Only send in person-to-person IMs.
	if (mDialog != IM_NOTHING_SPECIAL) return;

	std::string name;
	gAgent.buildFullname(name);

	pack_instant_message(
		gMessageSystem,
		gAgent.getID(),
		FALSE,
		gAgent.getSessionID(),
		mOtherParticipantUUID,
		name.c_str(),
		"typing",
		IM_ONLINE,
		(typing ? IM_TYPING_START : IM_TYPING_STOP),
		mSessionUUID);
	gAgent.sendReliableMessage();
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
		addHistoryLine(typing_start, gSavedSettings.getColor4("SystemChatColor"), false);
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
			mSpeakers->setSpeakerTyping(im_info->mFromID, FALSE);
		}
	}
}

//static
void LLFloaterIMPanel::chatFromLogFile(LLString line, void* userdata)
{
	LLFloaterIMPanel* self = (LLFloaterIMPanel*)userdata;
	
	//self->addHistoryLine(line, LLColor4::grey, FALSE);
	self->mHistoryEditor->appendColoredText(line, false, true, LLColor4::grey);

}
