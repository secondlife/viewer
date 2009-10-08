/** 
 * @file LLIMMgr.cpp
 * @brief Container for Instant Messaging
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

#include "llimview.h"

#include "llfloaterreg.h"
#include "llfontgl.h"
#include "llrect.h"
#include "llerror.h"
#include "llbutton.h"
#include "llhttpclient.h"
#include "llsdutil.h"
#include "llstring.h"
#include "lluictrlfactory.h"

#include "llagent.h"
#include "llavatariconctrl.h"
#include "llcallingcard.h"
#include "llchat.h"
#include "llresmgr.h"
#include "llfloaterchat.h"
#include "llfloaterchatterbox.h"
#include "llavataractions.h"
#include "llhttpnode.h"
#include "llimfloater.h"
#include "llimpanel.h"
#include "llresizebar.h"
#include "lltabcontainer.h"
#include "llviewercontrol.h"
#include "llfloater.h"
#include "llmutelist.h"
#include "llresizehandle.h"
#include "llkeyboard.h"
#include "llui.h"
#include "llviewermenu.h"
#include "llcallingcard.h"
#include "lltoolbar.h"
#include "llviewermessage.h"
#include "llviewerwindow.h"
#include "llnotify.h"
#include "llviewerregion.h"
#include "lltrans.h"
#include "llrecentpeople.h"

#include "llfirstuse.h"
#include "llagentui.h"

//
// Globals
//
LLIMMgr* gIMMgr = NULL;

//
// Statics
//
// *FIXME: make these all either UIStrings or Strings


std::map<LLUUID, LLIMModel::LLIMSession*> LLIMModel::sSessionsMap;



void toast_callback(const LLSD& msg){
	// do not show toast in busy mode
	if (gAgent.getBusy())
	{
		return;
	}
	
	//we send notifications to reset counter also
	if (msg["num_unread"].asInteger())
	{
		LLSD args;
		args["MESSAGE"] = msg["message"];
		args["TIME"] = msg["time"];
		args["FROM"] = msg["from"];
		args["FROM_ID"] = msg["from_id"];
		args["SESSION_ID"] = msg["session_id"];

		LLNotifications::instance().add("IMToast", args, LLSD(), boost::bind(&LLIMFloater::show, msg["session_id"].asUUID()));
	}
}

LLIMModel::LLIMModel() 
{
	addChangedCallback(LLIMFloater::newIMCallback);
	addChangedCallback(toast_callback);
}


LLIMModel::LLIMSession::LLIMSession( const LLUUID& session_id, const std::string& name, const EInstantMessage& type, const LLUUID& other_participant_id )
:	mSessionID(session_id),
	mName(name),
	mType(type),
	mNumUnread(0),
	mOtherParticipantID(other_participant_id),
	mVoiceChannel(NULL),
	mSpeakers(NULL)
{
	if (IM_NOTHING_SPECIAL == type || IM_SESSION_P2P_INVITE == type)
	{
		mVoiceChannel  = new LLVoiceChannelP2P(session_id, name, other_participant_id);
	}
	else
	{
		mVoiceChannel = new LLVoiceChannelGroup(session_id, name);
	}
	mSpeakers = new LLIMSpeakerMgr(mVoiceChannel);

	// All participants will be added to the list of people we've recently interacted with.
	mSpeakers->addListener(&LLRecentPeople::instance(), "add");
}

LLIMModel::LLIMSession::~LLIMSession()
{
	delete mSpeakers;
	mSpeakers = NULL;

	// End the text IM session if necessary
	if(gVoiceClient && mOtherParticipantID.notNull())
	{
		switch(mType)
		{
		case IM_NOTHING_SPECIAL:
		case IM_SESSION_P2P_INVITE:
			gVoiceClient->endUserIMSession(mOtherParticipantID);
			break;

		default:
			// Appease the linux compiler
			break;
		}
	}

	// HAVE to do this here -- if it happens in the LLVoiceChannel destructor it will call the wrong version (since the object's partially deconstructed at that point).
	mVoiceChannel->deactivate();
	
	delete mVoiceChannel;
	mVoiceChannel = NULL;
}

LLIMModel::LLIMSession* LLIMModel::findIMSession(const LLUUID& session_id) const
{
	return get_if_there(LLIMModel::instance().sSessionsMap, session_id,
		(LLIMModel::LLIMSession*) NULL);
}

void LLIMModel::updateSessionID(const LLUUID& old_session_id, const LLUUID& new_session_id)
{
	if (new_session_id == old_session_id) return;

	LLIMSession* session = findIMSession(old_session_id);
	if (session)
	{
		session->mSessionID = new_session_id;
		session->mVoiceChannel->updateSessionID(new_session_id);

		//*TODO set session initialized flag here? (IB)

		sSessionsMap.erase(old_session_id);
		sSessionsMap[new_session_id] = session;
	}
}

void LLIMModel::testMessages()
{
	LLUUID bot1_id("d0426ec6-6535-4c11-a5d9-526bb0c654d9");
	LLUUID bot1_session_id;
	std::string from = "IM Tester";

	bot1_session_id = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, bot1_id);
	newSession(bot1_session_id, from, IM_NOTHING_SPECIAL, bot1_id);
	addMessage(bot1_session_id, from, bot1_id, "Test Message: Hi from testerbot land!");

	LLUUID bot2_id;
	std::string firstname[] = {"Roflcopter", "Joe"};
	std::string lastname[] = {"Linden", "Tester", "Resident", "Schmoe"};

	S32 rand1 = ll_rand(sizeof firstname)/(sizeof firstname[0]);
	S32 rand2 = ll_rand(sizeof lastname)/(sizeof lastname[0]);
	
	from = firstname[rand1] + " " + lastname[rand2];
	bot2_id.generate(from);
	LLUUID bot2_session_id = LLIMMgr::computeSessionID(IM_NOTHING_SPECIAL, bot2_id);
	newSession(bot2_session_id, from, IM_NOTHING_SPECIAL, bot2_id);
	addMessage(bot2_session_id, from, bot2_id, "Test Message: Can I haz bear? ");
	addMessage(bot2_session_id, from, bot2_id, "Test Message: OMGWTFBBQ.");
}


bool LLIMModel::newSession(LLUUID session_id, std::string name, EInstantMessage type, LLUUID other_participant_id)
{
	if (is_in_map(sSessionsMap, session_id))
	{
		llwarns << "IM Session " << session_id << " already exists" << llendl;
		return false;
	}

	LLIMSession* session = new LLIMSession(session_id, name, type, other_participant_id);
	sSessionsMap[session_id] = session;

	LLIMMgr::getInstance()->notifyObserverSessionAdded(session_id, name, other_participant_id);

	return true;

}

bool LLIMModel::clearSession(LLUUID session_id)
{
	if (sSessionsMap.find(session_id) == sSessionsMap.end()) return false;
	delete (sSessionsMap[session_id]);
	sSessionsMap.erase(session_id);
	return true;
}

//*TODO remake it, instead of returing the list pass it as as parameter (IB)
std::list<LLSD> LLIMModel::getMessages(LLUUID session_id, int start_index)
{
	std::list<LLSD> return_list;

	LLIMSession* session = findIMSession(session_id);
	if (!session) 
	{
		llwarns << "session " << session_id << "does not exist " << llendl;
		return return_list;
	}

	int i = session->mMsgs.size() - start_index;

	for (std::list<LLSD>::iterator iter = session->mMsgs.begin(); 
		iter != session->mMsgs.end() && i > 0;
		iter++)
	{
		LLSD msg;
		msg = *iter;
		return_list.push_back(*iter);
		i--;
	}

	session->mNumUnread = 0;
	
	LLSD arg;
	arg["session_id"] = session_id;
	arg["num_unread"] = 0;
	mChangedSignal(arg);

    // TODO: in the future is there a more efficient way to return these
	//of course there is - return as parameter (IB)
	return return_list;

}

bool LLIMModel::addToHistory(LLUUID session_id, std::string from, std::string utf8_text) { 
	
	LLIMSession* session = findIMSession(session_id);

	if (!session) 
	{
		llwarns << "session " << session_id << "does not exist " << llendl;
		return false;
	}

	LLSD message;
	message["from"] = from;
	message["message"] = utf8_text;
	message["time"] = LLLogChat::timestamp(false);  //might want to add date separately
	message["index"] = (LLSD::Integer)session->mMsgs.size(); 

	session->mMsgs.push_front(message); 

	return true;

}

		
bool LLIMModel::addMessage(LLUUID session_id, std::string from, LLUUID from_id, std::string utf8_text) { 

	LLIMSession* session = findIMSession(session_id);

	if (!session) 
	{
		llwarns << "session " << session_id << "does not exist " << llendl;
		return false;
	}

	addToHistory(session_id, from, utf8_text);

	std::string agent_name;
	LLAgentUI::buildFullname(agent_name);
	
	session->mNumUnread++;

	// notify listeners
	LLSD arg;
	arg["session_id"] = session_id;
	arg["num_unread"] = session->mNumUnread;
	arg["message"] = utf8_text;
	arg["from"] = from;
	arg["from_id"] = from_id;
	arg["time"] = LLLogChat::timestamp(false);
	mChangedSignal(arg);

	return true;
}


const std::string& LLIMModel::getName(const LLUUID& session_id) const
{
	LLIMSession* session = findIMSession(session_id);

	if (!session) 
	{
		llwarns << "session " << session_id << "does not exist " << llendl;
		return LLStringUtil::null;
	}

	return session->mName;
}

const S32 LLIMModel::getNumUnread(const LLUUID& session_id) const
{
	LLIMSession* session = findIMSession(session_id);
	if (!session)
	{
		llwarns << "session " << session_id << "does not exist " << llendl;
		return -1;
	}

	return session->mNumUnread;
}

const LLUUID& LLIMModel::getOtherParticipantID(const LLUUID& session_id) const
{
	LLIMSession* session = findIMSession(session_id);
	if (!session)
	{
		llwarns << "session " << session_id << "does not exist " << llendl;
		return LLUUID::null;
	}

	return session->mOtherParticipantID;
}

EInstantMessage LLIMModel::getType(const LLUUID& session_id) const
{
	LLIMSession* session = findIMSession(session_id);
	if (!session)
	{
		llwarns << "session " << session_id << "does not exist " << llendl;
		return IM_COUNT;
	}

	return session->mType;
}

LLVoiceChannel* LLIMModel::getVoiceChannel( const LLUUID& session_id ) const
{
	LLIMSession* session = findIMSession(session_id);
	if (!session)
	{
		llwarns << "session " << session_id << "does not exist " << llendl;
		return NULL;
	}

	return session->mVoiceChannel;
}

LLIMSpeakerMgr* LLIMModel::getSpeakerManager( const LLUUID& session_id ) const
{
	LLIMSession* session = findIMSession(session_id);
	if (!session)
	{
		llwarns << "session " << session_id << "does not exist " << llendl;
		return NULL;
	}

	return session->mSpeakers;
}


// TODO get rid of other participant ID
void LLIMModel::sendTypingState(LLUUID session_id, LLUUID other_participant_id, BOOL typing) 
{
	std::string name;
	LLAgentUI::buildFullname(name);

	pack_instant_message(
		gMessageSystem,
		gAgent.getID(),
		FALSE,
		gAgent.getSessionID(),
		other_participant_id,
		name,
		std::string("typing"),
		IM_ONLINE,
		(typing ? IM_TYPING_START : IM_TYPING_STOP),
		session_id);
	gAgent.sendReliableMessage();
}

void LLIMModel::sendLeaveSession(LLUUID session_id, LLUUID other_participant_id) 
{
	if(session_id.notNull())
	{
		std::string name;
		LLAgentUI::buildFullname(name);
		pack_instant_message(
			gMessageSystem,
			gAgent.getID(),
			FALSE,
			gAgent.getSessionID(),
			other_participant_id,
			name, 
			LLStringUtil::null,
			IM_ONLINE,
			IM_SESSION_LEAVE,
			session_id);
		gAgent.sendReliableMessage();
	}
}


//*TODO update list of messages in a LLIMSession (IB)
void LLIMModel::sendMessage(const std::string& utf8_text,
					 const LLUUID& im_session_id,
					 const LLUUID& other_participant_id,
					 EInstantMessage dialog)
{
	std::string name;
	bool sent = false;
	LLAgentUI::buildFullname(name);

	const LLRelationship* info = NULL;
	info = LLAvatarTracker::instance().getBuddyInfo(other_participant_id);
	
	U8 offline = (!info || info->isOnline()) ? IM_ONLINE : IM_OFFLINE;
	
	if((offline == IM_OFFLINE) && (LLVoiceClient::getInstance()->isOnlineSIP(other_participant_id)))
	{
		// User is online through the OOW connector, but not with a regular viewer.  Try to send the message via SLVoice.
		sent = gVoiceClient->sendTextMessage(other_participant_id, utf8_text);
	}
	
	if(!sent)
	{
		// Send message normally.

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

	// If there is a mute list and this is not a group chat...
	if ( LLMuteList::getInstance() )
	{
		// ... the target should not be in our mute list for some message types.
		// Auto-remove them if present.
		switch( dialog )
		{
		case IM_NOTHING_SPECIAL:
		case IM_GROUP_INVITATION:
		case IM_INVENTORY_OFFERED:
		case IM_SESSION_INVITE:
		case IM_SESSION_P2P_INVITE:
		case IM_SESSION_CONFERENCE_START:
		case IM_SESSION_SEND: // This one is marginal - erring on the side of hearing.
		case IM_LURE_USER:
		case IM_GODLIKE_LURE_USER:
		case IM_FRIENDSHIP_OFFERED:
			LLMuteList::getInstance()->autoRemove(other_participant_id, LLMuteList::AR_IM);
			break;
		default: ; // do nothing
		}
	}

	if((dialog == IM_NOTHING_SPECIAL) && 
	   (other_participant_id.notNull()))
	{
		// Do we have to replace the /me's here?
		std::string from;
		LLAgentUI::buildFullname(from);
		LLIMModel::instance().addToHistory(im_session_id, from, utf8_text);

		//local echo for the legacy communicate panel
		std::string history_echo;
		std::string utf8_copy = utf8_text;
		LLAgentUI::buildFullname(history_echo);

		// Look for IRC-style emotes here.

		std::string prefix = utf8_copy.substr(0, 4);
		if (prefix == "/me " || prefix == "/me'")
		{
			utf8_copy.replace(0,3,"");
		}
		else
		{
			history_echo += ": ";
		}
		history_echo += utf8_copy;
		
		LLFloaterIMPanel* floater = gIMMgr->findFloaterBySession(im_session_id);
		if (floater) floater->addHistoryLine(history_echo, LLUIColorTable::instance().getColor("IMChatColor"), true, gAgent.getID());

		LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(im_session_id);
		if (speaker_mgr)
		{
			speaker_mgr->speakerChatted(gAgentID);
			speaker_mgr->setSpeakerTyping(gAgentID, FALSE);
		}
	}

	// Add the recipient to the recent people list.
	//*TODO should be deleted, because speaker manager updates through callback the recent list
	LLRecentPeople::instance().add(other_participant_id);
}
										  
boost::signals2::connection LLIMModel::addChangedCallback( boost::function<void (const LLSD& data)> cb )
{
	return mChangedSignal.connect(cb);
}

void session_starter_helper(
	const LLUUID& temp_session_id,
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
	LLAgentUI::buildFullname(name);

	msg->addStringFast(_PREHASH_FromAgentName, name);
	msg->addStringFast(_PREHASH_Message, LLStringUtil::null);
	msg->addU32Fast(_PREHASH_ParentEstateID, 0);
	msg->addUUIDFast(_PREHASH_RegionID, LLUUID::null);
	msg->addVector3Fast(_PREHASH_Position, gAgent.getPositionAgent());
}

void start_deprecated_conference_chat(
	const LLUUID& temp_session_id,
	const LLUUID& creator_id,
	const LLUUID& other_participant_id,
	const LLSD& agents_to_invite)
{
	U8* bucket;
	U8* pos;
	S32 count;
	S32 bucket_size;

	// *FIX: this could suffer from endian issues
	count = agents_to_invite.size();
	bucket_size = UUID_BYTES * count;
	bucket = new U8[bucket_size];
	pos = bucket;

	for(S32 i = 0; i < count; ++i)
	{
		LLUUID agent_id = agents_to_invite[i].asUUID();
		
		memcpy(pos, &agent_id, UUID_BYTES);
		pos += UUID_BYTES;
	}

	session_starter_helper(
		temp_session_id,
		other_participant_id,
		IM_SESSION_CONFERENCE_START);

	gMessageSystem->addBinaryDataFast(
		_PREHASH_BinaryBucket,
		bucket,
		bucket_size);

	gAgent.sendReliableMessage();
 
	delete[] bucket;
}

class LLStartConferenceChatResponder : public LLHTTPClient::Responder
{
public:
	LLStartConferenceChatResponder(
		const LLUUID& temp_session_id,
		const LLUUID& creator_id,
		const LLUUID& other_participant_id,
		const LLSD& agents_to_invite)
	{
		mTempSessionID = temp_session_id;
		mCreatorID = creator_id;
		mOtherParticipantID = other_participant_id;
		mAgents = agents_to_invite;
	}

	virtual void error(U32 statusNum, const std::string& reason)
	{
		//try an "old school" way.
		if ( statusNum == 400 )
		{
			start_deprecated_conference_chat(
				mTempSessionID,
				mCreatorID,
				mOtherParticipantID,
				mAgents);
		}

		//else throw an error back to the client?
		//in theory we should have just have these error strings
		//etc. set up in this file as opposed to the IMMgr,
		//but the error string were unneeded here previously
		//and it is not worth the effort switching over all
		//the possible different language translations
	}

private:
	LLUUID mTempSessionID;
	LLUUID mCreatorID;
	LLUUID mOtherParticipantID;

	LLSD mAgents;
};

// Returns true if any messages were sent, false otherwise.
// Is sort of equivalent to "does the server need to do anything?"
bool LLIMModel::sendStartSession(
	const LLUUID& temp_session_id,
	const LLUUID& other_participant_id,
	const std::vector<LLUUID>& ids,
	EInstantMessage dialog)
{
	if ( dialog == IM_SESSION_GROUP_START )
	{
		session_starter_helper(
			temp_session_id,
			other_participant_id,
			dialog);

		switch(dialog)
		{
		case IM_SESSION_GROUP_START:
			gMessageSystem->addBinaryDataFast(
				_PREHASH_BinaryBucket,
				EMPTY_BINARY_BUCKET,
				EMPTY_BINARY_BUCKET_SIZE);
			break;
		default:
			break;
		}
		gAgent.sendReliableMessage();

		return true;
	}
	else if ( dialog == IM_SESSION_CONFERENCE_START )
	{
		LLSD agents;
		for (int i = 0; i < (S32) ids.size(); i++)
		{
			agents.append(ids[i]);
		}

		//we have a new way of starting conference calls now
		LLViewerRegion* region = gAgent.getRegion();
		if (region)
		{
			std::string url = region->getCapability(
				"ChatSessionRequest");
			LLSD data;
			data["method"] = "start conference";
			data["session-id"] = temp_session_id;

			data["params"] = agents;

			LLHTTPClient::post(
				url,
				data,
				new LLStartConferenceChatResponder(
					temp_session_id,
					gAgent.getID(),
					other_participant_id,
					data["params"]));
		}
		else
		{
			start_deprecated_conference_chat(
				temp_session_id,
				gAgent.getID(),
				other_participant_id,
				agents);
		}
	}

	return false;
}



//
// Helper Functions
//

class LLViewerChatterBoxInvitationAcceptResponder :
	public LLHTTPClient::Responder
{
public:
	LLViewerChatterBoxInvitationAcceptResponder(
		const LLUUID& session_id,
		LLIMMgr::EInvitationType invitation_type)
	{
		mSessionID = session_id;
		mInvitiationType = invitation_type;
	}

	void result(const LLSD& content)
	{
		if ( gIMMgr)
		{
			LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(mSessionID);
			if (speaker_mgr)
			{
				//we've accepted our invitation
				//and received a list of agents that were
				//currently in the session when the reply was sent
				//to us.  Now, it is possible that there were some agents
				//to slip in/out between when that message was sent to us
				//and now.

				//the agent list updates we've received have been
				//accurate from the time we were added to the session
				//but unfortunately, our base that we are receiving here
				//may not be the most up to date.  It was accurate at
				//some point in time though.
				speaker_mgr->setSpeakers(content);

				//we now have our base of users in the session
				//that was accurate at some point, but maybe not now
				//so now we apply all of the udpates we've received
				//in case of race conditions
				speaker_mgr->updateSpeakers(gIMMgr->getPendingAgentListUpdates(mSessionID));
			}
			
			LLFloaterIMPanel* floaterp =
				gIMMgr->findFloaterBySession(mSessionID);

			if (floaterp)
			{
				if ( mInvitiationType == LLIMMgr::INVITATION_TYPE_VOICE )
				{
					floaterp->requestAutoConnect();
					LLFloaterIMPanel::onClickStartCall(floaterp);
					// always open IM window when connecting to voice
					LLFloaterReg::showInstance("communicate", LLSD(), TRUE);
				}
				else if ( mInvitiationType == LLIMMgr::INVITATION_TYPE_IMMEDIATE )
				{
					LLFloaterReg::showInstance("communicate", LLSD(), TRUE);
				}
			}

			gIMMgr->clearPendingAgentListUpdates(mSessionID);
			gIMMgr->clearPendingInvitation(mSessionID);
		}
	}

	void error(U32 statusNum, const std::string& reason)
	{		
		//throw something back to the viewer here?
		if ( gIMMgr )
		{
			gIMMgr->clearPendingAgentListUpdates(mSessionID);
			gIMMgr->clearPendingInvitation(mSessionID);

			LLFloaterIMPanel* floaterp =
				gIMMgr->findFloaterBySession(mSessionID);

			if ( floaterp )
			{
				if ( 404 == statusNum )
				{
					std::string error_string;
					error_string = "does not exist";

					floaterp->showSessionStartError(
						error_string);
				}
			}
		}
	}

private:
	LLUUID mSessionID;
	LLIMMgr::EInvitationType mInvitiationType;
};


// the other_participant_id is either an agent_id, a group_id, or an inventory
// folder item_id (collection of calling cards)

// static
LLUUID LLIMMgr::computeSessionID(
	EInstantMessage dialog,
	const LLUUID& other_participant_id)
{
	LLUUID session_id;
	if (IM_SESSION_GROUP_START == dialog)
	{
		// slam group session_id to the group_id (other_participant_id)
		session_id = other_participant_id;
	}
	else if (IM_SESSION_CONFERENCE_START == dialog)
	{
		session_id.generate();
	}
	else if (IM_SESSION_INVITE == dialog)
	{
		// use provided session id for invites
		session_id = other_participant_id;
	}
	else
	{
		LLUUID agent_id = gAgent.getID();
		if (other_participant_id == agent_id)
		{
			// if we try to send an IM to ourselves then the XOR would be null
			// so we just make the session_id the same as the agent_id
			session_id = agent_id;
		}
		else
		{
			// peer-to-peer or peer-to-asset session_id is the XOR
			session_id = other_participant_id ^ agent_id;
		}
	}
	return session_id;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLIncomingCallDialog
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
LLIncomingCallDialog::LLIncomingCallDialog(const LLSD& payload) :
	LLModalDialog(payload),
	mPayload(payload)
{
}

BOOL LLIncomingCallDialog::postBuild()
{
	LLSD caller_id = mPayload["caller_id"];
	EInstantMessage type = (EInstantMessage)mPayload["type"].asInteger();

	std::string call_type = getString("VoiceInviteP2P");
	std::string caller_name = mPayload["caller_name"].asString();
	if (caller_name == "anonymous")
	{
		caller_name = getString("anonymous");
	}
	
	setTitle(caller_name + " " + call_type);
	
	// If it is not a P2P invite, then it's an AdHoc invite
	if ( type != IM_SESSION_P2P_INVITE )
	{
		call_type = getString("VoiceInviteAdHoc");
	}

	LLUICtrl* caller_name_widget = getChild<LLUICtrl>("caller name");
	caller_name_widget->setValue(caller_name + " " + call_type);
	LLAvatarIconCtrl* icon = getChild<LLAvatarIconCtrl>("avatar_icon");
	icon->setValue(caller_id);

	childSetAction("Accept", onAccept, this);
	childSetAction("Reject", onReject, this);
	childSetAction("Start IM", onStartIM, this);
	childSetFocus("Accept");

	return TRUE;
}

//static
void LLIncomingCallDialog::onAccept(void* user_data)
{
	LLIncomingCallDialog* self = (LLIncomingCallDialog*)user_data;
	self->processCallResponse(0);
	self->closeFloater();
}

//static
void LLIncomingCallDialog::onReject(void* user_data)
{
	LLIncomingCallDialog* self = (LLIncomingCallDialog*)user_data;
	self->processCallResponse(1);
	self->closeFloater();
}

//static
void LLIncomingCallDialog::onStartIM(void* user_data)
{
	LLIncomingCallDialog* self = (LLIncomingCallDialog*)user_data;
	self->processCallResponse(2);
	self->closeFloater();
}

void LLIncomingCallDialog::processCallResponse(S32 response)
{
	LLUUID session_id = mPayload["session_id"].asUUID();
	EInstantMessage type = (EInstantMessage)mPayload["type"].asInteger();
	LLIMMgr::EInvitationType inv_type = (LLIMMgr::EInvitationType)mPayload["inv_type"].asInteger();
	bool voice = true;
	switch(response)
	{
	case 2: // start IM: just don't start the voice chat
	{
		voice = false;
		/* FALLTHROUGH */
	}
	case 0: // accept
	{
		if (type == IM_SESSION_P2P_INVITE)
		{
			// create a normal IM session
			session_id = gIMMgr->addP2PSession(
				mPayload["session_name"].asString(),
				mPayload["caller_id"].asUUID(),
				mPayload["session_handle"].asString());

			if (voice)
			{
				LLFloaterIMPanel* im_floater =
					gIMMgr->findFloaterBySession(
						session_id);

				if (im_floater)
				{
					im_floater->requestAutoConnect();
					LLFloaterIMPanel::onClickStartCall(im_floater);		
				}
			}

			// always open IM window when connecting to voice
			LLFloaterReg::showInstance("communicate", session_id);

			gIMMgr->clearPendingAgentListUpdates(session_id);
			gIMMgr->clearPendingInvitation(session_id);
		}
		else
		{
			gIMMgr->addSession(
				mPayload["session_name"].asString(),
				type,
				session_id);

			std::string url = gAgent.getRegion()->getCapability(
				"ChatSessionRequest");

			if (voice)
			{
				LLSD data;
				data["method"] = "accept invitation";
				data["session-id"] = session_id;
				LLHTTPClient::post(
					url,
					data,
					new LLViewerChatterBoxInvitationAcceptResponder(
						session_id,
						inv_type));
			}
		}
		if (voice)
		{
			break;
		}
	}
	case 1: // decline
	{
		if (type == IM_SESSION_P2P_INVITE)
		{
			if(gVoiceClient)
			{
				std::string s = mPayload["session_handle"].asString();
				gVoiceClient->declineInvite(s);
			}
		}
		else
		{
			std::string url = gAgent.getRegion()->getCapability(
				"ChatSessionRequest");

			LLSD data;
			data["method"] = "decline invitation";
			data["session-id"] = session_id;
			LLHTTPClient::post(
				url,
				data,
				NULL);
		}
	}

	gIMMgr->clearPendingAgentListUpdates(session_id);
	gIMMgr->clearPendingInvitation(session_id);
	}
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class LLIMViewFriendObserver
//
// Bridge to suport knowing when the inventory has changed.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class LLIMViewFriendObserver : public LLFriendObserver
{
public:
	LLIMViewFriendObserver(LLIMMgr* tv) : mTV(tv) {}
	virtual ~LLIMViewFriendObserver() {}
	virtual void changed(U32 mask)
	{
		if(mask & (LLFriendObserver::ADD | LLFriendObserver::REMOVE | LLFriendObserver::ONLINE))
		{
			mTV->refresh();
		}
	}
protected:
	LLIMMgr* mTV;
};


