/** 
 * @file llcallfloater.cpp
 * @author Mike Antipov
 * @brief Voice Control Panel in a Voice Chats (P2P, Group, Nearby...).
 *
 * $LicenseInfo:firstyear=2009&license=viewerlgpl$
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

#include "llcallfloater.h"

#include "llnotificationsutil.h"
#include "lltrans.h"

#include "llagent.h"
#include "llagentdata.h" // for gAgentID
#include "llavatarnamecache.h"
#include "llavatariconctrl.h"
#include "llavatarlist.h"
#include "lldraghandle.h"
#include "llimfloater.h"
#include "llimview.h"
#include "llfloaterreg.h"
#include "llparticipantlist.h"
#include "llspeakers.h"
#include "lltextutil.h"
#include "lltransientfloatermgr.h"
#include "llviewercontrol.h"
#include "llviewerdisplayname.h"
#include "llviewerwindow.h"
#include "llvoicechannel.h"
#include "llviewerparcelmgr.h"
#include "llfirstuse.h"

static void get_voice_participants_uuids(uuid_vec_t& speakers_uuids);
void reshape_floater(LLCallFloater* floater, S32 delta_height);

class LLNonAvatarCaller : public LLAvatarListItem
{
public:
	LLNonAvatarCaller() : LLAvatarListItem(false)
	{

	}
	BOOL postBuild()
	{
		BOOL rv = LLAvatarListItem::postBuild();

		if (rv)
		{
			setOnline(true);
			showLastInteractionTime(false);
			setShowProfileBtn(false);
			setShowInfoBtn(false);
			mAvatarIcon->setValue("Avaline_Icon");
			mAvatarIcon->setToolTip(std::string(""));
		}
		return rv;
	}

	void setName(const std::string& name)
	{
		const std::string& formatted_phone = LLTextUtil::formatPhoneNumber(name);
		LLAvatarListItem::setAvatarName(formatted_phone);
		LLAvatarListItem::setAvatarToolTip(formatted_phone);
	}

	void setSpeakerId(const LLUUID& id) { mSpeakingIndicator->setSpeakerId(id); }
};


static void* create_non_avatar_caller(void*)
{
	return new LLNonAvatarCaller;
}

LLVoiceChannel* LLCallFloater::sCurrentVoiceChannel = NULL;

LLCallFloater::LLCallFloater(const LLSD& key)
: LLTransientDockableFloater(NULL, false, key)
, mSpeakerManager(NULL)
, mParticipants(NULL)
, mAvatarList(NULL)
, mNonAvatarCaller(NULL)
, mVoiceType(VC_LOCAL_CHAT)
, mAgentPanel(NULL)
, mSpeakingIndicator(NULL)
, mIsModeratorMutedVoice(false)
, mInitParticipantsVoiceState(false)
{
	static LLUICachedControl<S32> voice_left_remove_delay ("VoiceParticipantLeftRemoveDelay", 10);
	mSpeakerDelayRemover = new LLSpeakersDelayActionsStorage(boost::bind(&LLCallFloater::removeVoiceLeftParticipant, this, _1), voice_left_remove_delay);

	mFactoryMap["non_avatar_caller"] = LLCallbackMap(create_non_avatar_caller, NULL);
	LLVoiceClient::instance().addObserver(this);
	LLTransientFloaterMgr::getInstance()->addControlView(this);

	// update the agent's name if display name setting change
	LLAvatarNameCache::addUseDisplayNamesCallback(boost::bind(&LLCallFloater::updateAgentModeratorState, this));
	LLViewerDisplayName::addNameChangedCallback(boost::bind(&LLCallFloater::updateAgentModeratorState, this));

}

LLCallFloater::~LLCallFloater()
{
	resetVoiceRemoveTimers();
	delete mSpeakerDelayRemover;

	delete mParticipants;
	mParticipants = NULL;

	mAvatarListRefreshConnection.disconnect();
	mVoiceChannelStateChangeConnection.disconnect();

	if(LLVoiceClient::instanceExists())
	{
		LLVoiceClient::getInstance()->removeObserver(this);
	}
	LLTransientFloaterMgr::getInstance()->removeControlView(this);
}

// virtual
BOOL LLCallFloater::postBuild()
{
	mAvatarList = getChild<LLAvatarList>("speakers_list");
	mAvatarListRefreshConnection = mAvatarList->setRefreshCompleteCallback(boost::bind(&LLCallFloater::onAvatarListRefreshed, this));

	childSetAction("leave_call_btn", boost::bind(&LLCallFloater::leaveCall, this));

	mNonAvatarCaller = findChild<LLNonAvatarCaller>("non_avatar_caller");
	mNonAvatarCaller->setVisible(FALSE);

	initAgentData();

	connectToChannel(LLVoiceChannel::getCurrentVoiceChannel());

	updateTransparency(TT_ACTIVE); // force using active floater transparency (STORM-730)
	
	updateSession();
	return TRUE;
}

// virtual
void LLCallFloater::onOpen(const LLSD& /*key*/)
{
	LLFirstUse::speak(false);
}

// virtual
void LLCallFloater::draw()
{
	// we have to refresh participants to display ones not in voice as disabled.
	// It should be done only when she joins or leaves voice chat.
	// But seems that LLVoiceClientParticipantObserver is not enough to satisfy this requirement.
	// *TODO: mantipov: remove from draw()

	// NOTE: it looks like calling onChange() here is not necessary,
	// but sometime it is not called properly from the observable object.
	// Seems this is a problem somewhere in Voice Client (LLVoiceClient::participantAddedEvent)
//	onChange();

	bool is_moderator_muted = LLVoiceClient::getInstance()->getIsModeratorMuted(gAgentID);

	if (mIsModeratorMutedVoice != is_moderator_muted)
	{
		setModeratorMutedVoice(is_moderator_muted);
	}

	// Need to resort the participant list if it's in sort by recent speaker order.
	if (mParticipants)
		mParticipants->updateRecentSpeakersOrder();

	LLFloater::draw();
}

// virtual
void LLCallFloater::setFocus( BOOL b )
{
	LLFloater::setFocus(b);

	// Force using active floater transparency (STORM-730).
	// We have to override setFocus() for LLCallFloater because selecting an item
	// of the voice morphing combobox causes the floater to lose focus and thus become transparent.
	updateTransparency(TT_ACTIVE);
}

// virtual
void LLCallFloater::onParticipantsChanged()
{
	if (NULL == mParticipants) return;
	updateParticipantsVoiceState();

	// Add newly joined participants.
	uuid_vec_t speakers_uuids;
	get_voice_participants_uuids(speakers_uuids);
	for (uuid_vec_t::const_iterator it = speakers_uuids.begin(); it != speakers_uuids.end(); it++)
	{
		mParticipants->addAvatarIDExceptAgent(*it);
	}
}

//////////////////////////////////////////////////////////////////////////
/// PRIVATE SECTION
//////////////////////////////////////////////////////////////////////////

void LLCallFloater::leaveCall()
{
	LLVoiceChannel* voice_channel = LLVoiceChannel::getCurrentVoiceChannel();
	if (voice_channel)
	{
		gIMMgr->endCall(voice_channel->getSessionID());
	}
}