bool inviteUserResponse(const LLSD& notification, const LLSD& response)
{
	const LLSD& payload = notification["payload"];
	LLUUID session_id = payload["session_id"].asUUID();
	EInstantMessage type = (EInstantMessage)payload["type"].asInteger();
	LLIMMgr::EInvitationType inv_type = (LLIMMgr::EInvitationType)payload["inv_type"].asInteger();
	S32 option = LLNotification::getSelectedOption(notification, response);
	switch(option) 
	{
	case 0: // accept
		{
			if (type == IM_SESSION_P2P_INVITE)
			{
				// create a normal IM session
				session_id = gIMMgr->addP2PSession(
					payload["session_name"].asString(),
					payload["caller_id"].asUUID(),
					payload["session_handle"].asString(),
					payload["session_uri"].asString());

				LLFloaterIMPanel* im_floater =
					gIMMgr->findFloaterBySession(
						session_id);
				if (im_floater)
				{
					im_floater->requestAutoConnect();
					LLFloaterIMPanel::onClickStartCall(im_floater);
					// always open IM window when connecting to voice
					LLFloaterReg::showInstance("communicate", session_id, TRUE);
				}

				gIMMgr->clearPendingAgentListUpdates(session_id);
				gIMMgr->clearPendingInvitation(session_id);
			}
			else
			{
				gIMMgr->addSession(
					payload["session_name"].asString(),
					type,
					session_id);

				std::string url = gAgent.getRegion()->getCapability(
					"ChatSessionRequest");

				LLSD data;
				data["method"] = "accept invitation";
				data["session-id"] = session_id;
				LLHTTPClient::post(
					url,
					data,
					new LLViewerChatterBoxInvitationAcceptResponder(
						session_id,
						inv_type));
			}
		}
		break;
	case 2: // mute (also implies ignore, so this falls through to the "ignore" case below)
	{
		// mute the sender of this invite
		if (!LLMuteList::getInstance()->isMuted(payload["caller_id"].asUUID()))
		{
			LLMute mute(payload["caller_id"].asUUID(), payload["caller_name"].asString(), LLMute::AGENT);
			LLMuteList::getInstance()->add(mute);
		}
	}
	/* FALLTHROUGH */
	
	case 1: // decline
	{
		if (type == IM_SESSION_P2P_INVITE)
		{
			if(gVoiceClient)
			{
				std::string s = payload["session_handle"].asString();
				gVoiceClient->declineInvite(s);
			}
		}
		else
		{
			std::string url = gAgent.getRegion()->getCapability(
				"ChatSessionRequest");

			LLSD data;
			data["method"] = "decline invitation";
			data["session-id"] = session_id;
			LLHTTPClient::post(
				url,
				data,
				NULL);				
		}
	}

	gIMMgr->clearPendingAgentListUpdates(session_id);
	gIMMgr->clearPendingInvitation(session_id);
	break;
	}
	
	return false;
}