void LLCallFloater::updateSession()
{
	LLVoiceChannel* voice_channel = LLVoiceChannel::getCurrentVoiceChannel();
	if (voice_channel)
	{
		LL_DEBUGS("Voice") << "Current voice channel: " << voice_channel->getSessionID() << LL_ENDL;

		if (mSpeakerManager && voice_channel->getSessionID() == mSpeakerManager->getSessionID())
		{
			LL_DEBUGS("Voice") << "Speaker manager is already set for session: " << voice_channel->getSessionID() << LL_ENDL;
			return;
		}
		else
		{
			mSpeakerManager = NULL;
		}
	}

	const LLUUID& session_id = voice_channel ? voice_channel->getSessionID() : LLUUID::null;

	LLIMModel::LLIMSession* im_session = LLIMModel::getInstance()->findIMSession(session_id);
	if (im_session)
	{
		mSpeakerManager = LLIMModel::getInstance()->getSpeakerManager(session_id);
		switch (im_session->mType)
		{
		case IM_NOTHING_SPECIAL:
		case IM_SESSION_P2P_INVITE:
			mVoiceType = VC_PEER_TO_PEER;

			if (!im_session->mOtherParticipantIsAvatar)
			{
				mVoiceType = VC_PEER_TO_PEER_AVALINE;
			}
			break;
		case IM_SESSION_CONFERENCE_START:
		case IM_SESSION_GROUP_START:
		case IM_SESSION_INVITE:
			if (gAgent.isInGroup(session_id))
			{
				mVoiceType = VC_GROUP_CHAT;
			}
			else
			{
				mVoiceType = VC_AD_HOC_CHAT;				
			}
			break;
		default:
			llwarning("Failed to determine voice call IM type", 0);
			mVoiceType = VC_GROUP_CHAT;
			break;
		}
	}

	if (NULL == mSpeakerManager)
	{
		// By default show nearby chat participants
		mSpeakerManager = LLLocalSpeakerMgr::getInstance();
		LL_DEBUGS("Voice") << "Set DEFAULT speaker manager" << LL_ENDL;
		mVoiceType = VC_LOCAL_CHAT;
	}

	updateTitle();

	// Hide "Leave Call" button for nearby chat
	bool is_local_chat = mVoiceType == VC_LOCAL_CHAT;
	getChildView("leave_call_btn_panel")->setVisible( !is_local_chat);

	refreshParticipantList();
	updateAgentModeratorState();

	// Show floater for voice calls & only in CONNECTED to voice channel state
	if (!is_local_chat &&
	    voice_channel &&
	    LLVoiceChannel::STATE_CONNECTED == voice_channel->getState())
	{
		LLIMFloater* im_floater = LLIMFloater::findInstance(session_id);
		bool show_me = !(im_floater && im_floater->getVisible());
		if (show_me) 
		{
			setVisible(true);
		}
	}
}

void LLCallFloater::refreshParticipantList()
{
	bool non_avatar_caller = VC_PEER_TO_PEER_AVALINE == mVoiceType;

	if (non_avatar_caller)
	{
		LLIMModel::LLIMSession* session = LLIMModel::instance().findIMSession(mSpeakerManager->getSessionID());
		mNonAvatarCaller->setSpeakerId(session->mOtherParticipantID);
		mNonAvatarCaller->setName(session->mName);
	}

	mNonAvatarCaller->setVisible(non_avatar_caller);
	mAvatarList->setVisible(!non_avatar_caller);

	if (!non_avatar_caller)
	{
		mParticipants = new LLParticipantList(mSpeakerManager, mAvatarList, true, mVoiceType != VC_GROUP_CHAT && mVoiceType != VC_AD_HOC_CHAT, false);
		mParticipants->setValidateSpeakerCallback(boost::bind(&LLCallFloater::validateSpeaker, this, _1));
		const U32 speaker_sort_order = gSavedSettings.getU32("SpeakerParticipantDefaultOrder");
		mParticipants->setSortOrder(LLParticipantList::EParticipantSortOrder(speaker_sort_order));
		
		if (LLLocalSpeakerMgr::getInstance() == mSpeakerManager)
		{
			mAvatarList->setNoItemsCommentText(getString("no_one_near"));
		}

		// we have to made delayed initialization of voice state of participant list.
		// it will be performed after first LLAvatarList refreshing in the onAvatarListRefreshed().
		mInitParticipantsVoiceState = true;
	}
}

void LLCallFloater::onAvatarListRefreshed()
{
	if (mInitParticipantsVoiceState)
	{
		initParticipantsVoiceState();
		mInitParticipantsVoiceState = false;
	}
	else
	{
		updateParticipantsVoiceState();
	}
}

// static
void LLCallFloater::sOnCurrentChannelChanged(const LLUUID& /*session_id*/)
{
	LLVoiceChannel* channel = LLVoiceChannel::getCurrentVoiceChannel();

	// *NOTE: if signal was sent for voice channel with LLVoiceChannel::STATE_NO_CHANNEL_INFO
	// it sill be sent for the same channel again (when state is changed).
	// So, lets ignore this call.
	if (channel == sCurrentVoiceChannel) return;

	LLCallFloater* call_floater = LLFloaterReg::getTypedInstance<LLCallFloater>("voice_controls");

	call_floater->connectToChannel(channel);
}

void LLCallFloater::onAvatarNameCache(const LLUUID& agent_id,
									  const LLAvatarName& av_name)
{
	LLStringUtil::format_map_t args;
	args["[NAME]"] = av_name.getCompleteName();
	std::string title = getString("title_peer_2_peer", args);
	setTitle(title);
}

void LLCallFloater::updateTitle()
{
	LLVoiceChannel* voice_channel = LLVoiceChannel::getCurrentVoiceChannel();
	if (mVoiceType == VC_PEER_TO_PEER)
	{
		LLUUID session_id = voice_channel->getSessionID();
		LLIMModel::LLIMSession* im_session =
			LLIMModel::getInstance()->findIMSession(session_id);
		if (im_session)
		{
			LLAvatarNameCache::get(im_session->mOtherParticipantID,
				boost::bind(&LLCallFloater::onAvatarNameCache,
					this, _1, _2));
			return;
		}
	}
	std::string title;
	switch (mVoiceType)
	{
	case VC_LOCAL_CHAT:
		title = getString("title_nearby");
		break;
	case VC_PEER_TO_PEER:
	case VC_PEER_TO_PEER_AVALINE:
		{
			title = voice_channel->getSessionName();

			if (VC_PEER_TO_PEER_AVALINE == mVoiceType)
			{
				title = LLTextUtil::formatPhoneNumber(title);
			}

			LLStringUtil::format_map_t args;
			args["[NAME]"] = title;
			title = getString("title_peer_2_peer", args);
		}
		break;
	case VC_AD_HOC_CHAT:
		title = getString("title_adhoc");
		break;
	case VC_GROUP_CHAT:
		{
			LLStringUtil::format_map_t args;
			args["[GROUP]"] = voice_channel->getSessionName();
			title = getString("title_group", args);
		}
		break;
	}

	setTitle(title);
}

void LLCallFloater::initAgentData()
{
	mAgentPanel = getChild<LLPanel> ("my_panel");

	if ( mAgentPanel )
	{
		mAgentPanel->getChild<LLUICtrl>("user_icon")->setValue(gAgentID);

		// Just use display name, because it's you
		LLAvatarName av_name;
		LLAvatarNameCache::get( gAgentID, &av_name );
		mAgentPanel->getChild<LLUICtrl>("user_text")->setValue(av_name.mDisplayName);

		mSpeakingIndicator = mAgentPanel->getChild<LLOutputMonitorCtrl>("speaking_indicator");
		mSpeakingIndicator->setSpeakerId(gAgentID);
	}
}

void LLCallFloater::setModeratorMutedVoice(bool moderator_muted)
{
	mIsModeratorMutedVoice = moderator_muted;

	if (moderator_muted)
	{
		LLNotificationsUtil::add("VoiceIsMutedByModerator");
	}
	mSpeakingIndicator->setIsMuted(moderator_muted);
}

void LLCallFloater::onModeratorNameCache(const LLAvatarName& av_name)
{
	std::string name;
	name = av_name.mDisplayName;

	if(mSpeakerManager && gAgent.isInGroup(mSpeakerManager->getSessionID()))
	{
		// This method can be called when LLVoiceChannel.mState == STATE_NO_CHANNEL_INFO
		// in this case there are not any speakers yet.
		if (mSpeakerManager->findSpeaker(gAgentID))
		{
			// Agent is Moderator
			if (mSpeakerManager->findSpeaker(gAgentID)->mIsModerator)

			{
				const std::string moderator_indicator(LLTrans::getString("IM_moderator_label")); 
				name += " " + moderator_indicator;
			}
		}
	}
	mAgentPanel->getChild<LLUICtrl>("user_text")->setValue(name);
}