//
// Member Functions
//

LLIMMgr::LLIMMgr() :
	mFriendObserver(NULL),
	mIMReceived(FALSE)
{
	static bool registered_dialog = false;
	if (!registered_dialog)
	{
		LLFloaterReg::add("incoming_call", "floater_incoming_call.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<LLIncomingCallDialog>);
		registered_dialog = true;
	}
		
	mFriendObserver = new LLIMViewFriendObserver(this);
	LLAvatarTracker::instance().addObserver(mFriendObserver);

	mPendingInvitations = LLSD::emptyMap();
	mPendingAgentListUpdates = LLSD::emptyMap();
}

LLIMMgr::~LLIMMgr()
{
	LLAvatarTracker::instance().removeObserver(mFriendObserver);
	delete mFriendObserver;
	// Children all cleaned up by default view destructor.
}

// Add a message to a session. 
void LLIMMgr::addMessage(
	const LLUUID& session_id,
	const LLUUID& target_id,
	const std::string& from,
	const std::string& msg,
	const std::string& session_name,
	EInstantMessage dialog,
	U32 parent_estate_id,
	const LLUUID& region_id,
	const LLVector3& position,
	bool link_name) // If this is true, then we insert the name and link it to a profile
{
	LLUUID other_participant_id = target_id;

	// don't process muted IMs
	if (LLMuteList::getInstance()->isMuted(
			other_participant_id,
			LLMute::flagTextChat) && !LLMuteList::getInstance()->isLinden(from))
	{
		return;
	}

	//not sure why...but if it is from ourselves we set the target_id
	//to be NULL
	if( other_participant_id == gAgent.getID() )
	{
		other_participant_id = LLUUID::null;
	}

	LLFloaterIMPanel* floater;
	LLUUID new_session_id = session_id;
	if (new_session_id.isNull())
	{
		//no session ID...compute new one
		new_session_id = computeSessionID(dialog, other_participant_id);
	}

	if (!LLIMModel::getInstance()->findIMSession(new_session_id))
	{
		LLIMModel::instance().newSession(session_id, session_name, dialog, other_participant_id);
	}

	floater = findFloaterBySession(new_session_id);
	if (!floater)
	{
		floater = findFloaterBySession(other_participant_id);
		if (floater)
		{
			llinfos << "found the IM session " << session_id 
				<< " by participant " << other_participant_id << llendl;
		}
	}

	// create IM window as necessary
	if(!floater)
	{
		std::string name = from;
		if(!session_name.empty() && session_name.size()>1)
		{
			name = session_name;
		}

		
		floater = createFloater(
			new_session_id,
			other_participant_id,
			name,
			dialog,
			FALSE);

		// When we get a new IM, and if you are a god, display a bit
		// of information about the source. This is to help liaisons
		// when answering questions.
		if(gAgent.isGodlike())
		{
			// *TODO:translate (low priority, god ability)
			std::ostringstream bonus_info;
			bonus_info << LLTrans::getString("***")+ " "+ LLTrans::getString("IMParentEstate") + ":" + " "
				<< parent_estate_id
			<< ((parent_estate_id == 1) ? "," + LLTrans::getString("IMMainland") : "")
				<< ((parent_estate_id == 5) ? "," + LLTrans::getString ("IMTeen") : "");

			// once we have web-services (or something) which returns
			// information about a region id, we can print this out
			// and even have it link to map-teleport or something.
			//<< "*** region_id: " << region_id << std::endl
			//<< "*** position: " << position << std::endl;

			floater->addHistoryLine(bonus_info.str(), LLUIColorTable::instance().getColor("SystemChatColor"));
			LLIMModel::instance().addMessage(new_session_id, from, other_participant_id, bonus_info.str());
		}

		make_ui_sound("UISndNewIncomingIMSession");
	}

	// now add message to floater
	bool is_from_system = target_id.isNull() || (from == SYSTEM_FROM);
	const LLColor4& color = ( is_from_system ? 
							  LLUIColorTable::instance().getColor("SystemChatColor") : 
							  LLUIColorTable::instance().getColor("IMChatColor"));
	if ( !link_name )
	{
		floater->addHistoryLine(msg,color); // No name to prepend, so just add the message normally
	}
	else
	{
		floater->addHistoryLine(msg, color, true, other_participant_id, from); // Insert linked name to front of message

		//*TODO consider moving that speaker management stuff into model (IB)
		LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(new_session_id);
		if (speaker_mgr)
		{
			speaker_mgr->speakerChatted(gAgentID);
			speaker_mgr->setSpeakerTyping(gAgentID, FALSE);
		}
	}

	LLIMModel::instance().addMessage(new_session_id, from, other_participant_id, msg);

	if( !LLFloaterReg::instanceVisible("communicate") && !floater->getVisible())
	{
		LLFloaterChatterBox* chat_floater = LLFloaterChatterBox::getInstance();
		
		//if the IM window is not open and the floater is not visible (i.e. not torn off)
		LLFloater* previouslyActiveFloater = chat_floater->getActiveFloater();

		// select the newly added floater (or the floater with the new line added to it).
		// it should be there.
		chat_floater->selectFloater(floater);

		//there was a previously unseen IM, make that old tab flashing
		//it is assumed that the most recently unseen IM tab is the one current selected/active
		if ( previouslyActiveFloater && getIMReceived() )
		{
			chat_floater->setFloaterFlashing(previouslyActiveFloater, TRUE);
		}

		//notify of a new IM
		notifyNewIM();
	}
}

void LLIMMgr::addSystemMessage(const LLUUID& session_id, const std::string& message_name, const LLSD& args)
{
	LLUIString message;
	
	// null session id means near me (chat history)
	if (session_id.isNull())
	{
		message = LLTrans::getString(message_name);
		message.setArgs(args);

		LLChat chat(message);
		chat.mSourceType = CHAT_SOURCE_SYSTEM;
		LLFloaterChat::addChatHistory(chat);
	}
	else // going to IM session
	{
		LLFloaterIMPanel* floaterp = findFloaterBySession(session_id);
		if (floaterp)
		{
			message = floaterp->getString(message_name);
			message.setArgs(args);

			gIMMgr->addMessage(session_id, LLUUID::null, SYSTEM_FROM, message.getString());
		}
	}
}

void LLIMMgr::notifyNewIM()
{
	if(!LLFloaterReg::instanceVisible("communicate"))
	{
		mIMReceived = TRUE;
	}
}

S32 LLIMMgr::getNumberOfUnreadIM()
{
	std::map<LLUUID, LLIMModel::LLIMSession*>::iterator it;
	
	S32 num = 0;
	for(it = LLIMModel::sSessionsMap.begin(); it != LLIMModel::sSessionsMap.end(); ++it)
	{
		if((*it).first != mBeingRemovedSessionID)
		{
			num += (*it).second->mNumUnread;
		}
	}

	return num;
}

void LLIMMgr::clearNewIMNotification()
{
	mIMReceived = FALSE;
}

BOOL LLIMMgr::getIMReceived() const
{
	return mIMReceived;
}

// This method returns TRUE if the local viewer has a session
// currently open keyed to the uuid. 
BOOL LLIMMgr::isIMSessionOpen(const LLUUID& uuid)
{
	LLFloaterIMPanel* floater = findFloaterBySession(uuid);
	if(floater) return TRUE;
	return FALSE;
}

LLUUID LLIMMgr::addP2PSession(const std::string& name,
							const LLUUID& other_participant_id,
							const std::string& voice_session_handle,
							const std::string& caller_uri)
{
	LLUUID session_id = addSession(name, IM_NOTHING_SPECIAL, other_participant_id);

	LLVoiceChannelP2P* voice_channel = dynamic_cast<LLVoiceChannelP2P*>(LLIMModel::getInstance()->getSpeakerManager(session_id));
	if (voice_channel)
	{
		voice_channel->setSessionHandle(voice_session_handle, caller_uri);
	}

	return session_id;
}

// This adds a session to the talk view. The name is the local name of
// the session, dialog specifies the type of session. If the session
// exists, it is brought forward.  Specifying id = NULL results in an
// im session to everyone. Returns the uuid of the session.
LLUUID LLIMMgr::addSession(
	const std::string& name,
	EInstantMessage dialog,
	const LLUUID& other_participant_id)
{
	LLDynamicArray<LLUUID> ids;
	ids.put(other_participant_id);
	return addSession(name, dialog, other_participant_id, ids);
}

// Adds a session using the given session_id.  If the session already exists 
// the dialog type is assumed correct. Returns the uuid of the session.
LLUUID LLIMMgr::addSession(
	const std::string& name,
	EInstantMessage dialog,
	const LLUUID& other_participant_id,
	const LLDynamicArray<LLUUID>& ids)
{
	if (0 == ids.getLength())
	{
		return LLUUID::null;
	}

	LLUUID session_id = computeSessionID(dialog,other_participant_id);

	if (!LLIMModel::getInstance()->findIMSession(session_id))
	{
		LLIMModel::instance().newSession(session_id, name, dialog, other_participant_id);
	}

	LLFloaterIMPanel* floater = findFloaterBySession(session_id);
	if(!floater)
	{
		// On creation, use the first element of ids as the
		// "other_participant_id"
		floater = createFloater(
			session_id,
			other_participant_id,
			name,
			dialog,
			TRUE,
			ids);

		if ( !floater ) return LLUUID::null;

		noteOfflineUsers(floater, ids);

		// Only warn for regular IMs - not group IMs
		if( dialog == IM_NOTHING_SPECIAL )
		{
			noteMutedUsers(floater, ids);
		}
	}
	floater->setInputFocus(TRUE);
	LLIMFloater::show(session_id);

	return session_id;
}

// This removes the panel referenced by the uuid, and then restores
// internal consistency. The internal pointer is not deleted? Did you mean
// a pointer to the corresponding LLIMSession? Session data is cleared now.
// Put a copy of UUID to avoid problem when passed reference becames invalid
// if it has been come from the object removed in observer.
void LLIMMgr::removeSession(LLUUID session_id)
{
	if (mBeingRemovedSessionID == session_id)
	{
		return;
	}
	
	LLFloaterIMPanel* floater = findFloaterBySession(session_id);
	if(floater)
	{
		mFloaters.erase(floater->getHandle());
		LLFloaterChatterBox::getInstance()->removeFloater(floater);
		//mTabContainer->removeTabPanel(floater);

		clearPendingInvitation(session_id);
		clearPendingAgentListUpdates(session_id);
	}

	// for some purposes storing ID of a sessios that is being removed
	mBeingRemovedSessionID = session_id;
	notifyObserverSessionRemoved(session_id);

	//if we don't clear session data on removing the session
	//we can't use LLBottomTray as observer of session creation/delettion and 
	//creating chiclets only on session created even, we need to handle chiclets creation
	//the same way as LLFloaterIMPanels were managed.
	LLIMModel::getInstance()->clearSession(session_id);

	// now this session is completely removed
	mBeingRemovedSessionID.setNull();
}

void LLIMMgr::inviteToSession(
	const LLUUID& session_id, 
	const std::string& session_name, 
	const LLUUID& caller_id, 
	const std::string& caller_name,
	EInstantMessage type,
	EInvitationType inv_type,
	const std::string& session_handle,
	const std::string& session_uri)
{
	//ignore invites from muted residents
	if (LLMuteList::getInstance()->isMuted(caller_id))
	{
		return;
	}

	std::string notify_box_type;

	BOOL ad_hoc_invite = FALSE;
	if(type == IM_SESSION_P2P_INVITE)
	{
		//P2P is different...they only have voice invitations
		notify_box_type = "VoiceInviteP2P";
	}
	else if ( gAgent.isInGroup(session_id) )
	{
		//only really old school groups have voice invitations
		notify_box_type = "VoiceInviteGroup";
	}
	else if ( inv_type == INVITATION_TYPE_VOICE )
	{
		//else it's an ad-hoc
		//and a voice ad-hoc
		notify_box_type = "VoiceInviteAdHoc";
		ad_hoc_invite = TRUE;
	}
	else if ( inv_type == INVITATION_TYPE_IMMEDIATE )
	{
		notify_box_type = "InviteAdHoc";
		ad_hoc_invite = TRUE;
	}

	LLSD payload;
	payload["session_id"] = session_id;
	payload["session_name"] = session_name;
	payload["caller_id"] = caller_id;
	payload["caller_name"] = caller_name;
	payload["type"] = type;
	payload["inv_type"] = inv_type;
	payload["session_handle"] = session_handle;
	payload["session_uri"] = session_uri;
	payload["notify_box_type"] = notify_box_type;
	
	LLVoiceChannel* channelp = LLVoiceChannel::getChannelByID(session_id);
	if (channelp && channelp->callStarted())
	{
		// you have already started a call to the other user, so just accept the invite
		LLNotifications::instance().forceResponse(LLNotification::Params("VoiceInviteP2P").payload(payload), 0);
		return;
	}

	if (type == IM_SESSION_P2P_INVITE || ad_hoc_invite)
	{
		// is the inviter a friend?
		if (LLAvatarTracker::instance().getBuddyInfo(caller_id) == NULL)
		{
			// if not, and we are ignoring voice invites from non-friends
			// then silently decline
			if (gSavedSettings.getBOOL("VoiceCallsFriendsOnly"))
			{
				// invite not from a friend, so decline
				LLNotifications::instance().forceResponse(LLNotification::Params("VoiceInviteP2P").payload(payload), 1);
				return;
			}
		}
	}

	if ( !mPendingInvitations.has(session_id.asString()) )
	{
		if (caller_name.empty())
		{
			gCacheName->get(caller_id, FALSE, boost::bind(&LLIMMgr::onInviteNameLookup, payload, _1, _2, _3, _4));
		}
		else
		{
			if (notify_box_type == "VoiceInviteP2P" || notify_box_type == "VoiceInviteAdHoc")
			{
				LLFloaterReg::showInstance("incoming_call", payload, TRUE);
			}
			else
			{
				LLSD args;
				args["NAME"] = caller_name;
				args["GROUP"] = session_name;

				LLNotifications::instance().add(notify_box_type, args, payload, &inviteUserResponse);
			}
		}
		mPendingInvitations[session_id.asString()] = LLSD();
	}
}

void LLIMMgr::onInviteNameLookup(LLSD payload, const LLUUID& id, const std::string& first, const std::string& last, BOOL is_group)
{
	payload["caller_name"] = first + " " + last;
	payload["session_name"] = payload["caller_name"].asString();

	std::string notify_box_type = payload["notify_box_type"].asString();

	if (notify_box_type == "VoiceInviteP2P" || notify_box_type == "VoiceInviteAdHoc")
	{
		LLFloaterReg::showInstance("incoming_call", payload, TRUE);
	}
	else
	{
		LLSD args;
		args["NAME"] = payload["caller_name"].asString();
	
		LLNotifications::instance().add(
			payload["notify_box_type"].asString(),
			args, 
			payload,
			&inviteUserResponse);
	}
}

void LLIMMgr::refresh()
{
}

void LLIMMgr::disconnectAllSessions()
{
	LLFloaterIMPanel* floater = NULL;
	std::set<LLHandle<LLFloater> >::iterator handle_it;
	for(handle_it = mFloaters.begin();
		handle_it != mFloaters.end();
		)
	{
		floater = (LLFloaterIMPanel*)handle_it->get();

		// MUST do this BEFORE calling floater->onClose() because that may remove the item from the set, causing the subsequent increment to crash.
		++handle_it;

		if (floater)
		{
			floater->setEnabled(FALSE);
			floater->closeFloater(TRUE);
		}
	}
}


// This method returns the im panel corresponding to the uuid
// provided. The uuid can either be a session id or an agent
// id. Returns NULL if there is no matching panel.
LLFloaterIMPanel* LLIMMgr::findFloaterBySession(const LLUUID& session_id)
{
	LLFloaterIMPanel* rv = NULL;
	std::set<LLHandle<LLFloater> >::iterator handle_it;
	for(handle_it = mFloaters.begin();
		handle_it != mFloaters.end();
		++handle_it)
	{
		rv = (LLFloaterIMPanel*)handle_it->get();
		if(rv && session_id == rv->getSessionID())
		{
			break;
		}
		rv = NULL;
	}
	return rv;
}


BOOL LLIMMgr::hasSession(const LLUUID& session_id)
{
	return (findFloaterBySession(session_id) != NULL);
}

void LLIMMgr::clearPendingInvitation(const LLUUID& session_id)
{
	if ( mPendingInvitations.has(session_id.asString()) )
	{
		mPendingInvitations.erase(session_id.asString());
	}
}

LLSD LLIMMgr::getPendingAgentListUpdates(const LLUUID& session_id)
{
	if ( mPendingAgentListUpdates.has(session_id.asString()) )
	{
		return mPendingAgentListUpdates[session_id.asString()];
	}
	else
	{
		return LLSD();
	}
}

void LLIMMgr::addPendingAgentListUpdates(
	const LLUUID& session_id,
	const LLSD& updates)
{
	LLSD::map_const_iterator iter;

	if ( !mPendingAgentListUpdates.has(session_id.asString()) )
	{
		//this is a new agent list update for this session
		mPendingAgentListUpdates[session_id.asString()] = LLSD::emptyMap();
	}

	if (
		updates.has("agent_updates") &&
		updates["agent_updates"].isMap() &&
		updates.has("updates") &&
		updates["updates"].isMap() )
	{
		//new school update
		LLSD update_types = LLSD::emptyArray();
		LLSD::array_iterator array_iter;

		update_types.append("agent_updates");
		update_types.append("updates");

		for (
			array_iter = update_types.beginArray();
			array_iter != update_types.endArray();
			++array_iter)
		{
			//we only want to include the last update for a given agent
			for (
				iter = updates[array_iter->asString()].beginMap();
				iter != updates[array_iter->asString()].endMap();
				++iter)
			{
				mPendingAgentListUpdates[session_id.asString()][array_iter->asString()][iter->first] =
					iter->second;
			}
		}
	}
	else if (
		updates.has("updates") &&
		updates["updates"].isMap() )
	{
		//old school update where the SD contained just mappings
		//of agent_id -> "LEAVE"/"ENTER"

		//only want to keep last update for each agent
		for (
			iter = updates["updates"].beginMap();
			iter != updates["updates"].endMap();
			++iter)
		{
			mPendingAgentListUpdates[session_id.asString()]["updates"][iter->first] =
				iter->second;
		}
	}
}

void LLIMMgr::clearPendingAgentListUpdates(const LLUUID& session_id)
{
	if ( mPendingAgentListUpdates.has(session_id.asString()) )
	{
		mPendingAgentListUpdates.erase(session_id.asString());
	}
}

void LLIMMgr::notifyObserverSessionAdded(const LLUUID& session_id, const std::string& name, const LLUUID& other_participant_id)
{
	for (session_observers_list_t::iterator it = mSessionObservers.begin(); it != mSessionObservers.end(); it++)
	{
		(*it)->sessionAdded(session_id, name, other_participant_id);
	}
}

void LLIMMgr::notifyObserverSessionRemoved(const LLUUID& session_id)
{
	for (session_observers_list_t::iterator it = mSessionObservers.begin(); it != mSessionObservers.end(); it++)
	{
		(*it)->sessionRemoved(session_id);
	}
}

void LLIMMgr::addSessionObserver(LLIMSessionObserver *observer)
{
	mSessionObservers.push_back(observer);
}

void LLIMMgr::removeSessionObserver(LLIMSessionObserver *observer)
{
	mSessionObservers.remove(observer);
}

// create a floater and update internal representation for
// consistency. Returns the pointer, caller (the class instance since
// it is a private method) is not responsible for deleting the
// pointer.  Add the floater to this but do not select it.
LLFloaterIMPanel* LLIMMgr::createFloater(
	const LLUUID& session_id,
	const LLUUID& other_participant_id,
	const std::string& session_label,
	EInstantMessage dialog,
	BOOL user_initiated,
	const LLDynamicArray<LLUUID>& ids)
{
	if (session_id.isNull())
	{
		llwarns << "Creating LLFloaterIMPanel with null session ID" << llendl;
	}

	llinfos << "LLIMMgr::createFloater: from " << other_participant_id 
			<< " in session " << session_id << llendl;
	LLFloaterIMPanel* floater = new LLFloaterIMPanel(session_label,
													 session_id,
													 other_participant_id,
													 ids,
													 dialog);
	LLTabContainer::eInsertionPoint i_pt = user_initiated ? LLTabContainer::RIGHT_OF_CURRENT : LLTabContainer::END;
	LLFloaterChatterBox::getInstance()->addFloater(floater, FALSE, i_pt);
	mFloaters.insert(floater->getHandle());
	return floater;
}

void LLIMMgr::noteOfflineUsers(
	LLFloaterIMPanel* floater,
	const LLDynamicArray<LLUUID>& ids)
{
	S32 count = ids.count();
	if(count == 0)
	{
		floater->addHistoryLine(LLTrans::getString("only_user_message"), LLUIColorTable::instance().getColor("SystemChatColor"));
	}
	else
	{
		const LLRelationship* info = NULL;
		LLAvatarTracker& at = LLAvatarTracker::instance();
		for(S32 i = 0; i < count; ++i)
		{
			info = at.getBuddyInfo(ids.get(i));
			std::string first, last;
			if(info && !info->isOnline()
			   && gCacheName->getName(ids.get(i), first, last))
			{
				LLUIString offline = LLTrans::getString("offline_message");
				offline.setArg("[FIRST]", first);
				offline.setArg("[LAST]", last);
				floater->addHistoryLine(offline, LLUIColorTable::instance().getColor("SystemChatColor"));
			}
		}
	}
}

void LLIMMgr::noteMutedUsers(LLFloaterIMPanel* floater,
								  const LLDynamicArray<LLUUID>& ids)
{
	// Don't do this if we don't have a mute list.
	LLMuteList *ml = LLMuteList::getInstance();
	if( !ml )
	{
		return;
	}

	S32 count = ids.count();
	if(count > 0)
	{
		for(S32 i = 0; i < count; ++i)
		{
			if( ml->isMuted(ids.get(i)) )
			{
				LLUIString muted = LLTrans::getString("muted_message");
				floater->addHistoryLine(muted);
				break;
			}
		}
	}
}

void LLIMMgr::processIMTypingStart(const LLIMInfo* im_info)
{
	processIMTypingCore(im_info, TRUE);
}

void LLIMMgr::processIMTypingStop(const LLIMInfo* im_info)
{
	processIMTypingCore(im_info, FALSE);
}

void LLIMMgr::processIMTypingCore(const LLIMInfo* im_info, BOOL typing)
{
	LLUUID session_id = computeSessionID(im_info->mIMType, im_info->mFromID);
	LLFloaterIMPanel* floater = findFloaterBySession(session_id);
	if (floater)
	{
		floater->processIMTyping(im_info, typing);
	}
}

void LLIMMgr::updateFloaterSessionID(
	const LLUUID& old_session_id,
	const LLUUID& new_session_id)
{
	LLFloaterIMPanel* floater = findFloaterBySession(old_session_id);
	if (floater)
	{
		floater->sessionInitReplyReceived(new_session_id);
	}
}

class LLViewerChatterBoxSessionStartReply : public LLHTTPNode
{
public:
	virtual void describe(Description& desc) const
	{
		desc.shortInfo("Used for receiving a reply to a request to initialize an ChatterBox session");
		desc.postAPI();
		desc.input(
			"{\"client_session_id\": UUID, \"session_id\": UUID, \"success\" boolean, \"reason\": string");
		desc.source(__FILE__, __LINE__);
	}

	virtual void post(ResponsePtr response,
					  const LLSD& context,
					  const LLSD& input) const
	{
		LLSD body;
		LLUUID temp_session_id;
		LLUUID session_id;
		bool success;

		body = input["body"];
		success = body["success"].asBoolean();
		temp_session_id = body["temp_session_id"].asUUID();

		if ( success )
		{
			session_id = body["session_id"].asUUID();
			gIMMgr->updateFloaterSessionID(
				temp_session_id,
				session_id);

			LLIMModel::getInstance()->updateSessionID(temp_session_id, session_id);

			LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(session_id);
			if (speaker_mgr)
			{
				speaker_mgr->setSpeakers(body);
				speaker_mgr->updateSpeakers(gIMMgr->getPendingAgentListUpdates(session_id));
			}

			LLFloaterIMPanel* floaterp = gIMMgr->findFloaterBySession(session_id);
			if (floaterp)
			{
				if ( body.has("session_info") )
				{
					floaterp->processSessionUpdate(body["session_info"]);
				}
			}

			gIMMgr->clearPendingAgentListUpdates(session_id);
		}
		else
		{
			//throw an error dialog and close the temp session's
			//floater
			LLFloaterIMPanel* floater = 
				gIMMgr->findFloaterBySession(temp_session_id);

			if ( floater )
			{
				floater->showSessionStartError(body["error"].asString());
			}
		}

		gIMMgr->clearPendingAgentListUpdates(session_id);
	}
};

class LLViewerChatterBoxSessionEventReply : public LLHTTPNode
{
public:
	virtual void describe(Description& desc) const
	{
		desc.shortInfo("Used for receiving a reply to a ChatterBox session event");
		desc.postAPI();
		desc.input(
			"{\"event\": string, \"reason\": string, \"success\": boolean, \"session_id\": UUID");
		desc.source(__FILE__, __LINE__);
	}

	virtual void post(ResponsePtr response,
					  const LLSD& context,
					  const LLSD& input) const
	{
		LLUUID session_id;
		bool success;

		LLSD body = input["body"];
		success = body["success"].asBoolean();
		session_id = body["session_id"].asUUID();

		if ( !success )
		{
			//throw an error dialog
			LLFloaterIMPanel* floater = 
				gIMMgr->findFloaterBySession(session_id);

			if (floater)
			{
				floater->showSessionEventError(
					body["event"].asString(),
					body["error"].asString());
			}
		}
	}
};

class LLViewerForceCloseChatterBoxSession: public LLHTTPNode
{
public:
	virtual void post(ResponsePtr response,
					  const LLSD& context,
					  const LLSD& input) const
	{
		LLUUID session_id;
		std::string reason;

		session_id = input["body"]["session_id"].asUUID();
		reason = input["body"]["reason"].asString();

		LLFloaterIMPanel* floater =
			gIMMgr ->findFloaterBySession(session_id);

		if ( floater )
		{
			floater->showSessionForceClose(reason);
		}
	}
};

class LLViewerChatterBoxSessionAgentListUpdates : public LLHTTPNode
{
public:
	virtual void post(
		ResponsePtr responder,
		const LLSD& context,
		const LLSD& input) const
	{
		const LLUUID& session_id = input["body"]["session_id"].asUUID();
		LLIMSpeakerMgr* speaker_mgr = LLIMModel::getInstance()->getSpeakerManager(session_id);
		if (speaker_mgr)
		{
			speaker_mgr->updateSpeakers(input["body"]);
		}
		else
		{
			//we don't have a speaker manager yet..something went wrong
			//we are probably receiving an update here before
			//a start or an acceptance of an invitation.  Race condition.
			gIMMgr->addPendingAgentListUpdates(
				input["body"]["session_id"].asUUID(),
				input["body"]);
		}
	}
};

class LLViewerChatterBoxSessionUpdate : public LLHTTPNode
{
public:
	virtual void post(
		ResponsePtr responder,
		const LLSD& context,
		const LLSD& input) const
	{
		LLFloaterIMPanel* floaterp = gIMMgr->findFloaterBySession(input["body"]["session_id"].asUUID());
		if (floaterp)
		{
			floaterp->processSessionUpdate(input["body"]["info"]);
		}
	}
};


class LLViewerChatterBoxInvitation : public LLHTTPNode
{
public:

	virtual void post(
		ResponsePtr response,
		const LLSD& context,
		const LLSD& input) const
	{
		//for backwards compatiblity reasons...we need to still
		//check for 'text' or 'voice' invitations...bleh
		if ( input["body"].has("instantmessage") )
		{
			LLSD message_params =
				input["body"]["instantmessage"]["message_params"];

			//do something here to have the IM invite behave
			//just like a normal IM
			//this is just replicated code from process_improved_im
			//and should really go in it's own function -jwolk
			if (gNoRender)
			{
				return;
			}
			LLChat chat;

			std::string message = message_params["message"].asString();
			std::string name = message_params["from_name"].asString();
			LLUUID from_id = message_params["from_id"].asUUID();
			LLUUID session_id = message_params["id"].asUUID();
			std::vector<U8> bin_bucket = message_params["data"]["binary_bucket"].asBinary();
			U8 offline = (U8)message_params["offline"].asInteger();
			
			time_t timestamp =
				(time_t) message_params["timestamp"].asInteger();

			BOOL is_busy = gAgent.getBusy();
			BOOL is_muted = LLMuteList::getInstance()->isMuted(
				from_id,
				name,
				LLMute::flagTextChat);

			BOOL is_linden = LLMuteList::getInstance()->isLinden(name);
			std::string separator_string(": ");
			int message_offset=0;

			//Handle IRC styled /me messages.
			std::string prefix = message.substr(0, 4);
			if (prefix == "/me " || prefix == "/me'")
			{
				separator_string = "";
				message_offset = 3;
			}
			
			chat.mMuted = is_muted && !is_linden;
			chat.mFromID = from_id;
			chat.mFromName = name;

			if (!is_linden && (is_busy || is_muted))
			{
				return;
			}

			// standard message, not from system
			std::string saved;
			if(offline == IM_OFFLINE)
			{
				saved = llformat("(Saved %s) ", formatted_time(timestamp).c_str());
			}
			std::string buffer = saved + message.substr(message_offset);

			BOOL is_this_agent = FALSE;
			if(from_id == gAgentID)
			{
				is_this_agent = TRUE;
			}
			gIMMgr->addMessage(
				session_id,
				from_id,
				name,
				buffer,
				std::string((char*)&bin_bucket[0]),
				IM_SESSION_INVITE,
				message_params["parent_estate_id"].asInteger(),
				message_params["region_id"].asUUID(),
				ll_vector3_from_sd(message_params["position"]),
				true);

			chat.mText = std::string("IM: ") + name + separator_string + saved + message.substr(message_offset);
			LLFloaterChat::addChat(chat, TRUE, is_this_agent);

			//K now we want to accept the invitation
			std::string url = gAgent.getRegion()->getCapability(
				"ChatSessionRequest");

			if ( url != "" )
			{
				LLSD data;
				data["method"] = "accept invitation";
				data["session-id"] = session_id;
				LLHTTPClient::post(
					url,
					data,
					new LLViewerChatterBoxInvitationAcceptResponder(
						session_id,
						LLIMMgr::INVITATION_TYPE_INSTANT_MESSAGE));
			}
		} //end if invitation has instant message
		else if ( input["body"].has("voice") )
		{
			if (gNoRender)
			{
				return;
			}
			
			if(!LLVoiceClient::voiceEnabled())
			{
				// Don't display voice invites unless the user has voice enabled.
				return;
			}

			gIMMgr->inviteToSession(
				input["body"]["session_id"].asUUID(), 
				input["body"]["session_name"].asString(), 
				input["body"]["from_id"].asUUID(),
				input["body"]["from_name"].asString(),
				IM_SESSION_INVITE,
				LLIMMgr::INVITATION_TYPE_VOICE);
		}
		else if ( input["body"].has("immediate") )
		{
			gIMMgr->inviteToSession(
				input["body"]["session_id"].asUUID(), 
				input["body"]["session_name"].asString(), 
				input["body"]["from_id"].asUUID(),
				input["body"]["from_name"].asString(),
				IM_SESSION_INVITE,
				LLIMMgr::INVITATION_TYPE_IMMEDIATE);
		}
	}
};

LLHTTPRegistration<LLViewerChatterBoxSessionStartReply>
   gHTTPRegistrationMessageChatterboxsessionstartreply(
	   "/message/ChatterBoxSessionStartReply");

LLHTTPRegistration<LLViewerChatterBoxSessionEventReply>
   gHTTPRegistrationMessageChatterboxsessioneventreply(
	   "/message/ChatterBoxSessionEventReply");

LLHTTPRegistration<LLViewerForceCloseChatterBoxSession>
    gHTTPRegistrationMessageForceclosechatterboxsession(
		"/message/ForceCloseChatterBoxSession");

LLHTTPRegistration<LLViewerChatterBoxSessionAgentListUpdates>
    gHTTPRegistrationMessageChatterboxsessionagentlistupdates(
	    "/message/ChatterBoxSessionAgentListUpdates");

LLHTTPRegistration<LLViewerChatterBoxSessionUpdate>
    gHTTPRegistrationMessageChatterBoxSessionUpdate(
	    "/message/ChatterBoxSessionUpdate");

LLHTTPRegistration<LLViewerChatterBoxInvitation>
    gHTTPRegistrationMessageChatterBoxInvitation(
		"/message/ChatterBoxInvitation");