void LLCallFloater::updateAgentModeratorState()
{
	LLAvatarNameCache::get(gAgentID, boost::bind(&LLCallFloater::onModeratorNameCache, this, _2));
}

static void get_voice_participants_uuids(uuid_vec_t& speakers_uuids)
{
	// Get a list of participants from VoiceClient
       std::set<LLUUID> participants;
       LLVoiceClient::getInstance()->getParticipantList(participants);
	
	for (std::set<LLUUID>::const_iterator iter = participants.begin();
		 iter != participants.end(); ++iter)
	{
		speakers_uuids.push_back(*iter);
	}

}

void LLCallFloater::initParticipantsVoiceState()
{
	// Set initial status for each participant in the list.
	std::vector<LLPanel*> items;
	mAvatarList->getItems(items);
	std::vector<LLPanel*>::const_iterator
		it = items.begin(),
		it_end = items.end();


	uuid_vec_t speakers_uuids;
	get_voice_participants_uuids(speakers_uuids);

	for(; it != it_end; ++it)
	{
		LLAvatarListItem *item = dynamic_cast<LLAvatarListItem*>(*it);
		
		if (!item)	continue;
		
		LLUUID speaker_id = item->getAvatarId();

		uuid_vec_t::const_iterator speaker_iter = std::find(speakers_uuids.begin(), speakers_uuids.end(), speaker_id);

		// If an avatarID assigned to a panel is found in a speakers list
		// obtained from VoiceClient we assign the JOINED status to the owner
		// of this avatarID.
		if (speaker_iter != speakers_uuids.end())
		{
			setState(item, STATE_JOINED);
		}
		else
		{
			LLPointer<LLSpeaker> speakerp = mSpeakerManager->findSpeaker(speaker_id);
			// If someone has already left the call before, we create his
			// avatar row panel with HAS_LEFT status and remove it after
			// the timeout, otherwise we create a panel with INVITED status
			if (speakerp.notNull() && speakerp.get()->mHasLeftCurrentCall)
			{
				setState(item, STATE_LEFT);
			}
			else
			{
				setState(item, STATE_INVITED);
			}
		}
	}
}

void LLCallFloater::updateParticipantsVoiceState()
{
	uuid_vec_t speakers_list;

	// Get a list of participants from VoiceClient
	uuid_vec_t speakers_uuids;
	get_voice_participants_uuids(speakers_uuids);

	// Updating the status for each participant already in list.
	std::vector<LLPanel*> items;
	mAvatarList->getItems(items);
	std::vector<LLPanel*>::const_iterator
		it = items.begin(),
		it_end = items.end();

	for(; it != it_end; ++it)
	{
		LLAvatarListItem *item = dynamic_cast<LLAvatarListItem*>(*it);
		if (!item) continue;

		const LLUUID participant_id = item->getAvatarId();
		bool found = false;

		uuid_vec_t::iterator speakers_iter = std::find(speakers_uuids.begin(), speakers_uuids.end(), participant_id);

		LL_DEBUGS("Voice") << "processing speaker: " << item->getAvatarName() << ", " << item->getAvatarId() << LL_ENDL;

		// If an avatarID assigned to a panel is found in a speakers list
		// obtained from VoiceClient we assign the JOINED status to the owner
		// of this avatarID.
		if (speakers_iter != speakers_uuids.end())
		{
			setState(item, STATE_JOINED);

			LLPointer<LLSpeaker> speaker = mSpeakerManager->findSpeaker(participant_id);
			if (speaker.isNull())
				continue;
			speaker->mHasLeftCurrentCall = FALSE;

			speakers_uuids.erase(speakers_iter);
			found = true;
		}

		if (!found)
		{
			updateNotInVoiceParticipantState(item);
		}
	}
}

void LLCallFloater::updateNotInVoiceParticipantState(LLAvatarListItem* item)
{
	LLUUID participant_id = item->getAvatarId();
	ESpeakerState current_state = getState(participant_id);

	switch (current_state)
	{
	case STATE_JOINED:
		// If an avatarID is not found in a speakers list from VoiceClient and
		// a panel with this ID has a JOINED status this means that this person
		// HAS LEFT the call.
		setState(item, STATE_LEFT);

		{
			LLPointer<LLSpeaker> speaker = mSpeakerManager->findSpeaker(participant_id);
			if (speaker.notNull())
			{
				speaker->mHasLeftCurrentCall = TRUE;
			}
		}
		break;
	case STATE_LEFT:
		// nothing to do. These states should not be changed.
		break;
	case STATE_INVITED:
		// If avatar was invited into group chat and went offline it is still exists in mSpeakerStateMap
		// If it goes online it will be rendered as JOINED via LAvatarListItem.
		// Lets update its visual representation. See EXT-6660
	case STATE_UNKNOWN:
		// If an avatarID is not found in a speakers list from VoiceClient and
		// a panel with this ID has an UNKNOWN status this means that this person
		// HAS ENTERED session but it is not in voice chat yet. So, set INVITED status
		setState(item, STATE_INVITED);
		break;
	default:
		// for possible new future states.
		llwarns << "Unsupported (" << getState(participant_id) << ") state for: " << item->getAvatarName()  << llendl;
		break;
	}
}

void LLCallFloater::setState(LLAvatarListItem* item, ESpeakerState state)
{
	// *HACK: mantipov: sometimes such situation is possible while switching to voice channel:
/*
	- voice channel is switched to the one user is joining
	- participant list is initialized with voice states: agent is in voice
	- than such log messages were found (with agent UUID)
			- LLVivoxProtocolParser::process_impl: parsing: <Response requestId="22" action="Session.MediaDisconnect.1"><ReturnCode>0</ReturnCode><Results><StatusCode>0</StatusCode><StatusString /></Results><InputXml><Request requestId="22" action="Session.MediaDisconnect.1"><SessionGroupHandle>9</SessionGroupHandle><SessionHandle>12</SessionHandle><Media>Audio</Media></Request></InputXml></Response>
			- LLVoiceClient::sessionState::removeParticipant: participant "sip:x2pwNkMbpR_mK4rtB_awASA==@bhr.vivox.com" (da9c0d90-c6e9-47f9-8ae2-bb41fdac0048) removed.
	- and than while updating participants voice states agent is marked as HAS LEFT
	- next updating of LLVoiceClient state makes agent JOINED
	So, lets skip HAS LEFT state for agent's avatar
*/
	if (STATE_LEFT == state && item->getAvatarId() == gAgentID) return;

	setState(item->getAvatarId(), state);

	switch (state)
	{
	case STATE_INVITED:
		item->setState(LLAvatarListItem::IS_VOICE_INVITED);
		break;
	case STATE_JOINED:
		removeVoiceRemoveTimer(item->getAvatarId());
		item->setState(LLAvatarListItem::IS_VOICE_JOINED);
		break;
	case STATE_LEFT:
		{
			setVoiceRemoveTimer(item->getAvatarId());
			item->setState(LLAvatarListItem::IS_VOICE_LEFT);
		}
		break;
	default:
		llwarns << "Unrecognized avatar panel state (" << state << ")" << llendl;
		break;
	}
}

void LLCallFloater::setVoiceRemoveTimer(const LLUUID& voice_speaker_id)
{
	mSpeakerDelayRemover->setActionTimer(voice_speaker_id);
}

bool LLCallFloater::removeVoiceLeftParticipant(const LLUUID& voice_speaker_id)
{
	uuid_vec_t& speaker_uuids = mAvatarList->getIDs();
	uuid_vec_t::iterator pos = std::find(speaker_uuids.begin(), speaker_uuids.end(), voice_speaker_id);
	if(pos != speaker_uuids.end())
	{
		speaker_uuids.erase(pos);
		mAvatarList->setDirty();
	}

	return false;
}


void LLCallFloater::resetVoiceRemoveTimers()
{
	mSpeakerDelayRemover->removeAllTimers();
}

void LLCallFloater::removeVoiceRemoveTimer(const LLUUID& voice_speaker_id)
{
	mSpeakerDelayRemover->unsetActionTimer(voice_speaker_id);
}

bool LLCallFloater::validateSpeaker(const LLUUID& speaker_id)
{
	bool is_valid = true;
	switch (mVoiceType)
	{
	case  VC_LOCAL_CHAT:
		{
			// A nearby chat speaker is considered valid it it's known to LLVoiceClient (i.e. has enabled voice).
			uuid_vec_t speakers;
			get_voice_participants_uuids(speakers);
			is_valid = std::find(speakers.begin(), speakers.end(), speaker_id) != speakers.end();
		}
		break;
	case VC_GROUP_CHAT:
		// if participant had left this call before do not allow add her again. See EXT-4216.
		// but if she Join she will be added into the list from the LLCallFloater::onChange()
		is_valid = STATE_LEFT != getState(speaker_id);
		break;
	default:
		// do nothing. required for Linux build
		break;
	}

	return is_valid;
}

void LLCallFloater::connectToChannel(LLVoiceChannel* channel)
{
	mVoiceChannelStateChangeConnection.disconnect();

	sCurrentVoiceChannel = channel;

	mVoiceChannelStateChangeConnection = sCurrentVoiceChannel->setStateChangedCallback(boost::bind(&LLCallFloater::onVoiceChannelStateChanged, this, _1, _2));

	updateState(channel->getState());
}

void LLCallFloater::onVoiceChannelStateChanged(const LLVoiceChannel::EState& old_state, const LLVoiceChannel::EState& new_state)
{
	// check is voice operational and if it doesn't work hide VCP (EXT-4397)
	if(LLVoiceClient::getInstance()->voiceEnabled() && LLVoiceClient::getInstance()->isVoiceWorking())
	{
		updateState(new_state);
	}
	else
	{
		closeFloater();
	}
}

void LLCallFloater::updateState(const LLVoiceChannel::EState& new_state)
{
	LL_DEBUGS("Voice") << "Updating state: " << new_state << ", session name: " << sCurrentVoiceChannel->getSessionName() << LL_ENDL;
	if (LLVoiceChannel::STATE_CONNECTED == new_state)
	{
		updateSession();
	}
	else
	{
		reset(new_state);
	}
}

void LLCallFloater::reset(const LLVoiceChannel::EState& new_state)
{
	// lets forget states from the previous session
	// for timers...
	resetVoiceRemoveTimers();

	// ...and for speaker state
	mSpeakerStateMap.clear();

	delete mParticipants;
	mParticipants = NULL;
	mAvatarList->clear();

	// These ifs were added instead of simply showing "loading" to make VCP work correctly in parcels
	// with disabled voice (EXT-4648 and EXT-4649)
	if (!LLViewerParcelMgr::getInstance()->allowAgentVoice() && LLVoiceChannel::STATE_HUNG_UP == new_state)
	{
		// hides "Leave Call" when call is ended in parcel with disabled voice- hiding usually happens in
		// updateSession() which won't be called here because connect to nearby voice never happens 
		getChildView("leave_call_btn_panel")->setVisible( false);
		// setting title to nearby chat an "no one near..." text- because in region with disabled
		// voice we won't have chance to really connect to nearby, so VCP is changed here manually
		setTitle(getString("title_nearby"));
		mAvatarList->setNoItemsCommentText(getString("no_one_near"));
	}
	// "loading" is shown  only when state is "ringing" to avoid showing it in nearby chat vcp
	// of parcels with disabled voice all the time- "no_one_near" is now shown there (EXT-4648)
	else if (new_state == LLVoiceChannel::STATE_RINGING)
	{
		// update floater to show Loading while waiting for data.
		mAvatarList->setNoItemsCommentText(LLTrans::getString("LoadingData"));
	}

	mAvatarList->setVisible(TRUE);
	mNonAvatarCaller->setVisible(FALSE);

	mSpeakerManager = NULL;
}

//EOF
